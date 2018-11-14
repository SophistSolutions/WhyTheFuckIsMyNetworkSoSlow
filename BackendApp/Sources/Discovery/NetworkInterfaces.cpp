/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include <algorithm>
#include <vector>

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/LinkMonitor.h"

#include "NetworkInterfaces.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::IO::Network;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Discovery;

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

/*
 ********************************************************************************
 ************************ Discovery::NetworkInterface ***************************
 ********************************************************************************
 */
NetworkInterface::NetworkInterface (const IO::Network::Interface& src)
    : Interface (src)
{
}

/*
 ********************************************************************************
 ***************** Discovery::CollectActiveNetworkInterfaces ********************
 ********************************************************************************
 */
Collection<NetworkInterface> Discovery::CollectActiveNetworkInterfaces ()
{
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    Debug::TraceContextBumper ctx{L"Discovery::CollectActiveNetworkInterfaces"};
#endif
    vector<NetworkInterface> results;
    for (Interface i : IO::Network::GetInterfaces ()) {
        if (i.fType != Interface::Type::eLoopback and i.fStatus and i.fStatus->Contains (Interface::Status::eRunning)) {
            NetworkInterface ni{i};
            results.push_back (ni);
        }
    }
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace (L"returns: %s", Characters::ToString (results).c_str ());
#endif
    return results;
}
