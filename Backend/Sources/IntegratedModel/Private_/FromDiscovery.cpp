/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Common/GUID.h"
#include "Stroika/Foundation/Common/KeyValuePair.h"
#include "Stroika/Foundation/Common/Property.h"
#include "Stroika/Foundation/Containers/KeyedCollection.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Execution/Logger.h"

#include "../../Common/BLOBMgr.h"
#include "../../Common/EthernetMACAddressOUIPrefixes.h"

#include "../../Discovery/Devices.h"
#include "../../Discovery/NetworkInterfaces.h"
#include "../../Discovery/Networks.h"

#include "FromDiscovery.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Common;
using namespace Stroika::Foundation::DataExchange;
using namespace Stroika::Foundation::Execution;
using namespace Stroika::Foundation::Memory;
using namespace Stroika::Foundation::IO::Network;
using namespace Stroika::Foundation::IO::Network::HTTP;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;

using Stroika::Foundation::Common::ConstantProperty;
using Stroika::Foundation::Common::GUID;

using WebServices::Model::Device;
using WebServices::Model::Network;
using WebServices::Model::NetworkAttachmentInfo;
using WebServices::Model::NetworkInterface;

namespace {
    URI TransformURL2LocalStorage_ (const URI& url)
    {
        Debug::TimingTrace ttrc{L"TransformURL2LocalStorage_", 100ms}; // sb very quick cuz we schedule url fetches for background

        // if we are unable to cache the url (say because the url is bad or the device is currently down)
        // just return the original
        try {
            // This BLOBMgr code wont block - it will push a request into a Q, and fetch whatever data is has (maybe none)
            optional<GUID> g = BackendApp::Common::BLOBMgr::sThe.AsyncAddBLOBFromURL (url);
            // tricky to know right host to supply here - will need some sort of configuration for
            // this (due to firewalls, NAT etc).
            // Use relative URL for now, as that should work for most cases
            if (g) {
                return URI{nullopt, nullopt, L"/api/v1/blob/" + g->ToString ()};
            }
        }
        catch (const std::system_error& e) {
            Logger::sThe.Log (Logger::eWarning, L"Database update: ignoring exception in TransformURL2LocalStorage_: %s",
                              Characters::ToString (e).c_str ());
            Assert (e.code () == errc::device_or_resource_busy); // this can happen talking to database (SQLITE_BUSY or SQLITE_LOCKED)
                                                                 // might be better to up timeout so more rare
        }
        catch (const Thread::AbortException&) {
            Execution::ReThrow ();
        }
        catch (...) {
            Logger::sThe.Log (Logger::eWarning, L"Database update: ignoring exception in TransformURL2LocalStorage_: %s",
                              Characters::ToString (current_exception ()).c_str ());
            AssertNotReached ();
        }
        DbgTrace (L"Failed to cache url (%s) - so returning original", Characters::ToString (url).c_str ());
        return url;
    }
    optional<URI> TransformURL2LocalStorage_ (const optional<URI>& url)
    {
        return url ? TransformURL2LocalStorage_ (*url) : optional<URI>{};
    }
}

namespace {

    Device Discovery2Model_ (const Discovery::Device& d)
    {
        Device newDev;
        newDev.fID    = d.fGUID;
        newDev.fNames = d.fNames;
        if (not d.fTypes.empty ()) {
            newDev.fTypes = d.fTypes; // leave missing if no discovered types
        }
        newDev.fSeen.fARP       = d.fSeen.fARP;
        newDev.fSeen.fCollector = d.fSeen.fCollector;
        newDev.fSeen.fICMP      = d.fSeen.fICMP;
        newDev.fSeen.fTCP       = d.fSeen.fTCP;
        newDev.fSeen.fUDP       = d.fSeen.fUDP;
        Assert (newDev.fSeen.EverSeen ()); // for now don't allow 'discovering' a device without having some initial data for some activity
        newDev.fOpenPorts = d.fOpenPorts;
        for (const auto& i : d.fAttachedNetworks) {
            constexpr bool            kIncludeLinkLocalAddresses_{Discovery::kIncludeLinkLocalAddressesInDiscovery};
            constexpr bool            kIncludeMulticastAddreses_{Discovery::kIncludeMulticastAddressesInDiscovery};
            Sequence<InternetAddress> addrs2Report;
            for (const auto& li : i.fValue.localAddresses) {
                if (not kIncludeLinkLocalAddresses_ and li.IsLinkLocalAddress ()) {
                    continue;
                }
                if (not kIncludeMulticastAddreses_ and li.IsMulticastAddress ()) {
                    continue;
                }
                addrs2Report += li;
            }
            newDev.fAttachedNetworks.Add (i.fKey, NetworkAttachmentInfo{i.fValue.hardwareAddresses, addrs2Report});
        }
        newDev.fAttachedNetworkInterfaces = d.fAttachedInterfaces; // @todo must merge += (but only when merging across differnt discoverers/networks)
        newDev.fPresentationURL = d.fPresentationURL;
        newDev.fManufacturer    = d.fManufacturer;
        newDev.fIcon            = TransformURL2LocalStorage_ (d.fIcon);
        newDev.fOperatingSystem = d.fOperatingSystem;
#if qDebug
        if (not d.fDebugProps.empty ()) {
            newDev.fDebugProps = d.fDebugProps;
        }
        {
            // List OUI names for each hardware address (and explicit missing for those we cannot lookup)
            using VariantValue = DataExchange::VariantValue;
            Mapping<String, VariantValue> t;
            for (const auto& i : d.fAttachedNetworks) {
                for (const auto& hwa : i.fValue.hardwareAddresses) {
                    auto o = BackendApp::Common::LookupEthernetMACAddressOUIFromPrefix (hwa);
                    t.Add (hwa, o ? VariantValue{*o} : VariantValue{});
                }
            }
            if (not t.empty ()) {
                if (not newDev.fDebugProps.has_value ()) {
                    newDev.fDebugProps = Mapping<String, VariantValue>{};
                }
                newDev.fDebugProps->Add (L"MACAddr2OUINames", VariantValue{t});
            }
        }
#endif
        Assert (newDev.fSeen.EverSeen ()); // maybe won't always require but look into any cases like this and probably remove them...
        return newDev;
    }
    Network Discovery2Model_ (const Discovery::Network& n)
    {
        Network nw{n.fNetworkAddresses};
        nw.fID                       = n.fGUID;
        nw.fNames                    = n.fNames;
        nw.fNetworkAddresses         = n.fNetworkAddresses;
        nw.fAttachedInterfaces       = n.fAttachedNetworkInterfaces;
        nw.fDNSServers               = n.fDNSServers;
        nw.fGateways                 = n.fGateways;
        nw.fGatewayHardwareAddresses = n.fGatewayHardwareAddresses;
        nw.fExternalAddresses        = n.fExternalAddresses;
        nw.fGEOLocInformation        = n.fGEOLocInfo;
        nw.fInternetServiceProvider  = n.fISP;
        nw.fSeen                     = n.fSeen;
#if qDebug
        if (not n.fDebugProps.empty ()) {
            nw.fDebugProps = n.fDebugProps;
        }
#endif
        return nw;
    }
    NetworkInterface Discovery2Model_ (const Discovery::NetworkInterface& n)
    {
        NetworkInterface nwi;
        nwi.fInternalInterfaceID = n.fInternalInterfaceID;
        nwi.fFriendlyName        = n.fFriendlyName;
        nwi.fDescription         = n.fDescription;
        //nwi.fNetworkGUID         = n.fNetworkGUID;    INTENTIONALLY OMITTED because doesn't correspond to our network ID, misleading, and unhelpful
        nwi.fType                 = n.fType;
        nwi.fHardwareAddress      = n.fHardwareAddress;
        nwi.fTransmitSpeedBaud    = n.fTransmitSpeedBaud;
        nwi.fReceiveLinkSpeedBaud = n.fReceiveLinkSpeedBaud;
        nwi.fWirelessInfo         = n.fWirelessInfo;
        nwi.fBindings             = n.fBindings;
        nwi.fGateways             = n.fGateways;
        nwi.fDNSServers           = n.fDNSServers;
        nwi.fStatus               = n.fStatus;
        nwi.fID                   = n.fGUID;
#if qDebug
        if (not n.fDebugProps.empty ()) {
            nwi.fDebugProps = n.fDebugProps;
        }
#endif
        return nwi;
    }

}

optional<GUID> IntegratedModel::Private_::FromDiscovery::GetMyDeviceID ()
{
    return Discovery::DevicesMgr::sThe.GetThisDeviceID ();
}

Sequence<NetworkInterface> IntegratedModel::Private_::FromDiscovery::GetNetworkInterfaces ()
{
    Debug::TimingTrace         ttrc{L"FromDiscovery::GetNetworkInterfaces_", 100ms};
    Sequence<NetworkInterface> result =
        Discovery::NetworkInterfacesMgr::sThe.CollectAllNetworkInterfaces ().Map< Sequence<NetworkInterface>> (
            [] (const Discovery::NetworkInterface& n) { return Discovery2Model_ (n); });
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace (L"returns: %s", Characters::ToString (result).c_str ());
#endif
    return result;
}

Sequence<Network> IntegratedModel::Private_::FromDiscovery::GetNetworks ()
{
    Debug::TimingTrace ttrc{L"FromDiscovery::GetNetworks_", 100ms};
    Sequence<Network>  result = Discovery::NetworksMgr::sThe.CollectActiveNetworks ().Map<Sequence<Network>> (
        [] (const Discovery::Network& n) { return Discovery2Model_ (n); });
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace (L"returns: %s", Characters::ToString (result).c_str ());
#endif
    return result;
}

Sequence<Device> IntegratedModel::Private_::FromDiscovery::GetDevices ()
{
    Debug::TimingTrace ttrc{L"FromDiscovery::GetDevices_", 100ms};
    // Fetch (UNSORTED) list of devices
    return Discovery::DevicesMgr::sThe.GetActiveDevices ().Map< Sequence<Device>> (
        [] (const Discovery::Device& d) { return Discovery2Model_ (d); });
}