/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Mgr_Private_RolledUpDevices_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Mgr_Private_RolledUpDevices_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include <optional>

#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/Containers/Sequence.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/Time/Realtime.h"

#include "../../WebServices/Model.h"

#include "DBAccess.h"
#include "RolledUpNetworks.h"

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

    using WebServices::Model::DeviceCollection;
    using WebServices::Model::NetworkAttachmentInfo;

    /**
     *  Data structure representing a copy of currently rolled up devices data (copyable).
     *
     *  This data MAYBE invalidated due to changes in Network::UserOverridesType settings (even historical database data
     *  cuz changes in rollup rules could cause how the historical data was rolled up to change).
     *
     *  \note   \em Thread-Safety   <a href="Thread-Safety.md#C++-Standard-Thread-Safety">C++-Standard-Thread-Safety</a>
     */
    struct RolledUpDevices {
    private:
        RolledUpDevices (DBAccess::Mgr* dbAccessMgr, const Iterable<Device>& devices2MergeIn, const Mapping<GUID, Device::UserOverridesType>& userOverrides,
                         const RolledUpNetworks& useRolledUpNetworks, const RolledUpNetworkInterfaces& useNetworkInterfaceRollups);

    public:
        RolledUpDevices (const RolledUpDevices&)            = default;
        RolledUpDevices (RolledUpDevices&&)                 = default;
        RolledUpDevices& operator= (const RolledUpDevices&) = default;
        RolledUpDevices& operator= (RolledUpDevices&&)      = default;

    public:
        /**
         *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
         */
        static RolledUpDevices GetCached (DBAccess::Mgr* dbAccessMgr, Time::DurationSecondsType allowedStaleness = 10.0);

    public:
        /**
         *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
         */
        static void InvalidateCache (DBAccess::Mgr* dbAccessMgr);

    public:
        /**
         *  This returns the current rolled up device objects.
         */
        nonvirtual DeviceCollection GetDevices () const;

    public:
        /**
         *  Given an aggregated device id, map to the correspoding rollup ID (todo do we need to handle missing case)
         * 
         *  \req is already valid rollup ID.
         */
        nonvirtual auto MapAggregatedID2ItsRollupID (const GUID& aggregatedDeviceID) const -> GUID;

    public:
        nonvirtual void ResetUserOverrides (DBAccess::Mgr* dbAccessMgr, const Mapping<GUID, Device::UserOverridesType>& userOverrides);

    public:
        nonvirtual void MergeIn (DBAccess::Mgr* dbAccessMgr, const Iterable<Device>& devices2MergeIn);

    private:
        enum class PassFailType_ {
            ePass,
            eFail
        };
        // if fails simple merge, returns false, so must call recomputeall
        PassFailType_ MergeIn_ (DBAccess::Mgr* dbAccessMgr, const Device& d2MergeIn);
        void          MergeInUpdate_ (const Device& rollupDevice, const Device& newDevice2MergeIn);
        void          MergeInNew_ (DBAccess::Mgr* dbAccessMgr, const Device& d2MergeIn);
        void          RecomputeAll_ (DBAccess::Mgr* dbAccessMgr);
        auto          MapAggregatedAttachments2Rollups_ (const Mapping<GUID, NetworkAttachmentInfo>& nats) -> Mapping<GUID, NetworkAttachmentInfo>;
        static bool   ShouldRollup_ (const Device& exisingRolledUpDevice, const Device& d2PotentiallyMergeIn);

    private:
        DeviceCollection          fStarterRollups_;
        DeviceCollection          fRolledUpDevices;
        DeviceCollection          fRawDevices_;
        Mapping<GUID, GUID>       fRaw2RollupIDMap_; // each aggregate deviceid is mapped to at most one rollup id)
        RolledUpNetworks          fUseRolledUpNetworks;
        RolledUpNetworkInterfaces fUseNetworkInterfaceRollups;
        // sRolledUpDevicesSoFar_: keep a cache of the rolled up devices so far, just
        // as a slight performance tweek
        static Synchronized<optional<RolledUpDevices>> sRolledUpDevicesSoFar_;
    };
    inline Synchronized<optional<RolledUpDevices>> RolledUpDevices::sRolledUpDevicesSoFar_;

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "RolledUpDevices.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Mgr_Private_RolledUpDevices_h_*/
