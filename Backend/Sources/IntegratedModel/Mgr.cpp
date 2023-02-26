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
#include "Stroika/Foundation/IO/FileSystem/WellKnownLocations.h"

#include "../Common/BLOBMgr.h"
#include "../Common/DB.h"

#include "../Discovery/Devices.h"
#include "../Discovery/NetworkInterfaces.h"
#include "../Discovery/Networks.h"

#include "AppVersion.h"

#include "Private_/DBAccess.h"
#include "Private_/FromDiscovery.h"
#include "Private_/RolledUpNetworkInterfaces.h"

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
     */
    namespace RollupSummary_ {

        using IntegratedModel::Device;
        using IntegratedModel::Network;
        using IntegratedModel::NetworkAttachmentInfo;
        using IntegratedModel::NetworkInterface;

        using IntegratedModel::Private_::RolledUpNetworkInterfaces;

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
            RolledUpNetworks (const Iterable<Network>& nets2MergeIn, const Mapping<GUID, Network::UserOverridesType>& userOverrides,
                              const RolledUpNetworkInterfaces& useNetworkInterfaceRollups)
                : fUseNetworkInterfaceRollups{useNetworkInterfaceRollups}
            {
                fStarterRollups_   = userOverrides.Map<Network> ([] (const auto& guid2UOTPair) -> Network {
                    Network nw;
                    nw.fID            = guid2UOTPair.fKey;
                    nw.fUserOverrides = guid2UOTPair.fValue;
                    if (nw.fUserOverrides and nw.fUserOverrides->fName) {
                        nw.fNames.Add (*nw.fUserOverrides->fName, 500);
                    }
                    return nw;
                });
                fRolledUpNetworks_ = fStarterRollups_;
                MergeIn (nets2MergeIn);
            }
            RolledUpNetworks (const RolledUpNetworks&)            = default;
            RolledUpNetworks (RolledUpNetworks&&)                 = default;
            RolledUpNetworks& operator= (RolledUpNetworks&&)      = default;
            RolledUpNetworks& operator= (const RolledUpNetworks&) = default;

        public:
            /**
             *  This returns the current rolled up network objects.
             */
            nonvirtual NetworkCollection GetNetworks () const { return fRolledUpNetworks_; }

        public:
            nonvirtual void ResetUserOverrides (const Mapping<GUID, Network::UserOverridesType>& userOverrides)
            {
                fStarterRollups_ = userOverrides.Map<Network> ([] (const auto& guid2UOTPair) -> Network {
                    Network nw;
                    nw.fID            = guid2UOTPair.fKey;
                    nw.fUserOverrides = guid2UOTPair.fValue;
                    if (nw.fUserOverrides and nw.fUserOverrides->fName) {
                        nw.fNames.Add (*nw.fUserOverrides->fName, 500);
                    }
                    return nw;
                });
                RecomputeAll_ ();
            }

        public:
            /**
             *  Given an aggregated network id, map to the correspoding rollup ID (todo do we need to handle missing case)
             * 
             *  \req is already valid rollup net ID.
             */
            nonvirtual auto MapAggregatedID2ItsRollupID (const GUID& netID) const -> GUID
            {
                if (auto r = fMapAggregatedNetID2RollupID_.Lookup (netID)) {
                    return *r;
                }
                // shouldn't get past here - debug if/why this hapepns - see comments below
                Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (
                    L"MapAggregatedID2ItsRollupID failed to find netID=%s", Characters::ToString (netID).c_str ())};
                if constexpr (qDebug) {
                    for ([[maybe_unused]] const auto& i : fRolledUpNetworks_) {
                        DbgTrace (L"rolledupNet=%s", Characters::ToString (i).c_str ());
                    }
                }
                Assert (false);     // @todo fix - because we guarantee each item rolled up exactly once - but happens sometimes on change of network - I think due to outdated device records referring to newer network not yet in this cache...
                WeakAssert (false); // @todo fix - because we guarantee each item rolled up exactly once - but happens sometimes on change of network - I think due to outdated device records referring to newer network not yet in this cache...
                return netID;
            }

        public:
            /**
             *  Modify this set of rolled up networks with this net2MergeIn. If we've seen this network before (by ID)
             *  possibly remove it from some rollup, and possibly even remove that (if now empty) rollup.
             *
             *  But typically this will just update the record for an existing rollup.
             */
            nonvirtual void MergeIn (const Iterable<Network>& nets2MergeIn)
            {
                fRawNetworks_ += nets2MergeIn;
                bool anyFailed = false;
                for (const Network& n : nets2MergeIn) {
                    if (MergeIn_ (n) == PassFailType_::eFail) {
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
             * INVALIDATE IF 'UserSettings' change, which might cause different rollups (this includes fingerprint to guid map)
             *
             *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
             */
            static RolledUpNetworks GetCached (Time::DurationSecondsType allowedStaleness = 5.0)
            {
                Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...RolledUpNetworks::GetCached")};
                Debug::TimingTrace        ttrc{L"RolledUpNetworks::GetCached", 1};
                // SynchronizedCallerStalenessCache object just assures one rollup RUNS internally at a time, and
                // that two calls in rapid succession, the second call re-uses the previous value
                static Cache::SynchronizedCallerStalenessCache<void, RolledUpNetworks> sCache_;
                // Disable fHoldWriteLockDuringCacheFill due to https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/23
                // See also
                //      https://stroika.atlassian.net/browse/STK-906 - possible enhancement to this configuration to work better avoiding
                //      See https://stroika.atlassian.net/browse/STK-907 - about needing some new mechanism in Stroika for deadlock detection/avoidance.
                // sCache_.fHoldWriteLockDuringCacheFill = true; // so only one call to filler lambda at a time
                return sCache_.LookupValue (sCache_.Ago (allowedStaleness), [allowedStaleness] () -> RolledUpNetworks {
                    /*
                     *  DEADLOCK NOTE
                     *      Since this can be called while rolling up DEVICES, its important that this code not call anything involving device rollup since
                     *      that could trigger a deadlock.
                     */
                    Debug::TraceContextBumper ctx{
                        Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...RolledUpNetworks::GetCached...cachefiller")};
                    Debug::TimingTrace ttrc{L"RolledUpNetworks::GetCached...cachefiller", 1};

                    // Start with the existing rolled up objects
                    // and merge in any more recent discovery changes
                    RolledUpNetworks result = [allowedStaleness] () {
                        auto rolledUpNetworkInterfacess = RolledUpNetworkInterfaces::GetCached (&sDBAccessMgr_.value (), allowedStaleness * 3.0); // longer allowedStaleness cuz we dont care much about this and the parts
                            // we look at really dont change
                        auto lk = sRolledUpNetworks_.rwget ();
                        if (not lk.cref ().has_value ()) {
                            if (not sDBAccessMgr_->GetFinishedInitialDBLoad ()) {
                                // Design Choice - could return non-standardized rollup IDs if DB not loaded, but then those IDs would
                                // disappear later in the run, leading to possible client confusion. Best to just not say anything til DB loaded
                                // Could ALSO do 2 stage DB load - critical stuff for IDs, and the detailed DB records. All we need is first
                                // stage for here...
                                Execution::Throw (HTTP::Exception{HTTP::StatusCodes::kServiceUnavailable, L"Database not yet loaded"_k});
                            };
                            // @todo add more stuff here - empty preset rules from DB
                            // merge two tables - ID to fingerprint and user settings tables and store those in this rollup early
                            // maybe make CTOR for rolledupnetworks take in ital DB netwworks and rules, and have copyis CTOR taking orig networks and new rules?
                            lk.store (RolledUpNetworks{sDBAccessMgr_->GetRawNetworks (), sDBAccessMgr_->GetNetworkUserSettings (), rolledUpNetworkInterfacess});
                        }
                        return Memory::ValueOf (lk.load ());
                    }();
                    result.MergeIn (IntegratedModel::Private_::FromDiscovery::GetNetworks ());
                    sRolledUpNetworks_.store (result); // save here so we can update rollup networks instead of creating anew each time
                    return result;
                });
            }

        public:
            /**
             *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
             */
            static void InvalidateCache ()
            {
                auto lk = sRolledUpNetworks_.rwget ();
                if (lk->has_value ()) {
                    lk.rwref ()->ResetUserOverrides (sDBAccessMgr_->GetNetworkUserSettings ());
                }
                // else OK if not yet loaded, nothing to invalidate
            }

        private:
            enum class PassFailType_ {
                ePass,
                eFail
            };
            // if fails simple merge, returns false, so must call recomputeall
            PassFailType_ MergeIn_ (const Network& net2MergeIn)
            {
                // @todo https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/75 - fix corner case
                Network::FingerprintType net2MergeInFingerprint      = net2MergeIn.GenerateFingerprintFromProperties ();
                const auto [oShouldRollIntoNet, shouldInvalidateAll] = ShouldRollupInto_ (net2MergeIn, net2MergeInFingerprint);
                if (shouldInvalidateAll == PassFailType_::ePass) {
                    if (oShouldRollIntoNet) {
                        // then we rollup to this same rollup network - very common case - so just re-rollup, and we are done
                        AddUpdateIn_ (*oShouldRollIntoNet, net2MergeIn, net2MergeInFingerprint);
                    }
                    else {
                        AddNewIn_ (net2MergeIn, net2MergeInFingerprint);
                    }
                    return PassFailType_::ePass;
                }
                return PassFailType_::eFail;
            }
            // Find the appropriate network to merge net2MergeIn into from our existing networks (using whatever keys we have to make this often quicker)
            // This can be slow for the case of a network change. And it can refer to a different rollup network than it used to.
            // second part of tuple returned is 'revalidateAll' - force recompute of whole rollup

            // @todo NOTE - a net COULD be directed to rollup into one network due to fingerprint and another due to matching gateway hardware ID
            // RESOLVE this by agreeing that hardwareGatewayID takes PRECEDENCE.
            //
            // And consider it a DATA ERROR (need code to assure cannot happen) - for the same hardare ID to appear in two rollup networks match rules
            // or the same (any type of rollup rule) to appear in differnt high level user settings)
            tuple<optional<Network>, PassFailType_> ShouldRollupInto_ (const Network& net2MergeIn, const Network::FingerprintType& net2MergeInFingerprint)
            {
                auto formerRollupID = fMapFingerprint2RollupID.Lookup (net2MergeInFingerprint);
                if (formerRollupID) {
                    auto alreadyRolledUpNetwork = Memory::ValueOf (fRolledUpNetworks_.Lookup (*formerRollupID)); // must be in list because we keep those in sync here in this class
                    if (ShouldRollupInto_CheckIsCompatibleWithTarget_ (net2MergeIn, net2MergeInFingerprint, alreadyRolledUpNetwork)) {
                        return make_tuple (alreadyRolledUpNetwork, PassFailType_::ePass);
                    }
                }
                // SEARCH FULL LIST of already rolled up networks to find the right network to roll into; if we already had rolled into a
                // different one, don't bother cuz have to recompute anyhow
                if (not formerRollupID.has_value ()) {
                    for (const auto& ri : fRolledUpNetworks_) {
                        if (ShouldRollupInto_CheckIsCompatibleWithTarget_ (net2MergeIn, net2MergeInFingerprint, ri)) {
                            return make_tuple (ri, PassFailType_::ePass);
                        }
                    }
                }
                return make_tuple (nullopt, PassFailType_::eFail);
            }
            bool ShouldRollupInto_CheckIsCompatibleWithTarget_ (const Network& net2MergeIn, const Network::FingerprintType& net2MergeInFingerprint,
                                                                const Network& targetRollup)
            {
                if (auto riu = targetRollup.fUserOverrides) {
                    if (riu->fAggregateFingerprints and riu->fAggregateFingerprints->Contains (net2MergeInFingerprint)) {
                        return true;
                    }
                    if (riu->fAggregateGatewayHardwareAddresses and
                        riu->fAggregateGatewayHardwareAddresses->Intersects (net2MergeIn.fGatewayHardwareAddresses)) {
                        return true;
                    }
                    if (riu->fAggregateNetworks and riu->fAggregateNetworks->Contains (net2MergeIn.fID)) {
                        return true;
                    }
                    if (riu->fAggregateNetworkInterfacesMatching) {
                        for (const auto& rule : *riu->fAggregateNetworkInterfacesMatching) {
                            // net2MergeIn is unaggregated and so net2MergeIn.fAttachedInterfaces are as well dont call fUseNetworkInterfaceRollups.GetConcreteNeworkInterfaces
                            // but also need api to grab aggreaged guy by ID
                            if (fUseNetworkInterfaceRollups.GetRawNetworkInterfaces (net2MergeIn.fAttachedInterfaces).All ([&] (const NetworkInterface& i) {
                                    return i.fType == rule.fInterfaceType and i.GenerateFingerprintFromProperties () == rule.fFingerprint;
                                })) {
                                return true;
                            }
                        }
                    }
                }
                return false;
            }
            void AddUpdateIn_ (const Network& addNet2MergeFromThisRollup, const Network& net2MergeIn, const Network::FingerprintType& net2MergeInFingerprint)
            {
                Assert (net2MergeIn.GenerateFingerprintFromProperties () == net2MergeInFingerprint); // provided to avoid cost of recompute
                Network newRolledUpNetwork = Network::Rollup (addNet2MergeFromThisRollup, net2MergeIn);
                newRolledUpNetwork.fAttachedInterfaces +=
                    fUseNetworkInterfaceRollups.MapAggregatedNetInterfaceID2ItsRollupID (net2MergeIn.fAttachedInterfaces);
                Assert (addNet2MergeFromThisRollup.fAggregatesFingerprints == newRolledUpNetwork.fAggregatesFingerprints); // spot check - should be same...
                fRolledUpNetworks_.Add (newRolledUpNetwork);
                fMapAggregatedNetID2RollupID_.Add (net2MergeIn.fID, newRolledUpNetwork.fID);
                fMapFingerprint2RollupID.Add (net2MergeInFingerprint, newRolledUpNetwork.fID);
            }
            void AddNewIn_ (const Network& net2MergeIn, const Network::FingerprintType& net2MergeInFingerprint)
            {
                Assert (net2MergeIn.GenerateFingerprintFromProperties () == net2MergeInFingerprint); // provided to avoid cost of recompute
                Network newRolledUpNetwork = net2MergeIn;
                newRolledUpNetwork.fAttachedInterfaces =
                    fUseNetworkInterfaceRollups.MapAggregatedNetInterfaceID2ItsRollupID (net2MergeIn.fAttachedInterfaces);
                newRolledUpNetwork.fAggregatesReversibly   = Set<GUID>{net2MergeIn.fID};
                newRolledUpNetwork.fAggregatesFingerprints = Set<Network::FingerprintType>{net2MergeInFingerprint};
                // @todo fix this code so each time through we UPDATE sDBAccessMgr_ with latest 'fingerprint' of each dynamic network
                newRolledUpNetwork.fID = sDBAccessMgr_->GenNewNetworkID (newRolledUpNetwork, net2MergeIn);
                if (fRolledUpNetworks_.Contains (newRolledUpNetwork.fID)) {
                    // Should probably never happen, but since depends on data in database, program defensively

                    // at this point we have a net2MergeIn that said 'no' to ShouldRollup to all existing networks we've rolled up before
                    // and yet somehow, result contains a network that used our ID?
                    auto shouldntRollUpButTookOurIDNet = Memory::ValueOf (fRolledUpNetworks_.Lookup (newRolledUpNetwork.fID));
                    DbgTrace (L"shouldntRollUpButTookOurIDNet=%s", Characters::ToString (shouldntRollUpButTookOurIDNet).c_str ());
                    DbgTrace (L"net2MergeIn=%s", Characters::ToString (net2MergeIn).c_str ());
                    //Assert (not ShouldRollup_ (shouldntRollUpButTookOurIDNet, net2MergeIn));
                    Logger::sThe.Log (Logger::eWarning, L"Got rollup network ID from cache that is already in use: %s (for external address %s)",
                                      Characters::ToString (newRolledUpNetwork.fID).c_str (),
                                      Characters::ToString (newRolledUpNetwork.fExternalAddresses).c_str ());
                    newRolledUpNetwork.fID = GUID::GenerateNew ();
                }
                newRolledUpNetwork.fUserOverrides = sDBAccessMgr_->LookupNetworkUserSettings (newRolledUpNetwork.fID);
                if (newRolledUpNetwork.fUserOverrides && newRolledUpNetwork.fUserOverrides->fName) {
                    newRolledUpNetwork.fNames.Add (*newRolledUpNetwork.fUserOverrides->fName, 500);
                }
                fRolledUpNetworks_.Add (newRolledUpNetwork);
                fMapAggregatedNetID2RollupID_.Add (net2MergeIn.fID, newRolledUpNetwork.fID);

                // is this guarnateed unique?
                fMapFingerprint2RollupID.Add (net2MergeInFingerprint, newRolledUpNetwork.fID);
            }
            void RecomputeAll_ ()
            {
                Debug::TraceContextBumper ctx{"{}...RolledUpNetworks::RecomputeAll_"};
                fRolledUpNetworks_.clear ();
                fMapAggregatedNetID2RollupID_.clear ();
                fMapFingerprint2RollupID.clear ();
                fRolledUpNetworks_ += fStarterRollups_;
                fRawNetworks_.Apply ([this] (const Network& n) {
                    if (MergeIn_ (n) == PassFailType_::eFail) {
                        AddNewIn_ (n, n.GenerateFingerprintFromProperties ());
                    }
                });
            }

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
        Synchronized<optional<RolledUpNetworks>> RolledUpNetworks::sRolledUpNetworks_;

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

                    auto rolledUpNetworks = RolledUpNetworks::GetCached (allowedStaleness * 3.0);                                             // longer allowedStaleness cuz we dont care much about this and the parts
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
    return Sequence<IntegratedModel::Network>{RollupSummary_::RolledUpNetworks::GetCached ().GetNetworks ()};
}

optional<IntegratedModel::Network> IntegratedModel::Mgr::GetNetwork (const GUID& id, optional<Duration>* ttl) const
{
    // first check rolled up networks, and then raw/unrolled up networks
    auto networkRollupsCache = RollupSummary_::RolledUpNetworks::GetCached ();
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
        RollupSummary_::RolledUpNetworks::InvalidateCache ();
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