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

#include "RolledUpNetworkInterfaces.h"

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

using IntegratedModel::Private_::RolledUpNetworkInterfaces;


/*
 ********************************************************************************
 ************** IntegratedModel::Private_::RolledUpNetworkInterfaces ************
 ********************************************************************************
 */
RolledUpNetworkInterfaces::RolledUpNetworkInterfaces (const Iterable<Device>& devices, const Iterable<NetworkInterface>& nets2MergeIn)
{
    Set<GUID>                  netIDs2Add = nets2MergeIn.Map<Set<GUID>> ([] (const auto& i) { return i.fID; });
    Set<GUID>                  netsAdded;
    NetworkInterfaceCollection nets2MergeInCollected{nets2MergeIn};
    for (const Device& d : devices) {
        if (d.fAttachedNetworkInterfaces) {
            d.fAttachedNetworkInterfaces->Apply ([&] (const GUID& netInterfaceID) {
                netsAdded.Add (netInterfaceID);
                MergeIn_ (d.fID, Memory::ValueOf (nets2MergeInCollected.Lookup (netInterfaceID)));
            });
        }
    }
    // https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/80 - could avoid this maybe??? and the bookkeeping above to compute this list...
    DbgTrace ("orphaned interface cnt {}"_f, (netIDs2Add - netsAdded).size ()); // We (temporarily) store network interfaces not associated with any device - if they are not interesting.
                                                                                // OR, could come from just bad data in database
    // Either way, just track them, and don't worry for now --LGP 2022-12-03
    for (const auto& netInterfaceWithoutDevice : (netIDs2Add - netsAdded)) {
        MergeIn_ (nullopt, Memory::ValueOf (nets2MergeInCollected.Lookup (netInterfaceWithoutDevice)));
    }
}

void RolledUpNetworkInterfaces::MergeIn_ (const optional<GUID>& forDeviceID, const Iterable<NetworkInterface>& netInterfaces2MergeIn)
{
    netInterfaces2MergeIn.Apply ([this, forDeviceID] (const NetworkInterface& n) { MergeIn_ (forDeviceID, n); });
}

nonvirtual Set<GUID> RolledUpNetworkInterfaces::GetConcreteIDsForRollup (const GUID& rollupID) const
{
    Set<GUID>        result;
    NetworkInterface rolledUpNI = Memory::ValueOf (fRolledUpNetworkInterfaces_.Lookup (rollupID));
    if (rolledUpNI.fAggregatesReversibly) {
        result = *rolledUpNI.fAggregatesReversibly;
    }
    if (rolledUpNI.fAggregatesIrreversibly) {
        result += *rolledUpNI.fAggregatesIrreversibly;
    }
    return result;
}

nonvirtual auto RolledUpNetworkInterfaces::MapAggregatedID2ItsRollupID (const GUID& aggregatedNetInterfaceID) const -> GUID
{
    if (auto r = fMapAggregatedNetInterfaceID2RollupID_.Lookup (aggregatedNetInterfaceID)) {
        return *r;
    }
    // shouldn't get past here - debug if/why this hapepns - see comments below
    DbgTrace ("MapAggregatedID2ItsRollupID failed to find aggregatedNetInterfaceID={}"_f, aggregatedNetInterfaceID);
    if constexpr (qDebug) {
        for ([[maybe_unused]] const auto& i : fRolledUpNetworkInterfaces_) {
            DbgTrace ("rolledupNetInterface={}"_f, i);
        }
    }
    Assert (false); // @todo fix - because we guarantee each item rolled up exactly once - but happens sometimes on change of network - I think due to outdated device records referring to newer network not yet in this cache...
    WeakAssert (false); // @todo fix - because we guarantee each item rolled up exactly once - but happens sometimes on change of network - I think due to outdated device records referring to newer network not yet in this cache...
    return aggregatedNetInterfaceID;
}

optional<Set<GUID>> RolledUpNetworkInterfaces::GetAttachedToDeviceIDs (const GUID& aggregatedNetworkInterfaceID) const
{
    Set<GUID> r{fAssociateAggregatedNetInterface2OwningDeviceID_.Lookup (aggregatedNetworkInterfaceID)};
    if (r.empty ()) {
        return nullopt;
    }
    return r;
}

NetworkInterfaceCollection RolledUpNetworkInterfaces::GetConcreteNeworkInterfaces (const Set<GUID>& concreteIDs) const
{
    Require (Set<GUID>{fRawNetworkInterfaces_.Keys ()}.ContainsAll (concreteIDs));
    return fRawNetworkInterfaces_.Where ([&concreteIDs] (const auto& i) { return concreteIDs.Contains (i.fID); });
}

RolledUpNetworkInterfaces RolledUpNetworkInterfaces::GetCached (DBAccess::Mgr* dbAccessMgr, Time::DurationSeconds allowedStaleness)
{
    RequireNotNull (dbAccessMgr);
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...RolledUpNetworkInterfaces::GetCached")};
    Debug::TimingTrace        ttrc{L"RolledUpNetworkInterfaces::GetCached", 1s};
    // SynchronizedCallerStalenessCache object just assures one rollup RUNS internally at a time, and
    // that two calls in rapid succession, the second call re-uses the previous value
    static Cache::SynchronizedCallerStalenessCache<void, RolledUpNetworkInterfaces> sCache_;
    // Disable fHoldWriteLockDuringCacheFill due to https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/23
    // See also
    //      https://stroika.atlassian.net/browse/STK-906 - possible enhancement to this configuration to work better avoiding
    //      See https://stroika.atlassian.net/browse/STK-907 - about needing some new mechanism in Stroika for deadlock detection/avoidance.
    // sCache_.fHoldWriteLockDuringCacheFill = true; // so only one call to filler lambda at a time
    return sCache_.LookupValue (sCache_.Ago (allowedStaleness), [dbAccessMgr] () -> RolledUpNetworkInterfaces {
        /*
         *  DEADLOCK NOTE
         *      Since this can be called while rolling up DEVICES, its important that this code not call anything involving device rollup since
         *      that could trigger a deadlock.
         */
        Debug::TraceContextBumper ctx{
            Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...RolledUpNetworkInterfaces::GetCached...cachefiller")};
        Debug::TimingTrace ttrc{L"RolledUpNetworkInterfaces::GetCached...cachefiller", 1s};

        // Start with the existing rolled up objects
        // and merge in any more recent discovery changes
        RolledUpNetworkInterfaces result = [dbAccessMgr] () {
            auto lk = sRolledUpNetworksInterfaces_.rwget ();
            if (not lk.cref ().has_value ()) {
                dbAccessMgr->CheckDatabaseLoadCompleted ();
                // @todo add more stuff here - empty preset rules from DB
                // merge two tables - ID to fingerprint and user settings tables and store those in this rollup early
                // maybe make CTOR for rolledupnetworks take in ital DB netwworks and rules, and have copyis CTOR taking orig networks and new rules?
                RolledUpNetworkInterfaces rollup = RolledUpNetworkInterfaces{dbAccessMgr->GetRawDevices (), dbAccessMgr->GetRawNetworkInterfaces ()};
                // handle orphaned network interfaces
                {
                    auto orphanedRawInterfaces =
                        rollup.GetRawNetworkInterfaces ().Where ([&] (auto ni) { return rollup.GetAttachedToDeviceIDs (ni.fID) == nullopt; });
                    if (not orphanedRawInterfaces.empty ()) {
                        DbgTrace ("Found: orphanedRawInterfaces={}"_f, orphanedRawInterfaces);
                        // https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/80
                        // remove from DB, and re-run...
                        // AND/OR see if found in NETWORK objects...
                        // We (temporarily) store network interfaces not associated with any device - if they are not interesting.
                        // OR, could come from just bad data in database
                        // Either way, just track them, and don't worry for now --LGP 2022-12-03
                        // Find a better place/process to handle this, but not important...
                        // NOTE - we OMIT
                    }
                }
                lk.store (rollup);
            }
            return Memory::ValueOf (lk.load ());
        }();
        // not sure we want to allow this? @todo consider throwing here or asserting out cuz nets rollup IDs would change after this
        result.MergeIn_ (FromDiscovery::GetMyDeviceID (), FromDiscovery::GetNetworkInterfaces ());
        sRolledUpNetworksInterfaces_.store (result); // save here so we can update rollup networks instead of creating anew each time
        return result;
    });
}

void RolledUpNetworkInterfaces::MergeIn_ (const optional<GUID>& forDeviceID, const NetworkInterface& net2MergeIn)
{
    fRawNetworkInterfaces_ += net2MergeIn;
    // @todo same FAIL logic we have in Network objects needed here
    // friendly name - for example - of network interface can change while running, so must be able to invalidate and recompute this list

    Network::FingerprintType netInterface2MergeInFingerprint = net2MergeIn.GenerateFingerprintFromProperties ();
    auto rolledUpNetworkInterace = NetworkInterface::Rollup (fRolledUpNetworkInterfaces_.Lookup (netInterface2MergeInFingerprint), net2MergeIn);
    fRolledUpNetworkInterfaces_.Add (rolledUpNetworkInterace);
    fMapAggregatedNetInterfaceID2RollupID_.Add (net2MergeIn.fID, rolledUpNetworkInterace.fID);
    if (forDeviceID) {
        fAssociateAggregatedNetInterface2OwningDeviceID_.Add (net2MergeIn.fID, *forDeviceID);
    }
}