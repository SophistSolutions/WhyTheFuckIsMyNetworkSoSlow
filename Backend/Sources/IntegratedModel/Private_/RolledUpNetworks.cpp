/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Cache/SynchronizedCallerStalenessCache.h"
#include "Stroika/Foundation/Common/GUID.h"
#include "Stroika/Foundation/Common/KeyValuePair.h"
#include "Stroika/Foundation/Common/Property.h"
#include "Stroika/Foundation/Containers/KeyedCollection.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Execution/Logger.h"

#include "../../Common/BLOBMgr.h"
#include "../../Common/EthernetMACAddressOUIPrefixes.h"

#include "../../Discovery/Devices.h"
#include "../../Discovery/NetworkInterfaces.h"
#include "../../Discovery/Networks.h"

#include "FromDiscovery.h"

#include "RolledUpNetworks.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Common;
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

using WebServices::Model::Device;
using WebServices::Model::DeviceCollection;
using WebServices::Model::Network;
using WebServices::Model::NetworkAttachmentInfo;
using WebServices::Model::NetworkCollection;
using WebServices::Model::NetworkInterface;
using WebServices::Model::NetworkInterfaceCollection;

using IntegratedModel::Private_::RolledUpNetworks;

#if kStroika_Version_FullVersion <= Stroika_Make_FULL_VERSION (3, 0, kStroika_Version_Stage_Dev, 8, 1)
#if qHasFeature_fmtlib && (FMT_VERSION >= 110000)
template <>
struct qStroika_Foundation_Characters_FMT_PREFIX_::formatter<Stroika::Foundation::Common::GUID, wchar_t>
    : Stroika::Foundation::Characters::ToStringFormatter<Stroika::Foundation::Common::GUID> {};
template <>
struct qStroika_Foundation_Characters_FMT_PREFIX_::formatter<Stroika::Foundation::Common::GUID, char>
    : Stroika::Foundation::Characters::ToStringFormatterASCII<Stroika::Foundation::Common::GUID> {};
static_assert (Stroika::Foundation::Configuration::StdCompat::formattable<Stroika::Foundation::Common::GUID, wchar_t>);
#endif
#if qHasFeature_fmtlib && (FMT_VERSION >= 110000)
template <>
struct qStroika_Foundation_Characters_FMT_PREFIX_::formatter<Stroika::Foundation::IO::Network::URI, wchar_t>
    : Stroika::Foundation::Characters::ToStringFormatter<Stroika::Foundation::IO::Network::URI> {};
template <>
struct qStroika_Foundation_Characters_FMT_PREFIX_::formatter<Stroika::Foundation::IO::Network::URI, char>
    : Stroika::Foundation::Characters::ToStringFormatterASCII<Stroika::Foundation::IO::Network::URI> {};
static_assert (Stroika::Foundation::Configuration::StdCompat::formattable<Stroika::Foundation::IO::Network::URI, wchar_t>);
#endif
#endif

/*
 ********************************************************************************
 ******************** IntegratedModel::Private_::RolledUpNetworks ***************
 ********************************************************************************
 */
RolledUpNetworks::RolledUpNetworks (DBAccess::Mgr* dbAccessMgr, const Iterable<Network>& nets2MergeIn,
                                    const Mapping<GUID, Network::UserOverridesType>& userOverrides, const RolledUpNetworkInterfaces& useNetworkInterfaceRollups)
    : fUseNetworkInterfaceRollups{useNetworkInterfaceRollups}
{
    fStarterRollups_   = userOverrides.Map<NetworkCollection> ([] (const auto& guid2UOTPair) -> Network {
        Network nw;
        nw.fID            = guid2UOTPair.fKey;
        nw.fUserOverrides = guid2UOTPair.fValue;
        if (nw.fUserOverrides and nw.fUserOverrides->fName) {
            nw.fNames.Add (*nw.fUserOverrides->fName, 500);
        }
        return nw;
    });
    fRolledUpNetworks_ = fStarterRollups_;
    MergeIn (dbAccessMgr, nets2MergeIn);
}

void RolledUpNetworks::ResetUserOverrides (DBAccess::Mgr* dbAccessMgr, const Mapping<GUID, Network::UserOverridesType>& userOverrides)
{
    RequireNotNull (dbAccessMgr);
    fStarterRollups_ = userOverrides.Map<NetworkCollection> ([] (const auto& guid2UOTPair) -> Network {
        Network nw;
        nw.fID            = guid2UOTPair.fKey;
        nw.fUserOverrides = guid2UOTPair.fValue;
        if (nw.fUserOverrides and nw.fUserOverrides->fName) {
            nw.fNames.Add (*nw.fUserOverrides->fName, 500);
        }
        return nw;
    });
    RecomputeAll_ (dbAccessMgr);
}

auto RolledUpNetworks::MapAggregatedID2ItsRollupID (const GUID& netID) const -> GUID
{
    if (auto r = fMapAggregatedNetID2RollupID_.Lookup (netID)) {
        return *r;
    }
    // shouldn't get past here - debug if/why this hapepns - see comments below
    DbgTrace ("MapAggregatedID2ItsRollupID failed to find netID={}"_f, netID);
    if constexpr (qDebug) {
        for ([[maybe_unused]] const auto& i : fRolledUpNetworks_) {
            DbgTrace ("rolledupNet={}"_f, i);
        }
    }
    Assert (false); // @todo fix - because we guarantee each item rolled up exactly once - but happens sometimes on change of network - I think due to outdated device records referring to newer network not yet in this cache...
    WeakAssert (false); // @todo fix - because we guarantee each item rolled up exactly once - but happens sometimes on change of network - I think due to outdated device records referring to newer network not yet in this cache...
    return netID;
}

void RolledUpNetworks::MergeIn (DBAccess::Mgr* dbAccessMgr, const Iterable<Network>& nets2MergeIn)
{
    fRawNetworks_ += nets2MergeIn;
    bool anyFailed = false;
    for (const Network& n : nets2MergeIn) {
        if (MergeIn_ (dbAccessMgr, n) == PassFailType_::eFail) {
            anyFailed = true;
            break;
        }
    }
    if (anyFailed) {
        RecomputeAll_ (dbAccessMgr);
    }
}

RolledUpNetworks RolledUpNetworks::GetCached (DBAccess::Mgr* dbAccessMgr, Time::DurationSeconds allowedStaleness)
{
    RequireNotNull (dbAccessMgr);
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs ("...RolledUpNetworks::GetCached")};
    Debug::TimingTrace        ttrc{L"RolledUpNetworks::GetCached", 1s};
    // SynchronizedCallerStalenessCache object just assures one rollup RUNS internally at a time, and
    // that two calls in rapid succession, the second call re-uses the previous value
    static Cache::SynchronizedCallerStalenessCache<void, RolledUpNetworks> sCache_;
    // Disable fHoldWriteLockDuringCacheFill due to https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/23
    // See also
    //      https://stroika.atlassian.net/browse/STK-906 - possible enhancement to this configuration to work better avoiding
    //      See https://stroika.atlassian.net/browse/STK-907 - about needing some new mechanism in Stroika for deadlock detection/avoidance.
    // sCache_.fHoldWriteLockDuringCacheFill = true; // so only one call to filler lambda at a time
    return sCache_.LookupValue (sCache_.Ago (allowedStaleness), [allowedStaleness, dbAccessMgr] () -> RolledUpNetworks {
        /*
         *  DEADLOCK NOTE
         *      Since this can be called while rolling up DEVICES, its important that this code not call anything involving device rollup since
         *      that could trigger a deadlock.
         */
        Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs ("...RolledUpNetworks::GetCached...cachefiller")};
        Debug::TimingTrace        ttrc{L"RolledUpNetworks::GetCached...cachefiller", 1s};

        // Start with the existing rolled up objects
        // and merge in any more recent discovery changes
        RolledUpNetworks result = [allowedStaleness, dbAccessMgr] () {
            auto rolledUpNetworkInterfacess = RolledUpNetworkInterfaces::GetCached (dbAccessMgr, allowedStaleness * 3.0); // longer allowedStaleness cuz we dont care much about this and the parts
                // we look at really dont change
            auto lk = sRolledUpNetworks_.rwget ();
            if (not lk.cref ().has_value ()) {
                dbAccessMgr->CheckDatabaseLoadCompleted ();
                // @todo add more stuff here - empty preset rules from DB
                // merge two tables - ID to fingerprint and user settings tables and store those in this rollup early
                // maybe make CTOR for rolledupnetworks take in ital DB netwworks and rules, and have copyis CTOR taking orig networks and new rules?
                lk.store (RolledUpNetworks{dbAccessMgr, dbAccessMgr->GetRawNetworks (), dbAccessMgr->GetNetworkUserSettings (), rolledUpNetworkInterfacess});
            }
            return Memory::ValueOf (lk.load ());
        }();
        result.MergeIn (dbAccessMgr, IntegratedModel::Private_::FromDiscovery::GetNetworks ());
        sRolledUpNetworks_.store (result); // save here so we can update rollup networks instead of creating anew each time
        return result;
    });
}

void RolledUpNetworks::InvalidateCache (DBAccess::Mgr* dbAccessMgr)
{
    RequireNotNull (dbAccessMgr);
    auto lk = sRolledUpNetworks_.rwget ();
    if (lk->has_value ()) {
        lk.rwref ()->ResetUserOverrides (dbAccessMgr, dbAccessMgr->GetNetworkUserSettings ());
    }
    // else OK if not yet loaded, nothing to invalidate
}

auto RolledUpNetworks::MergeIn_ (DBAccess::Mgr* dbAccessMgr, const Network& net2MergeIn) -> PassFailType_
{
    RequireNotNull (dbAccessMgr);
    // @todo https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/75 - fix corner case
    Network::FingerprintType net2MergeInFingerprint      = net2MergeIn.GenerateFingerprintFromProperties ();
    const auto [oShouldRollIntoNet, shouldInvalidateAll] = ShouldRollupInto_ (net2MergeIn, net2MergeInFingerprint);
    if (shouldInvalidateAll == PassFailType_::ePass) {
        if (oShouldRollIntoNet) {
            // then we rollup to this same rollup network - very common case - so just re-rollup, and we are done
            AddUpdateIn_ (*oShouldRollIntoNet, net2MergeIn, net2MergeInFingerprint);
        }
        else {
            AddNewIn_ (dbAccessMgr, net2MergeIn, net2MergeInFingerprint);
        }
        return PassFailType_::ePass;
    }
    return PassFailType_::eFail;
}

auto RolledUpNetworks::ShouldRollupInto_ (const Network& net2MergeIn, const Network::FingerprintType& net2MergeInFingerprint)
    -> tuple<optional<Network>, PassFailType_>
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

bool RolledUpNetworks::ShouldRollupInto_CheckIsCompatibleWithTarget_ (const Network& net2MergeIn, const Network::FingerprintType& net2MergeInFingerprint,
                                                                      const Network& targetRollup)
{
    if (auto riu = targetRollup.fUserOverrides) {
        if (riu->fAggregateFingerprints and riu->fAggregateFingerprints->Contains (net2MergeInFingerprint)) {
            return true;
        }
        if (riu->fAggregateGatewayHardwareAddresses and riu->fAggregateGatewayHardwareAddresses->Intersects (net2MergeIn.fGatewayHardwareAddresses)) {
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

void RolledUpNetworks::AddUpdateIn_ (const Network& addNet2MergeFromThisRollup, const Network& net2MergeIn, const Network::FingerprintType& net2MergeInFingerprint)
{
    Assert (net2MergeIn.GenerateFingerprintFromProperties () == net2MergeInFingerprint); // provided to avoid cost of recompute
    Network newRolledUpNetwork = Network::Rollup (addNet2MergeFromThisRollup, net2MergeIn);
    newRolledUpNetwork.fAttachedInterfaces += fUseNetworkInterfaceRollups.MapAggregatedNetInterfaceID2ItsRollupID (net2MergeIn.fAttachedInterfaces);
    Assert (addNet2MergeFromThisRollup.fAggregatesFingerprints == newRolledUpNetwork.fAggregatesFingerprints); // spot check - should be same...
    fRolledUpNetworks_.Add (newRolledUpNetwork);
    fMapAggregatedNetID2RollupID_.Add (net2MergeIn.fID, newRolledUpNetwork.fID);
    fMapFingerprint2RollupID.Add (net2MergeInFingerprint, newRolledUpNetwork.fID);
}

void RolledUpNetworks::AddNewIn_ (DBAccess::Mgr* dbAccessMgr, const Network& net2MergeIn, const Network::FingerprintType& net2MergeInFingerprint)
{
    RequireNotNull (dbAccessMgr);
    Assert (net2MergeIn.GenerateFingerprintFromProperties () == net2MergeInFingerprint); // provided to avoid cost of recompute
    Network newRolledUpNetwork = net2MergeIn;
    newRolledUpNetwork.fAttachedInterfaces = fUseNetworkInterfaceRollups.MapAggregatedNetInterfaceID2ItsRollupID (net2MergeIn.fAttachedInterfaces);
    newRolledUpNetwork.fAggregatesReversibly   = Set<GUID>{net2MergeIn.fID};
    newRolledUpNetwork.fAggregatesFingerprints = Set<Network::FingerprintType>{net2MergeInFingerprint};
    // @todo fix this code so each time through we UPDATE sDBAccessMgr_ with latest 'fingerprint' of each dynamic network
    newRolledUpNetwork.fID = dbAccessMgr->GenNewNetworkID (newRolledUpNetwork, net2MergeIn);
    if (fRolledUpNetworks_.Contains (newRolledUpNetwork.fID)) {
        // Should probably never happen, but since depends on data in database, program defensively

        // at this point we have a net2MergeIn that said 'no' to ShouldRollup to all existing networks we've rolled up before
        // and yet somehow, result contains a network that used our ID?
        auto shouldntRollUpButTookOurIDNet = Memory::ValueOf (fRolledUpNetworks_.Lookup (newRolledUpNetwork.fID));
        DbgTrace ("shouldntRollUpButTookOurIDNet={}"_f, shouldntRollUpButTookOurIDNet);
        DbgTrace (L"net2MergeIn={}"_f, net2MergeIn);
        //Assert (not ShouldRollup_ (shouldntRollUpButTookOurIDNet, net2MergeIn));
        Logger::sThe.Log (Logger::eWarning, "Got rollup network ID from cache that is already in use: {} (for external address {})"_f,
                          newRolledUpNetwork.fID, newRolledUpNetwork.fExternalAddresses);
        newRolledUpNetwork.fID = GUID::GenerateNew ();
    }
    newRolledUpNetwork.fUserOverrides = dbAccessMgr->LookupNetworkUserSettings (newRolledUpNetwork.fID);
    if (newRolledUpNetwork.fUserOverrides && newRolledUpNetwork.fUserOverrides->fName) {
        newRolledUpNetwork.fNames.Add (*newRolledUpNetwork.fUserOverrides->fName, 500);
    }
    fRolledUpNetworks_.Add (newRolledUpNetwork);
    fMapAggregatedNetID2RollupID_.Add (net2MergeIn.fID, newRolledUpNetwork.fID);

    // is this guarnateed unique?
    fMapFingerprint2RollupID.Add (net2MergeInFingerprint, newRolledUpNetwork.fID);
}

void RolledUpNetworks::RecomputeAll_ (DBAccess::Mgr* dbAccessMgr)
{
    Debug::TraceContextBumper ctx{"{}...RolledUpNetworks::RecomputeAll_"};
    RequireNotNull (dbAccessMgr);
    fRolledUpNetworks_.clear ();
    fMapAggregatedNetID2RollupID_.clear ();
    fMapFingerprint2RollupID.clear ();
    fRolledUpNetworks_ += fStarterRollups_;
    fRawNetworks_.Apply ([this, dbAccessMgr] (const Network& n) {
        if (MergeIn_ (dbAccessMgr, n) == PassFailType_::eFail) {
            AddNewIn_ (dbAccessMgr, n, n.GenerateFingerprintFromProperties ());
        }
    });
}