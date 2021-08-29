/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Common/GUID.h"
#include "Stroika/Foundation/Common/KeyValuePair.h"
#include "Stroika/Foundation/Common/Property.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/Database/SQL/ORM/Schema.h"
#include "Stroika/Foundation/Database/SQL/ORM/TableConnection.h"
#include "Stroika/Foundation/Database/SQL/ORM/Versioning.h"
#include "Stroika/Foundation/Database/SQL/SQLite.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Execution/Sleep.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/FileSystem/WellKnownLocations.h"

#include "../Common/BLOBMgr.h"
#include "../Common/EthernetMACAddressOUIPrefixes.h"

#include "../Discovery/Devices.h"
#include "../Discovery/NetworkInterfaces.h"
#include "../Discovery/Networks.h"

#include "AppVersion.h"

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

namespace {
    // For now (as of 2021-07-18) NYI
    // so dont write out, and dont merge from DB - only show current ones
    constexpr bool kSupportPersistedNetworkInterfaces_{false};
}

namespace {
    URI TransformURL2LocalStorage_ (const URI& url)
    {
        Debug::TimingTrace ttrc{L"TransformURL2LocalStorage_", 0.1}; // sb very quick cuz we schedule url fetches for background

        // if we are unable to cache the url (say because the url is bad or the device is currently down)
        // just return the original

        try {
            // This BLOBMgr code wont block - it will push a request into a Q, and fetch whatever data is has (maybe none)
            optional<GUID> g = BackendApp::Common::BLOBMgr::sThe.AsyncAddBLOBFromURL (url);
            // tricky to know right host to supply here - will need some sort of configuration for
            // this (due to firewalls, NAT etc).
            // Use relative URL for now, as that should work for most cases
            if (g) {
                return URI{nullopt, nullopt, L"/blob/" + g->ToString ()};
            }
        }
        catch (...) {
            AssertNotReached ();
        }
        DbgTrace (L"Failed to cache url (%s) - so returning original", Characters::ToString (url).c_str ());
        return url;
    }
    optional<URI> TransformURL2LocalStorage_ (const optional<URI>& url)
    {
        return url ? TransformURL2LocalStorage_ (*url) : optional<URI>{};
    }
}

namespace {
    /**
     *  Wrappers on the device manager APIs, that just fetch the discovered devices and convert to common
     *  integrated model (no datebase awareness)
     */
    namespace DiscoveryWrapper_ {
        using IntegratedModel::Device;
        using IntegratedModel::Network;
        using IntegratedModel::NetworkAttachmentInfo;
        Sequence<Network> GetNetworks_ ()
        {
            Debug::TimingTrace ttrc{L"DiscoveryWrapper_::GetNetworks_", 0.1};
            DateTime           now = DateTime::Now ();
            Sequence<Network>  result;
            for (Discovery::Network n : Discovery::NetworksMgr::sThe.CollectActiveNetworks ()) {
                Network nw{n.fNetworkAddresses};
                nw.fGUID                    = n.fGUID;
                nw.fFriendlyName            = n.fFriendlyName;
                nw.fNetworkAddresses        = n.fNetworkAddresses;
                nw.fAttachedInterfaces      = n.fAttachedNetworkInterfaces;
                nw.fDNSServers              = n.fDNSServers;
                nw.fGateways                = n.fGateways;
                nw.fExternalAddresses       = n.fExternalAddresses;
                nw.fGEOLocInformation       = n.fGEOLocInfo;
                nw.fInternetServiceProvider = n.fISP;
                nw.fLastSeenAt              = now; // if we are discovering it now, the network is there now...
#if qDebug
                if (not n.fDebugProps.empty ()) {
                    nw.fDebugProps = n.fDebugProps;
                }
#endif
                result += nw;
            }
#if USE_NOISY_TRACE_IN_THIS_MODULE_
            DbgTrace (L"returns: %s", Characters::ToString (result).c_str ());
#endif
            return result;
        }
        Sequence<Device> GetDevices_ ()
        {
            Debug::TimingTrace ttrc{L"DiscoveryWrapper_::GetDevices_", .1};
            // Fetch (UNSORTED) list of devices
            return Sequence<Device>{Discovery::DevicesMgr::sThe.GetActiveDevices ().Select<Device> ([] (const Discovery::Device& d) {
                Device newDev;
                newDev.fGUID = d.fGUID;
                newDev.name  = d.name;
                if (not d.fTypes.empty ()) {
                    newDev.fTypes = d.fTypes; // leave missing if no discovered types
                }
                newDev.fLastSeenAt = d.fLastSeenAt;
                newDev.fOpenPorts  = d.fOpenPorts;
                for (auto i : d.fAttachedNetworks) {
                    constexpr bool            kIncludeLinkLocalAddresses_{Discovery::kIncludeLinkLocalAddressesInDiscovery};
                    constexpr bool            kIncludeMulticastAddreses_{Discovery::kIncludeMulticastAddressesInDiscovery};
                    Sequence<InternetAddress> addrs2Report;
                    for (auto li : i.fValue.localAddresses) {
                        if (not kIncludeLinkLocalAddresses_ and li.IsLinkLocalAddress ()) {
                            continue;
                        }
                        if (not kIncludeMulticastAddreses_ and li.IsMulticastAddress ()) {
                            continue;
                        }
                        addrs2Report += li;
                    }
                    newDev.fAttachedNetworks.Add (i.fKey, NetworkAttachmentInfo{i.fValue.hardwareAddresses, addrs2Report});
                }
                newDev.fAttachedNetworkInterfaces = d.fAttachedInterfaces; // @todo must merge += (but only when merging across differnt discoverers/networks)
                newDev.fPresentationURL           = d.fPresentationURL;
                newDev.fManufacturer              = d.fManufacturer;
                newDev.fIcon                      = TransformURL2LocalStorage_ (d.fIcon);
                newDev.fOperatingSystem           = d.fOperatingSystem;
#if qDebug
                if (not d.fDebugProps.empty ()) {
                    newDev.fDebugProps = d.fDebugProps;
                }
                {
                    // List OUI names for each hardware address (and explicit missing for those we cannot lookup)
                    using VariantValue = DataExchange::VariantValue;
                    Mapping<String, VariantValue> t;
                    for (auto i : d.fAttachedNetworks) {
                        for (auto hwa : i.fValue.hardwareAddresses) {
                            auto o = BackendApp::Common::LookupEthernetMACAddressOUIFromPrefix (hwa);
                            t.Add (hwa, o ? VariantValue{*o} : VariantValue{});
                        }
                    }
                    if (not t.empty ()) {
                        if (not newDev.fDebugProps.has_value ()) {
                            newDev.fDebugProps = Mapping<String, VariantValue>{};
                        }
                        newDev.fDebugProps->Add (L"MACAddr2OUINames", VariantValue{t});
                    }
                }
#endif
                return newDev;
            })};
        }

    }
}

namespace {
    namespace RollupCommon_ {
        using IntegratedModel::Device;
        using IntegratedModel::Network;
        using IntegratedModel::NetworkAttachmentInfo;
        bool ShouldRollup_ (const Device& d1, const Device& d2)
        {
            // very rough first draft. Later add database stored 'exceptions' and/or rules tables to augment this logic
            auto hw1 = d1.GetHardwareAddresses ();
            auto hw2 = d2.GetHardwareAddresses ();
            if (Set<String>::Intersects (hw1, hw2)) {
                return true;
            }
            if (hw1.empty () and hw2.empty ()) {
                // then fold togehter if they have the same IP Addresses
               // return d1.GetInternetAddresses () == d2.GetInternetAddresses ();
                return Set<InternetAddress>::Intersects (d1.GetInternetAddresses (), d2.GetInternetAddresses ());
            }
            // unclear if above test should be if EITHER set is empty, maybe then do if timeframes very close?
            return false;
        }
        bool ShouldRollup_ (const Network& n1, const Network& n2)
        {
            // wag for now how to roll networks together
            if (n1.fNetworkAddresses != n2.fNetworkAddresses) {
                return false;
            }
            if (n1.fGEOLocInformation != n2.fGEOLocInformation) {
                return false;
            }
            return true;
        }
    }
}

namespace {
    namespace DBAccess_ {
        using namespace SQL::ORM;
        using namespace SQL::SQLite;

        constexpr VariantValue::Type kRepresentIDAs_ = VariantValue::Type::eBLOB; // else as string

        Execution::Thread::Ptr sDatabaseSyncThread_{};

        // the latest copy of what is in the DB (manually kept up to date) - @todo use KeyedCollection<> when supported
        Synchronized<Mapping<GUID, IntegratedModel::Device>>  sDBDevices_;
        Synchronized<Mapping<GUID, IntegratedModel::Network>> sDBNetworks_;

        // Workaround lack of KeyedCollection support
        Mapping<GUID, IntegratedModel::Device> ToKeyedCollectionStore_ (const Traversal::Iterable<IntegratedModel::Device>& ds)
        {
            using IntegratedModel::Device;
            Mapping<GUID, Device> devices; // @todo use KeyedCollection when available feature in Stroika
            ds.Apply ([&devices] (auto n) { devices.Add (n.fGUID, n); });
            return devices;
        }
        Mapping<GUID, IntegratedModel::Network> ToKeyedCollectionStore_ (const Traversal::Iterable<IntegratedModel::Network>& nets)
        {
            using IntegratedModel::Network;
            Mapping<GUID, Network> networks; // @todo use KeyedCollection when available feature in Stroika
            nets.Apply ([&networks] (auto n) { networks.Add (n.fGUID, n); });
            return networks;
        }

        /*
         *  Combined mapper for objects we write to the database. Contains all the objects mappers we need merged together,
         *  and any touchups on represenation we need (like writing GUID as BLOB rather than string).
         */
        const ConstantProperty<ObjectVariantMapper> kDBObjectMapper_{[] () {
            ObjectVariantMapper mapper;

            mapper += IntegratedModel::Device::kMapper;
            mapper += IntegratedModel::Network::kMapper;

            // ONLY DO THIS FOR WHEN WRITING TO DB -- store GUIDs as BLOBs - at least for database interactions (cuz more efficient)
            mapper.AddCommonType<Stroika::Foundation::Common::GUID> (kRepresentIDAs_);

            return mapper;
        }};

        const Schema::Table kDeviceTableSchema_{
            L"Devices",
            /*
             *  use the same names as the ObjectVariantMapper for simpler mapping, or specify an alternate name
             *  for ID, just as an example.
             */
            Collection<Schema::Field>{
#if __cpp_designated_initializers
                /**
                 *  For ID, generate random GUID (BLOB) automatically in database
                 */
                {.fName = L"ID", .fVariantValueName = L"id"sv, .fRequired = true, .fVariantValueType = kRepresentIDAs_, .fIsKeyField = true, .fDefaultExpression = L"randomblob(16)"sv},
                {.fName = L"name", .fVariantValueType = VariantValue::eString},
                {.fName = L"lastSeenAt", .fVariantValueType = VariantValue::eString},
#else
                {L"ID", L"id"sv, true, kRepresentIDAs_, nullopt, true, nullopt, L"randomblob(16)"sv},
                {L"name", nullopt, false, VariantValue::eString},
                {L"lastSeenAt", nullopt, false, VariantValue::eString},
#endif
            },
            Schema::CatchAllField{}};
        const Schema::Table kNetworkTableSchema_{
            L"Networks",
            /*
             *  use the same names as the ObjectVariantMapper for simpler mapping, or specify an alternate name
             *  for ID, just as an example.
             */
            Collection<Schema::Field>{
#if __cpp_designated_initializers
                /**
                 *  For ID, generate random GUID (BLOB) automatically in database
                 */
                {.fName = L"ID", .fVariantValueName = L"id"sv, .fRequired = true, .fVariantValueType = kRepresentIDAs_, .fIsKeyField = true, .fDefaultExpression = L"randomblob(16)"sv},
                {.fName = L"friendlyName", .fVariantValueType = VariantValue::eString},
                {.fName = L"lastSeenAt", .fVariantValueType = VariantValue::eString},
#else
                {L"ID", L"id"sv, true, kRepresentIDAs_, nullopt, true, nullopt, L"randomblob(16)"sv},
                {L"friendlyName", nullopt, false, VariantValue::eString},
                {L"lastSeenAt", nullopt, false, VariantValue::eString},
#endif
            },
            Schema::CatchAllField{}};
        static_assert (kRepresentIDAs_ == VariantValue::eBLOB); // @todo to support string, just change '.fDefaultExpression'

        template <typename T>
        T AddOrMergeUpdate_ (SQL::ORM::TableConnection<T>* dbConnTable, const T& d)
        {
            Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...AddOrMergeUpdate_", L"...,d=%s", Characters::ToString (d).c_str ())};
            RequireNotNull (dbConnTable);
            SQL::Transaction t{dbConnTable->pConnection ()->mkTransaction ()};
            optional<T>      result;
            if (auto dbObj = dbConnTable->GetByID (d.fGUID)) {
                result = T::Merge (*dbObj, d);
                dbConnTable->Update (*result);
            }
            else {
                result = d;
                dbConnTable->AddNew (d);
            }
            t.Commit ();
            Ensure (result.has_value ());
            return *result;
        }
        Connection::Ptr SetupDB_ ()
        {
            auto dbPath = IO::FileSystem::WellKnownLocations::GetApplicationData () / "WhyTheFuckIsMyNetworkSoSlow" / "db-v6.db";
            filesystem::create_directories (dbPath.parent_path ());
#if __cpp_designated_initializers
            auto options = Options{.fDBPath = dbPath, .fThreadingMode = Options::ThreadingMode::eMultiThread};
#else
            auto options = Options{dbPath, true, nullopt, nullopt, Options::ThreadingMode::eMultiThread};
#endif
            auto conn = Connection::New (options);

            // @todo use AppVersion? probably not not clearly. SHOULD MATCH but dont auto-update
            // verison just cuz code version goes up - maybe write both verisons
            constexpr Configuration::Version kCurrentVersion_ = Configuration::Version{1, 0, Configuration::VersionStage::Alpha, 0};
            Database::SQL::ORM::ProvisionForVersion (conn,
                                                     kCurrentVersion_,
                                                     Traversal::Iterable<Database::SQL::ORM::Schema::Table>{kDeviceTableSchema_, kNetworkTableSchema_});

            return conn;
        }
        void BackgroundDatabaseThread_ ()
        {
            Debug::TraceContextBumper                                       ctx{L"BackgroundDatabaseThread_ loop"};
            Connection::Ptr                                                 conn;
            unique_ptr<SQL::ORM::TableConnection<IntegratedModel::Device>>  deviceTableConnection;
            unique_ptr<SQL::ORM::TableConnection<IntegratedModel::Network>> networkTableConnection;
            while (true) {
                try {
                    if (conn == nullptr) {
                        conn = SetupDB_ ();
                    }
                    if (deviceTableConnection == nullptr) {
                        deviceTableConnection = make_unique<SQL::ORM::TableConnection<IntegratedModel::Device>> (conn, kDeviceTableSchema_, kDBObjectMapper_);
                        try {
                            sDBDevices_.store (ToKeyedCollectionStore_ (deviceTableConnection->GetAll ())); // pre-load in memory copy with whatever we had stored in the database
                        }
                        catch (...) {
                            deviceTableConnection = nullptr; // so we re-fetch
                            Execution::ReThrow ();
                        }
                    }
                    if (networkTableConnection == nullptr) {
                        networkTableConnection = make_unique<SQL::ORM::TableConnection<IntegratedModel::Network>> (conn, kNetworkTableSchema_, kDBObjectMapper_);
                        try {
                            sDBNetworks_.store (ToKeyedCollectionStore_ (networkTableConnection->GetAll ()));
                        }
                        catch (...) {
                            networkTableConnection = nullptr; // so we re-fetch
                            Execution::ReThrow ();
                        }
                    }

                    // periodically write the latest discovered data to the database

                    // UPDATE sDBNetworks_ INCREMENTALLY to reflect reflect these merges
                    for (auto ni : DiscoveryWrapper_::GetNetworks_ ()) {
                        if (not kSupportPersistedNetworkInterfaces_) {
                            ni.fAttachedInterfaces.clear ();
                        }
                        auto rec2Update = AddOrMergeUpdate_ (networkTableConnection.get (), ni);
                        sDBNetworks_.rwget ()->Add (rec2Update.fGUID, rec2Update);
                    }

                    // UPDATE sDBDevices_ INCREMENTALLY to reflect reflect these merges
                    for (auto di : DiscoveryWrapper_::GetDevices_ ()) {
                        if (not kSupportPersistedNetworkInterfaces_) {
                            di.fAttachedNetworkInterfaces = nullopt;
                        }
                        auto rec2Update = AddOrMergeUpdate_ (deviceTableConnection.get (), di);
                        sDBDevices_.rwget ()->Add (rec2Update.fGUID, rec2Update);
                    }

                    // only update periodically
                    Execution::Sleep (30s);
                }
                catch (const Thread::AbortException&) {
                    Execution::ReThrow ();
                }
                catch (...) {
                    DbgTrace (L"Ignoring (will retry in 30 seconds) exception in BackgroundDatabaseThread_ loop: %s", Characters::ToString (current_exception ()).c_str ());
                    Execution::Sleep (30s);
                }
            }
        }
    }
}

namespace {
    namespace RollupSummary_ {
        using namespace RollupCommon_;

        Synchronized<Mapping<GUID, Device>> sRolledUpDevices_;

        Mapping<GUID, Network> GetRolledUpNetworks ();

        // primitive draft
        Mapping<GUID, Device> GetRolledUpDevies ()
        {
            auto rolledUpNetworks = GetRolledUpNetworks ();

            //tmphack slow impl - instead build mapping table when constructing rollup
            // of networkinfo
            // @todo define struct for NetworksRollup (and devices) that has above map, and the REVERSE ID map
            // (individual 2 rollup), and then provide funciton to map a set of IDs to their rolled up IDs
            // AND DOCUMENT WHY GUARNATEED UNIQUE - IF AS I THINK IT IS - CUZ each item goes in one rollup)
            auto mapAggregatedNetID2ItsRollupID = [&] (GUID netID) -> GUID {
                for (auto i : rolledUpNetworks) {
                    if (i.fValue.fAggregatesReversibly and i.fValue.fAggregatesReversibly->Contains (netID)) {
                        return i.fKey;
                    }
                }
                // address- but not yet, as we get this too often, and too noisy in logs - WeakAsserteNotReached ();
                //AssertNotReached ();    // because we guarantee each item rolled up exactly once
                return netID;
            };
            auto reverseRollup = [&] (Mapping<GUID, NetworkAttachmentInfo> nats) -> Mapping<GUID, NetworkAttachmentInfo> {
                Mapping<GUID, NetworkAttachmentInfo> result;
                for (auto ni : nats) {
                    result.Add (mapAggregatedNetID2ItsRollupID (ni.fKey), ni.fValue);
                }
                return result;
            };

            // Start with the existing rolled up devices
            // and then add in (should be done just once) the values from the database,
            // and then keep adding any more recent discovery changes
            Mapping<GUID, Device> result               = sRolledUpDevices_.load ();
            auto                  doMergeOneIntoRollup = [&result, &reverseRollup] (const Device& d2MergeIn) {
                // @todo slow/quadradic - may need to tweak
                if (auto i = result.FindFirstThat ([&d2MergeIn] (auto kvpDevice) { return ShouldRollup_ (kvpDevice.fValue, d2MergeIn); })) {
                    // then merge this into that item
                    // @todo think out order of params and better document order of params!
                    auto tmp              = Device::Rollup (i->fValue, d2MergeIn);
                    tmp.fAttachedNetworks = reverseRollup (tmp.fAttachedNetworks);
                    result.Add (i->fKey, tmp);
                    Assert (result[i->fKey].fGUID == i->fKey); // sb using new KeyedCollection!
                }
                else {
                    Device newRolledUpDevice      = d2MergeIn;
                    newRolledUpDevice.fAggregatesReversibly = Set<GUID>{d2MergeIn.fGUID};
                    newRolledUpDevice.fGUID       = GUID::GenerateNew ();
                    result.Add (newRolledUpDevice.fGUID, newRolledUpDevice);
                    Assert (result[newRolledUpDevice.fGUID].fGUID == newRolledUpDevice.fGUID); // sb using new KeyedCollection!
                }
            };
            for (auto rdi : DBAccess_::sDBDevices_.load ().MappedValues ()) {
                doMergeOneIntoRollup (rdi);
            }
            for (Device d : DiscoveryWrapper_::GetDevices_ ()) {
                doMergeOneIntoRollup (d);
            }
            sRolledUpDevices_.store (result);
            return result;
        }

        Synchronized<Mapping<GUID, Network>> sRolledUpNetworks_;

        // primitive draft
        Mapping<GUID, Network> GetRolledUpNetworks ()
        {
            // Start with the existing rolled up devices
            // and then add in (should be done just once) the values from the database,
            // and then keep adding any more recent discovery changes
            Mapping<GUID, Network> result               = sRolledUpNetworks_.load ();
            auto                   doMergeOneIntoRollup = [&result] (const Network& d2MergeIn) {
                // @todo slow/quadradic - may need to tweak
                if (auto i = result.FindFirstThat ([&d2MergeIn] (auto kvpDevice) { return ShouldRollup_ (kvpDevice.fValue, d2MergeIn); })) {
                    // then merge this into that item
                    // @todo think out order of params and better document order of params!
                    result.Add (i->fKey, Network::Rollup (i->fValue, d2MergeIn));
                }
                else {
                    Network newRolledUpDevice     = d2MergeIn;
                    newRolledUpDevice.fAggregatesReversibly = Set<GUID>{d2MergeIn.fGUID};
                    newRolledUpDevice.fGUID       = GUID::GenerateNew ();
                    result.Add (newRolledUpDevice.fGUID, newRolledUpDevice);
                }
            };
            for (auto rdi : DBAccess_::sDBNetworks_.load ().MappedValues ()) {
                doMergeOneIntoRollup (rdi);
            }
            for (Network d : DiscoveryWrapper_::GetNetworks_ ()) {
                doMergeOneIntoRollup (d);
            }
            sRolledUpNetworks_.store (result);
            return result;
        }

    }
}

/*
 ********************************************************************************
 ************************** IntegratedModel::Mgr::Activator *********************
 ********************************************************************************
 */
IntegratedModel::Mgr::Activator::Activator ()
{
    Require (DBAccess_::sDatabaseSyncThread_ == nullptr);
    DBAccess_::sDatabaseSyncThread_ = Thread::New (DBAccess_::BackgroundDatabaseThread_, Thread::eAutoStart, L"BackgroundDatabaseThread"sv);
}

IntegratedModel::Mgr::Activator::~Activator ()
{
    DBAccess_::sDatabaseSyncThread_.AbortAndWaitForDone ();
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
    using IntegratedModel::Device;
    return Sequence<Device>{RollupSummary_::GetRolledUpDevies ().MappedValues ()};
}

optional<IntegratedModel::Device> IntegratedModel::Mgr::GetDevice (const Common::GUID& id) const
{
    // first check rolled up networks, and then raw/unrolled up networks
    auto result = RollupSummary_::GetRolledUpDevies ().Lookup (id);
    if (not result.has_value ()) {
        result = DBAccess_::sDBDevices_.load ().Lookup (id);
    }
    return result;
}

Sequence<IntegratedModel::Network> IntegratedModel::Mgr::GetNetworks () const
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"IntegratedModel::Mgr::GetNetworks")};
    Debug::TimingTrace        ttrc{L"IntegratedModel::Mgr::GetNetworks", 0.1};
    using IntegratedModel::Network;
    return Sequence<Network>{RollupSummary_::GetRolledUpNetworks ().MappedValues ()};
}

optional<IntegratedModel::Network> IntegratedModel::Mgr::GetNetwork (const Common::GUID& id) const
{
    // first check rolled up networks, and then raw/unrolled up networks
    auto result = RollupSummary_::GetRolledUpNetworks ().Lookup (id);
    if (not result.has_value ()) {
        result = DBAccess_::sDBNetworks_.load ().Lookup (id);
    }
    return result;
}

Collection<IntegratedModel::NetworkInterface> IntegratedModel::Mgr::GetNetworkInterfaces () const
{
    Collection<NetworkInterface> result;
    for (Discovery::NetworkInterface n : Discovery::NetworkInterfacesMgr::sThe.CollectAllNetworkInterfaces ()) {
        NetworkInterface nw{n};
        nw.fGUID = n.fGUID;
#if qDebug
        if (not n.fDebugProps.empty ()) {
            nw.fDebugProps = n.fDebugProps;
        }
#endif
        result += nw;
    }
    return result;
}

optional<IntegratedModel::NetworkInterface> IntegratedModel::Mgr::GetNetworkInterface (const Common::GUID& id) const
{
    for (auto i : GetNetworkInterfaces ()) {
        if (i.fGUID == id) {
            return i;
        }
    }
    return nullopt;
}