/*
* Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
*/
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_Devices_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_Devices_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#if defined(__cpp_impl_three_way_comparison)
#include <compare>
#endif

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Common/GUID.h"
#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/Containers/Mapping.h"
#include "Stroika/Foundation/IO/Network/InternetAddress.h"
#include "Stroika/Foundation/IO/Network/URI.h"

#include "../WebServices/Model.h"

#include "Networks.h"

/**
 *  \note Design Note
 *          Planned approach for Discovery
 *              o   UPnP/SSDP
 *              o   Ping
 *              o   Self
 *              o   ARP (?)
 *              o   Promiscuous mode on ethernet, and peek at packets
 *              o   Bonjour?
 *
 *          Planned approach to identify devices *get names etc)
 *              o   UPnP/SSDP
 *              o   Bonjour?
 *              o   TCP Window reverse-engineer
 *              o   open ports
 *              o   curl port 80 server header (and similar tricks for other protocols)
 *              o   Look at what is done by nmap
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Discovery {

    using Characters::String;
    using Containers::Collection;
    using Containers::Mapping;
    using Containers::Set;
    using IO::Network::InternetAddress;
    using IO::Network::URI;
    using Stroika::Foundation::Common::GUID;

    /**
     */
    using DeviceType = WebServices::Model::Device::DeviceType;

    /**
     */
    using OperatingSystem = WebServices::Model::OperatingSystem;

    /**
     */
    using Manufacturer = WebServices::Model::Manufacturer;

    /**
     */
    struct NetworkAttachmentInfo {
        Set<String>          hardwareAddresses;
        Set<InternetAddress> networkAddresses; // node not socket addresses

#if __cpp_impl_three_way_comparison < 201711
        bool operator== (const NetworkAttachmentInfo& rhs) const
        {
            if (hardwareAddresses != rhs.hardwareAddresses) {
                return false;
            }
            if (networkAddresses != rhs.networkAddresses) {
                return false;
            }
            return true;
        }
        bool operator!= (const NetworkAttachmentInfo& rhs) const
        {
            return not(*this == rhs);
        }
#else
        auto operator<=> (const NetworkAttachmentInfo&) const = default;
#endif

        nonvirtual String ToString () const;
    };

    /**
     *  Discovery::Device is the definition of a device in the discovery module. This is the level of detail
     *  captured by the discovery services.
     */
    struct Device {
        GUID                                 fGUID;
        String                               name;
        Set<DeviceType>                      fTypes;
        bool                                 fThisDevice{};
        Mapping<GUID, NetworkAttachmentInfo> fAttachedNetworks;
        optional<Set<GUID>>                  fAttachedInterfaces;
        optional<URI>                        fIcon;
        optional<Manufacturer>               fManufacturer;
        optional<URI>                        fPresentationURL;
        optional<OperatingSystem>            fOperatingSystem;
#if qDebug
        Mapping<String, DataExchange::VariantValue> fDebugProps;
#endif

        /**
         *  Combine from all network interfaces.
         */
        nonvirtual Set<String> GetHardwareAddresses () const;

        /**
         *  Combine from all network interfaces.
         */
        nonvirtual Set<InternetAddress> GetInternetAddresses () const;

        nonvirtual String ToString () const;

#if __cpp_impl_three_way_comparison < 201711
        bool operator== (const Device& rhs) const
        {
            if (fGUID != rhs.fGUID) {
                return false;
            }
            if (name != rhs.name) {
                return false;
            }
            if (fTypes != rhs.fTypes) {
                return false;
            }
            if (fThisDevice != rhs.fThisDevice) {
                return false;
            }
            if (fAttachedNetworks != rhs.fAttachedNetworks) {
                return false;
            }
            if (fAttachedInterfaces != rhs.fAttachedInterfaces) {
                return false;
            }
            if (fIcon != rhs.fIcon) {
                return false;
            }
            if (fManufacturer != rhs.fManufacturer) {
                return false;
            }
            if (fPresentationURL != rhs.fPresentationURL) {
                return false;
            }
            if (fOperatingSystem != rhs.fOperatingSystem) {
                return false;
            }
#if qDebug
            if (fDebugProps != rhs.fDebugProps) {
                return false;
            }
#endif
            return true;
        }
        bool operator!= (const Device& rhs) const
        {
            return not(*this == rhs);
        }
#else
        auto operator<=> (const Device&) const                = default;
#endif
    };

    /**
     *  Public APIs fully <a href='ThirdPartyComponents/Stroika/StroikaRoot/Documentation/Thread-Safety.md#Internally-Synchronized-Thread-Safety'>Internally-Synchronized-Thread-Safety</a>
     *
     *  Only legal to call DevicesMgr while its active (checked with assertions). Callers responsability to assure, but checked with assertions.
     */
    class DevicesMgr {
    public:
        DevicesMgr ()                  = default;
        DevicesMgr (const DevicesMgr&) = delete;

    public:
        /**
         *  At most one such object may exist. When it does, the NetworksMgr is active and usable. Its illegal to call otherwise.
         */
        struct Activator {
            Activator ();
            ~Activator ();
        };

    public:
        nonvirtual Collection<Device> GetActiveDevices (optional<Time::DurationSecondsType> allowedStaleness = {}) const;

    public:
        static DevicesMgr sThe;
    };

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "Devices.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_Devices_h_*/
