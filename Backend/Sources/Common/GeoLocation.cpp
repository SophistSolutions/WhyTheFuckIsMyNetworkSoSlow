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
    sb << "{";
    sb << "Country-Code: " << fCountryCode << ", "sv;
    sb << "City: " << fCity << ", "sv;
    sb << "Region-Code: " << fRegionCode << ", "sv;
    sb << "Postal-Code: " << fPostalCode << ", "sv;
    sb << "Latitude-And-Longitude: "sv << fLatitudeAndLongitude << ", "sv;
    sb << "}"sv;
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
