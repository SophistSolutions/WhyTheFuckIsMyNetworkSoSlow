/*
* Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Cache/SynchronizedCallerStalenessCache.h"
#include "Stroika/Foundation/Characters/RegularExpression.h"
#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Configuration/SystemConfiguration.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Cryptography/Digest/Algorithm/MD5.h"
#include "Stroika/Foundation/Cryptography/Format.h"
#include "Stroika/Foundation/Execution/Activity.h"
#include "Stroika/Foundation/Execution/Logger.h"
#include "Stroika/Foundation/Execution/Sleep.h"
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
using namespace Stroika::Foundation::Execution;
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

namespace {
    // derived based on experimentation on my network - need standards/referecnes! -- LGP 2019-02-20
    const String kDeviceType_SpeakerGroup_{L"urn:smartspeaker-audio:device:SpeakerGroup:1"sv};
    const String kDeviceType_ZonePlayer_{L"urn:schemas-upnp-org:device:ZonePlayer:1"sv};
    const String kDeviceType_WFADevice_{L"urn:schemas-wifialliance-org:device:WFADevice:1"sv};
    const String kDeviceType_WANConnectionDevice_{L"urn:schemas-upnp-org:device:WANConnectionDevice:1"sv};
    const String kDeviceType_WANDevice_{L"urn:schemas-upnp-org:device:WANDevice:1"sv};
    const String kDeviceType_MediaRenderer_{L"urn:schemas-upnp-org:device:MediaRenderer:1"sv};

    // probably shouldn't be this specifc
    const String kDeviceType_Roku_{L"urn:roku-com:device:player:1-0"sv};
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
    sb += L"GUID: " + Characters::ToString (fGUID) + L", ";
    sb += L"name: " + Characters::ToString (name) + L", ";
    sb += L"ipAddress: " + Characters::ToString (ipAddresses) + L", ";
    sb += L"types: " + Characters::ToString (fTypes) + L", ";
    if (fThisDevice) {
        sb += L"This-Device: " + Characters::ToString (fThisDevice) + L", ";
    }
    sb += L"fNetworks: " + Characters::ToString (fNetworks) + L", ";
    sb += L"fAttachedInterfaces: " + Characters::ToString (fAttachedInterfaces) + L", ";
    sb += L"fPresentationURL: " + Characters::ToString (fPresentationURL) + L", ";
    sb += L"fOperatingSystem: " + Characters::ToString (fOperatingSystem) + L", ";
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

        optional<String> fForcedName;

        struct SSDPInfo {
            optional<bool>          fAlive; // else Bye notification, or empty if neither -- probably replace with TIMINGS of last ALIVE, or Bye
            Set<String>             fUSNs;
            Set<URL>                fLocations;
            optional<String>        fServer;
            optional<String>        fManufacturer;
            Mapping<String, String> fDeviceType2FriendlyNameMap; //  http://upnp.org/specs/arch/UPnP-arch-DeviceArchitecture-v1.1.pdf - <deviceType> - Page 44
            optional<URL>           fPresentationURL;

            String ToString () const
            {
                StringBuilder sb;
                sb += L"{";
                sb += L"fUSNs: " + Characters::ToString (fUSNs) + L", ";
                sb += L"fAlive: " + Characters::ToString (fAlive) + L", ";
                sb += L"fLocations: " + Characters::ToString (fLocations) + L", ";
                sb += L"fManufacturer: " + Characters::ToString (fManufacturer) + L", ";
                sb += L"fServer: " + Characters::ToString (fServer) + L", ";
                sb += L"Device-Type-2-Friendly-Name-Map: " + Characters::ToString (fDeviceType2FriendlyNameMap) + L", ";
                sb += L"fPresentationURL: " + Characters::ToString (fPresentationURL) + L", ";
                sb += L"}";
                return sb.str ();
            }
        };
        optional<SSDPInfo> fSSDPInfo;

        void PatchDerivedFields ()
        {
            fNetworks = NetAndNetInterfaceMapper_::sThe.LookupNetworksGUIDs (ipAddresses);

            /*
             *  Name Calculation
             */
            name.clear ();
            if (fForcedName) {
                name = *fForcedName;
            }
            else if (fSSDPInfo.has_value ()) {
                // Special hack for sonos speakers
                if (fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_SpeakerGroup_) and
                    fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_ZonePlayer_)) {
                    static const RegularExpression kSonosRE_{L"([0-9.:]*)( - .*)"_RegEx};
                    String                         newName = fSSDPInfo->fDeviceType2FriendlyNameMap[kDeviceType_ZonePlayer_];
                    optional<String>               m1, m2;
                    if (newName.Match (kSonosRE_, &m1, &m2)) {
                        Assert (m1.has_value () and m2.has_value ());
                        name = fSSDPInfo->fDeviceType2FriendlyNameMap[kDeviceType_SpeakerGroup_] + *m2;
                    }
                }

                // pick any one of the friendly names, or the server name if we must
                if (name.empty ()) {
                    if (not fSSDPInfo->fDeviceType2FriendlyNameMap.empty ()) {
                        name = fSSDPInfo->fDeviceType2FriendlyNameMap.Nth (0).fValue;
                    }
                    else if (fSSDPInfo->fServer) {
                        name = GetPrettiedUpDeviceName_ (*fSSDPInfo->fServer);
                    }
                }

                fPresentationURL = fSSDPInfo->fPresentationURL;
            }
            if (name.empty ()) {
                // try reverse dns lookup
                for (auto i : ipAddresses) {
                    if (auto o = DNS::Default ().ReverseLookup (i)) {
                        name = *o;
                        break;
                    }
                }
            }
            if (name.empty ()) {
                name = L"Unknown"sv;
            }

            if (fSSDPInfo.has_value () and
                (fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_SpeakerGroup_) or
                 fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_ZonePlayer_))) {
                fTypes.Add (Discovery::DeviceType::eSpeaker);
            }

            if (fSSDPInfo.has_value () and
                (fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_WFADevice_) or
                 fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_WANConnectionDevice_) or
                 fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_WANDevice_))) {
                // @todo this logic could use improvement
                if (ipAddresses.Any ([](const InternetAddress& ia) { return ia.As<String> ().EndsWith (L".1"); })) {
                    fTypes.Add (Discovery::DeviceType::eRouter);
                }
                fTypes.Add (Discovery::DeviceType::eNetworkInfrastructure);
            }

            // probably insufficient
            if (fSSDPInfo.has_value () and
                // @todo - need a better way to detect - look at services not device type?
                (fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_MediaRenderer_))) {
                fTypes.Add (Discovery::DeviceType::eTV);
            }

            if (fSSDPInfo.has_value () and
                // @todo - need a better way to detect - look at services not device type?
                (fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_Roku_))) {
                fTypes.Add (Discovery::DeviceType::eMediaPlayer);
            }

            //tmphack
            if (not fThisDevice) {
                if (ipAddresses.Any ([](const InternetAddress& ia) { return ia.As<String> ().EndsWith (L".1"); })) {
                    fTypes.Add (Discovery::DeviceType::eRouter);
                }
            }

#if USE_NOISY_TRACE_IN_THIS_MODULE_
            DbgTrace (L"At end of PatchDerivedFields: %s", ToString ().c_str ());
#endif
        }

        String ToString () const
        {
            StringBuilder sb = Discovery::Device::ToString ().SubString (0, -1);
            if (fForcedName) {
                sb += L"fForcedName: " + Characters::ToString (fForcedName) + L", ";
            }
            sb += L"fSSDPInfo: " + Characters::ToString (fSSDPInfo) + L", ";
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
                  Thread::CleanupPtr::eAbortBeforeWaiting, Thread::New (DiscoveryChecker_, Thread::eAutoStart, L"MyDeviceDiscoverer"))
        {
        }

    private:
        static void DiscoveryChecker_ ()
        {
            static constexpr Activity kDiscovering_This_Device_{L"discovering this device"sv};
            while (true) {
                try {
                    DeclareActivity da{&kDiscovering_This_Device_};
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
                        // copy most/all fields -- @todo cleanup - do more automatically - all but GUID??? Need merge??
                        di.fTypes           = o->fTypes;
                        di.fForcedName      = o->fForcedName;
                        di.fThisDevice      = o->fThisDevice;
                        di.fOperatingSystem = OperatingSystem{Configuration::GetSystemConfiguration_OperatingSystem ().fPrettyNameWithMajorVersion};
                        di.PatchDerivedFields ();
                        l->Add (di.fGUID, di);
                    }
                }
                catch (const Thread::InterruptException&) {
                    Execution::ReThrow ();
                }
                catch (...) {
                    Execution::Logger::Get ().LogIfNew (Execution::Logger::Priority::eError, 5min, L"%s", Characters::ToString (current_exception ()).c_str ());
                }
                Execution::Sleep (30); // @todo tmphack - really wait til change in network
            }
        }
        Thread::CleanupPtr fMyDeviceDiscovererThread_;

        static optional<DiscoveryInfo_> GetMyDevice_ ()
        {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
            Debug::TraceContextBumper ctx{L"{}::GetMyDevice_"};
            DbgTrace (L"interfaces=%s", Characters::ToString (IO::Network::GetInterfaces ()).c_str ());
#endif
            DiscoveryInfo_ newDev;
            newDev.fForcedName = Configuration::GetSystemConfiguration_ComputerNames ().fHostname;
            newDev.fTypes.Add (DeviceType::ePC); //tmphack @todo fix
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
        {
            // SSDP can fail due to lack of permissions to bind to the appropriate sockets, or for example under WSL where we get protocol unsupported.
            // WARN to syslog, but no need to stop app
            try {
                fListener_ = make_unique<SSDP::Client::Listener> ([this](const SSDP::Advertisement& d) { this->RecieveSSDPAdvertisement_ (d); }, SSDP::Client::Listener::eAutoStart);
            }
            catch (...) {
                Logger::Get ().Log (Logger::Priority::eError, L"Problem starting SSDP Listener - so that source of discovery will be unavailable: %s", Characters::ToString (current_exception ()).c_str ());
            }
            try {
                fSearcher_ = make_unique<SSDP::Client::Search> ([this](const SSDP::Advertisement& d) { this->RecieveSSDPAdvertisement_ (d); }, SSDP::Client::Search::kRootDevice);
            }
            catch (...) {
                // only warning because searcher much less important - just helpful at very start of discovery
                Logger::Get ().Log (Logger::Priority::eWarning, L"Problem starting SSDP Searcher - so that source of discovery will be unavailable: %s", Characters::ToString (current_exception ()).c_str ());
            }
        }

    private:
        unique_ptr<SSDP::Client::Listener> fListener_;
        unique_ptr<SSDP::Client::Search>   fSearcher_;

    private:
        void RecieveSSDPAdvertisement_ (const SSDP::Advertisement& d)
        {
            constexpr Activity        kInterprettingSSDPMessageRecieved_{L"interpretting SSDP advertisement"sv};
            Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"RecieveSSDPAdvertisement_", L"d=%s", Characters::ToString (d).c_str ())};

            DeclareActivity activity{&kInterprettingSSDPMessageRecieved_};

            String               location = d.fLocation.GetFullURL ();
            URL                  locURL   = URL{location, URL::ParseOptions::eAsFullURL};
            String               locHost  = locURL.GetHost ();
            Set<InternetAddress> locAddrs = Set<InternetAddress>{IO::Network::DNS::Default ().GetHostAddresses (locHost)};

            // @todo - Maintain cache with age apx 60 minutes - mapping URL to UPnP::DeviceDescription objects

            /// @todo - Add Search support (once at startup, and then every 10 minutes? - config) - because it maybe some devices dont properly
            /// broadcast, and only respond to search, plus gives better immediate feedback when we first start up (at least helpful for debugging)
            optional<String> deviceFriendlyName;
            optional<String> deviceType;
            optional<String> manufactureName;
            optional<URL>    presentationURL;

            try {
                IO::Network::Transfer::Connection c = IO::Network::Transfer::CreateConnection ();
                c.SetURL (locURL);
                IO::Network::Transfer::Response r = c.GET ();
                if (r.GetSucceeded ()) {
                    Frameworks::UPnP::DeviceDescription deviceInfo = DeSerialize (r.GetData ());
                    deviceFriendlyName                             = deviceInfo.fFriendlyName;
                    deviceType                                     = deviceInfo.fDeviceType;
                    manufactureName                                = deviceInfo.fManufactureName;
                    presentationURL                                = deviceInfo.fPresentationURL;
                    DbgTrace (L"Found device description = %s", Characters::ToString (deviceInfo).c_str ());
                }
            }
            catch (...) {
                DbgTrace (L"Failed to fetch description: %s", Characters::ToString (current_exception ()).c_str ());
            }

            if (locHost.empty ()) {
                DbgTrace (L"oops - bad - probably should log - bad device response - find addr some other way");
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

                if (not di.fSSDPInfo) {
                    di.fSSDPInfo = DiscoveryInfo_::SSDPInfo{};
                }
                di.fSSDPInfo->fAlive = d.fAlive;

                if (presentationURL) {
                    // consider if value already there - warn if changes - should we collect multiple
                    di.fSSDPInfo->fPresentationURL = presentationURL;
                }

                if (di.fSSDPInfo->fServer.has_value () and di.fSSDPInfo->fServer != d.fServer) {
                    DbgTrace (L"Warning: different server IDs for same object");
                }
                di.fSSDPInfo->fServer = d.fServer;

                di.ipAddresses += locAddrs;
                if (deviceType and deviceFriendlyName) {
                    di.fSSDPInfo->fDeviceType2FriendlyNameMap.Add (*deviceType, *deviceFriendlyName);
                }
                Memory::CopyToIf (manufactureName, &di.fSSDPInfo->fManufacturer);

                di.PatchDerivedFields ();
                l->Add (di.fGUID, di);
            }
        }
    };
}

///tmphack -most of this should go into new Stroika module and also fancier
//-Add new stroika module 'IO::Network::Neighbors'
//- https://www.midnightfreddie.com/how-to-arp-a-in-ipv6.html
//-http ://man7.org/linux/man-pages/man8/ip-neighbour.8.html
//  -arp - a
//  - ip neigh show
//  - ip - 6 neigh show
//  - cat / proc / net / arp
//  - use that to fill in new devices / info for discovery
#include "Stroika/Foundation/Execution/ProcessRunner.h"
#include "Stroika/Foundation/Streams/MemoryStream.h"
#include "Stroika/Foundation/Streams/TextReader.h"
namespace {
    struct ArpRec_ {
        InternetAddress ia;
        String          fHardwareAddress;
    };
    Collection<ArpRec_> ArpDashA_ ()
    {
        //tmphack
        Collection<ArpRec_> result;

        using std::byte;
        ProcessRunner                    pr (L"arp -a");
        Streams::MemoryStream<byte>::Ptr useStdOut = Streams::MemoryStream<byte>::New ();
        pr.SetStdOut (useStdOut);
        pr.Run ();
        String                   out;
        Streams::TextReader::Ptr stdOut        = Streams::TextReader::New (useStdOut);
        bool                     skippedHeader = false;
        size_t                   headerLen     = 0;
        for (String i = stdOut.ReadLine (); not i.empty (); i = stdOut.ReadLine ()) {
#if qPlatform_Windows
            if (i.StartsWith (L" ")) {
                Sequence<String> s = i.Tokenize ();
                if (s.length () >= 3 and (s[2] == L"static" or s[2] == L"dynamic")) {
                    result += ArpRec_{InternetAddress{s[0]}, s[1]};
                }
            }
#elif qPlatform_POSIX
            Sequence<String> s = i.Tokenize ();
            if (s.length () >= 4) {
                // raspberrypi.34ChurchStreet.sophists.com (192.168.244.32) at b8:27:eb:cc:c7:80 [ether] on enp0s31f6
                // ? (192.168.244.173) at b8:3e:59:88:71:06 [ether] on enp0s31f6
                if (s[1].StartsWith (L"(") and s[1].EndsWith (L")")) {
                    result += ArpRec_{InternetAddress{s[1].SubString (1, -1)}, s[3]};
                }
            }
#endif
        }
        return result;
    }
}

namespace {
    /*
     ********************************************************************************
     *************************** MyNeighborDiscoverer_ ******************************
     ********************************************************************************
     */

    struct MyNeighborDiscoverer_ {
        MyNeighborDiscoverer_ ()
            : fMyThread_ (
                  Thread::CleanupPtr::eAbortBeforeWaiting, Thread::New (DiscoveryChecker_, Thread::eAutoStart, L"MyNeighborDiscoverer"))
        {
        }

    private:
        static void DiscoveryChecker_ ()
        {
            static constexpr Activity kDiscovering_NetNeighbors_{L"discovering this network neighbors"sv};
            while (true) {
                try {
                    DeclareActivity da{&kDiscovering_NetNeighbors_};
                    for (ArpRec_ i : ArpDashA_ ()) {
                        // soon store/pay attention to macaddr as better indicator of unique device id than ip addr

                        // ignore multicast addresses as they are not real devices(???always???)
                        if (i.ia.IsMulticastAddress ()) {
                            //DbgTrace (L"ignoring arped multicast address %s", Characters::ToString (i.ia).c_str ());
                            continue;
                        }
#if qPlatform_Windows
                        if (i.fHardwareAddress == L"ff-ff-ff-ff-ff-ff") {
                            //DbgTrace (L"ignoring arped fake(broadcast) address %s", Characters::ToString (i.ia).c_str ());
                            continue;
                        }
#endif

                        // merge in data
                        auto           l  = sDiscoveredDevices_.rwget ();
                        DiscoveryInfo_ di = [&]() {
                            DiscoveryInfo_ tmp{};
                            tmp.ipAddresses += i.ia;
                            if (optional<DiscoveryInfo_> o = FindMatchingDevice_ (l, tmp)) {
                                return *o;
                            }
                            else {
                                // Generate GUID - based on ipaddrs
                                tmp.fGUID = LookupPersistentDeviceID_ (tmp);
                                return tmp;
                            }
                        }();

                        di.PatchDerivedFields ();
                        l->Add (di.fGUID, di);
                    }
                }
                catch (const Thread::InterruptException&) {
                    Execution::ReThrow ();
                }
                catch (...) {
                    Execution::Logger::Get ().LogIfNew (Execution::Logger::Priority::eError, 5min, L"%s", Characters::ToString (current_exception ()).c_str ());
                }
                Execution::Sleep (1min); // unsure of right interval - maybe able to epoll or something so no actual polling needed
            }
        }
        Thread::CleanupPtr fMyThread_;
    };

    unique_ptr<MyNeighborDiscoverer_> sNeighborDiscoverer_;
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
        Require (static_cast<bool> (sNeighborDiscoverer_) == static_cast<bool> (sSSDPDeviceDiscoverer_));
        return sSSDPDeviceDiscoverer_ != nullptr;
    }
}

Discovery::DevicesMgr::Activator::Activator ()
{
    DbgTrace (L"Discovery::DevicesMgr::Activator::Activator: activating device discovery");
    Require (not IsActive_ ());
    sSSDPDeviceDiscoverer_ = make_unique<SSDPDeviceDiscoverer_> ();
    sMyDeviceDiscoverer_   = make_unique<MyDeviceDiscoverer_> ();
    sNeighborDiscoverer_   = make_unique<MyNeighborDiscoverer_> ();
}

Discovery::DevicesMgr::Activator::~Activator ()
{
    DbgTrace (L"Discovery::DevicesMgr::Activator::~Activator: deactivating device discovery");
    Require (IsActive_ ());
    sSSDPDeviceDiscoverer_.reset ();
    sMyDeviceDiscoverer_.reset ();
    sNeighborDiscoverer_.reset ();
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
#if USE_NOISY_TRACE_IN_THIS_MODULE_
        DbgTrace (L"sDiscoveredDevices_: %s", Characters::ToString (sDiscoveredDevices_.cget ()->MappedValues ()).c_str ());
#endif
        return sDiscoveredDevices_.cget ()->MappedValues (); // intentionally object-spice
    });
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace (L"returns: %s", Characters::ToString (results).c_str ());
#endif
    return results;
}
