/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_IWSAPI_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_IWSAPI_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Common/GUID.h"
#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/Containers/Sequence.h"
#include "Stroika/Foundation/DataExchange/InternetMediaType.h"

#include "JSONPATCH.h"
#include "Model.h"

/**
 *
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices {
    using namespace Model;

    using Containers::Collection;
    using Containers::Sequence;
    using Stroika::Foundation::Common::GUID;
    using Traversal::Iterable;

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
         *  curl  http://localhost/api/v1/about
         */
        virtual About GetAbout () const = 0;

    public:
        /**
         *  curl  http://localhost/api/v1/blob/{guid}
         */
        virtual tuple<Memory::BLOB, optional<DataExchange::InternetMediaType>> GetBLOB (const GUID& guid) const = 0;

    public:
        /**
         *  curl  http://localhost/api/v1/devices
         * 
         *      GetDevice(id) API second value in tuple is TTL
         */
        virtual Sequence<String> GetDevices (const optional<Set<GUID>>& ids = nullopt, const optional<DeviceSortParamters>& sort = {}) const = 0;
        virtual Sequence<Device> GetDevices_Recurse (const optional<Set<GUID>>& ids = nullopt, const optional<DeviceSortParamters>& sort = {}) const = 0;
        virtual tuple<Device, Duration> GetDevice (const String& id) const                                                  = 0;
        virtual void                    PatchDevice (const String& id, const JSONPATCH::OperationItemsType& patchDoc) const = 0;

    public:
        /**
         *  curl  http://localhost/api/v1/networks
         * 
         *      GetNetwork(id) API second value in tuple is TTL
         */
        virtual Sequence<String>         GetNetworks (const optional<Set<GUID>>& ids = nullopt) const                         = 0;
        virtual Sequence<Network>        GetNetworks_Recurse (const optional<Set<GUID>>& ids = nullopt) const                 = 0;
        virtual tuple<Network, Duration> GetNetwork (const String& id) const                                                  = 0;
        virtual void                     PatchNetwork (const String& id, const JSONPATCH::OperationItemsType& patchDoc) const = 0;

    public:
        /**
         *  curl  http://localhost/api/v1/network-interfaces
         * 
         *      GetNetworkInterface(id) API second value in tuple is TTL
         */
        virtual Collection<String>                GetNetworkInterfaces () const                = 0;
        virtual Collection<NetworkInterface>      GetNetworkInterfaces_Recurse () const        = 0;
        virtual tuple<NetworkInterface, Duration> GetNetworkInterface (const String& id) const = 0;

    public:
        /**
         *  curl  http://localhost/api/v1/operations/ping
         *      return time to ping, or throw on failure
         */
        virtual double Operation_Ping (const String& address) const = 0;

    public:
        /**
         *  curl  http://localhost/api/v1/operations/ping
         *      return time to ping, or throw on failure
         */
        virtual Operations::TraceRouteResults Operation_TraceRoute (const String& address, optional<bool> reverseDNSResults = {}) const = 0;

    public:
        /**
         *  curl  http://localhost/api/v1/operations/dns/calculate-negative-lookup-time
         */
        virtual Time::Duration Operation_DNS_CalculateNegativeLookupTime (optional<unsigned int> samples = {}) const = 0;

    public:
        /**
         *  curl  http://localhost/api/v1/operations/dns/lookup
         */
        virtual Operations::DNSLookupResults Operation_DNS_Lookup (const String& name) const = 0;

    public:
        /**
         *  curl  http://localhost/api/v1/operations/dns/calculate-score
         */
        virtual double Operation_DNS_CalculateScore () const = 0;

    public:
        /**
         *  curl  http://localhost/api/v1/operations/scan/FullRescan?device=ID
         */
        virtual DataExchange::VariantValue Operation_Scan_FullRescan (const String& deviceID) const = 0;

    public:
        /**
         *  curl  http://localhost/api/v1/operations/scan/Scan?addr=addr
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
