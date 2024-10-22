/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Private_DBAccess_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Private_DBAccess_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include <optional>

#include "Stroika/Foundation/Common/Version.h"
#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/Containers/Sequence.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/Database/SQL/ORM/Schema.h"
#include "Stroika/Foundation/Database/SQL/ORM/TableConnection.h"
#include "Stroika/Foundation/Database/SQL/ORM/Versioning.h"
#include "Stroika/Foundation/Database/SQL/SQLite.h"
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/Execution/Logger.h"
#include "Stroika/Foundation/Execution/Sleep.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/Execution/Thread.h"

#include "../../Common/DB.h"
#include "../../WebServices/Model.h"

/**
 
 */
namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::IntegratedModel::Private_::DBAccess {

    using namespace std;

    using namespace Stroika::Foundation;
    using namespace Stroika::Foundation::Characters;
    using namespace Stroika::Foundation::Common;
    using namespace Stroika::Foundation::Containers;
    using namespace Stroika::Foundation::Database;
    using namespace Stroika::Foundation::Debug;

    using Stroika::Foundation::Common::GUID;
    using Stroika::Foundation::Containers::Collection;
    using Stroika::Foundation::Containers::Sequence;
    using Stroika::Foundation::DataExchange::ObjectVariantMapper;
    using Stroika::Foundation::DataExchange::VariantValue;
    using Stroika::Foundation::Execution::Synchronized;
    using Stroika::Foundation::Time::Duration;

    using WebServices::Model::Device;
    using WebServices::Model::DeviceCollection;
    using WebServices::Model::Network;
    using WebServices::Model::NetworkAttachmentInfo;
    using WebServices::Model::NetworkCollection;
    using WebServices::Model::NetworkInterface;
    using WebServices::Model::NetworkInterfaceCollection;

    /**
     *  Wrapper on Database access all goes in this DBAccess::Mgr_ module; All ORM/data mapping etc
     *  goes on inside this module. All data upgrade etc processes in this module. All DB IO in this module.
     *  WebServices::Model objects  (Devices etc) go in and out.
     * 
     *  \note not a singleton in of itself. Lifetime externally managed. But there exists at most one of them.
     * 
     *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
     */
    class Mgr {
    public:
        Mgr ();
        Mgr (const Mgr&) = delete;

    public:
        Mgr& operator= (const Mgr&) = delete;

    public:
        ~Mgr ();

    public:
        nonvirtual GUID GenNewDeviceID (const Set<String>& hwAddresses);

    public:
        nonvirtual GUID GenNewNetworkID ([[maybe_unused]] const Network& rollupNetwork, const Network& containedNetwork);

    public:
        nonvirtual Mapping<GUID, Device::UserOverridesType> GetDeviceUserSettings () const;

    public:
        nonvirtual optional<Device::UserOverridesType> LookupDevicesUserSettings (const GUID& guid) const;

    public:
        nonvirtual bool SetDeviceUserSettings (const GUID& id, const std::optional<Device::UserOverridesType>& settings);

    public:
        nonvirtual Mapping<GUID, Network::UserOverridesType> GetNetworkUserSettings () const;

    public:
        nonvirtual optional<Network::UserOverridesType> LookupNetworkUserSettings (const GUID& guid) const;

    public:
        // return true if changed
        nonvirtual bool SetNetworkUserSettings (const GUID& id, const std::optional<Network::UserOverridesType>& settings);

    public:
        nonvirtual NetworkInterfaceCollection GetRawNetworkInterfaces () const;

    public:
        nonvirtual NetworkCollection GetRawNetworks () const;

    public:
        nonvirtual DeviceCollection GetRawDevices () const;

    public:
        /**
         *  Throw if dbload/setup not yet completed.
         */
        virtual void CheckDatabaseLoadCompleted () = 0;

    private:
        using Schema_Table         = SQL::ORM::Schema::Table;
        using Schema_Field         = SQL::ORM::Schema::Field;
        using Schema_CatchAllField = SQL::ORM::Schema::CatchAllField;

    private:
        static constexpr auto kRepresentIDAs_ = BackendApp::Common::DB::kRepresentIDAs_;

    private:
        static String GenRandomIDString_ (VariantValue::Type t);

    private:
        struct ExternalDeviceUserSettingsElt_ {
            GUID                      fDeviceID; // rolled up device id
            Device::UserOverridesType fUserSettings;
        };

    private:
        struct ExternalNetworkUserSettingsElt_ {
            GUID                       fNetworkID; // rolled up network id
            Network::UserOverridesType fUserSettings;
        };

    private:
        /*
         *  Combined mapper for objects we write to the database. Contains all the objects mappers we need merged together,
         *  and any touchups on represenation we need (like writing GUID as BLOB rather than string).
         */
        static const ConstantProperty<ObjectVariantMapper> kDBObjectMapper_;
        static const Schema_Table                          kDeviceUserSettingsSchema_;
        static const Schema_Table                          kNetworkUserSettingsSchema_;
        static const Schema_Table                          kDeviceTableSchema_;

        static const Schema_Table kNetworkInterfaceTableSchema_;
        static const Schema_Table kNetworkTableSchema_;

    private:
        static constexpr Version kCurrentVersion_ = Version{1, 0, VersionStage::Alpha, 0};
        BackendApp::Common::DB fDB_; // Not accessed directly except during construction/destruction (each TableConnection gets its own DB::ConnectionPtr)
        Execution::Thread::Ptr                                                               fDatabaseSyncThread_{};
        Synchronized<Mapping<GUID, Device::UserOverridesType>>                               fCachedDeviceUserSettings_;
        Synchronized<unique_ptr<SQL::ORM::TableConnection<ExternalDeviceUserSettingsElt_>>>  fDeviceUserSettingsTableConnection_;
        Synchronized<Mapping<GUID, Network::UserOverridesType>>                              fCachedNetworkUserSettings_;
        Synchronized<unique_ptr<SQL::ORM::TableConnection<ExternalNetworkUserSettingsElt_>>> fNetworkUserSettingsTableConnection_;
        unique_ptr<SQL::ORM::TableConnection<Device>>           fDeviceTableConnection_;  // only accessed from a background database thread
        unique_ptr<SQL::ORM::TableConnection<Network>>          fNetworkTableConnection_; // ''
        unique_ptr<SQL::ORM::TableConnection<NetworkInterface>> fNetworkInterfaceTableConnection_; // ''
        Synchronized<DeviceCollection>                          fDBDevices_;                       // mirror database contents in RAM
        Synchronized<NetworkCollection>                         fDBNetworks_;                      // ''
        Synchronized<NetworkInterfaceCollection>                fDBNetworkInterfaces_;             // ''

        // the latest copy of what is in the DB (manually kept up to date)
        // NOTE: These are all non-rolled up objects
        Synchronized<Mapping<String, GUID>> fAdvisoryHWAddr2GUIDCache_;

    protected:
        void _StartBackgroundThread ();

    private:
        void BackgroundDatabaseThread_ ();

    protected:
        /*
         *  Called to load the database. Even if the database is not present (being created) - this will be called once
         *  and must succeeed. Subclasses can override it to report back/know when database load is done.
         */
        virtual void _OneTimeStartupLoadDB ();
    };

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "DBAccess.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Private_DBAccess_h_*/
