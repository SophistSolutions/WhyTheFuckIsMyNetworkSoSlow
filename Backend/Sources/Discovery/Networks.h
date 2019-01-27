/*
* Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
*/
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_Networks_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_Networks_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/Containers/Sequence.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/IO/Network/CIDR.h"
#include "Stroika/Foundation/IO/Network/InternetAddress.h"

#include "../Common/GeoLocation.h"
#include "../Common/InternetServiceProvider.h"

/**
 *
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Discovery {

    using namespace Stroika;
    using namespace Stroika::Foundation;

    using Characters::String;
    using Containers::Collection;
    using Containers::Sequence;
    using Containers::Set;
    using IO::Network::CIDR;
    using IO::Network::InternetAddress;
    using Stroika::Foundation::Common::GUID;

    using namespace BackendApp::Common;

    struct Network {
        // @todo if you have an IPv4 and IPv6 network they maybe merged into a single logical network
        Set<CIDR>        fNetworkAddresses;
        GUID             fGUID;
        optional<String> fFriendlyName;

        Set<GUID> fAttachedNetworkInterfaces;

        Sequence<InternetAddress> fGateways;
        Sequence<InternetAddress> fDNSServers;

        // whatsmyip
        optional<Sequence<InternetAddress>> fExternalAddresses;

        optional<GEOLocationInformation> fGEOLocInfo;

        optional<InternetServiceProvider> fISP;

        nonvirtual String ToString () const;
    };

    Sequence<Network> CollectActiveNetworks ();
}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "Networks.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_Networks_h_*/
