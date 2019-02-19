/*
* Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Cache/SynchronizedCallerStalenessCache.h"
#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Configuration/SystemConfiguration.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Cryptography/Digest/Algorithm/MD5.h"
#include "Stroika/Foundation/Cryptography/Format.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/Execution/Thread.h"
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

using Execution::RWSynchronized;
using Execution::Thread;
using IO::Network::InternetAddress;
using Stroika::Foundation::Common::GUID;
using Stroika::Foundation::Common::KeyValuePair;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Discovery;

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

namespace {
    // @todo LIKE WITH NETWORK IDS - probably maintain a persistence cache mapping info - mostly HARDWARE ADDRESS - to a uniuque nummber (guidgen maybe).
    // THEN we will always identify a device as the sam thing even if it appears with diferent IP address on different network
    //
    // must be careful about virtual devices (like VMs) which use fake hardware addresses, so need some way to tell differnt devices (and then one from another?)
    //
    //tmphack -- MAYBE USE GUID-GEN UNTIL I HAVE DISK PERSISTENCE FOR THIS STUFF?
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
 ******************************* Discovery::Device ******************************
 ********************************************************************************
 */
String Discovery::Device::ToString () const
{
    StringBuilder sb;
    sb += L"{";
    sb += L"fGUID: " + Characters::ToString (fGUID) + L", ";
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

namespace {
    /*
     ********************************************************************************
     ********************** NetAndNetInterfaceMapper_ *******************************
     ********************************************************************************
     */
    class NetAndNetInterfaceMapper_ {
    public:
        NetAndNetInterfaceMapper_ () = default; // load networks and network interafces..

    public:
        Set<GUID> LookupNetworksGUIDs (const Set<InternetAddress>& ia) const
        {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
            Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"{}::NetAndNetInterfaceMapper_::LookupNetworksGUIDs (%s)", Characters::ToString (ia).c_str ())};
#endif
            Set<GUID> results;
            for (Discovery::Network&& nw : Discovery::NetworksMgr::sThe.CollectActiveNetworks ()) {
                DbgTrace (L"nw.fNetworkAddresses=%s", Characters::ToString (nw.fNetworkAddresses).c_str ());
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
#if USE_NOISY_TRACE_IN_THIS_MODULE_
            DbgTrace (L"returning %s", Characters::ToString (results).c_str ());
#endif
            return results;
        }

    public:
        static NetAndNetInterfaceMapper_ sThe;
    };
    NetAndNetInterfaceMapper_ NetAndNetInterfaceMapper_::sThe;
}

namespace {
    /*
     ********************************************************************************
     *********************** GetPrettiedUpDeviceName_ *******************************
     ********************************************************************************
     */
    String GetPrettiedUpDeviceName_ (const String& origName)
    {
        static const Mapping<String, String> kNamePrettyPrintMapper_{
            KeyValuePair<String, String>{L"ASUSTeK UPnP/1.1 MiniUPnPd/1.9"sv, L"ASUS Router"sv},
            KeyValuePair<String, String>{L"Microsoft-Windows/10.0 UPnP/1.0 UPnP-Device-Host/1.0"sv, L"Antiphon"sv},
            KeyValuePair<String, String>{L"POSIX, UPnP/1.0, Intel MicroStack/1.0.1347"sv, L"HP PhotoSmart"sv},
        };
        return kNamePrettyPrintMapper_.LookupValue (origName, origName);
    }
}

/*
 ********************************************************************************
 *************************** sDiscoveredDevices_ ********************************
 ********************************************************************************
 */
namespace {
    /*
     *  Keep extra internal details about the discovered devices which we don't advertise outside this module.
     */
    struct DiscoveryInfo_ : Discovery::Device {
        bool alive{}; // currently unused

        String ToString () const
        {
            StringBuilder sb = Discovery::Device::ToString ().SubString (0, -1);
			sb += L"alive: " + Characters::ToString (alive) + L", ";
			sb += L"}";
            return sb.str ();
        }
    };
    // NB: RWSynchronized because most accesses will be to read/lookup in this list; use Mapping<> because KeyedCollection NYI
    RWSynchronized<Mapping<GUID, DiscoveryInfo_>> sDiscoveredDevices_;

    // Look through all the existing devices, and if one appears to match, return it.
    optional<DiscoveryInfo_> FindMatchingDevice_ (decltype (sDiscoveredDevices_)::ReadableReference& rr, const DiscoveryInfo_& d)
    {
        for (const auto& di : rr->MappedValues ()) {
            if (not(d.ipAddresses ^ di.ipAddresses).empty ()) {
                return di;
            }
        }
        return {};
    }
}

namespace {
    /*
     ********************************************************************************
     *************************** MyDeviceDiscoverer_ ********************************
     ********************************************************************************
     */
    // @todo redo this with IDs, and have the thread keep running to update network info
    struct MyDeviceDiscoverer_ {
        MyDeviceDiscoverer_ ()
            : fMyDeviceDiscovererThread_ (
                  Thread::CleanupPtr::eAbortBeforeWaiting, Thread::New (DiscoveryChecker_, Thread::eAutoStart, L"MyDeviceDisoverer"))
        {
        }

    private:
        static void DiscoveryChecker_ ()
        {
            // @todo need loop
            if (auto o = GetMyDevice_ ()) {
                auto           l  = sDiscoveredDevices_.rwget ();
                DiscoveryInfo_ di = [&]() {
                    DiscoveryInfo_ tmp{};
                    tmp.ipAddresses += o->ipAddresses;
                    if (optional<DiscoveryInfo_> o = FindMatchingDevice_ (l, tmp)) {
                        return *o;
                    }
                    else {
                        // Generate GUID - based on ipaddrs
                        tmp.fGUID = LookupPersistentDeviceID_ (tmp);
                        return tmp;
                    }
                }();
                *static_cast<Discovery::Device*> (&di) = *o;
                l->Add (di.fGUID, di);
            }
        }
        Thread::CleanupPtr fMyDeviceDiscovererThread_;

        static optional<Discovery::Device> GetMyDevice_ ()
        {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
            Debug::TraceContextBumper ctx{L"{}::GetMyDevice_"};
            DbgTrace (L"interfaces=%s", Characters::ToString (IO::Network::GetInterfaces ()).c_str ());
#endif
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
            newDev.fGUID               = LookupPersistentDeviceID_ (newDev);
            return newDev;
        }
    };

    unique_ptr<MyDeviceDiscoverer_> sMyDeviceDiscoverer_;
}

namespace {
    /*
     ********************************************************************************
     *************************** SSDPDeviceDiscoverer_ ******************************
     ********************************************************************************
     */
    /*
     *  When constructed, push data as discovered into sDiscoveredDevices_
     */
    class SSDPDeviceDiscoverer_ {
    public:
        SSDPDeviceDiscoverer_ ()
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
            String               location = d.fLocation.GetFullURL ();
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
                // merge in data
                auto           l  = sDiscoveredDevices_.rwget ();
                DiscoveryInfo_ di = [&]() {
                    DiscoveryInfo_ tmp{};
                    tmp.ipAddresses += locAddrs;
                    if (optional<DiscoveryInfo_> o = FindMatchingDevice_ (l, tmp)) {
                        return *o;
                    }
                    else {
                        // Generate GUID - based on ipaddrs
                        tmp.fGUID = LookupPersistentDeviceID_ (tmp);
                        return tmp;
                    }
                }();

                di.alive = d.fAlive.value_or (true);
                di.name  = GetPrettiedUpDeviceName_ (friendlyName);
                di.ipAddresses += locAddrs;

                di.fNetworks = NetAndNetInterfaceMapper_::sThe.LookupNetworksGUIDs (di.ipAddresses);

                if (di.ipAddresses.Any ([](const InternetAddress& ia) { return ia.As<String> ().EndsWith (L".1"); })) {
                    di.type = Discovery::DeviceType::eRouter;
                }
                l->Add (di.fGUID, di);
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
    constexpr Time::DurationSecondsType kDefaultItemCacheLifetime_{1}; // this costs very little since just reading already cached data so default to quick check

    unique_ptr<SSDPDeviceDiscoverer_> sSSDPDeviceDiscoverer_;

    bool IsActive_ ()
    {
        Require (static_cast<bool> (sMyDeviceDiscoverer_) == static_cast<bool> (sSSDPDeviceDiscoverer_));
        return sSSDPDeviceDiscoverer_ != nullptr;
    }
}

Discovery::DevicesMgr::Activator::Activator ()
{
    DbgTrace (L"Discovery::DevicesMgr::Activator::Activator: activating device discovery");
    Require (not IsActive_ ());
    sSSDPDeviceDiscoverer_ = make_unique<SSDPDeviceDiscoverer_> ();
    sMyDeviceDiscoverer_   = make_unique<MyDeviceDiscoverer_> ();
}

Discovery::DevicesMgr::Activator::~Activator ()
{
    DbgTrace (L"Discovery::DevicesMgr::Activator::~Activator: deactivating device discovery");
    Require (IsActive_ ());
    sSSDPDeviceDiscoverer_.release ();
    sMyDeviceDiscoverer_.release ();
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
    Require (IsActive_ ());
    Collection<Discovery::Device> results;
    using Cache::SynchronizedCallerStalenessCache;
    static SynchronizedCallerStalenessCache<void, Collection<Discovery::Device>> sCache_;
    results = sCache_.LookupValue (sCache_.Ago (allowedStaleness.value_or (kDefaultItemCacheLifetime_)), []() {
        return sDiscoveredDevices_.cget ()->MappedValues ();
    });
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace (L"returns: %s", Characters::ToString (results).c_str ());
#endif
    return results;
}
