/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Cache/SynchronizedCallerStalenessCache.h"
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
            Logger::sThe.Log (Logger::eWarning, L"Database update: ignoring exception in TransformURL2LocalStorage_: %s",
                              Characters::ToString (e).c_str ());
            Assert (e.code () == errc::device_or_resource_busy); // this can happen talking to database (SQLITE_BUSY or SQLITE_LOCKED)
                                                                 // might be better to up timeout so more rare
        }
        catch (const Thread::AbortException&) {
            Execution::ReThrow ();
        }
        catch (...) {
            Logger::sThe.Log (Logger::eWarning, L"Database update: ignoring exception in TransformURL2LocalStorage_: %s",
                              Characters::ToString (current_exception ()).c_str ());
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
    struct Device_Key_Extractor_ {
        GUID operator() (const IntegratedModel::Device& t) const { return t.fID; };
    };
    using DeviceKeyedCollection_ =
        KeyedCollection<IntegratedModel::Device, GUID, KeyedCollection_DefaultTraits<IntegratedModel::Device, GUID, Device_Key_Extractor_>>;

    struct Network_Key_Extractor_ {
        GUID operator() (const IntegratedModel::Network& t) const { return t.fID; };
    };
    using NetworkKeyedCollection_ =
        KeyedCollection<IntegratedModel::Network, GUID, KeyedCollection_DefaultTraits<IntegratedModel::Network, GUID, Network_Key_Extractor_>>;

    struct NetworkInterface_Key_Extractor_ {
        GUID operator() (const IntegratedModel::NetworkInterface& t) const { return t.fID; };
    };
    using NetworkInterfaceCollection_ =
        Containers::KeyedCollection<IntegratedModel::NetworkInterface, GUID, Containers::KeyedCollection_DefaultTraits<IntegratedModel::NetworkInterface, GUID, NetworkInterface_Key_Extractor_>>;
}

namespace {
    /**
     *  Wrappers on the device manager APIs, that just fetch the discovered devices and convert to common
     *  integrated model (no datebase awareness)
     * 
     *  Note - IDs are simple to manage/maintain - they come from the discovery layer, and we SIMPLY RE-USE those IDs in this DiscoveryWrapper
     *  layer. That makes all ID/Pointers consistent. They just get handled differently by the ROLLUP layer.
     */
    namespace DiscoveryWrapper_ {
        using IntegratedModel::Device;
        using IntegratedModel::Network;
        using IntegratedModel::NetworkAttachmentInfo;
        using IntegratedModel::NetworkInterface;

        Device Discovery2Model_ (const Discovery::Device& d)
        {
            Device newDev;
            newDev.fID    = d.fGUID;
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
            newDev.fPresentationURL = d.fPresentationURL;
            newDev.fManufacturer    = d.fManufacturer;
            newDev.fIcon            = TransformURL2LocalStorage_ (d.fIcon);
            newDev.fOperatingSystem = d.fOperatingSystem;
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
        }
        Network Discovery2Model_ (const Discovery::Network& n)
        {
            Network nw{n.fNetworkAddresses};
            nw.fID                       = n.fGUID;
            nw.fNames                    = n.fNames;
            nw.fNetworkAddresses         = n.fNetworkAddresses;
            nw.fAttachedInterfaces       = n.fAttachedNetworkInterfaces;
            nw.fDNSServers               = n.fDNSServers;
            nw.fGateways                 = n.fGateways;
            nw.fGatewayHardwareAddresses = n.fGatewayHardwareAddresses;
            nw.fExternalAddresses        = n.fExternalAddresses;
            nw.fGEOLocInformation        = n.fGEOLocInfo;
            nw.fInternetServiceProvider  = n.fISP;
            nw.fSeen                     = n.fSeen;
#if qDebug
            if (not n.fDebugProps.empty ()) {
                nw.fDebugProps = n.fDebugProps;
            }
#endif
            return nw;
        }
        NetworkInterface Discovery2Model_ (const Discovery::NetworkInterface& n)
        {
            NetworkInterface nwi;
            nwi.fInternalInterfaceID = n.fInternalInterfaceID;
            nwi.fFriendlyName        = n.fFriendlyName;
            nwi.fDescription         = n.fDescription;
            //nwi.fNetworkGUID         = n.fNetworkGUID;    INTENTIONALLY OMITTED because doesn't correspond to our network ID, misleading, and unhelpful
            nwi.fType                 = n.fType;
            nwi.fHardwareAddress      = n.fHardwareAddress;
            nwi.fTransmitSpeedBaud    = n.fTransmitSpeedBaud;
            nwi.fReceiveLinkSpeedBaud = n.fReceiveLinkSpeedBaud;
            nwi.fWirelessInfo         = n.fWirelessInfo;
            nwi.fBindings             = n.fBindings;
            nwi.fGateways             = n.fGateways;
            nwi.fDNSServers           = n.fDNSServers;
            nwi.fStatus               = n.fStatus;
            nwi.fID                   = n.fGUID;
#if qDebug
            if (not n.fDebugProps.empty ()) {
                nwi.fDebugProps = n.fDebugProps;
            }
#endif
            return nwi;
        }

        optional<GUID> GetMyDeviceID_ () { return Discovery::DevicesMgr::sThe.GetThisDeviceID (); }

        /**
         * Map all the 'Discovery::Device' objects to 'Model::Device' objects.
         *
         *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
         */
        Sequence<Device> GetDevices_ ()
        {
            Debug::TimingTrace ttrc{L"DiscoveryWrapper_::GetDevices_", .1};
            // Fetch (UNSORTED) list of devices
            return Discovery::DevicesMgr::sThe.GetActiveDevices ().Map<Device, Sequence<Device>> (
                [] (const Discovery::Device& d) { return Discovery2Model_ (d); });
        }
        /**
         * Map all the 'Discovery::Network' objects to 'Model::Network' objects.
         *
         *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
         */
        Sequence<Network> GetNetworks_ ()
        {
            Debug::TimingTrace ttrc{L"DiscoveryWrapper_::GetNetworks_", 0.1};
            Sequence<Network>  result = Discovery::NetworksMgr::sThe.CollectActiveNetworks ().Map<Network, Sequence<Network>> (
                [] (const Discovery::Network& n) { return Discovery2Model_ (n); });
#if USE_NOISY_TRACE_IN_THIS_MODULE_
            DbgTrace (L"returns: %s", Characters::ToString (result).c_str ());
#endif
            return result;
        }

        /**
         * Map all the 'Discovery::NetworkInterface' objects to 'Model::NetworkInterface' objects.
         *
         *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
         */
        Sequence<NetworkInterface> GetNetworkInterfaces_ ()
        {
            Debug::TimingTrace         ttrc{L"DiscoveryWrapper_::GetNetworkInterfaces_", 0.1};
            Sequence<NetworkInterface> result =
                Discovery::NetworkInterfacesMgr::sThe.CollectAllNetworkInterfaces ().Map<NetworkInterface, Sequence<NetworkInterface>> (
                    [] (const Discovery::NetworkInterface& n) { return Discovery2Model_ (n); });

#if USE_NOISY_TRACE_IN_THIS_MODULE_
            DbgTrace (L"returns: %s", Characters::ToString (result).c_str ());
#endif
            return result;
        }
    }
}

namespace {
    /**
     *  Wrapper on Database access all goes in this DBAccess::Mgr_ module
     *
     *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
     */
    class DBAccessMgr_ {
    private:
        using Schema_Table         = SQL::ORM::Schema::Table;
        using Schema_Field         = SQL::ORM::Schema::Field;
        using Schema_CatchAllField = SQL::ORM::Schema::CatchAllField;

    private:
        static constexpr auto kRepresentIDAs_ = BackendApp::Common::DB::kRepresentIDAs_;

    public:
        DBAccessMgr_ ()
        {
            Debug::TraceContextBumper ctx{L"IntegratedModel::{}::Mgr_::CTOR"};
            fDeviceUserSettingsTableConnection_ = make_unique<SQL::ORM::TableConnection<ExternalDeviceUserSettingsElt_>> (
                fDBConnectionPtr_, kDeviceUserSettingsSchema_, kDBObjectMapper_,
                BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<ExternalDeviceUserSettingsElt_>> ());
            fNetworkUserSettingsTableConnection_ = make_unique<SQL::ORM::TableConnection<ExternalNetworkUserSettingsElt_>> (
                fDBConnectionPtr_, kNetworkUserSettingsSchema_, kDBObjectMapper_,
                BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<ExternalNetworkUserSettingsElt_>> ());
            fDeviceTableConnection_ = make_unique<SQL::ORM::TableConnection<IntegratedModel::Device>> (
                fDBConnectionPtr_, kDeviceTableSchema_, kDBObjectMapper_,
                BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<IntegratedModel::Device>> ());
            fNetworkTableConnection_ = make_unique<SQL::ORM::TableConnection<IntegratedModel::Network>> (
                fDBConnectionPtr_, kNetworkTableSchema_, kDBObjectMapper_,
                BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<IntegratedModel::Network>> ());
            fNetworkInterfaceTableConnection_ = make_unique<SQL::ORM::TableConnection<IntegratedModel::NetworkInterface>> (
                fDBConnectionPtr_, kNetworkInterfaceTableSchema_, kDBObjectMapper_,
                BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<IntegratedModel::NetworkInterface>> ());
            try {
                Debug::TimingTrace ttrc{L"...load of fCachedDeviceUserSettings_ from database ", 1};
                lock_guard         lock{this->fDBConnectionPtr_};
                fCachedDeviceUserSettings_.store (Mapping<GUID, Model::Device::UserOverridesType>{
                    fDeviceUserSettingsTableConnection_.rwget ().cref ()->GetAll ().Map<KeyValuePair<GUID, Model::Device::UserOverridesType>> (
                        [] (const auto& i) {
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
                    fNetworkUserSettingsTableConnection_.rwget ().cref ()->GetAll ().Map<KeyValuePair<GUID, Model::Network::UserOverridesType>> (
                        [] (const auto& i) {
                            return KeyValuePair<GUID, Model::Network::UserOverridesType>{i.fNetworkID, i.fUserSettings};
                        })});
            }
            catch (...) {
                Logger::sThe.Log (Logger::eCriticalError, L"Failed to load fCachedNetworkUserSettings_ from db: %s",
                                  Characters::ToString (current_exception ()).c_str ());
                Execution::ReThrow ();
            }

            Require (fDatabaseSyncThread_ == nullptr);
            fDatabaseSyncThread_ = Thread::New ([this] () { BackgroundDatabaseThread_ (); }, Thread::eAutoStart, L"BackgroundDatabaseThread"sv);
        }
        DBAccessMgr_ (const DBAccessMgr_&)            = delete;
        DBAccessMgr_& operator= (const DBAccessMgr_&) = delete;
        ~DBAccessMgr_ ()
        {
            Debug::TraceContextBumper ctx{L"IntegratedModel::{}::DBAccessMgr_::DTOR"};
            Execution::Thread::SuppressInterruptionInContext suppressInterruption; // must complete this abort and wait for done - this cannot abort/throw
            fDatabaseSyncThread_.AbortAndWaitForDone ();
        }

        nonvirtual GUID GenNewDeviceID (const Set<String>& hwAddresses)
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
        nonvirtual GUID GenNewNetworkID ([[maybe_unused]] const Model::Network& rollupNetwork, const Model::Network& containedNetwork)
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
        nonvirtual Mapping<GUID, Model::Device::UserOverridesType> GetDeviceUserSettings () const
        {
            Debug::TimingTrace ttrc{L"IntegratedModel...LookupDevicesUserSettings ()", 0.001};
            return fCachedDeviceUserSettings_.cget ().cref ();
        }
        nonvirtual optional<Model::Device::UserOverridesType> LookupDevicesUserSettings (const GUID& guid) const
        {
            Debug::TimingTrace ttrc{L"IntegratedModel...LookupDevicesUserSettings ()", 0.001};
            return fCachedDeviceUserSettings_.cget ().cref ().Lookup (guid);
        }
        nonvirtual bool SetDeviceUserSettings (const GUID& id, const std::optional<IntegratedModel::Device::UserOverridesType>& settings)
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
                fDeviceUserSettingsTableConnection_.rwget ().cref ()->Delete (id);
                return fCachedDeviceUserSettings_.rwget ().rwref ().RemoveIf (id);
            }
        }
        nonvirtual Mapping<GUID, Model::Network::UserOverridesType> GetNetworkUserSettings () const
        {
            Debug::TimingTrace ttrc{L"IntegratedModel...GetNetworkUserSettings ()", 0.001};
            return fCachedNetworkUserSettings_.cget ().cref ();
        }
        nonvirtual optional<Model::Network::UserOverridesType> LookupNetworkUserSettings (const GUID& guid) const
        {
            Debug::TimingTrace ttrc{L"IntegratedModel...LookupNetworkUserSettings ()", 0.001};
            return fCachedNetworkUserSettings_.cget ().cref ().Lookup (guid);
        }
        // return true if changed
        nonvirtual bool SetNetworkUserSettings (const GUID& id, const std::optional<IntegratedModel::Network::UserOverridesType>& settings)
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
                fNetworkUserSettingsTableConnection_.rwget ().cref ()->Delete (id);
                return fCachedNetworkUserSettings_.rwget ().rwref ().RemoveIf (id);
            }
        }
        nonvirtual NetworkInterfaceCollection_ GetRawNetworkInterfaces () const { return fDBNetworkInterfaces_; }
        nonvirtual NetworkKeyedCollection_     GetRawNetworks () const { return fDBNetworks_; }
        nonvirtual DeviceKeyedCollection_      GetRawDevices () const { return fDBDevices_; }
        nonvirtual bool                        GetFinishedInitialDBLoad () const { return fFinishedInitialDBLoad_; }

    private:
        static String GenRandomIDString_ (VariantValue::Type t)
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
        static inline const ConstantProperty<ObjectVariantMapper> kDBObjectMapper_{[] () {
            ObjectVariantMapper mapper;

            mapper += IntegratedModel::NetworkInterface::kMapper;
            mapper += IntegratedModel::Network::kMapper;
            mapper += IntegratedModel::Device::kMapper;

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
        static inline const Schema_Table                          kDeviceUserSettingsSchema_{
            L"DeviceUserSettings"sv,
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
        static inline const Schema_Table kNetworkUserSettingsSchema_{
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
        static inline const Schema_Table kDeviceTableSchema_{L"Devices"sv,
                                                             /*
             *  use the same names as the ObjectVariantMapper for simpler mapping, or specify an alternate name
             *  for ID, just as an example.
             */
                                                             Collection<Schema_Field>{
#if __cpp_designated_initializers
                                                                 /**
                 *  For ID, generate random GUID (BLOB) automatically in database
                 */
                                                                 {.fName              = L"ID"sv,
                                                                  .fVariantValueName  = L"id"sv,
                                                                  .fRequired          = true,
                                                                  .fVariantValueType  = kRepresentIDAs_,
                                                                  .fIsKeyField        = true,
                                                                  .fDefaultExpression = GenRandomIDString_ (kRepresentIDAs_)},
                                                                 {.fName = L"name"sv, .fVariantValueType = VariantValue::eString},
#else
                                                                  {L"ID", L"id"sv, true, kRepresentIDAs_, nullopt, true, nullopt,
                                                                   GenRandomIDString_ (kRepresentIDAs_)},
                                                                  {L"name", nullopt, false, VariantValue::eString},
#endif
                                                             },
                                                             Schema_CatchAllField{}};

        static inline const Schema_Table kNetworkInterfaceTableSchema_{
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
        static inline const Schema_Table kNetworkTableSchema_{L"Networks"sv,
                                                              /*
             *  use the same names as the ObjectVariantMapper for simpler mapping, or specify an alternate name
             *  for ID, just as an example.
             */
                                                              Collection<Schema_Field>{
#if __cpp_designated_initializers
                                                                  /**
                 *  For ID, generate random GUID (BLOB) automatically in database
                 */
                                                                  {.fName              = L"ID"sv,
                                                                   .fVariantValueName  = L"id"sv,
                                                                   .fRequired          = true,
                                                                   .fVariantValueType  = kRepresentIDAs_,
                                                                   .fIsKeyField        = true,
                                                                   .fDefaultExpression = GenRandomIDString_ (kRepresentIDAs_)},
                                                                  {.fName = L"friendlyName"sv, .fVariantValueType = VariantValue::eString},
#else
                                                                   {L"ID", L"id"sv, true, kRepresentIDAs_, nullopt, true, nullopt,
                                                                    GenRandomIDString_ (kRepresentIDAs_)},
                                                                   {L"friendlyName", nullopt, false, VariantValue::eString},
#endif
                                                              },
                                                              Schema_CatchAllField{}};

    private:
        static constexpr Configuration::Version kCurrentVersion_ = Configuration::Version{1, 0, Configuration::VersionStage::Alpha, 0};
        BackendApp::Common::DB                  fDB_{kCurrentVersion_,
                                    Traversal::Iterable<Schema_Table>{kDeviceTableSchema_, kDeviceUserSettingsSchema_, kNetworkTableSchema_,
                                                                                       kNetworkInterfaceTableSchema_, kNetworkUserSettingsSchema_}};
        Synchronized<SQL::Connection::Ptr>      fDBConnectionPtr_{fDB_.NewConnection ()};
        Execution::Thread::Ptr                  fDatabaseSyncThread_{};
        Synchronized<Mapping<GUID, IntegratedModel::Device::UserOverridesType>>              fCachedDeviceUserSettings_;
        Synchronized<unique_ptr<SQL::ORM::TableConnection<ExternalDeviceUserSettingsElt_>>>  fDeviceUserSettingsTableConnection_;
        Synchronized<Mapping<GUID, IntegratedModel::Network::UserOverridesType>>             fCachedNetworkUserSettings_;
        Synchronized<unique_ptr<SQL::ORM::TableConnection<ExternalNetworkUserSettingsElt_>>> fNetworkUserSettingsTableConnection_;
        unique_ptr<SQL::ORM::TableConnection<IntegratedModel::Device>> fDeviceTableConnection_; // only accessed from a background database thread
        unique_ptr<SQL::ORM::TableConnection<IntegratedModel::Network>>          fNetworkTableConnection_;          // ''
        unique_ptr<SQL::ORM::TableConnection<IntegratedModel::NetworkInterface>> fNetworkInterfaceTableConnection_; // ''
        Synchronized<DeviceKeyedCollection_>                                     fDBDevices_;           // mirror database contents in RAM
        Synchronized<NetworkKeyedCollection_>                                    fDBNetworks_;          // ''
        Synchronized<NetworkInterfaceCollection_>                                fDBNetworkInterfaces_; // ''
        atomic<bool>                                                             fFinishedInitialDBLoad_{false};

        // the latest copy of what is in the DB (manually kept up to date)
        // NOTE: These are all non-rolled up objects
        Synchronized<Mapping<String, GUID>> fAdvisoryHWAddr2GUIDCache_;

    private:
        void BackgroundDatabaseThread_ ()
        {
            Debug::TraceContextBumper ctx{L"BackgroundDatabaseThread_ loop"};
            optional<unsigned int>    netInterfaceSnapshotsLoaded{};
            optional<unsigned int>    netSnapshotsLoaded{};
            optional<unsigned int>    deviceSnapshotsLoaded{};
            while (true) {
                try {
                    // @todo Consider if we should do this logic in CTOR so we can lose the flag saying if we've started up...
                    // advantage of keeping this way is we get to starting to accept commands sooner. BUt so many of those
                    // commands we just need to reject cuz we aren't ready. UNSURE.

                    // load networks before devices because devices depend on networks but not the reverse
                    if (not netInterfaceSnapshotsLoaded.has_value ()) {
                        try {
                            Debug::TimingTrace ttrc{L"...initial load of fDBNetworkInterfaces_ from database ", 1};
                            auto               errorHandler = [] ([[maybe_unused]] const SQL::Statement::Row& r,
                                                    const exception_ptr& e) -> optional<IntegratedModel::NetworkInterface> {
                                // Just drop the record on the floor after logging
                                Logger::sThe.Log (Logger::eError, L"Error reading database of persisted network interfaces snapshot ('%s'): %s",
                                                                Characters::ToString (r).c_str (), Characters::ToString (e).c_str ());
                                return nullopt;
                            };
                            auto all                    = fNetworkInterfaceTableConnection_->GetAll (errorHandler);
                            netInterfaceSnapshotsLoaded = static_cast<unsigned int> (all.size ());
                            fDBNetworkInterfaces_.store (NetworkInterfaceCollection_{all});
                        }
                        catch (...) {
                            Logger::sThe.Log (Logger::eError, L"Probably important error reading database of old network interfaces data: %s",
                                              Characters::ToString (current_exception ()).c_str ());
                            Execution::ReThrow ();
                        }
                    }
                    if (not netSnapshotsLoaded.has_value ()) {
                        try {
                            Debug::TimingTrace ttrc{L"...initial load of fDBNetworks_ from database ", 1};
                            auto               errorHandler = [] ([[maybe_unused]] const SQL::Statement::Row& r,
                                                    const exception_ptr&                        e) -> optional<IntegratedModel::Network> {
                                // Just drop the record on the floor after logging
                                Logger::sThe.Log (Logger::eError, L"Error reading database of persisted network snapshot ('%s'): %s",
                                                                Characters::ToString (r).c_str (), Characters::ToString (e).c_str ());
                                return nullopt;
                            };
                            auto all           = fNetworkTableConnection_->GetAll (errorHandler);
                            netSnapshotsLoaded = static_cast<unsigned int> (all.size ());
                            fDBNetworks_.store (NetworkKeyedCollection_{all});
                        }
                        catch (...) {
                            Logger::sThe.Log (Logger::eError, L"Probably important error reading database of old networks data: %s",
                                              Characters::ToString (current_exception ()).c_str ());
                            Execution::ReThrow ();
                        }
                    }
                    if (not deviceSnapshotsLoaded.has_value ()) {
                        try {
                            Debug::TimingTrace ttrc{L"...initial load of fDBDevices_ from database ", 1};
                            auto               errorHandler = [] ([[maybe_unused]] const SQL::Statement::Row& r,
                                                    const exception_ptr&                        e) -> optional<IntegratedModel::Device> {
                                // Just drop the record on the floor after logging
                                Logger::sThe.Log (Logger::eError, L"Error reading database of persisted device snapshot ('%s'): %s",
                                                                Characters::ToString (r).c_str (), Characters::ToString (e).c_str ());
                                return nullopt;
                            };
                            auto all = fDeviceTableConnection_->GetAll (errorHandler);
                            if constexpr (qDebug) {
                                all.Apply ([] ([[maybe_unused]] const Model::Device& d) { Assert (!d.fUserOverrides); }); // tracked on rollup devices, not snapshot devices
                            }
                            deviceSnapshotsLoaded = static_cast<unsigned int> (all.size ());
                            fDBDevices_.store (DeviceKeyedCollection_{all}); // pre-load in memory copy with whatever we had stored in the database
                        }
                        catch (...) {
                            Logger::sThe.Log (Logger::eError, L"Probably important error reading database of old device data: %s",
                                              Characters::ToString (current_exception ()).c_str ());
                            Execution::ReThrow ();
                        }
                    }
                    if (not fFinishedInitialDBLoad_) {
                        Assert (deviceSnapshotsLoaded);
                        Assert (netSnapshotsLoaded);
                        Assert (netInterfaceSnapshotsLoaded);
                        Logger::sThe.Log (Logger::eInfo, L"Loaded %d network interface snapshots, %d network snapshots and %d device snapshots from database",
                                          *netInterfaceSnapshotsLoaded, *netSnapshotsLoaded, *deviceSnapshotsLoaded);
                        fFinishedInitialDBLoad_ = true;
                    }
                    // periodically write the latest discovered data to the database

                    // UPDATE fDBNetworkInterfaces_ INCREMENTALLY to reflect reflect these merges
                    DiscoveryWrapper_::GetNetworkInterfaces_ ().Apply ([this] (const Model::NetworkInterface& ni) {
                        lock_guard lock{this->fDBConnectionPtr_}; //tmphack fix underlying SQL orm wrapper stuff so not needed --LGP 2022-11-23
                        Assert (ni.fAggregatesReversibly == nullopt); // dont write these summary values
                        fNetworkInterfaceTableConnection_->AddOrUpdate (ni);
                        fDBNetworkInterfaces_.rwget ()->Add (ni);
                    });

                    // UPDATE fDBNetworks_ INCREMENTALLY to reflect reflect these merges
                    DiscoveryWrapper_::GetNetworks_ ().Apply ([this] (const Model::Network& n) {
                        Assert (n.fSeen);                         // don't track/write items which have never been seen
                        lock_guard lock{this->fDBConnectionPtr_}; //tmphack fix underlying SQL orm wrapper stuff so not needed --LGP 2022-11-23
                        Assert (n.fAggregatesReversibly == nullopt); // dont write these summary values
                        fNetworkTableConnection_->AddOrUpdate (n);
                        fDBNetworks_.rwget ()->Add (n);
                    });

                    // UPDATE fDBDevices_ INCREMENTALLY to reflect reflect these merges
                    DiscoveryWrapper_::GetDevices_ ().Apply ([this] (const Model::Device& d) {
                        Assert (d.fSeen.EverSeen ());
                        Assert (d.fSeen.EverSeen ());             // don't track/write items which have never been seen
                        Assert (d.fUserOverrides == nullopt);     // tracked on rollup devices, not snapshot devices
                        lock_guard lock{this->fDBConnectionPtr_}; //tmphack fix underlying SQL orm wrapper stuff so not needed --LGP 2022-11-23
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
    };
    optional<DBAccessMgr_> sDBAccessMgr_; // constructed on module activation

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
     */
    namespace RollupSummary_ {

        using IntegratedModel::Device;
        using IntegratedModel::Network;
        using IntegratedModel::NetworkAttachmentInfo;
        using IntegratedModel::NetworkInterface;

        /**
         *  Data structure representing a copy of currently rolled up network interfaces data (copyable).
         * 
         *  Unlike most othter rollup structures, we have no USER_SETTINGS here, so we ALWAYS rollup the same
         *  way. That means there is no ResetUserSettings, and no need to ever INVALIDATE the network interface settings.
         *  (note this refers something computed from historical database data, not soemthign where dynaic data still rolling in).
         * 
         *  \note   \em Thread-Safety   <a href="Thread-Safety.md#C++-Standard-Thread-Safety">C++-Standard-Thread-Safety</a>
         */
        struct RolledUpNetworkInterfaces {
        private:
            RolledUpNetworkInterfaces (const Iterable<Device>& devices, const Iterable<NetworkInterface>& nets2MergeIn)
            {
                Set<GUID>                   netIDs2Add = nets2MergeIn.Map<GUID, Set<GUID>> ([] (const auto& i) { return i.fID; });
                Set<GUID>                   netsAdded;
                NetworkInterfaceCollection_ nets2MergeInCollected{nets2MergeIn};
                for (const Device& d : devices) {
                    if (d.fAttachedNetworkInterfaces) {
                        d.fAttachedNetworkInterfaces->Apply ([&] (const GUID& netInterfaceID) {
                            netsAdded.Add (netInterfaceID);
                            MergeIn_ (d.fID, Memory::ValueOf (nets2MergeInCollected.Lookup (netInterfaceID)));
                        });
                    }
                }
                // https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/80 - could avoid this maybe??? and the bookkeeping above to compute this list...
                DbgTrace (L"orphaned interface Cnt %d", (netIDs2Add - netsAdded).size ()); // We (temporarily) store network interfaces not associated with any device - if they are not interesting.
                                                                                           // OR, could come from just bad data in database
                    // Either way, just track them, and don't worry for now --LGP 2022-12-03
                for (const auto& netInterfaceWithoutDevice : (netIDs2Add - netsAdded)) {
                    MergeIn_ (nullopt, Memory::ValueOf (nets2MergeInCollected.Lookup (netInterfaceWithoutDevice)));
                }
            }

        public:
            RolledUpNetworkInterfaces (const RolledUpNetworkInterfaces&)            = default;
            RolledUpNetworkInterfaces (RolledUpNetworkInterfaces&&)                 = default;
            RolledUpNetworkInterfaces& operator= (RolledUpNetworkInterfaces&&)      = default;
            RolledUpNetworkInterfaces& operator= (const RolledUpNetworkInterfaces&) = default;

        public:
            /**
             *  This returns the current rolled up network interface objects.
             */
            nonvirtual NetworkInterfaceCollection_ GetNetworkInterfacess () const { return fRolledUpNetworkInterfaces_; }

        private:
            /**
             */
            nonvirtual void MergeIn_ (const optional<GUID>& forDeviceID, const Iterable<NetworkInterface>& netInterfaces2MergeIn)
            {
                netInterfaces2MergeIn.Apply ([this, forDeviceID] (const NetworkInterface& n) { MergeIn_ (forDeviceID, n); });
            }

        public:
            /*
             *  Given a rollup network interface id, apply F to all the matching concrete interfaces.
             *      \req rollupID is contained in this rollup object as a valid rollup id
             */
            nonvirtual Set<GUID> GetConcreteIDsForRollup (const GUID& rollupID) const
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

        public:
            /**
             *  Given an aggregated network id, map to the correspoding rollup ID (todo do we need to handle missing case)
             * 
             *  \req is already valid rollup net ID.
             */
            nonvirtual auto MapAggregatedID2ItsRollupID (const GUID& aggregatedNetInterfaceID) const -> GUID
            {
                if (auto r = fMapAggregatedNetInterfaceID2RollupID_.Lookup (aggregatedNetInterfaceID)) {
                    return *r;
                }
                // shouldn't get past here - debug if/why this hapepns - see comments below
                Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (
                    L"MapAggregatedID2ItsRollupID failed to find aggregatedNetInterfaceID=%s",
                    Characters::ToString (aggregatedNetInterfaceID).c_str ())};
                if constexpr (qDebug) {
                    for ([[maybe_unused]] const auto& i : fRolledUpNetworkInterfaces_) {
                        DbgTrace (L"rolledupNetInterface=%s", Characters::ToString (i).c_str ());
                    }
                }
                Assert (false); // @todo fix - because we guarantee each item rolled up exactly once - but happens sometimes on change of network - I think due to outdated device records referring to newer network not yet in this cache...
                WeakAssert (false); // @todo fix - because we guarantee each item rolled up exactly once - but happens sometimes on change of network - I think due to outdated device records referring to newer network not yet in this cache...
                return aggregatedNetInterfaceID;
            }

        public:
            /**
             *  Argument networkInterfaceID must be aggregated network interfaceid, and returns aggrateged deviceids.
             */
            optional<Set<GUID>> GetAttachedToDeviceIDs (const GUID& aggregatedNetworkInterfaceID) const
            {
                Set<GUID> r{fAssociateAggregatedNetInterface2OwningDeviceID.Lookup (aggregatedNetworkInterfaceID)};
                if (r.empty ()) {
                    return nullopt;
                }
                return r;
            }

        public:
            /**
             *  \brief return the actual (concrete not rollup) NetworkInterface objects associated with the argument ids
             * 
             *      \req each concreteIDs is a valid concrete id contains in this rollup.
             */
            nonvirtual NetworkInterfaceCollection_ GetConcreteNeworkInterfaces (const Set<GUID>& concreteIDs) const
            {
                Require (Set<GUID>{fRawNetworkInterfaces_.Keys ()}.ContainsAll (concreteIDs));
                return fRawNetworkInterfaces_.Where ([&concreteIDs] (const auto& i) { return concreteIDs.Contains (i.fID); });
            }

        public:
            /**
             */
            nonvirtual NetworkInterface GetRollupNetworkInterface (const GUID& id) const
            {
                return Memory::ValueOf (fRolledUpNetworkInterfaces_.Lookup (id));
            }

        public:
            /**
             */
            nonvirtual NetworkInterfaceCollection_ GetRollupNetworkInterfaces (const Set<GUID>& rollupIDs) const
            {
                Require (Set<GUID>{fRolledUpNetworkInterfaces_.Keys ()}.ContainsAll (rollupIDs));
                return fRolledUpNetworkInterfaces_.Where ([&rollupIDs] (const auto& i) { return rollupIDs.Contains (i.fID); });
            }

        public:
            /**
             */
            nonvirtual NetworkInterfaceCollection_ GetRawNetworkInterfaces () const { return fRawNetworkInterfaces_; }
            nonvirtual NetworkInterfaceCollection_ GetRawNetworkInterfaces (const Set<GUID>& rawIDs) const
            {
                Require (Set<GUID>{fRawNetworkInterfaces_.Keys ()}.ContainsAll (rawIDs));
                return fRawNetworkInterfaces_.Where ([&rawIDs] (const auto& i) { return rawIDs.Contains (i.fID); });
            }

        public:
            /**
             *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
             */
            static RolledUpNetworkInterfaces GetCached (Time::DurationSecondsType allowedStaleness = 5.0)
            {
                Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...RolledUpNetworkInterfaces::GetCached")};
                Debug::TimingTrace        ttrc{L"RolledUpNetworkInterfaces::GetCached", 1};
                // SynchronizedCallerStalenessCache object just assures one rollup RUNS internally at a time, and
                // that two calls in rapid succession, the second call re-uses the previous value
                static Cache::SynchronizedCallerStalenessCache<void, RolledUpNetworkInterfaces> sCache_;
                // Disable fHoldWriteLockDuringCacheFill due to https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/23
                // See also
                //      https://stroika.atlassian.net/browse/STK-906 - possible enhancement to this configuration to work better avoiding
                //      See https://stroika.atlassian.net/browse/STK-907 - about needing some new mechanism in Stroika for deadlock detection/avoidance.
                // sCache_.fHoldWriteLockDuringCacheFill = true; // so only one call to filler lambda at a time
                return sCache_.LookupValue (sCache_.Ago (allowedStaleness), [] () -> RolledUpNetworkInterfaces {
                    /*
                     *  DEADLOCK NOTE
                     *      Since this can be called while rolling up DEVICES, its important that this code not call anything involving device rollup since
                     *      that could trigger a deadlock.
                     */
                    Debug::TraceContextBumper ctx{
                        Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...RolledUpNetworkInterfaces::GetCached...cachefiller")};
                    Debug::TimingTrace ttrc{L"RolledUpNetworkInterfaces::GetCached...cachefiller", 1};

                    // Start with the existing rolled up objects
                    // and merge in any more recent discovery changes
                    RolledUpNetworkInterfaces result = [] () {
                        auto lk = sRolledUpNetworksInterfaces_.rwget ();
                        if (not lk.cref ().has_value ()) {
                            if (not sDBAccessMgr_->GetFinishedInitialDBLoad ()) {
                                // Design Choice - could return non-standardized rollup IDs if DB not loaded, but then those IDs would
                                // disappear later in the run, leading to possible client confusion. Best to just not say anything til DB loaded
                                // Could ALSO do 2 stage DB load - critical stuff for IDs, and the detailed DB records. All we need is first
                                // stage for here...
                                Execution::Throw (HTTP::Exception{HTTP::StatusCodes::kServiceUnavailable, L"Database not yet loaded"_k});
                            };
                            // @todo add more stuff here - empty preset rules from DB
                            // merge two tables - ID to fingerprint and user settings tables and store those in this rollup early
                            // maybe make CTOR for rolledupnetworks take in ital DB netwworks and rules, and have copyis CTOR taking orig networks and new rules?
                            RolledUpNetworkInterfaces rollup =
                                RolledUpNetworkInterfaces{sDBAccessMgr_->GetRawDevices (), sDBAccessMgr_->GetRawNetworkInterfaces ()};
                            // handle orphaned network interfaces
                            {
                                auto orphanedRawInterfaces = rollup.GetRawNetworkInterfaces ().Where (
                                    [&] (auto ni) { return rollup.GetAttachedToDeviceIDs (ni.fID) == nullopt; });
                                if (not orphanedRawInterfaces.empty ()) {
                                    DbgTrace (L"Found: orphanedRawInterfaces=%s", Characters::ToString (orphanedRawInterfaces).c_str ());
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
                    result.MergeIn_ (DiscoveryWrapper_::GetMyDeviceID_ (), DiscoveryWrapper_::GetNetworkInterfaces_ ());
                    sRolledUpNetworksInterfaces_.store (result); // save here so we can update rollup networks instead of creating anew each time
                    return result;
                });
            }

        public:
            /**
             *  Given an aggregated (device) network interface id, map to the correspoding rollup ID
             */
            nonvirtual auto MapAggregatedNetInterfaceID2ItsRollupID (const GUID& netID) const -> GUID
            {
                if (auto r = fMapAggregatedNetInterfaceID2RollupID_.Lookup (netID)) {
                    return *r;
                }
                AssertNotReached ();
                return netID;
            }
            nonvirtual auto MapAggregatedNetInterfaceID2ItsRollupID (const Set<GUID>& netIDs) const -> Set<GUID>
            {
                return netIDs.Map<GUID, Set<GUID>> ([this] (const auto& i) { return MapAggregatedNetInterfaceID2ItsRollupID (i); });
            }

        private:
            void MergeIn_ (const optional<GUID>& forDeviceID, const NetworkInterface& net2MergeIn)
            {
                fRawNetworkInterfaces_ += net2MergeIn;
                // @todo same FAIL logic we have in Network objects needed here
                // friendly name - for example - of network interface can change while running, so must be able to invalidate and recompute this list

                Network::FingerprintType netInterface2MergeInFingerprint = net2MergeIn.GenerateFingerprintFromProperties ();
                auto                     rolledUpNetworkInterace =
                    NetworkInterface::Rollup (fRolledUpNetworkInterfaces_.Lookup (netInterface2MergeInFingerprint), net2MergeIn);
                fRolledUpNetworkInterfaces_.Add (rolledUpNetworkInterace);
                fMapAggregatedNetInterfaceID2RollupID_.Add (net2MergeIn.fID, rolledUpNetworkInterace.fID);
                if (forDeviceID) {
                    fAssociateAggregatedNetInterface2OwningDeviceID.Add (net2MergeIn.fID, *forDeviceID);
                }
            }

        private:
            NetworkInterfaceCollection_ fRawNetworkInterfaces_; // used for RecomputeAll_
            NetworkInterfaceCollection_ fRolledUpNetworkInterfaces_;
            Mapping<GUID, GUID> fMapAggregatedNetInterfaceID2RollupID_; // each aggregate net interface id is mapped to at most one rollup id)
            Association<GUID, GUID> fAssociateAggregatedNetInterface2OwningDeviceID;

        private:
            static Synchronized<optional<RolledUpNetworkInterfaces>> sRolledUpNetworksInterfaces_;
        };
        Synchronized<optional<RolledUpNetworkInterfaces>> RolledUpNetworkInterfaces::sRolledUpNetworksInterfaces_;

        /**
         *  Data structure representing a copy of currently rolled up networks data (copyable).
         * 
         *  This data MAYBE invalidated due to changes in Network::UserOverridesType settings (even historical database data
         *  cuz changes in rollup rules could cause how the historical data was rolled up to change).
         *
         *  \note   \em Thread-Safety   <a href="Thread-Safety.md#C++-Standard-Thread-Safety">C++-Standard-Thread-Safety</a>
         */
        struct RolledUpNetworks {
        public:
            RolledUpNetworks (const Iterable<Network>& nets2MergeIn, const Mapping<GUID, Network::UserOverridesType>& userOverrides,
                              const RolledUpNetworkInterfaces& useNetworkInterfaceRollups)
                : fUseNetworkInterfaceRollups{useNetworkInterfaceRollups}
            {
                fStarterRollups_   = userOverrides.Map<Network> ([] (const auto& guid2UOTPair) -> Network {
                    Network nw;
                    nw.fID            = guid2UOTPair.fKey;
                    nw.fUserOverrides = guid2UOTPair.fValue;
                    if (nw.fUserOverrides and nw.fUserOverrides->fName) {
                        nw.fNames.Add (*nw.fUserOverrides->fName, 500);
                    }
                    return nw;
                });
                fRolledUpNetworks_ = fStarterRollups_;
                MergeIn (nets2MergeIn);
            }
            RolledUpNetworks (const RolledUpNetworks&)            = default;
            RolledUpNetworks (RolledUpNetworks&&)                 = default;
            RolledUpNetworks& operator= (RolledUpNetworks&&)      = default;
            RolledUpNetworks& operator= (const RolledUpNetworks&) = default;

        public:
            /**
             *  This returns the current rolled up network objects.
             */
            nonvirtual NetworkKeyedCollection_ GetNetworks () const { return fRolledUpNetworks_; }

        public:
            nonvirtual void ResetUserOverrides (const Mapping<GUID, Network::UserOverridesType>& userOverrides)
            {
                fStarterRollups_ = userOverrides.Map<Network> ([] (const auto& guid2UOTPair) -> Network {
                    Network nw;
                    nw.fID            = guid2UOTPair.fKey;
                    nw.fUserOverrides = guid2UOTPair.fValue;
                    if (nw.fUserOverrides and nw.fUserOverrides->fName) {
                        nw.fNames.Add (*nw.fUserOverrides->fName, 500);
                    }
                    return nw;
                });
                RecomputeAll_ ();
            }

        public:
            /**
             *  Given an aggregated network id, map to the correspoding rollup ID (todo do we need to handle missing case)
             * 
             *  \req is already valid rollup net ID.
             */
            nonvirtual auto MapAggregatedID2ItsRollupID (const GUID& netID) const -> GUID
            {
                if (auto r = fMapAggregatedNetID2RollupID_.Lookup (netID)) {
                    return *r;
                }
                // shouldn't get past here - debug if/why this hapepns - see comments below
                Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (
                    L"MapAggregatedID2ItsRollupID failed to find netID=%s", Characters::ToString (netID).c_str ())};
                if constexpr (qDebug) {
                    for ([[maybe_unused]] const auto& i : fRolledUpNetworks_) {
                        DbgTrace (L"rolledupNet=%s", Characters::ToString (i).c_str ());
                    }
                }
                Assert (false); // @todo fix - because we guarantee each item rolled up exactly once - but happens sometimes on change of network - I think due to outdated device records referring to newer network not yet in this cache...
                WeakAssert (false); // @todo fix - because we guarantee each item rolled up exactly once - but happens sometimes on change of network - I think due to outdated device records referring to newer network not yet in this cache...
                return netID;
            }

        public:
            /**
             *  Modify this set of rolled up networks with this net2MergeIn. If we've seen this network before (by ID)
             *  possibly remove it from some rollup, and possibly even remove that (if now empty) rollup.
             *
             *  But typically this will just update the record for an existing rollup.
             */
            nonvirtual void MergeIn (const Iterable<Network>& nets2MergeIn)
            {
                fRawNetworks_ += nets2MergeIn;
                bool anyFailed = false;
                for (const Network& n : nets2MergeIn) {
                    if (MergeIn_ (n) == PassFailType_::eFail) {
                        anyFailed = true;
                        break;
                    }
                }
                if (anyFailed) {
                    RecomputeAll_ ();
                }
            }

        public:
            /**
             * INVALIDATE IF 'UserSettings' change, which might cause different rollups (this includes fingerprint to guid map)
             *
             *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
             */
            static RolledUpNetworks GetCached (Time::DurationSecondsType allowedStaleness = 5.0)
            {
                Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...RolledUpNetworks::GetCached")};
                Debug::TimingTrace        ttrc{L"RolledUpNetworks::GetCached", 1};
                // SynchronizedCallerStalenessCache object just assures one rollup RUNS internally at a time, and
                // that two calls in rapid succession, the second call re-uses the previous value
                static Cache::SynchronizedCallerStalenessCache<void, RolledUpNetworks> sCache_;
                // Disable fHoldWriteLockDuringCacheFill due to https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/23
                // See also
                //      https://stroika.atlassian.net/browse/STK-906 - possible enhancement to this configuration to work better avoiding
                //      See https://stroika.atlassian.net/browse/STK-907 - about needing some new mechanism in Stroika for deadlock detection/avoidance.
                // sCache_.fHoldWriteLockDuringCacheFill = true; // so only one call to filler lambda at a time
                return sCache_.LookupValue (sCache_.Ago (allowedStaleness), [allowedStaleness] () -> RolledUpNetworks {
                    /*
                     *  DEADLOCK NOTE
                     *      Since this can be called while rolling up DEVICES, its important that this code not call anything involving device rollup since
                     *      that could trigger a deadlock.
                     */
                    Debug::TraceContextBumper ctx{
                        Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...RolledUpNetworks::GetCached...cachefiller")};
                    Debug::TimingTrace ttrc{L"RolledUpNetworks::GetCached...cachefiller", 1};

                    // Start with the existing rolled up objects
                    // and merge in any more recent discovery changes
                    RolledUpNetworks result = [allowedStaleness] () {
                        auto rolledUpNetworkInterfacess = RolledUpNetworkInterfaces::GetCached (allowedStaleness * 3.0); // longer allowedStaleness cuz we dont care much about this and the parts
                            // we look at really dont change
                        auto lk = sRolledUpNetworks_.rwget ();
                        if (not lk.cref ().has_value ()) {
                            if (not sDBAccessMgr_->GetFinishedInitialDBLoad ()) {
                                // Design Choice - could return non-standardized rollup IDs if DB not loaded, but then those IDs would
                                // disappear later in the run, leading to possible client confusion. Best to just not say anything til DB loaded
                                // Could ALSO do 2 stage DB load - critical stuff for IDs, and the detailed DB records. All we need is first
                                // stage for here...
                                Execution::Throw (HTTP::Exception{HTTP::StatusCodes::kServiceUnavailable, L"Database not yet loaded"_k});
                            };
                            // @todo add more stuff here - empty preset rules from DB
                            // merge two tables - ID to fingerprint and user settings tables and store those in this rollup early
                            // maybe make CTOR for rolledupnetworks take in ital DB netwworks and rules, and have copyis CTOR taking orig networks and new rules?
                            lk.store (RolledUpNetworks{sDBAccessMgr_->GetRawNetworks (), sDBAccessMgr_->GetNetworkUserSettings (), rolledUpNetworkInterfacess});
                        }
                        return Memory::ValueOf (lk.load ());
                    }();
                    result.MergeIn (DiscoveryWrapper_::GetNetworks_ ());
                    sRolledUpNetworks_.store (result); // save here so we can update rollup networks instead of creating anew each time
                    return result;
                });
            }

        public:
            /**
             *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
             */
            static void InvalidateCache ()
            {
                auto lk = sRolledUpNetworks_.rwget ();
                if (lk->has_value ()) {
                    lk.rwref ()->ResetUserOverrides (sDBAccessMgr_->GetNetworkUserSettings ());
                }
                // else OK if not yet loaded, nothing to invalidate
            }

        private:
            enum class PassFailType_ {
                ePass,
                eFail
            };
            // if fails simple merge, returns false, so must call recomputeall
            PassFailType_ MergeIn_ (const Network& net2MergeIn)
            {
                // @todo https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/75 - fix corner case
                Network::FingerprintType net2MergeInFingerprint      = net2MergeIn.GenerateFingerprintFromProperties ();
                const auto [oShouldRollIntoNet, shouldInvalidateAll] = ShouldRollupInto_ (net2MergeIn, net2MergeInFingerprint);
                if (shouldInvalidateAll == PassFailType_::ePass) {
                    if (oShouldRollIntoNet) {
                        // then we rollup to this same rollup network - very common case - so just re-rollup, and we are done
                        AddUpdateIn_ (*oShouldRollIntoNet, net2MergeIn, net2MergeInFingerprint);
                    }
                    else {
                        AddNewIn_ (net2MergeIn, net2MergeInFingerprint);
                    }
                    return PassFailType_::ePass;
                }
                return PassFailType_::eFail;
            }
            // Find the appropriate network to merge net2MergeIn into from our existing networks (using whatever keys we have to make this often quicker)
            // This can be slow for the case of a network change. And it can refer to a different rollup network than it used to.
            // second part of tuple returned is 'revalidateAll' - force recompute of whole rollup

            // @todo NOTE - a net COULD be directed to rollup into one network due to fingerprint and another due to matching gateway hardware ID
            // RESOLVE this by agreeing that hardwareGatewayID takes PRECEDENCE.
            //
            // And consider it a DATA ERROR (need code to assure cannot happen) - for the same hardare ID to appear in two rollup networks match rules
            // or the same (any type of rollup rule) to appear in differnt high level user settings)
            tuple<optional<Network>, PassFailType_> ShouldRollupInto_ (const Network& net2MergeIn, const Network::FingerprintType& net2MergeInFingerprint)
            {
                auto formerRollupID = fMapFingerprint2RollupID.Lookup (net2MergeInFingerprint);
                if (formerRollupID) {
                    auto alreadyRolledUpNetwork = Memory::ValueOf (fRolledUpNetworks_.Lookup (*formerRollupID)); // must be in list because we keep those in sync here in this class
                    if (ShouldRollupInto_CheckIsCompatibleWithTarget_ (net2MergeIn, net2MergeInFingerprint, alreadyRolledUpNetwork)) {
                        return make_tuple (alreadyRolledUpNetwork, PassFailType_::ePass);
                    }
                }
                // SEARCH FULL LIST of already rolled up networks to find the right network to roll into; if we already had rolled into a
                // different one, don't bother cuz have to recompute anyhow
                if (not formerRollupID.has_value ()) {
                    for (const auto& ri : fRolledUpNetworks_) {
                        if (ShouldRollupInto_CheckIsCompatibleWithTarget_ (net2MergeIn, net2MergeInFingerprint, ri)) {
                            return make_tuple (ri, PassFailType_::ePass);
                        }
                    }
                }
                return make_tuple (nullopt, PassFailType_::eFail);
            }
            bool ShouldRollupInto_CheckIsCompatibleWithTarget_ (const Network& net2MergeIn, const Network::FingerprintType& net2MergeInFingerprint,
                                                                const Network& targetRollup)
            {
                if (auto riu = targetRollup.fUserOverrides) {
                    if (riu->fAggregateFingerprints and riu->fAggregateFingerprints->Contains (net2MergeInFingerprint)) {
                        return true;
                    }
                    if (riu->fAggregateGatewayHardwareAddresses and
                        riu->fAggregateGatewayHardwareAddresses->Intersects (net2MergeIn.fGatewayHardwareAddresses)) {
                        return true;
                    }
                    if (riu->fAggregateNetworks and riu->fAggregateNetworks->Contains (net2MergeIn.fID)) {
                        return true;
                    }
                    if (riu->fAggregateNetworkInterfacesMatching) {
                        for (const auto& rule : *riu->fAggregateNetworkInterfacesMatching) {
                            // net2MergeIn is unaggregated and so net2MergeIn.fAttachedInterfaces are as well dont call fUseNetworkInterfaceRollups.GetConcreteNeworkInterfaces
                            // but also need api to grab aggreaged guy by ID
                            if (fUseNetworkInterfaceRollups.GetRawNetworkInterfaces (net2MergeIn.fAttachedInterfaces).All ([&] (const NetworkInterface& i) {
                                    return i.fType == rule.fInterfaceType and i.GenerateFingerprintFromProperties () == rule.fFingerprint;
                                })) {
                                return true;
                            }
                        }
                    }
                }
                return false;
            }
            void AddUpdateIn_ (const Network& addNet2MergeFromThisRollup, const Network& net2MergeIn, const Network::FingerprintType& net2MergeInFingerprint)
            {
                Assert (net2MergeIn.GenerateFingerprintFromProperties () == net2MergeInFingerprint); // provided to avoid cost of recompute
                Network newRolledUpNetwork = Network::Rollup (addNet2MergeFromThisRollup, net2MergeIn);
                newRolledUpNetwork.fAttachedInterfaces +=
                    fUseNetworkInterfaceRollups.MapAggregatedNetInterfaceID2ItsRollupID (net2MergeIn.fAttachedInterfaces);
                Assert (addNet2MergeFromThisRollup.fAggregatesFingerprints == newRolledUpNetwork.fAggregatesFingerprints); // spot check - should be same...
                fRolledUpNetworks_.Add (newRolledUpNetwork);
                fMapAggregatedNetID2RollupID_.Add (net2MergeIn.fID, newRolledUpNetwork.fID);
                fMapFingerprint2RollupID.Add (net2MergeInFingerprint, newRolledUpNetwork.fID);
            }
            void AddNewIn_ (const Network& net2MergeIn, const Network::FingerprintType& net2MergeInFingerprint)
            {
                Assert (net2MergeIn.GenerateFingerprintFromProperties () == net2MergeInFingerprint); // provided to avoid cost of recompute
                Network newRolledUpNetwork = net2MergeIn;
                newRolledUpNetwork.fAttachedInterfaces =
                    fUseNetworkInterfaceRollups.MapAggregatedNetInterfaceID2ItsRollupID (net2MergeIn.fAttachedInterfaces);
                newRolledUpNetwork.fAggregatesReversibly   = Set<GUID>{net2MergeIn.fID};
                newRolledUpNetwork.fAggregatesFingerprints = Set<Network::FingerprintType>{net2MergeInFingerprint};
                // @todo fix this code so each time through we UPDATE sDBAccessMgr_ with latest 'fingerprint' of each dynamic network
                newRolledUpNetwork.fID = sDBAccessMgr_->GenNewNetworkID (newRolledUpNetwork, net2MergeIn);
                if (fRolledUpNetworks_.Contains (newRolledUpNetwork.fID)) {
                    // Should probably never happen, but since depends on data in database, program defensively

                    // at this point we have a net2MergeIn that said 'no' to ShouldRollup to all existing networks we've rolled up before
                    // and yet somehow, result contains a network that used our ID?
                    auto shouldntRollUpButTookOurIDNet = Memory::ValueOf (fRolledUpNetworks_.Lookup (newRolledUpNetwork.fID));
                    DbgTrace (L"shouldntRollUpButTookOurIDNet=%s", Characters::ToString (shouldntRollUpButTookOurIDNet).c_str ());
                    DbgTrace (L"net2MergeIn=%s", Characters::ToString (net2MergeIn).c_str ());
                    //Assert (not ShouldRollup_ (shouldntRollUpButTookOurIDNet, net2MergeIn));
                    Logger::sThe.Log (Logger::eWarning, L"Got rollup network ID from cache that is already in use: %s (for external address %s)",
                                      Characters::ToString (newRolledUpNetwork.fID).c_str (),
                                      Characters::ToString (newRolledUpNetwork.fExternalAddresses).c_str ());
                    newRolledUpNetwork.fID = GUID::GenerateNew ();
                }
                newRolledUpNetwork.fUserOverrides = sDBAccessMgr_->LookupNetworkUserSettings (newRolledUpNetwork.fID);
                if (newRolledUpNetwork.fUserOverrides && newRolledUpNetwork.fUserOverrides->fName) {
                    newRolledUpNetwork.fNames.Add (*newRolledUpNetwork.fUserOverrides->fName, 500);
                }
                fRolledUpNetworks_.Add (newRolledUpNetwork);
                fMapAggregatedNetID2RollupID_.Add (net2MergeIn.fID, newRolledUpNetwork.fID);

                // is this guarnateed unique?
                fMapFingerprint2RollupID.Add (net2MergeInFingerprint, newRolledUpNetwork.fID);
            }
            void RecomputeAll_ ()
            {
                Debug::TraceContextBumper ctx{"{}...RolledUpNetworks::RecomputeAll_"};
                fRolledUpNetworks_.clear ();
                fMapAggregatedNetID2RollupID_.clear ();
                fMapFingerprint2RollupID.clear ();
                fRolledUpNetworks_ += fStarterRollups_;
                fRawNetworks_.Apply ([this] (const Network& n) {
                    if (MergeIn_ (n) == PassFailType_::eFail) {
                        AddNewIn_ (n, n.GenerateFingerprintFromProperties ());
                    }
                });
            }

        private:
            RolledUpNetworkInterfaces fUseNetworkInterfaceRollups;
            NetworkKeyedCollection_   fRawNetworks_; // used for RecomputeAll_
            NetworkKeyedCollection_   fStarterRollups_;
            NetworkKeyedCollection_   fRolledUpNetworks_;
            Mapping<GUID, GUID>       fMapAggregatedNetID2RollupID_;          // each aggregate netid is mapped to at most one rollup id)
            Mapping<Network::FingerprintType, GUID> fMapFingerprint2RollupID; // each fingerprint can map to at most one rollup...
        private:
            static Synchronized<optional<RolledUpNetworks>> sRolledUpNetworks_;
        };
        Synchronized<optional<RolledUpNetworks>> RolledUpNetworks::sRolledUpNetworks_;

        /**
         *  Data structure representing a copy of currently rolled up devices data (copyable).
         *
         *  This data MAYBE invalidated due to changes in Network::UserOverridesType settings (even historical database data
         *  cuz changes in rollup rules could cause how the historical data was rolled up to change).
         *
         *  \note   \em Thread-Safety   <a href="Thread-Safety.md#C++-Standard-Thread-Safety">C++-Standard-Thread-Safety</a>
         */
        struct RolledUpDevices {
        public:
            RolledUpDevices (const Iterable<Device>& devices2MergeIn, const Mapping<GUID, Device::UserOverridesType>& userOverrides,
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
                MergeIn (devices2MergeIn);
            }
            RolledUpDevices (const RolledUpDevices&)            = default;
            RolledUpDevices (RolledUpDevices&&)                 = default;
            RolledUpDevices& operator= (const RolledUpDevices&) = default;
            RolledUpDevices& operator= (RolledUpDevices&&)      = default;

        public:
            /**
             *  This returns the current rolled up device objects.
             */
            nonvirtual DeviceKeyedCollection_ GetDevices () const { return fRolledUpDevices; }

        public:
            /**
             *  Given an aggregated device id, map to the correspoding rollup ID (todo do we need to handle missing case)
             * 
             *  \req is already valid rollup ID.
             */
            nonvirtual auto MapAggregatedID2ItsRollupID (const GUID& aggregatedDeviceID) const -> GUID
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

        public:
            nonvirtual void ResetUserOverrides (const Mapping<GUID, Device::UserOverridesType>& userOverrides)
            {
                RolledUpNetworkInterfaces networkInterfacesRollup = RolledUpNetworkInterfaces::GetCached ();
                fStarterRollups_                                  = userOverrides.Map<Device> ([] (const auto& guid2UOTPair) -> Device {
                    Device d;
                    d.fID            = guid2UOTPair.fKey;
                    d.fUserOverrides = guid2UOTPair.fValue;
                    if (d.fUserOverrides and d.fUserOverrides->fName) {
                        d.fNames.Add (*d.fUserOverrides->fName, 500);
                    }
                    return d;
                });
                RecomputeAll_ ();
            }

        public:
            nonvirtual void MergeIn (const Iterable<Device>& devices2MergeIn)
            {
                fRawDevices_ += devices2MergeIn;
                bool anyFailed = false;
                for (const Device& d : devices2MergeIn) {
                    if (MergeIn_ (d) == PassFailType_::eFail) {
                        anyFailed = true;
                        break;
                    }
                }
                if (anyFailed) {
                    RecomputeAll_ ();
                }
            }

        public:
            /**
             *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
             */
            static RolledUpDevices GetCached (Time::DurationSecondsType allowedStaleness = 10.0)
            {
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
                return sCache_.LookupValue (sCache_.Ago (allowedStaleness), [=] () -> RolledUpDevices {
                    Debug::TraceContextBumper ctx{
                        Stroika_Foundation_Debug_OptionalizeTraceArgs (L"...RolledUpDevices::GetCached...cachefiller")};
                    Debug::TimingTrace ttrc{L"RolledUpDevices::GetCached...cachefiller", 1};

                    auto rolledUpNetworks = RolledUpNetworks::GetCached (allowedStaleness * 3.0); // longer allowedStaleness cuz we dont care much about this and the parts
                                                                                                  // we look at really dont change
                    auto rolledUpNetworkInterfacess = RolledUpNetworkInterfaces::GetCached (allowedStaleness * 3.0); // longer allowedStaleness cuz we dont care much about this and the parts
                        // we look at really dont change

                    // Start with the existing rolled up objects
                    // and merge in any more recent discovery changes
                    RolledUpDevices result = [&] () {
                        auto lk = sRolledUpDevicesSoFar_.rwget ();
                        if (not lk.cref ().has_value ()) {
                            if (not sDBAccessMgr_->GetFinishedInitialDBLoad ()) {
                                // Design Choice - could return non-standardized rollup IDs if DB not loaded, but then those IDs would
                                // disappear later in the run, leading to possible client confusion. Best to just not say anything til DB loaded
                                // Could ALSO do 2 stage DB load - critical stuff for IDs, and the detailed DB records. All we need is first
                                // stage for here...
                                Execution::Throw (HTTP::Exception{HTTP::StatusCodes::kServiceUnavailable, L"Database not yet loaded"_k});
                            }
                            // @todo add more stuff here - empty preset rules from DB
                            // merge two tables - ID to fingerprint and user settings tables and store those in this rollup early
                            // maybe make CTOR for rolledupnetworks take in ital DB netwworks and rules, and have copyis CTOR taking orig networks and new rules?
                            RolledUpDevices initialDBDevices{sDBAccessMgr_->GetRawDevices (), sDBAccessMgr_->GetDeviceUserSettings (),
                                                             rolledUpNetworks, rolledUpNetworkInterfacess};
                            lk.store (initialDBDevices);
                        }
                        return Memory::ValueOf (lk.load ());
                    }();
                    // not sure we want to allow this? @todo consider throwing here or asserting out cuz nets rollup IDs would change after this
                    result.MergeIn (DiscoveryWrapper_::GetDevices_ ());
                    sRolledUpDevicesSoFar_.store (result); // save here so we can update rollup networks instead of creating anew each time
                    return result;
                });
            }

        public:
            /**
             *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
             */
            static void InvalidateCache ()
            {
                auto lk = sRolledUpDevicesSoFar_.rwget ();
                if (lk->has_value ()) {
                    lk.rwref ()->ResetUserOverrides (sDBAccessMgr_->GetDeviceUserSettings ());
                }
                // else OK if not yet loaded, nothing to invalidate
            }

        private:
            enum class PassFailType_ {
                ePass,
                eFail
            };
            // if fails simple merge, returns false, so must call recomputeall
            PassFailType_ MergeIn_ (const Device& d2MergeIn)
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
                        MergeInNew_ (d2MergeIn);
                    }
                    return PassFailType_::ePass;
                }
            }
            void MergeInUpdate_ (const Device& rollupDevice, const Device& newDevice2MergeIn)
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
            void MergeInNew_ (const Device& d2MergeIn)
            {
                Assert (not d2MergeIn.fAggregatesReversibly.has_value ());
                Device newRolledUpDevice                = d2MergeIn;
                newRolledUpDevice.fAggregatesReversibly = Set<GUID>{d2MergeIn.fID};
                newRolledUpDevice.fID                   = sDBAccessMgr_->GenNewDeviceID (d2MergeIn.GetHardwareAddresses ());
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
                newRolledUpDevice.fUserOverrides = sDBAccessMgr_->LookupDevicesUserSettings (newRolledUpDevice.fID);
                if (newRolledUpDevice.fUserOverrides && newRolledUpDevice.fUserOverrides->fName) {
                    newRolledUpDevice.fNames.Add (*newRolledUpDevice.fUserOverrides->fName, 500);
                }
                fRolledUpDevices += newRolledUpDevice;
                fRaw2RollupIDMap_.Add (d2MergeIn.fID, newRolledUpDevice.fID);
            }
            void RecomputeAll_ ()
            {
                fRolledUpDevices.clear ();
                fRaw2RollupIDMap_.clear ();
                fRolledUpDevices += fStarterRollups_;
                for (const auto& di : fRawDevices_) {
                    if (MergeIn_ (di) == PassFailType_::eFail) {
                        Assert (false); //nyi - or maybe just bug since we have mapping so device goes into two different rollups?
                    }
                }
            }
            auto MapAggregatedAttachments2Rollups_ (const Mapping<GUID, NetworkAttachmentInfo>& nats) -> Mapping<GUID, NetworkAttachmentInfo>
            {
                Mapping<GUID, NetworkAttachmentInfo> result;
                for (const auto& ni : nats) {
                    result.Add (fUseRolledUpNetworks.MapAggregatedID2ItsRollupID (ni.fKey), ni.fValue);
                }
                return result;
            };
            static bool ShouldRollup_ (const Device& exisingRolledUpDevice, const Device& d2PotentiallyMergeIn)
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

        private:
            DeviceKeyedCollection_    fStarterRollups_;
            DeviceKeyedCollection_    fRolledUpDevices;
            DeviceKeyedCollection_    fRawDevices_;
            Mapping<GUID, GUID>       fRaw2RollupIDMap_; // each aggregate deviceid is mapped to at most one rollup id)
            RolledUpNetworks          fUseRolledUpNetworks;
            RolledUpNetworkInterfaces fUseNetworkInterfaceRollups;
            // sRolledUpDevicesSoFar_: keep a cache of the rolled up devices so far, just
            // as a slight performance tweek
            static Synchronized<optional<RolledUpDevices>> sRolledUpDevicesSoFar_;
        };
        Synchronized<optional<RolledUpDevices>> RolledUpDevices::sRolledUpDevicesSoFar_;

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
    Require (sDBAccessMgr_ == nullopt);
    sDBAccessMgr_.emplace ();
}

IntegratedModel::Mgr::Activator::~Activator ()
{
    Debug::TraceContextBumper ctx{L"IntegratedModel::Mgr::Activator::~Activator"};
    Execution::Thread::SuppressInterruptionInContext suppressInterruption; // must complete this abort and wait for done - this cannot abort/throw
    sDBAccessMgr_ = nullopt;
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
    return Sequence<IntegratedModel::Device>{RollupSummary_::RolledUpDevices::GetCached ().GetDevices ()};
}

optional<IntegratedModel::Device> IntegratedModel::Mgr::GetDevice (const GUID& id, optional<Duration>* ttl) const
{
    // first check rolled up devices, and then raw/unrolled up devices
    // NOTE - this doesn't check the 'dynamic' copy of the devices - it waits til those get migrated to the DB, once ever
    // 30 seconds roughtly...
    auto devicesRollupCache = RollupSummary_::RolledUpDevices::GetCached ();
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
        if (result = sDBAccessMgr_->GetRawDevices ().Lookup (id)) {
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
        RollupSummary_::RolledUpDevices::InvalidateCache ();
    }
}

std::optional<GUID> IntegratedModel::Mgr::GetCorrespondingDynamicDeviceID (const GUID& id) const
{
    Set<GUID> dynamicDevices{Discovery::DevicesMgr::sThe.GetActiveDevices ().Map<GUID, Set<GUID>> ([] (const auto& d) { return d.fGUID; })};
    if (dynamicDevices.Contains (id)) {
        return id;
    }
    auto thisRolledUpDevice = RollupSummary_::RolledUpDevices::GetCached ().GetDevices ().Lookup (id);
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
    return Sequence<IntegratedModel::Network>{RollupSummary_::RolledUpNetworks::GetCached ().GetNetworks ()};
}

optional<IntegratedModel::Network> IntegratedModel::Mgr::GetNetwork (const GUID& id, optional<Duration>* ttl) const
{
    // first check rolled up networks, and then raw/unrolled up networks
    auto networkRollupsCache = RollupSummary_::RolledUpNetworks::GetCached ();
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
        RollupSummary_::RolledUpNetworks::InvalidateCache ();
    }
}

Collection<IntegratedModel::NetworkInterface> IntegratedModel::Mgr::GetNetworkInterfaces () const
{
    // AS OF 2022-11-02 this returns the currently active network interfaces, but changed to mimic other accessors (rollups returned)
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"IntegratedModel::Mgr::GetNetworkInterfaces")};
    Debug::TimingTrace        ttrc{L"IntegratedModel::Mgr::GetNetworkInterfaces", 0.1};
    return Collection<IntegratedModel::NetworkInterface>{RollupSummary_::RolledUpNetworkInterfaces::GetCached ().GetNetworkInterfacess ()};
}

optional<IntegratedModel::NetworkInterface> IntegratedModel::Mgr::GetNetworkInterface (const GUID& id, optional<Duration>* ttl) const
{
    // AS OF 2022-11-02 this returned the currently active network interfaces, but changed (2022-11-02) to mimic other accessors (rollups returned by default then raw records)
    auto networkInterfacesCache = RollupSummary_::RolledUpNetworkInterfaces::GetCached ();
    auto result                 = networkInterfacesCache.GetNetworkInterfacess ().Lookup (id);
    if (result) {
        if (ttl != nullptr) {
            *ttl = kTTLForRollupsReturned_;
        }
        auto deviceRollupCache = RollupSummary_::RolledUpDevices::GetCached ();
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