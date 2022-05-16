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
    const BackendApp::Common::InternetServiceProvider kHughsNet_ISP_{L"Hughes Network Systems"sv};
}

namespace {
    struct Device_Key_Extractor_ {
        GUID operator() (const IntegratedModel::Device& t) const { return t.fGUID; };
    };
    using DeviceKeyedCollection_ = KeyedCollection<IntegratedModel::Device, GUID, KeyedCollection_DefaultTraits<IntegratedModel::Device, GUID, Device_Key_Extractor_>>;

    struct Network_Key_Extractor_ {
        GUID operator() (const IntegratedModel::Network& t) const { return t.fGUID; };
    };
    using NetworkKeyedCollection_ = KeyedCollection<IntegratedModel::Network, GUID, KeyedCollection_DefaultTraits<IntegratedModel::Network, GUID, Network_Key_Extractor_>>;
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
            for (const Discovery::Network& n : Discovery::NetworksMgr::sThe.CollectActiveNetworks ()) {
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
                for (const auto& i : d.fAttachedNetworks) {
                    constexpr bool            kIncludeLinkLocalAddresses_{Discovery::kIncludeLinkLocalAddressesInDiscovery};
                    constexpr bool            kIncludeMulticastAddreses_{Discovery::kIncludeMulticastAddressesInDiscovery};
                    Sequence<InternetAddress> addrs2Report;
                    for (const auto& li : i.fValue.localAddresses) {
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
                    for (const auto& i : d.fAttachedNetworks) {
                        for (const auto& hwa : i.fValue.hardwareAddresses) {
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
        bool ShouldRollup_ (const Device& exisingRolledUpDevice, const Device& d2)
        {
            if ((exisingRolledUpDevice.fAggregatesIrreversibly and exisingRolledUpDevice.fAggregatesIrreversibly->Contains (d2.fGUID))
                or (exisingRolledUpDevice.fAggregatesIrreversibly and exisingRolledUpDevice.fAggregatesIrreversibly->Contains (d2.fGUID))
                ) {
                // we retry the same 'discovered' networks repeatedly and re-roll them up.
                // mostly this is handled by having the same hardware addresses, but sometimes (like for main discovered device)
                // MAY not yet / always have network interface). And besides, this check cheaper/faster probably.
                return true;
            }
            // very rough first draft. Later add database stored 'exceptions' and/or rules tables to augment this logic
            auto hw1 = exisingRolledUpDevice.GetHardwareAddresses ();
            auto hw2 = d2.GetHardwareAddresses ();
            if (Set<String>::Intersects (hw1, hw2)) {
                return true;
            }
            // If EITHER device has no hardware addresses, there is little to identify it, so roll it up with anything with the same IP address, by default
            if (hw1.empty () or hw2.empty ()) {
                // then fold togehter if they have the same IP Addresses
                // return d1.GetInternetAddresses () == d2.GetInternetAddresses ();
                return Set<InternetAddress>::Intersects (exisingRolledUpDevice.GetInternetAddresses (), d2.GetInternetAddresses ());
            }
            // unclear if above test should be if EITHER set is empty, maybe then do if timeframes very close?
            return false;
        }
        bool ShouldRollup_ (const Network& exisingRolledUpNet, const Network& n2)
        {
            if ((exisingRolledUpNet.fAggregatesIrreversibly and exisingRolledUpNet.fAggregatesIrreversibly->Contains (n2.fGUID))
                or (exisingRolledUpNet.fAggregatesIrreversibly and exisingRolledUpNet.fAggregatesIrreversibly->Contains (n2.fGUID))
                ) {
                // we retry the same 'discovered' networks repeatedly and re-roll them up
                return true;
            }
            /*
             *  A network is not a super-well defined concept, so deciding if two instances of a network refer to the same
             *  network is a bit of a judgement call.
             * 
             *  BUt a few key things probably make sense:
             *      >   Same ISP
             *      >   Same GeoLoc (with exceptions)
             *      >   Same IPv4 CIDR
             *      >   Same Gateway addresses
             * 
             *  Things we allow to differ:
             *      >   details of any IP-v6 network addresses (if there were IPV4 CIDRs agreed upon).
             * 
             *  At least thats by best guess to start as of 2021-08-29
             */
            if (exisingRolledUpNet.fInternetServiceProvider != n2.fInternetServiceProvider) {
                return false;
            }
            {
                // Hughsnet makes geoloc information comparison ineffective (frequently generates many different locations depending on when you try)
                // @todo add mobile networks like verizon wireless
                bool networkAllowedToChangeGeoLoc = false;
                if (exisingRolledUpNet.fInternetServiceProvider == kHughsNet_ISP_) {
                    networkAllowedToChangeGeoLoc = true;
                }
                if (not networkAllowedToChangeGeoLoc and exisingRolledUpNet.fGEOLocInformation != n2.fGEOLocInformation) {
                    return false;
                }
            }
            if (exisingRolledUpNet.fGateways != n2.fGateways) {
                // for some reason, gateway list sometimes contains IPv4 only, and sometimes IPv4 and IPv6 addresses
                // treat the list the same if the gateway list ipv4s at least are the same (and non-empty)
                // if IPv4 CIDRs same (and non-empty), then ignore differences in IPv4 addressses
                Set<InternetAddress> ipv4s1{exisingRolledUpNet.fGateways.Where ([] (const InternetAddress& i) { return i.GetAddressFamily () == InternetAddress::AddressFamily::V4; })};
                Set<InternetAddress> ipv4s2{n2.fGateways.Where ([] (const InternetAddress& i) { return i.GetAddressFamily () == InternetAddress::AddressFamily::V4; })};
                if (ipv4s1 != ipv4s2) {
                    return false;
                }
                // If we got here, they differ in IPv6 (or other) address. If they matched on IPV4 (not trivially - because there were none)
                // ignore the (ipv6) differences
                if (ipv4s1.empty ()) {
                    return false;
                }
            }
            if (exisingRolledUpNet.fNetworkAddresses != n2.fNetworkAddresses) {
                // if IPv4 CIDRs same (and non-empty), then ignore differences in IPv4 addressses
                Set<CIDR> ipv4s1{exisingRolledUpNet.fNetworkAddresses.Where ([] (const auto& i) { return i.GetBaseInternetAddress ().GetAddressFamily () == InternetAddress::AddressFamily::V4; })};
                Set<CIDR> ipv4s2{n2.fNetworkAddresses.Where ([] (const auto& i) { return i.GetBaseInternetAddress ().GetAddressFamily () == InternetAddress::AddressFamily::V4; })};
                // However, sometimes we have PRIVATE (internal) networks that float their addresses, like WSL etc) so if both private, and
                // have same name, then treat that as a rollup too
                if (not ipv4s1.empty ()) {
                    if (
                        exisingRolledUpNet.fFriendlyName == n2.fFriendlyName and exisingRolledUpNet.fNetworkAddresses.All ([] (const auto& i) { return i.GetBaseInternetAddress ().IsPrivateAddress (); }) and n2.fNetworkAddresses.All ([] (const auto& i) { return i.GetBaseInternetAddress ().IsPrivateAddress (); })) {
                        return true;
                    }
                }
                if (ipv4s1 != ipv4s2) {
                    return false;
                }
                // If we got here, they differ in IPv6 (or other) address. If they matched on IPV4 (not trivially - because there were none)
                // ignore the (ipv6) differences
                if (ipv4s1.empty ()) {
                    return false;
                }
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

        // the latest copy of what is in the DB (manually kept up to date)
        Synchronized<DeviceKeyedCollection_>  sDBDevices_;
        Synchronized<NetworkKeyedCollection_> sDBNetworks_;

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
                {.fName = L"ID"sv, .fVariantValueName = L"id"sv, .fRequired = true, .fVariantValueType = kRepresentIDAs_, .fIsKeyField = true, .fDefaultExpression = L"randomblob(16)"sv},
                {.fName = L"name"sv, .fVariantValueType = VariantValue::eString},
                {.fName = L"lastSeenAt"sv, .fVariantValueType = VariantValue::eString},
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
                {.fName = L"ID"sv, .fVariantValueName = L"id"sv, .fRequired = true, .fVariantValueType = kRepresentIDAs_, .fIsKeyField = true, .fDefaultExpression = L"randomblob(16)"sv},
                {.fName = L"friendlyName"sv, .fVariantValueType = VariantValue::eString},
                {.fName = L"lastSeenAt"sv, .fVariantValueType = VariantValue::eString},
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
            auto dbPath = IO::FileSystem::WellKnownLocations::GetApplicationData () / "WhyTheFuckIsMyNetworkSoSlow" / "db-v7.db";
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
#if qDefaultTracingOn
            bool madeItToEndOfLoadDBCode = false;
#endif
            while (true) {
                try {
                    if (conn == nullptr) {
                        conn = SetupDB_ ();
                    }
                    // load networks before devices because devices depend on networks but not the reverse
                    if (networkTableConnection == nullptr) {
                        networkTableConnection = make_unique<SQL::ORM::TableConnection<IntegratedModel::Network>> (conn, kNetworkTableSchema_, kDBObjectMapper_);
                        try {
                            Debug::TimingTrace ttrc{L"...initial load of sDBNetworks_ from database ", 1};
                            sDBNetworks_.store (NetworkKeyedCollection_{networkTableConnection->GetAll ()});
                        }
                        catch (...) {
                            networkTableConnection = nullptr; // so we re-fetch
                            Execution::ReThrow ();
                        }
                    }
                    if (deviceTableConnection == nullptr) {
                        deviceTableConnection = make_unique<SQL::ORM::TableConnection<IntegratedModel::Device>> (conn, kDeviceTableSchema_, kDBObjectMapper_);
                        try {
                            Debug::TimingTrace ttrc{L"...initial load of sDBDevices_ from database ", 1};
                            sDBDevices_.store (DeviceKeyedCollection_{deviceTableConnection->GetAll ()}); // pre-load in memory copy with whatever we had stored in the database
                        }
                        catch (...) {
                            deviceTableConnection = nullptr; // so we re-fetch
                            Execution::ReThrow ();
                        }
                    }
#if qDefaultTracingOn
                    if (not madeItToEndOfLoadDBCode) {
                        DbgTrace (L"Completed initial database load of sDBDevices_ and sDBNetworks_");
                        madeItToEndOfLoadDBCode = true;
                    }
#endif
                    // periodically write the latest discovered data to the database

                    // UPDATE sDBNetworks_ INCREMENTALLY to reflect reflect these merges
                    for (auto ni : DiscoveryWrapper_::GetNetworks_ ()) {
                        if (not kSupportPersistedNetworkInterfaces_) {
                            ni.fAttachedInterfaces.clear ();
                        }
                        auto rec2Update = AddOrMergeUpdate_ (networkTableConnection.get (), ni);
                        sDBNetworks_.rwget ()->Add (rec2Update);
                    }

                    // UPDATE sDBDevices_ INCREMENTALLY to reflect reflect these merges
                    for (auto di : DiscoveryWrapper_::GetDevices_ ()) {
                        if (not kSupportPersistedNetworkInterfaces_) {
                            di.fAttachedNetworkInterfaces = nullopt;
                        }
                        auto rec2Update = AddOrMergeUpdate_ (deviceTableConnection.get (), di);
                        sDBDevices_.rwget ()->Add (rec2Update);
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

        struct RolledUpNetworks {
            // @todo add much more here - different useful summaries of same info
            NetworkKeyedCollection_ fNetworks;
        };
        RolledUpNetworks GetRolledUpNetworks (Time::DurationSecondsType allowedStaleness = 5.0);

        struct RolledUpDevices {
            // @todo add much more here - different useful summaries of same info
            DeviceKeyedCollection_ fDevices;
        };
        RolledUpDevices GetRolledUpDevies (Time::DurationSecondsType allowedStaleness = 10.0)
        {
            Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...GetRolledUpDevies")};
            Debug::TimingTrace        ttrc{L"GetRolledUpDevies", 1};
            // SynchronizedCallerStalenessCache object just assures one rollup RUNS internally at a time, and
            // that two calls in rapid succession, the second call re-uses the previous value
            static Cache::SynchronizedCallerStalenessCache<void, RolledUpDevices> sCache_;
            // Disable this cache setting due to https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/23
            // See also
            //      https://stroika.atlassian.net/browse/STK-906 - possible enhancement to this configuration to work better avoiding
            //      See https://stroika.atlassian.net/browse/STK-907 - about needing some new mechanism in Stroika for deadlock detection/avoidance.
            // sCache_.fHoldWriteLockDuringCacheFill = true; // so only one call to filler lambda at a time
            return sCache_.LookupValue (sCache_.Ago (allowedStaleness), [=] () -> RolledUpDevices {
                Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...GetRolledUpDevies...cachefiller")};
                Debug::TimingTrace        ttrc{L"GetRolledUpDevies...cachefiller", 1};
                static RolledUpDevices    sRolledUpDevices_; // keep always across runs so we have consisent IDs

                auto rolledUpNetworks = GetRolledUpNetworks (allowedStaleness * 10.0); // longer allowedStaleness cuz we dont care much about this and the parts
                                                                                       // we look at really dont change

                //tmphack slow impl - instead build mapping table when constructing rollup
                // of networkinfo
                // @todo define struct for NetworksRollup (and devices) that has above map, and the REVERSE ID map
                // (individual 2 rollup), and then provide funciton to map a set of IDs to their rolled up IDs
                // AND DOCUMENT WHY GUARNATEED UNIQUE - IF AS I THINK IT IS - CUZ each item goes in one rollup)
                auto mapAggregatedNetID2ItsRollupID = [&] (const GUID& netID) -> GUID {
                    for (const auto& i : rolledUpNetworks.fNetworks) {
                        if (i.fAggregatesReversibly and i.fAggregatesReversibly->Contains (netID)) {
                            return i.fGUID;
                        }
                    }
                    // address- but not yet, as we get this too often, and too noisy in logs - WeakAsserteNotReached ();
                    //AssertNotReached ();    // because we guarantee each item rolled up exactly once
                    return netID;
                };
                auto reverseRollup = [&] (const Mapping<GUID, NetworkAttachmentInfo>& nats) -> Mapping<GUID, NetworkAttachmentInfo> {
                    Mapping<GUID, NetworkAttachmentInfo> result;
                    for (const auto& ni : nats) {
                        result.Add (mapAggregatedNetID2ItsRollupID (ni.fKey), ni.fValue);
                    }
                    return result;
                };

                // Start with the existing rolled up devices
                // and then add in (should be done just once) the values from the database,
                // and then keep adding any more recent discovery changes
                RolledUpDevices result               = sRolledUpDevices_;
                auto            doMergeOneIntoRollup = [&result, &reverseRollup] (const Device& d2MergeIn) {
                    // @todo slow/quadradic - may need to tweak
                    if (auto i = result.fDevices.Find ([&d2MergeIn] (auto const& exisingRolledUpDevice) { return ShouldRollup_ (exisingRolledUpDevice, d2MergeIn); })) {
                        // then merge this into that item
                        // @todo think out order of params and better document order of params!
                        auto tmp              = Device::Rollup (*i, d2MergeIn);
                        tmp.fAttachedNetworks = reverseRollup (tmp.fAttachedNetworks);
                        result.fDevices.Add (tmp);
                    }
                    else {
                        Device newRolledUpDevice                = d2MergeIn;
                        newRolledUpDevice.fAggregatesReversibly = Set<GUID>{d2MergeIn.fGUID};
                        newRolledUpDevice.fGUID                 = GUID::GenerateNew ();
                        result.fDevices.Add (newRolledUpDevice);
                    }
                };
                static bool sDidMergeFromDatabase_ = false; // no need to roll these up more than once
                if (not sDidMergeFromDatabase_) {
                    for (const auto& rdi : DBAccess_::sDBDevices_.load ()) {
                        doMergeOneIntoRollup (rdi);
                        sDidMergeFromDatabase_ = true;
                    }
                }
                for (const Device& d : DiscoveryWrapper_::GetDevices_ ()) {
                    doMergeOneIntoRollup (d);
                }
                sRolledUpDevices_ = result;
                return result;
            });
        }

        RolledUpNetworks GetRolledUpNetworks (Time::DurationSecondsType allowedStaleness)
        {
            Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...GetRolledUpNetworks")};
            Debug::TimingTrace        ttrc{L"GetRolledUpNetworks", 1};
            // SynchronizedCallerStalenessCache object just assures one rollup RUNS internally at a time, and
            // that two calls in rapid succession, the second call re-uses the previous value
            static Cache::SynchronizedCallerStalenessCache<void, RolledUpNetworks> sCache_;
            // Disable this cache setting due to https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/23
            // See also
            //      https://stroika.atlassian.net/browse/STK-906 - possible enhancement to this configuration to work better avoiding
            //      See https://stroika.atlassian.net/browse/STK-907 - about needing some new mechanism in Stroika for deadlock detection/avoidance.
            // sCache_.fHoldWriteLockDuringCacheFill = true; // so only one call to filler lambda at a time
            return sCache_.LookupValue (sCache_.Ago (allowedStaleness), [] () -> RolledUpNetworks {

                /*
                 *  DEADLOCK NOTE
                 *      Since this can be called while rolling up DEVICES, its important that this code not call anything involving device rollup since
                 *      that could trigger a deadlock.
                 */
                Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...GetRolledUpNetworks...cachefiller")};
                Debug::TimingTrace        ttrc{L"GetRolledUpNetworks...cachefiller", 1};

                static RolledUpNetworks sRolledUpNetworks_;
                // Start with the existing rolled up devices
                // and then add in (should be done just once) the values from the database,
                // and then keep adding any more recent discovery changes
                RolledUpNetworks result               = sRolledUpNetworks_;
                auto             doMergeOneIntoRollup = [&result] (const Network& net2MergeIn) {
                    // @todo slow/quadradic - may need to tweak
                    if (auto i = result.fNetworks.Find ([&net2MergeIn] (auto const& exisingRolledUpNet) { return ShouldRollup_ (exisingRolledUpNet, net2MergeIn); })) {
                        // then merge this into that item
                        // @todo think out order of params and better document order of params!
                        result.fNetworks.Add (Network::Rollup (*i, net2MergeIn));
                    }
                    else {
                        Network newRolledUpDevice               = net2MergeIn;
                        newRolledUpDevice.fAggregatesReversibly = Set<GUID>{net2MergeIn.fGUID};
                        newRolledUpDevice.fGUID                 = GUID::GenerateNew ();
                        result.fNetworks.Add (newRolledUpDevice);
                    }
                };
                static bool sDidMergeFromDatabase_ = false; // no need to roll these up more than once
                if (not sDidMergeFromDatabase_) {
                    for (const auto& rdi : DBAccess_::sDBNetworks_.load ()) {
                        doMergeOneIntoRollup (rdi);
                        sDidMergeFromDatabase_ = true;
                    }
                }
                for (const Network& d : DiscoveryWrapper_::GetNetworks_ ()) {
                    doMergeOneIntoRollup (d);
                }
                sRolledUpNetworks_ = result;
                return result;
            });
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
    Execution::Thread::SuppressInterruptionInContext suppressInterruption; // must complete this abort and wait for done - this cannot abort/throw
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
    return Sequence<IntegratedModel::Device>{RollupSummary_::GetRolledUpDevies ().fDevices};
}

optional<IntegratedModel::Device> IntegratedModel::Mgr::GetDevice (const Common::GUID& id) const
{
    // first check rolled up networks, and then raw/unrolled up networks
    auto result = RollupSummary_::GetRolledUpDevies ().fDevices.Lookup (id);
    if (not result.has_value ()) {
        result = DBAccess_::sDBDevices_.load ().Lookup (id);
        if (result) {
            result->fIDPersistent       = true;
            result->fHistoricalSnapshot = true;
        }
    }
    return result;
}

Sequence<IntegratedModel::Network> IntegratedModel::Mgr::GetNetworks () const
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"IntegratedModel::Mgr::GetNetworks")};
    Debug::TimingTrace        ttrc{L"IntegratedModel::Mgr::GetNetworks", 0.1};
    return Sequence<IntegratedModel::Network>{RollupSummary_::GetRolledUpNetworks ().fNetworks};
}

optional<IntegratedModel::Network> IntegratedModel::Mgr::GetNetwork (const Common::GUID& id) const
{
    // first check rolled up networks, and then raw/unrolled up networks
    auto result = RollupSummary_::GetRolledUpNetworks ().fNetworks.Lookup (id);
    if (not result.has_value ()) {
        result = DBAccess_::sDBNetworks_.load ().Lookup (id);
        if (result) {
            result->fIDPersistent       = true;
            result->fHistoricalSnapshot = true;
        }
    }
    return result;
}

Collection<IntegratedModel::NetworkInterface> IntegratedModel::Mgr::GetNetworkInterfaces () const
{
    Collection<NetworkInterface> result;
    for (const Discovery::NetworkInterface& n : Discovery::NetworkInterfacesMgr::sThe.CollectAllNetworkInterfaces ()) {
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
    for (const auto& i : GetNetworkInterfaces ()) {
        if (i.fGUID == id) {
            return i;
        }
    }
    return nullopt;
}