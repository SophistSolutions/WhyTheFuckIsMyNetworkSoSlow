/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Configuration/SystemConfiguration.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/DNS.h"
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/LinkMonitor.h"

#include "Stroika/Frameworks/UPnP/SSDP/Client/Listener.h"

#include "WSImpl.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Execution;
using namespace Stroika::Frameworks::UPnP;
using namespace Stroika::Frameworks::UPnP::SSDP;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;

namespace {
    //tmphack - this goes elsewhere - in module to do background scanning/DB of machiens

    struct DiscoveryInfo_ {
        IO::Network::InternetAddress fAddr;
        bool                         alive{};
        String                       server;
    };

    Collection<DiscoveryInfo_> GetSoFarDiscoveredDevices_ ()
    {
        static Synchronized<Mapping<String, DiscoveryInfo_>> sDiscoveredDevices_;
        static SSDP::Client::Listener sListener_{
            [&](const SSDP::Advertisement& d) {
                DbgTrace (L"Recieved SSDP advertisement: %s", Characters::ToString (d).c_str ());
                String                                   location = d.fLocation.GetFullURL ();
                Optional<bool>                           alive    = d.fAlive;
                URL                                      locURL   = URL{location, URL::ParseOptions::eAsFullURL};
                String                                   locHost  = locURL.GetHost ();
                Collection<IO::Network::InternetAddress> locAddrs = IO::Network::DNS::Default ().GetHostAddresses (locHost);
                if (locHost.empty ()) {
                    DbgTrace (L"oops - bad - probabkly should log - bad device resposne - find addr some other way");
                }
                else {
                    sDiscoveredDevices_.rwget ()->Add (location, DiscoveryInfo_{locAddrs.Nth (0), alive.Value (true), d.fServer});
                }
            },
            SSDP::Client::Listener::eAutoStart};
        return sDiscoveredDevices_.cget ()->MappedValues ();
    }
}

Collection<BackendApp::WebServices::Device> WSImpl::GetDevices () const
{
    static const Mapping<String, String> kNamePrettyPrintMapper_{
        Common::KeyValuePair<String, String>{L"ASUSTeK UPnP/1.1 MiniUPnPd/1.9", L"ASUS Router"},
        Common::KeyValuePair<String, String>{L"Microsoft-Windows/10.0 UPnP/1.0 UPnP-Device-Host/1.0", L"Antiphon"},
        Common::KeyValuePair<String, String>{L"POSIX, UPnP/1.0, Intel MicroStack/1.0.1347", L"HP PhotoSmart"},
    };

    using namespace IO::Network;
    static const Device kFakeDevicePrototype_ = Device{
        L"name",
        L"ipAddress",
        L"ipv4",
        L"ipv6",
        L"Phone",
        L"192.168.244.0/24",
        L"255.255.255.0",
        67,
        true,
        false};

    Collection<BackendApp::WebServices::Device> devices = GetSoFarDiscoveredDevices_ ().Select<Device> ([](const DiscoveryInfo_& d) {
        Device newDev    = kFakeDevicePrototype_;
        newDev.ipAddress = d.fAddr.As<String> ();
        if (auto o = d.fAddr.AsAddressFamily (InternetAddress::AddressFamily::V4)) {
            newDev.ipv4 = o->As<String> ();
        }
        if (auto o = d.fAddr.AsAddressFamily (InternetAddress::AddressFamily::V6)) {
            newDev.ipv6 = o->As<String> ();
        }
        newDev.connected = d.alive;
        newDev.name      = d.server;
        newDev.important = newDev.ipAddress.EndsWith (L".1"); //tmphack
        if (newDev.ipAddress.EndsWith (L".1")) {
            newDev.type = L"Router";
        }
        newDev.name = kNamePrettyPrintMapper_.LookupValue (newDev.name, newDev.name);
        return newDev;
    });

    InternetAddress thisMachineAddr = GetPrimaryInternetAddress ();

    Containers::Set<InternetAddress> found;
    for (IO::Network::Interface i : IO::Network::GetInterfaces ()) {
        if (i.fType != Interface::Type::eLoopback and i.fStatus and i.fStatus->Contains (Interface::Status::eRunning) and not i.fBindings.Contains (thisMachineAddr)) {
            Device newDev    = kFakeDevicePrototype_;
            newDev.ipAddress = thisMachineAddr.As<String> ();
            if (found.Contains (thisMachineAddr)) {
                continue;
            }
            else {
                found.Add (thisMachineAddr);
            }
            if (auto o = thisMachineAddr.AsAddressFamily (InternetAddress::AddressFamily::V4)) {
                newDev.ipv4 = o->As<String> ();
            }
            if (auto o = thisMachineAddr.AsAddressFamily (InternetAddress::AddressFamily::V6)) {
                newDev.ipv6 = o->As<String> ();
            }
            newDev.connected = true;
            newDev.name      = Configuration::GetSystemConfiguration_ComputerNames ().fHostname;
            newDev.important = true;
            newDev.type      = L"Laptop";
            newDev.name      = kNamePrettyPrintMapper_.LookupValue (newDev.name, newDev.name);
            devices.Add (newDev);
        }
    }
    return devices;
}
