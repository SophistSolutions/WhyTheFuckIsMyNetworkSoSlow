/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Common/GUID.h"
#include "Stroika/Foundation/Common/KeyValuePair.h"
#include "Stroika/Foundation/Containers/KeyedCollection.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/Database/Document/LocalDocumentDB.h"
#include "Stroika/Foundation/Database/Document/ObjectCollection.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Execution/Logger.h"
#include "Stroika/Foundation/IO/FileSystem/WellKnownLocations.h"

#include "../../Common/AppConfiguration.h"
#include "../../Common/BLOBMgr.h"
#include "../../Common/EthernetMACAddressOUIPrefixes.h"
#include "../../Discovery/Devices.h"
#include "../../Discovery/NetworkInterfaces.h"
#include "../../Discovery/Networks.h"

#include "FromDiscovery.h"

#include "DBAccess.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Common;
using namespace Stroika::Foundation::DataExchange;
using namespace Stroika::Foundation::Database;
using namespace Stroika::Foundation::Execution;
using namespace Stroika::Foundation::Memory;
using namespace Stroika::Foundation::IO::Network;
using namespace Stroika::Foundation::IO::Network::HTTP;
using namespace Stroika::Foundation::Traversal;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;

using Stroika::Foundation::Common::GUID;

using WebServices::Model::Device;
using WebServices::Model::DeviceCollection;
using WebServices::Model::Network;
using WebServices::Model::NetworkAttachmentInfo;
using WebServices::Model::NetworkCollection;
using WebServices::Model::NetworkInterface;
using WebServices::Model::NetworkInterfaceCollection;

using IntegratedModel::Private_::DBAccess::Mgr;

/*
 ********************************************************************************
 ****************** IntegratedModel::Private_::DBAccess::Mgr ********************
 ********************************************************************************
 */
const LazyInitialized<ObjectVariantMapper> Mgr::kDBObjectMapper_{[] () {
    ObjectVariantMapper mapper;

    mapper += NetworkInterface::kMapper;
    mapper += Network::kMapper;
    mapper += Device::kMapper;

    mapper.AddCommonType<Range<DateTime>> (ObjectVariantMapper::RangeSerializerOptions{"lowerBound"sv, "upperBound"sv}); // lower-camel-case names happier in javascript?

    mapper.AddClass<ExternalDeviceUserSettingsElt_> ({
        {"UserSettings"sv, &ExternalDeviceUserSettingsElt_::fUserSettings},
        {"DeviceID"sv, &ExternalDeviceUserSettingsElt_::fDeviceID},
    });
    mapper.AddClass<ExternalNetworkUserSettingsElt_> ({
        {"UserSettings"sv, &ExternalNetworkUserSettingsElt_::fUserSettings},
        {"NetworkID"sv, &ExternalNetworkUserSettingsElt_::fNetworkID},
    });

    return mapper;
}};

Mgr::Mgr ()
{
    Debug::TraceContextBumper ctx{"IntegratedModel::{}::Mgr_::CTOR"};

    auto conn = fDB_.GetInternallySynchronizedConnection (); // a single shared connection-ptr to DB (internally synchronized)

    fDeviceUserSettingsTableConnection_ =
        Document::ObjectCollection::New<ExternalDeviceUserSettingsElt_> (conn.CreateCollection ("DeviceUserSettings"sv), kDBObjectMapper_);
    fNetworkUserSettingsTableConnection_ =
        Document::ObjectCollection::New<ExternalNetworkUserSettingsElt_> (conn.CreateCollection ("NetworkUserSettings"sv), kDBObjectMapper_);
    fDeviceTableConnection_  = Document::ObjectCollection::New<Device> (conn.CreateCollection ("Devices"sv), kDBObjectMapper_);
    fNetworkTableConnection_ = Document::ObjectCollection::New<Network> (conn.CreateCollection ("Networks"sv), kDBObjectMapper_);
    fNetworkInterfaceTableConnection_ =
        Document::ObjectCollection::New<NetworkInterface> (conn.CreateCollection ("NetworkInterfaces"sv), kDBObjectMapper_);

    try {
        Debug::TimingTrace ttrc{"...load of fCachedDeviceUserSettings_ from database ", 1s};
        fCachedDeviceUserSettings_.store (Mapping<GUID, Model::Device::UserOverridesType>{
            fDeviceUserSettingsTableConnection_.rwget ().cref ().GetAll ().Map<Iterable<KeyValuePair<GUID, Model::Device::UserOverridesType>>> (
                [] (const auto& i) { return KeyValuePair<GUID, Model::Device::UserOverridesType>{i.fDeviceID, i.fUserSettings}; })});
    }
    catch (...) {
        Logger::sThe.Log (Logger::eCriticalError, "Failed to load fCachedDeviceUserSettings_ from db: {}"_f, current_exception ());
        Execution::ReThrow ();
    }
    try {
        Debug::TimingTrace ttrc{"...load of fCachedNetworkUserSettings_ from database ", 1s};
        fCachedNetworkUserSettings_.store (
            fNetworkUserSettingsTableConnection_.rwget ().cref ().GetAll ().Map<Mapping<GUID, Model::Network::UserOverridesType>> (
                [] (const auto& i) { return KeyValuePair<GUID, Model::Network::UserOverridesType>{i.fNetworkID, i.fUserSettings}; }));
    }
    catch (...) {
        Logger::sThe.Log (Logger::eCriticalError, "Failed to load fCachedNetworkUserSettings_ from db: {}"_f, current_exception ());
        Execution::ReThrow ();
    }
}

Mgr::~Mgr ()
{
    Debug::TraceContextBumper ctx{"IntegratedModel::{}::Mgr::DTOR"};
    Execution::Thread::SuppressInterruptionInContext suppressInterruption; // must complete this abort and wait for done - this cannot abort/throw
    fDatabaseSyncThread_.AbortAndWaitForDone ();
}

GUID Mgr::GenNewDeviceID (const Set<String>& hwAddresses)
{
    GUID newRes = GUID::GenerateNew ();
    if (hwAddresses.empty ()) {
        WeakAssert (false);
    }
    else {
        Model::Device::UserOverridesType tmp;
        tmp.fAggregateDeviceHardwareAddresses = hwAddresses;
        SetDeviceUserSettings (newRes, tmp);
    }
    return newRes;
}

GUID Mgr::GenNewNetworkID ([[maybe_unused]] const Model::Network& rollupNetwork, const Model::Network& containedNetwork)
{
    Debug::TimingTrace                ttrc{"GenNewNetworkID", 1ms}; // sb very quick
    GUID                              newRes = GUID::GenerateNew ();
    Model::Network::UserOverridesType tmp;
    tmp.fAggregateFingerprints = Set<GUID>{containedNetwork.GenerateFingerprintFromProperties ()};
    /*
     *  Automatically create rules to group 'internal device networks'
     */
    using NetworkInterfaceAggregateRule = Model::Network::UserOverridesType::NetworkInterfaceAggregateRule;
    for (const GUID& i : containedNetwork.fAttachedInterfaces) {
        auto ni = Memory::ValueOf (GetRawNetworkInterfaces ().Lookup (i));
        if (ni.fType == IO::Network::Interface::Type::eDeviceVirtualInternalNetwork) {
            if (tmp.fAggregateNetworkInterfacesMatching == nullopt) {
                tmp.fAggregateNetworkInterfacesMatching = Sequence<NetworkInterfaceAggregateRule>{};
            }
            *tmp.fAggregateNetworkInterfacesMatching += NetworkInterfaceAggregateRule{*ni.fType, ni.GenerateFingerprintFromProperties ()};
        }
    }
    SetNetworkUserSettings (newRes, tmp);
    return newRes;
}

bool Mgr::SetDeviceUserSettings (const GUID& id, const std::optional<Device::UserOverridesType>& settings)
{
    Debug::TimingTrace ttrc{"IntegratedModel ... SetDeviceUserSettings", 100ms};
    // first check if legit id, and then store
    // @todo check if good id and throw if not...
    auto lk = fCachedDeviceUserSettings_.rwget ();
    if (settings) {
        if (fCachedDeviceUserSettings_.cget ().cref ().Lookup (id) != settings) {
            fDeviceUserSettingsTableConnection_.rwget ().cref ().AddOrUpdate (ExternalDeviceUserSettingsElt_{id, *settings});
            fCachedDeviceUserSettings_.rwget ().rwref ().Add (id, *settings);
            return true;
        }
        return false;
    }
    else {
        fDeviceUserSettingsTableConnection_.rwget ().cref ().Remove (id.As<String> ());
        return fCachedDeviceUserSettings_.rwget ().rwref ().RemoveIf (id);
    }
}

bool Mgr::SetNetworkUserSettings (const GUID& id, const std::optional<Network::UserOverridesType>& settings)
{
    Debug::TimingTrace ttrc{"IntegratedModel ... SetNetworkUserSettings", 100ms};
    // first check if legit id, and then store
    // @todo check if good id and throw if not...
    auto lk = fCachedNetworkUserSettings_.rwget ();
    if (settings) {
        if (fCachedNetworkUserSettings_.cget ().cref ().Lookup (id) != settings) {
            fNetworkUserSettingsTableConnection_.rwget ().cref ().AddOrUpdate (ExternalNetworkUserSettingsElt_{id, *settings});
            fCachedNetworkUserSettings_.rwget ().rwref ().Add (id, *settings);
            return true;
        }
        return false;
    }
    else {
        fNetworkUserSettingsTableConnection_.rwget ().cref ().Remove (id.As<String> ());
        return fCachedNetworkUserSettings_.rwget ().rwref ().RemoveIf (id);
    }
}

void Mgr::_StartBackgroundThread ()
{
    Require (fDatabaseSyncThread_ == nullptr);
    fDatabaseSyncThread_ = Thread::New ([this] () { BackgroundDatabaseThread_ (); }, Thread::eAutoStart, "BackgroundDatabaseThread"sv);
}

void Mgr::BackgroundDatabaseThread_ ()
{
    Debug::TraceContextBumper ctx{"BackgroundDatabaseThread_ loop"};
    _OneTimeStartupLoadDB (); // if this fails we fail (has internal retry where appropriate)
    while (true) {
        try {
            // periodically write the latest discovered data to the database
            Debug::TraceContextBumper ctx1{"inner loop"};

            // UPDATE fDBNetworkInterfaces_ INCREMENTALLY to reflect reflect these merges
            FromDiscovery::GetNetworkInterfaces ().Apply ([this] (const Model::NetworkInterface& ni) {
                Assert (ni.fAggregatesReversibly == nullopt); // dont write these summary values
                fNetworkInterfaceTableConnection_.AddOrUpdate (ni);
                fDBNetworkInterfaces_.rwget ()->Add (ni);
            });

            // UPDATE fDBNetworks_ INCREMENTALLY to reflect reflect these merges
            FromDiscovery::GetNetworks ().Apply ([this] (const Model::Network& n) {
                Assert (n.fSeen);                            // don't track/write items which have never been seen
                Assert (n.fAggregatesReversibly == nullopt); // dont write these summary values
                fNetworkTableConnection_.AddOrUpdate (n);
                fDBNetworks_.rwget ()->Add (n);
            });

            // UPDATE fDBDevices_ INCREMENTALLY to reflect reflect these merges
            FromDiscovery::GetDevices ().Apply ([this] (const Model::Device& d) {
                Assert (d.fSeen.EverSeen ());
                Assert (d.fSeen.EverSeen ());                // don't track/write items which have never been seen
                Assert (d.fUserOverrides == nullopt);        // tracked on rollup devices, not snapshot devices
                Assert (d.fAggregatesReversibly == nullopt); // dont write these summary values
                auto rec2Update = fDB_.AddOrMergeUpdate (fDeviceTableConnection_, d);
                fDBDevices_.rwget ()->Add (rec2Update);
            });

            if (auto backupCfg = BackendApp::Common::gAppConfiguration->fBackupData) {
                BackupDB2_ (backupCfg->fFile);
            }

            // only update periodically
            Execution::Sleep (30s);
        }
        catch (const Thread::AbortException&) {
            Execution::ReThrow ();
        }
        catch (...) {
            Logger::sThe.Log (Logger::eWarning, "Database update: ignoring exception in BackgroundDatabaseThread_ loop (will retry in 30 seconds): {}"_f,
                              current_exception ());
            Execution::Sleep (30s);
        }
    }
}

void Mgr::BackupDB2_ (const filesystem::path& backupFile)
{
    Debug::TraceContextBumper ctx{"BackupDB2_"};
    try {
        using namespace Database;
        using namespace Database::Document;

        filesystem::path fullBackupFileName = backupFile;
        if (fullBackupFileName.empty ()) {
            fullBackupFileName = "backup.json"sv;
        }
        if (fullBackupFileName.is_relative ()) {
            fullBackupFileName = IO::FileSystem::WellKnownLocations::GetApplicationData () / "WhyTheFuckIsMyNetworkSoSlow" / fullBackupFileName;
        }

        LocalDocumentDB::Options options{.fInternallySynchronizedLetter = Execution::eInternallySynchronized,
                                         .fStorage                      = LocalDocumentDB::Options::SingleFileStorage{
                                                                  .fFile = fullBackupFileName, .fForceCreateNew = true, .fFlushOnEachWrite = false}};
        auto                     db = LocalDocumentDB::New (options);

        ObjectCollection::Ptr<ExternalDeviceUserSettingsElt_> deviceUserSettings =
            ObjectCollection::New<ExternalDeviceUserSettingsElt_> (db.CreateCollection ("DeviceUserSettings"sv), kDBObjectMapper_);
        ObjectCollection::Ptr<ExternalNetworkUserSettingsElt_> networkUserSettingsTableConnection_ =
            ObjectCollection::New<ExternalNetworkUserSettingsElt_> (db.CreateCollection ("NetworkUserSettings"sv), kDBObjectMapper_);
        ObjectCollection::Ptr<Device> deviceTableConnection_ = ObjectCollection::New<Device> (db.CreateCollection ("Devices"sv), kDBObjectMapper_);
        ObjectCollection::Ptr<Network> networkTableConnection_ = ObjectCollection::New<Network> (db.CreateCollection ("Networks"sv), kDBObjectMapper_);
        ObjectCollection::Ptr<NetworkInterface> networkInterfaceTableConnection_ =
            ObjectCollection::New<NetworkInterface> (db.CreateCollection ("NetworkInterfaces"sv), kDBObjectMapper_);

        fCachedDeviceUserSettings_.load ().Apply ([&] (const KeyValuePair<GUID, Device::UserOverridesType>& i) {
            deviceUserSettings.Add (ExternalDeviceUserSettingsElt_{.fDeviceID = i.fKey, .fUserSettings = i.fValue});
        });
        fCachedNetworkUserSettings_.load ().Apply ([&] (const KeyValuePair<GUID, Network::UserOverridesType>& i) {
            networkUserSettingsTableConnection_.Add (ExternalNetworkUserSettingsElt_{.fNetworkID = i.fKey, .fUserSettings = i.fValue});
        });
        fDBDevices_.load ().Apply ([&] (const Device& i) { deviceTableConnection_.Add (i); });
        fDBNetworks_.load ().Apply ([&] (const Network& i) { networkTableConnection_.Add (i); });
        fDBNetworkInterfaces_.load ().Apply ([&] (const NetworkInterface& i) { networkInterfaceTableConnection_.Add (i); });
        db.Flush ();

        static bool sNotedFilenameOnce_{false};
        if (not sNotedFilenameOnce_) {
            sNotedFilenameOnce_ = true;
            Logger::sThe.Log (Logger::eInfo, "Backed up database to file {}"_f, fullBackupFileName);
        }
    }
    catch (...) {
        Logger::sThe.Log (Logger::eError, "Failed to backup database to file {}: {}"_f, backupFile, current_exception ());
    }
}

void Mgr::_OneTimeStartupLoadDB ()
{
    /*
     *  Considered loading this from CTOR, but then the rest of APP load would be delayed by this DB load. Better to
     *  let what can startup do so, and just block the webservices and things that depend on database load 
     *  elsewhere.
     */
    Debug::TraceContextBumper ctx{"_OneTimeStartupLoadDB"};
    optional<unsigned int>    netInterfaceSnapshotsLoaded{};
    optional<unsigned int>    netSnapshotsLoaded{};
    optional<unsigned int>    deviceSnapshotsLoaded{};

    auto fetchInterfacesNetworks = [this] () -> unsigned int {
        try {
            Debug::TimingTrace ttrc{"...initial load of fDBNetworkInterfaces_ from database ", 1s};
            auto               all = fNetworkInterfaceTableConnection_.GetAll ();
            fDBNetworkInterfaces_.store (NetworkInterfaceCollection{all});
            return static_cast<unsigned int> (all.size ());
        }
        catch (...) {
            Logger::sThe.Log (Logger::eError, "Probably important error reading database of old network interfaces data: {}"_f, current_exception ());
            Execution::ReThrow ();
        }
    };
    auto fetchNets = [this] () -> unsigned int {
        try {
            Debug::TimingTrace ttrc{"...initial load of fDBNetworks_ from database ", 1s};
            auto               all = fNetworkTableConnection_.GetAll ();
            fDBNetworks_.store (NetworkCollection{all});
            return static_cast<unsigned int> (all.size ());
        }
        catch (...) {
            Logger::sThe.Log (Logger::eError, "Probably important error reading database of old networks data: {}"_f, current_exception ());
            Execution::ReThrow ();
        }
    };
    auto fetchDevices = [this] () -> unsigned int {
        try {
            Debug::TimingTrace ttrc{"...initial load of fDBDevices_ from database ", 1s};
            auto               all = fDeviceTableConnection_.GetAll ();
            if constexpr (qDebug) {
                all.Apply ([] ([[maybe_unused]] const Device& d) { Assert (!d.fUserOverrides); }); // tracked on rollup devices, not snapshot devices
            }
            fDBDevices_.store (DeviceCollection{all}); // pre-load in memory copy with whatever we had stored in the database
            return static_cast<unsigned int> (all.size ());
        }
        catch (...) {
            Logger::sThe.Log (Logger::eError, "Probably important error reading database of old device data: {}"_f, current_exception ());
            Execution::ReThrow ();
        }
    };

    // retry in case of failure
    while (true) {
        try {
            // load networks before devices because devices depend on networks but not the reverse
            // each loader local-function succeeds or throws
            if (not netInterfaceSnapshotsLoaded.has_value ()) {
                netInterfaceSnapshotsLoaded = fetchInterfacesNetworks ();
            }
            if (not netSnapshotsLoaded.has_value ()) {
                netSnapshotsLoaded = fetchNets ();
            }
            if (not deviceSnapshotsLoaded.has_value ()) {
                deviceSnapshotsLoaded = fetchDevices ();
            }
            // If we get this far without throwing, we are DONE
            return;
        }
        catch (const Thread::AbortException&) {
            Execution::ReThrow ();
        }
        catch (...) {
            Logger::sThe.Log (Logger::eWarning, "Database error: ignoring exception in OneTimeStartup_ loop (will retry in 10 seconds): {}"_f,
                              current_exception ());
            Execution::Sleep (10s);
        }
    }
}

uintmax_t Mgr::GetDBFileSize () const
{
    return this->fDB_.GetFileSize ();
}
