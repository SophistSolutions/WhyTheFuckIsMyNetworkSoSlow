/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_WSImpl_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_WSImpl_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/Containers/Sequence.h"

#include "IWSAPI.h"
/**
*
*/

namespace WhyTheFuckIsMyNetworkSoSlow {
    namespace BackendApp {
        namespace WebServices {
            using namespace Model;

            using Containers::Collection;

            /**
             *  Implementation of WebService calls.
             */
            class WSImpl : public IWSAPI {
            public:
                virtual Collection<Device> GetDevices () const override;
            };
        }
    }
}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "WSImpl.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_WSImpl_h_*/
