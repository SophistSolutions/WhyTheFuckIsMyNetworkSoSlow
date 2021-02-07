/*
* Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
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

    using Characters::String;
    using Containers::Set;

    // @todo add list of SCANNED ports, ToString () method
    struct PortScanResults {
        //Set<uint16_t> fKnownOpenPorts;
        Set<String> fDiscoveredOpenPorts; // in format tcp:80, or icmp:8, or udp:4040

        // later return other inferred results from scan, like Server: headers etc..
    };

    struct ScanOptions {
        enum Style { eQuick,
                     eBasic,
                     eRandomBasicOne,
                     eFull };
        optional<Style> fStyle;
    };

    /**
     *  Don't throw on network errors, just report back a set of successfully scanned (known now open) ports.
     */
    PortScanResults ScanPorts (const IO::Network::InternetAddress& ia, const optional<ScanOptions>& options = nullopt);

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "PortScanner.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_PortScanner_h_*/
