/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"

#include "Networks.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::IO::Network;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Discovery;

String Discovery::CIDR::ToString () const
{
    StringBuilder sb;
    sb += L"BaseAddress: " + Characters::ToString (fBaseAddress) + L"/" + Characters::ToString (fSignificantBits);
    return sb.str ();
}

String Discovery::Network::ToString () const
{
    StringBuilder sb;
    sb += L"IP-Address: " + Characters::ToString (fIPAddress) + L", ";
    if (fSSID) {
        sb += L"SSID: " + Characters::ToString (fSSID) + L", ";
    }
    return sb.str ();
}

Collection<Network> Discovery::CollectActiveNetworks ()
{
    Collection<Network> results;
    //@todo tmphack
    results += Network{CIDR{InternetAddress (L"192.168.244.0"), 24}, String{L"Sophists Church St. WLAN"}};
    return results;
}
