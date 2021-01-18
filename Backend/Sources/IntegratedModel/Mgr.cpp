/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2020.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Debug/TimingTrace.h"

#include "../Common/BLOBMgr.h"
#include "../Common/EthernetMACAddressOUIPrefixes.h"

#include "../Discovery/Devices.h"
#include "../Discovery/NetworkInterfaces.h"
#include "../Discovery/Networks.h"

#include "Mgr.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Common;
using namespace Stroika::Foundation::Execution;
using namespace Stroika::Foundation::Memory;
using namespace Stroika::Foundation::IO::Network;
using namespace Stroika::Foundation::IO::Network::HTTP;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;

using Stroika::Foundation::Common::GUID;

namespace {
    URI TransformURL2LocalStorage_ (const URI& url)
    {
        Debug::TimingTrace ttrc{L"TransformURL2LocalStorage_", 0.1}; // sb very quick cuz we schedule url fetches for background

        // if we are unable to cache the url (say because the url is bad or the device is currently down)
        // just return the original

        try {
            // This BLOBMgr code wont block - it will push a request into a Q, and fetch whatever data is has (maybe none)
            optional<GUID> g = BackendApp::Common::BLOBMgr::sThe.AsyncAddBLOBFromURL (url);
            // tricky to know right host to supply here - will need some sort of configuration for
            // this (due to firewalls, NAT etc).
            // Use relative URL for now, as that should work for most cases
            if (g) {
                return URI{nullopt, nullopt, L"/blob/" + g->ToString ()};
            }
        }
        catch (...) {
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

/*
 ********************************************************************************
 ************************** IntegratedModel::Mgr::Activator *********************
 ********************************************************************************
 */
IntegratedModel::Mgr::Activator::Activator ()
{
}
IntegratedModel::Mgr::Activator::~Activator ()
{
}

/*
 ********************************************************************************
 ****************************** IntegratedModel::Mgr ****************************
 ********************************************************************************
 */
Sequence<IntegratedModel::Device> IntegratedModel::Mgr::GetDevices () const
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"IntegratedModel::Mgr::GetDevices")};
    Debug::TimingTrace        ttrc{L"IntegratedModel::Mgr::GetDevices", .1};

    // Fetch (UNSORTED) list of devices
    Sequence<Device> devices = Sequence<Device>{Discovery::DevicesMgr::sThe.GetActiveDevices ().Select<Device> ([] (const Discovery::Device& d) {
        Device newDev;
        newDev.fGUID = d.fGUID;
        newDev.name  = d.name;
        if (not d.fTypes.empty ()) {
            newDev.fTypes = d.fTypes; // leave missing if no discovered types
        }
        newDev.fLastSeenAt = d.fLastSeenAt;
        newDev.fOpenPorts  = d.fOpenPorts;
        for (auto i : d.fAttachedNetworks) {
            constexpr bool            kIncludeLinkLocalAddresses_{Discovery::kIncludeLinkLocalAddressesInDiscovery};
            constexpr bool            kIncludeMulticastAddreses_{Discovery::kIncludeMulticastAddressesInDiscovery};
            Sequence<InternetAddress> addrs2Report;
            for (auto li : i.fValue.localAddresses) {
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
        newDev.fPresentationURL           = d.fPresentationURL;
        newDev.fManufacturer              = d.fManufacturer;
        newDev.fIcon                      = TransformURL2LocalStorage_ (d.fIcon);
        newDev.fOperatingSystem           = d.fOperatingSystem;
#if qDebug
        if (not d.fDebugProps.empty ()) {
            newDev.fDebugProps = d.fDebugProps;
        }
        {
            // List OUI names for each hardware address (and explicit missing for those we cannot lookup)
            using VariantValue = DataExchange::VariantValue;
            Mapping<String, VariantValue> t;
            for (auto i : d.fAttachedNetworks) {
                for (auto hwa : i.fValue.hardwareAddresses) {
                    auto o = BackendApp::Common::LookupEthernetMACAddressOUIFromPrefix (hwa);
                    t.Add (hwa, o ? VariantValue{*o} : VariantValue{});
                }
            }
            if (not t.empty ()) {
                if (not newDev.fDebugProps.has_value ()) {
                    newDev.fDebugProps = Mapping<String, VariantValue>{};
                }
                newDev.fDebugProps->Add (L"MACAddr2OUINames", t);
            }
        }
#endif
        return newDev;
    })};
    return devices;
}

std::optional<IntegratedModel::Device> IntegratedModel::Mgr::GetDevice (const Common::GUID& id) const
{
    // @todo can make much faster
    for (auto i : GetDevices ()) {
        if (i.fGUID == id) {
            return i;
        }
    }
    return nullopt;
}

Sequence<IntegratedModel::Network> IntegratedModel::Mgr::GetNetworks () const
{
    Debug::TimingTrace ttrc{L"IntegratedModel::Mgr::GetNetworks", 0.1};
    Sequence<Network>  result;
    for (Discovery::Network n : Discovery::NetworksMgr::sThe.CollectActiveNetworks ()) {
        Network nw{n.fNetworkAddresses};
        nw.fGUID                    = n.fGUID;
        nw.fFriendlyName            = n.fFriendlyName;
        nw.fNetworkAddresses        = n.fNetworkAddresses;
        nw.fAttachedInterfaces      = n.fAttachedNetworkInterfaces;
        nw.fDNSServers              = n.fDNSServers;
        nw.fGateways                = n.fGateways;
        nw.fExternalAddresses       = n.fExternalAddresses;
        nw.fGEOLocInformation       = n.fGEOLocInfo;
        nw.fInternetServiceProvider = n.fISP;
#if qDebug
        if (not n.fDebugProps.empty ()) {
            nw.fDebugProps = n.fDebugProps;
        }
#endif
        result += nw;
    }
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace (L"returns: %s", Characters::ToString (result).c_str ());
#endif
    return result;
}

std::optional<IntegratedModel::Network> IntegratedModel::Mgr::GetNetwork (const Common::GUID& id) const
{
    // @todo can make much faster
    for (auto i : GetNetworks ()) {
        if (i.fGUID == id) {
            return i;
        }
    }
    return nullopt;
}

Collection<IntegratedModel::NetworkInterface> IntegratedModel::Mgr::GetNetworkInterfaces () const
{
    Collection<NetworkInterface> result;
    for (Discovery::NetworkInterface n : Discovery::NetworkInterfacesMgr::sThe.CollectAllNetworkInterfaces ()) {
        NetworkInterface nw{n};
        nw.fGUID = n.fGUID;
#if qDebug
        if (not n.fDebugProps.empty ()) {
            nw.fDebugProps = n.fDebugProps;
        }
#endif
        result += nw;
    }
    return result;
}

optional<IntegratedModel::NetworkInterface> IntegratedModel::Mgr::GetNetworkInterface (const Common::GUID& id) const
{
    for (auto i : GetNetworkInterfaces ()) {
        if (i.fGUID == id) {
            return i;
        }
    }
    return nullopt;
}