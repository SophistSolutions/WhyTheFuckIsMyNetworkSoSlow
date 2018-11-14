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
    using IO::Network::CIDR;

    /**
     *  This roughly mimics the Stroika class IO::Network::Interface, or what you would see 
     *  returned on windows ipconfig/all, or unix "ip link show"
     * 
     */
    struct NetworkInterface {

        /**
         *  GUID for this interface - MANUFACTURED by WTF
         // @todo rename fID, and actually auto-generate it uniquly somehow (or OK to use iwndows based one)
         */
        String fGUID;

        /**
         *  @see IO::Network::Interface
         */
        String fInternalInterfaceID;

        /**
         *  @see IO::Network::Interface
         */
        String fFriendlyName;

        /**
         *  @see IO::Network::Interface
         */
        optional<String> fDescription;

        /**
         *  @see IO::Network::Interface
         */
        optional<Common::GUID> fNetworkGUID;

        using Type = IO::Network::Interface::Type;

        /**
         *  @see IO::Network::Interface
         */
        optional<Type> fType;

        /**
         *  @see IO::Network::Interface
         */
        optional<String> fHardwareAddress;

        /**
         *  @see IO::Network::Interface
         */
        optional<uint64_t> fTransmitSpeedBaud;

        /**
         *  @see IO::Network::Interface
         */
        optional<uint64_t> fReceiveLinkSpeedBaud;

        using Binding = IO::Network::Interface::Binding;

        /**
         *  @see IO::Network::Interface
         */
        Containers::Collection<Binding> fBindings; // can be IPv4 or IPv6

        using Status = IO::Network::Interface::Status;

        /**
         *  @see IO::Network::Interface
         */
        optional<Containers::Set<Status>> fStatus;

        // @todo add optional SSID

        /**
         *  @see Characters::ToString ();
         */
        nonvirtual String ToString () const;
    };

    // early draft - BASED ON IO::Network::Interface
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

        String fFriendlyName; //tmphack - list of interfaces attached to network

        // Todo - WTF allocated ID - and one inherited from network interface (windows only) - CLARIFY - probably call OURs just fID (and change others in this module to match)
        optional<String> fNetworkGUID;

        // @todo add default-dns-servers and default-gateway

        // PROBABLY idnetify same network - by default - as hash of default gateway's macaddr - or at least look at that to compare...
        // thogh that doesnt quite work cuz if you cahnge router, you need o call it the same network.... It's just a big clue... Not sure how
        // best to deal with this fuzzy identity notion

        // @todo add SSIDs here

#if 0
        using Status = IO::Network::Interface::Status;

        /**
          @todo NOT sure we want status for network - at least different sort of status 
         */
        optional<Containers::Set<Status>> fStatus;
#endif
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
