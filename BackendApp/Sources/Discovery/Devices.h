/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_Devices_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_Devices_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/IO/Network/InternetAddress.h"
#include "Stroika/Foundation/Memory/Optional.h"

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
 */

namespace WhyTheFuckIsMyNetworkSoSlow {
    namespace BackendApp {
        namespace Discovery {

            using Characters::String;
            using Containers::Collection;
            using Containers::Set;
            using IO::Network::InternetAddress;
            using Memory::Optional;

            /**
             */
            using DeviceType = WebServices::Model::Device::DeviceType;

            /**
             *  Discovery::Device is the definition of a device in the discovery module. This is the level of detail
             *  captured by the discovery services.
             */
            struct Device {
                String               name;
                Set<InternetAddress> ipAddresses;
                Optional<DeviceType> type;
                bool                 fThisDevice{};

                nonvirtual String ToString () const;
            };

            /**
             *  DeviceDiscoverer is internally syncronized - so its methods can be called from any thread.
             */
            class DeviceDiscoverer {
            public:
                DeviceDiscoverer ()                        = delete;
                DeviceDiscoverer (const DeviceDiscoverer&) = delete;
                DeviceDiscoverer (const Network& forNetwork);
                DeviceDiscoverer& operator= (const DeviceDiscoverer&) = delete;
                ~DeviceDiscoverer () = default;

            public:
                nonvirtual Collection<Device> GetActiveDevices () const;

            private:
                class Rep_;
                unique_ptr<Rep_> fRep_;
            };
        }
    }
}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "Devices.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_Devices_h_*/
