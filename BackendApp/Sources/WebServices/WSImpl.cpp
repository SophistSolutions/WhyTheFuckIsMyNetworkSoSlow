/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/LinkMonitor.h"

#include "../Discovery/Devices.h"
#include "../Discovery/Networks.h"

#include "WSImpl.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Execution;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;

namespace {
    shared_ptr<Discovery::DeviceDiscoverer> GetDiscoverer_ ()
    {
        static Synchronized<Mapping<Discovery::Network, shared_ptr<Discovery::DeviceDiscoverer>>> sDiscovery_;

        Collection<Discovery::Network> tmp = Discovery::CollectActiveNetworks ();

        if (tmp.empty ()) {
            Execution::Throw (Execution::StringException (L"no active network"));
        }
        Discovery::Network net = tmp.Nth (0);

        auto l = sDiscovery_.rwget ();
        if (auto i = l->Lookup (net)) {
            return *i;
        }
        auto r = make_shared<Discovery::DeviceDiscoverer> (net);
        l->Add (net, r);
        return r;
    }
}

Collection<BackendApp::WebServices::Device> WSImpl::GetDevices () const
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
        newDev.connected = true;
        newDev.name      = d.name;
        newDev.type      = d.type;
        newDev.important = newDev.type == Device::DeviceType::eRouter or d.fThisDevice;
        return newDev;
    });
    return devices;
}
