/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_InternetServiceProvider_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_InternetServiceProvider_h_ 1

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
    struct InternetServiceProvider {
        optional<String> name;

        nonvirtual String ToString () const;
    };

    optional<InternetServiceProvider> LookupInternetServiceProvider (InternetAddress ia);

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "InternetServiceProvider.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_InternetServiceProvider_h_*/
