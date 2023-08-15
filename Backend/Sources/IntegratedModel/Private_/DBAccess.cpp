/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

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

using Stroika::Foundation::Common::ConstantProperty;
using Stroika::Foundation::Common::GUID;

using WebServices::Model::Device;
using WebServices::Model::DeviceCollection;
using WebServices::Model::Network;
using WebServices::Model::NetworkAttachmentInfo;
using WebServices::Model::NetworkCollection;
using WebServices::Model::NetworkInterface;
using WebServices::Model::NetworkInterfaceCollection;

using Schema_Table         = SQL::ORM::Schema::Table;
using Schema_Field         = SQL::ORM::Schema::Field;
using Schema_CatchAllField = SQL::ORM::Schema::CatchAllField;

using IntegratedModel::Private_::DBAccess::Mgr;

/*
 ********************************************************************************
 ****************** IntegratedModel::Private_::DBAccess::Mgr ********************
 ********************************************************************************
 */
const ConstantProperty<ObjectVariantMapper> Mgr::kDBObjectMapper_{[] () {
    ObjectVariantMapper mapper;

    mapper += NetworkInterface::kMapper;
    mapper += Network::kMapper;
    mapper += Device::kMapper;

    mapper.AddCommonType<Range<DateTime>> (ObjectVariantMapper::RangeSerializerOptions{L"lowerBound"sv, L"upperBound"sv}); // lower-camel-case names happier in javascript?

    mapper.AddClass<ExternalDeviceUserSettingsElt_> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"UserSettings", StructFieldMetaInfo{&ExternalDeviceUserSettingsElt_::fUserSettings}},
        {L"DeviceID", StructFieldMetaInfo{&ExternalDeviceUserSettingsElt_::fDeviceID}},
    });
    mapper.AddClass<ExternalNetworkUserSettingsElt_> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"UserSettings", StructFieldMetaInfo{&ExternalNetworkUserSettingsElt_::fUserSettings}},
        {L"NetworkID", StructFieldMetaInfo{&ExternalNetworkUserSettingsElt_::fNetworkID}},
    });

    // ONLY DO THIS FOR WHEN WRITING TO DB -- store GUIDs as BLOBs - at least for database interactions (cuz probably more efficient)
    mapper.AddCommonType<GUID> (BackendApp::Common::DB::kRepresentIDAs_);

    return mapper;
}};

const Schema_Table Mgr::kDeviceUserSettingsSchema_{L"DeviceUserSettings"sv,
                                                   /*
     */
                                                   Collection<Schema_Field>{
#if __cpp_designated_initializers
                                                       {.fName = L"DeviceID"sv, .fRequired = true, .fVariantValueType = kRepresentIDAs_, .fIsKeyField = true},
#else
                                                       {L"DeviceID", nullopt, true, kRepresentIDAs_, nullopt, true},
#endif
                                                   },
                                                   Schema_CatchAllField{}};
const Schema_Table Mgr::kNetworkUserSettingsSchema_{
    L"NetworkUserSettings"sv,
    /*
             */
    Collection<Schema_Field>{
#if __cpp_designated_initializers
        {.fName = L"NetworkID"sv, .fRequired = true, .fVariantValueType = kRepresentIDAs_, .fIsKeyField = true},
#else
        {L"NetworkID", nullopt, true, kRepresentIDAs_, nullopt, true},
#endif
    },
    Schema_CatchAllField{}};

const Schema_Table Mgr::kDeviceTableSchema_{
    L"Devices"sv,
    /*
     *  use the same names as the ObjectVariantMapper for simpler mapping, or specify an alternate name
     *  for ID, just as an example.
     */
    Collection<Schema_Field>{
#if __cpp_designated_initializers
        /**
         *  For ID, generate random GUID (BLOB) automatically in database
         */
        {.fName = L"ID"sv, .fVariantValueName = L"id"sv, .fRequired = true, .fVariantValueType = kRepresentIDAs_, .fIsKeyField = true, .fDefaultExpression = GenRandomIDString_ (kRepresentIDAs_)},
        {.fName = L"name"sv, .fVariantValueType = VariantValue::eString},
#else
        {L"ID", L"id"sv, true, kRepresentIDAs_, nullopt, true, nullopt, GenRandomIDString_ (kRepresentIDAs_)},
        {L"name", nullopt, false, VariantValue::eString},
#endif
    },
    Schema_CatchAllField{}};

const Schema_Table Mgr::kNetworkInterfaceTableSchema_{
    L"NetworkInteraces"sv,
    /*
     *  use the same names as the ObjectVariantMapper for simpler mapping, or specify an alternate name
     *  for ID, just as an example.
     */
    Collection<Schema_Field>{
#if __cpp_designated_initializers
        {.fName = L"ID"sv, .fVariantValueName = L"id"sv, .fRequired = true, .fVariantValueType = kRepresentIDAs_, .fIsKeyField = true},
        {.fName = L"friendlyName"sv, .fVariantValueType = VariantValue::eString},
        {.fName = L"hardwareAddress"sv, .fVariantValueType = VariantValue::eString},
        {.fName = L"type"sv, .fVariantValueType = VariantValue::eString},
#else
        {L"ID", L"id"sv, true, kRepresentIDAs_, nullopt, true, nullopt},
        {L"friendlyName", nullopt, false, VariantValue::eString},
        {L"hardwareAddress", nullopt, false, VariantValue::eString},
        {L"type", nullopt, false, VariantValue::eString},
#endif
    },
    Schema_CatchAllField{}};

const Schema_Table Mgr::kNetworkTableSchema_{
    L"Networks"sv,
    /*
     *  use the same names as the ObjectVariantMapper for simpler mapping, or specify an alternate name
     *  for ID, just as an example.
     */
    Collection<Schema_Field>{
#if __cpp_designated_initializers
        /**
         *  For ID, generate random GUID (BLOB) automatically in database
         */
        {.fName = L"ID"sv, .fVariantValueName = L"id"sv, .fRequired = true, .fVariantValueType = kRepresentIDAs_, .fIsKeyField = true, .fDefaultExpression = GenRandomIDString_ (kRepresentIDAs_)},
        {.fName = L"friendlyName"sv, .fVariantValueType = VariantValue::eString},
#else
        {L"ID", L"id"sv, true, kRepresentIDAs_, nullopt, true, nullopt, GenRandomIDString_ (kRepresentIDAs_)},
        {L"friendlyName", nullopt, false, VariantValue::eString},
#endif
    },
    Schema_CatchAllField{}};

Mgr::Mgr ()
    : fDB_{kCurrentVersion_, Traversal::Iterable<Schema_Table>{kDeviceTableSchema_, kDeviceUserSettingsSchema_, kNetworkTableSchema_,
                                                               kNetworkInterfaceTableSchema_, kNetworkUserSettingsSchema_}}
{
    Debug::TraceContextBumper ctx{L"IntegratedModel::{}::Mgr_::CTOR"};
    fDeviceUserSettingsTableConnection_ = make_unique<SQL::ORM::TableConnection<ExternalDeviceUserSettingsElt_>> (
        fDBConnectionPtr_, kDeviceUserSettingsSchema_, kDBObjectMapper_,
        BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<ExternalDeviceUserSettingsElt_>> ());
    fNetworkUserSettingsTableConnection_ = make_unique<SQL::ORM::TableConnection<ExternalNetworkUserSettingsElt_>> (
        fDBConnectionPtr_, kNetworkUserSettingsSchema_, kDBObjectMapper_,
        BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<ExternalNetworkUserSettingsElt_>> ());
    fDeviceTableConnection_ = make_unique<SQL::ORM::TableConnection<Device>> (
        fDBConnectionPtr_, kDeviceTableSchema_, kDBObjectMapper_,
        BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<Device>> ());
    fNetworkTableConnection_ = make_unique<SQL::ORM::TableConnection<Network>> (
        fDBConnectionPtr_, kNetworkTableSchema_, kDBObjectMapper_,
        BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<Network>> ());
    fNetworkInterfaceTableConnection_ = make_unique<SQL::ORM::TableConnection<NetworkInterface>> (
        fDBConnectionPtr_, kNetworkInterfaceTableSchema_, kDBObjectMapper_,
        BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<NetworkInterface>> ());
    try {
        Debug::TimingTrace ttrc{L"...load of fCachedDeviceUserSettings_ from database ", 1};
        lock_guard         lock{this->fDBConnectionPtr_};
        fCachedDeviceUserSettings_.store (Mapping<GUID, Model::Device::UserOverridesType>{
            fDeviceUserSettingsTableConnection_.rwget ().cref ()->GetAll ().Map<KeyValuePair<GUID, Model::Device::UserOverridesType>> ([] (const auto& i) {
                return KeyValuePair<GUID, Model::Device::UserOverridesType>{i.fDeviceID, i.fUserSettings};
            })});
    }
    catch (...) {
        Logger::sThe.Log (Logger::eCriticalError, L"Failed to load fCachedDeviceUserSettings_ from db: %s",
                          Characters::ToString (current_exception ()).c_str ());
        Execution::ReThrow ();
    }
    try {
        Debug::TimingTrace ttrc{L"...load of fCachedNetworkUserSettings_ from database ", 1};
        lock_guard         lock{this->fDBConnectionPtr_};
        fCachedNetworkUserSettings_.store (Mapping<GUID, Model::Network::UserOverridesType>{
            fNetworkUserSettingsTableConnection_.rwget ().cref ()->GetAll ().Map<KeyValuePair<GUID, Model::Network::UserOverridesType>> ([] (const auto& i) {
                return KeyValuePair<GUID, Model::Network::UserOverridesType>{i.fNetworkID, i.fUserSettings};
            })});
    }
    catch (...) {
        Logger::sThe.Log (Logger::eCriticalError, L"Failed to load fCachedNetworkUserSettings_ from db: %s",
                          Characters::ToString (current_exception ()).c_str ());
        Execution::ReThrow ();
    }
}

Mgr::~Mgr ()
{
    Debug::TraceContextBumper                        ctx{L"IntegratedModel::{}::Mgr::DTOR"};
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
    Debug::TimingTrace                ttrc{L"GenNewNetworkID", 0.001}; // sb very quick
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
    Debug::TimingTrace ttrc{L"IntegratedModel ... SetDeviceUserSettings", 0.1};
    // first check if legit id, and then store
    // @todo check if good id and throw if not...
    auto       lk = fCachedDeviceUserSettings_.rwget ();
    lock_guard lock{this->fDBConnectionPtr_}; //tmphack fix underlying SQL orm wrapper stuff so not needed --LGP 2022-11-23
    if (settings) {
        if (fCachedDeviceUserSettings_.cget ().cref ().Lookup (id) != settings) {
            fDeviceUserSettingsTableConnection_.rwget ().cref ()->AddOrUpdate (ExternalDeviceUserSettingsElt_{id, *settings});
            fCachedDeviceUserSettings_.rwget ().rwref ().Add (id, *settings);
            return true;
        }
        return false;
    }
    else {
        if constexpr (kRepresentIDAs_ == VariantValue::Type::eString) {
            fDeviceUserSettingsTableConnection_.rwget ().cref ()->Delete (VariantValue{id.As<String> ()});
        }
        else {
            fDeviceUserSettingsTableConnection_.rwget ().cref ()->Delete (id);
        }
        return fCachedDeviceUserSettings_.rwget ().rwref ().RemoveIf (id);
    }
}

bool Mgr::SetNetworkUserSettings (const GUID& id, const std::optional<Network::UserOverridesType>& settings)
{
    Debug::TimingTrace ttrc{L"IntegratedModel ... SetNetworkUserSettings", 0.1};
    // first check if legit id, and then store
    // @todo check if good id and throw if not...
    auto lk = fCachedNetworkUserSettings_.rwget ();
    if (settings) {
        if (fCachedNetworkUserSettings_.cget ().cref ().Lookup (id) != settings) {
            fNetworkUserSettingsTableConnection_.rwget ().cref ()->AddOrUpdate (ExternalNetworkUserSettingsElt_{id, *settings});
            fCachedNetworkUserSettings_.rwget ().rwref ().Add (id, *settings);
            return true;
        }
        return false;
    }
    else {
        if constexpr (kRepresentIDAs_ == VariantValue::Type::eString) {
            fNetworkUserSettingsTableConnection_.rwget ().cref ()->Delete (VariantValue{id.As<String> ()});
        }
        else {
            fNetworkUserSettingsTableConnection_.rwget ().cref ()->Delete (id);
        }
        return fCachedNetworkUserSettings_.rwget ().rwref ().RemoveIf (id);
    }
}

String Mgr::GenRandomIDString_ (VariantValue::Type t)
{
    switch (t) {
        case VariantValue::Type::eString:
            return L"randomblob (16)";
        case VariantValue::Type::eBLOB:
            // https://stackoverflow.com/questions/10104662/is-there-uid-datatype-in-sqlite-if-yes-then-how-to-generate-value-for-that
            return L"select substr(u,1,8)||'-'||substr(u,9,4)||'-4'||substr(u,13,3)|| '-' || v || substr (u, 17, 3) || '-' || "
                   L"substr (u, 21, 12) from (select lower (hex (randomblob (16))) as u, substr ('89ab', abs (random ()) % 4 + 1, "
                   L"1) as v) ";
        default:
            RequireNotReached ();
            return L"";
    }
}

void Mgr::_StartBackgroundThread ()
{

    Require (fDatabaseSyncThread_ == nullptr);
    fDatabaseSyncThread_ = Thread::New ([this] () { BackgroundDatabaseThread_ (); }, Thread::eAutoStart, L"BackgroundDatabaseThread"sv);
}

void Mgr::BackgroundDatabaseThread_ ()
{
    Debug::TraceContextBumper ctx{L"BackgroundDatabaseThread_ loop"};
    _OneTimeStartupLoadDB (); // if this fails we fail (has internal retry where appropriate)
    while (true) {
        try {
            // periodically write the latest discovered data to the database

            // UPDATE fDBNetworkInterfaces_ INCREMENTALLY to reflect reflect these merges
            FromDiscovery::GetNetworkInterfaces ().Apply ([this] (const Model::NetworkInterface& ni) {
                lock_guard lock{this->fDBConnectionPtr_};     //tmphack fix underlying SQL orm wrapper stuff so not needed --LGP 2022-11-23
                Assert (ni.fAggregatesReversibly == nullopt); // dont write these summary values
                fNetworkInterfaceTableConnection_->AddOrUpdate (ni);
                fDBNetworkInterfaces_.rwget ()->Add (ni);
            });

            // UPDATE fDBNetworks_ INCREMENTALLY to reflect reflect these merges
            FromDiscovery::GetNetworks ().Apply ([this] (const Model::Network& n) {
                Assert (n.fSeen);                            // don't track/write items which have never been seen
                lock_guard lock{this->fDBConnectionPtr_};    //tmphack fix underlying SQL orm wrapper stuff so not needed --LGP 2022-11-23
                Assert (n.fAggregatesReversibly == nullopt); // dont write these summary values
                fNetworkTableConnection_->AddOrUpdate (n);
                fDBNetworks_.rwget ()->Add (n);
            });

            // UPDATE fDBDevices_ INCREMENTALLY to reflect reflect these merges
            FromDiscovery::GetDevices ().Apply ([this] (const Model::Device& d) {
                Assert (d.fSeen.EverSeen ());
                Assert (d.fSeen.EverSeen ());                // don't track/write items which have never been seen
                Assert (d.fUserOverrides == nullopt);        // tracked on rollup devices, not snapshot devices
                lock_guard lock{this->fDBConnectionPtr_};    //tmphack fix underlying SQL orm wrapper stuff so not needed --LGP 2022-11-23
                Assert (d.fAggregatesReversibly == nullopt); // dont write these summary values
                auto rec2Update = fDB_.AddOrMergeUpdate (fDeviceTableConnection_.get (), d);
                fDBDevices_.rwget ()->Add (rec2Update);
            });

            // only update periodically
            Execution::Sleep (30s);
        }
        catch (const Thread::AbortException&) {
            Execution::ReThrow ();
        }
        catch (...) {
            //DbgTrace (L"Ignoring (will retry in 30 seconds) exception in BackgroundDatabaseThread_ loop: %s", Characters::ToString (current_exception ()).c_str ());
            Logger::sThe.Log (Logger::eWarning, L"Database update: ignoring exception in BackgroundDatabaseThread_ loop (will retry in 30 seconds): %s",
                              Characters::ToString (current_exception ()).c_str ());
            Execution::Sleep (30s);
        }
    }
}

void Mgr::_OneTimeStartupLoadDB ()
{
    /*
     *  Considered loading this from CTOR, but then the rest of APP load would be delayed by this DB load. Better to
     *  let what can startup do so, and just block the webservices and things that depend on database load 
     *  elsewhere.
     */
    Debug::TraceContextBumper ctx{L"OneTimeStartup_"};
    optional<unsigned int>    netInterfaceSnapshotsLoaded{};
    optional<unsigned int>    netSnapshotsLoaded{};
    optional<unsigned int>    deviceSnapshotsLoaded{};
    auto                      fetchInterfacesNetworks = [this] () -> unsigned int {
        try {
            Debug::TimingTrace ttrc{L"...initial load of fDBNetworkInterfaces_ from database ", 1};
            auto               errorHandler = [] ([[maybe_unused]] const SQL::Statement::Row& r, const exception_ptr& e) -> optional<NetworkInterface> {
                // Just drop the record on the floor after logging
                Logger::sThe.Log (Logger::eError, L"Error reading database of persisted network interfaces snapshot ('%s'): %s",
                                                                     Characters::ToString (r).c_str (), Characters::ToString (e).c_str ());
                return nullopt;
            };
            auto all = fNetworkInterfaceTableConnection_->GetAll (errorHandler);
            fDBNetworkInterfaces_.store (NetworkInterfaceCollection{all});
            return static_cast<unsigned int> (all.size ());
        }
        catch (...) {
            Logger::sThe.Log (Logger::eError, L"Probably important error reading database of old network interfaces data: %s",
                                                   Characters::ToString (current_exception ()).c_str ());
            Execution::ReThrow ();
        }
    };
    auto fetchNets = [this] () -> unsigned int {
        try {
            Debug::TimingTrace ttrc{L"...initial load of fDBNetworks_ from database ", 1};
            auto               errorHandler = [] ([[maybe_unused]] const SQL::Statement::Row& r, const exception_ptr& e) -> optional<Network> {
                // Just drop the record on the floor after logging
                Logger::sThe.Log (Logger::eError, L"Error reading database of persisted network snapshot ('%s'): %s",
                                                Characters::ToString (r).c_str (), Characters::ToString (e).c_str ());
                return nullopt;
            };
            auto all = fNetworkTableConnection_->GetAll (errorHandler);
            fDBNetworks_.store (NetworkCollection{all});
            return static_cast<unsigned int> (all.size ());
        }
        catch (...) {
            Logger::sThe.Log (Logger::eError, L"Probably important error reading database of old networks data: %s",
                              Characters::ToString (current_exception ()).c_str ());
            Execution::ReThrow ();
        }
    };
    auto fetchDevices = [this] () -> unsigned int {
        try {
            Debug::TimingTrace ttrc{L"...initial load of fDBDevices_ from database ", 1};
            auto               errorHandler = [] ([[maybe_unused]] const SQL::Statement::Row& r, const exception_ptr& e) -> optional<Device> {
                // Just drop the record on the floor after logging
                Logger::sThe.Log (Logger::eError, L"Error reading database of persisted device snapshot ('%s'): %s",
                                                Characters::ToString (r).c_str (), Characters::ToString (e).c_str ());
                return nullopt;
            };
            auto all = fDeviceTableConnection_->GetAll (errorHandler);
            if constexpr (qDebug) {
                all.Apply ([] ([[maybe_unused]] const Device& d) { Assert (!d.fUserOverrides); }); // tracked on rollup devices, not snapshot devices
            }
            fDBDevices_.store (DeviceCollection{all}); // pre-load in memory copy with whatever we had stored in the database
            return static_cast<unsigned int> (all.size ());
        }
        catch (...) {
            Logger::sThe.Log (Logger::eError, L"Probably important error reading database of old device data: %s",
                              Characters::ToString (current_exception ()).c_str ());
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
            //DbgTrace (L"Ignoring (will retry in 30 seconds) exception in BackgroundDatabaseThread_ loop: %s", Characters::ToString (current_exception ()).c_str ());
            Logger::sThe.Log (Logger::eWarning, L"Database error: ignoring exception in OneTimeStartup_ loop (will retry in 10 seconds): %s",
                              Characters::ToString (current_exception ()).c_str ());
            Execution::Sleep (10s);
        }
    }
}
