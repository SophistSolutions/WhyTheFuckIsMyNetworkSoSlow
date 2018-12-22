/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"

#include "GEOLocAndISPLookup.h"

#include "InternetServiceProvider.h"

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
 *************** BackendApp::Common::InternetServiceProvider *********************
 ********************************************************************************
 */
String BackendApp::Common::InternetServiceProvider::ToString () const
{
    StringBuilder sb;
    sb += L"{";
    sb += L"name: " + Characters::ToString (name) + L", ";
    sb += L"}";
    return sb.str ();
}

/*
 ********************************************************************************
 ******************** BackendApp::Common::LookupInternetServiceProvider *********************
 ********************************************************************************
 */
optional<InternetServiceProvider> BackendApp::Common::LookupInternetServiceProvider (InternetAddress ia)
{
    if (auto o = GEOLocAndISPLookup (ia)) {
        return get<1> (*o);
    }
    return nullopt;
}
