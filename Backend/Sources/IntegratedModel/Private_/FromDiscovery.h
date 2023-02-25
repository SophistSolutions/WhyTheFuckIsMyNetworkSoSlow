/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Mgr_Private_Discovery_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Mgr_Private_Discovery_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include <optional>

#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/Containers/Sequence.h"

#include "../../WebServices/Model.h"

/**
 *  Wrappers on the discovery manager APIs, that just fetch the discovered devices and convert to common
 *  integrated model (no datebase awareness)
 * 
 *  Note - IDs are simple to manage/maintain - they come from the discovery layer, and we SIMPLY RE-USE those IDs in this DiscoveryWrapper
 *  layer. That makes all ID/Pointers consistent.
 */
namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::IntegratedModel::Private_::FromDiscovery {

    using namespace std;

    using WebServices::Model::Device;
    using WebServices::Model::Network;
    using WebServices::Model::NetworkAttachmentInfo;
    using WebServices::Model::NetworkInterface;

    using Stroika::Foundation::Common::GUID;
    using Stroika::Foundation::Containers::Collection;
    using Stroika::Foundation::Containers::Sequence;
    using Stroika::Foundation::Time::Duration;

    /**
     */
    optional<GUID> GetMyDeviceID ();

    /**
     * Map all the 'Discovery::NetworkInterface' objects to 'Model::NetworkInterface' objects.
     *
     *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
     */
    Sequence<NetworkInterface> GetNetworkInterfaces ();

    /**
     * Map all the 'Discovery::Network' objects to 'Model::Network' objects.
     *
     *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
     */
    Sequence<Network> GetNetworks ();

    /**
     * Map all the 'Discovery::Device' objects to 'Model::Device' objects.
     *
     *  \note   \em Thread-Safety   <a href="Thread-Safety.md#Internally-Synchronized-Thread-Safety">Internally-Synchronized-Thread-Safety</a>
     */
    Sequence<Device> GetDevices ();

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "FromDiscovery.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Mgr_Private_Discovery_h_*/
