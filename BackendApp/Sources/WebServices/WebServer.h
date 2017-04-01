/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_WebServer_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_WebServer_h_ 1

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

            /*
             *  To test this example:
             *      o   Run the service (under the debugger if you wish)
             *      o   curl  http://localhost:8080/ OR
             *      o   curl  http://localhost:8080/Devices
             *      o   curl  http://localhost:8080/FRED OR      (to see error handling)
             *      o   curl -H "Content-Type: application/json" -X POST -d
             * '{"AppState":"Start"}' http://localhost:8080/SetAppState
             */

            /**
             *  Implementation of WebService calls.
             */
            class WebServer {
            public:
                WebServer (const shared_ptr<IWSAPI>& wsImpl);

            private:
                class Rep_;
                shared_ptr<Rep_> fRep_;
            };
        }
    }
}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "WebServer.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_WebServer_h_*/
