/*
* Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Cache/SynchronizedCallerStalenessCache.h"
#include "Stroika/Foundation/Cache/SynchronizedTimedCache.h"
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
#include "Stroika/Foundation/IO/Network/Neighbors.h"
#include "Stroika/Foundation/IO/Network/Transfer/Client.h"
#include "Stroika/Foundation/IO/Network/Transfer/Connection.h"

#include "Stroika/Frameworks/UPnP/DeviceDescription.h"
#include "Stroika/Frameworks/UPnP/SSDP/Client/Listener.h"
#include "Stroika/Frameworks/UPnP/SSDP/Client/Search.h"

#include "../Common/EthernetMACAddressOUIPrefixes.h"

#include "NetworkInterfaces.h"

#include "Devices.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::DataExchange;
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
        StringBuilder sb;
        {
            // try using hardware addresses
            for (auto i : SortedSet<String>{d.GetHardwareAddresses ()}) {
                sb += i;
            }
        }
        if (sb.empty ()) {
            SortedSet<InternetAddress> x{d.GetInternetAddresses ()};
            if (not x.empty ()) {
                sb += x.Nth (0).As<String> ();
            }
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

namespace {
    optional<String> ReverseDNSLookup_ (const InternetAddress& inetAddr)
    {
        static const Time::Duration                                             kCacheTTL_{5min}; // @todo fix when Stroika Duration bug supports constexpr this should
        static Cache::SynchronizedTimedCache<InternetAddress, optional<String>> sCache_{kCacheTTL_};
        return sCache_.LookupValue (inetAddr, [] (const InternetAddress& inetAddr) {
            static const DNS kDNS_ = DNS::Default ();
            return kDNS_.ReverseLookup (inetAddr);
        });
    }
    Set<InternetAddress> DNSLookup_ (const String& hostOrIPAddress)
    {
        static const Time::Duration                                        kCacheTTL_{5min}; // @todo fix when Stroika Duration bug supports constexpr this should
        static Cache::SynchronizedTimedCache<String, Set<InternetAddress>> sCache_{kCacheTTL_};
        return sCache_.LookupValue (hostOrIPAddress, [] (const String& hostOrIPAddress) -> Set<InternetAddress> {
            static const DNS kDNS_ = DNS::Default ();
            return Set<InternetAddress>{kDNS_.GetHostAddresses (hostOrIPAddress)};
        });
    }
}

/*
 ********************************************************************************
 ******************* Discovery::NetworkAttachmentInfo ***************************
 ********************************************************************************
 */
String NetworkAttachmentInfo::ToString () const
{
    StringBuilder sb;
    sb += L"{";
    sb += L"hardwareAddress: " + Characters::ToString (hardwareAddresses) + L", ";
    sb += L"networkAddresses: " + Characters::ToString (networkAddresses) + L", ";
    sb += L"}";
    return sb.str ();
}

/*
 ********************************************************************************
 ******************************* Discovery::Device ******************************
 ********************************************************************************
 */
Set<String> Discovery::Device::GetHardwareAddresses () const
{
    Set<String> result;
    for (auto iNet : fAttachedNetworks) {
        result += iNet.fValue.hardwareAddresses;
    }
    return result;
}

Set<InternetAddress> Discovery::Device::GetInternetAddresses () const
{
    Set<InternetAddress> result;
    for (auto iNet : fAttachedNetworks) {
        result += iNet.fValue.networkAddresses;
    }
    return result;
}

String Discovery::Device::ToString () const
{
    StringBuilder sb;
    sb += L"{";
    sb += L"GUID: " + Characters::ToString (fGUID) + L", ";
    sb += L"name: " + Characters::ToString (name) + L", ";
    sb += L"icon: " + Characters::ToString (fIcon) + L", ";
    sb += L"manufacturer: " + Characters::ToString (fManufacturer) + L", ";
    sb += L"types: " + Characters::ToString (fTypes) + L", ";
    if (fThisDevice) {
        sb += L"This-Device: " + Characters::ToString (fThisDevice) + L", ";
    }
    sb += L"attachedNetworks: " + Characters::ToString (fAttachedNetworks) + L", ";
    sb += L"attachedInterfaces: " + Characters::ToString (fAttachedInterfaces) + L", ";
    sb += L"presentationURL: " + Characters::ToString (fPresentationURL) + L", ";
    sb += L"operatingSystem: " + Characters::ToString (fOperatingSystem) + L", ";
#if qDebug
    sb += L"debugProps: " + Characters::ToString (fDebugProps) + L", ";
#endif
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
        Set<GUID> LookupNetworksGUIDs (const Iterable<InternetAddress>& ia) const
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
            Set<URI>                fLocations;
            optional<String>        fServer;
            optional<String>        fManufacturer;
            optional<URI>           fManufacturerURI;
            Mapping<String, String> fDeviceType2FriendlyNameMap; //  http://upnp.org/specs/arch/UPnP-arch-DeviceArchitecture-v1.1.pdf - <deviceType> - Page 44
            optional<URI>           fPresentationURL;
            Time::DateTime          fLastSSDPMessageRecievedAt{Time::DateTime::Now ()};
#if qDebug
            optional<SSDP::Advertisement> fLastAdvertisement;
#endif

            String ToString () const
            {
                StringBuilder sb;
                sb += L"{";
                sb += L"fUSNs: " + Characters::ToString (fUSNs) + L", ";
                sb += L"fAlive: " + Characters::ToString (fAlive) + L", ";
                sb += L"fLocations: " + Characters::ToString (fLocations) + L", ";
                sb += L"fManufacturer: " + Characters::ToString (fManufacturer) + L", ";
                sb += L"fManufacturerURI: " + Characters::ToString (fManufacturerURI) + L", ";
                sb += L"fServer: " + Characters::ToString (fServer) + L", ";
                sb += L"Device-Type-2-Friendly-Name-Map: " + Characters::ToString (fDeviceType2FriendlyNameMap) + L", ";
                sb += L"fPresentationURL: " + Characters::ToString (fPresentationURL) + L", ";
                sb += L"fLastSSDPMessageRecievedAt: " + Characters::ToString (fLastSSDPMessageRecievedAt) + L", ";
#if qDebug
                sb += L"fLastAdvertisement: " + Characters::ToString (fLastAdvertisement) + L", ";
#endif
                sb += L"}";
                return sb.str ();
            }
        };
        optional<SSDPInfo> fSSDPInfo;

        void AddIPAddresses_ (const InternetAddress& addr, const optional<String>& hwAddr = nullopt)
        {
            // merge the addrs into each matching network interface
            AddIPAddresses_ (Iterable<InternetAddress>{addr}, hwAddr);
        }
        void AddIPAddresses_ (const Iterable<InternetAddress>& addrs, const optional<String>& hwAddr = nullopt)
        {
            // merge the addrs into each matching network interface
            for (GUID nw : NetAndNetInterfaceMapper_::sThe.LookupNetworksGUIDs (addrs)) {
                NetworkAttachmentInfo nwAttachmentInfo = fAttachedNetworks.LookupValue (nw);
                nwAttachmentInfo.networkAddresses += addrs;
                if (hwAddr) {
                    nwAttachmentInfo.hardwareAddresses += *hwAddr;
                }
                fAttachedNetworks.Add (nw, nwAttachmentInfo);
            }
        }

        void PatchDerivedFields ()
        {
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
                }

                fPresentationURL = fSSDPInfo->fPresentationURL;
            }
            if (name.empty ()) {
                // try reverse dns lookup
                for (auto i : GetInternetAddresses ()) {
                    if (auto o = ReverseDNSLookup_ (i)) {
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

            {
                // See if its addresses intersect with any network gateways - if so - its a router
                Set<InternetAddress> gateways;
                for (GUID netGUID : fAttachedNetworks.Keys ()) {
                    gateways += NetworksMgr::sThe.GetNetworkByID (netGUID).fGateways;
                }
                if (not(gateways ^ GetInternetAddresses ()).empty ()) {
                    fTypes.Add (Discovery::DeviceType::eRouter);
                }
            }

            // Manufacturer info
            if (fSSDPInfo and fSSDPInfo->fManufacturer) {
                Manufacturer m = fManufacturer.value_or (Manufacturer{});
                m.fFullName    = fSSDPInfo->fManufacturer;
                fManufacturer  = m;
            }
            if (fSSDPInfo and fSSDPInfo->fManufacturerURI) {
                Manufacturer m = fManufacturer.value_or (Manufacturer{});
                m.fWebSiteURL  = fSSDPInfo->fManufacturerURI;
                fManufacturer  = m;
            }
            if (not fManufacturer or not fManufacturer->fFullName or not fManufacturer->fShortName) {
                for (auto hwa : GetHardwareAddresses ()) {
                    if (auto o = BackendApp::Common::LookupEthernetMACAddressOUIFromPrefix (hwa)) {
                        if (not fManufacturer) {
                            fManufacturer = Manufacturer{};
                        }
                        bool longName = o->Contains (L" "); //  primitive guess
                        if (longName) {
                            if (not fManufacturer->fFullName) {
                                fManufacturer->fFullName = *o;
                            }
                        }
                        else {
                            if (not fManufacturer->fShortName) {
                                fManufacturer->fShortName = *o;
                            }
                        }
                    }
                }
            }

#if qDebug
            if (fSSDPInfo) {
                fDebugProps.Add (L"SSDPInfo"sv,
                                 VariantValue{
                                     Mapping<String, VariantValue>{
                                         pair<String, VariantValue>{L"deviceType2FriendlyNameMap"sv, Characters::ToString (fSSDPInfo->fDeviceType2FriendlyNameMap)},
                                         pair<String, VariantValue>{L"USNs"sv, Characters::ToString (fSSDPInfo->fUSNs)},
                                         pair<String, VariantValue>{L"server"sv, Characters::ToString (fSSDPInfo->fServer)},
                                         pair<String, VariantValue>{L"manufacturer"sv, Characters::ToString (fSSDPInfo->fManufacturer)},
                                         pair<String, VariantValue>{L"manufacturer-URL"sv, Characters::ToString (fSSDPInfo->fManufacturerURI)},
                                         pair<String, VariantValue>{L"lastAdvertisement"sv, Characters::ToString (fSSDPInfo->fLastAdvertisement)},
                                         pair<String, VariantValue>{L"lastSSDPMessageRecievedAt"sv, Characters::ToString (fSSDPInfo->fLastSSDPMessageRecievedAt)},
                                         pair<String, VariantValue>{L"locations"sv, Characters::ToString (fSSDPInfo->fLocations)}}});
            }

#endif

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
            if (not(d.GetInternetAddresses () ^ di.GetInternetAddresses ()).empty ()) {
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
                    if (optional<DiscoveryInfo_> o = GetMyDevice_ ()) {
                        auto           l  = sDiscoveredDevices_.rwget ();
                        DiscoveryInfo_ di = [&] () {
                            DiscoveryInfo_ tmp{};
                            tmp.fAttachedNetworks = o->fAttachedNetworks;
                            if (optional<DiscoveryInfo_> oo = FindMatchingDevice_ (l, tmp)) {
                                return *oo;
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
                        di.fOperatingSystem = OperatingSystem{Configuration::GetSystemConfiguration_ActualOperatingSystem ().fPrettyNameWithVersionDetails};
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
                    i.fBindings.Apply ([&] (const Interface::Binding& ia) {
                        if (not kIncludeMulticastAddressesInDiscovery and ia.fInternetAddress.IsMulticastAddress ()) {
                            return;
                        }
                        newDev.AddIPAddresses_ (ia.fInternetAddress, i.fHardwareAddress);
                    });
                }
            }
            newDev.fAttachedInterfaces = Set<GUID>{Discovery::NetworkInterfacesMgr::sThe.CollectAllNetworkInterfaces ().Select<GUID> ([] (auto iFace) { return iFace.fGUID; })};
            newDev.fGUID               = LookupPersistentDeviceID_ (newDev);
#if USE_NOISY_TRACE_IN_THIS_MODULE_
            DbgTrace (L"returning: %s", Characters::ToString (newDev).c_str ());
#endif
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
                fListener_ = make_unique<SSDP::Client::Listener> (
                    [this] (const SSDP::Advertisement& d) { this->RecieveSSDPAdvertisement_ (d); },
                    SSDP::Client::Listener::eAutoStart);
            }
            catch (...) {
                Logger::Get ().Log (Logger::Priority::eError, L"Problem starting SSDP Listener - so that source of discovery will be unavailable: %s", Characters::ToString (current_exception ()).c_str ());
            }
            try {
                static const Time::Duration kReSearchInterval_{30min}; // not sure what interval makes sense
                fSearcher_ = make_unique<SSDP::Client::Search> (
                    [this] (const SSDP::Advertisement& d) { this->RecieveSSDPAdvertisement_ (d); },
                    SSDP::Client::Search::kRootDevice, kReSearchInterval_);
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

            Set<InternetAddress> locAddrs;
            if (d.fLocation.GetAuthority () and d.fLocation.GetAuthority ()->GetHost ()) {
                URI::Host h = *d.fLocation.GetAuthority ()->GetHost ();
                if (h.AsInternetAddress ()) {
                    locAddrs = Set<InternetAddress>{*h.AsInternetAddress ()};
                }
                else {
                    locAddrs = DNSLookup_ (*h.AsRegisteredName ());
                }
            }

            // @todo - Maintain cache with age apx 60 minutes - mapping URL to UPnP::DeviceDescription objects

            /// @todo - Add Search support (once at startup, and then every 10 minutes? - config) - because it maybe some devices dont properly
            /// broadcast, and only respond to search, plus gives better immediate feedback when we first start up (at least helpful for debugging)
            optional<String> deviceFriendlyName;
            optional<String> deviceType;
            optional<String> manufactureName;
            optional<URI>    manufacturerURL;
            optional<URI>    presentationURL;
            optional<URI>    deviceIconURL;

            try {
                using namespace IO::Network::Transfer;
                Connection::Ptr c = Connection::New ();
                Response        r = c.GET (d.fLocation);
                if (r.GetSucceeded ()) {
                    Frameworks::UPnP::DeviceDescription deviceInfo = DeSerialize (r.GetData ());
                    deviceFriendlyName                             = deviceInfo.fFriendlyName;
                    deviceType                                     = deviceInfo.fDeviceType;
                    manufactureName                                = deviceInfo.fManufactureName;
                    manufacturerURL                                = deviceInfo.fManufacturingURL;
                    presentationURL                                = deviceInfo.fPresentationURL;
                    if (deviceInfo.fIcons.has_value () and not deviceInfo.fIcons->empty ()) {
                        deviceIconURL = d.fLocation.Combine (deviceInfo.fIcons->Nth (0).fURL);
                    }
                    if (manufacturerURL.has_value ()) {
                        manufacturerURL = d.fLocation.Combine (*manufacturerURL);
                    }
                    DbgTrace (L"Found device description = %s", Characters::ToString (deviceInfo).c_str ());
                }
            }
            catch (...) {
                DbgTrace (L"Failed to fetch description: %s", Characters::ToString (current_exception ()).c_str ());
            }

            if (locAddrs.empty ()) {
                DbgTrace (L"oops - bad - probably should log - bad device response - find addr some other way");
            }
            else {
                // merge in data
                auto           l  = sDiscoveredDevices_.rwget ();
                DiscoveryInfo_ di = [&] () {
                    DiscoveryInfo_ tmp{};
                    tmp.AddIPAddresses_ (locAddrs);
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

                Memory::CopyToIf (deviceIconURL, &di.fIcon);
                Memory::CopyToIf (manufacturerURL, &di.fSSDPInfo->fManufacturerURI);

                di.fSSDPInfo->fLocations.Add (d.fLocation);
                di.fSSDPInfo->fUSNs.Add (d.fUSN);

                if (presentationURL) {
                    // consider if value already there - warn if changes - should we collect multiple
                    di.fSSDPInfo->fPresentationURL = presentationURL;
                }

                if (di.fSSDPInfo->fServer.has_value () and di.fSSDPInfo->fServer != d.fServer) {
                    DbgTrace (L"Warning: different server IDs for same object");
                }
                di.fSSDPInfo->fServer = d.fServer;

                di.AddIPAddresses_ (locAddrs);
                if (deviceType and deviceFriendlyName) {
                    di.fSSDPInfo->fDeviceType2FriendlyNameMap.Add (*deviceType, *deviceFriendlyName);
                }
                Memory::CopyToIf (manufactureName, &di.fSSDPInfo->fManufacturer);

                di.fSSDPInfo->fLastSSDPMessageRecievedAt = Time::DateTime::Now (); // update each message, even if already created

#if qDebug
                di.fSSDPInfo->fLastAdvertisement = d;
#endif

                if (not di.fOperatingSystem.has_value ()) {
                    if (di.fSSDPInfo->fServer and di.fSSDPInfo->fServer->Contains (L"Linux")) {
                        di.fOperatingSystem = Discovery::OperatingSystem{L"Linux"};
                    }
                    else if (di.fSSDPInfo->fServer and di.fSSDPInfo->fServer->Contains (L"POSIX")) {
                        di.fOperatingSystem = Discovery::OperatingSystem{L"POSIX"};
                    }
                }
                di.PatchDerivedFields ();
                l->Add (di.fGUID, di);
            }
        }
    };

    unique_ptr<SSDPDeviceDiscoverer_> sSSDPDeviceDiscoverer_;
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
            using Neighbor = NeighborsMonitor::Neighbor;
            NeighborsMonitor monitor{};
            while (true) {
                try {
                    DeclareActivity da{&kDiscovering_NetNeighbors_};
                    for (Neighbor i : monitor.GetNeighbors ()) {
                        // soon store/pay attention to macaddr as better indicator of unique device id than ip addr

                        // ignore multicast addresses as they are not real devices(???always???)
                        if (i.fInternetAddress.IsMulticastAddress ()) {
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
                        DiscoveryInfo_ di = [&] () {
                            DiscoveryInfo_ tmp{};
                            tmp.AddIPAddresses_ (i.fInternetAddress, i.fHardwareAddress);
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

    // Really always want all true, just add ability to turn some off to ease debugging
    constexpr bool kInclude_SSDP_Discoverer_{true};
    constexpr bool kInclude_MyDevice_Discoverer_{true};
    constexpr bool kInclude_Neighbor_Discoverer_{true};

    bool IsActive_ ()
    {
        if constexpr (kInclude_MyDevice_Discoverer_ and kInclude_SSDP_Discoverer_) {
            Require (static_cast<bool> (sMyDeviceDiscoverer_) == static_cast<bool> (sSSDPDeviceDiscoverer_));
        }
        if constexpr (kInclude_MyDevice_Discoverer_ and kInclude_Neighbor_Discoverer_) {
            Require (static_cast<bool> (sMyDeviceDiscoverer_) == static_cast<bool> (sNeighborDiscoverer_));
        }
        if constexpr (kInclude_SSDP_Discoverer_ and kInclude_Neighbor_Discoverer_) {
            Require (static_cast<bool> (sSSDPDeviceDiscoverer_) == static_cast<bool> (sNeighborDiscoverer_));
        }
        if constexpr (kInclude_SSDP_Discoverer_) {
            return sSSDPDeviceDiscoverer_ != nullptr;
        }
        if constexpr (kInclude_MyDevice_Discoverer_) {
            return sMyDeviceDiscoverer_ != nullptr;
        }
        if constexpr (kInclude_Neighbor_Discoverer_) {
            return sNeighborDiscoverer_ != nullptr;
        }
    }
}

Discovery::DevicesMgr::Activator::Activator ()
{
    DbgTrace (L"Discovery::DevicesMgr::Activator::Activator: activating device discovery");
    Require (not IsActive_ ());
    if constexpr (kInclude_SSDP_Discoverer_) {
        sSSDPDeviceDiscoverer_ = make_unique<SSDPDeviceDiscoverer_> ();
    }
    if constexpr (kInclude_MyDevice_Discoverer_) {
        sMyDeviceDiscoverer_ = make_unique<MyDeviceDiscoverer_> ();
    }
    if constexpr (kInclude_Neighbor_Discoverer_) {
        sNeighborDiscoverer_ = make_unique<MyNeighborDiscoverer_> ();
    }
}

Discovery::DevicesMgr::Activator::~Activator ()
{
    DbgTrace (L"Discovery::DevicesMgr::Activator::~Activator: deactivating device discovery");
    Require (IsActive_ ());
    if constexpr (kInclude_SSDP_Discoverer_) {
        sSSDPDeviceDiscoverer_.reset ();
    }
    if constexpr (kInclude_MyDevice_Discoverer_) {
        sMyDeviceDiscoverer_.reset ();
    }
    if constexpr (kInclude_Neighbor_Discoverer_) {
        sNeighborDiscoverer_.reset ();
    }
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
    results = sCache_.LookupValue (sCache_.Ago (allowedStaleness.value_or (kDefaultItemCacheLifetime_)), [] () {
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
