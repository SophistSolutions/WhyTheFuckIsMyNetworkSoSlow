/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Cache/SynchronizedCallerStalenessCache.h"
#include "Stroika/Foundation/Common/GUID.h"
#include "Stroika/Foundation/Common/KeyValuePair.h"
#include "Stroika/Foundation/Common/Property.h"
#include "Stroika/Foundation/Containers/Association.h"
#include "Stroika/Foundation/Containers/KeyedCollection.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/Database/SQL/ORM/Schema.h"
#include "Stroika/Foundation/Database/SQL/ORM/TableConnection.h"
#include "Stroika/Foundation/Database/SQL/ORM/Versioning.h"
#include "Stroika/Foundation/Database/SQL/SQLite.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Execution/Logger.h"
#include "Stroika/Foundation/Execution/Sleep.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/Execution/TimeOutException.h"

#include "../Common/BLOBMgr.h"
#include "../Common/DB.h"

#include "../Discovery/Devices.h"
#include "../Discovery/NetworkInterfaces.h"
#include "../Discovery/Networks.h"

#include "AppVersion.h"

#include "Private_/DBAccess.h"
#include "Private_/FromDiscovery.h"
#include "Private_/RolledUpNetworkInterfaces.h"
#include "Private_/RolledUpNetworks.h"

#include "Mgr.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Common;
using namespace Stroika::Foundation::Database;
using namespace Stroika::Foundation::DataExchange;
using namespace Stroika::Foundation::Execution;
using namespace Stroika::Foundation::Memory;
using namespace Stroika::Foundation::IO::Network;
using namespace Stroika::Foundation::IO::Network::HTTP;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;

using Stroika::Foundation::Common::ConstantProperty;
using Stroika::Foundation::Common::GUID;
using Stroika::Foundation::Traversal::Range;

using WebServices::Model::Device;
using WebServices::Model::DeviceCollection;
using WebServices::Model::Network;
using WebServices::Model::NetworkAttachmentInfo;
using WebServices::Model::NetworkCollection;
using WebServices::Model::NetworkInterface;
using WebServices::Model::NetworkInterfaceCollection;

namespace {
    optional<IntegratedModel::Private_::DBAccess::Mgr> sDBAccessMgr_; // constructed on module activation
}

namespace {
    constexpr chrono::duration<double> kTTLForRollupsReturned_{10s};
    constexpr chrono::duration<double> kTTLForActiveObjectsReturned_{10s};
    constexpr chrono::duration<double> kTTLForHistroicalDBObjectsReturned_{24h};

    /**
     *  \breif RollupSummary_ - Data structures representing a rollups of various bits of networking/device etc data
     * 
     *  \note   These rollup objects are copyable.
     * 
     *  \note   Important design principle is that RolledUp... objects can be captured as of a point
     *          in time, and then additional data 'rolled in'.
     * 
     *          The reason this is important, is we have alot of historical data as a baseline (which is unchanging).
     *          And a small amount of data which is dynamic (e.g. current network readings). And we want to compute
     *          a summary of the past data with new data rolled in.
     * 
     *          We do most of the work up to a point in time (from the database). Save that snapshot. And then merge
     *          in additional data as needed.
     * 
     *  \note   The one thing that could cause us to have to COMPLETELY redo our computations, is when we change 'user settings'
     *          which control how the rollup happens.
     * 
     *  \note   Because each of these objects (e.g. Network and NetworkInterfaces) are separately rolled up, and separately cached,
     *          they CAN be out of date with respect to one another (@todo CONSIDER FIXING THIS BY PASSING IN ROLLUP TO USE
     *          AND QUICK CHECK IF SAME).
     */
    namespace RollupSummary_ {

        using IntegratedModel::Device;
        using IntegratedModel::Network;
        using IntegratedModel::NetworkAttachmentInfo;
        using IntegratedModel::NetworkInterface;

        using IntegratedModel::Private_::RolledUpNetworkInterfaces;
        using IntegratedModel::Private_::RolledUpNetworks;

        /**
         *  Data structure representing a copy of currently rolled up devices data (copyable).
         *
         *  This data MAYBE invalidated due to changes in Network::UserOverridesType settings (even historical database data
         *  cuz changes in rollup rules could cause how the historical data was rolled up to change).
         *
         *  \note   \em Thread-Safety   <a href="Thread-Safety.md#C++-Standard-Thread-Safety">C++-Standard-Thread-Safety</a>
         */
        struct RolledUpDevices {
        public:
            RolledUpDevices (const Iterable<Device>& devices2MergeIn, const Mapping<GUID, Device::UserOverridesType>& userOverrides,
                             const RolledUpNetworks& useRolledUpNetworks, const RolledUpNetworkInterfaces& useNetworkInterfaceRollups)
                : fUseRolledUpNetworks{useRolledUpNetworks}
                , fUseNetworkInterfaceRollups{useNetworkInterfaceRollups}
            {
                fStarterRollups_ = userOverrides.Map<Device> ([] (const auto& guid2UOTPair) -> Device {
                    Device d;
                    d.fID            = guid2UOTPair.fKey;
                    d.fUserOverrides = guid2UOTPair.fValue;
                    if (d.fUserOverrides and d.fUserOverrides->fName) {
                        d.fNames.Add (*d.fUserOverrides->fName, 500);
                    }
                    return d;
                });
                fRolledUpDevices = fStarterRollups_;
                MergeIn (devices2MergeIn);
            }
            RolledUpDevices (const RolledUpDevices&)            = default;
            RolledUpDevices (RolledUpDevices&&)                 = default;
            RolledUpDevices& operator= (const RolledUpDevices&) = default;
            RolledUpDevices& operator= (RolledUpDevices&&)      = default;

        public:
            /**
             *  This returns the current rolled up device objects.
             */
            nonvirtual DeviceCollection GetDevices () const { return fRolledUpDevices; }

        public:
            /**
             *  Given an aggregated device id, map to the correspoding rollup ID (todo do we need to handle missing case)
             * 
             *  \req is already valid rollup ID.
             */
            nonvirtual auto MapAggregatedID2ItsRollupID (const GUID& aggregatedDeviceID) const -> GUID
            {
                if (auto r = fRaw2RollupIDMap_.Lookup (aggregatedDeviceID)) {
                    return *r;
                }
                // shouldn't get past here - debug if/why this hapepns - see comments below
                Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (
                    L"MapAggregatedID2ItsRollupID failed to find netID=%s", Characters::ToString (aggregatedDeviceID).c_str ())};
                if constexpr (qDebug) {
                    for ([[maybe_unused]] const auto& i : fRolledUpDevices) {
                        DbgTrace (L"rolledupDevice=%s", Characters::ToString (i).c_str ());
                    }
                }
                Assert (false);     // not seen yet, but verify - maybe data can leak through to this from webservice request...
                WeakAssert (false); // @todo fix - because we guarantee each item rolled up exactly once - but happens sometimes on change of network - I think due to outdated device records referring to newer network not yet in this cache...
                return aggregatedDeviceID;
            }

        public:
            nonvirtual void ResetUserOverrides (const Mapping<GUID, Device::UserOverridesType>& userOverrides)
            {
                RolledUpNetworkInterfaces networkInterfacesRollup = RolledUpNetworkInterfaces::GetCached (&sDBAccessMgr_.value ());
                fStarterRollups_                                  = userOverrides.Map<Device> ([] (const auto& guid2UOTPair) -> Device {
                    Device d;
                    d.fID            = guid2UOTPair.fKey;
                    d.fUserOverrides = guid2UOTPair.fValue;
                    if (d.fUserOverrides and d.fUserOverrides->fName) {
                        d.fNames.Add (*d.fUserOverrides->fName, 500);
                    }
                    return d;
                });
                RecomputeAll_ ();
            }

        public:
            nonvirtual void MergeIn (const Iterable<Device>& devices2MergeIn)
            {
                fRawDevices_ += devices2MergeIn;
                bool anyFailed = false;
                for (const Device& d : devices2MergeIn) {
                    if (MergeIn_ (d) == PassFailType_::eFail) {
                        anyFailed = true;
                        break;
                    }
                }
                if (anyFailed) {
                    RecomputeAll_ ();
                }
            }

        public:
            /**
             *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
             */
            static RolledUpDevices GetCached (Time::DurationSecondsType allowedStaleness = 10.0)
            {
                Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...RolledUpDevices::GetCached")};
                Debug::TimingTrace        ttrc{L"...RolledUpDevices::GetCached", 1};
                // SynchronizedCallerStalenessCache object just assures one rollup RUNS internally at a time, and
                // that two calls in rapid succession, the second call re-uses the previous value
                static Cache::SynchronizedCallerStalenessCache<void, RolledUpDevices> sCache_;
                // Disable this cache setting due to https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/23
                // See also
                //      https://stroika.atlassian.net/browse/STK-906 - possible enhancement to this configuration to work better avoiding
                //      See https://stroika.atlassian.net/browse/STK-907 - about needing some new mechanism in Stroika for deadlock detection/avoidance.
                // sCache_.fHoldWriteLockDuringCacheFill = true; // so only one call to filler lambda at a time
                return sCache_.LookupValue (sCache_.Ago (allowedStaleness), [=] () -> RolledUpDevices {
                    Debug::TraceContextBumper ctx{
                        Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...RolledUpDevices::GetCached...cachefiller")};
                    Debug::TimingTrace ttrc{L"RolledUpDevices::GetCached...cachefiller", 1};

                    auto rolledUpNetworks = RolledUpNetworks::GetCached (&sDBAccessMgr_.value (), allowedStaleness * 3.0);                    // longer allowedStaleness cuz we dont care much about this and the parts
                                                                                                                                              // we look at really dont change
                    auto rolledUpNetworkInterfacess = RolledUpNetworkInterfaces::GetCached (&sDBAccessMgr_.value (), allowedStaleness * 3.0); // longer allowedStaleness cuz we dont care much about this and the parts
                        // we look at really dont change

                    // Start with the existing rolled up objects
                    // and merge in any more recent discovery changes
                    RolledUpDevices result = [&] () {
                        auto lk = sRolledUpDevicesSoFar_.rwget ();
                        if (not lk.cref ().has_value ()) {
                            if (not sDBAccessMgr_->GetFinishedInitialDBLoad ()) {
                                // Design Choice - could return non-standardized rollup IDs if DB not loaded, but then those IDs would
                                // disappear later in the run, leading to possible client confusion. Best to just not say anything til DB loaded
                                // Could ALSO do 2 stage DB load - critical stuff for IDs, and the detailed DB records. All we need is first
                                // stage for here...
                                Execution::Throw (HTTP::Exception{HTTP::StatusCodes::kServiceUnavailable, L"Database not yet loaded"_k});
                            }
                            // @todo add more stuff here - empty preset rules from DB
                            // merge two tables - ID to fingerprint and user settings tables and store those in this rollup early
                            // maybe make CTOR for rolledupnetworks take in ital DB netwworks and rules, and have copyis CTOR taking orig networks and new rules?
                            RolledUpDevices initialDBDevices{sDBAccessMgr_->GetRawDevices (), sDBAccessMgr_->GetDeviceUserSettings (),
                                                             rolledUpNetworks, rolledUpNetworkInterfacess};
                            lk.store (initialDBDevices);
                        }
                        return Memory::ValueOf (lk.load ());
                    }();
                    // not sure we want to allow this? @todo consider throwing here or asserting out cuz nets rollup IDs would change after this
                    result.MergeIn (IntegratedModel::Private_::FromDiscovery::GetDevices ());
                    sRolledUpDevicesSoFar_.store (result); // save here so we can update rollup networks instead of creating anew each time
                    return result;
                });
            }

        public:
            /**
             *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
             */
            static void InvalidateCache ()
            {
                auto lk = sRolledUpDevicesSoFar_.rwget ();
                if (lk->has_value ()) {
                    lk.rwref ()->ResetUserOverrides (sDBAccessMgr_->GetDeviceUserSettings ());
                }
                // else OK if not yet loaded, nothing to invalidate
            }

        private:
            enum class PassFailType_ {
                ePass,
                eFail
            };
            // if fails simple merge, returns false, so must call recomputeall
            PassFailType_ MergeIn_ (const Device& d2MergeIn)
            {
                // see if it still should be rolled up in the place it was last rolled up as a shortcut
                if (optional<GUID> prevRollupID = fRaw2RollupIDMap_.Lookup (d2MergeIn.fID)) {
                    Device rollupDevice = Memory::ValueOf (fRolledUpDevices.Lookup (*prevRollupID)); // must be there cuz in sync with fRaw2RollupIDMap_
                    if (ShouldRollup_ (rollupDevice, d2MergeIn)) {
                        MergeInUpdate_ (rollupDevice, d2MergeIn);
                        return PassFailType_::ePass;
                    }
                    else {
                        return PassFailType_::eFail; // rollup changed, so recompute all
                    }
                }
                else {
                    // then see if it SHOULD be rolled into an existing rollup device, or if we should create a new one
                    if (auto i = fRolledUpDevices.First (
                            [&d2MergeIn] (const auto& exisingRolledUpDevice) { return ShouldRollup_ (exisingRolledUpDevice, d2MergeIn); })) {
                        MergeInUpdate_ (*i, d2MergeIn);
                    }
                    else {
                        MergeInNew_ (d2MergeIn);
                    }
                    return PassFailType_::ePass;
                }
            }
            void MergeInUpdate_ (const Device& rollupDevice, const Device& newDevice2MergeIn)
            {
                Device d2MergeInPatched            = newDevice2MergeIn;
                d2MergeInPatched.fAttachedNetworks = MapAggregatedAttachments2Rollups_ (d2MergeInPatched.fAttachedNetworks);
                Device tmp                         = Device::Rollup (rollupDevice, d2MergeInPatched);

                Assert (tmp.fID == rollupDevice.fID); // rollup cannot change device ID

                tmp.fAttachedNetworkInterfaces = rollupDevice.fAttachedNetworkInterfaces;
                if (newDevice2MergeIn.fAttachedNetworkInterfaces) {
                    if (tmp.fAttachedNetworkInterfaces == nullopt) {
                        tmp.fAttachedNetworkInterfaces = Set<GUID>{};
                    }
                    *tmp.fAttachedNetworkInterfaces +=
                        fUseNetworkInterfaceRollups.MapAggregatedNetInterfaceID2ItsRollupID (*newDevice2MergeIn.fAttachedNetworkInterfaces);
                }

                // userSettings already added on first rollup
                fRolledUpDevices += tmp;
                fRaw2RollupIDMap_.Add (newDevice2MergeIn.fID, tmp.fID);
            }
            void MergeInNew_ (const Device& d2MergeIn)
            {
                Assert (not d2MergeIn.fAggregatesReversibly.has_value ());
                Device newRolledUpDevice                = d2MergeIn;
                newRolledUpDevice.fAggregatesReversibly = Set<GUID>{d2MergeIn.fID};
                newRolledUpDevice.fID                   = sDBAccessMgr_->GenNewDeviceID (d2MergeIn.GetHardwareAddresses ());
                if (GetDevices ().Contains (newRolledUpDevice.fID)) {
                    // Should probably never happen, but since depends on data in database, program defensively
                    Logger::sThe.Log (Logger::eWarning, L"Got rollup device ID from cache that is already in use: %s (for hardware addresses %s)",
                                      Characters::ToString (newRolledUpDevice.fID).c_str (),
                                      Characters::ToString (d2MergeIn.GetHardwareAddresses ()).c_str ());
                    newRolledUpDevice.fID = GUID::GenerateNew ();
                }
                newRolledUpDevice.fAttachedNetworks = MapAggregatedAttachments2Rollups_ (newRolledUpDevice.fAttachedNetworks);
                if (d2MergeIn.fAttachedNetworkInterfaces) {
                    newRolledUpDevice.fAttachedNetworkInterfaces =
                        fUseNetworkInterfaceRollups.MapAggregatedNetInterfaceID2ItsRollupID (*d2MergeIn.fAttachedNetworkInterfaces);
                }
                newRolledUpDevice.fUserOverrides = sDBAccessMgr_->LookupDevicesUserSettings (newRolledUpDevice.fID);
                if (newRolledUpDevice.fUserOverrides && newRolledUpDevice.fUserOverrides->fName) {
                    newRolledUpDevice.fNames.Add (*newRolledUpDevice.fUserOverrides->fName, 500);
                }
                fRolledUpDevices += newRolledUpDevice;
                fRaw2RollupIDMap_.Add (d2MergeIn.fID, newRolledUpDevice.fID);
            }
            void RecomputeAll_ ()
            {
                fRolledUpDevices.clear ();
                fRaw2RollupIDMap_.clear ();
                fRolledUpDevices += fStarterRollups_;
                for (const auto& di : fRawDevices_) {
                    if (MergeIn_ (di) == PassFailType_::eFail) {
                        Assert (false); //nyi - or maybe just bug since we have mapping so device goes into two different rollups?
                    }
                }
            }
            auto MapAggregatedAttachments2Rollups_ (const Mapping<GUID, NetworkAttachmentInfo>& nats) -> Mapping<GUID, NetworkAttachmentInfo>
            {
                Mapping<GUID, NetworkAttachmentInfo> result;
                for (const auto& ni : nats) {
                    result.Add (fUseRolledUpNetworks.MapAggregatedID2ItsRollupID (ni.fKey), ni.fValue);
                }
                return result;
            };
            static bool ShouldRollup_ (const Device& exisingRolledUpDevice, const Device& d2PotentiallyMergeIn)
            {
                if ((exisingRolledUpDevice.fAggregatesIrreversibly and
                     exisingRolledUpDevice.fAggregatesIrreversibly->Contains (d2PotentiallyMergeIn.fID)) or
                    (exisingRolledUpDevice.fAggregatesIrreversibly and
                     exisingRolledUpDevice.fAggregatesIrreversibly->Contains (d2PotentiallyMergeIn.fID))) {
                    // we retry the same 'discovered' networks repeatedly and re-roll them up.
                    // mostly this is handled by having the same hardware addresses, but sometimes (like for main discovered device)
                    // MAY not yet / always have network interface). And besides, this check cheaper/faster probably.
                    return true;
                }
                // very rough first draft. Later add database stored 'exceptions' and/or rules tables to augment this logic
                Set<String> existingRollupHWAddresses             = exisingRolledUpDevice.GetHardwareAddresses ();
                Set<String> d2PotentiallyMergeInHardwareAddresses = d2PotentiallyMergeIn.GetHardwareAddresses ();
                if (Set<String>::Intersects (existingRollupHWAddresses, d2PotentiallyMergeInHardwareAddresses)) {
                    return true;
                }
                if (auto userSettings = exisingRolledUpDevice.fUserOverrides) {
                    if (userSettings->fAggregateDeviceHardwareAddresses) {
                        if (userSettings->fAggregateDeviceHardwareAddresses->Intersects (d2PotentiallyMergeInHardwareAddresses)) {
                            return true;
                        }
                    }
                }
                // If EITHER device has no hardware addresses, there is little to identify it, so roll it up with anything with the same IP address, by default
                if (existingRollupHWAddresses.empty () or d2PotentiallyMergeInHardwareAddresses.empty ()) {
                    // then fold together if they have the same IP Addresses
                    // return d1.GetInternetAddresses () == d2.GetInternetAddresses ();
                    return Set<InternetAddress>::Intersects (exisingRolledUpDevice.GetInternetAddresses (),
                                                             d2PotentiallyMergeIn.GetInternetAddresses ());
                }
                // unclear if above test should be if EITHER set is empty, maybe then do if timeframes very close?
                return false;
            }

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
        Synchronized<optional<RolledUpDevices>> RolledUpDevices::sRolledUpDevicesSoFar_;

    }
}

/*
 ********************************************************************************
 ************************** IntegratedModel::Mgr::Activator *********************
 ********************************************************************************
 */
IntegratedModel::Mgr::Activator::Activator ()
{
    Debug::TraceContextBumper ctx{L"IntegratedModel::Mgr::Activator::Activator"};
    Require (sDBAccessMgr_ == nullopt);
    sDBAccessMgr_.emplace ();
}

IntegratedModel::Mgr::Activator::~Activator ()
{
    Debug::TraceContextBumper                        ctx{L"IntegratedModel::Mgr::Activator::~Activator"};
    Execution::Thread::SuppressInterruptionInContext suppressInterruption; // must complete this abort and wait for done - this cannot abort/throw
    sDBAccessMgr_ = nullopt;
}

/*
 ********************************************************************************
 ****************************** IntegratedModel::Mgr ****************************
 ********************************************************************************
 */
Sequence<IntegratedModel::Device> IntegratedModel::Mgr::GetDevices () const
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"IntegratedModel::Mgr::GetDevices")};
    Debug::TimingTrace        ttrc{L"IntegratedModel::Mgr::GetDevices", .1};
    return Sequence<IntegratedModel::Device>{RollupSummary_::RolledUpDevices::GetCached ().GetDevices ()};
}

optional<IntegratedModel::Device> IntegratedModel::Mgr::GetDevice (const GUID& id, optional<Duration>* ttl) const
{
    // first check rolled up devices, and then raw/unrolled up devices
    // NOTE - this doesn't check the 'dynamic' copy of the devices - it waits til those get migrated to the DB, once ever
    // 30 seconds roughtly...
    auto devicesRollupCache = RollupSummary_::RolledUpDevices::GetCached ();
    auto result             = devicesRollupCache.GetDevices ().Lookup (id);
    if (result) {
        if (ttl != nullptr) {
            bool justStarted = Time::GetTickCount () < 60; // if just started, this trick of looking at EverSeen() doesn't work (cuz maybe just not discovered yet)
            auto everSeen    = result->fSeen.EverSeen ();
            // This isn't a super-reliable way to check - find a better more reliable way to set the ttl
            if (not justStarted and everSeen and everSeen->GetUpperBound () + 15min < DateTime::Now ()) {
                *ttl = 2min;
            }
            else {
                *ttl = kTTLForRollupsReturned_;
            }
        }
    }
    else {
        if (result = sDBAccessMgr_->GetRawDevices ().Lookup (id)) {
            result->fIDPersistent = true;
            result->fAggregatedBy = devicesRollupCache.MapAggregatedID2ItsRollupID (id);
            if (ttl != nullptr) {
                auto everSeen = result->fSeen.EverSeen ();
                // This isn't a super-reliable way to check - find a better more reliable way to set the ttl
                if (everSeen and everSeen->GetUpperBound () >= (DateTime::Now () - 1h)) {
                    *ttl = kTTLForActiveObjectsReturned_;
                }
                else {
                    *ttl = kTTLForHistroicalDBObjectsReturned_;
                }
            }
        }
    }
    return result;
}

std::optional<IntegratedModel::Device::UserOverridesType> IntegratedModel::Mgr::GetDeviceUserSettings (const GUID& id) const
{
    return sDBAccessMgr_->LookupDevicesUserSettings (id);
}

void IntegratedModel::Mgr::SetDeviceUserSettings (const Common::GUID& id, const std::optional<IntegratedModel::Device::UserOverridesType>& settings)
{
    if (sDBAccessMgr_->SetDeviceUserSettings (id, settings)) {
        RollupSummary_::RolledUpDevices::InvalidateCache ();
    }
}

std::optional<GUID> IntegratedModel::Mgr::GetCorrespondingDynamicDeviceID (const GUID& id) const
{
    Set<GUID> dynamicDevices{Discovery::DevicesMgr::sThe.GetActiveDevices ().Map<GUID, Set<GUID>> ([] (const auto& d) { return d.fGUID; })};
    if (dynamicDevices.Contains (id)) {
        return id;
    }
    auto thisRolledUpDevice = RollupSummary_::RolledUpDevices::GetCached ().GetDevices ().Lookup (id);
    if (thisRolledUpDevice and thisRolledUpDevice->fAggregatesReversibly) {
        // then find the dynamic device corresponding to this rollup, which will be (as of 2022-06-22) in the aggregates reversibly list
        if (auto ff = thisRolledUpDevice->fAggregatesReversibly->First ([&] (const GUID& d) -> bool { return dynamicDevices.Contains (d); })) {
            Assert (dynamicDevices.Contains (*ff));
            return *ff;
        }
        DbgTrace (L"Info: GetCorrespondingDynamicDeviceID found rollup device with no corresponding dynamic device (can happen if its a "
                  L"hisorical device not on network right now)");
    }
    return nullopt;
}

Sequence<IntegratedModel::Network> IntegratedModel::Mgr::GetNetworks () const
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"IntegratedModel::Mgr::GetNetworks")};
    Debug::TimingTrace        ttrc{L"IntegratedModel::Mgr::GetNetworks", 0.1};
    return Sequence<IntegratedModel::Network>{RollupSummary_::RolledUpNetworks::GetCached (&sDBAccessMgr_.value ()).GetNetworks ()};
}

optional<IntegratedModel::Network> IntegratedModel::Mgr::GetNetwork (const GUID& id, optional<Duration>* ttl) const
{
    // first check rolled up networks, and then raw/unrolled up networks
    auto networkRollupsCache = RollupSummary_::RolledUpNetworks::GetCached (&sDBAccessMgr_.value ());
    auto result              = networkRollupsCache.GetNetworks ().Lookup (id);
    if (result) {
        if (ttl != nullptr) {
            *ttl = kTTLForRollupsReturned_;
        }
    }
    else {
        result = sDBAccessMgr_->GetRawNetworks ().Lookup (id);
        if (result) {
            result->fIDPersistent = true;
            result->fAggregatedBy = networkRollupsCache.MapAggregatedID2ItsRollupID (id);
            auto everSeen         = result->fSeen;
            // This isn't a super-reliable way to check - find a better more reliable way to set the ttl
            if (everSeen.GetUpperBound () >= (DateTime::Now () - 1h)) {
                *ttl = kTTLForActiveObjectsReturned_;
            }
            else {
                *ttl = kTTLForHistroicalDBObjectsReturned_;
            }
        }
    }
    return result;
}

std::optional<IntegratedModel::Network::UserOverridesType> IntegratedModel::Mgr::GetNetworkUserSettings (const GUID& id) const
{
    return sDBAccessMgr_->LookupNetworkUserSettings (id);
}

void IntegratedModel::Mgr::SetNetworkUserSettings (const Common::GUID& id, const std::optional<IntegratedModel::Network::UserOverridesType>& settings)
{
    if (sDBAccessMgr_->SetNetworkUserSettings (id, settings)) {
        RollupSummary_::RolledUpNetworks::InvalidateCache (&sDBAccessMgr_.value ());
    }
}

Collection<IntegratedModel::NetworkInterface> IntegratedModel::Mgr::GetNetworkInterfaces () const
{
    // AS OF 2022-11-02 this returns the currently active network interfaces, but changed to mimic other accessors (rollups returned)
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"IntegratedModel::Mgr::GetNetworkInterfaces")};
    Debug::TimingTrace        ttrc{L"IntegratedModel::Mgr::GetNetworkInterfaces", 0.1};
    return Collection<IntegratedModel::NetworkInterface>{RollupSummary_::RolledUpNetworkInterfaces::GetCached (&sDBAccessMgr_.value ()).GetNetworkInterfacess ()};
}

optional<IntegratedModel::NetworkInterface> IntegratedModel::Mgr::GetNetworkInterface (const GUID& id, optional<Duration>* ttl) const
{
    // AS OF 2022-11-02 this returned the currently active network interfaces, but changed (2022-11-02) to mimic other accessors (rollups returned by default then raw records)
    auto networkInterfacesCache = RollupSummary_::RolledUpNetworkInterfaces::GetCached (&sDBAccessMgr_.value ());
    auto result                 = networkInterfacesCache.GetNetworkInterfacess ().Lookup (id);
    if (result) {
        if (ttl != nullptr) {
            *ttl = kTTLForRollupsReturned_;
        }
        auto deviceRollupCache = RollupSummary_::RolledUpDevices::GetCached ();
        // could cache this info so dont need to search...
        if (auto i = deviceRollupCache.GetDevices ().First (
                [&id] (const Device& d) { return d.fAttachedNetworkInterfaces and d.fAttachedNetworkInterfaces->Contains (id); })) {
            result->fAttachedToDevices = Set<GUID>{i->fID};
        }
    }
    else {
        result = sDBAccessMgr_->GetRawNetworkInterfaces ().Lookup (id);
        if (result) {
            result->fIDPersistent      = true;
            result->fAggregatedBy      = networkInterfacesCache.MapAggregatedID2ItsRollupID (id);
            result->fAttachedToDevices = networkInterfacesCache.GetAttachedToDeviceIDs (id);
            // @todo MUST FIX THIS - sometimes need more info to tell if its a recent one or ancient
            *ttl = kTTLForActiveObjectsReturned_;
        }
    }
    return result;
}