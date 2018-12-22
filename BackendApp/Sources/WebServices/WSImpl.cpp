/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Cryptography/Digest/Algorithm/MD5.h"
#include "Stroika/Foundation/Cryptography/Format.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/LinkMonitor.h"

#include "../Discovery/Devices.h"
#include "../Discovery/NetworkInterfaces.h"
#include "../Discovery/Networks.h"

#include "WSImpl.h"

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Execution;

using Stroika::Foundation::Common::GUID;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;

namespace {
    shared_ptr<Discovery::DeviceDiscoverer> GetDiscoverer_ ()
    {
        using Discovery::DeviceDiscoverer;
        using Discovery::Network;
        static Synchronized<Mapping<Network, shared_ptr<DeviceDiscoverer>>> sDiscovery_{
            Stroika::Foundation::Common::DeclareEqualsComparer ([](Network l, Network r) { return l.fGUID == r.fGUID; }),
        };

        Sequence<Discovery::Network> tmp = Discovery::CollectActiveNetworks ();

        if (tmp.empty ()) {
            Execution::Throw (Execution::StringException (L"no active network"));
        }
        Discovery::Network net = tmp[0];

        auto l = sDiscovery_.rwget ();
        if (auto i = l->Lookup (net)) {
            return *i;
        }
        auto r = make_shared<Discovery::DeviceDiscoverer> (net);
        l->Add (net, r);
        return r;
    }
}

namespace {
    // @todo LIKE WITH NETWORK IDS - probably maintain a persistence cache mapping info - mostly HARDWARE ADDRESS - to a uniuque nummber (guidgen maybe).
    // THEN we will always identify a device as the sam thing even if it appears with diferent IP address on different network
    //
    // must be careful about virtual devices (like VMs) which use fake hardware addresses, so need some way to tell differnt devices (and then one from another?)
    //
    //tmphack
    GUID LookupPersistentDeviceID_ (const Discovery::Device& d)
    {
        using IO::Network::InternetAddress;
        SortedSet<InternetAddress> x{d.ipAddresses};
        StringBuilder              sb;
        if (not x.empty ()) {
            sb += x.Nth (0).As<String> ();
        }
        sb += d.name;
        using namespace Cryptography::Digest;
        return Cryptography::Format<GUID> (Digester<Algorithm::MD5>::ComputeDigest (Memory::BLOB::Raw (sb.str ().AsUTF8 ())));
    }
}

/*
 ********************************************************************************
 ************************************* WSImpl ***********************************
 ********************************************************************************
 */
Collection<String> WSImpl::GetDevices () const
{
    Collection<String> result;
    for (BackendApp::WebServices::Device n : GetDevices_Recurse ()) {
        result += Characters::ToString (n.fGUID);
    }
    return result;
}

Collection<BackendApp::WebServices::Device> WSImpl::GetDevices_Recurse () const
{
    using namespace IO::Network;

    Collection<BackendApp::WebServices::Device> devices = GetDiscoverer_ ()->GetActiveDevices ().Select<BackendApp::WebServices::Device> ([](const Discovery::Device& d) {
        BackendApp::WebServices::Device newDev;
        d.ipAddresses.Apply ([&](const InternetAddress& a) {
            // prefer having IPv4 addr at head of list
            //
            //@todo - CRAP CODE - RETHINK!!!
            String addrStr = a.As<String> ();
            if (not newDev.ipAddresses.Contains (addrStr)) {
                if (auto o = a.AsAddressFamily (InternetAddress::AddressFamily::V4)) {
                    if (newDev.ipAddresses.Contains (o->As<String> ())) {
                        newDev.ipAddresses.Remove (*newDev.ipAddresses.IndexOf (o->As<String> ()));
                    }
                    newDev.ipAddresses.Prepend (o->As<String> ());
                }
                if (not newDev.ipAddresses.Contains (addrStr)) {
                    newDev.ipAddresses.Append (addrStr);
                }
                if (auto o = a.AsAddressFamily (InternetAddress::AddressFamily::V6)) {
                    if (not newDev.ipAddresses.Contains (o->As<String> ())) {
                        newDev.ipAddresses.Append (o->As<String> ());
                    }
                }
            }
        });

        newDev.fGUID = LookupPersistentDeviceID_ (d);
        newDev.name  = d.name;
        newDev.type  = d.type;
        newDev.fAttachedNetworks += d.fNetwork;
        newDev.fAttachedNetworkInterfaces = d.fAttachedInterfaces; // @todo must merge += (but only when merging across differnt discoverers/networks)
        newDev.important                  = newDev.type == Device::DeviceType::eRouter or d.fThisDevice;
        return newDev;
    });
    return devices;
}

Device WSImpl::GetDevice (const String& id) const
{
    // @todo quick hack impl
    for (auto i : GetDevices_Recurse ()) {
        if (i.fGUID == GUID{id}) {
            return i;
        }
    }
    Execution::Throw (Execution::StringException (L"no such id"));
}

Sequence<String> WSImpl::GetNetworks () const
{
    Sequence<String> result;
    for (Discovery::Network n : Discovery::CollectActiveNetworks ()) {
        result += Characters::ToString (n.fGUID);
    }
    return result;
}

Sequence<BackendApp::WebServices::Network> WSImpl::GetNetworks_Recurse () const
{
    Sequence<BackendApp::WebServices::Network> result;

    // @todo parameterize if we return all or just active networks
    for (Discovery::Network n : Discovery::CollectActiveNetworks ()) {
        BackendApp::WebServices::Network nw{n.fNetworkAddresses};

        nw.fGUID                    = n.fGUID;
        nw.fFriendlyName            = n.fFriendlyName;
        nw.fNetworkAddresses        = n.fNetworkAddresses;
        nw.fAttachedInterfaces      = n.fAttachedNetworkInterfaces;
        nw.fDNSServers              = n.fDNSServers;
        nw.fGateways                = n.fGateways;
        nw.fExternalAddresses       = n.fExternalAddresses;
        nw.fGEOLocInformation       = n.fGEOLocInfo;
        nw.fInternetServiceProvider = n.fISP;

        result += nw;
    }
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace (L"returns: %s", Characters::ToString (result).c_str ());
#endif
    return result;
}

Network WSImpl::GetNetwork (const String& id) const
{
    // @todo quick hack impl
    for (auto i : GetNetworks_Recurse ()) {
        if (i.fGUID == GUID{id}) {
            return i;
        }
    }
    Execution::Throw (Execution::StringException (L"no such id"));
}

Collection<String> WSImpl::GetNetworkInterfaces (bool filterRunningOnly) const
{
    Collection<String> result;

    for (Discovery::NetworkInterface n : Discovery::CollectAllNetworkInterfaces ()) {
        bool passedFilter{true};
        if (filterRunningOnly) {
            if (n.fStatus.has_value () and not n.fStatus->Contains (IO::Network::Interface::Status::eConnected)) {
                passedFilter = false;
            }
        }
        if (passedFilter) {
            result += Characters::ToString (n.fGUID);
        }
    }
    return result;
}

Collection<BackendApp::WebServices::NetworkInterface> WSImpl::GetNetworkInterfaces_Recurse (bool filterRunningOnly) const
{
    Collection<BackendApp::WebServices::NetworkInterface> result;

    for (Discovery::NetworkInterface n : Discovery::CollectAllNetworkInterfaces ()) {
        bool passedFilter{true};
        if (filterRunningOnly) {
            if (n.fStatus.has_value () and not n.fStatus->Contains (IO::Network::Interface::Status::eConnected)) {
                passedFilter = false;
            }
        }
        if (passedFilter) {
            BackendApp::WebServices::NetworkInterface nw{n};

            /**
             */
            nw.fGUID = n.fGUID;

            result += nw;
        }
    }
    return result;
}

NetworkInterface WSImpl::GetNetworkInterface (const String& id) const
{
    // @todo quick hack impl
    for (auto i : GetNetworkInterfaces_Recurse (false)) {
        if (i.fGUID == GUID{id}) {
            return i;
        }
    }
    Execution::Throw (Execution::StringException (L"no such id"));
}

/*
 ********************************************************************************
 **************** WebServices::TmpHackAssureStartedMonitoring *******************
 ********************************************************************************
 */
void WebServices::TmpHackAssureStartedMonitoring ()
{
    GetDiscoverer_ ();
}
