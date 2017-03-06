/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Configuration/SystemConfiguration.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/DNS.h"
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/LinkMonitor.h"

#include "Stroika/Frameworks/UPnP/SSDP/Client/Listener.h"

#include "Devices.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::IO;
using namespace Stroika::Foundation::IO::Network;
using namespace Stroika::Frameworks;
using namespace Stroika::Frameworks::UPnP;

using Execution::Synchronized;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Discovery;

/*
 ********************************************************************************
 ******************************* Discovery::Device ******************************
 ********************************************************************************
 */
String Discovery::Device::ToString () const
{
    StringBuilder sb;
    sb += L"{";
    sb += L"name: " + Characters::ToString (name) + L", ";
    sb += L"ipAddress: " + Characters::ToString (ipAddress) + L", ";
    sb += L"type: " + Characters::ToString (type) + L", ";
    sb += L"}";
    return sb.str ();
}

/*
 ********************************************************************************
 ************************** Discovery::DeviceDiscoverer *************************
 ********************************************************************************
 */
class DeviceDiscoverer::Rep_ {
    struct DiscoveryInfo_ {
        IO::Network::InternetAddress fAddr;
        bool                         alive{};
        String                       server;
    };

    mutable Synchronized<Mapping<String, DiscoveryInfo_>> fDiscoveredDevices_;

public:
    Rep_ (const Network& forNetwork)
    {
    }
    Collection<Discovery::Device> GetActiveDevices () const
    {
        static const Mapping<String, String> kNamePrettyPrintMapper_{
            Common::KeyValuePair<String, String>{L"ASUSTeK UPnP/1.1 MiniUPnPd/1.9", L"ASUS Router"},
            Common::KeyValuePair<String, String>{L"Microsoft-Windows/10.0 UPnP/1.0 UPnP-Device-Host/1.0", L"Antiphon"},
            Common::KeyValuePair<String, String>{L"POSIX, UPnP/1.0, Intel MicroStack/1.0.1347", L"HP PhotoSmart"},
        };
        Collection<Discovery::Device> results;
        for (DiscoveryInfo_ di : GetSoFarDiscoveredDevices_ ()) {
            Discovery::Device newDev;

            newDev.name      = di.server;
            newDev.name      = kNamePrettyPrintMapper_.LookupValue (newDev.name, newDev.name);
            newDev.ipAddress = di.fAddr;
            newDev.type      = L"Unknown";

            if (newDev.ipAddress.As<String> ().EndsWith (L".1")) {
                newDev.type = L"Router";
            }

            results.Add (newDev);
        }
        if (auto i = GetMyDevice_ ()) {
            results.Add (*i);
        }
        return results;
    }

private:
    Collection<DiscoveryInfo_> GetSoFarDiscoveredDevices_ () const
    {
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
                    fDiscoveredDevices_.rwget ()->Add (location, DiscoveryInfo_{locAddrs.Nth (0), alive.Value (true), d.fServer});
                }
            },
            SSDP::Client::Listener::eAutoStart};
        return fDiscoveredDevices_.cget ()->MappedValues ();
    }
    Optional<Discovery::Device> GetMyDevice_ () const
    {
        InternetAddress thisMachineAddr = GetPrimaryInternetAddress ();

        Containers::Set<InternetAddress> found;
        for (IO::Network::Interface i : IO::Network::GetInterfaces ()) {
            if (i.fType != Interface::Type::eLoopback and i.fStatus and i.fStatus->Contains (Interface::Status::eRunning) and not i.fBindings.Contains (thisMachineAddr)) {
                Discovery::Device newDev;
                newDev.ipAddress = thisMachineAddr.As<String> ();
                if (found.Contains (thisMachineAddr)) {
                    continue;
                }
                else {
                    found.Add (thisMachineAddr);
                }
                newDev.name = Configuration::GetSystemConfiguration_ComputerNames ().fHostname;
                newDev.type = L"Laptop";
                return newDev;
            }
        }
        return {};
    }
};

DeviceDiscoverer::DeviceDiscoverer (const Discovery::Network& forNetwork)
    : fRep_ (make_unique<Rep_> (forNetwork))
{
}

Collection<Discovery::Device> DeviceDiscoverer::GetActiveDevices () const
{
    return fRep_->GetActiveDevices ();
}

DeviceDiscoverer::~DeviceDiscoverer ()
{
}
