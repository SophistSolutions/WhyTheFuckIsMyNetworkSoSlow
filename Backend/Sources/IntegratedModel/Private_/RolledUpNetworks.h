/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Private_RolledUpNetworks_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Private_RolledUpNetworks_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include <optional>

#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/Containers/Sequence.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/Time/Realtime.h"

#include "../../WebServices/Model.h"

#include "DBAccess.h"
#include "RolledUpNetworkInterfaces.h"

/**
 */
namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::IntegratedModel::Private_ {

    using namespace std;

    using namespace Stroika::Foundation;
    using Stroika::Foundation::Common::GUID;
    using Stroika::Foundation::Containers::Mapping;
    using Stroika::Foundation::Containers::Set;
    using Stroika::Foundation::Execution::Synchronized;
    using Stroika::Foundation::Traversal::Iterable;

    using WebServices::Model::Network;
    using WebServices::Model::NetworkCollection;

    /**
     *  Data structure representing a copy of currently rolled up networks data (copyable).
     * 
     *  This data MAYBE invalidated due to changes in Network::UserOverridesType settings (even historical database data
     *  cuz changes in rollup rules could cause how the historical data was rolled up to change).
     *
     *  \note   \em Thread-Safety   <a href="Thread-Safety.md#C++-Standard-Thread-Safety">C++-Standard-Thread-Safety</a>
     */
    struct RolledUpNetworks {
    public:
        RolledUpNetworks (DBAccess::Mgr* dbAccessMgr, const Iterable<Network>& nets2MergeIn,
                          const Mapping<GUID, Network::UserOverridesType>& userOverrides, const RolledUpNetworkInterfaces& useNetworkInterfaceRollups);
        RolledUpNetworks (const RolledUpNetworks&)            = default;
        RolledUpNetworks (RolledUpNetworks&&)                 = default;
        RolledUpNetworks& operator= (RolledUpNetworks&&)      = default;
        RolledUpNetworks& operator= (const RolledUpNetworks&) = default;

    public:
        /**
         * INVALIDATE IF 'UserSettings' change, which might cause different rollups (this includes fingerprint to guid map)
         *
         *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
         */
        static RolledUpNetworks GetCached (DBAccess::Mgr* dbAccessMgr, Time::DurationSeconds allowedStaleness = 5.0s);

    public:
        /**
         *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
         */
        static void InvalidateCache (DBAccess::Mgr* dbAccessMgr);

    public:
        /**
         *  This returns the current rolled up network objects.
         */
        nonvirtual NetworkCollection GetNetworks () const;

    public:
        nonvirtual void ResetUserOverrides (DBAccess::Mgr* dbAccessMgr, const Mapping<GUID, Network::UserOverridesType>& userOverrides);

    public:
        /**
         *  Given an aggregated network id, map to the correspoding rollup ID (todo do we need to handle missing case)
         * 
         *  \req is already valid rollup net ID.
         */
        nonvirtual auto MapAggregatedID2ItsRollupID (const GUID& netID) const -> GUID;

    public:
        /**
         *  Modify this set of rolled up networks with this net2MergeIn. If we've seen this network before (by ID)
         *  possibly remove it from some rollup, and possibly even remove that (if now empty) rollup.
         *
         *  But typically this will just update the record for an existing rollup.
         */
        nonvirtual void MergeIn (DBAccess::Mgr* dbAccessMgr, const Iterable<Network>& nets2MergeIn);

    private:
        enum class PassFailType_ {
            ePass,
            eFail
        };
        // if fails simple merge, returns false, so must call recomputeall
        PassFailType_ MergeIn_ (DBAccess::Mgr* dbAccessMgr, const Network& net2MergeIn);
        // Find the appropriate network to merge net2MergeIn into from our existing networks (using whatever keys we have to make this often quicker)
        // This can be slow for the case of a network change. And it can refer to a different rollup network than it used to.
        // second part of tuple returned is 'revalidateAll' - force recompute of whole rollup

        // @todo NOTE - a net COULD be directed to rollup into one network due to fingerprint and another due to matching gateway hardware ID
        // RESOLVE this by agreeing that hardwareGatewayID takes PRECEDENCE.
        //
        // And consider it a DATA ERROR (need code to assure cannot happen) - for the same hardare ID to appear in two rollup networks match rules
        // or the same (any type of rollup rule) to appear in differnt high level user settings)
        tuple<optional<Network>, PassFailType_> ShouldRollupInto_ (const Network& net2MergeIn, const Network::FingerprintType& net2MergeInFingerprint);
        bool ShouldRollupInto_CheckIsCompatibleWithTarget_ (const Network& net2MergeIn, const Network::FingerprintType& net2MergeInFingerprint,
                                                            const Network& targetRollup);
        void AddUpdateIn_ (const Network& addNet2MergeFromThisRollup, const Network& net2MergeIn, const Network::FingerprintType& net2MergeInFingerprint);
        void AddNewIn_ (DBAccess::Mgr* dbAccessMgr, const Network& net2MergeIn, const Network::FingerprintType& net2MergeInFingerprint);
        void RecomputeAll_ (DBAccess::Mgr* dbAccessMgr);

    private:
        RolledUpNetworkInterfaces               fUseNetworkInterfaceRollups;
        NetworkCollection                       fRawNetworks_; // used for RecomputeAll_
        NetworkCollection                       fStarterRollups_;
        NetworkCollection                       fRolledUpNetworks_;
        Mapping<GUID, GUID>                     fMapAggregatedNetID2RollupID_; // each aggregate netid is mapped to at most one rollup id)
        Mapping<Network::FingerprintType, GUID> fMapFingerprint2RollupID;      // each fingerprint can map to at most one rollup...
    private:
        static Synchronized<optional<RolledUpNetworks>> sRolledUpNetworks_;
    };
    inline Synchronized<optional<RolledUpNetworks>> RolledUpNetworks::sRolledUpNetworks_;

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "RolledUpNetworks.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Private_RolledUpNetworks_h_*/
