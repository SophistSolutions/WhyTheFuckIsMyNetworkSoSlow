/*
* Copyright(c) Sophist Solutions, Inc. 1990-2020.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

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

/*
 ********************************************************************************
 ************************* Discovery::ScanPorts *********************************
 ********************************************************************************
 */
namespace {
    // template so for some ports we can replace implementation and change how handled
    template <int PortNumber>
    void DoTCPScan_ (const InternetAddress& ia, PortScanResults* results)
    {
        AssertNotNull (results);
        try {
            ConnectionOrientedStreamSocket::Ptr s = ConnectionOrientedStreamSocket::New (SocketAddress::INET, Socket::STREAM);
            s.Connect (SocketAddress{ia, PortNumber});
            results->fKnownOpenPorts += PortNumber;
        }
        catch (...) {
            // Ignored - we typically we get connection failures
        }
    }
}

PortScanResults Discovery::ScanPorts (const InternetAddress& ia)
{
    PortScanResults results{};

    DoTCPScan_<22> (ia, &results);   // SSH
    DoTCPScan_<80> (ia, &results);   // HTTP
    DoTCPScan_<443> (ia, &results);  // HTTPS
    DoTCPScan_<515> (ia, &results);  // Line Printer Daemon (LPD)
    DoTCPScan_<631> (ia, &results);  // IPP (internet printing protocol)
    DoTCPScan_<3389> (ia, &results); // RDP

    return results;
}
