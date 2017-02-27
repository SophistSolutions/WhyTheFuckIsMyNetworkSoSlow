/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Configuration/SystemConfiguration.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/DNS.h"
#include "Stroika/Foundation/IO/Network/Interface.h"

#include "Stroika/Frameworks/UPnP/SSDP/Client/Listener.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Execution;
using namespace Stroika::Frameworks::UPnP;
using namespace Stroika::Frameworks::UPnP::SSDP;

#include "WSImpl.h"

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
        static SSDP::Client::Listener sListener_; // @todo with move CTOR once we support that
#if 0
        static  SSDP::Client::Listener sListener_ = move ([]() {
            SSDP::Client::Listener l;
            return move (l);
        });
#endif
        //tmphack
        static bool sFirstTime_ = true;
        if (sFirstTime_) {
            sFirstTime_ = false;
            sListener_.AddOnFoundCallback ([&](const SSDP::Advertisement& d) {
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
            });
            sListener_.Start ();
        }
        return sDiscoveredDevices_.cget ()->MappedValues ();
    }
}

Collection<BackendApp::WebServices::Device> WSImpl::GetDevices () const
{
    static const Device kFakeDevicePrototype_ = Device{
        L"name",
        L"ipAddress",
        L"ipv4",
        L"ipv6",
        L"Phone",
        L"./images/phone.png",
        L"192.168.244.0/24",
        L"255.255.255.0",
        67,
        true,
        false};

    Collection<BackendApp::WebServices::Device> devices = GetSoFarDiscoveredDevices_ ().Select<Device> ([](const DiscoveryInfo_& d) {
        Device newDev    = kFakeDevicePrototype_;
        newDev.ipv4      = d.fAddr.As<String> ();
        newDev.ipAddress = d.fAddr.As<String> ();
        newDev.connected = d.alive;
        newDev.name      = d.server;
        newDev.important = newDev.ipAddress.EndsWith (L".1"); //tmphack
        return newDev;
    });
    for (IO::Network::Interface i : IO::Network::GetInterfaces ()) {
        if (i.fType != IO::Network::Interface::Type::eLoopback and i.fStatus and i.fStatus->Contains (IO::Network::Interface::Status::eRunning) and not i.fBindings.empty ()) {
            Device newDev    = kFakeDevicePrototype_;
            newDev.ipv4      = i.fBindings.First ()->As<String> ();
            newDev.ipAddress = newDev.ipv4;
            newDev.connected = true;
            newDev.name      = Configuration::GetSystemConfiguration_ComputerNames ().fHostname;
            newDev.important = true;
            devices.Add (newDev);
        }
    }
    return devices;

#if 0

    return Collection<Device>{
        Device{
            L"Robert's Phone",
            L"192.168.244.34",
            L"fe80::ec4:7aff:fec7:7f1c",
            L"Phone",
            L"./images/phone.png",
            L"192.168.244.0/24",
            L"255.255.255.0",
            67,
            true},
        Device{
            L"WAP - Main",
            L"192.168.244.87",
            L"fe80::ea3:5fef:fed7:98cc",
            L"WAP",
            L"./images/WAP.png",
            L"192.168.244.0/24",
            L"255.255.255.0",
            34,
            true},
    };
#endif
}
