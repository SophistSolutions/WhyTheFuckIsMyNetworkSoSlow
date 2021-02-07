/*
* Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"

#include "GeoLocAndISPLookup.h"

#include "GeoLocation.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common;

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

/*
 ********************************************************************************
 *************** BackendApp::Common::GEOLocationInformation *********************
 ********************************************************************************
 */
String BackendApp::Common::GEOLocationInformation::ToString () const
{
    StringBuilder sb;
    sb += L"{";
    sb += L"Country-Code: " + Characters::ToString (fCountryCode) + L", ";
    sb += L"City: " + Characters::ToString (fCity) + L", ";
    sb += L"Region-Code: " + Characters::ToString (fRegionCode) + L", ";
    sb += L"Postal-Code: " + Characters::ToString (fPostalCode) + L", ";
    sb += L"Latitude-And-Longitude: " + Characters::ToString (fLatitudeAndLongitude) + L", ";
    sb += L"}";
    return sb.str ();
}

/*
 ********************************************************************************
 ******************** BackendApp::Common::LookupGEOLocation *********************
 ********************************************************************************
 */
optional<GEOLocationInformation> BackendApp::Common::LookupGEOLocation (InternetAddress ia)
{
    if (auto o = GEOLocAndISPLookup (ia)) {
        return get<0> (*o);
    }
    return nullopt;
}
