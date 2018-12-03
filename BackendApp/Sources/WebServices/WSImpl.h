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

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices {

    using Containers::Collection;

    /**
     *  Implementation of WebService calls.
     */
    class WSImpl : public IWSAPI {
    public:
        virtual Collection<Device>           GetDevices () const override;
        virtual Sequence<Network>            GetNetworks () const override;
        virtual Collection<String>           GetNetworkInterfaces (bool filterRunningOnly) const override;
        virtual Collection<NetworkInterface> GetNetworkInterfaces_Recurse (bool filterRunningOnly) const override;
        virtual NetworkInterface             GetNetworkInterface (const String& id) const override;
    };

    void TmpHackAssureStartedMonitoring ();

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "WSImpl.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_WSImpl_h_*/
