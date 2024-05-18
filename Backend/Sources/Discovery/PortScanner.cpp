/*
* Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include <random>

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Execution/Activity.h"
#include "Stroika/Foundation/IO/Network/ConnectionOrientedStreamSocket.h"
#include "Stroika/Foundation/IO/Network/Port.h"

#include "Stroika/Frameworks/NetworkMonitor/Ping.h"

#include "NetworkInterfaces.h"

#include "PortScanner.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Execution;
using namespace Stroika::Foundation::IO;
using namespace Stroika::Foundation::IO::Network;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Discovery;

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

#define SUPPRESS_NOISY_TRACE_IN_THIS_MODULE_ 1

/*
 ********************************************************************************
 ************************* Discovery::ScanPorts *********************************
 ********************************************************************************
 */
namespace {
    void DoTCPScan_ (const InternetAddress& ia, PortType portNumber, bool quickOpen, PortScanResults* results)
    {
#if SUPPRESS_NOISY_TRACE_IN_THIS_MODULE_
        Debug::TraceContextSuppressor traceSuppress;
#endif
        AssertNotNull (results);
        try {
            ConnectionOrientedStreamSocket::Ptr s = ConnectionOrientedStreamSocket::New (SocketAddress::INET, Socket::STREAM);
            s.Connect (SocketAddress{ia, portNumber}, quickOpen ? 5s : 30s);
            results->fDiscoveredOpenPorts += Characters::Format ("tcp:{}"_f, portNumber);
            results->fIncludesTCP = true;
        }
        catch (...) {
            // Ignored - we typically we get connection failures
        }
    }
    void ICMPPingScan_ (const InternetAddress& ia, PortScanResults* results)
    {
#if SUPPRESS_NOISY_TRACE_IN_THIS_MODULE_
        Debug::TraceContextSuppressor traceSuppress;
#endif
        // also add check for ICMP PING
        Frameworks::NetworkMonitor::Ping::Pinger p{ia};
        try {
            auto r = p.RunOnce (); //incomplete
            results->fDiscoveredOpenPorts.Add (L"icmp:ping"sv);
            results->fIncludedICMP = true;
        }
        catch (const Network::InternetProtocol::ICMP::V4::DestinationUnreachableException&) {
            DbgTrace ("ICMPPingScan_(ia={}): Dest Unreachable: Typically means router blocking"_f, ia);
        }
        catch (...) {
            DbgTrace ("ICMPPingScan_ (addr {}): Ignoring exception: {}"_f, ia, current_exception ());
        }
    }
}

PortScanResults Discovery::ScanPorts (const InternetAddress& ia, const optional<ScanOptions>& options)
{
    PortScanResults results{};
    auto            scanningThisAddress = LazyEvalActivity ([&] () -> String { return Characters::Format ("scanning ports on {}"_f, ia); });
    DeclareActivity da{&scanningThisAddress};

    if (options and options->fStyle == ScanOptions::eQuick) {
        ICMPPingScan_ (ia, &results);
        DoTCPScan_ (ia, WellKnownPorts::TCP::kSSH, true, &results);
        return results;
    }

    constexpr PortType              kKerberos_ = 88; // and many other things
    static const Sequence<PortType> kBasicPorts_{
        WellKnownPorts::TCP::kSSH,
        WellKnownPorts::TCP::kHTTP,
        WellKnownPorts::TCP::kHTTPS,
        WellKnownPorts::TCP::kSMB,
        WellKnownPorts::TCP::kMicrosoftDS,
        WellKnownPorts::TCP::kLPD,
        WellKnownPorts::TCP::kIPP,
        WellKnownPorts::TCP::kRDP,
        WellKnownPorts::TCP::kSIP,
        WellKnownPorts::TCP::kVNC,
        WellKnownPorts::TCP::kHTTPAlt,
        kKerberos_, // experiment and see how much this comes up
    };
    if (options and options->fStyle == ScanOptions::eRandomBasicOne) {
        static mt19937 sRng_{std::random_device{}()};
        size_t         selected = uniform_int_distribution<size_t>{0, kBasicPorts_.size ()}(sRng_);
        if (selected == kBasicPorts_.size ()) {
            ICMPPingScan_ (ia, &results);
        }
        else {
            DoTCPScan_ (ia, kBasicPorts_[selected], true, &results);
        }
        return results;
    }

    ICMPPingScan_ (ia, &results);
    for (auto port : kBasicPorts_) {
        DoTCPScan_ (ia, port, true, &results);
    }

    return results;
}
