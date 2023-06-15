/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

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
#include "Private_/RolledUpDevices.h"
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
    struct MyDBAccessRep_ : IntegratedModel::Private_::DBAccess::Mgr {
        using inherited = IntegratedModel::Private_::DBAccess::Mgr;
        atomic<bool> fFinishedInitialDBLoad_{false};
        MyDBAccessRep_ () { _StartBackgroundThread (); }
        virtual void CheckDatabaseLoadCompleted () override
        {
            if (not fFinishedInitialDBLoad_) {
                // Design Choice - could return non-standardized rollup IDs if DB not loaded, but then those IDs would
                // disappear later in the run, leading to possible client confusion. Best to just not say anything til DB loaded
                // Could ALSO do 2 stage DB load - critical stuff for IDs, and the detailed DB records. All we need is first
                // stage for here...
                static const auto kException_ = HTTP::Exception{HTTP::StatusCodes::kServiceUnavailable, L"Database initialization not yet completed"_k};
                Execution::Throw (kException_);
            }
        }
        virtual void _OneTimeStartupLoadDB () override
        {
            Debug::TraceContextBumper ctx{L"MyDBAccessRep_::_OneTimeStartupLoadDB"};
            inherited::_OneTimeStartupLoadDB ();
            Logger::sThe.Log (Logger::eInfo, L"Loaded %d network interface snapshots, %d network snapshots and %d device snapshots from databas",
                              GetRawNetworkInterfaces ().size (), GetRawNetworks ().size (), GetRawDevices ().size ());
            // @todo post-procesing, maybe deleting some user settings
            PruneBadNetworks_ ();
            Logger::sThe.Log (Logger::eInfo, L"Successully post-processed database");
            fFinishedInitialDBLoad_ = true;
        }
        void PruneBadNetworks_ ()
        {
            Debug::TraceContextBumper ctx{L"MyDBAccessRep_::PruneBadNetworks_"};
            using namespace IntegratedModel::Private_;
            try {
                Mapping<GUID, Network::UserOverridesType> netUserSettings = GetNetworkUserSettings ();
                RolledUpNetworkInterfaces                 tmpNetInterfacerollups{this->GetRawDevices (), this->GetRawNetworkInterfaces ()};
                RolledUpNetworks tmpNetworkRollup{this, this->GetRawNetworks (), netUserSettings, tmpNetInterfacerollups};
                auto             isBad = [&] (const KeyValuePair<GUID, Network::UserOverridesType> kvp) {
                    // @todo check for bad and remove
                    // See if it has BOTH zero concrete networks inside, and is not referenced by any devices
                    try {
                        Network nw = Memory::ValueOf (tmpNetworkRollup.GetNetworks ().Lookup (kvp.fKey));
                        size_t  aggCnt{};
                        if (nw.fAggregatesIrreversibly) {
                            aggCnt += nw.fAggregatesIrreversibly->size ();
                        }
                        if (nw.fAggregatesReversibly) {
                            aggCnt += nw.fAggregatesReversibly->size ();
                        }
                        if (aggCnt == 0) {
                            // Not sure if this is ever legit
                            DbgTrace (L"Found net to remove: %s", Characters::ToString (kvp.fKey).c_str ());
                            return true;
                        }
                        return false;
                    }
                    catch (...) {
                        AssertNotReached ();
                        return true;
                    }
                };
                Set<GUID> toRemove;
                for (auto i : netUserSettings) {
                    if (isBad (i)) {
                        toRemove += i.fKey;
                    }
                }
                if (not toRemove.empty ()) {
                    Logger::sThe.Log (Logger::eWarning, L"Networks marked for purge: %s", Characters::ToString (toRemove).c_str ());
                    for (auto i : toRemove) {
                        this->SetNetworkUserSettings (i, nullopt);
                    }
                }
            }
            catch (...) {
                AssertNotReached (); // would be bad...
            }
        }
        bool GetFinishedInitialDBLoad () const { return fFinishedInitialDBLoad_; }
    };
    unique_ptr<MyDBAccessRep_> sDBAccessMgr_; // constructed on module activation
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

        using IntegratedModel::Private_::RolledUpDevices;
        using IntegratedModel::Private_::RolledUpNetworkInterfaces;
        using IntegratedModel::Private_::RolledUpNetworks;

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
    Require (sDBAccessMgr_ == nullptr);
    sDBAccessMgr_ = make_unique<MyDBAccessRep_> ();
}

IntegratedModel::Mgr::Activator::~Activator ()
{
    Debug::TraceContextBumper ctx{L"IntegratedModel::Mgr::Activator::~Activator"};
    Execution::Thread::SuppressInterruptionInContext suppressInterruption; // must complete this abort and wait for done - this cannot abort/throw
    sDBAccessMgr_.reset ();
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
    return Sequence<IntegratedModel::Device>{RollupSummary_::RolledUpDevices::GetCached (sDBAccessMgr_.get ()).GetDevices ()};
}

optional<IntegratedModel::Device> IntegratedModel::Mgr::GetDevice (const GUID& id, optional<Duration>* ttl) const
{
    // first check rolled up devices, and then raw/unrolled up devices
    // NOTE - this doesn't check the 'dynamic' copy of the devices - it waits til those get migrated to the DB, once ever
    // 30 seconds roughtly...
    auto devicesRollupCache = RollupSummary_::RolledUpDevices::GetCached (sDBAccessMgr_.get ());
    auto result             = devicesRollupCache.GetDevices ().Lookup (id);
    if (result) {
        if (ttl != nullptr) {
            bool justStarted = Time::GetTickCount () < 60; // if just started, this trick of looking at EverSeen() doesn't work (cuz maybe just not discovered yet)
            auto everSeen = result->fSeen.EverSeen ();
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
        if ((result = sDBAccessMgr_->GetRawDevices ().Lookup (id)) != nullopt) {
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
        RollupSummary_::RolledUpDevices::InvalidateCache (sDBAccessMgr_.get ());
    }
}

std::optional<GUID> IntegratedModel::Mgr::GetCorrespondingDynamicDeviceID (const GUID& id) const
{
    Set<GUID> dynamicDevices{Discovery::DevicesMgr::sThe.GetActiveDevices ().Map<GUID, Set<GUID>> ([] (const auto& d) { return d.fGUID; })};
    if (dynamicDevices.Contains (id)) {
        return id;
    }
    auto thisRolledUpDevice = RollupSummary_::RolledUpDevices::GetCached (sDBAccessMgr_.get ()).GetDevices ().Lookup (id);
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
    return Sequence<IntegratedModel::Network>{RollupSummary_::RolledUpNetworks::GetCached (sDBAccessMgr_.get ()).GetNetworks ()};
}

optional<IntegratedModel::Network> IntegratedModel::Mgr::GetNetwork (const GUID& id, optional<Duration>* ttl) const
{
    // first check rolled up networks, and then raw/unrolled up networks
    auto networkRollupsCache = RollupSummary_::RolledUpNetworks::GetCached (sDBAccessMgr_.get ());
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
        RollupSummary_::RolledUpNetworks::InvalidateCache (sDBAccessMgr_.get ());
    }
}

Collection<IntegratedModel::NetworkInterface> IntegratedModel::Mgr::GetNetworkInterfaces () const
{
    // AS OF 2022-11-02 this returns the currently active network interfaces, but changed to mimic other accessors (rollups returned)
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"IntegratedModel::Mgr::GetNetworkInterfaces")};
    Debug::TimingTrace        ttrc{L"IntegratedModel::Mgr::GetNetworkInterfaces", 0.1};
    return Collection<IntegratedModel::NetworkInterface>{
        RollupSummary_::RolledUpNetworkInterfaces::GetCached (sDBAccessMgr_.get ()).GetNetworkInterfacess ()};
}

optional<IntegratedModel::NetworkInterface> IntegratedModel::Mgr::GetNetworkInterface (const GUID& id, optional<Duration>* ttl) const
{
    // AS OF 2022-11-02 this returned the currently active network interfaces, but changed (2022-11-02) to mimic other accessors (rollups returned by default then raw records)
    auto networkInterfacesCache = RollupSummary_::RolledUpNetworkInterfaces::GetCached (sDBAccessMgr_.get ());
    auto result                 = networkInterfacesCache.GetNetworkInterfacess ().Lookup (id);
    if (result) {
        if (ttl != nullptr) {
            *ttl = kTTLForRollupsReturned_;
        }
        auto deviceRollupCache = RollupSummary_::RolledUpDevices::GetCached (sDBAccessMgr_.get ());
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