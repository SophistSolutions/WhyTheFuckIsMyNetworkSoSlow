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
    using Time::DateTime;
    using Time::Duration;
    using Traversal::Range;

    /**
     */
    struct OperatingSystem {
        /**
         *  Open enum, but basic names are:
         *      o   Linux
         *      o   Windows
         *      o   MacOS
         *      o   BSD
         *      o   Unix            (for cases not BSD or Linux)
         */
        optional<String> fMajorOSCategory;

        /**
         *  For Linux, try to show distribution.
         *  e.g. "Ubuntu 18.04", "Red-Hat 9", "Windows 10 Version 1809 (OS Build 17763.349)", "Linux (unknown)"
         */
        optional<String> fFullVersionedOSName;

#if __cpp_impl_three_way_comparison < 201711
        bool operator== (const OperatingSystem& rhs) const
        {
            if (fMajorOSCategory != rhs.fMajorOSCategory) {
                return false;
            }
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

        /**
         */
        Network () = default;
        Network (const Set<CIDR>& nas);

        // @todo - WTF allocated ID - and one inherited from network interface (windows only) - CLARIFY - probably call OURs just fID (and change others in this module to match)
        GUID fGUID;

        optional<String> fFriendlyName; //tmphack - list of interfaces attached to network

        /*
         * This list of addresses will typically have one IPV4 and one IPV6 address
         */
        Set<CIDR> fNetworkAddresses;

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

        /**
         *   // NOTE - COULD USE DisjointRange<> - WOULD make more sense but maybe not worth the work
         */
        Range<DateTime> fSeen;

        /**
         * This network summary represents an aggregation of the following network objects.
         */
        optional<Set<GUID>> fAggregatesReversibly;

        /**
         *  So far NYI, but idea is we automatically rollup some old stuff and store that as a new record in the DB.
         */
        optional<Set<GUID>> fAggregatesIrreversibly;

        /**
         *  this ID is stored in the database
         */
        optional<bool> fIDPersistent;

        /**
         *  this data is a historical (database) snapshot, vs a dynamic rollup
         */
        optional<bool> fHistoricalSnapshot;

#if qDebug
        optional<Mapping<String, DataExchange::VariantValue>> fDebugProps;
#endif

        /**
         *  @see Characters::ToString ();
         */
        nonvirtual String ToString () const;

        static const DataExchange::ObjectVariantMapper kMapper;

        static Network Merge (const Network& baseNetwork, const Network& priorityNetwork);
        static Network Rollup (const Network& rollupNetwork, const Network& instanceNetwork2Add);
    };

    /**
     * Subset of (interesting) intformation about a Network (wrt its attachment to a device)
     */
    struct NetworkAttachmentInfo {
        Set<String>               hardwareAddresses;
        Sequence<InternetAddress> localAddresses; // bound addresses (this machine @ this address)

        nonvirtual String ToString () const;
    };

    /**
     */
    struct Device {

        Device ()              = default;
        Device (const Device&) = default;
        Device (Device&&)      = default;
        Device& operator= (const Device&) = default;
        Device& operator= (Device&&) = default;

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
         *  Basically this is a datetime range, but we keep a separate one for each way of being seen.
         */
        struct SeenType {
            optional<Range<DateTime>> fARP;
            optional<Range<DateTime>> fCollector;
            optional<Range<DateTime>> fICMP;
            optional<Range<DateTime>> fTCP;
            optional<Range<DateTime>> fUDP;

            /**
             * Combine (union bounds) all the ranges.
             */
            nonvirtual optional<Range<DateTime>> EverSeen () const;

            /**
             *  @see Characters::ToString ();
             */
            nonvirtual String ToString () const;

            static const DataExchange::ObjectVariantMapper kMapper;
        };

        /**
         */
        SeenType fSeen;

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

        /**
         * This device summary represents an aggregation of the following device objects.
         */
        optional<Set<GUID>> fAggregatesReversibly;

        /**
         *  So far NYI, but idea is we automatically rollup some old stuff and store that as a new record in the DB.
         */
        optional<Set<GUID>> fAggregatesIrreversibly;

        /**
         *  this ID is stored in the database
         */
        optional<bool> fIDPersistent;

        /**
         *  this data is a historical (database) snapshot, vs a dynamic rollup
         */
        optional<bool> fHistoricalSnapshot;

#if qDebug
        optional<Mapping<String, DataExchange::VariantValue>> fDebugProps;
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

        static const DataExchange::ObjectVariantMapper kMapper;

        /**
         * \brief  Merge data from two device objects, but the second one, where they have conflicts, takes precedence (that includes the ID).
         */
        static Device Merge (const Device& baseDevice, const Device& priorityDevice);

        /**
         * \brief  Combine two device objects, but track the original IDs. LHS sb the one from previous rollups.
         */
        static Device Rollup (const Device& rollupDevice, const Device& instanceDevice2Add);
    };

    /**
     */
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
                Duration fTime;    // time to that hop
                String   fAddress; // address of hop - can be InternetAddress or DNS address depending
            };
            Sequence<Hop> fHops;
        };

        struct DNSLookupResults {
            optional<String> fResult;     // just print first result - maybe missing if error occured on lookup
            Duration         fLookupTime; // often misleadingly quick due to caching
        };

        extern const DataExchange::ObjectVariantMapper kMapper;
    }

    struct About {
        Configuration::Version fOverallApplicationVersion;

        struct APIServerInfo {

            Configuration::Version fVersion;
            struct ComponentInfo {
                String        fName;
                String        fVersion;
                optional<URI> fURL;

                nonvirtual String ToString () const;
            };
            Sequence<ComponentInfo> fComponentVersions;

            struct CurrentMachine {
                OperatingSystem    fOperatingSystem;
                optional<Duration> fMachineUptime;
                optional<double>   fTotalCPUUsage{};
                optional<double>   fRunQLength{};

                nonvirtual String ToString () const;
            };
            CurrentMachine fCurrentMachine;

            struct CurrentProcess {
                optional<Duration> fProcessUptime;
                optional<double>   fAverageCPUTimeUsed;
                optional<uint64_t> fWorkingOrResidentSetSize;
                optional<double>   fCombinedIOReadRate;
                optional<double>   fCombinedIOWriteRate;

                nonvirtual String ToString () const;
            };
            CurrentProcess fCurrentProcess;

            /**
             * WSAPI related stats - for now - averaged over the last 5 minutes.
             */
            struct APIEndpoint {
                unsigned int       fCallsCompleted{};
                optional<Duration> fMeanDuration;
                optional<Duration> fMedianDuration;
                optional<Duration> fMaxDuration;
                unsigned int       fErrors{};
                optional<float>    fMedianWebServerConnections;
                optional<float>    fMedianProcessingWebServerConnections;
                optional<float>    fMedianRunningAPITasks;

                nonvirtual String ToString () const;
            };
            optional<APIEndpoint> fAPIEndpoint;

            /**
             * Database related stats - for now - averaged over the last 5 minutes.
             */
            struct Database {
                unsigned int        fReads{};
                unsigned int        fWrites{};
                unsigned int        fErrors{};
                optional<Duration>  fMeanReadDuration;
                optional<Duration>  fMedianReadDuration;
                optional<Duration>  fMeanWriteDuration;
                optional<Duration>  fMedianWriteDuration;
                optional<Duration>  fMaxDuration;
                optional<uintmax_t> fFileSize;

                nonvirtual String ToString () const;
            };
            optional<Database> fDatabase;

            nonvirtual String ToString () const;
        };
        APIServerInfo fAPIServerInfo;

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
