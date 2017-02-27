/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_Model_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_Model_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
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
                using Memory::Optional;

                /**
                 */
                struct Device {
                    String name;
                    String ipAddress;
                    String ipv4;
                    String ipv6;
                    String type;
                    String image;
                    String network;
                    String networkMask;
                    float  signalStrength{};
                    bool   connected{};
                    bool   important{};

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
