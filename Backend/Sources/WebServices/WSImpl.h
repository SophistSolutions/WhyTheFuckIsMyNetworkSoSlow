/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
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
    class WSImpl final : public IWSAPI {
    public:
        WSImpl ();
        virtual ~WSImpl () override = default;

    public:
        virtual About                                                          GetAbout () const override;
        virtual tuple<Memory::BLOB, optional<DataExchange::InternetMediaType>> GetBLOB (const GUID& guid) const override;
        virtual Sequence<String> GetDevices (const optional<Set<GUID>>& ids, const optional<DeviceSortParamters>& sort) const override;
        virtual Sequence<Device> GetDevices_Recurse (const optional<Set<GUID>>& ids, const optional<DeviceSortParamters>& sort) const override;
        virtual tuple<Device, Duration>      GetDevice (const String& id) const override;
        virtual void                         PatchDevice (const String& id, const JSONPATCH::OperationItemsType& patchDoc) const override;
        virtual Sequence<String>             GetNetworks (const optional<Set<GUID>>& ids) const override;
        virtual Sequence<Network>            GetNetworks_Recurse (const optional<Set<GUID>>& ids) const override;
        virtual tuple<Network, Duration>     GetNetwork (const String& id) const override;
        virtual void                         PatchNetwork (const String& id, const JSONPATCH::OperationItemsType& patchDoc) const override;
        virtual Collection<String>           GetNetworkInterfaces () const override;
        virtual Collection<NetworkInterface> GetNetworkInterfaces_Recurse () const override;
        virtual tuple<NetworkInterface, Duration> GetNetworkInterface (const String& id) const override;
        virtual double                            Operation_Ping (const String& address) const override;
        virtual Operations::TraceRouteResults Operation_TraceRoute (const String& address, optional<bool> reverseDNSResults) const override;
        virtual Duration                      Operation_DNS_CalculateNegativeLookupTime (optional<unsigned int> samples) const override;
        virtual Operations::DNSLookupResults  Operation_DNS_Lookup (const String& name) const override;
        virtual double                        Operation_DNS_CalculateScore () const override;
        virtual DataExchange::VariantValue    Operation_Scan_FullRescan (const String& deviceID) const override;
        virtual DataExchange::VariantValue    Operation_Scan_Scan (const String& addr) const override;

    private:
        struct Rep_;

    private:
        shared_ptr<Rep_> fRep_;
    };

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "WSImpl.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_WSImpl_h_*/
