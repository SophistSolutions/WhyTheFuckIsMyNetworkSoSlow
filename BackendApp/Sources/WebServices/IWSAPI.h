/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_IWSAPI_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_IWSAPI_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/Containers/Sequence.h"

#include "Model.h"

/**
 *
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices {
    using namespace Model;

    using Containers::Collection;
    using Containers::Sequence;

    /**
     */
    class IWSAPI {
    protected:
        IWSAPI () = default;

    public:
        IWSAPI (const IWSAPI&) = delete;
        virtual ~IWSAPI ()     = default;

    public:
        /**
         *  curl  http://localhost:8080/devices
         */
        virtual Collection<String> GetDevices () const                = 0;
        virtual Collection<Device> GetDevices_Recurse () const        = 0;
        virtual Device             GetDevice (const String& id) const = 0;

    public:
        /**
         *  curl  http://localhost:8080/networks
         */
        virtual Sequence<String>  GetNetworks () const                = 0;
        virtual Sequence<Network> GetNetworks_Recurse () const        = 0;
        virtual Network           GetNetwork (const String& id) const = 0;

    public:
        /**
         *  curl  http://localhost:8080/network-interfaces
         */
        virtual Collection<String>           GetNetworkInterfaces (bool filterRunningOnly) const         = 0;
        virtual Collection<NetworkInterface> GetNetworkInterfaces_Recurse (bool filterRunningOnly) const = 0;
        virtual NetworkInterface             GetNetworkInterface (const String& id) const                = 0;

    public:
        /**
         *  curl  http://localhost:8080/operations/ping
         *      return time to ping, or throw on failure
         */
        virtual double Operation_Ping (const String& address) const = 0;

    public:
        /**
         *  curl  http://localhost:8080/operations/ping
         *      return time to ping, or throw on failure
         */
        virtual Operations::TraceRouteResults Operation_TraceRoute (const String& address, optional<bool> reverseDNSResults = {}) const = 0;

    public:
        /**
         *  curl  http://localhost:8080/operations/dns/calculate-negative-lookup-time
         */
        virtual Time::Duration Operation_DNS_CalculateNegativeLookupTime (optional<unsigned int> samples = {}) const = 0;
    };

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "IWSAPI.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_IWSAPI_h_*/
