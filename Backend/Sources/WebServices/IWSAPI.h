/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2020.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_IWSAPI_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_IWSAPI_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Common/GUID.h"
#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/Containers/Sequence.h"
#include "Stroika/Foundation/DataExchange/InternetMediaType.h"

#include "Model.h"

/**
 *
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices {
    using namespace Model;

    using Containers::Collection;
    using Containers::Sequence;
    using Stroika::Foundation::Common::GUID;

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
         *  curl  http://localhost:8080/about
         */
        virtual About GetAbout () const = 0;

    public:
        /**
         *  curl  http://localhost:8080/blob/{guid}
         */
        virtual tuple<Memory::BLOB, DataExchange::InternetMediaType> GetBLOB (const GUID& guid) const = 0;

    public:
        /**
         *  curl  http://localhost:8080/devices
         */
        virtual Sequence<String> GetDevices (const optional<DeviceSortParamters>& sort = {}) const         = 0;
        virtual Sequence<Device> GetDevices_Recurse (const optional<DeviceSortParamters>& sort = {}) const = 0;
        virtual Device           GetDevice (const String& id) const                                        = 0;

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

    public:
        /**
         *  curl  http://localhost:8080/operations/dns/lookup
         */
        virtual Operations::DNSLookupResults Operation_DNS_Lookup (const String& name) const = 0;

    public:
        /**
         *  curl  http://localhost:8080/operations/dns/calculate-score
         */
        virtual double Operation_DNS_CalculateScore () const = 0;

    public:
        /**
         *  curl  http://localhost:8080/operations/scan/FullRescan?device=ID
         */
        virtual DataExchange::VariantValue Operation_Scan_FullRescan (const String& deviceID) const = 0;

    public:
        /**
         *  curl  http://localhost:8080/operations/scan/Scan?addr=addr
         *  note - addr may be DNS name or ip address.
         */
        virtual DataExchange::VariantValue Operation_Scan_Scan (const String& addr) const = 0;
    };

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "IWSAPI.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_IWSAPI_h_*/
