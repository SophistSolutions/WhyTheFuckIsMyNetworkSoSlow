/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include <random>

#include "Stroika/Foundation/Cache/BloomFilter.h"
#include "Stroika/Foundation/Cache/TimedCache.h"
#include "Stroika/Foundation/Characters/Format.h"
#include "Stroika/Foundation/Characters/RegularExpression.h"
#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Common/SystemConfiguration.h"
#include "Stroika/Foundation/Containers/KeyedCollection.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Cryptography/Digest/Algorithm/MD5.h"
#include "Stroika/Foundation/Cryptography/Digest/Hash.h"
#include "Stroika/Foundation/Cryptography/Format.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Execution/Activity.h"
#include "Stroika/Foundation/Execution/IntervalTimer.h"
#include "Stroika/Foundation/Execution/Logger.h"
#include "Stroika/Foundation/Execution/Sleep.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/Execution/Thread.h"
#include "Stroika/Foundation/Execution/ThreadPool.h"
#include "Stroika/Foundation/IO/Network/DNS.h"
#include "Stroika/Foundation/IO/Network/HTTP/ClientErrorException.h"
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/LinkMonitor.h"
#include "Stroika/Foundation/IO/Network/Neighbors.h"
#include "Stroika/Foundation/IO/Network/Transfer/Connection.h"
#include "Stroika/Foundation/Traversal/DiscreteRange.h"

#include "Stroika/Frameworks/NetworkMonitor/Ping.h"
#include "Stroika/Frameworks/UPnP/DeviceDescription.h"
#include "Stroika/Frameworks/UPnP/SSDP/Client/CachingListener.h"

#include "../Common/EthernetMACAddressOUIPrefixes.h"

#include "NetworkInterfaces.h"
#include "PortScanner.h"

#include "Devices.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Common;
using namespace Stroika::Foundation::DataExchange;
using namespace Stroika::Foundation::Execution;
using namespace Stroika::Foundation::IO;
using namespace Stroika::Foundation::IO::Network;
using namespace Stroika::Foundation::Traversal;

using namespace Stroika::Frameworks;
using namespace Stroika::Frameworks::UPnP;

using Execution::Logger;
using Execution::RWSynchronized;
using IO::Network::InternetAddress;
using Stroika::Foundation::Common::GUID;
using Stroika::Foundation::Common::KeyValuePair;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Discovery;

// Comment this in to turn on aggressive noisy DbgTrace in this module
// #define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

// Turn on to debug issues with lock contention and upgradelocks
//#define qLOCK_DEBUGGING_ 1

namespace {
    // derived based on experimentation on my network - need standards/referecnes! -- LGP 2019-02-20
    const String kDeviceType_SpeakerGroup_{"urn:smartspeaker-audio:device:SpeakerGroup:1"sv};
    const String kDeviceType_ZonePlayer_{"urn:schemas-upnp-org:device:ZonePlayer:1"sv};
    const String kDeviceType_WFADevice_{"urn:schemas-wifialliance-org:device:WFADevice:1"sv};
    const String kDeviceType_WANConnectionDevice_{"urn:schemas-upnp-org:device:WANConnectionDevice:1"sv};
    const String kDeviceType_WLANAccessPointDevice_{"urn:schemas-upnp-org:device:WLANAccessPointDevice:1"sv};
    const String kDeviceType_WANDevice_{"urn:schemas-upnp-org:device:WANDevice:1"sv};
    const String kDeviceType_MediaRenderer_{"urn:schemas-upnp-org:device:MediaRenderer:1"sv};
    const String kDeviceType_DIALServer_{"urn:dial-multiscreen-org:device:dial:1"sv}; // typically TV, Blu-ray player, set-top-box, or similar device
    const String kDeviceType_DIALReceiver_{"urn:dial-multiscreen-org:device:dialreceiver:1"sv};
    // probably shouldn't be this specifc
    const String kDeviceType_Roku_{"urn:roku-com:device:player:1-0"sv};
}

namespace {
    optional<String> ReverseDNSLookup_ (const InternetAddress& inetAddr)
    {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
        Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs ("{}::ReverseDNSLookup_", "inetAddr={}"_f, inetAddr)};
#endif
        static constexpr auto                                                   kCacheTTL_{5min};
        static Cache::SynchronizedTimedCache<InternetAddress, optional<String>> sCache_{kCacheTTL_};
        //sCache_.fHoldWriteLockDuringCacheFill = true; // see random false positive - see if this affects -LGP 2022-11-21 - assertexternally...https://stroika.atlassian.net/browse/STK-956
        try {
            return sCache_.LookupValue (inetAddr, [] (const InternetAddress& inetAddr) { return DNS::kThe.ReverseLookup (inetAddr); });
        }
        catch (...) {
            sCache_.Add (inetAddr, nullopt); // negative cache for kCacheTTL_
            return nullopt;                  // if DNS is failing, just dont do this match, dont abandon all data collection
        }
    }
    Set<InternetAddress> DNSLookup_ (const String& hostOrIPAddress)
    {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
        Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs ("{}::DNSLookup_", "hostOrIPAddress={}"_f, hostOrIPAddress)};
#endif
        static constexpr auto                                              kCacheTTL_{5min};
        static Cache::SynchronizedTimedCache<String, Set<InternetAddress>> sCache_{kCacheTTL_};
        return sCache_.LookupValue (hostOrIPAddress, [] (const String& hostOrIPAddress) -> Set<InternetAddress> {
            return Set<InternetAddress>{DNS::kThe.GetHostAddresses (hostOrIPAddress)};
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
    sb << "{"sv;
    sb << "hardwareAddress: "sv << hardwareAddresses << ", "sv;
    sb << "localAddresses: "sv << localAddresses;
    sb << "}"sv;
    return sb;
}

/*
 ********************************************************************************
 ******************************* Discovery::Device ******************************
 ********************************************************************************
 */
Set<String> Discovery::Device::GetHardwareAddresses () const
{
    Set<String> result;
    for (const auto& iNet : fAttachedNetworks) {
        result += iNet.fValue.hardwareAddresses;
    }
    return result;
}

Set<InternetAddress> Discovery::Device::GetInternetAddresses () const
{
    Set<InternetAddress> result;
    for (const auto& iNet : fAttachedNetworks) {
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
    for (const auto& iNet : fAttachedNetworks) {
        for (const auto& aNetAddr : iNet.fValue.localAddresses) {
            if (aNetAddr.GetAddressFamily () == InternetAddress::AddressFamily::V4) {
                result += aNetAddr;
            }
            break;
        }
    }
    if (result.empty ()) {
        for (const auto& iNet : fAttachedNetworks) {
            for (const auto& aNetAddr : iNet.fValue.localAddresses) {
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
    sb << "{"sv;
    sb << "GUID: "sv << fGUID << ", "sv;
    sb << "names: "sv << fNames << ", "sv;
    sb << "icon: "sv << fIcon << ", "sv;
    sb << "manufacturer: "sv << fManufacturer << ", "sv;
    sb << "types: "sv << fTypes << ", "sv;
    if (fThisDevice) {
        sb << "This-Device: "sv << fThisDevice << ", "sv;
    }
    sb << "attachedNetworks: "sv << fAttachedNetworks << ", "sv;
    sb << "attachedInterfaces: "sv << fAttachedInterfaces << ", "sv;
    sb << "presentationURL: "sv << fPresentationURL << ", "sv;
    sb << "operatingSystem: "sv << fOperatingSystem << ", "sv;
#if qStroika_Foundation_Debug_AssertionsChecked
    sb << "debugProps: "sv << fDebugProps;
#endif
    sb << "}"sv;
    return sb;
}

namespace {
    /*
     ********************************************************************************
     ************************* NetAndNetInterfaceMapper_ ****************************
     ********************************************************************************
     */
    class NetAndNetInterfaceMapper_ {
    public:
        NetAndNetInterfaceMapper_ () = default; // load networks and network interafces..

    public:
        Set<GUID> LookupNetworksGUIDs (const Iterable<InternetAddress>& ia) const
        {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
            Debug::TraceContextBumper ctx{
                Stroika_Foundation_Debug_OptionalizeTraceArgs ("{}::NetAndNetInterfaceMapper_::LookupNetworksGUIDs", "ia={}"_f, ia)};
#endif
            Set<GUID> results;
            for (const Discovery::Network& nw : Discovery::NetworksMgr::sThe.CollectActiveNetworks ()) {
                for (const InternetAddress& i : ia) {
                    if (nw.Contains (i)) {
                        results += nw.fGUID;
                    }
                }
            }
#if USE_NOISY_TRACE_IN_THIS_MODULE_
            DbgTrace (L"returning {}"_f, results);
#endif
            return results;
        }
        Set<GUID> LookupNetworksGUIDs (const InternetAddress& ia) const
        {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
            Debug::TraceContextBumper ctx{
                Stroika_Foundation_Debug_OptionalizeTraceArgs ("{}::NetAndNetInterfaceMapper_::LookupNetworksGUIDs", "ia={}"_f, ia)};
#endif
            Set<GUID> results;
            for (const Discovery::Network& nw : Discovery::NetworksMgr::sThe.CollectActiveNetworks ()) {
                if (nw.Contains (ia)) {
                    results += nw.fGUID;
                }
            }
#if USE_NOISY_TRACE_IN_THIS_MODULE_
            DbgTrace ("returning {}"_f, results);
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

        struct SSDPInfo {
            optional<bool>   fAlive; // true while any of its advertisements is in force, false once none is (@see SSDPDeviceDiscoverer_)
            Set<String>      fUSNs;  // those in force
            Set<URI>         fLocations; // those in force
            optional<String> fServer;
            optional<String> fManufacturer;
            optional<URI>    fManufacturerURI;
            Mapping<String, String> fDeviceType2FriendlyNameMap; //  http://upnp.org/specs/arch/UPnP-arch-DeviceArchitecture-v1.1.pdf - <deviceType> - Page 44
            optional<URI>  fPresentationURL;
            Time::DateTime fLastSSDPMessageRecievedAt{Time::DateTime::Now ()}; // when last merged: just heard, or still in force
#if qStroika_Foundation_Debug_AssertionsChecked
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
#if qStroika_Foundation_Debug_AssertionsChecked
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
            bool operator== (const SSDPInfo&) const = default;
#endif

            String ToString () const
            {
                StringBuilder sb;
                sb << "{"sv;
                sb << "USNs: "sv << fUSNs << ", "sv;
                sb << "Alive: "sv << fAlive << ", "sv;
                sb << "Locations: "sv << fLocations << ", "sv;
                sb << "Manufacturer: "sv << fManufacturer << ", "sv;
                sb << "Manufacturer-URI: "sv << fManufacturerURI << ", "sv;
                sb << "Server: "sv << fServer << ", "sv;
                sb << "Device-Type-2-Friendly-Name-Map: "sv << fDeviceType2FriendlyNameMap << ", "sv;
                sb << "Presentation-URL: "sv << fPresentationURL << ", ";
                sb << "Last-SSDP-Message-Recieved-At: "sv << fLastSSDPMessageRecievedAt << ", "sv;
#if qStroika_Foundation_Debug_AssertionsChecked
                sb << "Last-Advertisement: " << fLastAdvertisement;
#endif
                sb << "}"sv;
                return sb;
            }
        };
        optional<SSDPInfo> fSSDPInfo;

        void AddNetworkAddresses_ (const InternetAddress& addr, const optional<String>& hwAddr = nullopt)
        {
            // merge the addrs into each matching network interface
            AddNetworkAddresses_ (Iterable<InternetAddress>{addr}, hwAddr);
        }
        void AddNetworkAddresses_ (const Iterable<InternetAddress>& addrs, const optional<String>& hwAddr = nullopt)
        {
            // merge the addrs into each matching network interface
            unsigned int totalAddrs{};
            unsigned int totalAddrsSuppressedQuietly{};
            unsigned int totalAdds{};
            for (const InternetAddress& ia : addrs) {
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
                for (const GUID& nw : NetAndNetInterfaceMapper_::sThe.LookupNetworksGUIDs (ia)) {
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
                DbgTrace ("AddNetworkAddresses_({}) called, but no matching interfaces found"_f, addrs);
                constexpr bool kExtraDebugging_ = true;
                if (kExtraDebugging_) {
                    // Debug why we get "Unknown" device on hercules/linux with Found-By-MyNeighborDiscoverer_-I looking good, but no networks (and docker appears as an active network)
                    DbgTrace ("AddNetworkAddresses_: totalAddrs={}, totalAddrsSuppressedQuietly={}, totalAdds={}"_f, totalAddrs,
                              totalAddrsSuppressedQuietly, totalAdds);
                    DbgTrace ("AddNetworkAddresses_: NetAndNetInterfaceMapper_::sThe.LookupNetworksGUIDs (ia)={}"_f,
                              NetAndNetInterfaceMapper_::sThe.LookupNetworksGUIDs (addrs));
                    DbgTrace ("AddNetworkAddresses_: Discovery::NetworksMgr::sThe.CollectActiveNetworks ()={}"_f,
                              Discovery::NetworksMgr::sThe.CollectActiveNetworks ());
                }
            }
        }

        void PatchDerivedFields ()
        {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
            Debug::TraceContextBumper ctx{"{}::DiscoveryInfo_::PatchDerivedFields"};
#endif

            /*
             *  Names Calculation
             */
            if (fSSDPInfo.has_value ()) {
                // Special hack for sonos speakers
                if (fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_SpeakerGroup_) and
                    fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_ZonePlayer_)) {
                    static const RegularExpression kSonosRE_{L"([0-9.:]*)( - .*)"_RegEx};
                    String                         newName = fSSDPInfo->fDeviceType2FriendlyNameMap[kDeviceType_ZonePlayer_];
                    optional<String>               m1, m2;
                    if (newName.Matches (kSonosRE_, &m1, &m2)) {
                        Assert (m1.has_value () and m2.has_value ());
                        String speakerGroup = fSSDPInfo->fDeviceType2FriendlyNameMap[kDeviceType_SpeakerGroup_];
                        if (speakerGroup.empty ()) {
                            Assert (m2->length () >= 3); // because matched SPACE-SPACE.*
                            fNames.Add (m2->SubString (3), 100);
#if qStroika_Foundation_Debug_AssertionsChecked
                            fDebugProps.Add (L"SSDP-DeviceType2FriendlyName-SONOS-HACK-devicefriendlyname"sv, m2->SubString (3));
#endif
                        }
                        else {
                            fNames.Add (speakerGroup + *m2, 99);
#if qStroika_Foundation_Debug_AssertionsChecked
                            fDebugProps.Add ("SSDP-DeviceType2FriendlyName-SONOS-HACK-speakergroup"sv, speakerGroup + *m2);
#endif
                        }
                    }
                }

                // pick any one of the friendly names, or the server name if we must
                if (not fSSDPInfo->fDeviceType2FriendlyNameMap.empty () and not fSSDPInfo->fDeviceType2FriendlyNameMap.Nth (0).fValue.empty ()) {
                    fNames.Add (fSSDPInfo->fDeviceType2FriendlyNameMap.Nth (0).fValue, 95);
#if qStroika_Foundation_Debug_AssertionsChecked
                    fDebugProps.Add ("SSDP-DeviceType2FriendlyName"sv, fSSDPInfo->fDeviceType2FriendlyNameMap.Nth (0).fValue);
#endif
                }

                fPresentationURL = fSSDPInfo->fPresentationURL;
            }
            // try reverse dns lookup
            for (const auto& i : GetInternetAddresses ()) {
                if (auto o = ReverseDNSLookup_ (i)) {
                    fNames.Add (*o, 96);
#if qStroika_Foundation_Debug_AssertionsChecked
                    fDebugProps.Add ("reverse-dns-name"sv, *o);
#endif
                    break;
                }
            }

            fNames.Add (GetPreferredDisplayInternetAddresses ().Join (), 1); // default/backup name

            if (fSSDPInfo.has_value () and (fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_SpeakerGroup_) or
                                            fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_ZonePlayer_))) {
                fTypes.Add (Discovery::DeviceType::eSpeaker);
            }

            if (fSSDPInfo.has_value () and (fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_WFADevice_) or
                                            fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_WANConnectionDevice_) or
                                            fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_WLANAccessPointDevice_) or
                                            fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_WANDevice_))) {
                fTypes.Add (Discovery::DeviceType::eNetworkInfrastructure);
            }

            // I believe a media renderer can be EITHER a full TV, or just something with speakers...
            if (fSSDPInfo.has_value () and
                // @todo - need a better way to detect - look at services not device type?
                (fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_MediaRenderer_))) {
                fTypes.Add (Discovery::DeviceType::eSpeaker);
            }

            if (fSSDPInfo.has_value () and (
                                               // @todo - need a better way to detect - look at services not device type?
                                               fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_Roku_))) {
                fTypes.Add (Discovery::DeviceType::eMediaPlayer);
            }

            // So far only seen used for Amazon Fire Stick, but could be used for TV, according to
            // http://www.dial-multiscreen.org/dial-protocol-specification
            if (fSSDPInfo.has_value () and (fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_DIALServer_))) {
                // @todo look more closely at firestick - I think dialserver isnt neceesarily a media player
                fTypes.Add (Discovery::DeviceType::eMediaPlayer);
                //fTypes.Add (Discovery::DeviceType::eTV);
            }

            if (fSSDPInfo.has_value () and fSSDPInfo->fDeviceType2FriendlyNameMap.ContainsKey (kDeviceType_DIALReceiver_)) {
                //fTypes.Add (Discovery::DeviceType::eMediaPlayer);
                fTypes.Add (Discovery::DeviceType::eTV);
            }

            {
                // See if its addresses intersect with any network gateways - if so - its a router
                Set<InternetAddress> gateways;
                for (const GUID& netGUID : fAttachedNetworks.Keys ()) {
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
                for (const auto& hwa : GetHardwareAddresses ()) {
                    if (auto o = BackendApp::Common::LookupEthernetMACAddressOUIFromPrefix (hwa)) {
                        if (not fManufacturer) {
                            fManufacturer = Manufacturer{};
                        }
                        bool longName = o->Contains (L" "sv); //  primitive guess
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
                static const String kSMBPort_ = Characters::Format ("tcp:{}"_f, IO::Network::WellKnownPorts::TCP::kSMB);
                for (const auto& hwa : GetHardwareAddresses ()) {
                    if (auto o = BackendApp::Common::LookupEthernetMACAddressOUIFromPrefix (hwa)) {
                        if (o == "Oracle VirtualBox virtual NIC"sv) {
                            fTypes.Add (Discovery::DeviceType::eVirtualMachine);
                        }
                        if ((o == "Synology Incorporated"sv or o == "Buffalo.inc"sv or o == "Seagate Technology"sv or o == "Seagate Cloud Systems"sv) and
                            fOpenPorts and fOpenPorts->Contains (kSMBPort_)) {
                            fTypes.Add (Discovery::DeviceType::eNetworkAttachedStorage);
                        }
                    }
                }
            }

            static const String kIPPPort_ = Characters::Format ("tcp:{}"_f, IO::Network::WellKnownPorts::TCP::kIPP);
            static const String kLPDPort_ = Characters::Format ("tcp:{}"_f, IO::Network::WellKnownPorts::TCP::kLPD);
            if (fOpenPorts and (fOpenPorts->Contains (kIPPPort_) or fOpenPorts->Contains (kLPDPort_)) and
                (fManufacturer and (fManufacturer->Contains ("Hewlett Packard"_k) or fManufacturer->Contains ("Epson"_k) or
                                    fManufacturer->Contains ("Canon"_k) or fManufacturer->Contains ("Brother"_k)))) {
                fTypes.Add (DeviceType::ePrinter);
            }

            Assert (fSeen.fARP or fSeen.fCollector or fSeen.fICMP or fSeen.fTCP or fSeen.fUDP); // else confusing why this was added

#if qStroika_Foundation_Debug_AssertionsChecked
            if (fSSDPInfo) {
                fDebugProps.Add (L"SSDPInfo"sv, VariantValue{Mapping<String, VariantValue> {
                                     pair<String, VariantValue>{L"deviceType2FriendlyNameMap"sv,
                                                                Mapping<String, VariantValue> {
                                                                    fSSDPInfo->fDeviceType2FriendlyNameMap
                                                                }},
                                     pair<String, VariantValue>{"alive"sv, fSSDPInfo->fAlive},
                                     pair<String, VariantValue>{"USNs"sv, kMyMapper_.FromObject (fSSDPInfo->fUSNs)},
                                     pair<String, VariantValue>{"server"sv, fSSDPInfo->fServer},
                                     pair<String, VariantValue>{"manufacturer"sv, fSSDPInfo->fManufacturer},
                                     pair<String, VariantValue>{"manufacturer-URL"sv, kMyMapper_.FromObject (fSSDPInfo->fManufacturerURI)},
                                     pair<String, VariantValue>{"lastAdvertisement"sv, kMyMapper_.FromObject (fSSDPInfo->fLastAdvertisement)},
                                     pair<String, VariantValue>{"lastSSDPMessageReceivedAt"sv, fSSDPInfo->fLastSSDPMessageRecievedAt},
                                     pair<String, VariantValue> {
                                         "locations"sv,
                                         kMyMapper_.FromObject (fSSDPInfo->fLocations)
                                     }
                                 }});
            }
#endif

#if USE_NOISY_TRACE_IN_THIS_MODULE_
            DbgTrace ("At end of PatchDerivedFields: {}"_f, ToString ());
#endif
        }

#if __cpp_impl_three_way_comparison < 201711
        bool operator== (const DiscoveryInfo_& rhs) const
        {
            if (not Discovery::Device::operator== (rhs)) {
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
        bool operator== (const DiscoveryInfo_&) const = default;
#endif

        String ToString () const
        {
            StringBuilder sb = Discovery::Device::ToString ().SubString (0, -1);
            sb << "SSDP-Info: "sv << fSSDPInfo << ", "sv;
            sb << "}"sv;
            return sb;
        }
    };

    struct Device_Key_Extractor_ {
        GUID operator() (const DiscoveryInfo_& t) const
        {
            return t.fGUID;
        };
    };
    using DiscoveryDeviceCollection_ =
        KeyedCollection<DiscoveryInfo_, GUID, KeyedCollection_DefaultTraits<DiscoveryInfo_, GUID, Device_Key_Extractor_>>;

    // NB: RWSynchronized because most accesses will be to read/lookup in this list; use Mapping<> because KeyedCollection NYI
    // Note, when we first start, there will be more contention, so we'll get conflicts (and we dbgtrace log them to be sure
    // not too many).
#if qLOCK_DEBUGGING_
    //Synchronized<Mapping<GUID, DiscoveryInfo_>, Tracing_Synchronized_Traits<shared_timed_mutex>> sDiscoveredDevices_;
    Synchronized<DiscoveryDeviceCollection_, Tracing_Synchronized_Traits<shared_timed_mutex>> sDiscoveredDevices_;
#else
    //RWSynchronized<Mapping<GUID, DiscoveryInfo_>> sDiscoveredDevices_;
    RWSynchronized<DiscoveryDeviceCollection_> sDiscoveredDevices_;
#endif

// turn on tracking of locks on sDiscoveredDevices_
#if qStroika_Foundation_Debug_DefaultTracingOn && qLOCK_DEBUGGING_
    int sDiscoveredDevicesLockNamed_ = [] () {
        sDiscoveredDevices_.fDbgTraceLocksName = L"DiscoveredDevices_";
        return 0;
    }();
#endif

    // Look through all the existing devices, and if one appears to match, return it.
    optional<DiscoveryInfo_> FindMatchingDevice_ (decltype (sDiscoveredDevices_)::ReadableReference& rr, const DiscoveryInfo_& d)
    {
        for (const auto& di : rr.load ()) {
            if (d.GetHardwareAddresses ().Intersects (di.GetHardwareAddresses ())) {
                return di;
            }
            if (d.GetInternetAddresses ().Intersects (di.GetInternetAddresses ())) {
                return di;
            }
        }
        return nullopt;
    }

    using DeviceFinder_ = function<optional<DiscoveryInfo_> (decltype (sDiscoveredDevices_)::ReadableReference& rr)>;
    using DeviceMerger_ = function<optional<DiscoveryInfo_> (const optional<DiscoveryInfo_>& match)>;

    /*
     *  Store in sDiscoveredDevices_ what merge makes of the device find finds there: merge gets that device, or nullopt (none -
     *  so for a discoverer, a new device), and returns the device to store, or nullopt to store nothing. Returns the device as
     *  stored (nullopt if merge stored nothing).
     *
     *  The arguments:
     *      - find (or found, or deviceID): only what identifies the device. FindMatchingDevice_ reads just found's hardware
     *        and internet addresses; whatever else found holds reaches the store only when merge starts a new device from it -
     *        on a match, merge starts from the stored device.
     *      - merge: everything that depends on the stored device - what to add to it, and, last, PatchDerivedFields. Work that
     *        needs nothing from the stored device (a port scan, a ping, an SSDP description's fetch) is done before the call,
     *        and its results captured by merge.
     *
     *  merge runs first with no lock held, so its slow part - PatchDerivedFields' reverse DNS lookups, the network list's
     *  refresh - blocks no reader; the write lock is held only to find the device again and store. If another writer changed
     *  it meanwhile, merge runs again, under the write lock: so it must not lock sDiscoveredDevices_ itself, and must change
     *  nothing but what it returns.
     */
    optional<DiscoveryInfo_> StoreMerged_ (const DeviceFinder_& find, const DeviceMerger_& merge)
    {
        optional<DiscoveryInfo_> match = [&] () {
            auto l = sDiscoveredDevices_.cget ();
            return find (l);
        }();
        optional<DiscoveryInfo_> di = merge (match);
        auto                     l  = sDiscoveredDevices_.rwget ();
        Debug::TimingTrace       ttrc{"StoreMerged_ held sDiscoveredDevices_' write lock", 0.5s};
        if (optional<DiscoveryInfo_> matchNow = find (l); matchNow != match) {
            DbgTrace ("StoreMerged_: the matching device changed while merging, so merging again"_f);
            match = matchNow;
            di    = merge (match);
        }
        if (di and di != match) { // else nothing changed, so no write
            Assert (di->fGUID != GUID{});
            l.rwref ().Add (*di);
        }
        return di;
    }
    optional<DiscoveryInfo_> StoreMerged_ (const DiscoveryInfo_& found, const DeviceMerger_& merge) ///< \brief merge into the device matching what a discoverer found (@see FindMatchingDevice_)
    {
        return StoreMerged_ ([&] (decltype (sDiscoveredDevices_)::ReadableReference& rr) { return FindMatchingDevice_ (rr, found); }, merge);
    }
    optional<DiscoveryInfo_> StoreMerged_ (const GUID& deviceID, const DeviceMerger_& merge) ///< \brief merge into the device with this ID
    {
        return StoreMerged_ ([&] (decltype (sDiscoveredDevices_)::ReadableReference& rr) { return rr.cref ().Lookup (deviceID); }, merge);
    }
}

namespace {
    void PatchSeen_ (DiscoveryInfo_* d, const PortScanResults& scanResults)
    {
        if (scanResults.fIncludedICMP) {
            d->fSeen.fICMP = Memory::NullCoalesce (d->fSeen.fICMP).Extend (DateTime::Now ());
        }
        if (scanResults.fIncludesTCP) {
            d->fSeen.fTCP = Memory::NullCoalesce (d->fSeen.fTCP).Extend (DateTime::Now ());
        }
        if (scanResults.fIncludesUDP) {
            d->fSeen.fUDP = Memory::NullCoalesce (d->fSeen.fUDP).Extend (DateTime::Now ());
        }
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
            : fMyDeviceDiscovererThread_{Thread::CleanupPtr::eAbortBeforeWaiting, Thread::New (DiscoveryChecker_, Thread::eAutoStart, "MyDeviceDiscoverer"_k)}
        {
        }

        optional<GUID> GetThisDeviceID () const
        {
            return sCachedValue_.load ();
        }

    private:
        static void DiscoveryChecker_ ()
        {
            static constexpr Activity kDiscovering_This_Device_{"discovering this device"sv};
            while (true) {
                try {
                    DeclareActivity da{&kDiscovering_This_Device_};
                    if (optional<DiscoveryInfo_> thisDevice = GetMyDevice_ ()) {
                        DiscoveryInfo_ found{};
                        found.fAttachedNetworks         = thisDevice->fAttachedNetworks;
                        found.fAttachedInterfaces       = thisDevice->fAttachedInterfaces;
                        found.fNames                    = thisDevice->fNames;
                        optional<DiscoveryInfo_> stored = StoreMerged_ (found, [&] (const optional<DiscoveryInfo_>& match) {
                            DiscoveryInfo_ di = found;
                            if (match) {
                                di = *match; // merge
                                di.fAttachedNetworks += thisDevice->fAttachedNetworks;
                                Memory::AccumulateIf (&di.fAttachedInterfaces, thisDevice->fAttachedInterfaces);
                                for (const auto& n : thisDevice->fNames) {
                                    di.fNames.Add (n.fName, n.fPriority);
                                }
#if qStroika_Foundation_Debug_AssertionsChecked
                                di.fDebugProps.Add ("Updated-By-MyDeviceDiscoverer_-At", DateTime::Now ());
#endif
                            }
                            else {
                                di.fGUID = GUID::GenerateNew ();
#if qStroika_Foundation_Debug_AssertionsChecked
                                di.fDebugProps.Add ("Created-By-MyDeviceDiscoverer-At", DateTime::Now ());
                                di.fDebugProps.Add ("Created-By-MyDeviceDiscoverer-With-Networks", Characters::ToString (di.fAttachedNetworks));
                                di.fDebugProps.Add ("Created-By-MyDeviceDiscoverer-With-Interfaces", Characters::ToString (di.fAttachedInterfaces));
#endif
                            }
                            // copy most/all fields -- @todo cleanup - do more automatically - all but GUID??? Need merge??
                            di.fTypes           = thisDevice->fTypes;
                            di.fThisDevice      = thisDevice->fThisDevice;
                            auto osInfo         = GetSystemConfiguration_ActualOperatingSystem ();
                            di.fOperatingSystem = OperatingSystem{osInfo.fTokenName, osInfo.fPrettyNameWithVersionDetails};
                            di.fSeen.fCollector = Memory::NullCoalesce (di.fSeen.fCollector).Extend (DateTime::Now ());
                            di.PatchDerivedFields ();
                            return di;
                        });
                        sCachedValue_.store (stored->fGUID); // only once it is in sDiscoveredDevices_
                    }
                }
                catch (const Thread::AbortException&) {
                    Execution::ReThrow ();
                }
                catch (...) {
                    Logger::sThe.Log (Logger::eError, "{}"_f, current_exception ());
                }
                Execution::Sleep (30s); // @todo tmphack - really wait til change in network
            }
        }
        Thread::CleanupPtr fMyDeviceDiscovererThread_;

        static optional<DiscoveryInfo_> GetMyDevice_ ()
        {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
            Debug::TraceContextBumper ctx{"{}::GetMyDevice_", "interfaces={}"_f, IO::Network::SystemInterfacesMgr{}.GetAll ()};
#endif
            DiscoveryInfo_ newDev;
            newDev.fNames.Add (GetSystemConfiguration_ComputerNames ().fHostname, 200);
            newDev.fTypes += DeviceType::ePC; // not sure what this means, or if its the best place
            newDev.fTypes += DeviceType::eWTFCollector;
            newDev.fThisDevice = true;
            SystemInterfacesMgr interfacesMgr;
            for (const Interface& i : interfacesMgr.GetAll ()) {
                if (i.fType != Interface::Type::eLoopback and i.fStatus and i.fStatus->Contains (Interface::Status::eRunning)) {
                    i.fBindings.fAddresses.Apply ([&] (const InternetAddress& ia) { newDev.AddNetworkAddresses_ (ia, i.fHardwareAddress); });
                }
            }
            newDev.fAttachedInterfaces = Discovery::NetworkInterfacesMgr::sThe.CollectAllNetworkInterfaces ().Map<Set<GUID>> (
                [] (const auto& iFace) { return iFace.fGUID; });

            if (newDev.GetHardwareAddresses ().empty ()) {
                DbgTrace ("no hardware address, so returning no 'MyDevice'"_f);
                return nullopt;
            }

            // No need to set GUID - set in caller
#if USE_NOISY_TRACE_IN_THIS_MODULE_
            DbgTrace ("returning: {}"_f, newDev);
#endif
            return newDev;
        }

        static inline Synchronized<optional<GUID>> sCachedValue_;
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
     *
     *  Stroika's SSDP CachingListener listens and searches, following networks as they come and go, and keeps what is
     *  advertised - telling us of each change: an advertisement added, or removed (by an ssdp:byebye, or its max-age running
     *  out). Its callbacks run on its threads, which a network change and shutdown wait for, so they only queue the change:
     *  fWorker_, a thread of our own, does the slow parts - resolving the LOCATION, fetching the device description, merging
     *  into sDiscoveredDevices_ - one change at a time, in order.
     *
     *  It reports changes, not each advertisement heard, so each minute the timer has fWorker_ merge what is still in force
     *  again (@see Refresh_), extending each device's fSeen.fUDP. The timer also retries constructing the listener, if that
     *  failed (e.g. its socket could not be created or bound).
     */
    class SSDPDeviceDiscoverer_ {
    public:
        SSDPDeviceDiscoverer_ ()
            : fIntervalTimerAdder_{[this] () {
                                       Debug::TraceContextBumper ctx{"SSDPDeviceDiscoverer_ TIMER HANDLER"}; // to debug https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/78
                                       if (fListener_ == nullptr) {
                                           IgnoreExceptionsExceptThreadAbortForCall (ConstructListener_ (true));
                                       }
                                       else {
                                           fWorker_.AddTask ([this, inForce = fListener_->GetAdvertisements ()] () { Refresh_ (inForce); });
                                       }
                                   },
                                   1min}
        {
            IgnoreExceptionsExceptThreadAbortForCall (ConstructListener_ (false));
        }

    private:
        nonvirtual void ConstructListener_ (bool notifyOfSuccess)
        {
            // SSDP can fail due to lack of permissions to bind to the appropriate sockets, or for example under WSL where we get protocol unsupported.
            // WARN to syslog, but no need to stop app
            try {
                static const Time::Duration kReSearchInterval_{10min}; // not sure what interval makes sense
                fListener_ = make_unique<SSDP::Client::CachingListener> (
                    [this] (const SSDP::Advertisement& a) { QueueChange_ (a); },
                    SSDP::Client::CachingListener::Options{.fSearchFor = SSDP::kTarget_UPNPRootDevice, .fSearchRepeatInterval = kReSearchInterval_});
                fListener_->Start (); // once fListener_ is set: QueueChange_ uses it
                if (notifyOfSuccess) {
                    Logger::sThe.Log (Logger::eInfo, "(Re-)Started SSDP Listener"_f);
                }
            }
            catch (const Thread::AbortException&) {
                fListener_.reset ();
                Execution::ReThrow ();
            }
            catch (...) {
                fListener_.reset ();
                Logger::sThe.Log (Logger::eWarning, "Problem starting SSDP Listener: {} - a significant source of discovery will be missed, but it will be retried automatically"_f,
                                  current_exception ());
            }
        }

    private:
        // on the listener's threads (@see CachingListener): only queue the change - a removal with what is still in force, which
        // within a callback has that change made
        void QueueChange_ (const SSDP::Advertisement& a)
        {
            if (a.fAlive == true) {
                fWorker_.AddTask ([this, a] () { Merge_ (a); });
            }
            else {
                fWorker_.AddTask ([this, a, inForce = fListener_->GetAdvertisements ()] () { Removed_ (a, inForce); });
            }
        }

    private:
        static Set<InternetAddress> LocationAddresses_ (const URI& location)
        {
            if (location.GetAuthority () and location.GetAuthority ()->GetHost ()) {
                URI::Host h = *location.GetAuthority ()->GetHost ();
                if (h.AsInternetAddress ()) {
                    return Set<InternetAddress>{*h.AsInternetAddress ()};
                }
                return DNSLookup_ (*h.AsRegisteredName ());
            }
            return Set<InternetAddress>{};
        }

        // the CONFIGID.UPNP.ORG header: changed whenever the device's description is (UPnP Device Architecture 1.1, section 1.2.2)
        static optional<String> ConfigID_ (const SSDP::Advertisement& a)
        {
            constexpr String::EqualsComparer kCaseInsensitive_{Characters::eCaseInsensitive};
            if (auto h = a.fRawHeaders.First (
                    [&] (const KeyValuePair<String, String>& i) { return kCaseInsensitive_ (i.fKey, "CONFIGID.UPNP.ORG"_k); })) {
                return h->fValue;
            }
            return nullopt;
        }

    private:
        /*
         *  A LOCATION's device description is fetched as an advertisement there is added, and not again while any there is in
         *  force (Removed_ forgets it once none is): UPnP Device Architecture 1.1, section 1.2.2, has it static until then -
         *  unless its CONFIGID.UPNP.ORG changes, saying it changed. One that failed is tried again after kRetryFailedFetchAfter_.
         */
        struct Fetched_ {
            Time::TimePointSeconds fAt;
            bool                   fSucceeded;
            optional<String>       fConfigID;
        };
        static constexpr Time::DurationSeconds kRetryFailedFetchAfter_{10min};

        optional<Frameworks::UPnP::DeviceDescription> FetchDescriptionIfDue_ (const SSDP::Advertisement& a)
        {
            optional<String>       configID = ConfigID_ (a);
            Time::TimePointSeconds now      = Time::GetTickCount ();
            if (optional<Fetched_> f = fFetched_.Lookup (a.fLocation)) {
                bool current = f->fSucceeded ? f->fConfigID == configID : now < f->fAt + kRetryFailedFetchAfter_;
                if (current) {
                    return nullopt;
                }
            }
            try {
                using namespace IO::Network::Transfer;
                // as of Stroika 3.0d25, Windows' WinHTTP backend applies the timeout only between reads of the body - not to the
                // connect, or the wait for a response (https://github.com/SophistSolutions/Stroika/issues/1246): there, a LOCATION
                // that never accepts the connection takes Windows' own TCP give-up, about 21 seconds
                static constexpr Time::DurationSeconds kFetchDescriptionTimeout_{5s};
                Connection::Ptr                        c = Connection::New ();
                c.SetTimeout (kFetchDescriptionTimeout_);
                Response                            r      = c.GET (a.fLocation);
                Frameworks::UPnP::DeviceDescription result = DeSerialize (r.GetData ()).Resolve (a.fLocation); // its URLs absolute
                DbgTrace ("Found device description = {}"_f, result);
                fFetched_.Add (a.fLocation, Fetched_{now, true, configID});
                return result;
            }
            catch (const Thread::AbortException&) {
                Execution::ReThrow ();
            }
            catch (...) {
                DbgTrace ("Failed to fetch description: {}"_f, current_exception ());
                fFetched_.Add (a.fLocation, Fetched_{now, false, configID});
                return nullopt;
            }
        }

    private:
        // on fWorker_: an advertisement in force - just added, or still there (@see Refresh_) - merged into sDiscoveredDevices_
        void Merge_ (const SSDP::Advertisement& a)
        {
            constexpr Activity        kInterprettingSSDPMessageRecieved_{"interpretting SSDP advertisement"sv};
            Debug::TraceContextBumper ctx{"SSDPDeviceDiscoverer_::Merge_", "a={}"_f, a};

            DeclareActivity activity{&kInterprettingSSDPMessageRecieved_};

            Set<InternetAddress> locAddrs = LocationAddresses_ (a.fLocation);
            DiscoveryInfo_       found{};
            found.AddNetworkAddresses_ (locAddrs);
            if (found.fAttachedNetworks.empty ()) {
                // one at an address on none of our networks (link-local, with kIncludeLinkLocalAddressesInDiscovery false, say) makes
                // no device, so its description is not worth fetching
                DbgTrace ("Ignoring SSDP advertisement for device on no network (possibly because of kIncludeLinkLocalAddressesInDiscovery etc suppression)"_f);
                return;
            }

            optional<String> deviceFriendlyName;
            optional<String> deviceType;
            optional<String> manufactureName;
            optional<URI>    manufacturerURL;
            optional<URI>    presentationURL;
            optional<URI>    deviceIconURL;
            if (optional<Frameworks::UPnP::DeviceDescription> deviceInfo = FetchDescriptionIfDue_ (a)) {
                deviceFriendlyName = deviceInfo->fFriendlyName;
                deviceType         = deviceInfo->fDeviceType;
                manufactureName    = deviceInfo->fManufactureName;
                manufacturerURL    = deviceInfo->fManufacturingURL;
                presentationURL    = deviceInfo->fPresentationURL;
                if (deviceInfo->fIcons.has_value () and not deviceInfo->fIcons->empty ()) {
                    deviceIconURL = deviceInfo->fIcons->Nth (0).fURL;
                }
            }

            // merge in data
            StoreMerged_ (found, [&] (const optional<DiscoveryInfo_>& match) {
                DiscoveryInfo_ di = found;
                // Note - we don't generally get hardware address from IPAddress
                if (match) {
                    di = *match; // then merge in possible additions
                    di.AddNetworkAddresses_ (locAddrs);
#if qStroika_Foundation_Debug_AssertionsChecked
                    di.fDebugProps.Add ("Updated-By-SSDPDeviceDiscoverer_-At", DateTime::Now ());
#endif
                }
                else {
#if qStroika_Foundation_Debug_AssertionsChecked
                    di.fDebugProps.Add ("Found-By-SSDPDeviceDiscoverer_-At", DateTime::Now ());
#endif
                    di.fGUID = GUID::GenerateNew ();
                }
                Assert (not di.GetInternetAddresses ().empty ()); // can happen if we find address in tmp.AddIPAddress_() that's not bound to any adapter (but that shouldn't happen so investigate but is for now so ignore briefly)

                if (not di.fSSDPInfo) {
                    di.fSSDPInfo = DiscoveryInfo_::SSDPInfo{};
                }
                di.fSSDPInfo->fAlive = true;

                Memory::CopyToIf (&di.fIcon, deviceIconURL);
                Memory::CopyToIf (&di.fSSDPInfo->fManufacturerURI, manufacturerURL);

                di.fSSDPInfo->fLocations.Add (a.fLocation);
                di.fSSDPInfo->fUSNs.Add (a.fUSN);

                Memory::CopyToIf (&di.fSSDPInfo->fPresentationURL, presentationURL); // consider if value already there - warn if changes - should we collect multiple

                if (di.fSSDPInfo->fServer.has_value () and di.fSSDPInfo->fServer != a.fServer) {
                    DbgTrace ("Warning: different server IDs for same object: di.fSSDPInfo={}, a.fServer='{}"_f, di.fSSDPInfo, a.fServer);
                }
                di.fSSDPInfo->fServer = a.fServer;

                if (deviceType and deviceFriendlyName) {
                    di.fSSDPInfo->fDeviceType2FriendlyNameMap.Add (*deviceType, *deviceFriendlyName);
                }
                Memory::CopyToIf (&di.fSSDPInfo->fManufacturer, manufactureName);

                di.fSSDPInfo->fLastSSDPMessageRecievedAt = Time::DateTime::Now ();
                di.fSeen.fUDP = Memory::NullCoalesce (di.fSeen.fUDP).Extend (di.fSSDPInfo->fLastSSDPMessageRecievedAt);

#if qStroika_Foundation_Debug_AssertionsChecked
                di.fSSDPInfo->fLastAdvertisement = a;
#endif

                if (not di.fOperatingSystem.has_value ()) {
                    if (di.fSSDPInfo->fServer and di.fSSDPInfo->fServer->Contains ("Linux"_k)) {
                        di.fOperatingSystem = Discovery::OperatingSystem{"Linux"_k};
                    }
                    else if (di.fSSDPInfo->fServer and di.fSSDPInfo->fServer->Contains ("POSIX"_k)) {
                        di.fOperatingSystem = Discovery::OperatingSystem{"Unix"_k};
                    }
                }
                di.PatchDerivedFields ();
                return di;
            });
        }

        // on fWorker_: an advertisement removed - by an ssdp:byebye, or its max-age running out - leaving inForce. What of it is no
        // longer in force is no longer listed; and with nothing of its device in force, the device is no longer alive (and Refresh_
        // no longer extends its fSeen.fUDP)
        void Removed_ (const SSDP::Advertisement& a, const Collection<SSDP::Advertisement>& inForce)
        {
            Debug::TraceContextBumper ctx{"SSDPDeviceDiscoverer_::Removed_", "a={}"_f, a};
            bool locationInForce = inForce.Any ([&] (const SSDP::Advertisement& i) { return i.fLocation == a.fLocation; });
            if (not locationInForce) {
                fFetched_.RemoveIf (a.fLocation); // so, added again, its description is fetched again
            }
            DiscoveryInfo_ found{};
            found.AddNetworkAddresses_ (LocationAddresses_ (a.fLocation));
            StoreMerged_ (found, [&] (const optional<DiscoveryInfo_>& match) -> optional<DiscoveryInfo_> {
                if (not match or not match->fSSDPInfo) {
                    return nullopt;
                }
                DiscoveryInfo_ di = *match;
                if (not inForce.Any ([&] (const SSDP::Advertisement& i) { return i.fUSN == a.fUSN; })) {
                    di.fSSDPInfo->fUSNs.RemoveIf (a.fUSN);
                }
                if (not locationInForce) {
                    di.fSSDPInfo->fLocations.RemoveIf (a.fLocation);
                }
                if (di.fSSDPInfo->fUSNs.empty ()) {
                    di.fSSDPInfo->fAlive = false;
                }
                di.PatchDerivedFields (); // fDebugProps' SSDPInfo among them
                return di;
            });
        }

        // on fWorker_, each minute: what is still in force, merged again - one advertisement per LOCATION - so each device's
        // fSeen.fUDP is extended, and its description fetched again if due
        void Refresh_ (const Collection<SSDP::Advertisement>& inForce)
        {
            Debug::TraceContextBumper         ctx{"SSDPDeviceDiscoverer_::Refresh_"};
            Mapping<URI, SSDP::Advertisement> byLocation;
            for (const SSDP::Advertisement& a : inForce) {
                byLocation.Add (a.fLocation, a);
            }
            for (const KeyValuePair<URI, SSDP::Advertisement>& i : byLocation) {
                IgnoreExceptionsExceptThreadAbortForCall (Merge_ (i.fValue));
            }
        }

    private:
        Mapping<URI, Fetched_> fFetched_; // only fWorker_ uses it
        // destroyed in reverse order: the timer, then the listener - so nothing more is queued - then fWorker_, which aborts the
        // change it is on, and waits for it
        ThreadPool fWorker_{ThreadPool::Options{.fThreadCount = 1, .fThreadPoolName = "SSDPDeviceDiscoverer"_k}};
        unique_ptr<SSDP::Client::CachingListener> fListener_;
        IntervalTimer::Adder                      fIntervalTimerAdder_;
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
            : fMyThread_{Thread::CleanupPtr::eAbortBeforeWaiting, Thread::New (DiscoveryChecker_, Thread::eAutoStart, "MyNeighborDiscoverer"sv)}
        {
        }

    private:
        static void DiscoveryChecker_ ()
        {
            Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs ("{}::MyNeighborDiscoverer_::DiscoveryChecker_")};
            static constexpr Activity kDiscovering_NetNeighbors_{"discovering this network neighbors"sv};
            using Neighbor = NeighborsMonitor::Neighbor;
            NeighborsMonitor monitor{};
            while (true) {
                try {
                    DeclareActivity           da{&kDiscovering_NetNeighbors_};
                    Debug::TraceContextBumper ctx1{Stroika_Foundation_Debug_OptionalizeTraceArgs ("monitor.GetNeighbors ()")};
                    for (const Neighbor& i : monitor.GetNeighbors ()) {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
                        DbgTrace ("i={}"_f, i);
#endif
                        // soon store/pay attention to macaddr as better indicator of unique device id than ip addr

                        // ignore multicast addresses as they are not real devices(???always???)
                        if (i.fInternetAddress.IsMulticastAddress ()) {
                            //DbgTrace (L"ignoring arped multicast address %s", Characters::ToString (i.ia).c_str ());
                            continue;
                        }

                        DiscoveryInfo_ found{};
                        found.AddNetworkAddresses_ (i.fInternetAddress, i.fHardwareAddress);
                        if (found.fAttachedNetworks.empty ()) {
                            // so no hardware or internet address either - matching no device
                            DbgTrace ("Ignoring MyNeighborDiscoverer_ neighbor {} because it was not on a known network"_f, i);
                            continue;
                        }
#if qLOCK_DEBUGGING_
                        Debug::TraceContextBumper ctxLock1{"sDiscoveredDevices_ - discovering this network neighbors "};
#endif

                        // merge in data
                        StoreMerged_ (found, [&] (const optional<DiscoveryInfo_>& match) {
                            DiscoveryInfo_ di = found;
                            if (match) {
                                di = *match;
                                di.AddNetworkAddresses_ (i.fInternetAddress, i.fHardwareAddress); // merge in additions
#if qStroika_Foundation_Debug_AssertionsChecked
                                di.fDebugProps.Add ("Updated-By-MyNeighborDiscoverer_-At", DateTime::Now ());
#endif
                            }
                            else {
                                di.fGUID = GUID::GenerateNew ();
#if qStroika_Foundation_Debug_AssertionsChecked
                                di.fDebugProps.Add ("Found-By-MyNeighborDiscoverer_-At"sv, DateTime::Now ());
                                di.fDebugProps.Add ("Found-By-MyNeighborDiscoverer_-I"sv, Characters::ToString (i)); // to debug why sometimes we add but has no network info
#endif
                            }
                            Assert (not di.GetInternetAddresses ().empty ()); // can happen if we find address in tmp.AddIPAddress_() that's not bound to any adapter (but that shouldn't happen so investigate but is for now so ignore breifly)

                            di.fSeen.fARP = Memory::NullCoalesce (di.fSeen.fARP).Extend (DateTime::Now ());

                            di.PatchDerivedFields ();
                            return di;
                        });
                    }
                }
                catch (const Thread::AbortException&) {
                    Execution::ReThrow ();
                }
                catch (...) {
                    Logger::sThe.Log (Logger::eError, "{}"_f, current_exception ());
                }
                Execution::Sleep (1min); // unsure of right interval - maybe able to epoll or something so no actual polling needed - note no lock held here
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
            : fMyThread_{Thread::CleanupPtr::eAbortBeforeWaiting, Thread::New (Checker_, Thread::eAutoStart, "RandomWalkThroughSubnetDiscoverer"sv)}
        {
        }

    private:
        static void Checker_ ()
        {
            Debug::TraceContextBumper ctx{"RandomWalkThroughSubnetDiscoverer_::Checker_"};
            static constexpr Activity kDiscovering_THIS_{"discovering by random scans"sv};

            static constexpr auto kMinTimeBetweenScans_{5s};
            static constexpr auto kOnErrorTimeBetweenScans_{30s};

            //constexpr auto               kAllowedNetworkStaleness_ = 1min;
            constexpr Time::DurationSeconds kAllowedNetworkStaleness_ = 60s;

            /*
             *  Use a BloomFilter instead of a Set<> since we dont want to waste alot of memory storing
             *  EVERY item we visited and discarded, and we dont need to be perfect, its a slow random walk and devices
             *  could appear and disappear during a scan anyhow...
             */
            optional<DiscreteRange<InternetAddress>> scanAddressRange;
            unique_ptr<Cache::BloomFilter<int>>      addressesProbablyUsed;

            double sizeFactor{1}; // (DOESNT APPEAR NEEDED) - use more bloom filter bits than needed for full set, cuz otherwise get too many collisions as adding
            double maxFalsePositivesAllowed      = .5;  // bloom filter stops working well if much past this probability limit
            double maxFractionOfAddrSpaceScanned = .75; // our algorithm wastes alot of time computing random numbers past this limit

            chrono::time_point rateLimiterWaitUntil = chrono::steady_clock::now ();
            while (true) {
                try {
                    DeclareActivity da{&kDiscovering_THIS_};

                    Execution::SleepUntil (rateLimiterWaitUntil);
                    rateLimiterWaitUntil = chrono::steady_clock::now () + kMinTimeBetweenScans_;

                    // Keep scanning the given range til we're (mostly) done
                    if (not scanAddressRange) {
                        Sequence<Discovery::Network> activeNetworks = Discovery::NetworksMgr::sThe.CollectActiveNetworks (kAllowedNetworkStaleness_);
                        if (activeNetworks.empty ()) {
                            DbgTrace ("No active network, so postponing random device address scan"_f);
                            rateLimiterWaitUntil = chrono::steady_clock::now () + kOnErrorTimeBetweenScans_;
                            continue;
                        }
                        // Scanning really only works for IPv4 since too large a range otherwise
                        for (const Discovery::Network& nw : activeNetworks) {
                            for (const CIDR& cidr : nw.fNetworkAddresses) {
                                if (cidr.GetBaseInternetAddress ().GetAddressFamily () == InternetAddress::AddressFamily::V4) {
                                    scanAddressRange = cidr.GetRange ();
                                    DbgTrace ("Selecting scanAddressRange={}"_f, scanAddressRange);
                                    break;
                                }
                            }
                            if (scanAddressRange) {
                                break;
                            }
                        }
                        if (scanAddressRange) {
                            addressesProbablyUsed = make_unique<Cache::BloomFilter<int>> (
                                static_cast<size_t> (sizeFactor * scanAddressRange->GetNumberOfContainedPoints ()));
                        }
                    }
                    if (not scanAddressRange) {
                        // try again later
                        DbgTrace ("No active IPV4 network, so postponing random device address scan"_f);
                        rateLimiterWaitUntil = chrono::steady_clock::now () + kOnErrorTimeBetweenScans_;
                        continue;
                    }
                    AssertNotNull (addressesProbablyUsed);

                    //
                    // pick first few addresses randomly, and when nearly full, clear, and try again
                    // This doesn't guarantee scanning every address, but the number of addresses could be large (e.g. class B network)
                    // and it takes so long to scan, we'll miss a bunch anyhow. Retrying later statistically guarantees we find everything
                    // that's responding and around long enuf
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
                        DbgTrace ("Completed full ({}/{} => {} fraction) scan of (scanAddressRange={}), with randomCollisions={}, resetting list, to start rescanning..."_f,
                                  bloomFilterStats.fApparentlyDistinctAddCalls, scanAddressRange->GetNumberOfContainedPoints (),
                                  double (bloomFilterStats.fApparentlyDistinctAddCalls) / scanAddressRange->GetNumberOfContainedPoints (),
                                  scanAddressRange, bloomFilterStats.fActualAddCalls - bloomFilterStats.fApparentlyDistinctAddCalls);
                        DbgTrace ("addressesProbablyUsed.GetStatistics ()={}"_f, bloomFilterStats);
                        addressesProbablyUsed.reset ();
                        scanAddressRange.reset ();
                        rateLimiterWaitUntil = chrono::steady_clock::now () + 15s;
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
                            DbgTrace ("No obvious device at ip {} because no scan results (ScanOptions::eQuick)"_f, ia);
                        }
                        else {
                            DiscoveryInfo_ found{};
                            found.AddNetworkAddresses_ (ia);
                            StoreMerged_ (found, [&] (const optional<DiscoveryInfo_>& match) {
                                DiscoveryInfo_ di = found;
                                if (match) {
                                    WeakAssertNotReached (); // This case should basically never happen (maybe lose support) - because we check before running ping if its already in the list
                                    // if found, update to say what ports we found
                                    di = *match;
                                    Memory::AccumulateIf (&di.fOpenPorts, scanResults.fDiscoveredOpenPorts);
                                    PatchSeen_ (&di, scanResults);
                                    di.PatchDerivedFields ();
#if qStroika_Foundation_Debug_AssertionsChecked
                                    di.fDebugProps.Add ("Updated-By-RandomWalkThroughSubnetDiscoverer_-At"sv, DateTime::Now ());
#endif
                                    DbgTrace ("Updated device {} for fKnownOpenPorts: {}"_f, di.fGUID, scanResults.fDiscoveredOpenPorts);
                                }
                                else {
                                    di.fGUID = GUID::GenerateNew ();
                                    // only CREATE an entry for addresses where we found a port
                                    di.fOpenPorts = scanResults.fDiscoveredOpenPorts;
                                    PatchSeen_ (&di, scanResults);
                                    di.PatchDerivedFields ();
                                    // NOTE - at this point - we have no hardware address - could get from IO::Network::Neighbors API, but too costly, not worth while here
#if qStroika_Foundation_Debug_AssertionsChecked
                                    di.fDebugProps.Add (L"Found-By-RandomWalkThroughSubnetDiscoverer_-At", DateTime::Now ());
#endif
                                    DbgTrace ("Added device {} for fKnownOpenPorts: {}"_f, di.fGUID, scanResults.fDiscoveredOpenPorts);
                                }
                                Assert (not di.GetInternetAddresses ().empty ()); // shouldn't happen
                                return di;
                            });
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
                            auto           l = sDiscoveredDevices_.cget ();
                            DiscoveryInfo_ tmp{};
                            tmp.AddNetworkAddresses_ (ia);
                            if (optional<DiscoveryInfo_> oo = FindMatchingDevice_ (l, tmp)) {
                                need2CheckAddr = false;
                            }
                        }
                        if (need2CheckAddr) {
                            runPingCheck (ia);
                        }
                    }
                }
                catch (const Thread::AbortException&) {
                    Execution::ReThrow ();
                }
                catch (...) {
                    Logger::sThe.Log (Logger::eError, "{}"_f, current_exception ());
                    rateLimiterWaitUntil = chrono::steady_clock::now () + 30s;
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
            static constexpr Activity kDiscovering_THIS_{"checking status of active devices"sv};

            static constexpr auto kMinTimeBetweenScans_{5s};

            //constexpr auto               kAllowedNetworkStaleness_ = 1min;
            //constexpr Time::DurationSecondsType kAllowedNetworkStaleness_ = 60;

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
                        DbgTrace ("Port scanning on existing device {} (addr {}) returned these ports: {}"_f, deviceID, ia, scanResults.fDiscoveredOpenPorts);
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
                            optional<DiscoveryInfo_> stored =
                                StoreMerged_ (deviceID, [&] (const optional<DiscoveryInfo_>& match) -> optional<DiscoveryInfo_> {
                                    if (not match) {
                                        return nullopt;
                                    }
                                    // update to say what ports we found
                                    DiscoveryInfo_ di = *match;
                                    Memory::AccumulateIf (&di.fOpenPorts, scanResults.fDiscoveredOpenPorts);
                                    PatchSeen_ (&di, scanResults);
                                    di.PatchDerivedFields ();
#if qStroika_Foundation_Debug_AssertionsChecked
                                    di.fDebugProps.Add (L"Updated-By-KnownDevicePortScanner_-At", DateTime::Now ());
#endif
                                    DbgTrace ("Updated device {} for fKnownOpenPorts: {}"_f, di.fGUID, scanResults.fDiscoveredOpenPorts);
                                    return di;
                                });
                            if (not stored) {
                                WeakAssertNotReached (); // objects CAN disappear from list of devices (eventually we will support expiring/deleting)
                            }
                        }
                    };

                    if (auto o = sDiscoveredDevices_.cget ().cref ().Lookup (**devices2CheckIterator)) {
                        for (const auto& ia : o->GetInternetAddresses ()) {
                            runPingCheck (o->fGUID, ia);
                        }
                    }
                    ++(*devices2CheckIterator);
                }
                catch (const Thread::AbortException&) {
                    Execution::ReThrow ();
                }
                catch (...) {
                    Logger::sThe.Log (Logger::eError, "{}"_f, current_exception ());
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
    constexpr Time::DurationSeconds kDefaultItemCacheLifetime_{1s}; // this costs very little since just reading already cached data so default to quick check

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
    Debug::TraceContextBumper ctx{"Discovery::DevicesMgr::Activator::Activator"};
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
    Debug::TraceContextBumper ctx{"Discovery::DevicesMgr::Activator::~Activator"};
    Require (IsActive_ ());
    Execution::Thread::SuppressInterruptionInContext suppressInterruption; // must complete this abort and wait for done - this cannot abort/throw
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

optional<GUID> Discovery::DevicesMgr::GetThisDeviceID () const
{
    if (sMyDeviceDiscoverer_ != nullptr) {
        return sMyDeviceDiscoverer_->GetThisDeviceID ();
    }
    return nullopt;
}

Collection<Discovery::Device> Discovery::DevicesMgr::GetActiveDevices (optional<Time::DurationSeconds> allowedStaleness) const
{
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    Debug::TraceContextBumper ctx{"Discovery::GetActiveDevices"};
#endif
    Debug::TimingTrace ttrc{"Discovery::DevicesMgr::GetActiveDevices", 1.0s};

    Require (IsActive_ ());
    Collection<Discovery::Device> results;
    static Cache::TimedCache      sCache_ = Cache::SynchronizedTimedCache<void, Collection<Discovery::Device>>{};
    results                               = sCache_.LookupValue (allowedStaleness.value_or (kDefaultItemCacheLifetime_), [] () {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
        DbgTrace ("sDiscoveredDevices_: {}"_f, sDiscoveredDevices_.load ());
#endif
        // NOTE - intentionally omit devices with no hardware addresses
        return sDiscoveredDevices_.load ().Where ([] (const Discovery::Device& d) { return not d.GetHardwareAddresses ().empty (); }); // intentionally object-spice
    });
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace ("returns: {}"_f, results);
#endif
    return results;
}

void Discovery::DevicesMgr::ReScan (const GUID& deviceID)
{
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    Debug::TraceContextBumper ctx{"Discovery::ReScan"};
#endif
    Debug::TimingTrace        ttrc{"Discovery::DevicesMgr::ReScan"};
    static constexpr Activity kRescanning_Device_{"rescanning device"sv};
    DeclareActivity           da{&kRescanning_Device_};

    auto findDeviceInfoAndClearFoundPorts = [] (const GUID& deviceID) {
        optional<DiscoveryInfo_> stored = StoreMerged_ (deviceID, [] (const optional<DiscoveryInfo_>& match) -> optional<DiscoveryInfo_> {
            if (not match) {
                return nullopt;
            }
            DiscoveryInfo_ di = *match;
            di.PatchDerivedFields ();
            di.fOpenPorts = nullopt;
            return di;
        });
        if (not stored) {
            Execution::Throw (IO::Network::HTTP::ClientErrorException{"deviceID not recognized"sv});
        }
        return *stored;
    };
    auto addOpenPorts = [] (const GUID& deviceID, const PortScanResults& portScanResults) {
        optional<DiscoveryInfo_> stored = StoreMerged_ (deviceID, [&] (const optional<DiscoveryInfo_>& match) -> optional<DiscoveryInfo_> {
            if (not match) {
                return nullopt;
            }
            DiscoveryInfo_ di = *match;
            for (const String& p : portScanResults.fDiscoveredOpenPorts) {
                Memory::AccumulateIf (&di.fOpenPorts, p);
            }
            PatchSeen_ (&di, portScanResults);
            di.PatchDerivedFields ();
            DbgTrace ("Updated device {} for fKnownOpenPorts: {}"_f, di.fGUID, portScanResults.fDiscoveredOpenPorts);
            return di;
        });
        if (not stored) {
            AssertNotReached ();
        }
    };

    DiscoveryInfo_ initialDeviceInfo = findDeviceInfoAndClearFoundPorts (deviceID);
    // now now just run scan using limited portscan API
    // but redo scanning one at a time so I can SHOW results immediately, as they appear
    for (const auto& ia : initialDeviceInfo.GetInternetAddresses ()) {
        PortScanResults results = ScanPorts (ia, ScanOptions{ScanOptions::eFull});
        addOpenPorts (deviceID, results);
    }
}

VariantValue DevicesMgr::ScanAndReturnReport (const InternetAddress& addr)
{
    PortScanResults               results = ScanPorts (addr, ScanOptions{ScanOptions::eFull});
    Mapping<String, VariantValue> result;
    result.Add ("openPorts", VariantValue{results.fDiscoveredOpenPorts.Map<Sequence<VariantValue>> ([] (String i) { return VariantValue{i}; })});
    return VariantValue{result};
}
