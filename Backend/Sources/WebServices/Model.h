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
#include "../Common/PrioritizedName.h"

/**
 *
 */
namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model {

    using namespace Stroika::Foundation;
    using Characters::String;
    using Containers::Mapping;
    using Containers::Sequence;
    using Containers::Set;
    using Containers::SortedCollection;
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
        auto operator<=> (const Manufacturer&) const    = default;
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
    struct NetworkInterface {
        NetworkInterface ()                            = default;
        NetworkInterface (const NetworkInterface& src) = default;

        using SystemIDType = IO::Network::Interface::SystemIDType;

        /**
         *  \brief platformInterfaceID
         * @see IO::Network::Interface::fInternalInterfaceID
         */
        SystemIDType fInternalInterfaceID;

        /**
        * @see IO::Network::Interface::fFriendlyName
         */
        String fFriendlyName;

        /**
        * @see IO::Network::Interface::fDescription
         */
        optional<String> fDescription;

        /**
        * @see IO::Network::Interface::Type
         */
        using Type = IO::Network::Interface::Type;

        /**
        * @see IO::Network::Interface::fType
         */
        optional<Type> fType;

        /**
        * @see IO::Network::Interface::fHardwareAddress
         */
        optional<String> fHardwareAddress;

        /**
        * @see IO::Network::Interface::fTransmitSpeedBaud
         */
        optional<uint64_t> fTransmitSpeedBaud;

        /**
        * @see IO::Network::Interface::fReceiveLinkSpeedBaud
         */
        optional<uint64_t> fReceiveLinkSpeedBaud;

        /**
         */
        using WirelessInfo = IO::Network::Interface::WirelessInfo;

        /**
        * @see IO::Network::Interface::fWirelessInfo
         */
        optional<WirelessInfo> fWirelessInfo;

        /**
         */
        using Bindings = IO::Network::Interface::Bindings;

        /**
        * @see IO::Network::Interface::fBindings
         */
        Bindings fBindings;

        /**
        * @see IO::Network::Interface::fGateways
         */
        optional<Containers::Sequence<InternetAddress>> fGateways;

        /**
        * @see IO::Network::Interface::fDNSServers
         */
        optional<Containers::Sequence<InternetAddress>> fDNSServers;

        /**
        * @see IO::Network::Interface::Status
         */
        using Status = IO::Network::Interface::Status;

        /**
        * @see IO::Network::Interface::fStatus
         */
        optional<Containers::Set<Status>> fStatus;

        /**
         *  GUID for this interface - MANUFACTURED by WTF
         // @todo rename fID, and actually auto-generate it uniquely somehow (or OK to use windows based one)
         */
        GUID fGUID;

        /**
         *  In WSAPI, we show this record/value in objects which are rolled up into other objects.
         * 
         *  \note - this is not set by the Rollup/Merge methods, and not stored in the database, but dynamically added
         *          by the IntegrationModel::Mgr
         */
        optional<GUID> fAggregatedBy;

        /**
         *  This information is not stored, or settable, but some WSAPI interfaces may return this information.
         *  If provided, this set will typically have a single member.
         */
        optional<Set<GUID>> fAttachedToDevices;

        /**
         * This NetworkInterface summary represents an aggregation of the following NetworkInterface objects.
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

        using FingerprintType = GUID;

        /**
         *  This returns a property that can be used for object identity - if two networkinterfaces have the same fingerprint
         *  they can usefully be throught of as the same (MAYBE need to augment this with ID of device they come from?).
         * 
         *  Features that get copied up/preserved in rollup:
         *      o   fDescription
         *      o   fFriendlyName
         *      o   fHardwareAddress
         *      o   fInternalInterfaceID
         *      o   fType
         */
        nonvirtual FingerprintType GenerateFingerprintFromProperties () const;

#if qDebug
        optional<Mapping<String, DataExchange::VariantValue>> fDebugProps;
#endif

        /**
         *  @see Characters::ToString ();
         */
        nonvirtual String ToString () const;

        static const DataExchange::ObjectVariantMapper kMapper;

        /**
         *  This code ensures that the resulting rollup has same GenerateFingerprintFromProperties () as instanceNetwork2Add.GenerateFingerprintFromProperties ()
         * 
         *  Features that get copied up/preserved in rollup:
         *      o   fDescription
         *      o   fFriendlyName
         *      o   fHardwareAddress
         *      o   fInternalInterfaceID
         *      o   fType
         *      Ensure (r.GenerateFingerprintFromProperties () == instanceNetwork2Add.GenerateFingerprintFromProperties ());
         */
        static NetworkInterface Rollup (const optional<NetworkInterface>& previousRollupNetworkInterface, const NetworkInterface& instanceNetwork2Add);
    };

    /**
     */
    struct Network {

        /**
         */
        Network ()                   = default;
        Network (Network&& src)      = default;
        Network (const Network& src) = default;
        explicit Network (const Set<CIDR>& nas);

        nonvirtual Network& operator= (Network&& rhs)      = default;
        nonvirtual Network& operator= (const Network& rhs) = default;

        // @todo - WTF allocated ID - and one inherited from network interface (windows only) - CLARIFY - probably call OURs just fID (and change others in this module to match)
        GUID fGUID;

        /**
         *  In WSAPI, we show this record/value in objects which are rolled up into other objects.
         * 
         *  \note - this is not set by the Rollup/Merge methods, and not stored in the database, but dynamically added
         *          by the IntegrationModel::Mgr
         */
        optional<GUID> fAggregatedBy;

        /**
         */
        Common::PrioritizedNames fNames;

        /*
         * This list of addresses will typically have one IPV4 and one IPV6 address
         */
        Set<CIDR> fNetworkAddresses;

        /**
         */
        Set<GUID> fAttachedInterfaces;

        /**
         *  Once used Sequence<> instead of Set<>, because for some purposes, order matters. But for the most part, not
         *  how we treat gateways, so ignore order, and use Set<>
         */
        Set<InternetAddress> fGateways;

        /**
         *  hardware addresses of all gateways on this network. This together with the gateway addresses, can be used
         *  to fingerprint a network.
         */
        Set<String> fGatewayHardwareAddresses;

        /**
         *  Could be Sequence<> instead of Set<> because order matters for DNS servers. But use set cuz we don't lookup
         *  and we could easily combine from different sources. Set good enuf for now.
         */
        Set<InternetAddress> fDNSServers;

        // PROBABLY identify same network - by default - as hash of default gateway's macaddr - or at least look at that to compare...
        // though that doesn't quite work cuz if you change router, you need o call it the same network.... It's just a big clue... Not sure how
        // best to deal with this fuzzy identity notion

        // @todo add SSIDs here - as a good hint in identenifying same network

        // whatsmyip
        optional<Set<InternetAddress>> fExternalAddresses;

        /**
         */
        optional<Common::GEOLocationInformation> fGEOLocInformation;

        /**
         */
        optional<Common::InternetServiceProvider> fInternetServiceProvider;

        /**
         *   NOTE - COULD USE DisjointRange<> - WOULD make more sense but maybe not worth the work
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

        using FingerprintType = GUID;

        /**
         *  If present, this is the union of all fingerprints combined into this network. This is dynamically determined
         *  (a computed property) of the given network. That means if a network (dynamic and still running) changes any properties
         *  its GenerateFingerprintFromProperties () may change.
         */
        optional<Set<FingerprintType>> fAggregatesFingerprints;

        /**
         *  this ID is stored in the database
         */
        optional<bool> fIDPersistent;

        /**
         * per network settings specified externally (typically GUI user edit)
         */
        struct UserOverridesType {
            /**
             */
            optional<String> fName;

            /**
             */
            optional<Set<String>> fTags;

            /**
             * @todo consider but probably interpret the text as markdown.
             */
            optional<String> fNotes;

            // AddFingerprint/RemoveFingerprint/AddID/RemoveID optional<set<guid>>> here so user can customize what gets rolled into this net.

            // LOSE NEGATIVE RULES - DONT THIS OR THAT - ONLY INCLUDE POSITIVE RULES CUZ THEN CLEAR HOW TO RESOLVE CONFLICTS

            optional<Set<GUID>>            fAggregateNetworks;
            optional<Set<FingerprintType>> fAggregateFingerprints;
            optional<Set<String>>          fAggregateGatewayHardwareAddresses;

            struct NetworkInterfaceAggregateRule {
                IO::Network::Interface::Type fInterfaceType;
                FingerprintType              fFingerprint;

                /**
                 *  @see Characters::ToString ();
                 */
                nonvirtual String ToString () const;

#if __cpp_impl_three_way_comparison < 201711
                bool operator== (const NetworkInterfaceAggregateRule& rhs) const
                {
                    if (fInterfaceType != rhs.fInterfaceType) {
                        return false;
                    }
                    if (fFingerprint != rhs.fFingerprint) {
                        return false;
                    }
                    return true;
                }
                bool operator!= (const NetworkInterfaceAggregateRule& rhs) const
                {
                    return not(*this == rhs);
                }
#else
                auto operator<=> (const NetworkInterfaceAggregateRule&) const = default;
#endif
            };
            optional<Sequence<NetworkInterfaceAggregateRule>> fAggregateNetworkInterfacesMatching;

#if __cpp_impl_three_way_comparison < 201711
            bool operator== (const UserOverridesType& rhs) const
            {
                if (fName != rhs.fName) {
                    return false;
                }
                if (fTags != rhs.fTags) {
                    return false;
                }
                if (fNotes != rhs.fNotes) {
                    return false;
                }
                if (fAggregateNetworks != rhs.fAggregateNetworks) {
                    return false;
                }
                if (fAggregateFingerprints != rhs.fAggregateFingerprints) {
                    return false;
                }
                if (fAggregateGatewayHardwareAddresses != rhs.fAggregateGatewayHardwareAddresses) {
                    return false;
                }
                if (fAggregateNetworkInterfacesMatching != rhs.fAggregateNetworkInterfacesMatching) {
                    return false;
                }
                return true;
            }
            bool operator!= (const UserOverridesType& rhs) const
            {
                return not(*this == rhs);
            }
#else
            auto operator<=> (const UserOverridesType&) const = default;
#endif

            nonvirtual bool IsNonTrivial () const;

            /**
             *  @see Characters::ToString ();
             */
            nonvirtual String ToString () const;

            static const DataExchange::ObjectVariantMapper kMapper;
        };

        /**
         */
        optional<UserOverridesType> fUserOverrides;

#if qDebug
        optional<Mapping<String, DataExchange::VariantValue>> fDebugProps;
#endif

        /**
         */
        nonvirtual FingerprintType GenerateFingerprintFromProperties () const;

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

        Device ()                         = default;
        Device (const Device&)            = default;
        Device (Device&&)                 = default;
        Device& operator= (const Device&) = default;
        Device& operator= (Device&&)      = default;

        GUID fGUID;

        /**
         *  In WSAPI, we show this record/value in objects which are rolled up into other objects.
         * 
         *  \note - this is not set by the Rollup/Merge methods, and not stored in the database, but dynamically added
         *          by the IntegrationModel::Mgr
         */
        optional<GUID> fAggregatedBy;

        Common::PrioritizedNames fNames;

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
            eWTFCollector,

            Stroika_Define_Enum_Bounds (eMediaPlayer, eWTFCollector)
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
         * per device settings specified externally (typically GUI user edit)
         */
        struct UserOverridesType {
            /**
             */
            optional<String> fName;

            /**
             */
            optional<Set<String>> fTags;

            /**
             * @todo consider but probably interpret the text as markdown.
             */
            optional<String> fNotes;

            // /AddID/RemoveID optional<set<guid>>> here so user can customize what gets rolled into this device.
            optional<Set<GUID>> fAggregateDevices;

            // Automatically merge into this device anything with the given device hardware address
            optional<Set<String>> fAggregateDeviceHardwareAddresses;
#if __cpp_impl_three_way_comparison < 201711
            bool operator== (const UserOverridesType& rhs) const
            {
                if (fName != rhs.fName) {
                    return false;
                }
                if (fTags != rhs.fTags) {
                    return false;
                }
                if (fNotes != rhs.fNotes) {
                    return false;
                }
                if (fAggregateDevices != rhs.fAggregateDevices) {
                    return false;
                }
                if (fAggregateDeviceHardwareAddresses != rhs.fAggregateDeviceHardwareAddresses) {
                    return false;
                }
                return true;
            }
            bool operator!= (const UserOverridesType& rhs) const
            {
                return not(*this == rhs);
            }
#else
            auto operator<=> (const UserOverridesType&) const = default;
#endif

            nonvirtual bool IsNonTrivial () const;

            /**
             *  @see Characters::ToString ();
             */
            nonvirtual String ToString () const;

            static const DataExchange::ObjectVariantMapper kMapper;
        };

        /**
         */
        optional<UserOverridesType> fUserOverrides;

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
