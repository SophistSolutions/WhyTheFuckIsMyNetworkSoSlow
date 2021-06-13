/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include <random>

#include "Stroika/Foundation/Cache/BloomFilter.h"
#include "Stroika/Foundation/Cache/SynchronizedCallerStalenessCache.h"
#include "Stroika/Foundation/Cache/SynchronizedTimedCache.h"
#include "Stroika/Foundation/Characters/Format.h"
#include "Stroika/Foundation/Characters/RegularExpression.h"
#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Configuration/SystemConfiguration.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Cryptography/Digest/Algorithm/MD5.h"
#include "Stroika/Foundation/Cryptography/Digest/Hash.h"
#include "Stroika/Foundation/Cryptography/Format.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Execution/Activity.h"
#include "Stroika/Foundation/Execution/Logger.h"
#include "Stroika/Foundation/Execution/Sleep.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/Execution/Thread.h"
#include "Stroika/Foundation/IO/Network/DNS.h"
#include "Stroika/Foundation/IO/Network/HTTP/ClientErrorException.h"
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/LinkMonitor.h"
#include "Stroika/Foundation/IO/Network/Neighbors.h"
#include "Stroika/Foundation/IO/Network/Transfer/Connection.h"
#include "Stroika/Foundation/Traversal/DiscreteRange.h"

#include "Stroika/Frameworks/NetworkMonitor/Ping.h"
#include "Stroika/Frameworks/UPnP/DeviceDescription.h"
#include "Stroika/Frameworks/UPnP/SSDP/Client/Listener.h"
#include "Stroika/Frameworks/UPnP/SSDP/Client/Search.h"

#include "../Common/EthernetMACAddressOUIPrefixes.h"

#include "NetworkInterfaces.h"
#include "PortScanner.h"

#include "Devices.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::DataExchange;
using namespace Stroika::Foundation::Execution;
using namespace Stroika::Foundation::IO;
using namespace Stroika::Foundation::IO::Network;
using namespace Stroika::Foundation::Traversal;

using namespace Stroika::Frameworks;
using namespace Stroika::Frameworks::UPnP;

using Execution::RWSynchronized;
using IO::Network::InternetAddress;
using Stroika::Foundation::Common::GUID;
using Stroika::Foundation::Common::KeyValuePair;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Discovery;

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

// Turn on to debug issues with lock contention and upgradelocks
//#define qLOCK_DEBUGGING_ 1

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
        using namespace Stroika::Foundation::Cryptography::Digest;
        return Hash<String, Digester<Algorithm::MD5>, GUID>{}(sb.str ());
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
    const String kDeviceType_DIALServer_{L"urn:dial-multiscreen-org:device:dial:1"sv}; // typically TV, Blu-ray player, set-top-box, or similar device
    const String kDeviceType_DIALReceiver_{L"urn:dial-multiscreen-org:device:dialreceiver:1"sv};
    // probably shouldn't be this specifc
    const String kDeviceType_Roku_{L"urn:roku-com:device:player:1-0"sv};
}

namespace {
    optional<String> ReverseDNSLookup_ (const InternetAddress& inetAddr)
    {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
        Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"{}::ReverseDNSLookup_", L"inetAddr=%s", Characters::ToString (inetAddr).c_str ())};
#endif
        static const Time::Duration                                             kCacheTTL_{5min}; // @todo fix when Stroika Duration bug supports constexpr this should
        static Cache::SynchronizedTimedCache<InternetAddress, optional<String>> sCache_{kCacheTTL_};
        try {
            return sCache_.LookupValue (inetAddr, [] (const InternetAddress& inetAddr) {
                static const DNS kDNS_ = DNS::Default ();
                return kDNS_.ReverseLookup (inetAddr);
            });
        }
        catch (...) {
            return nullopt; // if DNS is failing, just dont do this match, dont abandon all data collection
        }
    }
    Set<InternetAddress> DNSLookup_ (const String& hostOrIPAddress)
    {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
        Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"{}::DNSLookup_", L"hostOrIPAddress=%s", Characters::ToString (hostOrIPAddress).c_str ())};
#endif
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
    sb += L"localAddresses: " + Characters::ToString (localAddresses);
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
        result += iNet.fValue.localAddresses;
    }
    return result;
}

Sequence<InternetAddress> Discovery::Device::GetPreferredDisplayInternetAddresses () const
{
    /*
     *  Try to pick the best IP Addrs, and put them first.
     *
     *  Prefer IPv4, @todo maybe just suppress IPV6 addresses IF they match IPv4
     * 
     * @todo suppress choices for inactive networks
     */
    Set<InternetAddress> result;
    for (auto iNet : fAttachedNetworks) {
        for (auto aNetAddr : iNet.fValue.localAddresses) {
            if (aNetAddr.GetAddressFamily () == InternetAddress::AddressFamily::V4) {
                result += aNetAddr;
            }
            break;
        }
    }
    if (result.empty ()) {
        for (auto iNet : fAttachedNetworks) {
            for (auto aNetAddr : iNet.fValue.localAddresses) {
                result += aNetAddr;
                break;
            }
        }
    }
    return Sequence<InternetAddress>{result};
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
    sb += L"debugProps: " + Characters::ToString (fDebugProps);
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
            Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"{}::NetAndNetInterfaceMapper_::LookupNetworksGUIDs", L"ia=%s", Characters::ToString (ia).c_str ())};
#endif
            Set<GUID> results;
            for (Discovery::Network&& nw : Discovery::NetworksMgr::sThe.CollectActiveNetworks ()) {
                for (InternetAddress i : ia) {
                    if (nw.Contains (i)) {
                        results += nw.fGUID;
                    }
                }
            }
#if USE_NOISY_TRACE_IN_THIS_MODULE_
            DbgTrace (L"returning %s", Characters::ToString (results).c_str ());
#endif
            return results;
        }
        Set<GUID> LookupNetworksGUIDs (const InternetAddress& ia) const
        {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
            Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"{}::NetAndNetInterfaceMapper_::LookupNetworksGUIDs", L"ia=%s", Characters::ToString (ia).c_str ())};
#endif
            Set<GUID> results;
            for (Discovery::Network&& nw : Discovery::NetworksMgr::sThe.CollectActiveNetworks ()) {
                if (nw.Contains (ia)) {
                    results += nw.fGUID;
                }
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

DISABLE_COMPILER_MSC_WARNING_START (4573);
DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
const ObjectVariantMapper kMyMapper_ = [] () {
    using Stroika::Frameworks::UPnP::SSDP::Advertisement;
    ObjectVariantMapper mapper;
    mapper.AddCommonType<Set<URI>> ();
    mapper += Advertisement::kMapper;
    mapper.AddCommonType<optional<Advertisement>> ();
    return mapper;
}();
DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
DISABLE_COMPILER_MSC_WARNING_END (4573);

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

#if __cpp_impl_three_way_comparison < 201711
            bool operator== (const SSDPInfo& rhs) const
            {
                if (fAlive != rhs.fAlive) {
                    return false;
                }
                if (fUSNs != rhs.fUSNs) {
                    return false;
                }
                if (fLocations != rhs.fLocations) {
                    return false;
                }
                if (fServer != rhs.fServer) {
                    return false;
                }
                if (fManufacturer != rhs.fManufacturer) {
                    return false;
                }
                if (fManufacturerURI != rhs.fManufacturerURI) {
                    return false;
                }
                if (fDeviceType2FriendlyNameMap != rhs.fDeviceType2FriendlyNameMap) {
                    return false;
                }
                if (fPresentationURL != rhs.fPresentationURL) {
                    return false;
                }
                if (fLastSSDPMessageRecievedAt != rhs.fLastSSDPMessageRecievedAt) {
                    return false;
                }
#if qDebug
                if (fLastAdvertisement != rhs.fLastAdvertisement) {
                    return false;
                }
#endif
                return true;
            }
            bool operator!= (const SSDPInfo& rhs) const
            {
                return not(*this == rhs);
            }
#else
            auto operator<=> (const SSDPInfo&) const = default;
#endif

            String ToString () const
            {
                StringBuilder sb;
                sb += L"{";
                sb += L"USNs: " + Characters::ToString (fUSNs) + L", ";
                sb += L"Alive: " + Characters::ToString (fAlive) + L", ";
                sb += L"Locations: " + Characters::ToString (fLocations) + L", ";
                sb += L"Manufacturer: " + Characters::ToString (fManufacturer) + L", ";
                sb += L"Manufacturer-URI: " + Characters::ToString (fManufacturerURI) + L", ";
                sb += L"Server: " + Characters::ToString (fServer) + L", ";
                sb += L"Device-Type-2-Friendly-Name-Map: " + Characters::ToString (fDeviceType2FriendlyNameMap) + L", ";
                sb += L"Presentation-URL: " + Characters::ToString (fPresentationURL) + L", ";
                sb += L"Last-SSDP-Message-Recieved-At: " + Characters::ToString (fLastSSDPMessageRecievedAt) + L", ";
#if qDebug
                sb += L"Last-Advertisement: " + Characters::ToString (fLastAdvertisement);
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
            unsigned int totalAddrs{};
            unsigned int totalAddrsSuppressedQuietly{};
            unsigned int totalAdds{};
            for (InternetAddress ia : addrs) {
                totalAddrs++;
                if (not kIncludeMulticastAddressesInDiscovery and ia.IsMulticastAddress ()) {
                    totalAddrsSuppressedQuietly++;
                    continue;
                }
                if (not kIncludeLinkLocalAddressesInDiscovery) {
                    if (ia.IsLinkLocalAddress ()) {
                        totalAddrsSuppressedQuietly++;
                        continue; // skip link-local addresses, they are only used for special purposes like discovery, and aren't part of the network
                    }
                }
                for (GUID nw : NetAndNetInterfaceMapper_::sThe.LookupNetworksGUIDs (ia)) {
                    NetworkAttachmentInfo nwAttachmentInfo = fAttachedNetworks.LookupValue (nw);
                    nwAttachmentInfo.localAddresses += ia;
                    if (hwAddr) {
                        nwAttachmentInfo.hardwareAddresses += *hwAddr;
                    }
                    fAttachedNetworks.Add (nw, nwAttachmentInfo);
                    totalAdds++;
                }
            }
            if (totalAdds == 0 and totalAddrsSuppressedQuietly < totalAddrs) {
                DbgTrace (L"AddIPAddresses_(%s) called, but no matching interfaces found", Characters::ToString (addrs).c_str ());
            }
        }

        void PatchDerivedFields ()
        {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
            Debug::TraceContextBumper ctx{"{}::DiscoveryInfo_::PatchDerivedFields"};
#endif

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
                        String speakerGroup = fSSDPInfo->fDeviceType2FriendlyNameMap[kDeviceType_SpeakerGroup_];
                        if (speakerGroup.empty ()) {
                            Assert (m2->length () >= 3); // because matched SPACE-SPACE.*
                            name = m2->SubString (3);
                        }
                        else {
                            name = speakerGroup + *m2;
                        }
#if qDebug
                        fDebugProps.Add (L"SSDP-DeviceType2FriendlyName-SONOS-HACK"sv, name);
#endif
                    }
                }

                // pick any one of the friendly names, or the server name if we must
                if (name.empty ()) {
                    if (not fSSDPInfo->fDeviceType2FriendlyNameMap.empty ()) {
                        name = fSSDPInfo->fDeviceType2FriendlyNameMap.Nth (0).fValue;
#if qDebug
                        fDebugProps.Add (L"SSDP-DeviceType2FriendlyName"sv, name);
#endif
                    }
                }

                fPresentationURL = fSSDPInfo->fPresentationURL;
            }
            if (name.empty ()) {
                // try reverse dns lookup
                for (auto i : GetInternetAddresses ()) {
                    if (auto o = ReverseDNSLookup_ (i)) {
                        name = *o;
#if qDebug
                        fDebugProps.Add (L"reverse-dns-name"sv, name);
#endif
                        break;
                    }
                }
            }
            if (name.empty ()) {
                constexpr bool kUseFirstIPAddrIfUnknown_ = true;
                if (kUseFirstIPAddrIfUnknown_) {
                    name = GetPreferredDisplayInternetAddresses ().Join ();
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

            // I believe a media renderer can be EITHER a full TV, or just something with speakers...
            if (fSSDPInfo.has_value () and
                // @todo - need a better way to detect - look at services not device type?
                (fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_MediaRenderer_))) {
                fTypes.Add (Discovery::DeviceType::eSpeaker);
            }

            if (fSSDPInfo.has_value () and
                (
                    // @todo - need a better way to detect - look at services not device type?
                    fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_Roku_))) {
                fTypes.Add (Discovery::DeviceType::eMediaPlayer);
            }

            // So far only seen used for Amazon Fire Stick, but could be used for TV, according to
            // http://www.dial-multiscreen.org/dial-protocol-specification
            if (fSSDPInfo.has_value () and
                (fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_DIALServer_))) {
                // @todo look more closely at firestick - I think dialserver isnt neceesarily a media player
                fTypes.Add (Discovery::DeviceType::eMediaPlayer);
                //fTypes.Add (Discovery::DeviceType::eTV);
            }

            if (fSSDPInfo.has_value () and
                (fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_DIALReceiver_))) {
                //fTypes.Add (Discovery::DeviceType::eMediaPlayer);
                fTypes.Add (Discovery::DeviceType::eTV);
            }

            {
                // See if its addresses intersect with any network gateways - if so - its a router
                Set<InternetAddress> gateways;
                for (GUID netGUID : fAttachedNetworks.Keys ()) {
                    IgnoreExceptionsExceptThreadAbortForCall (gateways += NetworksMgr::sThe.GetNetworkByID (netGUID).fGateways); // if network disappears dont fail to patch
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

            // Add various type(s) if hardware address matches and other open ports fields
            {
                static const String kSMBPort_ = Characters::Format (L"tcp:%d", IO::Network::WellKnownPorts::TCP::kSMB);
                for (auto hwa : GetHardwareAddresses ()) {
                    if (auto o = BackendApp::Common::LookupEthernetMACAddressOUIFromPrefix (hwa)) {
                        if (o == L"Oracle VirtualBox virtual NIC"sv) {
                            fTypes.Add (Discovery::DeviceType::eVirtualMachine);
                        }
                        if ((o == L"Synology Incorporated"sv or o == L"Buffalo.inc"sv or o == L"Seagate Technology"sv or o == L"Seagate Cloud Systems"sv) and fOpenPorts and fOpenPorts->Contains (kSMBPort_)) {
                            fTypes.Add (Discovery::DeviceType::eNetworkAttachedStorage);
                        }
                    }
                }
            }

            static const String kIPPPort_ = Characters::Format (L"tcp:%d", IO::Network::WellKnownPorts::TCP::kIPP);
            static const String kLPDPort_ = Characters::Format (L"tcp:%d", IO::Network::WellKnownPorts::TCP::kLPD);
            if (fOpenPorts and (fOpenPorts->Contains (kIPPPort_) or fOpenPorts->Contains (kLPDPort_)) and (fManufacturer and (fManufacturer->Contains (L"Hewlett Packard"_k) or fManufacturer->Contains (L"Epson"_k) or fManufacturer->Contains (L"Canon"_k) or fManufacturer->Contains (L"Brother"_k)))) {
                fTypes.Add (DeviceType::ePrinter);
            }

#if qDebug
            if (fSSDPInfo) {
                fDebugProps.Add (L"SSDPInfo"sv,
                                 VariantValue{
                                     Mapping<String, VariantValue> {
                                         pair<String, VariantValue>{L"deviceType2FriendlyNameMap"sv, Mapping<String, VariantValue> { fSSDPInfo->fDeviceType2FriendlyNameMap }},
                                         pair<String, VariantValue>{L"USNs"sv, kMyMapper_.FromObject (fSSDPInfo->fUSNs)},
                                         pair<String, VariantValue>{L"server"sv, fSSDPInfo->fServer},
                                         pair<String, VariantValue>{L"manufacturer"sv, fSSDPInfo->fManufacturer},
                                         pair<String, VariantValue>{L"manufacturer-URL"sv, kMyMapper_.FromObject (fSSDPInfo->fManufacturerURI)},
                                         pair<String, VariantValue>{L"lastAdvertisement"sv, kMyMapper_.FromObject (fSSDPInfo->fLastAdvertisement)},
                                         pair<String, VariantValue>{L"lastSSDPMessageRecievedAt"sv, fSSDPInfo->fLastSSDPMessageRecievedAt},
                                         pair<String, VariantValue> { L"locations"sv,
                                                                      kMyMapper_.FromObject (fSSDPInfo->fLocations) }
                                     }});
            }
#endif

#if USE_NOISY_TRACE_IN_THIS_MODULE_
            DbgTrace (L"At end of PatchDerivedFields: %s", ToString ().c_str ());
#endif
        }

#if __cpp_impl_three_way_comparison < 201711
        bool operator== (const DiscoveryInfo_& rhs) const
        {
            if (not Discovery::Device::operator== (rhs)) {
                return false;
            }
            if (fForcedName != rhs.fForcedName) {
                return false;
            }
            if (fSSDPInfo != rhs.fSSDPInfo) {
                return false;
            }
            return true;
        }
        bool operator!= (const DiscoveryInfo_& rhs) const
        {
            return not(*this == rhs);
        }
#else
        auto operator<=> (const DiscoveryInfo_&) const = default;
#endif

        String ToString () const
        {
            StringBuilder sb = Discovery::Device::ToString ().SubString (0, -1);
            if (fForcedName) {
                sb += L"Forced-Name: " + Characters::ToString (fForcedName) + L", ";
            }
            sb += L"SSDP-Info: " + Characters::ToString (fSSDPInfo) + L", ";
            sb += L"}";
            return sb.str ();
        }
    };
    // NB: RWSynchronized because most accesses will be to read/lookup in this list; use Mapping<> because KeyedCollection NYI
    // Note, when we first start, there will be more contention, so we'll get conflicts (and we dbgtrace log them to be sure
    // not too many).
#if qLOCK_DEBUGGING_
    Synchronized<Mapping<GUID, DiscoveryInfo_>, Tracing_Synchronized_Traits<shared_timed_mutex>> sDiscoveredDevices_;
#else
    RWSynchronized<Mapping<GUID, DiscoveryInfo_>> sDiscoveredDevices_;
#endif

// turn on tracking of locks on sDiscoveredDevices_
#if qDefaultTracingOn && qLOCK_DEBUGGING_
    int ignored = [] () {
        sDiscoveredDevices_.fDbgTraceLocksName = L"DiscoveredDevices_";
        return 0;
    }();
#endif

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
            : fMyDeviceDiscovererThread_{
                  Thread::CleanupPtr::eAbortBeforeWaiting, Thread::New (DiscoveryChecker_, Thread::eAutoStart, L"MyDeviceDiscoverer"_k)}
        {
        }

    private:
        static void DiscoveryChecker_ ()
        {
            static constexpr Activity kDiscovering_This_Device_{L"discovering this device"sv};
            unsigned int              retriedLockCount = 0;
            while (true) {
                try {
                    DeclareActivity da{&kDiscovering_This_Device_};
                    if (optional<DiscoveryInfo_> o = GetMyDevice_ ()) {
                    again:
                        if (retriedLockCount > 0) {
                            Execution::Sleep (1s); // sleep before retrying read-lock so readlock not held so long nobody can update
                        }
                        auto           l  = sDiscoveredDevices_.cget ();
                        DiscoveryInfo_ di = [&] () {
                            DiscoveryInfo_ tmp{};
                            tmp.fAttachedNetworks   = o->fAttachedNetworks;
                            tmp.fAttachedInterfaces = o->fAttachedInterfaces;
                            if (optional<DiscoveryInfo_> oo = FindMatchingDevice_ (l, tmp)) {
                                tmp = *oo; // merge
                                tmp.fAttachedNetworks += o->fAttachedNetworks;
                                Memory::AccumulateIf (&tmp.fAttachedInterfaces, o->fAttachedInterfaces);
                                return tmp;
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
                        di.fLastSeenAt      = DateTime::Now ();
                        di.PatchDerivedFields ();

                        Assert (di.fGUID != GUID{});
                        // Skip upgrade look to reduce the number of write locks we do, for the common case when there is no
                        // actual change
                        if (l->Lookup (di.fGUID) == di) {
#if qLOCK_DEBUGGING_
                            DbgTrace (L"!!! no change in ***MyDeviceDiscoverer_***  so skipping ");
#endif
                            goto nextTry;
                        }
#if qLOCK_DEBUGGING_
                        DbgTrace (L"!!! have change in ***MyDeviceDiscoverer_ so waiting to update");
#endif

                        Assert (di.fGUID != GUID{});
                        if (not sDiscoveredDevices_.UpgradeLockNonAtomicallyQuietly (
                                &l, [&] (auto&& writeLock) {
                                    writeLock.rwref ().Add (di.fGUID, di);
#if qLOCK_DEBUGGING_
                                    DbgTrace (L"!!! succeeded  updating writelock ***MyDeviceDiscoverer_");
#endif
                                },
                                5s)) {
                            // Failed merge, so try the entire acquire/update; this should be fairly rare (except when alot of contention like when we first start),
                            // and will cause a recomputation of the merge
                            retriedLockCount++;
                            DbgTrace (L"MyDeviceDiscoverer_: failed to update sDiscoveredDevices_ so retrying (cnt=%d)", retriedLockCount);
                            goto again;
                        }
                    }
                }
                catch (const Thread::InterruptException&) {
                    Execution::ReThrow ();
                }
                catch (...) {
                    Execution::Logger::Get ().LogIfNew (Execution::Logger::Priority::eError, 5min, L"%s", Characters::ToString (current_exception ()).c_str ());
                }

            nextTry:
                Execution::Sleep (30s); // @todo tmphack - really wait til change in network
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
            SystemInterfacesMgr interfacesMgr;
            for (Interface i : interfacesMgr.GetAll ()) {
                if (i.fType != Interface::Type::eLoopback and i.fStatus and i.fStatus->Contains (Interface::Status::eRunning)) {
                    i.fBindings.fAddresses.Apply ([&] (const InternetAddress& ia) {
                        newDev.AddIPAddresses_ (ia, i.fHardwareAddress);
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
                static const Time::Duration kReSearchInterval_{10min}; // not sure what interval makes sense
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

            unsigned int retriedLockCount = 0;
            if (d.fLocation) {
                try {
                    using namespace IO::Network::Transfer;
                    Connection::Ptr                     c          = Connection::New ();
                    Response                            r          = c.GET (d.fLocation);
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
                catch (...) {
                    DbgTrace (L"Failed to fetch description: %s", Characters::ToString (current_exception ()).c_str ());
                }
            }
            else {
                DbgTrace (L"no location, so no fetched device description");
            }

            WeakAssert (not locAddrs.empty ()); // CAN happen if dns name, and we cannot do dns lookup, but unsure we should include the device.
            if (not locAddrs.empty ()) {
            // merge in data
            again:
                if (retriedLockCount > 0) {
                    Execution::Sleep (1s); // sleep without the lock, but not first time processing message - just on retries
                }
                auto           l  = sDiscoveredDevices_.cget ();
                DiscoveryInfo_ di = [&] () {
                    DiscoveryInfo_ tmp{};
                    tmp.AddIPAddresses_ (locAddrs);
                    if (optional<DiscoveryInfo_> o = FindMatchingDevice_ (l, tmp)) {
                        tmp = *o; // then merge in possible additions
                        tmp.AddIPAddresses_ (locAddrs);
                        return tmp;
                    }
                    else {
                        // Generate GUID - based on ipaddrs
                        tmp.fGUID = LookupPersistentDeviceID_ (tmp);
                        return tmp;
                    }
                }();
                WeakAssert (not di.GetInternetAddresses ().empty ()); // can happen if we find address in tmp.AddIPAddress_() thats not bound to any adapter (but that shouldnt happen so investigate but is for now so ignore breifly)

                if (not di.fSSDPInfo) {
                    di.fSSDPInfo = DiscoveryInfo_::SSDPInfo{};
                }
                di.fSSDPInfo->fAlive = d.fAlive;

                Memory::CopyToIf (&di.fIcon, deviceIconURL);
                Memory::CopyToIf (&di.fSSDPInfo->fManufacturerURI, manufacturerURL);

                di.fSSDPInfo->fLocations.Add (d.fLocation);
                di.fSSDPInfo->fUSNs.Add (d.fUSN);

                Memory::CopyToIf (&di.fSSDPInfo->fPresentationURL, presentationURL); // consider if value already there - warn if changes - should we collect multiple

                if (di.fSSDPInfo->fServer.has_value () and di.fSSDPInfo->fServer != d.fServer) {
                    DbgTrace (L"Warning: different server IDs for same object");
                }
                di.fSSDPInfo->fServer = d.fServer;

                // @todo verify - but I think already done
                //di.AddIPAddresses_ (locAddrs);

                if (deviceType and deviceFriendlyName) {
                    di.fSSDPInfo->fDeviceType2FriendlyNameMap.Add (*deviceType, *deviceFriendlyName);
                }
                Memory::CopyToIf (&di.fSSDPInfo->fManufacturer, manufactureName);

                di.fSSDPInfo->fLastSSDPMessageRecievedAt = Time::DateTime::Now (); // update each message, even if already created
                di.fLastSeenAt                           = di.fSSDPInfo->fLastSSDPMessageRecievedAt;

#if qDebug
                di.fSSDPInfo->fLastAdvertisement = d;
#endif

                if (not di.fOperatingSystem.has_value ()) {
                    if (di.fSSDPInfo->fServer and di.fSSDPInfo->fServer->Contains (L"Linux"_k)) {
                        di.fOperatingSystem = Discovery::OperatingSystem{L"Linux"_k};
                    }
                    else if (di.fSSDPInfo->fServer and di.fSSDPInfo->fServer->Contains (L"POSIX"_k)) {
                        di.fOperatingSystem = Discovery::OperatingSystem{L"POSIX"_k};
                    }
                }
                di.PatchDerivedFields ();
                Assert (di.fGUID != GUID{});
                if (not sDiscoveredDevices_.UpgradeLockNonAtomicallyQuietly (
                        &l, [&] (auto&& writeLock) {
                            writeLock.rwref ().Add (di.fGUID, di);
#if qLOCK_DEBUGGING_
                            DbgTrace (L"!!! succeeded  updating writelock ***RecieveSSDPAdvertisement_");
#endif
                        },
                        5s)) {
                    // Failed merge, so try the entire acquire/update; this should be fairly rare (except when alot of contention like when we first start),
                    // and will cause a recomputation of the merge
                    retriedLockCount++;
                    DbgTrace (L"RecieveSSDPAdvertisement_: failed to update RecieveSSDPAdvertisement_ so retrying (cnt=%d)", retriedLockCount);
                    goto again; // release the lock and try again
                }
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
            : fMyThread_{Thread::CleanupPtr::eAbortBeforeWaiting, Thread::New (DiscoveryChecker_, Thread::eAutoStart, L"MyNeighborDiscoverer"sv)}
        {
        }

    private:
        static void DiscoveryChecker_ ()
        {
            Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"{}::MyNeighborDiscoverer_::DiscoveryChecker_")};
            static constexpr Activity kDiscovering_NetNeighbors_{L"discovering this network neighbors"sv};
            using Neighbor = NeighborsMonitor::Neighbor;
            NeighborsMonitor monitor{};
            while (true) {
                try {
                    DeclareActivity           da{&kDiscovering_NetNeighbors_};
                    Debug::TraceContextBumper ctx1{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"monitor.GetNeighbors ()")};
                    for (Neighbor i : monitor.GetNeighbors ()) {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
                        DbgTrace (L"i=%s", Characters::ToString (i).c_str ());
#endif
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

                        unsigned int retriedLockCount = 0;
                    again:
                        if (retriedLockCount > 0) {
                            Execution::Sleep (1s); // sleep without the lock, but not first time processing message - just on retries
                        }
#if qLOCK_DEBUGGING_
                        Debug::TraceContextBumper ctxLock1{L"sDiscoveredDevices_ - discovering this network neighbors "};
#endif

                        // merge in data
                        auto           l  = sDiscoveredDevices_.cget ();
                        DiscoveryInfo_ di = [&] () {
                            DiscoveryInfo_ tmp{};
                            tmp.AddIPAddresses_ (i.fInternetAddress, i.fHardwareAddress);
                            if (optional<DiscoveryInfo_> o = FindMatchingDevice_ (l, tmp)) {
                                tmp = *o;
                                tmp.AddIPAddresses_ (i.fInternetAddress, i.fHardwareAddress); // merge in additions
                                return tmp;
                            }
                            else {
                                // Generate GUID - based on ipaddrs
                                tmp.fGUID = LookupPersistentDeviceID_ (tmp);
                                return tmp;
                            }
                        }();

                        // INTENTIONALLY DONT UPDATE tmp.fLastSeenAt cuz this info can be quite stale

                        di.PatchDerivedFields ();

                        // Skip upgrade look to reduce the number of write locks we do, for the common case when there is no
                        // actual change
                        if (l->Lookup (di.fGUID) == di) {
#if qLOCK_DEBUGGING_
                            DbgTrace (L"!!! no change in ***MyNeighborDiscoverer_***  so skipping ");
#endif
                            continue;
                        }
#if qLOCK_DEBUGGING_
                        DbgTrace (L"have change in ***MyNeighborDiscoverer_*** so about to call UpgradeLockNonAtomicallyQuietly/1");
#endif

                        Assert (di.fGUID != GUID{});
                        if (not sDiscoveredDevices_.UpgradeLockNonAtomicallyQuietly (
                                &l, [&] (auto&& writeLock) {
                                    writeLock.rwref ().Add (di.fGUID, di);
#if qLOCK_DEBUGGING_
                                    DbgTrace (L"!!! succeeded  updating with writelock ***MyNeighborDiscoverer_");
#endif
                                },
                                5s)) {
                            // Failed merge, so try the entire acquire/update; this should be fairly rare (except when alot of contention like when we first start),
                            // and will cause a recomputation of the merge
                            retriedLockCount++;
                            DbgTrace (L"MyNeighborDiscoverer_: failed to update sDiscoveredDevices_ so retrying (cnt=%d)", retriedLockCount);
                            goto again;
                        }
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

namespace {
    /*
     ********************************************************************************
     ************************ RandomWalkThroughSubnetDiscoverer_ ********************
     ********************************************************************************
     */
    struct RandomWalkThroughSubnetDiscoverer_ {
        RandomWalkThroughSubnetDiscoverer_ ()
            : fMyThread_ (
                  Thread::CleanupPtr::eAbortBeforeWaiting, Thread::New (Checker_, Thread::eAutoStart, L"RandomWalkThroughSubnetDiscoverer"))
        {
        }

    private:
        static void Checker_ ()
        {
            Debug::TraceContextBumper ctx{L"RandomWalkThroughSubnetDiscoverer_::Checker_"};
            static constexpr Activity kDiscovering_THIS_{L"discovering by random scans"sv};

            static constexpr auto kMinTimeBetweenScans_{5s};

            //constexpr auto               kAllowedNetworkStaleness_ = 1min;
            constexpr Time::DurationSecondsType kAllowedNetworkStaleness_ = 60;

            /*
             *  Use a BloomFilter instead of a Set<> since we dont want to waste alot of memory storing
             *  EVERY item we visited and discarded, and we dont need to be perfect, its a slow random walk and devices
             *  could appear and disappear during a scan anyhow...
             */
            optional<DiscreteRange<InternetAddress>> scanAddressRange;
            unique_ptr<Cache::BloomFilter<int>>      addressesProbablyUsed;

            double sizeFactor{1};                       // (DOESNT APPEAR NEEDED) - use more bloom filter bits than needed for full set, cuz otherwise get too many collisions as adding
            double maxFalsePositivesAllowed      = .5;  // bloom filter stops working well if much past this probability limit
            double maxFractionOfAddrSpaceScanned = .75; // our algorithm wastes alot of time computing random numbers past this limit
            while (true) {
                try {
                    DeclareActivity da{&kDiscovering_THIS_};

                    // Keep scanning the given range til we're (mostly) done
                    if (not scanAddressRange) {
                        Sequence<Discovery::Network> activeNetworks = Discovery::NetworksMgr::sThe.CollectActiveNetworks (kAllowedNetworkStaleness_);
                        if (activeNetworks.empty ()) {
                            DbgTrace (L"No active network, so postponing random device address scan");
                            Execution::Sleep (30s);
                            continue;
                        }
                        // Scanning really only works for IPv4 since too large a range otherwise
                        for (Discovery::Network nw : activeNetworks) {
                            for (CIDR cidr : nw.fNetworkAddresses) {
                                if (cidr.GetBaseInternetAddress ().GetAddressFamily () == InternetAddress::AddressFamily::V4) {
                                    scanAddressRange = cidr.GetRange ();
                                    DbgTrace (L"Selecting scanAddressRange=%s", Characters::ToString (scanAddressRange).c_str ());
                                    break;
                                }
                            }
                            if (scanAddressRange) {
                                break;
                            }
                        }
                        if (scanAddressRange) {
                            addressesProbablyUsed = make_unique<Cache::BloomFilter<int>> (static_cast<size_t> (sizeFactor * scanAddressRange->GetNumberOfContainedPoints ()));
                        }
                    }
                    if (not scanAddressRange) {
                        // try again later
                        DbgTrace (L"No active IPV4 network, so postponing random device address scan");
                        Execution::Sleep (30s);
                        continue;
                    }
                    AssertNotNull (addressesProbablyUsed);

                    //
                    // pick first few addresses randomly, and when nearly full, clear, and try again
                    // This doesn't gaurantee scanning every address, but the number of addresses could be large (e.g. class B network)
                    // and it takes so long to scan, we'll miss a bunch anyhow. Retrying later statistically guarnatees we find everything
                    // thats responding and around long enuf
                    //
                    optional<unsigned int> selected;

                    auto bloomFilterStats = addressesProbablyUsed->GetStatistics ();
                    //DbgTrace (L"***addressesProbablyUsed->GetStatistics ()=%s", Characters::ToString (bloomFilterStats).c_str ());
                    if (bloomFilterStats.ProbabilityOfFalsePositive () < maxFalsePositivesAllowed and
                        double (bloomFilterStats.fApparentlyDistinctAddCalls) / scanAddressRange->GetNumberOfContainedPoints () < maxFractionOfAddrSpaceScanned) {
                        static mt19937 sRng_{std::random_device{}()};
                        selected = uniform_int_distribution<unsigned int>{1, scanAddressRange->GetNumberOfContainedPoints () - 2}(sRng_);
                    }
                    else {
                        DbgTrace (L"Completed full (%d/%d => %f fraction) scan of (scanAddressRange=%s), with randomCollisions=%d, resetting list, to start rescanning...",
                                  bloomFilterStats.fApparentlyDistinctAddCalls, scanAddressRange->GetNumberOfContainedPoints (),
                                  double (bloomFilterStats.fApparentlyDistinctAddCalls) / scanAddressRange->GetNumberOfContainedPoints (),
                                  Characters::ToString (scanAddressRange).c_str (),
                                  bloomFilterStats.fActualAddCalls - bloomFilterStats.fApparentlyDistinctAddCalls);
                        DbgTrace (L"addressesProbablyUsed.GetStatistics ()=%s", Characters::ToString (bloomFilterStats).c_str ());
                        addressesProbablyUsed.reset ();
                        scanAddressRange.reset ();
                        Execution::Sleep (1s);
                        continue;
                    }
                    Assert (selected);

                    auto runPingCheck = [] (const InternetAddress& ia) {
                        PortScanResults scanResults = ScanPorts (ia, ScanOptions{ScanOptions::eQuick});
                        //DbgTrace (L"Port scanning %s returned these ports: %s", Characters::ToString (ia).c_str (), Characters::ToString (scanResults.fKnownOpenPorts).c_str ());

                        if (not scanResults.fDiscoveredOpenPorts.empty ()) {
                            // also add check for ICMP PING
                        }

                        // then flag found device and when via random pings/portscan, and record portscan result.
                        if (scanResults.fDiscoveredOpenPorts.empty ()) {
                            DbgTrace (L"No obvious device at ip %s for because no scan results (ScanOptions::eQuick)", Characters::ToString (ia).c_str ());
                        }
                        else {
                            DiscoveryInfo_ tmp{};
                            tmp.AddIPAddresses_ (ia);

                            auto l = sDiscoveredDevices_.rwget (); // grab write lock because almost assured of making changes (at least last seen)
                            // @todo RECONSIDER - MAYBE DO READ AND UPGRADE CUZ OF CASE WHERE NO SCAN RESULTS - WANT TO NOT BOTHER LOCKING

                            if (optional<DiscoveryInfo_> oo = FindMatchingDevice_ (l, tmp)) {
                                WeakAsserteNotReached (); // This case should basically never happen (maybe lose support) - because we check before running ping if its already in the list
                                // if found, update to say what ports we found
                                tmp = *oo;
                                Memory::AccumulateIf (&tmp.fOpenPorts, scanResults.fDiscoveredOpenPorts);
                                tmp.fLastSeenAt = DateTime::Now ();
                                tmp.PatchDerivedFields ();
                                Assert (tmp.fGUID != GUID{});
                                l.rwref ().Add (tmp.fGUID, tmp);
                                DbgTrace (L"Updated device %s for fKnownOpenPorts: %s", Characters::ToString (tmp.fGUID).c_str (), Characters::ToString (scanResults.fDiscoveredOpenPorts).c_str ());
                            }
                            else {
                                tmp.fGUID = LookupPersistentDeviceID_ (tmp);
                                // only CREATE an entry for addresses where we found a port
                                tmp.fOpenPorts  = scanResults.fDiscoveredOpenPorts;
                                tmp.fLastSeenAt = DateTime::Now ();
                                tmp.PatchDerivedFields ();
#if qDebug
                                tmp.fDebugProps.Add (L"Found-Through-Network-SYN-Scan", true);
#endif
                                Assert (tmp.fGUID != GUID{});
                                l.rwref ().Add (tmp.fGUID, tmp);
                                DbgTrace (L"Added device %s for fKnownOpenPorts: %s", Characters::ToString (tmp.fGUID).c_str (), Characters::ToString (scanResults.fDiscoveredOpenPorts).c_str ());
                            }
                        }
                    };

                    // We MAY skip scanning some addresses because of bloomfilter inaccuracy, but this allows us to skip lots
                    // of pointless random rescans, so its worth it to check the result of Add()
                    if (addressesProbablyUsed->Add (*selected)) {
                        InternetAddress ia = scanAddressRange->GetLowerBound ().Offset (*selected);

                        /*
                         *  dont bother probing if we already have the device in our list
                         */
                        bool need2CheckAddr{true};
                        {
                            auto           l = sDiscoveredDevices_.cget (); // grab write lock because almost assured of making changes (at least last seen)
                            DiscoveryInfo_ tmp{};
                            tmp.AddIPAddresses_ (ia);
                            if (optional<DiscoveryInfo_> oo = FindMatchingDevice_ (l, tmp)) {
                                need2CheckAddr = false;
                            }
                        }
                        if (need2CheckAddr) {
                            runPingCheck (ia);
                            Execution::Sleep (kMinTimeBetweenScans_);
                        }
                    }
                }
                catch (const Thread::InterruptException&) {
                    Execution::ReThrow ();
                }
                catch (...) {
                    Execution::Logger::Get ().LogIfNew (Execution::Logger::Priority::eError, 5min, L"%s", Characters::ToString (current_exception ()).c_str ());
                }
            }
        }
        Thread::CleanupPtr fMyThread_;
    };

    unique_ptr<RandomWalkThroughSubnetDiscoverer_> sRandomWalkThroughSubnetDiscoverer_;
}

namespace {
    /*
     ********************************************************************************
     ***************************** KnownDevicePortScanner_ **************************
     ********************************************************************************
     */
    struct KnownDevicePortScanner_ {
        KnownDevicePortScanner_ ()
            : fMyThread_{Thread::CleanupPtr::eAbortBeforeWaiting, Thread::New (Checker_, Thread::eAutoStart, L"KnownDevicePortScanner"sv)}
        {
        }

    private:
        static void Checker_ ()
        {
            static constexpr Activity kDiscovering_THIS_{L"checking status of active devices"sv};

            static constexpr auto kMinTimeBetweenScans_{5s};

            //constexpr auto               kAllowedNetworkStaleness_ = 1min;
            constexpr Time::DurationSecondsType kAllowedNetworkStaleness_ = 60;

            Sequence<GUID>           devices2Check;
            optional<Iterator<GUID>> devices2CheckIterator;

            while (true) {
                Execution::Sleep (kMinTimeBetweenScans_);

                try {
                    DeclareActivity da{&kDiscovering_THIS_};

                    if (devices2Check.empty ()) {
                        devices2Check         = Sequence<GUID>{sDiscoveredDevices_.cget ().cref ().Keys ()};
                        devices2CheckIterator = devices2Check.begin ();
                    }
                    if (devices2CheckIterator == devices2Check.end ()) {
                        devices2CheckIterator = nullopt;
                        devices2Check         = Sequence<GUID>{};
                    }
                    if (devices2Check.empty ()) {
                        Execution::Sleep (30s);
                        continue;
                    }

                    auto runPingCheck = [] (const GUID& deviceID, const InternetAddress& ia) {
                        PortScanResults scanResults = ScanPorts (ia, ScanOptions{ScanOptions::eRandomBasicOne});
#if USE_NOISY_TRACE_IN_THIS_MODULE_
                        DbgTrace (L"Port scanning on existing device %s (addr %s) returned these ports: %s", Characters::ToString (deviceID).c_str (), Characters::ToString (ia).c_str (), Characters::ToString (scanResults.fKnownOpenPorts).c_str ());
#endif

                        {
                            // also add check for ICMP PING
                            Frameworks::NetworkMonitor::Ping::Pinger p{ia};
                            try {
                                auto r = p.RunOnce (); //incomplete
                                // @todo document TTL arg to Pinger...
                                scanResults.fDiscoveredOpenPorts.Add (L"icmp:ping"sv);
                            }
                            catch (...) {
                            }
                        }

                        // then flag found device and when via random pings/portscan, and record portscan result.
                        if (not scanResults.fDiscoveredOpenPorts.empty ()) {
                            auto           l = sDiscoveredDevices_.rwget ();
                            DiscoveryInfo_ tmp{};
                            tmp.AddIPAddresses_ (ia);
                            if (optional<DiscoveryInfo_> oo = l.cref ().Lookup (deviceID)) {
                                // if found, update to say what ports we found
                                tmp = *oo;
                                Memory::AccumulateIf (&tmp.fOpenPorts, scanResults.fDiscoveredOpenPorts);
                                tmp.fLastSeenAt = DateTime::Now ();
                                tmp.PatchDerivedFields ();
                                Assert (tmp.fGUID != GUID{});
                                l.rwref ().Add (tmp.fGUID, tmp);
                                DbgTrace (L"Updated device %s for fKnownOpenPorts: %s", Characters::ToString (tmp.fGUID).c_str (), Characters::ToString (scanResults.fDiscoveredOpenPorts).c_str ());
                            }
                            else {
                                WeakAsserteNotReached (); // objects CAN disappear from list of devices (eventually we will support expiring/deleting)
                            }
                        }
                    };

                    if (auto o = sDiscoveredDevices_.cget ().cref ().Lookup (**devices2CheckIterator)) {
                        for (auto ia : o->GetInternetAddresses ()) {
                            runPingCheck (o->fGUID, ia);
                        }
                    }
                    (*devices2CheckIterator)++;
                }
                catch (const Thread::InterruptException&) {
                    Execution::ReThrow ();
                }
                catch (...) {
                    Execution::Logger::Get ().LogIfNew (Execution::Logger::Priority::eError, 5min, L"%s", Characters::ToString (current_exception ()).c_str ());
                }
            }
        }
        Thread::CleanupPtr fMyThread_;
    };

    unique_ptr<KnownDevicePortScanner_> sKnownDevicePortScanner_;
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
    constexpr bool kInclude_PortScan_Discoverer_{true};

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
        if constexpr (kInclude_PortScan_Discoverer_ and kInclude_Neighbor_Discoverer_) {
            Require (static_cast<bool> (sRandomWalkThroughSubnetDiscoverer_) == static_cast<bool> (sNeighborDiscoverer_));
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
        if constexpr (kInclude_PortScan_Discoverer_) {
            return sRandomWalkThroughSubnetDiscoverer_ != nullptr;
        }
        return sKnownDevicePortScanner_ != nullptr;
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
    if constexpr (kInclude_PortScan_Discoverer_) {
        sRandomWalkThroughSubnetDiscoverer_ = make_unique<RandomWalkThroughSubnetDiscoverer_> ();
    }
    sKnownDevicePortScanner_ = make_unique<KnownDevicePortScanner_> ();
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
    if constexpr (kInclude_PortScan_Discoverer_) {
        sRandomWalkThroughSubnetDiscoverer_.reset ();
    }
    sKnownDevicePortScanner_.reset ();
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
    Debug::TimingTrace ttrc{L"Discovery::DevicesMgr::GetActiveDevices", 1.0};

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

namespace {
    auto ICMPPing_ = [] (const InternetAddress& ia) -> bool {
        Frameworks::NetworkMonitor::Ping::Pinger p{ia};
        try {
            auto r = p.RunOnce (); //incomplete
            return true;
        }
        catch (...) {
            return false;
        }
    };
}

void Discovery::DevicesMgr::InitiateReScan (const GUID& deviceID)
{
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    Debug::TraceContextBumper ctx{L"Discovery::InitiateReScan"};
#endif
    Debug::TimingTrace        ttrc{L"Discovery::DevicesMgr::InitiateReScan"};
    static constexpr Activity kRescanning_Device_{L"rescanning device"sv};
    DeclareActivity           da{&kRescanning_Device_};

    auto findDeviceInfoAndClearFoundPorts = [] (const GUID& deviceID) {
        auto l = sDiscoveredDevices_.rwget ();
        if (optional<DiscoveryInfo_> oo = l.rwref ().Lookup (deviceID)) {
            DiscoveryInfo_ tmp{*oo};
            tmp.PatchDerivedFields ();
            Assert (tmp.fGUID != GUID{});
            tmp.fOpenPorts = nullopt;
            l.rwref ().Add (tmp.fGUID, tmp);
            return tmp;
        }
        Execution::Throw (IO::Network::HTTP::ClientErrorException{L"deviceID not recognized"});
    };
    auto addOpenPort = [] (const GUID& deviceID, const String& openPort) {
        auto l = sDiscoveredDevices_.rwget ();
        if (optional<DiscoveryInfo_> oo = l.rwref ().Lookup (deviceID)) {
            DiscoveryInfo_ tmp{*oo};
            Memory::AccumulateIf (&tmp.fOpenPorts, openPort);
            tmp.fLastSeenAt = DateTime::Now ();
            tmp.PatchDerivedFields ();
            Assert (tmp.fGUID != GUID{});
            l.rwref ().Add (tmp.fGUID, tmp);
            DbgTrace (L"Updated device %s for fKnownOpenPorts: %s", Characters::ToString (tmp.fGUID).c_str (), Characters::ToString (openPort).c_str ());
        }
        else {
            AssertNotReached ();
        }
    };

    DiscoveryInfo_ initialDeviceInfo = findDeviceInfoAndClearFoundPorts (deviceID);
    for (auto ia : initialDeviceInfo.GetInternetAddresses ()) {
        if (ICMPPing_ (ia)) {
            addOpenPort (deviceID, L"icmp:ping"sv);
        }
    }
    // now now just run scan using limited portscan API
    // but redo scanning one at a time so I can SHOW results immediately, as they appear
    for (auto ia : initialDeviceInfo.GetInternetAddresses ()) {
        PortScanResults results = ScanPorts (ia, ScanOptions{ScanOptions::eFull});
        for (String p : results.fDiscoveredOpenPorts) {
            addOpenPort (deviceID, p);
        }
    }
}

VariantValue DevicesMgr::ScanAndReturnReport (const InternetAddress& addr)
{
    Mapping<String, VariantValue> result;
    Set<String>                   ports;
    if (ICMPPing_ (addr)) {
        ports += L"icmp:ping"sv;
    }
    PortScanResults results = ScanPorts (addr, ScanOptions{ScanOptions::eFull});
    for (String p : results.fDiscoveredOpenPorts) {
        ports += p;
    }
    result.Add (L"openPorts", VariantValue{
                                  ports.Select<VariantValue> ([] (String i) { return VariantValue{i}; })});
    return VariantValue{result};
}
