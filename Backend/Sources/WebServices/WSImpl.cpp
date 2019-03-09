/*
* Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Configuration/SystemConfiguration.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/DNS.h"
#include "Stroika/Foundation/IO/Network/HTTP/ClientErrorException.h"
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/LinkMonitor.h"

#include "Stroika/Frameworks/NetworkMonitor/Ping.h"
#include "Stroika/Frameworks/NetworkMonitor/Traceroute.h"

#include "Stroika-Current-Version.h"

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
		 OperatingSystem{Configuration::GetSystemConfiguration_OperatingSystem ().fPrettyNameWithMajorVersion}
	};
    return kAbout_;
}

Collection<String> WSImpl::GetDevices () const
{
    Collection<String> result;
    for (BackendApp::WebServices::Device n : GetDevices_Recurse ()) {
        result += Characters::ToString (n.fGUID);
    }
    return result;
}

Collection<BackendApp::WebServices::Device> WSImpl::GetDevices_Recurse () const
{
    using namespace IO::Network;
    Sequence<BackendApp::WebServices::Device> devices = Sequence<BackendApp::WebServices::Device>{Discovery::DevicesMgr::sThe.GetActiveDevices ().Select<BackendApp::WebServices::Device> ([](const Discovery::Device& d) {
        BackendApp::WebServices::Device newDev;
        d.ipAddresses.Apply ([&](const InternetAddress& a) {
            // prefer having IPv4 addr at head of list
            //
            //@todo - CRAP CODE - RETHINK!!!
            String addrStr = a.As<String> ();
            if (not newDev.ipAddresses.Contains (addrStr)) {
                if (auto o = a.AsAddressFamily (InternetAddress::AddressFamily::V4)) {
                    if (newDev.ipAddresses.Contains (o->As<String> ())) {
                        newDev.ipAddresses.Remove (*newDev.ipAddresses.IndexOf (o->As<String> ()));
                    }
                    newDev.ipAddresses.Prepend (o->As<String> ());
                }
                if (not newDev.ipAddresses.Contains (addrStr)) {
                    newDev.ipAddresses.Append (addrStr);
                }
                if (auto o = a.AsAddressFamily (InternetAddress::AddressFamily::V6)) {
                    if (not newDev.ipAddresses.Contains (o->As<String> ())) {
                        newDev.ipAddresses.Append (o->As<String> ());
                    }
                }
            }
        });

        newDev.fGUID                      = d.fGUID;
        newDev.name                       = d.name;
        newDev.fTypes                     = d.fTypes;
        newDev.fAttachedNetworks          = d.fNetworks;
        newDev.fAttachedNetworkInterfaces = d.fAttachedInterfaces; // @todo must merge += (but only when merging across differnt discoverers/networks)
		newDev.fPresentationURL = d.fPresentationURL;
		newDev.fOperatingSystem = d.fOperatingSystem;
        return newDev;
    })};

    //tmphack - convert to vector<> cuz Stroika sequence doesnt supprot randomaccessiterators and so cannot easily sort
    // This won't work (as is) anymore when we return the RIGHT VALUE for the type - just counting on ahck where my computer gets assgined type Laptop --LGP 2019-02-18
    vector<BackendApp::WebServices::Device> dv = devices.As<vector<BackendApp::WebServices::Device>> ();
    sort (dv.begin (), dv.end (), [](const BackendApp::WebServices::Device& lhs, const BackendApp::WebServices::Device& rhs) -> bool {
        int lPri = 0;
        int rPri = 0;
        // super primitive sort strategy...
        if (lhs.fTypes and lhs.fTypes->Contains (Device::DeviceType::ePC)) {
            lPri = 10;
        }
        if (lhs.fTypes and lhs.fTypes->Contains (Device::DeviceType::eRouter)) {
            lPri = 5;
        }
        if (rhs.fTypes and rhs.fTypes->Contains (Device::DeviceType::ePC)) {
            rPri = 10;
        }
        if (rhs.fTypes and rhs.fTypes->Contains (Device::DeviceType::eRouter)) {
            rPri = 5;
        }
        return lPri < rPri;
    });
    return Collection<BackendApp::WebServices::Device>{dv};

    return devices;
}

Device WSImpl::GetDevice (const String& id) const
{
    GUID compareWithID = ClientErrorException::TreatExceptionsAsClientError ([&]() { return GUID{id}; });
    // @todo quick hack impl
    for (auto i : GetDevices_Recurse ()) {
        if (i.fGUID == compareWithID) {
            return i;
        }
    }
    Execution::Throw (ClientErrorException (L"no such id"sv));
}

Sequence<String> WSImpl::GetNetworks () const
{
    Sequence<String> result;
    for (Discovery::Network n : Discovery::NetworksMgr::sThe.CollectActiveNetworks ()) {
        result += Characters::ToString (n.fGUID);
    }
    return result;
}

Sequence<BackendApp::WebServices::Network> WSImpl::GetNetworks_Recurse () const
{
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

        result += nw;
    }
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace (L"returns: %s", Characters::ToString (result).c_str ());
#endif
    return result;
}

Network WSImpl::GetNetwork (const String& id) const
{
    GUID compareWithID = ClientErrorException::TreatExceptionsAsClientError ([&]() { return GUID{id}; });
    // @todo quick hack impl
    for (auto i : GetNetworks_Recurse ()) {
        if (i.fGUID == compareWithID) {
            return i;
        }
    }
    Execution::Throw (ClientErrorException (L"no such id"sv));
}

Collection<String> WSImpl::GetNetworkInterfaces (bool filterRunningOnly) const
{
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

            result += nw;
        }
    }
    return result;
}

NetworkInterface WSImpl::GetNetworkInterface (const String& id) const
{
    GUID compareWithID = ClientErrorException::TreatExceptionsAsClientError ([&]() { return GUID{id}; });

    // @todo quick hack impl
    for (auto i : GetNetworkInterfaces_Recurse (false)) {
        if (i.fGUID == compareWithID) {
            return i;
        }
    }
    Execution::Throw (ClientErrorException (L"no such id"sv));
}

double WSImpl::Operation_Ping (const String& address) const
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"WSImpl::Operation_Ping (%s)", Characters::ToString (address).c_str ())};

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
    auto addrs = DNS::Default ().GetHostAddresses (address);
    if (addrs.size () < 1) {
        Execution::Throw (Execution::Exception (L"no addr"sv));
    }

    NetworkMonitor::Ping::SampleResults t = NetworkMonitor::Ping::Sample (addrs[0], Ping::SampleOptions{kInterSampleTime_, sampleCount}, options);
    if (t.fMedianPingTime) {
        return t.fMedianPingTime->As<double> ();
    }

    return 1;
}

Operations::TraceRouteResults WSImpl::Operation_TraceRoute (const String& address, optional<bool> reverseDNSResults) const
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"WSImpl::Operation_TraceRoute (%s)", Characters::ToString (address).c_str ())};

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
    unsigned int              hopIdx{1};
    for (Traceroute::Hop h : hops) {
        String hopName = [=]() {
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
        Execution::Throw (ClientErrorException (L"samples must be > 0"sv));
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
