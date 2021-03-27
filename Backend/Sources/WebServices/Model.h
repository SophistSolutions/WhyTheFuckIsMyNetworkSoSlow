/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_Model_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_Model_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#if defined(__cpp_impl_three_way_comparison)
#include <compare>
#endif

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Configuration/Version.h"
#include "Stroika/Foundation/Containers/Mapping.h"
#include "Stroika/Foundation/Containers/Sequence.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/IO/Network/CIDR.h"
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/InternetAddress.h"
#include "Stroika/Foundation/IO/Network/URI.h"
#include "Stroika/Foundation/Time/DateTime.h"
#include "Stroika/Foundation/Time/Duration.h"

#include "../Common/GeoLocation.h"
#include "../Common/InternetServiceProvider.h"

/**
 *
 */
namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model {

    using namespace Stroika::Foundation;
    using Characters::String;
    using Containers::Mapping;
    using Containers::Sequence;
    using Containers::Set;
    using IO::Network::CIDR;
    using IO::Network::InternetAddress;
    using IO::Network::URI;
    using Stroika::Foundation::Common::GUID;

    /**
     */
    struct OperatingSystem {
        /**
         *  For Linux, try to show distribution.
         *  e.g. "Ubuntu 18.04", "Red-Hat 9", "Windows 10 Version 1809 (OS Build 17763.349)", "Linux (unknown)"
         */
        optional<String> fFullVersionedOSName;

#if __cpp_impl_three_way_comparison < 201711
        bool operator== (const OperatingSystem& rhs) const
        {
            if (fFullVersionedOSName != rhs.fFullVersionedOSName) {
                return false;
            }
            return true;
        }
        bool operator!= (const OperatingSystem& rhs) const
        {
            return not(*this == rhs);
        }
#else
        auto operator<=> (const OperatingSystem&) const = default;
#endif

        /**
         *  @see Characters::ToString ();
         */
        nonvirtual String ToString () const;

        static const DataExchange::ObjectVariantMapper kMapper;
    };

    struct Manufacturer {
        optional<String> fShortName;
        optional<String> fFullName;
        optional<URI>    fWebSiteURL;

        bool Contains (const String& name)
        {
            if (fShortName and fShortName->Contains (name, Characters::CompareOptions::eCaseInsensitive)) {
                return true;
            }
            if (fFullName and fFullName->Contains (name, Characters::CompareOptions::eCaseInsensitive)) {
                return true;
            }
            return false;
        }

#if __cpp_impl_three_way_comparison < 201711
        bool operator== (const Manufacturer& rhs) const
        {
            if (fShortName != rhs.fShortName) {
                return false;
            }
            if (fFullName != rhs.fFullName) {
                return false;
            }
            if (fWebSiteURL != rhs.fWebSiteURL) {
                return false;
            }
            return true;
        }
        bool operator!= (const Manufacturer& rhs) const
        {
            return not(*this == rhs);
        }
#else
        auto operator<=> (const Manufacturer&) const = default;
#endif

        /**
         *  @see Characters::ToString ();
         */
        nonvirtual String ToString () const;

        static const DataExchange::ObjectVariantMapper kMapper;
    };

    /**
     *  This roughly mimics the Stroika class IO::Network::Interface, or what you would see 
     *  returned on windows ipconfig/all, or unix "ip link show"
     */
    struct NetworkInterface : IO::Network::Interface {
        NetworkInterface ()                            = default;
        NetworkInterface (const NetworkInterface& src) = default;
        NetworkInterface (const IO::Network::Interface& src);

        /**
         *  GUID for this interface - MANUFACTURED by WTF
         // @todo rename fID, and actually auto-generate it uniquely somehow (or OK to use windows based one)
         */
        GUID fGUID;

#if qDebug
        optional<Mapping<String, DataExchange::VariantValue>> fDebugProps;
#endif

        /**
         *  @see Characters::ToString ();
         */
        nonvirtual String ToString () const;

        static const DataExchange::ObjectVariantMapper kMapper;
    };

    /**
     */
    struct Network {

        Network () = default;
        Network (const Set<CIDR>& nas)
            : fNetworkAddresses{nas}
        {
        }

        /*
         * This list of addresses will typically have one IPV4 and one IPV6 address
         */
        Set<CIDR> fNetworkAddresses;

        optional<String> fFriendlyName; //tmphack - list of interfaces attached to network

        // Todo - WTF allocated ID - and one inherited from network interface (windows only) - CLARIFY - probably call OURs just fID (and change others in this module to match)
        GUID fGUID;

        Set<GUID> fAttachedInterfaces;

        /**
         *  Sequence<> instead of Set<> because order matters for gateways (?) - or maybe can not be more than one?
         */
        Sequence<InternetAddress> fGateways;

        /**
         *  Sequence<> instead of Set<> because order matters for DNS servers.
         */
        Sequence<InternetAddress> fDNSServers;

        // PROBABLY idnetify same network - by default - as hash of default gateway's macaddr - or at least look at that to compare...
        // thogh that doesnt quite work cuz if you cahnge router, you need o call it the same network.... It's just a big clue... Not sure how
        // best to deal with this fuzzy identity notion

        // @todo add SSIDs here - as a good hint in identenifying same network

        // whatsmyip
        optional<Set<InternetAddress>> fExternalAddresses;

        optional<Common::GEOLocationInformation> fGEOLocInformation;

        optional<Common::InternetServiceProvider> fInternetServiceProvider;

#if qDebug
        optional<Mapping<String, DataExchange::VariantValue>> fDebugProps;
#endif

        /**
         *  @see Characters::ToString ();
         */
        nonvirtual String ToString () const;

        static const DataExchange::ObjectVariantMapper kMapper;
    };

    struct NetworkAttachmentInfo {
        Set<String>               hardwareAddresses;
        Sequence<InternetAddress> localAddresses; // bound addresses (this machine @ this address)

        nonvirtual String ToString () const;
    };

    /**
     */
    struct Device {
        GUID fGUID;

        String name;

        /**
         */
        enum class DeviceType {
            eMediaPlayer,
            eNetworkAttachedStorage,
            eNetworkInfrastructure,
            ePC,
            ePhone,
            ePrinter,
            eRouter,
            eSpeaker,
            eTablet,
            eTV,
            eVirtualMachine,

            Stroika_Define_Enum_Bounds (eMediaPlayer, eVirtualMachine)
        };

        /**
         *  missing is unknown type of device
         */
        optional<Set<DeviceType>> fTypes;

        /**
         */
        optional<URI> fIcon;

        /**
         */
        optional<Time::DateTime> fLastSeenAt;

        /**
         */
        optional<Manufacturer> fManufacturer;

        /**
         */
        Mapping<GUID, NetworkAttachmentInfo> fAttachedNetworks;

        /**
         *  Ports can be open with a number of different protocols. Examples include:
         *      tcp:80,
         *      udp:3451
         *      icmp: 8     (refers to message type 8)
         * 
         *  This is a loose notion of port because ICMP doesn't truely have ports, but message types
         */
        optional<Set<String>> fOpenPorts;

        /**
         *  This comes from the SSDP presentation url. Its a URL which can be used by a UI and web-browser to
         * 'open' the device and view/control it.
         */
        optional<URI> fPresentationURL;

        /**
         *  This generally applies to 'this computer', but when we merge data across devices, we can see multiple devices with multiple interfaces.
         *  You can use this to lookup details about wireless connections etc, associated with each network interface.
         */
        optional<Set<GUID>> fAttachedNetworkInterfaces;

        optional<OperatingSystem> fOperatingSystem;

#if qDebug
        optional<Mapping<String, DataExchange::VariantValue>> fDebugProps;
#endif

        /**
         *  Combine from all network interfaces.
         */
        nonvirtual Set<InternetAddress> GetInternetAddresses () const;

        nonvirtual String ToString () const;

        static const DataExchange::ObjectVariantMapper kMapper;
    };

    struct DeviceSortParamters {

        /**
         */
        struct SearchTerm {
            /**
             */
            enum class By {
                eAddress,
                ePriority,
                eName,
                eType,

                Stroika_Define_Enum_Bounds (eAddress, eType)
            };
            By             fBy{By::eEND};
            optional<bool> fAscending;

            nonvirtual String ToString () const;

            static const DataExchange::ObjectVariantMapper kMapper;
        };
        Sequence<SearchTerm> fSearchTerms;

        // GUID or CIDR
        optional<String> fCompareNetwork;

        nonvirtual String ToString () const;

        static const DataExchange::ObjectVariantMapper kMapper;
    };

    namespace Operations {

        struct TraceRouteResults {
            struct Hop {
                Time::Duration fTime;    // time to that hop
                String         fAddress; // address of hop - can be InternetAddress or DNS address depending
            };
            Sequence<Hop> fHops;
        };

        struct DNSLookupResults {
            optional<String> fResult;     // just print first result - maybe missing if error occured on lookup
            Time::Duration   fLookupTime; // often misleadingly quick due to caching
        };

        extern const DataExchange::ObjectVariantMapper kMapper;
    }

    struct About {
        Configuration::Version fOverallApplicationVersion;
        struct ComponentInfo {
            String        fName;
            String        fVersion;
            optional<URI> fURL;

            nonvirtual String ToString () const;
        };
        Sequence<ComponentInfo> fComponentVersions;
        OperatingSystem         fOperatingSystem;

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
