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

#include "NetworkInterfaces.h"

#include "PortScanner.h"

using namespace std;

using namespace Stroika::Foundation;
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
            results->fDiscoveredOpenPorts += Characters::Format (L"tcp:%d", portNumber);
        }
        catch (...) {
            // Ignored - we typically we get connection failures
        }
    }
}

PortScanResults Discovery::ScanPorts (const InternetAddress& ia, const optional<ScanOptions>& options)
{
    PortScanResults results{};

    if (options and options->fStyle == ScanOptions::eQuick) {
        DoTCPScan_ (ia, WellKnownPorts::TCP::kSSH, true, &results);
        return results;
    }

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
    };
    if (options and options->fStyle == ScanOptions::eRandomBasicOne) {
        static mt19937 sRng_{std::random_device{}()};
        size_t         selected = uniform_int_distribution<size_t>{0, kBasicPorts_.size () - 1}(sRng_);
        DoTCPScan_ (ia, kBasicPorts_[selected], true, &results);
        return results;
    }

    for (auto port : kBasicPorts_) {
        DoTCPScan_ (ia, port, true, &results);
    }

    return results;
}
