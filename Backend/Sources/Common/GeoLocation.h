/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_GeoLocation_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_GeoLocation_h_ 1

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
        optional<String>              fRegionCode;           // Region/state short code (FIPS or ISO)
        optional<String>              fPostalCode;           // zipcode
        optional<tuple<float, float>> fLatitudeAndLongitude; // Latitude/longitude

        nonvirtual String ToString () const;

#if __cpp_impl_three_way_comparison < 201711
        bool operator== (const GEOLocationInformation& rhs) const
        {
            if (fCountryCode != rhs.fCountryCode) {
                return false;
            }
            if (fCity != rhs.fCity) {
                return false;
            }
            if (fRegionCode != rhs.fRegionCode) {
                return false;
            }
            if (fPostalCode != rhs.fPostalCode) {
                return false;
            }
            if (fLatitudeAndLongitude != rhs.fLatitudeAndLongitude) {
                return false;
            }
            return true;
        }
        bool operator!= (const GEOLocationInformation& rhs) const { return not(*this == rhs); }
#else
        auto operator<=> (const GEOLocationInformation&) const = default;
#endif
    };

    optional<GEOLocationInformation> LookupGEOLocation (InternetAddress ia);

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "GeoLocation.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_GeoLocation_h_*/
