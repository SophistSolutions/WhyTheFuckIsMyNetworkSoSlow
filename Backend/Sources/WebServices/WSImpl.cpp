/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2020.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Configuration/SystemConfiguration.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/DNS.h"
#include "Stroika/Foundation/IO/Network/HTTP/ClientErrorException.h"
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/LinkMonitor.h"

#include "Stroika/Frameworks/NetworkMonitor/Ping.h"
#include "Stroika/Frameworks/NetworkMonitor/Traceroute.h"

#include "Stroika-Current-Version.h"

#include "../Common/BLOBMgr.h"
#include "../Common/EthernetMACAddressOUIPrefixes.h"
#include "../Discovery/Devices.h"
#include "../Discovery/NetworkInterfaces.h"
#include "../Discovery/Networks.h"

#include "AppVersion.h"

#include "WSImpl.h"

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Execution;

using Stroika::Foundation::Common::GUID;
using Stroika::Foundation::IO::Network::HTTP::ClientErrorException;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;

/*
 ********************************************************************************
 ************************************* WSImpl ***********************************
 ********************************************************************************
 */
About WSImpl::GetAbout () const
{
    static const About kAbout_{
        AppVersion::kVersion,
        Mapping<String, Configuration::Version>{{KeyValuePair<String, Configuration::Version>{L"Stroika"sv, Configuration::Version{kStroika_Version_FullVersion}}}},
        OperatingSystem{Configuration::GetSystemConfiguration_ActualOperatingSystem ().fPrettyNameWithVersionDetails}};
    return kAbout_;
}

tuple<Memory::BLOB, DataExchange::InternetMediaType> WSImpl::GetBLOB (const GUID& guid) const
{
    return Common::BLOBMgr::sThe.GetBLOB (guid);
}

Sequence<String> WSImpl::GetDevices (const optional<DeviceSortParamters>& sort) const
{
    Sequence<String> result;
    for (BackendApp::WebServices::Device n : GetDevices_Recurse (sort)) {
        result += Characters::ToString (n.fGUID);
    }
    return result;
}

namespace {
    URI TransformURL2LocalStorage_ (const URI& url)
    {
        Debug::TimingTrace ttrc{L"TransformURL2LocalStorage_", 0.1}; // sb very quick cuz we schedule url fetches for background

        // if we are unable to cache the url (say because the url is bad or the device is currently down)
        // just return the original

        try {
            // This BLOBMgr code wont block - it will push a request into a Q, and fetch whatever data is has (maybe none)
            optional<GUID> g = BackendApp::Common::BLOBMgr::sThe.AsyncAddBLOBFromURL (url);
            // tricky to know right host to supply here - will need some sort of configuration for
            // this (due to firewalls, NAT etc).
            // Use relative URL for now, as that should work for most cases
            if (g) {
                return URI{nullopt, nullopt, L"/blob/" + g->ToString ()};
            }
        }
        catch (...) {
            AssertNotReached ();
        }
        DbgTrace (L"Failed to cache url (%s) - so returning original", Characters::ToString (url).c_str ());
        return url;
    }
    optional<URI> TransformURL2LocalStorage_ (const optional<URI>& url)
    {
        return url ? TransformURL2LocalStorage_ (*url) : optional<URI>{};
    }
}

Sequence<BackendApp::WebServices::Device> WSImpl::GetDevices_Recurse (const optional<DeviceSortParamters>& sort) const
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"WSImpl::GetDevices_Recurse", L"sort=%s", Characters::ToString (sort).c_str ())};
    Debug::TimingTrace        ttrc{L"WSImpl::GetDevices_Recurse", .1};

    // Compute effective sort Search Terms - filling in optional values
    Sequence<DeviceSortParamters::SearchTerm> searchTerms;
    {
        if (sort) {
            searchTerms = sort->fSearchTerms;
        }
        if (searchTerms.empty ()) {
            searchTerms += DeviceSortParamters::SearchTerm{DeviceSortParamters::SearchTerm::By::ePriority, false};
        }
        for (auto i = searchTerms.begin (); i != searchTerms.end (); ++i) {
            if (not i->fAscending.has_value ()) {
                auto p = *i;
                switch (p.fBy) {
                    case DeviceSortParamters::SearchTerm::By::ePriority:
                        p.fAscending = false;
                        break;
                    default:
                        p.fAscending = true;
                        break;
                }
                searchTerms.Update (i, p);
            }
        }
    }

    // Compute effective sortCompareNetwork - as a set of CIDRs
    optional<Set<CIDR>> sortCompareNetwork;
    if (sort and sort->fCompareNetwork) {
        // CIDR will contain a / and GUID won't so use that to distinguish
        if (sort->fCompareNetwork->Contains (L"/"_k)) {
            sortCompareNetwork = Set<CIDR>{CIDR{*sort->fCompareNetwork}}; // OK to throw if invalid
        }
        else {
            for (Discovery::Network n : Discovery::NetworksMgr::sThe.CollectActiveNetworks ()) {
                if (n.fGUID == sort->fCompareNetwork) {
                    sortCompareNetwork = n.fNetworkAddresses;
                }
            }
        }
    }

    DISABLE_COMPILER_MSC_WARNING_START (4456)
    // Fetch (UNSORTED) list of devices
    Sequence<BackendApp::WebServices::Device> devices = Sequence<BackendApp::WebServices::Device>{Discovery::DevicesMgr::sThe.GetActiveDevices ().Select<BackendApp::WebServices::Device> ([] (const Discovery::Device& d) {
        BackendApp::WebServices::Device newDev;
        newDev.fGUID = d.fGUID;
        newDev.name  = d.name;
        if (not d.fTypes.empty ()) {
            newDev.fTypes = d.fTypes; // leave missing if no discovered types
        }
        newDev.fLastSeenAt = d.fLastSeenAt;
        newDev.fOpenPorts  = d.fOpenPorts;
        for (auto i : d.fAttachedNetworks) {
            constexpr bool            kIncludeLinkLocalAddresses_{Discovery::kIncludeLinkLocalAddressesInDiscovery};
            constexpr bool            kIncludeMulticastAddreses_{Discovery::kIncludeMulticastAddressesInDiscovery};
            Sequence<InternetAddress> addrs2Report;
            for (auto i : i.fValue.localAddresses) {
                if (not kIncludeLinkLocalAddresses_ and i.IsLinkLocalAddress ()) {
                    continue;
                }
                if (not kIncludeMulticastAddreses_ and i.IsMulticastAddress ()) {
                    continue;
                }
                addrs2Report += i;
            }
            newDev.fAttachedNetworks.Add (i.fKey, NetworkAttachmentInfo{i.fValue.hardwareAddresses, addrs2Report});
        }
        newDev.fAttachedNetworkInterfaces = d.fAttachedInterfaces; // @todo must merge += (but only when merging across differnt discoverers/networks)
        newDev.fPresentationURL           = d.fPresentationURL;
        newDev.fManufacturer              = d.fManufacturer;
        newDev.fIcon                      = TransformURL2LocalStorage_ (d.fIcon);
        newDev.fOperatingSystem           = d.fOperatingSystem;
#if qDebug
        if (not d.fDebugProps.empty ()) {
            newDev.fDebugProps = d.fDebugProps;
        }
        {
            // List OUI names for each hardware address (and explicit missing for those we cannot lookup)
            using VariantValue = DataExchange::VariantValue;
            Mapping<String, VariantValue> t;
            for (auto i : d.fAttachedNetworks) {
                for (auto hwa : i.fValue.hardwareAddresses) {
                    auto o = BackendApp::Common::LookupEthernetMACAddressOUIFromPrefix (hwa);
                    t.Add (hwa, o ? VariantValue{*o} : VariantValue{});
                }
            }
            if (not t.empty ()) {
                if (not newDev.fDebugProps.has_value ()) {
                    newDev.fDebugProps = Mapping<String, VariantValue>{};
                }
                newDev.fDebugProps->Add (L"MACAddr2OUINames", t);
            }
        }
#endif
        return newDev;
    })};
    DISABLE_COMPILER_MSC_WARNING_END (4456)

    // Sort them
    for (DeviceSortParamters::SearchTerm st : searchTerms) {
        switch (st.fBy) {
            case DeviceSortParamters::SearchTerm::By::ePriority: {
                devices = devices.OrderBy ([st] (const BackendApp::WebServices::Device& lhs, const BackendApp::WebServices::Device& rhs) -> bool {
                    // super primitive sort strategy...
                    auto priFun = [] (const BackendApp::WebServices::Device& d) {
                        int pri = 0;
                        if (d.fTypes and d.fTypes->Contains (Device::DeviceType::ePC)) {
                            pri = 10;
                        }
                        if (d.fTypes and d.fTypes->Contains (Device::DeviceType::eRouter)) {
                            pri = 5;
                        }
                        return pri;
                    };
                    int lPri = priFun (lhs);
                    int rPri = priFun (rhs);
                    Assert (st.fAscending);
                    bool ascending = *st.fAscending;
                    return ascending ? (lPri < rPri) : (lPri > rPri);
                });
                break;
                case DeviceSortParamters::SearchTerm::By::eAddress: {
                    devices = devices.OrderBy ([st, sortCompareNetwork] (const BackendApp::WebServices::Device& lhs, const BackendApp::WebServices::Device& rhs) -> bool {
                        Assert (st.fAscending);
                        bool ascending = *st.fAscending;
                        auto lookup    = [=] (const BackendApp::WebServices::Device& d) -> InternetAddress {
                            if (sortCompareNetwork) {
                                // if multiple, grab the first (somewhat arbitrary) - maybe should grab the least?
                                for (InternetAddress ia : d.GetInternetAddresses ()) {
                                    if (sortCompareNetwork->Any ([&] (const CIDR& cidr) { return cidr.GetRange ().Contains (ia); })) {
                                        return ia;
                                    }
                                }
                                return ascending ? InternetAddress::max () : InternetAddress::min (); // not matching always show up at end of list
                            }
                            else {
                                // @todo - consider which address to use? maybe least if ascending, and max if decesnding?
                                return d.GetInternetAddresses ().NthValue (0);
                            }
                        };
                        InternetAddress l = lookup (lhs);
                        InternetAddress r = lookup (rhs);
                        return ascending ? (l < r) : (l > r);
                    });
                } break;
                case DeviceSortParamters::SearchTerm::By::eName: {
                    devices = devices.OrderBy ([st, sortCompareNetwork] (const BackendApp::WebServices::Device& lhs, const BackendApp::WebServices::Device& rhs) -> bool {
                        Assert (st.fAscending);
                        bool ascending = *st.fAscending;
                        return ascending ? (lhs.name < rhs.name) : (lhs.name > rhs.name);
                    });
                } break;
                case DeviceSortParamters::SearchTerm::By::eType: {
                    devices = devices.OrderBy ([st, sortCompareNetwork] (const BackendApp::WebServices::Device& lhs, const BackendApp::WebServices::Device& rhs) -> bool {
                        Assert (st.fAscending);
                        bool ascending = *st.fAscending;
                        // tricky to compare types cuz we have a set of types. And types in those sets have subtypes
                        // If TS is a subtype (more specific type) from T, then treat TS > T
                        auto mapTypeToOrder = [] (Device::DeviceType dt) -> double {
                            switch (dt) {
                                case Device::DeviceType::ePC:
                                    return 1;
                                case Device::DeviceType::eNetworkInfrastructure:
                                    return 2;
                                case Device::DeviceType::eRouter:
                                    return 2.1;
                                case Device::DeviceType::eTablet:
                                    return 3;
                                case Device::DeviceType::ePhone:
                                    return 4;
                                case Device::DeviceType::eMediaPlayer:
                                    return 5;
                                case Device::DeviceType::eSpeaker:
                                    return 5.1;
                                case Device::DeviceType::eTV:
                                    return 5.2;
                                default:
                                    return 99;
                            }
                        };
                        auto mapTypeToOrder2 = [&] (const optional<Set<Device::DeviceType>> dt) -> double {
                            double f = 99;
                            if (dt) {
                                for (auto d : *dt) {
                                    f = min (f, mapTypeToOrder (d));
                                }
                            }
                            return f;
                        };
                        return ascending ? (mapTypeToOrder2 (lhs.fTypes) < mapTypeToOrder2 (rhs.fTypes)) : (mapTypeToOrder2 (lhs.fTypes) > mapTypeToOrder2 (rhs.fTypes));
                    });
                } break;
                default: {
                    Execution::Throw (ClientErrorException{L"missing or invalid By in search specification"_k});
                } break;
            }
        }
    }

    return devices;
}

Device WSImpl::GetDevice (const String& id) const
{
    Debug::TimingTrace ttrc{L"WSImpl::GetDevice", 0.1};
    GUID               compareWithID = ClientErrorException::TreatExceptionsAsClientError ([&] () { return GUID{id}; });
    // @todo quick hack impl
    for (auto i : GetDevices_Recurse (nullopt)) {
        if (i.fGUID == compareWithID) {
            return i;
        }
    }
    Execution::Throw (ClientErrorException{L"no such id"sv});
}

Sequence<String> WSImpl::GetNetworks () const
{
    Debug::TimingTrace ttrc{L"WSImpl::GetNetworks", 0.1};
    Sequence<String>   result;
    for (Discovery::Network n : Discovery::NetworksMgr::sThe.CollectActiveNetworks ()) {
        result += Characters::ToString (n.fGUID);
    }
    return result;
}

Sequence<BackendApp::WebServices::Network> WSImpl::GetNetworks_Recurse () const
{
    Debug::TimingTrace                         ttrc{L"WSImpl::GetNetworks_Recurse", 0.1};
    Sequence<BackendApp::WebServices::Network> result;

    // @todo parameterize if we return all or just active networks
    for (Discovery::Network n : Discovery::NetworksMgr::sThe.CollectActiveNetworks ()) {
        BackendApp::WebServices::Network nw{n.fNetworkAddresses};

        nw.fGUID                    = n.fGUID;
        nw.fFriendlyName            = n.fFriendlyName;
        nw.fNetworkAddresses        = n.fNetworkAddresses;
        nw.fAttachedInterfaces      = n.fAttachedNetworkInterfaces;
        nw.fDNSServers              = n.fDNSServers;
        nw.fGateways                = n.fGateways;
        nw.fExternalAddresses       = n.fExternalAddresses;
        nw.fGEOLocInformation       = n.fGEOLocInfo;
        nw.fInternetServiceProvider = n.fISP;
#if qDebug
        if (not n.fDebugProps.empty ()) {
            nw.fDebugProps = n.fDebugProps;
        }
#endif

        result += nw;
    }
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace (L"returns: %s", Characters::ToString (result).c_str ());
#endif
    return result;
}

Network WSImpl::GetNetwork (const String& id) const
{
    Debug::TimingTrace ttrc{L"WSImpl::GetNetwork", 0.1};
    GUID               compareWithID = ClientErrorException::TreatExceptionsAsClientError ([&] () { return GUID{id}; });
    // @todo quick hack impl
    for (auto i : GetNetworks_Recurse ()) {
        if (i.fGUID == compareWithID) {
            return i;
        }
    }
    Execution::Throw (ClientErrorException{L"no such id"sv});
}

Collection<String> WSImpl::GetNetworkInterfaces (bool filterRunningOnly) const
{
    Debug::TimingTrace ttrc{L"WSImpl::GetNetworkInterfaces", 0.1};
    Collection<String> result;

    for (Discovery::NetworkInterface n : Discovery::NetworkInterfacesMgr::sThe.CollectAllNetworkInterfaces ()) {
        bool passedFilter{true};
        if (filterRunningOnly) {
            if (n.fStatus.has_value () and not n.fStatus->Contains (IO::Network::Interface::Status::eConnected)) {
                passedFilter = false;
            }
        }
        if (passedFilter) {
            result += Characters::ToString (n.fGUID);
        }
    }
    return result;
}

Collection<BackendApp::WebServices::NetworkInterface> WSImpl::GetNetworkInterfaces_Recurse (bool filterRunningOnly) const
{
    Collection<BackendApp::WebServices::NetworkInterface> result;

    for (Discovery::NetworkInterface n : Discovery::NetworkInterfacesMgr::sThe.CollectAllNetworkInterfaces ()) {
        bool passedFilter{true};
        if (filterRunningOnly) {
            if (n.fStatus.has_value () and not n.fStatus->Contains (IO::Network::Interface::Status::eConnected)) {
                passedFilter = false;
            }
        }
        if (passedFilter) {
            BackendApp::WebServices::NetworkInterface nw{n};

            /**
             */
            nw.fGUID = n.fGUID;
#if qDebug
            if (not n.fDebugProps.empty ()) {
                nw.fDebugProps = n.fDebugProps;
            }
#endif

            result += nw;
        }
    }
    return result;
}

NetworkInterface WSImpl::GetNetworkInterface (const String& id) const
{
    GUID compareWithID = ClientErrorException::TreatExceptionsAsClientError ([&] () { return GUID{id}; });

    // @todo quick hack impl
    for (auto i : GetNetworkInterfaces_Recurse (false)) {
        if (i.fGUID == compareWithID) {
            return i;
        }
    }
    Execution::Throw (ClientErrorException{L"no such id"sv});
}

double WSImpl::Operation_Ping (const String& address) const
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"WSImpl::Operation_Ping", L"address=%s", Characters::ToString (address).c_str ())};

    using namespace Stroika::Foundation::IO::Network;
    using namespace Stroika::Foundation::IO::Network::InternetProtocol;
    using namespace Stroika::Foundation::Time;
    using namespace Stroika::Frameworks;
    using namespace Stroika::Frameworks::NetworkMonitor;

    size_t                packetSize  = Ping::Options::kDefaultPayloadSize + sizeof (ICMP::V4::PacketHeader); // historically, the app ping has measured this including ICMP packet header, but not ip packet header size
    unsigned int          maxHops     = Ping::Options::kDefaultMaxHops;
    unsigned int          sampleCount = 3;
    static const Duration kInterSampleTime_{"PT.1S"};

    Ping::Options options{};
    options.fPacketPayloadSize = Ping::Options::kAllowedICMPPayloadSizeRange.Pin (packetSize - sizeof (ICMP::V4::PacketHeader));
    options.fMaxHops           = maxHops;
    //   options.fSampleInfo                   = Ping::Options::SampleInfo{kInterSampleTime_, sampleCount};

    // write GetHostAddress () function in DNS that throws if not at least one
    auto addrs = DNS::Default ().GetHostAddresses (address, InternetAddress::AddressFamily::V4);
    if (addrs.size () < 1) {
        Execution::Throw (Execution::Exception{L"no addr"sv});
    }

    NetworkMonitor::Ping::SampleResults t = NetworkMonitor::Ping::Sample (addrs[0], Ping::SampleOptions{kInterSampleTime_, sampleCount}, options);
    if (t.fMedianPingTime) {
        return t.fMedianPingTime->As<double> ();
    }

    return numeric_limits<double>::infinity ();
}

Operations::TraceRouteResults WSImpl::Operation_TraceRoute (const String& address, optional<bool> reverseDNSResults) const
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"WSImpl::Operation_TraceRoute", L"address=%s, reverseDNSResults=%s", Characters::ToString (address).c_str (), Characters::ToString (reverseDNSResults).c_str ())};

    using namespace Stroika::Foundation::IO::Network;
    using namespace Stroika::Foundation::IO::Network::InternetProtocol;
    using namespace Stroika::Foundation::Time;
    using namespace Stroika::Frameworks;
    using namespace Stroika::Frameworks::NetworkMonitor;

    bool revDNS = reverseDNSResults.value_or (true);

    size_t                packetSize  = Ping::Options::kDefaultPayloadSize + sizeof (ICMP::V4::PacketHeader); // historically, the app ping has measured this including ICMP packet header, but not ip packet header size
    unsigned int          maxHops     = Ping::Options::kDefaultMaxHops;
    unsigned int          sampleCount = 3;
    static const Duration kInterSampleTime_{"PT.1S"};

    Traceroute::Options options{};
    options.fPacketPayloadSize = Ping::Options::kAllowedICMPPayloadSizeRange.Pin (packetSize - sizeof (ICMP::V4::PacketHeader));
    options.fMaxHops           = maxHops;

    options.fTimeout = Duration{5.0};
    //   options.fSampleInfo                   = Ping::Options::SampleInfo{kInterSampleTime_, sampleCount};

    // write GetHostAddress () funciton in DNS that throws if not at least one

    Model::Operations::TraceRouteResults results;

    Sequence<Traceroute::Hop> hops = Traceroute::Run (DNS::Default ().GetHostAddress (address), options);
    // unsigned int              hopIdx{1};
    for (Traceroute::Hop h : hops) {
        String hopName = [=] () {
            String addrStr = h.fAddress.As<String> ();
            if (revDNS) {
                if (auto rdnsName = DNS::Default ().QuietReverseLookup (h.fAddress)) {
                    return *rdnsName;
                }
            }
            return addrStr;
        }();
        results.fHops += Operations::TraceRouteResults::Hop{h.fTime, hopName};
        // cout << hopIdx++ << "\t" << h.fTime.PrettyPrint ().AsNarrowSDKString () << "\t" << hopName.AsNarrowSDKString () << endl;
    }

    return results;
}

Time::Duration WSImpl::Operation_DNS_CalculateNegativeLookupTime (optional<unsigned int> samples) const
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"WSImpl::Operation_DNS_CalculateNegativeLookupTime")};
    constexpr unsigned int    kDefault_Samples = 7;
    unsigned int              useSamples       = samples.value_or (kDefault_Samples);
    if (useSamples == 0) {
        Execution::Throw (ClientErrorException{L"samples must be > 0"sv});
    }
    uniform_int_distribution<mt19937::result_type> allUInt16Distribution{0, numeric_limits<uint32_t>::max ()};
    static mt19937                                 sRng_{std::random_device () ()};
    Sequence<Time::DurationSecondsType>            measurements;
    for (unsigned int i = 0; i < useSamples; ++i) {
        String                    randomAddress = Characters::Format (L"www.xxxabc%d.com", allUInt16Distribution (sRng_));
        Time::DurationSecondsType startAt       = Time::GetTickCount ();
        IgnoreExceptionsForCall (IO::Network::DNS::Default ().GetHostAddress (randomAddress));
        measurements += Time::GetTickCount () - startAt;
    }
    Assert (measurements.Median ().has_value ());
    return Time::Duration{*measurements.Median ()};
}

Operations::DNSLookupResults WSImpl::Operation_DNS_Lookup (const String& name) const
{
    Debug::TraceContextBumper    ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"WSImpl::Operation_DNS_Lookup", L"name=%s", name.c_str ())};
    Operations::DNSLookupResults result;
    Time::DurationSecondsType    startAt = Time::GetTickCount ();
    IgnoreExceptionsForCall (result.fResult = Characters::ToString (IO::Network::DNS::Default ().GetHostAddress (name)));
    result.fLookupTime = Time::Duration{Time::GetTickCount () - startAt};
    return result;
}

double WSImpl::Operation_DNS_CalculateScore () const
{
    // decent estimate of score is (weighted) sum of these numbers - capped at some maximum, and then 1-log of that number (log to skew so mostly sensative to small differences around small numbers and big is big, and 1- to get 1 better score than zero)
    double           totalWeightedTime{};
    constexpr double kNegLookupWeight = 2.5;
    totalWeightedTime += kNegLookupWeight * Operation_DNS_CalculateNegativeLookupTime ({}).As<double> ();
    constexpr double kPosLookupWeight = 25; // much higher than kNegLookupWeight because this is the time for cached entries lookup which will naturally be much smaller
    totalWeightedTime += kPosLookupWeight * (0 + Operation_DNS_Lookup (L"www.google.com"sv).fLookupTime.As<double> () + Operation_DNS_Lookup (L"www.amazon.com"sv).fLookupTime.As<double> () + Operation_DNS_Lookup (L"www.youtube.com"sv).fLookupTime.As<double> ());
    Assert (totalWeightedTime >= 0);
    constexpr double kScoreCutOff_               = 10.0;
    constexpr double kShiftAndScaleVerticallyBy_ = 10;
    double           score{(kShiftAndScaleVerticallyBy_ - log (totalWeightedTime / (kScoreCutOff_ / 10))) / kShiftAndScaleVerticallyBy_};

    //DbgTrace (L"totalWeightedTime=%f", totalWeightedTime);
    //DbgTrace (L"log=%f", log (totalWeightedTime / (kScoreCutOff_ / 10)));
    //DbgTrace (L"score=%f", score);

    score = Math::PinInRange<double> (score, 0, 1);
    Ensure (0 <= score and score <= 1.0);
    return score;
}

DataExchange::VariantValue WSImpl::Operation_Scan_FullRescan (const String& deviceID) const
{
    Debug::TraceContextBumper  ctx{L"WSImpl::Operation_Scan_FullRescan"};
    DataExchange::VariantValue x;
    GUID                       useDeviceID = ClientErrorException::TreatExceptionsAsClientError ([&] () { return GUID{deviceID}; });
    Discovery::DevicesMgr::sThe.InitiateReScan (useDeviceID);
    return x;
}

DataExchange::VariantValue WSImpl::Operation_Scan_Scan (const String& addr) const
{
    Debug::TraceContextBumper  ctx{L"WSImpl::Operation_Scan_Scan"};
    DataExchange::VariantValue x;
    InternetAddress            useAddr = ClientErrorException::TreatExceptionsAsClientError ([&] () { return IO::Network::DNS::Default ().GetHostAddress (addr); });
    return Discovery::DevicesMgr::sThe.ScanAndReturnReport (useAddr);
}
