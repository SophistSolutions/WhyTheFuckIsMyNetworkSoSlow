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

#include "RolledUpDevices.h"

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

using IntegratedModel::Private_::RolledUpDevices;

/*
 ********************************************************************************
 ******************** IntegratedModel::Private_::RolledUpDevices ****************
 ********************************************************************************
 */

RolledUpDevices::RolledUpDevices (DBAccess::Mgr* dbAccessMgr, const Iterable<Device>& devices2MergeIn, const Mapping<GUID, Device::UserOverridesType>& userOverrides,
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
    MergeIn (dbAccessMgr, devices2MergeIn);
}

auto RolledUpDevices::MapAggregatedID2ItsRollupID (const GUID& aggregatedDeviceID) const -> GUID
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

void RolledUpDevices::ResetUserOverrides (DBAccess::Mgr* dbAccessMgr, const Mapping<GUID, Device::UserOverridesType>& userOverrides)
{
    RequireNotNull (dbAccessMgr);
    RolledUpNetworkInterfaces networkInterfacesRollup = RolledUpNetworkInterfaces::GetCached (dbAccessMgr);
    fStarterRollups_                                  = userOverrides.Map<Device> ([] (const auto& guid2UOTPair) -> Device {
        Device d;
        d.fID            = guid2UOTPair.fKey;
        d.fUserOverrides = guid2UOTPair.fValue;
        if (d.fUserOverrides and d.fUserOverrides->fName) {
            d.fNames.Add (*d.fUserOverrides->fName, 500);
        }
        return d;
    });
    RecomputeAll_ (dbAccessMgr);
}

void RolledUpDevices::MergeIn (DBAccess::Mgr* dbAccessMgr, const Iterable<Device>& devices2MergeIn)
{
    RequireNotNull (dbAccessMgr);
    fRawDevices_ += devices2MergeIn;
    bool anyFailed = false;
    for (const Device& d : devices2MergeIn) {
        if (MergeIn_ (dbAccessMgr, d) == PassFailType_::eFail) {
            anyFailed = true;
            break;
        }
    }
    if (anyFailed) {
        RecomputeAll_ (dbAccessMgr);
    }
}

RolledUpDevices RolledUpDevices::GetCached (DBAccess::Mgr* dbAccessMgr, Time::DurationSecondsType allowedStaleness)
{
    RequireNotNull (dbAccessMgr);
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
    return sCache_.LookupValue (sCache_.Ago (allowedStaleness), [dbAccessMgr, allowedStaleness] () -> RolledUpDevices {
        Debug::TraceContextBumper ctx{
            Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...RolledUpDevices::GetCached...cachefiller")};
        Debug::TimingTrace ttrc{L"RolledUpDevices::GetCached...cachefiller", 1};

        auto rolledUpNetworks = RolledUpNetworks::GetCached (dbAccessMgr, allowedStaleness * 3.0);                    // longer allowedStaleness cuz we dont care much about this and the parts
                                                                                                                      // we look at really dont change
        auto rolledUpNetworkInterfacess = RolledUpNetworkInterfaces::GetCached (dbAccessMgr, allowedStaleness * 3.0); // longer allowedStaleness cuz we dont care much about this and the parts
            // we look at really dont change

        // Start with the existing rolled up objects
        // and merge in any more recent discovery changes
        RolledUpDevices result = [&] () {
            auto lk = sRolledUpDevicesSoFar_.rwget ();
            if (not lk.cref ().has_value ()) {
                if (not dbAccessMgr->GetFinishedInitialDBLoad ()) {
                    // Design Choice - could return non-standardized rollup IDs if DB not loaded, but then those IDs would
                    // disappear later in the run, leading to possible client confusion. Best to just not say anything til DB loaded
                    // Could ALSO do 2 stage DB load - critical stuff for IDs, and the detailed DB records. All we need is first
                    // stage for here...
                    Execution::Throw (HTTP::Exception{HTTP::StatusCodes::kServiceUnavailable, L"Database not yet loaded"_k});
                }
                // @todo add more stuff here - empty preset rules from DB
                // merge two tables - ID to fingerprint and user settings tables and store those in this rollup early
                // maybe make CTOR for rolledupnetworks take in ital DB netwworks and rules, and have copyis CTOR taking orig networks and new rules?
                RolledUpDevices initialDBDevices{dbAccessMgr, dbAccessMgr->GetRawDevices (), dbAccessMgr->GetDeviceUserSettings (),
                                                 rolledUpNetworks, rolledUpNetworkInterfacess};
                lk.store (initialDBDevices);
            }
            return Memory::ValueOf (lk.load ());
        }();
        // not sure we want to allow this? @todo consider throwing here or asserting out cuz nets rollup IDs would change after this
        result.MergeIn (dbAccessMgr, IntegratedModel::Private_::FromDiscovery::GetDevices ());
        sRolledUpDevicesSoFar_.store (result); // save here so we can update rollup networks instead of creating anew each time
        return result;
    });
}

void RolledUpDevices::InvalidateCache (DBAccess::Mgr* dbAccessMgr)
{
    RequireNotNull (dbAccessMgr);
    auto lk = sRolledUpDevicesSoFar_.rwget ();
    if (lk->has_value ()) {
        lk.rwref ()->ResetUserOverrides (dbAccessMgr, dbAccessMgr->GetDeviceUserSettings ());
    }
    // else OK if not yet loaded, nothing to invalidate
}

auto RolledUpDevices::MergeIn_ (DBAccess::Mgr* dbAccessMgr, const Device& d2MergeIn) -> PassFailType_
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
            MergeInNew_ (dbAccessMgr, d2MergeIn);
        }
        return PassFailType_::ePass;
    }
}

void RolledUpDevices::MergeInUpdate_ (const Device& rollupDevice, const Device& newDevice2MergeIn)
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

void RolledUpDevices::MergeInNew_ (DBAccess::Mgr* dbAccessMgr, const Device& d2MergeIn)
{
    RequireNotNull (dbAccessMgr);
    Assert (not d2MergeIn.fAggregatesReversibly.has_value ());
    Device newRolledUpDevice                = d2MergeIn;
    newRolledUpDevice.fAggregatesReversibly = Set<GUID>{d2MergeIn.fID};
    newRolledUpDevice.fID                   = dbAccessMgr->GenNewDeviceID (d2MergeIn.GetHardwareAddresses ());
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
    newRolledUpDevice.fUserOverrides = dbAccessMgr->LookupDevicesUserSettings (newRolledUpDevice.fID);
    if (newRolledUpDevice.fUserOverrides && newRolledUpDevice.fUserOverrides->fName) {
        newRolledUpDevice.fNames.Add (*newRolledUpDevice.fUserOverrides->fName, 500);
    }
    fRolledUpDevices += newRolledUpDevice;
    fRaw2RollupIDMap_.Add (d2MergeIn.fID, newRolledUpDevice.fID);
}

void RolledUpDevices::RecomputeAll_ (DBAccess::Mgr* dbAccessMgr)
{
    RequireNotNull (dbAccessMgr);
    fRolledUpDevices.clear ();
    fRaw2RollupIDMap_.clear ();
    fRolledUpDevices += fStarterRollups_;
    for (const auto& di : fRawDevices_) {
        if (MergeIn_ (dbAccessMgr, di) == PassFailType_::eFail) {
            Assert (false); //nyi - or maybe just bug since we have mapping so device goes into two different rollups?
        }
    }
}

auto RolledUpDevices::MapAggregatedAttachments2Rollups_ (const Mapping<GUID, NetworkAttachmentInfo>& nats) -> Mapping<GUID, NetworkAttachmentInfo>
{
    Mapping<GUID, NetworkAttachmentInfo> result;
    for (const auto& ni : nats) {
        result.Add (fUseRolledUpNetworks.MapAggregatedID2ItsRollupID (ni.fKey), ni.fValue);
    }
    return result;
};

bool RolledUpDevices::ShouldRollup_ (const Device& exisingRolledUpDevice, const Device& d2PotentiallyMergeIn)
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
