/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Mgr_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Mgr_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include <optional>

#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/Containers/Sequence.h"

#include "../WebServices/Model.h"

/**
 *  Wrapper on the discovered devices and networks, as well as persistence and rollup.
 */
namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::IntegratedModel {

    using namespace std;

    using Stroika::Foundation::Containers::Collection;
    using Stroika::Foundation::Containers::Sequence;
    using Stroika::Foundation::Time::Duration;

    using WebServices::Model::Device;
    using WebServices::Model::DeviceCollection;
    using WebServices::Model::Network;
    using WebServices::Model::NetworkAttachmentInfo;
    using WebServices::Model::NetworkCollection;
    using WebServices::Model::NetworkInterface;
    using WebServices::Model::NetworkInterfaceCollection;

    /**
     *  Wrapper on the discovered devices and networks, as well as persistence. It adds its own IDs, separate
     *  from discovered ids (and map them and provide apis to adjust mapping).
     * 
     *  This is where all the object identity logic and rollup logic lies.
     * 
     *  This delegates to the layer that does database storage, and discovery. And it provides access to settings
     *  that configure how rollup is done.
     * 
     *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
     */
    class Mgr {
    private:
        Mgr ()           = default;
        Mgr (const Mgr&) = delete;

    public:
        /**
         *  At most one such object may exist. When it does, the IntegratedModel is active and usable. Its illegal to call otherwise.
         */
        struct Activator {
            Activator ();
            ~Activator ();
        };

    public:
        static Mgr sThe;

    public:
        /**
         */
        nonvirtual Sequence<Device> GetDevices () const;

    public:
        /**
         *  Note that 'id' could refer to a rolled-up device, or an aggregated device, and the appropriate type of Device object
         *  will be returned. If the id doesn't refer to a device, this returns nullopt, not an exception.
         */
        nonvirtual optional<Device> GetDevice (const Common::GUID& id, optional<Duration>* ttl = nullptr) const;

    public:
        /**
         *  If there are no user overrides (or the id is invalid) this will return nullopt.
         */
        nonvirtual optional<Device::UserOverridesType> GetDeviceUserSettings (const Common::GUID& id) const;

    public:
        /**
         *  if the id is invalid, this will throw (id refers to a rolled up object id). If settings are nullopt, this
         *  clears the user settings.
         */
        nonvirtual void SetDeviceUserSettings (const Common::GUID& id, const std::optional<Device::UserOverridesType>& settings);

    public:
        /**
         *  For the given id, which can be a rolledup or aggregated device id, return the corresponding
         *  dynamic device id, if any. Can return nullopt if none found.
         */
        nonvirtual std::optional<Common::GUID> GetCorrespondingDynamicDeviceID (const Common::GUID& id) const;

    public:
        /**
         */
        nonvirtual Sequence<Network> GetNetworks () const;

    public:
        /**
         */
        nonvirtual std::optional<Network> GetNetwork (const Common::GUID& id, optional<Duration>* ttl = nullptr) const;

    public:
        /**
         *  If there are no user overrides (or the id is invalid) this will return nullopt.
         */
        nonvirtual std::optional<Network::UserOverridesType> GetNetworkUserSettings (const Common::GUID& id) const;

    public:
        /**
         *  if the id is invalid, this will throw (id refers to a rolled up object id). If settings are nullopt, this
         *  clears the user settings.
         */
        nonvirtual void SetNetworkUserSettings (const Common::GUID& id, const std::optional<Network::UserOverridesType>& settings);

    public:
        /**
         */
        nonvirtual Collection<NetworkInterface> GetNetworkInterfaces () const;

    public:
        /**
         */
        nonvirtual std::optional<NetworkInterface> GetNetworkInterface (const Common::GUID& id, optional<Duration>* ttl = nullptr) const;
    };
    inline Mgr Mgr::sThe;

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "Mgr.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Mgr_h_*/
