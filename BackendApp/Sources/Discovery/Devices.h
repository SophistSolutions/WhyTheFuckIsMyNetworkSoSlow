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

#include "Networks.h"

/**
 *
 */

namespace WhyTheFuckIsMyNetworkSoSlow {
    namespace BackendApp {
        namespace Discovery {

            using Characters::String;
            using Containers::Collection;
            using IO::Network::InternetAddress;
            using Memory::Optional;

            struct Device {
                String          name;
                InternetAddress ipAddress;
                String          type;

                nonvirtual String ToString () const;
            };

            /*
             *  DeviceDiscoverer is internally syncronized - so its methods can be called from any thread.
             */
            class DeviceDiscoverer {
            public:
                DeviceDiscoverer ()                        = delete;
                DeviceDiscoverer (const DeviceDiscoverer&) = delete;
                DeviceDiscoverer (const Network& forNetwork);
                DeviceDiscoverer& operator= (const DeviceDiscoverer&) = delete;
                ~DeviceDiscoverer ();

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
