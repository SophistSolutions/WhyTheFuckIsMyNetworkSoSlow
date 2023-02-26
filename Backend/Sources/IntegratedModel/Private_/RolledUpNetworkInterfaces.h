/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Mgr_Private_RolledUpNetworkInterfaces_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Mgr_Private_RolledUpNetworkInterfaces_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include <optional>

#include "Stroika/Foundation/Containers/Association.h"
#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/Containers/Sequence.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/Time/Realtime.h"

#include "../../WebServices/Model.h"

#include "DBAccess.h"

/**
 */
namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::IntegratedModel::Private_ {

    using namespace std;

    using namespace Stroika::Foundation;
    using Stroika::Foundation::Common::GUID;
    using Stroika::Foundation::Containers::Association;
    using Stroika::Foundation::Containers::Mapping;
    using Stroika::Foundation::Containers::Set;
    using Stroika::Foundation::Execution::Synchronized;
    using Stroika::Foundation::Traversal::Iterable;

    using WebServices::Model::Device;
    using WebServices::Model::NetworkInterface;
    using WebServices::Model::NetworkInterfaceCollection;

    /**
     *  Data structure representing a copy of currently rolled up network interfaces data (copyable).
     * 
     *  Unlike most othter rollup structures, we have no USER_SETTINGS here, so we ALWAYS rollup the same
     *  way. That means there is no ResetUserSettings, and no need to ever INVALIDATE the network interface settings.
     *  (note this refers something computed from historical database data, not something where dynaic data still rolling in).
     * 
     *  \note   \em Thread-Safety   <a href="Thread-Safety.md#C++-Standard-Thread-Safety">C++-Standard-Thread-Safety</a>
     */
    struct RolledUpNetworkInterfaces {
    private:
        RolledUpNetworkInterfaces (const Iterable<Device>& devices, const Iterable<NetworkInterface>& nets2MergeIn);

    public:
        RolledUpNetworkInterfaces (const RolledUpNetworkInterfaces&)            = default;
        RolledUpNetworkInterfaces (RolledUpNetworkInterfaces&&)                 = default;
        RolledUpNetworkInterfaces& operator= (RolledUpNetworkInterfaces&&)      = default;
        RolledUpNetworkInterfaces& operator= (const RolledUpNetworkInterfaces&) = default;

    public:
        /**
         *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
         */
        static RolledUpNetworkInterfaces GetCached (DBAccess::Mgr* dbAccessMgr, Time::DurationSecondsType allowedStaleness = 5.0);

    public:
        /**
         *  This returns the current rolled up network interface objects.
         */
        nonvirtual NetworkInterfaceCollection GetNetworkInterfacess () const;

    public:
        /*
         *  Given a rollup network interface id, return all the matching concrete interfaces.
         *      \req rollupID is contained in this rollup object as a valid rollup id
         */
        nonvirtual Set<GUID> GetConcreteIDsForRollup (const GUID& rollupID) const;

    public:
        /**
         *  Given an aggregated network id, map to the correspoding rollup ID (todo do we need to handle missing case)
         * 
         *  \req is already valid rollup net ID.
         */
        nonvirtual auto MapAggregatedID2ItsRollupID (const GUID& aggregatedNetInterfaceID) const -> GUID;

    public:
        /**
         *  Argument networkInterfaceID must be aggregated network interfaceid, and returns aggrateged deviceids.
         */
        nonvirtual optional<Set<GUID>> GetAttachedToDeviceIDs (const GUID& aggregatedNetworkInterfaceID) const;

    public:
        /**
         *  \brief return the actual (concrete not rollup) NetworkInterface objects associated with the argument ids
         * 
         *      \req each concreteIDs is a valid concrete id contains in this rollup.
         */
        nonvirtual NetworkInterfaceCollection GetConcreteNeworkInterfaces (const Set<GUID>& concreteIDs) const;

    public:
        /**
         */
        nonvirtual NetworkInterface GetRollupNetworkInterface (const GUID& id) const;

    public:
        /**
         */
        nonvirtual NetworkInterfaceCollection GetRollupNetworkInterfaces (const Set<GUID>& rollupIDs) const;

    public:
        /**
         */
        nonvirtual NetworkInterfaceCollection GetRawNetworkInterfaces () const;
        nonvirtual NetworkInterfaceCollection GetRawNetworkInterfaces (const Set<GUID>& rawIDs) const;

    public:
        /**
         *  Given an aggregated (device) network interface id, map to the correspoding rollup ID, if present.
         *  \note return optional<GUID> because this can the caches of devices and networks and network interfaces
         *        can be slightly out of sync
         */
        nonvirtual auto MapAggregatedNetInterfaceID2ItsRollupID (const GUID& netID) const -> optional<GUID>;
        nonvirtual auto MapAggregatedNetInterfaceID2ItsRollupID (const Set<GUID>& netIDs) const -> Set<GUID>;

    private:
        /**
         */
        nonvirtual void MergeIn_ (const optional<GUID>& forDeviceID, const Iterable<NetworkInterface>& netInterfaces2MergeIn);

    private:
        nonvirtual void MergeIn_ (const optional<GUID>& forDeviceID, const NetworkInterface& net2MergeIn);

    private:
        NetworkInterfaceCollection fRawNetworkInterfaces_; // used for RecomputeAll_
        NetworkInterfaceCollection fRolledUpNetworkInterfaces_;
        Mapping<GUID, GUID>        fMapAggregatedNetInterfaceID2RollupID_; // each aggregate net interface id is mapped to at most one rollup id)
        Association<GUID, GUID>    fAssociateAggregatedNetInterface2OwningDeviceID_;

    private:
        static Synchronized<optional<RolledUpNetworkInterfaces>> sRolledUpNetworksInterfaces_;
    };
    inline Synchronized<optional<RolledUpNetworkInterfaces>> RolledUpNetworkInterfaces::sRolledUpNetworksInterfaces_;

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "RolledUpNetworkInterfaces.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Mgr_Private_RolledUpNetworkInterfaces_h_*/
