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
#include "Stroika/Foundation/Execution/Logger.h"
#include "Stroika/Foundation/Execution/Sleep.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/Execution/TimeOutException.h"
#include "Stroika/Foundation/IO/FileSystem/WellKnownLocations.h"

#include "../Common/BLOBMgr.h"
#include "../Common/DB.h"
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
using Stroika::Foundation::Traversal::Range;

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
                return URI{nullopt, nullopt, L"/api/v1/blob/" + g->ToString ()};
            }
        }
        catch (const std::system_error& e) {
            Logger::sThe.Log (Logger::eWarning, L"Database update: ignoring exception in TransformURL2LocalStorage_: %s", Characters::ToString (e).c_str ());
            Assert (e.code () == errc::device_or_resource_busy); // this can happen talking to database (SQLITE_BUSY or SQLITE_LOCKED)
                                                                 // might be better to up timeout so more rare
        }
        catch (const Thread::AbortException&) {
            Execution::ReThrow ();
        }
        catch (...) {
            Logger::sThe.Log (Logger::eWarning, L"Database update: ignoring exception in TransformURL2LocalStorage_: %s", Characters::ToString (current_exception ()).c_str ());
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
        // Map all the 'Discovery::Network' objects to 'Model::Network' objects.
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
                nw.fSeen                    = Range<DateTime>{now, now}; // discovered now, when rolled up, expand timeframe
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
                newDev.fGUID  = d.fGUID;
                newDev.fNames = d.fNames;
                if (not d.fTypes.empty ()) {
                    newDev.fTypes = d.fTypes; // leave missing if no discovered types
                }
                newDev.fSeen.fARP       = d.fSeen.fARP;
                newDev.fSeen.fCollector = d.fSeen.fCollector;
                newDev.fSeen.fICMP      = d.fSeen.fICMP;
                newDev.fSeen.fTCP       = d.fSeen.fTCP;
                newDev.fSeen.fUDP       = d.fSeen.fUDP;
                Assert (newDev.fSeen.EverSeen ()); // for now don't allow 'discovering' a device without having some initial data for some activity
                newDev.fOpenPorts = d.fOpenPorts;
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
                Assert (newDev.fSeen.EverSeen ()); // maybe won't always require but look into any cases like this and probably remove them...
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
            if ((exisingRolledUpDevice.fAggregatesIrreversibly and exisingRolledUpDevice.fAggregatesIrreversibly->Contains (d2.fGUID)) or (exisingRolledUpDevice.fAggregatesIrreversibly and exisingRolledUpDevice.fAggregatesIrreversibly->Contains (d2.fGUID))) {
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
                // then fold together if they have the same IP Addresses
                // return d1.GetInternetAddresses () == d2.GetInternetAddresses ();
                return Set<InternetAddress>::Intersects (exisingRolledUpDevice.GetInternetAddresses (), d2.GetInternetAddresses ());
            }
            // unclear if above test should be if EITHER set is empty, maybe then do if timeframes very close?
            return false;
        }
        bool ShouldRollup_ (const Network& exisingRolledUpNet, const Network& n2)
        {
            if ((exisingRolledUpNet.fAggregatesIrreversibly and exisingRolledUpNet.fAggregatesIrreversibly->Contains (n2.fGUID)) or (exisingRolledUpNet.fAggregatesIrreversibly and exisingRolledUpNet.fAggregatesIrreversibly->Contains (n2.fGUID))) {
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

        // the latest copy of what is in the DB (manually kept up to date)
        // NOTE: These are all non-rolled up objects
        Synchronized<Mapping<String, GUID>>          sAdvisoryHWAddr2GUIDCache;
        Synchronized<Mapping<InternetAddress, GUID>> sAdvisoryExternalIPAddr2NetworkGUIDCache;
        Synchronized<DeviceKeyedCollection_>         sDBDevices_;
        Synchronized<NetworkKeyedCollection_>        sDBNetworks_;
        atomic<bool>                                 sFinishedInitialDBLoad_{false};

        namespace Private_ {
            constexpr VariantValue::Type kRepresentIDAs_ = VariantValue::Type::eBLOB; // else as string

            struct HWAddr2GUIDElt_ {
                String HWAddress;
                GUID   DeviceID;
            };

            struct ExternalIPAddr2NetGUIDElt_ {
                InternetAddress ExternalNetworkAddress;
                GUID            NetworkID;
            };

            struct ExternalDeviceUserSettingsElt_ {
                GUID                             fDeviceID; // rolled up device id
                Model::Device::UserOverridesType fUserSettings;
            };

            struct ExternalNetworkUserSettingsElt_ {
                GUID                              fNetworkID; // rolled up network id
                Model::Network::UserOverridesType fUserSettings;
            };

            /*
             *  Combined mapper for objects we write to the database. Contains all the objects mappers we need merged together,
             *  and any touchups on represenation we need (like writing GUID as BLOB rather than string).
             */
            const ConstantProperty<ObjectVariantMapper> kDBObjectMapper_{[] () {
                ObjectVariantMapper mapper;

                mapper += IntegratedModel::Device::kMapper;
                mapper += IntegratedModel::Network::kMapper;

                //tmphack need better way to make this context sensative!!! @todo SOON FIX
                mapper.AddCommonType<Range<DateTime>> (ObjectVariantMapper::RangeSerializerOptions{L"lowerBound"sv, L"upperBound"sv}); // lower-camel-case names happier in javascript?

                mapper.AddClass<HWAddr2GUIDElt_> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
                    {L"HWAddress", StructFieldMetaInfo{&HWAddr2GUIDElt_::HWAddress}},
                    {L"DeviceID", StructFieldMetaInfo{&HWAddr2GUIDElt_::DeviceID}},
                });
                mapper.AddClass<ExternalIPAddr2NetGUIDElt_> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
                    {L"ExternalNetworkAddress", StructFieldMetaInfo{&ExternalIPAddr2NetGUIDElt_::ExternalNetworkAddress}},
                    {L"NetworkID", StructFieldMetaInfo{&ExternalIPAddr2NetGUIDElt_::NetworkID}},
                });
                mapper.AddClass<ExternalDeviceUserSettingsElt_> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
                    {L"UserSettings", StructFieldMetaInfo{&ExternalDeviceUserSettingsElt_::fUserSettings}},
                    {L"DeviceID", StructFieldMetaInfo{&ExternalDeviceUserSettingsElt_::fDeviceID}},
                });
                mapper.AddClass<ExternalNetworkUserSettingsElt_> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
                    {L"UserSettings", StructFieldMetaInfo{&ExternalNetworkUserSettingsElt_::fUserSettings}},
                    {L"NetworkID", StructFieldMetaInfo{&ExternalNetworkUserSettingsElt_::fNetworkID}},
                });

                // ONLY DO THIS FOR WHEN WRITING TO DB -- store GUIDs as BLOBs - at least for database interactions (cuz probably more efficient)
                mapper.AddCommonType<GUID> (kRepresentIDAs_);

                return mapper;
            }};
            const Schema::Table                         kDeviceIDCacheTableSchema_{
                L"DevicesIDCache"sv,
                /*
                 */
                Collection<Schema::Field>{
#if __cpp_designated_initializers
                    {.fName = L"HWAddress"sv, .fRequired = true, .fIsKeyField = true},
                    {.fName = L"DeviceID"sv, .fRequired = true, .fVariantValueType = kRepresentIDAs_},
#else
                    {L"HWAddress", nullopt, true, VariantValue::eString, nullopt, true},
                    {L"DeviceID", nullopt, true, kRepresentIDAs_, nullopt, false},
#endif
                }};
            const Schema::Table kNetworkIDCacheTableSchema_{
                L"NetworkIDCache"sv,
                /*
                 */
                Collection<Schema::Field>{
#if __cpp_designated_initializers
                    {.fName = L"ExternalNetworkAddress"sv, .fRequired = true, .fIsKeyField = true},
                    {.fName = L"NetworkID"sv, .fRequired = true, .fVariantValueType = kRepresentIDAs_},
#else
                    {L"ExternalNetworkAddress", nullopt, true, VariantValue::eString, nullopt, true},
                    {L"NetworkID", nullopt, true, kRepresentIDAs_, nullopt, false},
#endif
                }};
            const Schema::Table kDeviceUserSettingsSchema_{
                L"DeviceUserSettings"sv,
                /*
                 */
                Collection<Schema::Field>{
#if __cpp_designated_initializers
                    {.fName = L"DeviceID"sv, .fRequired = true, .fVariantValueType = kRepresentIDAs_, .fIsKeyField = true},
#else
                    {L"DeviceID", nullopt, true, kRepresentIDAs_, nullopt, true},
#endif
                },
                Schema::CatchAllField{}};
            const Schema::Table kNetworkUserSettingsSchema_{
                L"NetworkUserSettings"sv,
                /*
                 */
                Collection<Schema::Field>{
#if __cpp_designated_initializers
                    {.fName = L"NetworkID"sv, .fRequired = true, .fVariantValueType = kRepresentIDAs_, .fIsKeyField = true},
#else
                    {L"NetworkID", nullopt, true, kRepresentIDAs_, nullopt, true},
#endif
                },
                Schema::CatchAllField{}};
            const Schema::Table kDeviceTableSchema_{
                L"Devices"sv,
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
#else
                    {L"ID", L"id"sv, true, kRepresentIDAs_, nullopt, true, nullopt, L"randomblob(16)"sv},
                    {L"name", nullopt, false, VariantValue::eString},
#endif
                },
                Schema::CatchAllField{}};
            const Schema::Table kNetworkTableSchema_{
                L"Networks"sv,
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
#else
                    {L"ID", L"id"sv, true, kRepresentIDAs_, nullopt, true, nullopt, L"randomblob(16)"sv},
                    {L"friendlyName", nullopt, false, VariantValue::eString},
#endif
                },
                Schema::CatchAllField{}};
            static_assert (kRepresentIDAs_ == VariantValue::eBLOB); // @todo to support string, just change '.fDefaultExpression'
        }

        class Mgr_ {
        private:
            static constexpr Configuration::Version kCurrentVersion_ = Configuration::Version{1, 0, Configuration::VersionStage::Alpha, 0};
            BackendApp::Common::DB                  fDB_{
                kCurrentVersion_,
                Traversal::Iterable<Database::SQL::ORM::Schema::Table>{Private_::kDeviceIDCacheTableSchema_, Private_::kNetworkIDCacheTableSchema_, Private_::kDeviceTableSchema_, Private_::kDeviceUserSettingsSchema_, Private_::kNetworkTableSchema_, Private_::kNetworkUserSettingsSchema_}};
            Synchronized<SQL::Connection::Ptr>                                                             fDBConnectionPtr_{fDB_.NewConnection ()};
            Execution::Thread::Ptr                                                                         fDatabaseSyncThread_{};
            unique_ptr<SQL::ORM::TableConnection<Private_::HWAddr2GUIDElt_>>                               fHWAddr2GUIDCacheTableConnection_;
            unique_ptr<SQL::ORM::TableConnection<Private_::ExternalIPAddr2NetGUIDElt_>>                    fExternalIPAddr2NetGUIDCacheTableConnection_;
            Synchronized<Mapping<GUID, IntegratedModel::Device::UserOverridesType>>                        fCachedDeviceUserSettings_;
            Synchronized<unique_ptr<SQL::ORM::TableConnection<Private_::ExternalDeviceUserSettingsElt_>>>  fDeviceUserSettingsTableConnection_;
            Synchronized<Mapping<GUID, IntegratedModel::Network::UserOverridesType>>                       fCachedNetworkUserSettings_;
            Synchronized<unique_ptr<SQL::ORM::TableConnection<Private_::ExternalNetworkUserSettingsElt_>>> fNetworkUserSettingsTableConnection_;

        public:
            Mgr_ ()
            {
                Debug::TraceContextBumper ctx{L"IntegratedModel::{}::Mgr_::CTOR"};
                fHWAddr2GUIDCacheTableConnection_            = make_unique<SQL::ORM::TableConnection<Private_::HWAddr2GUIDElt_>> (fDBConnectionPtr_, Private_::kDeviceIDCacheTableSchema_, Private_::kDBObjectMapper_, BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<Private_::HWAddr2GUIDElt_>> ());
                fExternalIPAddr2NetGUIDCacheTableConnection_ = make_unique<SQL::ORM::TableConnection<Private_::ExternalIPAddr2NetGUIDElt_>> (fDBConnectionPtr_, Private_::kNetworkIDCacheTableSchema_, Private_::kDBObjectMapper_, BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<Private_::ExternalIPAddr2NetGUIDElt_>> ());
                fDeviceUserSettingsTableConnection_          = make_unique<SQL::ORM::TableConnection<Private_::ExternalDeviceUserSettingsElt_>> (fDBConnectionPtr_, Private_::kDeviceUserSettingsSchema_, Private_::kDBObjectMapper_, BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<Private_::ExternalDeviceUserSettingsElt_>> ());
                fNetworkUserSettingsTableConnection_         = make_unique<SQL::ORM::TableConnection<Private_::ExternalNetworkUserSettingsElt_>> (fDBConnectionPtr_, Private_::kNetworkUserSettingsSchema_, Private_::kDBObjectMapper_, BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<Private_::ExternalNetworkUserSettingsElt_>> ());

                try {
                    Debug::TimingTrace ttrc{L"...load of sAdvisoryHWAddr2GUIDCache from database ", 1};
                    lock_guard         lock{this->fDBConnectionPtr_};
                    sAdvisoryHWAddr2GUIDCache.store (Mapping<String, GUID>{fHWAddr2GUIDCacheTableConnection_->GetAll ().Select<KeyValuePair<String, GUID>> ([] (const auto& i) { return KeyValuePair<String, GUID>{i.HWAddress, i.DeviceID}; })});
                }
                catch (...) {
                    Logger::sThe.Log (Logger::eError, L"Failed to load sAdvisoryHWAddr2GUIDCache from db: %s", Characters::ToString (current_exception ()).c_str ());
                    Execution::ReThrow ();
                }
                try {
                    Debug::TimingTrace ttrc{L"...load of sAdvisoryExternalIPAddr2NetworkGUIDCache from database ", 1};
                    lock_guard         lock{this->fDBConnectionPtr_};
                    sAdvisoryExternalIPAddr2NetworkGUIDCache.store (Mapping<InternetAddress, GUID>{fExternalIPAddr2NetGUIDCacheTableConnection_->GetAll ().Select<KeyValuePair<InternetAddress, GUID>> ([] (const auto& i) { return KeyValuePair<InternetAddress, GUID>{i.ExternalNetworkAddress, i.NetworkID}; })});
                }
                catch (...) {
                    Logger::sThe.Log (Logger::eError, L"Failed to load sAdvisoryExternalIPAddr2NetworkGUIDCache from db: %s", Characters::ToString (current_exception ()).c_str ());
                    Execution::ReThrow ();
                }
                try {
                    Debug::TimingTrace ttrc{L"...load of fCachedDeviceUserSettings_ from database ", 1};
                    lock_guard         lock{this->fDBConnectionPtr_};
                    fCachedDeviceUserSettings_.store (Mapping<GUID, Model::Device::UserOverridesType>{fDeviceUserSettingsTableConnection_.rwget ().cref ()->GetAll ().Select<KeyValuePair<GUID, Model::Device::UserOverridesType>> ([] (const auto& i) { return KeyValuePair<GUID, Model::Device::UserOverridesType>{i.fDeviceID, i.fUserSettings}; })});
                }
                catch (...) {
                    Logger::sThe.Log (Logger::eError, L"Failed to load fCachedDeviceUserSettings_ from db: %s", Characters::ToString (current_exception ()).c_str ());
                    Execution::ReThrow ();
                }
                try {
                    Debug::TimingTrace ttrc{L"...load of fCachedNetworkUserSettings_ from database ", 1};
                    lock_guard         lock{this->fDBConnectionPtr_};
                    fCachedNetworkUserSettings_.store (Mapping<GUID, Model::Network::UserOverridesType>{fNetworkUserSettingsTableConnection_.rwget ().cref ()->GetAll ().Select<KeyValuePair<GUID, Model::Network::UserOverridesType>> ([] (const auto& i) { return KeyValuePair<GUID, Model::Network::UserOverridesType>{i.fNetworkID, i.fUserSettings}; })});
                }
                catch (...) {
                    Logger::sThe.Log (Logger::eError, L"Failed to load fCachedNetworkUserSettings_ from db: %s", Characters::ToString (current_exception ()).c_str ());
                    Execution::ReThrow ();
                }

                Require (fDatabaseSyncThread_ == nullptr);
                fDatabaseSyncThread_ = Thread::New ([this] () { BackgroundDatabaseThread_ (); }, Thread::eAutoStart, L"BackgroundDatabaseThread"sv);
            }
            Mgr_ (const Mgr_&) = delete;
            Mgr_& operator= (const Mgr_&) = delete;
            ~Mgr_ ()
            {
                Debug::TraceContextBumper                        ctx{L"IntegratedModel::{}::Mgr_::DTOR"};
                Execution::Thread::SuppressInterruptionInContext suppressInterruption; // must complete this abort and wait for done - this cannot abort/throw
                fDatabaseSyncThread_.AbortAndWaitForDone ();
            }
            GUID GenNewDeviceID (const String& hwAddress)
            {
                Debug::TimingTrace ttrc{L"GenNewDeviceID", 0.001}; // sb very quick
                auto               l = DBAccess_::sAdvisoryHWAddr2GUIDCache.rwget ();
                if (auto o = l->Lookup (hwAddress)) {
                    return *o;
                }
                GUID newRes = GUID::GenerateNew ();
                l->Add (hwAddress, newRes);
                lock_guard lock{this->fDBConnectionPtr_};
                try {
                    fHWAddr2GUIDCacheTableConnection_->AddNew (Private_::HWAddr2GUIDElt_{hwAddress, newRes});
                }
                catch (...) {
                    Logger::sThe.Log (Logger::eWarning, L"Ignoring error writing hwaddr2deviceid cache table: %s", Characters::ToString (current_exception ()).c_str ());
                }
                return newRes;
            }
            GUID GenNewDeviceID (const Set<String>& hwAddresses)
            {
                // consider if there is a better way to select which hwaddress to use
                return hwAddresses.empty () ? GUID::GenerateNew () : GenNewDeviceID (*hwAddresses.First ());
            }
            GUID GenNewNetworkID (const InternetAddress& externalAddress)
            {
                Debug::TimingTrace ttrc{L"GenNewNetworkID", 0.001}; // sb very quick
                auto               l = DBAccess_::sAdvisoryExternalIPAddr2NetworkGUIDCache.rwget ();
                if (auto o = l->Lookup (externalAddress)) {
                    return *o;
                }
                GUID newRes = GUID::GenerateNew ();
                l->Add (externalAddress, newRes);
                lock_guard lock{this->fDBConnectionPtr_};
                try {
                    fExternalIPAddr2NetGUIDCacheTableConnection_->AddNew (Private_::ExternalIPAddr2NetGUIDElt_{externalAddress, newRes});
                }
                catch (...) {
                    Logger::sThe.Log (Logger::eWarning, L"Ignoring error writing ipaddr2NetworkID cache table: %s", Characters::ToString (current_exception ()).c_str ());
                }
                return newRes;
            }
            GUID GenNewNetworkID (const optional<Set<InternetAddress>>& externalAddresses)
            {
                return (externalAddresses == nullopt or externalAddresses->empty ()) ? GUID::GenerateNew () : GenNewNetworkID (*externalAddresses->First ());
            }
            optional<Model::Device::UserOverridesType> LookupDevicesUserSettings (const GUID& guid) const
            {
                Debug::TimingTrace ttrc{L"IntegratedModel...LookupDevicesUserSettings ()", 0.001};
                return fCachedDeviceUserSettings_.cget ().cref ().Lookup (guid);
            }
            void SetDeviceUserSettings (const GUID& id, const std::optional<IntegratedModel::Device::UserOverridesType>& settings)
            {
                Debug::TimingTrace ttrc{L"IntegratedModel ... SetDeviceUserSettings", 0.1};
                // first check if legit id, and then store
                // @todo check if good id and throw if not...
                auto lk = fCachedDeviceUserSettings_.rwget ();
                if (settings) {
                    fDeviceUserSettingsTableConnection_.rwget ().cref ()->AddOrUpdate (Private_::ExternalDeviceUserSettingsElt_{id, *settings});
                    fCachedDeviceUserSettings_.rwget ().rwref ().Add (id, *settings);
                }
                else {
                    fDeviceUserSettingsTableConnection_.rwget ().cref ()->DeleteByID (id);
                    fCachedDeviceUserSettings_.rwget ().rwref ().RemoveIf (id);
                }
            }
            optional<Model::Network::UserOverridesType> LookupNetworkUserSettings (const GUID& guid) const
            {
                Debug::TimingTrace ttrc{L"IntegratedModel...LookupNetworkUserSettings ()", 0.001};
                return fCachedNetworkUserSettings_.cget ().cref ().Lookup (guid);
            }
            void SetNetworkUserSettings (const GUID& id, const std::optional<IntegratedModel::Network::UserOverridesType>& settings)
            {
                Debug::TimingTrace ttrc{L"IntegratedModel ... SetNetworkUserSettings", 0.1};
                // first check if legit id, and then store
                // @todo check if good id and throw if not...
                auto lk = fCachedNetworkUserSettings_.rwget ();
                if (settings) {
                    fNetworkUserSettingsTableConnection_.rwget ().cref ()->AddOrUpdate (Private_::ExternalNetworkUserSettingsElt_{id, *settings});
                    fCachedNetworkUserSettings_.rwget ().rwref ().Add (id, *settings);
                }
                else {
                    fNetworkUserSettingsTableConnection_.rwget ().cref ()->DeleteByID (id);
                    fCachedNetworkUserSettings_.rwget ().rwref ().RemoveIf (id);
                }
            }

        private:
            void BackgroundDatabaseThread_ ()
            {
                Debug::TraceContextBumper                                       ctx{L"BackgroundDatabaseThread_ loop"};
                unique_ptr<SQL::ORM::TableConnection<IntegratedModel::Device>>  deviceTableConnection;
                unique_ptr<SQL::ORM::TableConnection<IntegratedModel::Network>> networkTableConnection;
                unsigned int                                                    netSnapshotsLoaded{};
                unsigned int                                                    deviceSnapshotsLoaded{};
                while (true) {
                    try {
                        // load networks before devices because devices depend on networks but not the reverse
                        if (networkTableConnection == nullptr) {
                            networkTableConnection = make_unique<SQL::ORM::TableConnection<IntegratedModel::Network>> (fDBConnectionPtr_, Private_::kNetworkTableSchema_, Private_::kDBObjectMapper_, BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<IntegratedModel::Network>> ());
                            try {
                                Debug::TimingTrace ttrc{L"...initial load of sDBNetworks_ from database ", 1};
                                auto               errorHandler = [] ([[maybe_unused]] const SQL::Statement::Row& r, const exception_ptr& e) -> optional<IntegratedModel::Network> {
                                    // Just drop the record on the floor after logging
                                    Logger::sThe.Log (Logger::eError, L"Error reading database of persisted network snapshot ('%s'): %s", Characters::ToString (r).c_str (), Characters::ToString (e).c_str ());
                                    return nullopt;
                                };
                                auto all           = networkTableConnection->GetAll (errorHandler);
                                netSnapshotsLoaded = static_cast<unsigned int> (all.size ());
                                sDBNetworks_.store (NetworkKeyedCollection_{all});
                            }
                            catch (...) {
                                Logger::sThe.Log (Logger::eError, L"Probably important error reading database of old networks data: %s", Characters::ToString (current_exception ()).c_str ());
                                networkTableConnection = nullptr; // so we re-fetch
                                Execution::ReThrow ();
                            }
                        }
                        if (deviceTableConnection == nullptr) {
                            deviceTableConnection = make_unique<SQL::ORM::TableConnection<IntegratedModel::Device>> (fDBConnectionPtr_, Private_::kDeviceTableSchema_, Private_::kDBObjectMapper_, BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<IntegratedModel::Device>> ());
                            try {
                                Debug::TimingTrace ttrc{L"...initial load of sDBDevices_ from database ", 1};
                                auto               errorHandler = [] ([[maybe_unused]] const SQL::Statement::Row& r, const exception_ptr& e) -> optional<IntegratedModel::Device> {
                                    // Just drop the record on the floor after logging
                                    Logger::sThe.Log (Logger::eError, L"Error reading database of persisted device snapshot ('%s'): %s", Characters::ToString (r).c_str (), Characters::ToString (e).c_str ());
                                    return nullopt;
                                };
                                auto all = deviceTableConnection->GetAll (errorHandler);
#if qDebug
                                all.Apply ([] (const Model::Device& d) { Assert (!d.fUserOverrides); }); // tracked on rollup devices, not snapshot devices
#endif
                                deviceSnapshotsLoaded = static_cast<unsigned int> (all.size ());
                                sDBDevices_.store (DeviceKeyedCollection_{all}); // pre-load in memory copy with whatever we had stored in the database
                            }
                            catch (...) {
                                Logger::sThe.Log (Logger::eError, L"Probably important error reading database of old device data: %s", Characters::ToString (current_exception ()).c_str ());
                                deviceTableConnection = nullptr; // so we re-fetch
                                Execution::ReThrow ();
                            }
                        }
                        if (not sFinishedInitialDBLoad_) {
                            Logger::sThe.Log (Logger::eInfo, L"Loaded %d network snapshots and %d device snapshots from database", netSnapshotsLoaded, deviceSnapshotsLoaded);
                            sFinishedInitialDBLoad_ = true;
                        }
                        // periodically write the latest discovered data to the database

                        // UPDATE sDBNetworks_ INCREMENTALLY to reflect reflect these merges
                        for (auto ni : DiscoveryWrapper_::GetNetworks_ ()) {
                            if (not kSupportPersistedNetworkInterfaces_) {
                                ni.fAttachedInterfaces.clear ();
                            }
                            Assert (ni.fSeen); // don't track/write items which have never been seen
                            auto rec2Update = fDB_.AddOrMergeUpdate (networkTableConnection.get (), ni);
                            sDBNetworks_.rwget ()->Add (rec2Update);
                        }

                        // UPDATE sDBDevices_ INCREMENTALLY to reflect reflect these merges
                        for (auto di : DiscoveryWrapper_::GetDevices_ ()) {
                            Assert (di.fSeen.EverSeen ());
                            if (not kSupportPersistedNetworkInterfaces_) {
                                di.fAttachedNetworkInterfaces = nullopt;
                            }
                            Assert (di.fSeen.EverSeen ());         // don't track/write items which have never been seen
                            Assert (di.fUserOverrides == nullopt); // tracked on rollup devices, not snapshot devices
                            auto rec2Update = fDB_.AddOrMergeUpdate (deviceTableConnection.get (), di);
                            sDBDevices_.rwget ()->Add (rec2Update);
                        }

                        // only update periodically
                        Execution::Sleep (30s);
                    }
                    catch (const Thread::AbortException&) {
                        Execution::ReThrow ();
                    }
                    catch (...) {
                        //DbgTrace (L"Ignoring (will retry in 30 seconds) exception in BackgroundDatabaseThread_ loop: %s", Characters::ToString (current_exception ()).c_str ());
                        Logger::sThe.Log (Logger::eWarning, L"Database update: ignoring exception in BackgroundDatabaseThread_ loop (will retry in 30 seconds): %s", Characters::ToString (current_exception ()).c_str ());
                        Execution::Sleep (30s);
                    }
                }
            }
        };
        optional<Mgr_> sMgr_;
    }
}

namespace {
    namespace RollupSummary_ {
        using namespace RollupCommon_;

        struct RolledUpNetworks {
            // @todo add much more here - different useful summaries of same info
            NetworkKeyedCollection_ fNetworks;

            //tmphack slow impl - instead build mapping table when constructing rollup
            // of networkinfo
            // @todo define struct for NetworksRollup (and devices) that has above map, and the REVERSE ID map
            // (individual 2 rollup), and then provide funciton to map a set of IDs to their rolled up IDs
            // AND DOCUMENT WHY GUARNATEED UNIQUE - IF AS I THINK IT IS - CUZ each item goes in one rollup)
            auto MapAggregatedNetID2ItsRollupID (const GUID& netID) -> GUID
            {
                for (const auto& i : fNetworks) {
                    if (i.fAggregatesReversibly and i.fAggregatesReversibly->Contains (netID)) {
                        return i.fGUID;
                    }
                    if (i.fAggregatesIrreversibly and i.fAggregatesIrreversibly->Contains (netID)) {
                        return i.fGUID;
                    }
                }
                DbgTrace (L"Failed to find netID=%s", Characters::ToString (netID).c_str ());
                for (const auto& i : fNetworks) {
                    DbgTrace (L"rolledupNet=%s", Characters::ToString (i).c_str ());
                }
                WeakAssert (false); // @todo fix - because we guarantee each item rolled up exactly once - but happens sometimes on change of network - I think due to outdated device records referring to newer network not yet in this cache...
                return netID;
            };
        };
        RolledUpNetworks GetRolledUpNetworks (Time::DurationSecondsType allowedStaleness = 5.0);

        struct RolledUpDevices {
        public:
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

                auto rolledUpNetworks = GetRolledUpNetworks (allowedStaleness * 10.0); // longer allowedStaleness cuz we dont care much about this and the parts
                                                                                       // we look at really dont change

                auto mapAggregatedAttachments2Rollups = [&] (const Mapping<GUID, NetworkAttachmentInfo>& nats) -> Mapping<GUID, NetworkAttachmentInfo> {
                    Mapping<GUID, NetworkAttachmentInfo> result;
                    for (const auto& ni : nats) {
                        result.Add (rolledUpNetworks.MapAggregatedNetID2ItsRollupID (ni.fKey), ni.fValue);
                    }
                    return result;
                };

                // sRolledUpDevicesFromDB_: keep a cache of the rolled up devices as of our first load from the database, just
                // as a slight performance tweek.
                // NOTE - we would NOT need to use Syncrhonized here if we used sCache_.fHoldWriteLockDuringCacheFill
                static Synchronized<RolledUpDevices> sRolledUpDevicesFromDB_;

                // Start with the existing rolled up devices
                // and then add in (should be done just once) the values from the database,
                // and then keep adding any more recent discovery changes
                RolledUpDevices result               = sRolledUpDevicesFromDB_;
                auto            doMergeOneIntoRollup = [&result, &mapAggregatedAttachments2Rollups] (const Device& d2MergeIn) {
                    // @todo slow/quadradic - may need to tweak
                    // NOTE - can use DBAccess_::sMgr_->GenNewDeviceID () as TRICK - since uses device id to 'index' / find right item more quickly
                    // for large lists...
                    if (auto i = result.fDevices.First ([&d2MergeIn] (const auto& exisingRolledUpDevice) { return ShouldRollup_ (exisingRolledUpDevice, d2MergeIn); })) {
                        Device d2MergeInPatched            = d2MergeIn;
                        d2MergeInPatched.fAttachedNetworks = mapAggregatedAttachments2Rollups (d2MergeInPatched.fAttachedNetworks);
                        Device tmp                         = Device::Rollup (*i, d2MergeInPatched);
                        Assert (tmp.fGUID == i->fGUID); // rollup cannot change device ID
                        // userSettings already added on first rollup
                        result.fDevices.Add (tmp); // update
                    }
                    else {
                        Assert (not d2MergeIn.fAggregatesReversibly.has_value ());
                        Device newRolledUpDevice                = d2MergeIn;
                        newRolledUpDevice.fAggregatesReversibly = Set<GUID>{d2MergeIn.fGUID};
                        newRolledUpDevice.fGUID                 = DBAccess_::sMgr_->GenNewDeviceID (d2MergeIn.GetHardwareAddresses ());
                        if (result.fDevices.Contains (newRolledUpDevice.fGUID)) {
                            // Should probably never happen, but since depends on data in database, program defensively
                            Logger::sThe.Log (Logger::eWarning, L"Got rollup device ID from cache that is already in use: %s (for hardware addresses %s)", Characters::ToString (newRolledUpDevice.fGUID).c_str (), Characters::ToString (d2MergeIn.GetHardwareAddresses ()).c_str ());
                            newRolledUpDevice.fGUID = GUID::GenerateNew ();
                        }
                        newRolledUpDevice.fAttachedNetworks = mapAggregatedAttachments2Rollups (newRolledUpDevice.fAttachedNetworks);
                        newRolledUpDevice.fUserOverrides    = DBAccess_::sMgr_->LookupDevicesUserSettings (newRolledUpDevice.fGUID);
                        if (newRolledUpDevice.fUserOverrides && newRolledUpDevice.fUserOverrides->fName) {
                            newRolledUpDevice.fNames.Add (*newRolledUpDevice.fUserOverrides->fName, 500);
                        }
                        result.fDevices.Add (newRolledUpDevice);
                    }
                };
                static bool sDidMergeFromDatabase_ = false; // no need to roll these up more than once, and be sure to do that one rollup after sFinishedInitialDBLoad_
                if (not sDidMergeFromDatabase_ and DBAccess_::sFinishedInitialDBLoad_) {
                    sDidMergeFromDatabase_ = true;
                    for (const auto& rdi : DBAccess_::sDBDevices_.load ()) {
                        doMergeOneIntoRollup (rdi);
                    }
                    sRolledUpDevicesFromDB_ = result;
                }

                for (const Device& d : DiscoveryWrapper_::GetDevices_ ()) {
                    doMergeOneIntoRollup (d);
                }
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
                        Network newRolledUpNetwork               = net2MergeIn;
                        newRolledUpNetwork.fAggregatesReversibly = Set<GUID>{net2MergeIn.fGUID};
                        newRolledUpNetwork.fGUID                 = DBAccess_::sMgr_->GenNewNetworkID (newRolledUpNetwork.fExternalAddresses);
                        if (result.fNetworks.Contains (newRolledUpNetwork.fGUID)) {
                            // Should probably never happen, but since depends on data in database, program defensively
                            Logger::sThe.Log (Logger::eWarning, L"Got rollup network ID from cache that is already in use: %s (for external address %s)", Characters::ToString (newRolledUpNetwork.fGUID).c_str (), Characters::ToString (newRolledUpNetwork.fExternalAddresses).c_str ());
                            newRolledUpNetwork.fGUID = GUID::GenerateNew ();
                        }
                        newRolledUpNetwork.fUserOverrides = DBAccess_::sMgr_->LookupNetworkUserSettings (newRolledUpNetwork.fGUID);
                        if (newRolledUpNetwork.fUserOverrides && newRolledUpNetwork.fUserOverrides->fName) {
                            //  newRolledUpNetwork.fNames.Add (*newRolledUpNetwork.fUserOverrides->fName, 500);
                        }
                        result.fNetworks.Add (newRolledUpNetwork);
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
    Debug::TraceContextBumper ctx{L"IntegratedModel::Mgr::Activator::Activator"};
    Require (DBAccess_::sMgr_ == nullopt);
    DBAccess_::sMgr_.emplace ();
}

IntegratedModel::Mgr::Activator::~Activator ()
{
    Debug::TraceContextBumper                        ctx{L"IntegratedModel::Mgr::Activator::~Activator"};
    Execution::Thread::SuppressInterruptionInContext suppressInterruption; // must complete this abort and wait for done - this cannot abort/throw
    DBAccess_::sMgr_ = nullopt;
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

optional<IntegratedModel::Device> IntegratedModel::Mgr::GetDevice (const GUID& id) const
{
    // first check rolled up devices, and then raw/unrolled up devices
    // NOTE - this doesn't check the 'dynamic' copy of the devices - it waits til those get migrated to the DB, once ever
    // 30 seconds roughtly...
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

std::optional<IntegratedModel::Device::UserOverridesType> IntegratedModel::Mgr::GetDeviceUserSettings (const GUID& id) const
{
    return DBAccess_::sMgr_->LookupDevicesUserSettings (id);
}

void IntegratedModel::Mgr::SetDeviceUserSettings (const Common::GUID& id, const std::optional<IntegratedModel::Device::UserOverridesType>& settings)
{
    DBAccess_::sMgr_->SetDeviceUserSettings (id, settings);
}

std::optional<GUID> IntegratedModel::Mgr::GetCorrespondingDynamicDeviceID (const GUID& id) const
{
    Set<GUID> dynamicDevices{Discovery::DevicesMgr::sThe.GetActiveDevices ().Select<GUID> ([] (const auto& d) { return d.fGUID; })};
    if (dynamicDevices.Contains (id)) {
        return id;
    }
    auto thisRolledUpDevice = RollupSummary_::GetRolledUpDevies ().fDevices.Lookup (id);
    if (thisRolledUpDevice and thisRolledUpDevice->fAggregatesReversibly) {
        // then find the dynamic device corresponding to this rollup, which will be (as of 2022-06-22) in the aggregates reversibly list
        if (auto ff = thisRolledUpDevice->fAggregatesReversibly->First ([&] (const GUID& d) -> bool { return dynamicDevices.Contains (d); })) {
            Assert (dynamicDevices.Contains (*ff));
            return *ff;
        }
        DbgTrace (L"Info: GetCorrespondingDynamicDeviceID found rollup device with no corresponding dynamic device (can happen if its a hisorical device not on network right now)");
    }
    return nullopt;
}

Sequence<IntegratedModel::Network> IntegratedModel::Mgr::GetNetworks () const
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"IntegratedModel::Mgr::GetNetworks")};
    Debug::TimingTrace        ttrc{L"IntegratedModel::Mgr::GetNetworks", 0.1};
    return Sequence<IntegratedModel::Network>{RollupSummary_::GetRolledUpNetworks ().fNetworks};
}

optional<IntegratedModel::Network> IntegratedModel::Mgr::GetNetwork (const GUID& id) const
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

std::optional<IntegratedModel::Network::UserOverridesType> IntegratedModel::Mgr::GetNetworkUserSettings (const GUID& id) const
{
    return DBAccess_::sMgr_->LookupNetworkUserSettings (id);
}

void IntegratedModel::Mgr::SetNetworkUserSettings (const Common::GUID& id, const std::optional<IntegratedModel::Network::UserOverridesType>& settings)
{
    DBAccess_::sMgr_->SetNetworkUserSettings (id, settings);
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

optional<IntegratedModel::NetworkInterface> IntegratedModel::Mgr::GetNetworkInterface (const GUID& id) const
{
    return GetNetworkInterfaces ().First ([&] (const auto& i) { return i.fGUID == id; });
}