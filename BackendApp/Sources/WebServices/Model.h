/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_Model_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_Model_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Containers/Sequence.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/IO/Network/CIDR.h"
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/InternetAddress.h"

/**
 *
 */
namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model {

    using namespace Stroika::Foundation;
    using Characters::String;
    using Containers::Sequence;
    using Containers::Set;
    using IO::Network::CIDR;
    using IO::Network::InternetAddress;

    /**
     *  This roughly mimics the Stroika class IO::Network::Interface, or what you would see 
     *  returned on windows ipconfig/all, or unix "ip link show"
     * 
     */
    struct NetworkInterface : IO::Network::Interface {
        NetworkInterface ()                            = default;
        NetworkInterface (const NetworkInterface& src) = default;
        NetworkInterface (const IO::Network::Interface& src);

        /**
         *  GUID for this interface - MANUFACTURED by WTF
         // @todo rename fID, and actually auto-generate it uniquly somehow (or OK to use iwndows based one)
         */
        String fGUID;

        /**
         *  @see Characters::ToString ();
         */
        nonvirtual String ToString () const;

        static const DataExchange::ObjectVariantMapper kMapper;
    };

    /**
     */
    struct Network {

        Network ()
            : Network (CIDR{IO::Network::V4::kAddrAny, 32})

        {
        }
        Network (CIDR na)
            : fNetworkAddress (na)
        {
        }

        CIDR fNetworkAddress;

        optional<String> fFriendlyName; //tmphack - list of interfaces attached to network

        // Todo - WTF allocated ID - and one inherited from network interface (windows only) - CLARIFY - probably call OURs just fID (and change others in this module to match)
        Common::GUID fGUID;

        Set<String> fAttachedInterfaces;

        Sequence<InternetAddress> fGateways;
        Sequence<InternetAddress> fDNSServers;

        // PROBABLY idnetify same network - by default - as hash of default gateway's macaddr - or at least look at that to compare...
        // thogh that doesnt quite work cuz if you cahnge router, you need o call it the same network.... It's just a big clue... Not sure how
        // best to deal with this fuzzy identity notion

        // @todo add SSIDs here - as a good hint in identenifying same network

        /**
         *  @see Characters::ToString ();
         */
        nonvirtual String ToString () const;

        static const DataExchange::ObjectVariantMapper kMapper;
    };

    /**
     */
    struct Device {
        /**
        @ tood rename as "fID" and just document perisstnet id
         */
        String persistentID;

        String name;

        /**
         * Bindings

         redo this as Collection<InternetAddress>
         */
        Sequence<String> ipAddresses;

        /**
         */
        enum class DeviceType {
            eLaptop,
            eDesktop,
            eTablet,
            ePhone,
            eRouter,
            ePrinter,
            eInfrastructureDevice,

            Stroika_Define_Enum_Bounds (eLaptop, eInfrastructureDevice)
        };

        /**
         *  missing is unknown type of device
         */
        optional<DeviceType> type;

        /*
         *  in CIDR notation.
         */
        String network;

        optional<float> signalStrength{};
        bool            connected{};
        bool            important{};

        nonvirtual String ToString () const;

        static const DataExchange::ObjectVariantMapper kMapper;
    };
}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "Model.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_Model_h_*/
