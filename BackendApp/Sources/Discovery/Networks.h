/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_Networks_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_Networks_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/Containers/Sequence.h"
#include "Stroika/Foundation/IO/Network/CIDR.h"
#include "Stroika/Foundation/IO/Network/InternetAddress.h"

/**
 *
 */

namespace WhyTheFuckIsMyNetworkSoSlow {
    namespace BackendApp {
        namespace Discovery {

            using namespace Stroika;
            using namespace Stroika::Foundation;

            using Characters::String;
            using Containers::Collection;
            using IO::Network::CIDR;
            using IO::Network::InternetAddress;

            struct Network {
                CIDR             fIPAddress;
                optional<String> fSSID;

                bool operator== (const Network& rhs) const
                {
                    return fIPAddress == rhs.fIPAddress and fSSID == rhs.fSSID;
                }

                nonvirtual String ToString () const;
            };

            Collection<Network> CollectActiveNetworks ();
        }
    }
}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "Networks.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_Networks_h_*/
