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
    namespace DBAccess_ {
        using namespace SQL::ORM;
        using namespace SQL::SQLite;

        constexpr VariantValue::Type kRepresentIDAs_ = VariantValue::Type::eBLOB; // else as string

        Execution::Thread::Ptr sDatabaseSyncThread_{};

        // the latest copy of what is in the DB
        Synchronized<Sequence<IntegratedModel::Device>> sDBDevices_;
        // the latest copy of what is in the DB
        Synchronized<Sequence<IntegratedModel::Network>> sDBNetworks_;

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
                {.fName = L"ID", .fVariantValueFieldName = L"id"sv, .fVariantType = kRepresentIDAs_, .fIsKeyField = true, .fDefaultExpression = L"randomblob(16)"sv, .fNotNull = true},
                {.fName = L"name", .fVariantType = VariantValue::eString, .fNotNull = false},
                {.fName = L"lastSeenAt", .fVariantType = VariantValue::eString, .fNotNull = true},
#else
                {L"ID", L"id"sv, false, kRepresentIDAs_, nullopt, true, nullopt, L"randomblob(16)"sv, true},
                {L"name", nullopt, false, VariantValue::eString},
                {L"lastSeenAt", nullopt, false, VariantValue::eString, false, nullopt, nullopt, true},
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
                {.fName = L"ID", .fVariantValueFieldName = L"id"sv, .fVariantType = kRepresentIDAs_, .fIsKeyField = true, .fDefaultExpression = L"randomblob(16)"sv, .fNotNull = true},
                {.fName = L"friendlyName", .fVariantType = VariantValue::eString, .fNotNull = false},
                {.fName = L"lastSeenAt", .fVariantType = VariantValue::eString, .fNotNull = true},
#else
                {L"ID", L"id"sv, false, kRepresentIDAs_, nullopt, true, nullopt, L"randomblob(16)"sv, true},
                {L"friendlyName", nullopt, false, VariantValue::eString},
                {L"lastSeenAt", nullopt, false, VariantValue::eString, false, nullopt, nullopt, true},
#endif
            },
            Schema::CatchAllField{}};
        static_assert (kRepresentIDAs_ == VariantValue::eBLOB); // @todo to support string, just change '.fDefaultExpression'

        template <typename T>
        void AddOrMergeUpdate_ (SQL::ORM::TableConnection<T>* dbConnTable, const T& d)
        {
            Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...AddOrMergeUpdate_", L"...,d=%s", Characters::ToString (d).c_str ())};
            RequireNotNull (dbConnTable);
            SQL::Transaction t{dbConnTable->pConnection ()->mkTransaction ()};
            if (auto dbObj = dbConnTable->GetByID (d.fGUID)) {
                dbConnTable->Update (T::Merge (*dbObj, d));
            }
            else {
                dbConnTable->AddNew (d);
            }
            t.Commit ();
        }
        Connection::Ptr SetupDB_ ()
        {
            auto dbPath = IO::FileSystem::WellKnownLocations::GetApplicationData () / "WhyTheFuckIsMyNetworkSoSlow" / "db-v4.db";
            filesystem::create_directories (dbPath.parent_path ());
#if __cpp_designated_initializers
            auto options = Options{.fDBPath = dbPath, .fThreadingMode = Options::ThreadingMode::eMultiThread};
#else
            auto options = Options{dbPath, true, nullopt, nullopt, Options::ThreadingMode::eMultiThread};
#endif
            auto initializeDB = [] (const Connection::Ptr& c) {
                c.Exec (Schema::StandardSQLStatements{kDeviceTableSchema_}.CreateTable ());
                c.Exec (Schema::StandardSQLStatements{kNetworkTableSchema_}.CreateTable ());
            };
            return Connection::New (options, initializeDB);
        }
        void BackgroundDatabaseThread_ ()
        {
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
                        sDBDevices_.store (deviceTableConnection->GetAll ());
                    }
                    if (networkTableConnection == nullptr) {
                        networkTableConnection = make_unique<SQL::ORM::TableConnection<IntegratedModel::Network>> (conn, kNetworkTableSchema_, kDBObjectMapper_);
                        sDBNetworks_.store (networkTableConnection->GetAll ());
                    }

                    // @todo UPDATE sDBDevices_/sDBNetworks_ to reflect reflect these merges (but maybe not useful)
                    for (auto ni : DiscoveryWrapper_::GetNetworks_ ()) {
                        AddOrMergeUpdate_ (networkTableConnection.get (), ni);
                    }
                    // @todo UPDATE sDBDevices_ on each one
                    for (auto di : DiscoveryWrapper_::GetDevices_ ()) {
                        AddOrMergeUpdate_ (deviceTableConnection.get (), di);
                    }

                    // only update periodically
                    Execution::Sleep (30s);
                }
                catch (const Thread::AbortException&) {
                    Execution::ReThrow ();
                }
                catch (...) {
                    DbgTrace (L"Ignoring exception in BackgroundDatabaseThread_ loop: %s", Characters::ToString (current_exception ()).c_str ());
                }
                // periodically write the latest discovered data to the database
            }
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
    Mapping<GUID, Device> devices; // @todo use KeyedCollection when available feature in Stroika
    DBAccess_::sDBDevices_->Apply ([&devices] (auto d) { devices.Add (d.fGUID, d); });
    for (Device d : DiscoveryWrapper_::GetDevices_ ()) {
        if (auto dbDevice = devices.Lookup (d.fGUID)) {
            devices.Add (d.fGUID, Device::Merge (*dbDevice, d));
        }
        else {
            devices.Add (d.fGUID, d);
        }
    }
    return Sequence<Device>{devices.MappedValues ()};
}

std::optional<IntegratedModel::Device> IntegratedModel::Mgr::GetDevice (const Common::GUID& id) const
{
    // @todo can make much faster
    for (auto i : GetDevices ()) {
        if (i.fGUID == id) {
            return i;
        }
    }
    return nullopt;
}

Sequence<IntegratedModel::Network> IntegratedModel::Mgr::GetNetworks () const
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"IntegratedModel::Mgr::GetNetworks")};
    Debug::TimingTrace        ttrc{L"IntegratedModel::Mgr::GetNetworks", 0.1};
    using IntegratedModel::Network;
    Mapping<GUID, Network> networks; // @todo use KeyedCollection when available feature in Stroika
    DBAccess_::sDBNetworks_->Apply ([&networks] (auto n) { networks.Add (n.fGUID, n); });
    for (Network n : DiscoveryWrapper_::GetNetworks_ ()) {
        if (auto dbNetwork = networks.Lookup (n.fGUID)) {
            networks.Add (n.fGUID, Network::Merge (*dbNetwork, n));
        }
        else {
            networks.Add (n.fGUID, n);
        }
    }
    return Sequence<Network>{networks.MappedValues ()};
}

std::optional<IntegratedModel::Network> IntegratedModel::Mgr::GetNetwork (const Common::GUID& id) const
{
    // @todo can make much faster
    for (auto i : GetNetworks ()) {
        if (i.fGUID == id) {
            return i;
        }
    }
    return nullopt;
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