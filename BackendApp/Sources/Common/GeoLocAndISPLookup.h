/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_GEOLocAndISPLookup_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_GEOLocAndISPLookup_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/IO/Network/InternetAddress.h"

#include "GEOLocation.h"
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
     */
    optional<tuple<GEOLocationInformation, InternetServiceProvider>> GEOLocAndISPLookup (InternetAddress ia);

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "GEOLocAndISPLookup.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_GEOLocAndISPLookup_h_*/
