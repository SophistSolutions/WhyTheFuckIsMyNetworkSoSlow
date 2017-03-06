/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/LinkMonitor.h"

#include "Networks.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::IO::Network;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Discovery;

// Comment this in to turn on aggressive noisy DbgTrace in this module
#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

/*
 ********************************************************************************
 ******************************** Discovery::CIDR *******************************
 ********************************************************************************
 */
String Discovery::CIDR::ToString () const
{
    StringBuilder sb;
    sb += L"{";
    sb += L"BaseAddress: " + Characters::ToString (fBaseAddress) + L"/" + Characters::ToString (fSignificantBits);
    sb += L"}";
    return sb.str ();
}

/*
 ********************************************************************************
 ***************************** Discovery::Network *******************************
 ********************************************************************************
 */
String Discovery::Network::ToString () const
{
    StringBuilder sb;
    sb += L"{";
    sb += L"IP-Address: " + Characters::ToString (fIPAddress) + L", ";
    if (fSSID) {
        sb += L"SSID: " + Characters::ToString (fSSID) + L", ";
    }
    sb += L"}";
    return sb.str ();
}

/*
 ********************************************************************************
 ******************** Discovery::CollectActiveNetworks **************************
 ********************************************************************************
 */
Collection<Network> Discovery::CollectActiveNetworks ()
{
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    Debug::TraceContextBumper ctx{L"Discovery::CollectActiveNetworks"};
#endif
    Collection<Network> results;
    for (IO::Network::Interface i : IO::Network::GetInterfaces ()) {
        if (i.fType != Interface::Type::eLoopback and i.fStatus and i.fStatus->Contains (Interface::Status::eRunning)) {
            // prefer the v4 IP addr if any, and otherwise show v6
            if (not i.fBindings.empty ()) {
                InternetAddress useAddr = i.fBindings.Nth (0);
                i.fBindings.Apply ([&](InternetAddress i) { if (i.GetAddressFamily () == InternetAddress::AddressFamily::V4) { useAddr = i; } });
                //tmphack - just say CIDR 24  - til we can fix @todo - FIX
                results += Network{CIDR{useAddr, 24}, i.fFriendlyName};
            }
        }
    }
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace (L"returns: %s", Characters::ToString (results).c_str ());
#endif
    return results;
}
