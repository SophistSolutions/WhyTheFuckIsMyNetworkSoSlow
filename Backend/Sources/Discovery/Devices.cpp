/*
* Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Cache/SynchronizedCallerStalenessCache.h"
#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Configuration/SystemConfiguration.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/DNS.h"
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/LinkMonitor.h"
#include "Stroika/Foundation/IO/Network/Transfer/Client.h"
#include "Stroika/Foundation/IO/Network/Transfer/Connection.h"

#include "Stroika/Frameworks/UPnP/DeviceDescription.h"
#include "Stroika/Frameworks/UPnP/SSDP/Client/Listener.h"
#include "Stroika/Frameworks/UPnP/SSDP/Client/Search.h"

#include "NetworkInterfaces.h"

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
using IO::Network::InternetAddress;
using Stroika::Foundation::Common::GUID;
using Stroika::Foundation::Common::KeyValuePair;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Discovery;

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

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
    sb += L"ipAddress: " + Characters::ToString (ipAddresses) + L", ";
    if (type) {
        sb += L"type: " + Characters::ToString (type) + L", ";
    }
    if (fThisDevice) {
        sb += L"This-Device: " + Characters::ToString (fThisDevice) + L", ";
    }
    sb += L"fNetworks: " + Characters::ToString (fNetworks) + L", ";
    sb += L"fAttachedInterfaces: " + Characters::ToString (fAttachedInterfaces) + L", ";
    sb += L"}";
    return sb.str ();
}

/*
 ********************************************************************************
 ****************************** DeviceDiscoverer_ *******************************
 ********************************************************************************
 */
namespace {

    class NetAndNetInterfaceMapper_ {
    public:
        NetAndNetInterfaceMapper_ () = default; // load networks and network interafces..

    public:
        Set<GUID> LookupNetworksGUIDs (const Set<InternetAddress>& ia) const
        {
            Set<GUID> results;
            for (Discovery::Network&& nw : Discovery::NetworksMgr::sThe.CollectActiveNetworks ()) {
                for (const CIDR& nwi : nw.fNetworkAddresses) {
                    for (const InternetAddress& i : ia) {
                        if (nwi.GetRange ().Contains (i)) {
                            results += nw.fGUID;
                            goto out;
                        }
                    }
                }
            out:;
            }
            return results;
        }

    public:
        static NetAndNetInterfaceMapper_ sThe;
    };
    NetAndNetInterfaceMapper_ NetAndNetInterfaceMapper_::sThe;

    struct DiscoveryInfo_ {
        Set<IO::Network::InternetAddress> fInternetAddresses;
        bool                              alive{};
        String                            server;
    };
    Synchronized<Mapping<String, DiscoveryInfo_>> sDiscoveredDevices_;

    optional<Discovery::Device> GetMyDevice_ ()
    {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
        Debug::TraceContextBumper ctx{L"{}::GetMyDevice_"};
#endif
        DbgTrace (L"interfaces=%s", Characters::ToString (IO::Network::GetInterfaces ()).c_str ());
        Discovery::Device newDev;
        newDev.name        = Configuration::GetSystemConfiguration_ComputerNames ().fHostname;
        newDev.type        = DeviceType::eLaptop; //tmphack @todo fix
        newDev.fThisDevice = true;
        for (Interface i : IO::Network::GetInterfaces ()) {
            if (i.fType != Interface::Type::eLoopback and i.fStatus and i.fStatus->Contains (Interface::Status::eRunning)) {
                i.fBindings.Apply ([&](const Interface::Binding& ia) {
                    if (not ia.fInternetAddress.IsMulticastAddress ()) {
                        newDev.ipAddresses += ia.fInternetAddress;
                    }
                });
            }
        }
        newDev.fNetworks           = NetAndNetInterfaceMapper_::sThe.LookupNetworksGUIDs (newDev.ipAddresses);
        newDev.fAttachedInterfaces = Set<GUID>{Discovery::NetworkInterfacesMgr::sThe.CollectAllNetworkInterfaces ().Select<GUID> ([](auto iFace) { return iFace.fGUID; })};
        return newDev;
    }

    Collection<Discovery::Device> GetActiveDevices_ ()
    {
        static const Mapping<String, String> kNamePrettyPrintMapper_{
            KeyValuePair<String, String>{L"ASUSTeK UPnP/1.1 MiniUPnPd/1.9"sv, L"ASUS Router"sv},
            KeyValuePair<String, String>{L"Microsoft-Windows/10.0 UPnP/1.0 UPnP-Device-Host/1.0"sv, L"Antiphon"sv},
            KeyValuePair<String, String>{L"POSIX, UPnP/1.0, Intel MicroStack/1.0.1347"sv, L"HP PhotoSmart"sv},
        };
        Collection<Discovery::Device> results;
        for (DiscoveryInfo_ di : sDiscoveredDevices_.cget ()->MappedValues ()) {
            Discovery::Device newDev;

            newDev.name = di.server;
            newDev.name = kNamePrettyPrintMapper_.LookupValue (newDev.name, newDev.name);
            newDev.ipAddresses += di.fInternetAddresses;
            newDev.type      = nullopt;
            newDev.fNetworks = NetAndNetInterfaceMapper_::sThe.LookupNetworksGUIDs (newDev.ipAddresses);
            if (newDev.ipAddresses.Any ([](const InternetAddress& ia) { return ia.As<String> ().EndsWith (L".1"); })) {
                newDev.type = Discovery::DeviceType::eRouter;
            }

            results.Add (newDev);
        }
        if (auto i = GetMyDevice_ ()) {
            results.Add (*i);
        }
        return results;
    }

    /*
     *  DeviceDiscoverer is internally syncronized - so its methods can be called from any thread.
     *
     *  @todo this CURRENTLY only discovers for a single network, but we should discover devices on all networks (and merge them somehow when they are the smae device on multiple networks)
     */
    class DeviceDiscoverer_ {
    public:
        DeviceDiscoverer_ ()
            : fListener_{[this](const SSDP::Advertisement& d) { this->RecieveSSDPAdvertisement_ (d); }, SSDP::Client::Listener::eAutoStart}
            , fSearcher_{[this](const SSDP::Advertisement& d) { this->RecieveSSDPAdvertisement_ (d); }, SSDP::Client::Search::kRootDevice}
        {
        }

    private:
        SSDP::Client::Listener fListener_;
        SSDP::Client::Search   fSearcher_;

    private:
        void RecieveSSDPAdvertisement_ (const SSDP::Advertisement& d)
        {
            DbgTrace (L"Recieved SSDP advertisement: %s", Characters::ToString (d).c_str ());
            using IO::Network::InternetAddress;
            String               location = d.fLocation.GetFullURL ();
            optional<bool>       alive    = d.fAlive;
            URL                  locURL   = URL{location, URL::ParseOptions::eAsFullURL};
            String               locHost  = locURL.GetHost ();
            Set<InternetAddress> locAddrs = Set<InternetAddress>{IO::Network::DNS::Default ().GetHostAddresses (locHost)};

            String friendlyName = d.fServer; // if all else fails..

            // @todo - Maintain cache with age apx 60 minutes - mapping URL to UPnP::DeviceDescription objects

            /// @todo - Add Search support (once at startup, and then every 10 minutes? - config) - because it maybe some devices dont properly
            /// broadcast, and only respond to search, plus gives better immediate feedback when we first start up (at least helpful for debugging)

            try {
                IO::Network::Transfer::Connection c = IO::Network::Transfer::CreateConnection ();
                c.SetURL (locURL);
                IO::Network::Transfer::Response r = c.GET ();
                if (r.GetSucceeded ()) {
                    Frameworks::UPnP::DeviceDescription deviceInfo = DeSerialize (r.GetData ());
                    friendlyName                                   = deviceInfo.fFriendlyName;
                    DbgTrace (L"found device description = %s", Characters::ToString (deviceInfo).c_str ());
                }
            }
            catch (...) {
                DbgTrace (L"failed to fetch description: %s", Characters::ToString (current_exception ()).c_str ());
            }

            if (locHost.empty ()) {
                DbgTrace (L"oops - bad - probably should log - bad device resposne - find addr some other way");
            }
            else {
                sDiscoveredDevices_.rwget ()->Add (location, DiscoveryInfo_{locAddrs, alive.value_or (true), friendlyName});
            }
        }
    };

}

/*
 ********************************************************************************
 ********************* Discovery::DevicesMgr::Activator *************************
 ********************************************************************************
 */
namespace {
    constexpr Time::DurationSecondsType kDefaultItemCacheLifetime_{2}; // this costs very little since just reading already cached data so default to quick check

    unique_ptr<DeviceDiscoverer_> sSSDPDeviceDiscoverer_;

    bool sActive_{false};
}

Discovery::DevicesMgr::Activator::Activator ()
{
    DbgTrace (L"Discovery::DevicesMgr::Activator::Activator: activating device discovery");
    Require (not sActive_);
    sSSDPDeviceDiscoverer_ = make_unique<DeviceDiscoverer_> ();
    sActive_               = true;
    IgnoreExceptionsForCall (sThe.GetActiveDevices ());
}

Discovery::DevicesMgr::Activator::~Activator ()
{
    DbgTrace (L"Discovery::DevicesMgr::Activator::~Activator: deactivating device discovery");
    Require (sActive_);
    sSSDPDeviceDiscoverer_.release ();
    sActive_ = false;

    // @todo - must shutdown any active threads
}

/*
 ********************************************************************************
 **************************** Discovery::DevicesMgr *****************************
 ********************************************************************************
 */

DevicesMgr DevicesMgr::sThe;

Collection<Discovery::Device> Discovery::DevicesMgr::GetActiveDevices (optional<Time::DurationSecondsType> allowedStaleness) const
{
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    Debug::TraceContextBumper ctx{L"Discovery::GetActiveDevices"};
#endif
    Require (sActive_);
    Collection<Discovery::Device> results;
    using Cache::SynchronizedCallerStalenessCache;
    static SynchronizedCallerStalenessCache<void, Collection<Discovery::Device>> sCache_;
    results = sCache_.LookupValue (sCache_.Ago (allowedStaleness.value_or (kDefaultItemCacheLifetime_)), []() {
        return GetActiveDevices_ ();
    });
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace (L"returns: %s", Characters::ToString (results).c_str ());
#endif
    return results;
}
