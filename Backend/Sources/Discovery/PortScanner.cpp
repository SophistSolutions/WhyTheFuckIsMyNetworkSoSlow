/*
* Copyright(c) Sophist Solutions, Inc. 1990-2020.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include <random>

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Execution/Activity.h"
#include "Stroika/Foundation/IO/Network/ConnectionOrientedStreamSocket.h"

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
    // template so for some ports we can replace implementation and change how handled
    template <int PortNumber>
    void DoTCPScan_ (const InternetAddress& ia, bool quickOpen, PortScanResults* results)
    {
#if SUPPRESS_NOISY_TRACE_IN_THIS_MODULE_
        Debug::TraceContextSuppressor traceSuppress;
#endif
        AssertNotNull (results);
        try {
            ConnectionOrientedStreamSocket::Ptr s = ConnectionOrientedStreamSocket::New (SocketAddress::INET, Socket::STREAM);
            s.Connect (SocketAddress{ia, PortNumber}, quickOpen ? 5s : 30s);
            results->fDiscoveredOpenPorts += Characters::Format (L"tcp:%d", PortNumber);
        }
        catch (...) {
            // Ignored - we typically we get connection failures
        }
    }
    void DoTCPScan_ (int portNumber, const InternetAddress& ia, bool quickOpen, PortScanResults* results)
    {
        switch (portNumber) {
            case 22:
                DoTCPScan_<22> (ia, quickOpen, results);
                break;
            case 80:
                DoTCPScan_<80> (ia, quickOpen, results);
                break;
            case 139:
                DoTCPScan_<139> (ia, quickOpen, results);
                break;
            case 443:
                DoTCPScan_<443> (ia, quickOpen, results);
                break;
            case 445:
                DoTCPScan_<445> (ia, quickOpen, results);
                break;
            case 515:
                DoTCPScan_<515> (ia, quickOpen, results);
                break;
            case 631:
                DoTCPScan_<631> (ia, quickOpen, results);
                break;
            case 3389:
                DoTCPScan_<3389> (ia, quickOpen, results);
                break;
            case 5060:
                DoTCPScan_<5060> (ia, quickOpen, results);
                break;
        }
    }
}

PortScanResults Discovery::ScanPorts (const InternetAddress& ia, const optional<ScanOptions>& options)
{
    PortScanResults results{};

    if (options and options->fStyle == ScanOptions::eQuick) {
        DoTCPScan_<22> (ia, true, &results); // SSH
        return results;
    }
    if (options and options->fStyle == ScanOptions::eRandomBasicOne) {
        Sequence<uint16_t> ports{
            22,   // SSH
            80,   // HTTP
            139,  // SMB
            443,  // HTTPS
            445,  // microsoft-ds
            515,  // Line Printer Daemon (LPD)
            631,  // IPP (internet printing protocol)
            3389, // RDP
            5060, // SIP
        };
        static mt19937 sRng_{std::random_device{}()};
        size_t         selected = uniform_int_distribution<size_t>{0, ports.size () - 1}(sRng_);
        DoTCPScan_ (ports[selected], ia, true, &results);
        return results;
    }

    DoTCPScan_<22> (ia, true, &results);   // SSH
    DoTCPScan_<80> (ia, true, &results);   // HTTP
    DoTCPScan_<139> (ia, true, &results);  // SMB
    DoTCPScan_<443> (ia, true, &results);  // HTTPS
    DoTCPScan_<445> (ia, true, &results);  // microsoft-ds
    DoTCPScan_<515> (ia, true, &results);  // Line Printer Daemon (LPD)
    DoTCPScan_<631> (ia, true, &results);  // IPP (internet printing protocol)
    DoTCPScan_<3389> (ia, true, &results); // RDP
    DoTCPScan_<5060> (ia, true, &results); // SIP (https://isc.sans.edu/diary/Cyber+Security+Awareness+Month+-+Day+20+-+Ports+5060+%26+5061+-+SIP+%28VoIP%29/7405)

    return results;
}
