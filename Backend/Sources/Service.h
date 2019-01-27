/*
* Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
*/
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Service_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Service_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Frameworks/Service/Main.h"

/**
*/

namespace WhyTheFuckIsMyNetworkSoSlow {
    namespace BackendApp {

        using namespace Stroika::Foundation;
        using namespace Stroika::Frameworks::Service;

        /**
         *  This class contains the 'main service loop' - which is how a service is started and shut down (and modules tied together).
         */
        struct WTFAppServiceRep : Main::IApplicationRep {
            WTFAppServiceRep ()          = default;
            virtual ~WTFAppServiceRep () = default;

        public:
            virtual void MainLoop (const std::function<void()>& startedCB) override;

        public:
            virtual Main::ServiceDescription GetServiceDescription () const override;
        };
    }
}

/*
********************************************************************************
***************************** Implementation Details ***************************
********************************************************************************
*/
#include "Service.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Service_h_*/
