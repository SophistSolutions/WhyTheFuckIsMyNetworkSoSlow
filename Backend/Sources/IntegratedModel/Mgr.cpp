/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Common/GUID.h"
#include "Stroika/Foundation/Common/KeyValuePair.h"
#include "Stroika/Foundation/Common/Property.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/Database/SQL/ORM/Schema.h"
#include "Stroika/Foundation/Database/SQL/SQLite.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Execution/Sleep.h"
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
    namespace DBAccess_ {
        using namespace SQL::ORM;
        using namespace SQL::SQLite;

        constexpr VariantValue::Type kRepresentIDAs_ = VariantValue::Type::eBLOB; // else as string

        Execution::Thread::Ptr sDatabaseSyncThread_{};

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
                {.fName = L"name", .fVariantType = VariantValue::eString, .fNotNull = false}
#else
                {L"ID", L"id"sv, false, kRepresentIDAs_, nullopt, true, nullopt, L"randomblob(16)"sv, true},
                {L"name", nullopt, false, VariantValue::eString, nullopt, nullopt, nullopt, nullopt, false}
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
                {.fName = L"name", .fVariantType = VariantValue::eString, .fNotNull = false}
#else
                {L"ID", L"id"sv, false, kRepresentIDAs_, nullopt, true, nullopt, L"randomblob(16)"sv, true},
                {L"name", nullopt, false, VariantValue::eString, nullopt, nullopt, nullopt, nullopt, false}
#endif
            },
            Schema::CatchAllField{}};
        static_assert (kRepresentIDAs_ == VariantValue::eBLOB); // @todo to support string, just change '.fDefaultExpression'

        void addDevice (Connection::Ptr conn, const IntegratedModel::Device& d)
        {
            Statement addDeviceStatement{conn, Schema::StandardSQLStatements{kDeviceTableSchema_}.Insert ()};
            addDeviceStatement.Bind (kDeviceTableSchema_.MapToDB (kDBObjectMapper_->FromObject (d).As<Mapping<String, VariantValue>> ()));
            DbgTrace (L"sqlorg = %s", addDeviceStatement.GetSQL ().c_str ());
            DbgTrace (L"sqlorgexpanded = %s", addDeviceStatement.GetSQL (Statement::WhichSQLFlag::eExpanded).c_str ());
            addDeviceStatement.Execute ();
        }
        optional<IntegratedModel::Device> getDeviceByID (Connection::Ptr conn, const GUID& id)
        {
            using DataExchange::VariantValue;
            using IntegratedModel::Device;
            using Stroika::Foundation::Common::KeyValuePair;
            Statement getDeviceStatement{conn, Schema::StandardSQLStatements{kDeviceTableSchema_}.GetByID ()};
            getDeviceStatement.Bind (initializer_list<KeyValuePair<String, VariantValue>>{{kDeviceTableSchema_.GetIDField ()->fName, VariantValue{kDBObjectMapper_->FromObject (id)}}});

            DbgTrace (L"sqlorg = %s", getDeviceStatement.GetSQL ().c_str ());
            DbgTrace (L"sqlorgexpanded = %s", getDeviceStatement.GetSQL (Statement::WhichSQLFlag::eExpanded).c_str ());
            auto rows = getDeviceStatement.GetAllRows ()
                            .Select<Device> ([] (const Statement::Row& r) {
                                return kDBObjectMapper_->ToObject<Device> (VariantValue{kDeviceTableSchema_.MapFromDB (r)});
                            });
            if (rows.empty ()) {
                return nullopt;
            }
            Ensure (rows.size () == 1); // cuz arg sb a key
            return *rows.First ();
        }
        void addOrUpdateDevice (Connection::Ptr conn, const IntegratedModel::Device& d)
        {
            Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...addOrUpdateDevice", L"...,d=%s", Characters::ToString (d).c_str ())};
            using IntegratedModel::Device;
            if (d.name.StartsWith (L"John")) {
                int breakhere = 1;
            }
            if (auto dbDev = getDeviceByID (conn, d.fGUID)) {
                Device    merged = Device::Merge (*dbDev, d);
                Statement updateDeviceStatement{conn, Schema::StandardSQLStatements{kDeviceTableSchema_}.UpdateByID ()};
                updateDeviceStatement.Execute (kDeviceTableSchema_.MapToDB (kDBObjectMapper_->FromObject (merged).As<Mapping<String, VariantValue>> ()));
            }
            else {
                try {
                    addDevice (conn, d);
                }
                catch (...) {

                    auto dbDevx = getDeviceByID (conn, d.fGUID);

                    throw;
                }
            }
        }
        void addNetwork (Connection::Ptr conn, const IntegratedModel::Network& n)
        {
            Statement addDeviceStatement{conn, Schema::StandardSQLStatements{kNetworkTableSchema_}.Insert ()};
            addDeviceStatement.Execute (kNetworkTableSchema_.MapToDB (kDBObjectMapper_->FromObject (n).As<Mapping<String, VariantValue>> ()));
        }

        Connection::Ptr SetupDB_ ()
        {
            auto dbPath = IO::FileSystem::WellKnownLocations::GetApplicationData () / "WhyTheFuckIsMyNetworkSoSlow" / "db-v1.db";
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
            Connection::Ptr conn;
            while (true) {
                try {
                    if (conn == nullptr) {
                        conn = SetupDB_ ();
                    }
                }
                catch (const Thread::AbortException&) {
                    Execution::ReThrow ();
                }
                catch (...) {
                    DbgTrace (L"Ignoring exception in BackgroundDatabaseThread_ loop: %s", Characters::ToString (current_exception ()).c_str ());
                }
                Execution::Sleep (30s); // quick hack - write out what we have every 30 seconds
                try {
                    // quick hack - use WSAPI to fetch all networsk
                    for (auto ni : IntegratedModel::Mgr::sThe.GetNetworks ()) {
                        addNetwork (conn, ni);
                    }
                }
                catch (const Thread::AbortException&) {
                    Execution::ReThrow ();
                }
                catch (...) {
                    DbgTrace (L"Ignoring exception in BackgroundDatabaseThread_ loop: %s", Characters::ToString (current_exception ()).c_str ());
                }
                try {
                    // quick hack - use WSAPI to fetch all networks
                    for (auto di : IntegratedModel::Mgr::sThe.GetDevices ()) {
                        addOrUpdateDevice (conn, di);
                    }
                }
                catch (const Thread::AbortException&) {
                    Execution::ReThrow ();
                }
                catch (...) {
                    DbgTrace (L"Ignoring exception in BackgroundDatabaseThread_ loop: %s", Characters::ToString (current_exception ()).c_str ());
                }
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

    // Fetch (UNSORTED) list of devices
    Sequence<Device> devices = Sequence<Device>{Discovery::DevicesMgr::sThe.GetActiveDevices ().Select<Device> ([] (const Discovery::Device& d) {
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
    return devices;
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
    Debug::TimingTrace ttrc{L"IntegratedModel::Mgr::GetNetworks", 0.1};
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