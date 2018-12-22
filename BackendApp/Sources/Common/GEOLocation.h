/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_GEOLocation_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_GEOLocation_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/IO/Network/InternetAddress.h"

/**
 *
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common {

    using namespace Stroika;
    using namespace Stroika::Foundation;

    using Characters::String;
    using IO::Network::InternetAddress;

    /**
     */
    struct GEOLocationInformation {
        optional<String>              fCountryCode; // Two-letter country code ISO 3166-1 alpha-2
        optional<String>              fCity;
        optional<String>              fRegionCode;            // Region/state short code (FIPS or ISO)
        optional<String>              fPostalCode;            // zipcode
        optional<tuple<float, float>> fLattitudeAndLongitude; // Latitude/longitude

        nonvirtual String ToString () const;
    };

    optional<GEOLocationInformation> LookupGEOLocation (InternetAddress ia);

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "GEOLocation.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_GEOLocation_h_*/
