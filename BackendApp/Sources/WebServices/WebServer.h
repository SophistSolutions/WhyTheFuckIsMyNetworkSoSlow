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

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices {

    using namespace Model;

    using Containers::Collection;

    /**
             *  WebServer handles basic HTTP web server technology, and translates to ISAPI interface web-service implementation methods.
             */
    class WebServer {
    public:
        WebServer (const shared_ptr<IWSAPI>& wsImpl);

    private:
        class Rep_;
        shared_ptr<Rep_> fRep_;
    };

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "WebServer.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_WebServer_h_*/
