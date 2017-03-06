/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_Model_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_Model_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Containers/Sequence.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/IO/Network/InternetAddress.h"
#include "Stroika/Foundation/Memory/Optional.h"

/**
 *
 */

namespace WhyTheFuckIsMyNetworkSoSlow {
    namespace BackendApp {
        namespace WebServices {
            namespace Model {

                using namespace Stroika::Foundation;
                using Characters::String;
                using Containers::Sequence;
                using Memory::Optional;

                /**
                 */
                struct Device {
                    String name;
                    /**
                     * Bindings
                     */
                    Sequence<String> ipAddresses;
                    String           type;
                    /*
                     *  in CIDR notation.
                     */
                    String          network;
                    Optional<float> signalStrength{};
                    bool            connected{};
                    bool            important{};

                    nonvirtual String ToString () const;

                    static const DataExchange::ObjectVariantMapper kMapper;
                };
            }
        }
    }
}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "Model.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_Model_h_*/
