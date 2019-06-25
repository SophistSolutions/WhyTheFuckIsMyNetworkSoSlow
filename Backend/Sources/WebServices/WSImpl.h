/*
* Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
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
    using Containers::Sequence;

    /**
     *  Implementation of WebService calls.
     */
    class WSImpl : public IWSAPI {
    public:
        virtual About                                                GetAbout () const override;
        virtual tuple<Memory::BLOB, DataExchange::InternetMediaType> GetBLOB (const GUID& guid) const override;
        virtual Sequence<String>                                     GetDevices (const optional<DeviceSortParamters>& sort) const override;
        virtual Sequence<Device>                                     GetDevices_Recurse (const optional<DeviceSortParamters>& sort) const override;
        virtual Device                                               GetDevice (const String& id) const override;
        virtual Sequence<String>                                     GetNetworks () const override;
        virtual Sequence<Network>                                    GetNetworks_Recurse () const override;
        virtual Network                                              GetNetwork (const String& id) const override;
        virtual Collection<String>                                   GetNetworkInterfaces (bool filterRunningOnly) const override;
        virtual Collection<NetworkInterface>                         GetNetworkInterfaces_Recurse (bool filterRunningOnly) const override;
        virtual NetworkInterface                                     GetNetworkInterface (const String& id) const override;
        virtual double                                               Operation_Ping (const String& address) const override;
        virtual Operations::TraceRouteResults                        Operation_TraceRoute (const String& address, optional<bool> reverseDNSResults) const override;
        virtual Time::Duration                                       Operation_DNS_CalculateNegativeLookupTime (optional<unsigned int> samples) const override;
        virtual Operations::DNSLookupResults                         Operation_DNS_Lookup (const String& name) const override;
        virtual double                                               Operation_DNS_CalculateScore () const override;
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
