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
 *
 */
namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::IntegratedModel {

    using Stroika::Foundation::Containers::Collection;
    using Stroika::Foundation::Containers::Sequence;

    using WebServices::Model::Device;
    using WebServices::Model::Network;
    using WebServices::Model::NetworkAttachmentInfo;
    using WebServices::Model::NetworkInterface;

    /**
     *  For now - this is a simple wrapper on the discovered devices and networks. But soon it will add persistence, and
     *  its own IDs, separate from discovered ids (and map them and provide apis to adjust mapping). --LGP 2021-01-17
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
         */
        nonvirtual std::optional<Device> GetDevice (const Common::GUID& id) const;

    public:
        /**
         */
        nonvirtual Sequence<Network> GetNetworks () const;

    public:
        /**
         */
        nonvirtual std::optional<Network> GetNetwork (const Common::GUID& id) const;

    public:
        /**
         */
        nonvirtual Collection<NetworkInterface> GetNetworkInterfaces () const;

    public:
        /**
         */
        nonvirtual std::optional<NetworkInterface> GetNetworkInterface (const Common::GUID& id) const;
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
