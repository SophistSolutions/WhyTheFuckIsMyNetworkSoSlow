/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_GEOLocAndISPLookup_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_GEOLocAndISPLookup_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/IO/Network/InternetAddress.h"

#include "GeoLocation.h"
#include "InternetServiceProvider.h"

/**
 *
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common {

    using namespace Stroika;
    using namespace Stroika::Foundation;

    using Characters::String;
    using IO::Network::InternetAddress;

    /**
     *  \note currently ignoring 'isMobile', 'isTORuser', 'isProxyUser' - and maybe a few more. Not sure we can get these
     *        reliably, nor that we need them.
     */
    optional<tuple<GEOLocationInformation, InternetServiceProvider>> GEOLocAndISPLookup (InternetAddress ia);

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "GeoLocAndISPLookup.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_GEOLocAndISPLookup_h_*/
