/*
* Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
*/
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_PortScanner_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_PortScanner_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/IO/Network/InternetAddress.h"

#include "../WebServices/Model.h"

/**
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Discovery {

    using Containers::Set;

    struct PortScanResults {
        Set<uint16_t> fKnownOpenPorts;

        // later return other inferred results from scan, like Server: headers etc..
    };

    /**
     *  Don't throw on network errors, just report back a set of successfully scanned (known now open) ports.
     */
    PortScanResults ScanPorts (const IO::Network::InternetAddress& ia);

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "PortScanner.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_PortScanner_h_*/
