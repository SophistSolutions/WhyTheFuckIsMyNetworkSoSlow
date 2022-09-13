/*
* Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
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

#include "../Common/PrioritizedName.h"

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
    using Stroika::Foundation::Time::DateTime;
    using Stroika::Foundation::Traversal::Range;

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
        Set<InternetAddress> localAddresses; // bound addresses (this machine @ this address)

#if __cpp_impl_three_way_comparison < 201711
        bool operator== (const NetworkAttachmentInfo& rhs) const
        {
            if (hardwareAddresses != rhs.hardwareAddresses) {
                return false;
            }
            if (localAddresses != rhs.localAddresses) {
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
        Common::PrioritizedNames             fNames;
        Set<DeviceType>                      fTypes;
        bool                                 fThisDevice{};
        Mapping<GUID, NetworkAttachmentInfo> fAttachedNetworks;
        optional<Set<GUID>>                  fAttachedInterfaces;
        struct SeenType {
            optional<Range<DateTime>> fARP;
            optional<Range<DateTime>> fCollector; // host device collecting data
            optional<Range<DateTime>> fICMP;
            optional<Range<DateTime>> fTCP;
            optional<Range<DateTime>> fUDP;
#if __cpp_impl_three_way_comparison < 201711
            bool operator== (const SeenType& rhs) const
            {
                if (fARP != rhs.fARP) {
                    return false;
                }
                if (fCollector != rhs.fCollector) {
                    return false;
                }
                if (fICMP != rhs.fICMP) {
                    return false;
                }
                if (fTCP != rhs.fTCP) {
                    return false;
                }
                if (fUDP != rhs.fUDP) {
                    return false;
                }
                return true;
            }
            bool operator!= (const SeenType& rhs) const
            {
                return not(*this == rhs);
            }
#else
            auto operator<=> (const SeenType&) const = default;
#endif
        };
        SeenType                  fSeen;
        optional<Set<String>>     fOpenPorts;
        optional<URI>             fIcon;
        optional<Manufacturer>    fManufacturer;
        optional<URI>             fPresentationURL;
        optional<OperatingSystem> fOperatingSystem;
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

        /**
         *  Get PREFERERRED (typically one) IP Address; typically this is the LOCAL IP addr on the main real network. None of these things are objective, and could change over time.
         *  Typically this will return 1 IPv4 Address
         */
        nonvirtual Sequence<InternetAddress> GetPreferredDisplayInternetAddresses () const;

        nonvirtual String ToString () const;

#if __cpp_impl_three_way_comparison < 201711
        bool operator== (const Device& rhs) const
        {
            if (fGUID != rhs.fGUID) {
                return false;
            }
            if (fNames != rhs.fNames) {
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
            if (fSeen != rhs.fSeen) {
                return false;
            }
            if (fOpenPorts != rhs.fOpenPorts) {
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
        auto operator<=> (const Device&) const = default;
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
        /**
         *  Background scanning - initaiate rescan (later parameterize) @todo parameterize - scan known, scan unknown, scan particular network (CIDR);
         *  Syncrhonous scan - return when complete.
         */
        nonvirtual void ReScan (const GUID& deviceID);

    public:
        /**
         *  Synchronously Scan the given address, and report a human readable report of the results.
         */
        nonvirtual DataExchange::VariantValue ScanAndReturnReport (const InternetAddress& addr);

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
