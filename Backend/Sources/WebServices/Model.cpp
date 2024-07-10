/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Cryptography/Digest/Algorithm/MD5.h"
#include "Stroika/Foundation/Cryptography/Digest/Digester.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Writer.h"
#include "Stroika/Foundation/IO/Network/CIDR.h"
#include "Stroika/Foundation/IO/Network/InternetAddress.h"
#include "Stroika/Foundation/Memory/Optional.h"

#include "Stroika/Frameworks/UPnP/SSDP/Advertisement.h"

#include "Model.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::DataExchange;
using namespace Stroika::Foundation::IO::Network;

using Memory::NullCoalesce;
using Stroika::Foundation::Common::GUID;
using Traversal::Range;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model;

namespace {
    // lower-camel-case names happier in javascript?
    const ObjectVariantMapper::TypeMappingDetails kDateRangeMapper_ =
        ObjectVariantMapper::MakeCommonSerializer<Range<DateTime>> (ObjectVariantMapper::RangeSerializerOptions{L"lowerBound"sv, L"upperBound"sv});

    const ObjectVariantMapper::TypeMappingDetails kOptionalDateRangeMapper_ =
        ObjectVariantMapper::MakeCommonSerializer<optional<Range<DateTime>>> (ObjectVariantMapper::OptionalSerializerOptions{kDateRangeMapper_});
}

namespace {
    constexpr bool kIncludeFingerprintsInOutputTMP2Test_ = false; // to debug fingerprint code - dont really emit this
}

namespace Stroika::Foundation::DataExchange {
    template <>
    CIDR ObjectVariantMapper::ToObject (const ToObjectMapperType<CIDR>& toObjectMapper, const VariantValue& v) const
    {
        CIDR tmp{InternetAddress{}, 0};
        ToObject (toObjectMapper, v, &tmp);
        return tmp;
    }
}

namespace {
    void MergeSeen_ (Range<DateTime>* target2Update, const Range<DateTime>& timeToInclude)
    {
        RequireNotNull (target2Update);
        *target2Update = target2Update->UnionBounds (timeToInclude);
    }
    void MergeSeen_ (Device::SeenType* lhs, const Device::SeenType& rhs)
    {
        RequireNotNull (lhs);
        // @todo consider if this should be a disjoint union and use Range::Union.... - more logically correct, but perhaps less useful
        if (rhs.fARP) {
            lhs->fARP = NullCoalesce (lhs->fARP).UnionBounds (*rhs.fARP);
        }
        if (rhs.fCollector) {
            lhs->fCollector = NullCoalesce (lhs->fCollector).UnionBounds (*rhs.fCollector);
        }
        if (rhs.fICMP) {
            lhs->fICMP = NullCoalesce (lhs->fICMP).UnionBounds (*rhs.fICMP);
        }
        if (rhs.fTCP) {
            lhs->fTCP = NullCoalesce (lhs->fTCP).UnionBounds (*rhs.fTCP);
        }
        if (rhs.fUDP) {
            lhs->fUDP = NullCoalesce (lhs->fUDP).UnionBounds (*rhs.fUDP);
        }
    };
}

/*
 ********************************************************************************
 ************************** Model::OperatingSystem ******************************
 ********************************************************************************
 */
String OperatingSystem::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "majorOSCategory: "sv << fMajorOSCategory << ", "sv;
    sb << "fullVersionedOSName: "sv << fFullVersionedOSName << ", "sv;
    sb << "}"sv;
    return sb;
}

const ObjectVariantMapper OperatingSystem::kMapper = [] () {
    ObjectVariantMapper mapper;
    mapper.AddClass<OperatingSystem> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"majorOSCategory"sv, StructFieldMetaInfo{&OperatingSystem::fMajorOSCategory}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"fullVersionedName"sv, StructFieldMetaInfo{&OperatingSystem::fFullVersionedOSName}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    return mapper;
}();

/*
 ********************************************************************************
 ***************************** Model::Manufacturer ******************************
 ********************************************************************************
 */
String Model::Manufacturer::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "shortName: "sv << fShortName << ", "sv;
    sb << "fullName: "sv << fFullName << ", "sv;
    sb << "webSiteURL: "sv << fWebSiteURL << ", "sv;
    sb << "}"sv;
    return sb;
}

const ObjectVariantMapper Model::Manufacturer::kMapper = [] () {
    ObjectVariantMapper mapper;
    mapper.AddCommonType<optional<String>> ();
    mapper.AddCommonType<URI> ();
    mapper.AddCommonType<optional<URI>> ();
    mapper.AddClass<Manufacturer> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"shortName"sv, StructFieldMetaInfo{&Manufacturer::fShortName}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"fullName"sv, StructFieldMetaInfo{&Manufacturer::fFullName}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"webSiteURL"sv, StructFieldMetaInfo{&Manufacturer::fWebSiteURL}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    return mapper;
}();

/*
 ********************************************************************************
 **************************** Model::NetworkInterface ***************************
 ********************************************************************************
 */
String NetworkInterface::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "ID: "sv << fID << ", ";
    sb << "Internal-Interface-ID: "sv << fInternalInterfaceID << ", "sv;
    sb << "Friendly-Name: "sv << fFriendlyName << ", "sv;
    if (fDescription) {
        sb << "Description: "sv << *fDescription << ", "sv;
    }
    if (fType) {
        sb << "Type: "sv << *fType << ", "sv;
    }
    if (fHardwareAddress) {
        sb << "Hardware-Address: "sv << *fHardwareAddress << ", "sv;
    }
    if (fTransmitSpeedBaud) {
        sb << "Transmit-Speed-Baud: "sv << *fTransmitSpeedBaud << ", "sv;
    }
    if (fReceiveLinkSpeedBaud) {
        sb << "Receive-Link-Speed-Baud: "sv << *fReceiveLinkSpeedBaud << ", "sv;
    }
    if (fWirelessInfo) {
        sb << "Wireless-Info: "sv << fWirelessInfo << ", "sv;
    }
    sb << "Bindings: "sv << fBindings << ", "sv;

    sb << "Gateways: "sv << fGateways << ", "sv;
    sb << "DNS-Servers: "sv << fDNSServers << ", "sv;
    if (fStatus) {
        sb << "Status: "sv << *fStatus << ", "sv;
    }
    if (fAggregatedBy) {
        sb << "AggregatedBy: "sv << *fAggregatedBy << ", "sv;
    }
    if (fAttachedToDevices) {
        sb << "AttachedToDevices: "sv << *fAttachedToDevices << ", "sv;
    }
    if (fAggregatesReversibly) {
        sb << "AggregatesReversibly: "sv << fAggregatesReversibly << ", "sv;
    }
    if (fAggregatesIrreversibly) {
        sb << "AggregatesIrreversibly: "sv << fAggregatesIrreversibly << ", "sv;
    }
    if (fIDPersistent) {
        sb << "IDPersistent: "sv << fIDPersistent << ", "sv;
    }
    sb << "}"sv;
    return sb;
}

auto NetworkInterface::GenerateFingerprintFromProperties () const -> FingerprintType
{
    StringBuilder sb;
    sb << fInternalInterfaceID;
    sb << "/"sv;
    sb << fFriendlyName;
    sb << "/"sv;
    if (fDescription) {
        sb << *fDescription;
    }
    sb << "/"sv;
    if (fType) {
        sb << Configuration::DefaultNames<NetworkInterface::Type>::k.GetName (*fType);
    }
    sb << "/"sv;
    if (fHardwareAddress) {
        sb << *fHardwareAddress;
    }
    return Cryptography::Digest::ComputeDigest<Cryptography::Digest::Algorithm::MD5> (sb.str ());
}

const ObjectVariantMapper NetworkInterface::kMapper = [] () {
    ObjectVariantMapper mapper;

    using TypeMappingDetails = ObjectVariantMapper::TypeMappingDetails;

    mapper.AddCommonType<NetworkInterface::Type> ();
    mapper.AddCommonType<optional<NetworkInterface::Type>> ();
    mapper.AddCommonType<InternetAddress> ();
    mapper.AddCommonType<optional<InternetAddress>> ();
    mapper.AddCommonType<Sequence<InternetAddress>> ();
    mapper.AddCommonType<optional<Sequence<InternetAddress>>> ();

    using IO::Network::CIDR;
    mapper.AddCommonType<CIDR> ();
    mapper.AddCommonType<Set<CIDR>> ();
    mapper.AddCommonType<Collection<CIDR>> ();
    mapper.AddCommonType<Collection<InternetAddress>> ();

    mapper.AddCommonType<IO::Network::Interface::WirelessInfo::State> ();
    mapper.AddCommonType<optional<IO::Network::Interface::WirelessInfo::State>> ();
    mapper.AddCommonType<IO::Network::Interface::WirelessInfo::ConnectionMode> ();
    mapper.AddCommonType<optional<IO::Network::Interface::WirelessInfo::ConnectionMode>> ();
    mapper.AddCommonType<IO::Network::Interface::WirelessInfo::BSSType> ();
    mapper.AddCommonType<optional<IO::Network::Interface::WirelessInfo::BSSType>> ();
    mapper.AddCommonType<IO::Network::Interface::WirelessInfo::PhysicalConnectionType> ();
    mapper.AddCommonType<optional<IO::Network::Interface::WirelessInfo::PhysicalConnectionType>> ();
    mapper.AddCommonType<IO::Network::Interface::WirelessInfo::AuthAlgorithm> ();
    mapper.AddCommonType<optional<IO::Network::Interface::WirelessInfo::AuthAlgorithm>> ();

    mapper.AddClass<NetworkInterface::WirelessInfo> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"SSID"sv, StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fSSID}},
        {"state"sv, StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fState}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"connectionMode"sv, StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fConnectionMode}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"profileName"sv, StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fProfileName}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"BSSType"sv, StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fBSSType}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"MACAddress"sv, StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fMACAddress}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"physicalConnectionType"sv, StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fPhysicalConnectionType},
         ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"signalQuality"sv, StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fSignalQuality}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"securityEnabled"sv, StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fSecurityEnabled}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"802.1XEnabled"sv, StructFieldMetaInfo{&NetworkInterface::WirelessInfo::f8021XEnabled}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"authAlgorithm"sv, StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fAuthAlgorithm}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"cipher"sv, StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fCipher}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<optional<NetworkInterface::WirelessInfo>> ();
    mapper.AddCommonType<NetworkInterface::Status> ();
    mapper.AddCommonType<Set<NetworkInterface::Status>> ();
    mapper.AddCommonType<optional<Set<NetworkInterface::Status>>> ();

    mapper.AddCommonType<optional<GUID>> ();
    mapper.AddCommonType<Set<GUID>> ();
    mapper.AddCommonType<optional<Set<GUID>>> ();

    mapper.AddClass<NetworkInterface> (initializer_list<ObjectVariantMapper::StructFieldInfo> {
        {"platformInterfaceID"sv, StructFieldMetaInfo{&NetworkInterface::fInternalInterfaceID}},
            {"id"sv, StructFieldMetaInfo{&NetworkInterface::fID}},
            {"aggregatedBy"sv, StructFieldMetaInfo{&NetworkInterface::fAggregatedBy}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {"attachedToDevices"sv, StructFieldMetaInfo{&NetworkInterface::fAttachedToDevices}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {"friendlyName"sv, StructFieldMetaInfo{&NetworkInterface::fFriendlyName}},
            {"description"sv, StructFieldMetaInfo{&NetworkInterface::fDescription}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {"type"sv, StructFieldMetaInfo{&NetworkInterface::fType}},
            {"hardwareAddress"sv, StructFieldMetaInfo{&NetworkInterface::fHardwareAddress}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {"transmitSpeedBaud"sv, StructFieldMetaInfo{&NetworkInterface::fTransmitSpeedBaud}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {"receiveLinkSpeedBaud"sv, StructFieldMetaInfo{&NetworkInterface::fReceiveLinkSpeedBaud}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {"wirelessInformation"sv, StructFieldMetaInfo{&NetworkInterface::fWirelessInfo}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            //SEE OVERRIDE BELOW {L"boundAddressRanges", StructFieldMetaInfo{&NetworkInterface::fBindings.fAddressRanges}},
            //SEE OVERRIDE BELOW {L"boundAddresses", StructFieldMetaInfo{&NetworkInterface::fBindings.fAddresses}},
            // StructFieldMetaInfo{} doesn't work with nested members - https://stackoverflow.com/questions/1929887/is-pointer-to-inner-struct-member-forbidden
            // So override type mappers manually to select right sub-element of Bindings
            {"boundAddressRanges"sv, StructFieldMetaInfo{&NetworkInterface::fBindings},
             TypeMappingDetails{ObjectVariantMapper::FromObjectMapperType<NetworkInterface::Bindings> (
                                    [] (const ObjectVariantMapper& mapper, const NetworkInterface::Bindings* objOfType) -> VariantValue {
                                        if (not objOfType->fAddressRanges.empty ()) {
                                            return mapper.FromObject (objOfType->fAddressRanges);
                                        }
                                        return VariantValue{};
                                    }),
                                ObjectVariantMapper::ToObjectMapperType<NetworkInterface::Bindings> (
                                    [=] (const ObjectVariantMapper& mapper, const VariantValue& d, NetworkInterface::Bindings* intoObj) -> void {
                                        if (d != VariantValue{}) {
                                            intoObj->fAddressRanges = mapper.ToObject<Containers::Collection<CIDR>> (d);
                                        }
                                    })}},
            {"boundAddresses"sv, StructFieldMetaInfo{&NetworkInterface::fBindings},
             TypeMappingDetails{ObjectVariantMapper::FromObjectMapperType<NetworkInterface::Bindings> (
                                    [] (const ObjectVariantMapper& mapper, const NetworkInterface::Bindings* objOfType) -> VariantValue {
                                        if (not objOfType->fAddresses.empty ()) {
                                            return mapper.FromObject (objOfType->fAddresses);
                                        }
                                        return VariantValue{};
                                    }),
                                ObjectVariantMapper::ToObjectMapperType<NetworkInterface::Bindings> (
                                    [=] (const ObjectVariantMapper& mapper, const VariantValue& d, NetworkInterface::Bindings* intoObj) -> void {
                                        if (d != VariantValue{}) {
                                            intoObj->fAddresses = mapper.ToObject<Containers::Collection<InternetAddress>> (d);
                                        }
                                    })}},
            {"gateways"sv, StructFieldMetaInfo{&NetworkInterface::fGateways}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {"DNSServers"sv, StructFieldMetaInfo{&NetworkInterface::fDNSServers}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {"status"sv, StructFieldMetaInfo{&NetworkInterface::fStatus}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {"aggregatesReversibly"sv, StructFieldMetaInfo{&NetworkInterface::fAggregatesReversibly}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {"aggregatesIrreversibly"sv, StructFieldMetaInfo{&NetworkInterface::fAggregatesIrreversibly},
             ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {"idIsPersistent"sv, StructFieldMetaInfo{&NetworkInterface::fIDPersistent}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
#if qDebug
            {"debugProps"sv, StructFieldMetaInfo{&NetworkInterface::fDebugProps}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
#endif
    });
    if constexpr (kIncludeFingerprintsInOutputTMP2Test_) {
        mapper.AddSubClass<NetworkInterface, NetworkInterface> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
            {"fingerprint"sv, TypeMappingDetails{ObjectVariantMapper::FromObjectMapperType<NetworkInterface> (
                                                     [] (const ObjectVariantMapper&, const NetworkInterface* objOfType) -> VariantValue {
                                                         return VariantValue{objOfType->GenerateFingerprintFromProperties ().As<String> ()};
                                                     }),
                                                 ObjectVariantMapper::ToObjectMapperType<NetworkInterface> (nullptr)}},
        });
    }

    mapper.AddCommonType<Collection<NetworkInterface>> ();
    return mapper;
}();

NetworkInterface NetworkInterface::Rollup (const optional<NetworkInterface>& previousRollupNetworkInterface, const NetworkInterface& instanceNetwork2Add)
{
    if (previousRollupNetworkInterface) {
        NetworkInterface r = *previousRollupNetworkInterface;
        Assert (r.fID == instanceNetwork2Add.GenerateFingerprintFromProperties ());
        Assert (r.GenerateFingerprintFromProperties () == instanceNetwork2Add.GenerateFingerprintFromProperties ());
        Assert (r.fAggregatesReversibly); // because already its a rollup of something
        r.fAggregatesReversibly->Add (instanceNetwork2Add.fID);
        Ensure (r.GenerateFingerprintFromProperties () == instanceNetwork2Add.GenerateFingerprintFromProperties ());
        return r;
    }
    else {
        NetworkInterface r;
        r.fID                   = instanceNetwork2Add.GenerateFingerprintFromProperties ();
        r.fFriendlyName         = instanceNetwork2Add.fFriendlyName;
        r.fDescription          = instanceNetwork2Add.fDescription;
        r.fType                 = instanceNetwork2Add.fType;
        r.fHardwareAddress      = instanceNetwork2Add.fHardwareAddress;
        r.fInternalInterfaceID  = instanceNetwork2Add.fInternalInterfaceID;
        r.fAggregatesReversibly = Set<GUID>{instanceNetwork2Add.fID};
        Assert (r.GenerateFingerprintFromProperties () == r.fID); // captured all that matters/part of fingerprint    return rollupNetwork;
        Ensure (r.GenerateFingerprintFromProperties () == instanceNetwork2Add.GenerateFingerprintFromProperties ());
        return r;
    }
}

/*
 ********************************************************************************
 ****** Model::Network::UserOverridesType::NetworkInterfaceAggregateRule ********
 ********************************************************************************
 */
String Model::Network::UserOverridesType::NetworkInterfaceAggregateRule::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "interfaceType: "sv << fInterfaceType << ", "sv;
    sb << "fingerprint: "sv << fFingerprint;
    sb << "}"sv;
    return sb;
}

/*
 ********************************************************************************
 ********************** Model::Network::UserOverridesType ***********************
 ********************************************************************************
 */
bool Model::Network::UserOverridesType::IsNonTrivial () const
{
    return fName or fTags or fNotes or fAggregateNetworks or fAggregateFingerprints or fAggregateGatewayHardwareAddresses or
           fAggregateNetworkInterfacesMatching;
}

String Model::Network::UserOverridesType::ToString () const
{
    StringBuilder sb;
    sb << "{";
    if (fName) {
        sb << "Name: "sv << fName << ", "sv;
    }
    if (fTags) {
        sb << "Tags: "sv << fTags << ", "sv;
    }
    if (fNotes) {
        sb << "Notes: "sv << fNotes << ", "sv;
    }
    if (fAggregateNetworks) {
        sb << "AggregateNetworks: "sv << fAggregateNetworks << ", "sv;
    }
    if (fAggregateFingerprints) {
        sb << "AggregateFingerprints: "sv << fAggregateFingerprints << ", "sv;
    }
    if (fAggregateGatewayHardwareAddresses) {
        sb << "AggregateGatewayHardwareAddresses: "sv << fAggregateGatewayHardwareAddresses << ", "sv;
    }
    if (fAggregateNetworkInterfacesMatching) {
        sb << "AggregateNetworkInterfacesMatching: "sv << fAggregateNetworkInterfacesMatching << ", "sv;
    }
    sb << "}"sv;
    return sb;
}

const DataExchange::ObjectVariantMapper Model::Network::UserOverridesType::kMapper = [] () {
    ObjectVariantMapper mapper;
    mapper.AddCommonType<optional<String>> ();
    mapper.AddCommonType<Set<String>> ();
    mapper.AddCommonType<optional<Set<String>>> ();
    mapper.AddCommonType<Set<GUID>> ();
    mapper.AddCommonType<optional<Set<GUID>>> ();
    mapper.AddCommonType<Stroika::Foundation::IO::Network::Interface::Type> ();
    mapper.AddClass<UserOverridesType::NetworkInterfaceAggregateRule> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"interfaceType"sv, StructFieldMetaInfo{&UserOverridesType::NetworkInterfaceAggregateRule::fInterfaceType}},
        {"fingerprint"sv, StructFieldMetaInfo{&UserOverridesType::NetworkInterfaceAggregateRule::fFingerprint}},
    });
    mapper.AddCommonType<Sequence<UserOverridesType::NetworkInterfaceAggregateRule>> ();
    mapper.AddCommonType<optional<Sequence<UserOverridesType::NetworkInterfaceAggregateRule>>> ();
    mapper.AddClass<UserOverridesType> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"name"sv, StructFieldMetaInfo{&UserOverridesType::fName}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"tags"sv, StructFieldMetaInfo{&UserOverridesType::fTags}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"notes"sv, StructFieldMetaInfo{&UserOverridesType::fNotes}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"aggregateNetworks"sv, StructFieldMetaInfo{&UserOverridesType::fAggregateNetworks}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"aggregateFingerprints"sv, StructFieldMetaInfo{&UserOverridesType::fAggregateFingerprints}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"aggregateGatewayHardwareAddresses"sv, StructFieldMetaInfo{&UserOverridesType::fAggregateGatewayHardwareAddresses},
         ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"aggregateNetworkInterfacesMatching"sv, StructFieldMetaInfo{&UserOverridesType::fAggregateNetworkInterfacesMatching},
         ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    return mapper;
}();

/*
 ********************************************************************************
 ********************************** Model::Network ******************************
 ********************************************************************************
 */
Network Network::Merge (const Network& baseNetwork, const Network& priorityNetwork)
{
    // Note: items that are atomic can be copied with CopyToIf (handles optional part)
    // Items that are structured, if (most of the time) you want to just conditionally replace the ones
    // that are present, iterate and copy2if or add subelements.
    Network merged = baseNetwork;
    merged.fID     = priorityNetwork.fID;
    for (const auto& i : priorityNetwork.fNames) {
        merged.fNames.Add (i.fName, i.fPriority);
    }
    merged.fNetworkAddresses.AddAll (priorityNetwork.fNetworkAddresses);
    merged.fAttachedInterfaces.AddAll (priorityNetwork.fAttachedInterfaces);
    merged.fGateways += priorityNetwork.fGateways;
    merged.fGatewayHardwareAddresses += priorityNetwork.fGatewayHardwareAddresses;
    priorityNetwork.fDNSServers.Apply ([&] (auto inetAddr) { merged.fDNSServers += inetAddr; });
    Memory::AccumulateIf (&merged.fExternalAddresses, priorityNetwork.fExternalAddresses);
    Memory::CopyToIf (&merged.fGEOLocInformation, priorityNetwork.fGEOLocInformation);
    Memory::CopyToIf (&merged.fInternetServiceProvider, priorityNetwork.fInternetServiceProvider);
    MergeSeen_ (&merged.fSeen, priorityNetwork.fSeen);
    Memory::AccumulateIf (&merged.fAggregatesReversibly, priorityNetwork.fAggregatesReversibly); // @todo consider if this is right way to combine
    Memory::AccumulateIf (&merged.fAggregatesIrreversibly, priorityNetwork.fAggregatesIrreversibly);
    Memory::AccumulateIf (&merged.fAggregatesFingerprints, priorityNetwork.fAggregatesFingerprints);
    Memory::CopyToIf (&merged.fIDPersistent, priorityNetwork.fIDPersistent);
    Memory::CopyToIf (&merged.fUserOverrides, priorityNetwork.fUserOverrides); // for now, no need to look inside and accumulate because only one place can generate user-overrides - some special TBD database record - LGP 2022-09-14
#if qDebug
    if (priorityNetwork.fDebugProps) {
        // copy sub-elements of debug props
        Mapping<String, VariantValue> newProps = NullCoalesce (merged.fDebugProps);
        for (auto i : *priorityNetwork.fDebugProps) {
            newProps.Add (i);
        }
        merged.fDebugProps = newProps;
    }
#endif
    return merged;
}

Network Network::Rollup (const Network& rollupNetwork, const Network& instanceNetwork2Add)
{
    // @todo this must take ARG OF attachedNetworkInterfaceDicoveryID2RollupIDMap and use for merged in interfaces (not just savedInterfaces)

    // Use seen.Ever() to decide which 'device' gets precedence in merging. Give the most
    // recent device precedence
    Network merged = (rollupNetwork.fSeen.empty () or rollupNetwork.fSeen.GetUpperBound () < instanceNetwork2Add.fSeen.GetUpperBound ())
                         ? Merge (rollupNetwork, instanceNetwork2Add)
                         : Merge (instanceNetwork2Add, rollupNetwork);
    // regardless of dates, keep the rollupDevice GUID and original rolled up interface ids
    merged.fID                     = rollupNetwork.fID;
    merged.fAttachedInterfaces     = rollupNetwork.fAttachedInterfaces;
    merged.fAggregatesReversibly   = rollupNetwork.fAggregatesReversibly;
    merged.fAggregatesIrreversibly = rollupNetwork.fAggregatesIrreversibly;

    if (merged.fAggregatesReversibly.has_value ()) {
        merged.fAggregatesReversibly->Add (instanceNetwork2Add.fID);
    }
    else {
        merged.fAggregatesReversibly = Set<GUID>{instanceNetwork2Add.fID};
    }
    // merged.fAggregatesIrreversibly = nullopt;
    merged.fIDPersistent = false;
    Memory::CopyToIf (&merged.fUserOverrides, instanceNetwork2Add.fUserOverrides); // for now, no need to look inside and accumulate because only one place can generate user-overrides - some special TBD database record - LGP 2022-09-14
    return merged;
}

Network::FingerprintType Network::GenerateFingerprintFromProperties () const
{
    /*
     *  A network is not a super-well defined concept, so deciding if two instances of a network refer to the same
     *  network is a bit of a judgement call.
     * 
     *  But a few key things probably make sense:
     *      >   Same ISP
     *      >   Same GeoLoc (with exceptions)
     *      >   Same IPv4 CIDR
     *      >   Same Gateway addresses (hardware address, local gateway address, external address)
     * 
     *  Things we allow to differ:
     *      >   details of any IP-v6 network addresses (if there were IPV4 CIDRs agreed upon).
     * 
     *  Now since geoloc tends to vary needlessly, and same for ISP reporting, just use the external address.
     *  This TOO can float (and maybe we want to do something about that like trim a few significant bits?).
     * 
     *  note - if external id floats, this will change, and it will look like a new network. NOT SURE best to include
     *  network id, but often a good idea, and not hard to add records allowing combine of networks (more of a PITA than
     *  other way where we didn't include and had to force separate).
     */
    auto useCIDRs = [] (const Set<CIDR>& ias) {
        // for some reason, gateway list sometimes contains IPv4 only, and sometimes IPv4 and IPv6 addresses
        // treat the list the same if the gateway list ipv4s at least are the same (and non-empty)
        // if IPv4 CIDRs same (and non-empty), then ignore differences in IPv4 addressses
        Set<CIDR> ipv4s{
            ias.Where ([] (const CIDR& i) { return i.GetBaseInternetAddress ().GetAddressFamily () == InternetAddress::AddressFamily::V4; })};
        return ipv4s.empty () ? ias : ipv4s;
    };
    auto useAddresses = [] (const Set<InternetAddress>& ias) {
        // for some reason, gateway list sometimes contains IPv4 only, and sometimes IPv4 and IPv6 addresses
        // treat the list the same if the gateway list ipv4s at least are the same (and non-empty)
        // if IPv4 CIDRs same (and non-empty), then ignore differences in IPv4 addressses
        Set<InternetAddress> ipv4s{
            ias.Where ([] (const InternetAddress& i) { return i.GetAddressFamily () == InternetAddress::AddressFamily::V4; })};
        return ipv4s.empty () ? ias : ipv4s;
    };

    // combine hardare addresses of gateway with gateway address, and external ip address (and hash)
    // into a single ID that probably mostly uniquely ids a network.
    StringBuilder sb;

#if __cplusplus >= kStrokia_Foundation_Configuration_cplusplus_20
    auto accumList = [&sb]<typename T> (const Set<T>& elts)
#else
    auto accumList = [&sb] (const auto& elts)
#endif
    {
#if not(__cplusplus >= kStrokia_Foundation_Configuration_cplusplus_20)
        using T = typename decay_t<decltype (elts)>::value_type;
#endif
        switch (elts.size ()) {
            case 0:
                sb << "/"sv;
                break;
            case 1:
                sb << elts.Nth (0).template As<String> ();
                sb << "/"sv;
                break;
            default: {
                // regularize
                for (const T& i : SortedSet<T>{elts}) {
                    sb << i.template As<String> ();
                    sb << "/"sv;
                }
            } break;
        }
    };
    if (fExternalAddresses) {
        accumList (*fExternalAddresses);
    }
    sb << ";"sv;
    accumList (useCIDRs (fNetworkAddresses));
    sb << ";"sv;
    accumList (useAddresses (fGateways));
    sb << ";"sv;
    accumList (fGatewayHardwareAddresses);
    return Cryptography::Digest::ComputeDigest<Cryptography::Digest::Algorithm::MD5> (sb.str ());
}

String Network::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "ID: "sv << fID << ", "sv;
    sb << "Network-Addresses: "sv << fNetworkAddresses << ", "sv;
    sb << "Names: "sv << fNames << ", ";
    if (fAggregatedBy) {
        sb << "AggregatedBy: "sv << fAggregatedBy << ", "sv;
    }
    sb << "Attached-Interfaces: "sv << fAttachedInterfaces << ", "sv;
    sb << "Gateways: " << fGateways << ", ";
    sb << "GatewayHardwareAddresses: "sv << fGatewayHardwareAddresses << ", "sv;
    sb << "DNS-Servers: "sv << fDNSServers << ", "sv;
    sb << "Seen: "sv << fSeen << ", "sv;
    sb << "Aggregates-Reversibly: "sv << fAggregatesReversibly << ", "sv;
    sb << "Aggregates-Irreverisbly: "sv << fAggregatesIrreversibly << ", "sv;
    sb << "Aggregates-Fingerprints: "sv << fAggregatesFingerprints << ", "sv;
    sb << "IDPersistent: "sv << fIDPersistent;
    sb << "}"sv;
    return sb;
}

const ObjectVariantMapper Network::kMapper = [] () {
    using namespace BackendApp::Common;

    ObjectVariantMapper mapper;

    using IO::Network::CIDR;
    mapper.AddCommonType<CIDR> ();
    mapper.AddCommonType<Sequence<CIDR>> ();
    mapper.AddCommonType<Set<CIDR>> ();
    mapper.AddCommonType<InternetAddress> ();
    mapper.AddCommonType<Sequence<InternetAddress>> ();
    mapper.AddCommonType<Set<InternetAddress>> ();
    mapper.AddCommonType<optional<InternetAddress>> ();
    mapper.AddCommonType<optional<Sequence<InternetAddress>>> ();
    mapper.AddCommonType<optional<Set<InternetAddress>>> ();
    mapper.AddCommonType<optional<GUID>> ();
    mapper.AddCommonType<Set<GUID>> ();
    mapper.AddCommonType<optional<Set<GUID>>> ();
    mapper += Common::PrioritizedNames::kMapper;

    if (true) {
        // looks better as an object, than as an array
        // see https://stroika.atlassian.net/browse/STK-923
        struct laglon_ {
            float lat;
            float lon;
        };
        mapper.AddClass<laglon_> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
            {"latitude"sv, StructFieldMetaInfo{&laglon_::lat}},
            {"longitude"sv, StructFieldMetaInfo{&laglon_::lon}},
        });
        mapper.Add<tuple<float, float>> (
            [] (const ObjectVariantMapper& mapper, const tuple<float, float>* obj) -> VariantValue {
                return mapper.FromObject (laglon_{get<0> (*obj), get<1> (*obj)});
            },
            [] (const ObjectVariantMapper& mapper, const VariantValue& d, tuple<float, float>* intoObj) -> void {
                auto tmp{mapper.ToObject<laglon_> (d)};
                *intoObj = make_tuple (tmp.lat, tmp.lon);
            });
    }
    else {
        mapper.AddCommonType<tuple<float, float>> (); // works but represents as an array
    }

    mapper.AddCommonType<optional<tuple<float, float>>> ();

    mapper.AddClass<InternetServiceProvider> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"name", StructFieldMetaInfo{&InternetServiceProvider::name}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<optional<InternetServiceProvider>> ();

    mapper.AddClass<GEOLocationInformation> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"countryCode"sv, StructFieldMetaInfo{&GEOLocationInformation::fCountryCode}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"city"sv, StructFieldMetaInfo{&GEOLocationInformation::fCity}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"regionCode"sv, StructFieldMetaInfo{&GEOLocationInformation::fRegionCode}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"postalCode"sv, StructFieldMetaInfo{&GEOLocationInformation::fPostalCode}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"coordinates"sv, StructFieldMetaInfo{&GEOLocationInformation::fLatitudeAndLongitude}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<optional<GEOLocationInformation>> ();

    mapper += UserOverridesType::kMapper;
    mapper.AddCommonType<optional<UserOverridesType>> ();

    mapper.AddClass<Network> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"id"sv, StructFieldMetaInfo{&Network::fID}},
        {"names"sv, StructFieldMetaInfo{&Network::fNames}},
        {"networkAddresses"sv, StructFieldMetaInfo{&Network::fNetworkAddresses}},
        {"attachedInterfaces"sv, StructFieldMetaInfo{&Network::fAttachedInterfaces}},
        {"gateways"sv, StructFieldMetaInfo{&Network::fGateways}},
        {"gatewayHardwareAddresses"sv, StructFieldMetaInfo{&Network::fGatewayHardwareAddresses}},
        {"DNSServers"sv, StructFieldMetaInfo{&Network::fDNSServers}},
        {"externalAddresses"sv, StructFieldMetaInfo{&Network::fExternalAddresses}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"geographicLocation"sv, StructFieldMetaInfo{&Network::fGEOLocInformation}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"internetServiceProvider"sv, StructFieldMetaInfo{&Network::fInternetServiceProvider}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"aggregatedBy"sv, StructFieldMetaInfo{&Network::fAggregatedBy}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"seen"sv, StructFieldMetaInfo{&Network::fSeen}, kDateRangeMapper_},
        {"aggregatesReversibly"sv, StructFieldMetaInfo{&Network::fAggregatesReversibly}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"aggregatesIrreversibly"sv, StructFieldMetaInfo{&Network::fAggregatesIrreversibly}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"aggregatesFingerprints"sv, StructFieldMetaInfo{&Network::fAggregatesFingerprints}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"idIsPersistent"sv, StructFieldMetaInfo{&Network::fIDPersistent}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"userOverrides"sv, StructFieldMetaInfo{&Network::fUserOverrides}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
#if qDebug
    mapper.AddSubClass<Network, Network> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"debugProps"sv, StructFieldMetaInfo{&Network::fDebugProps}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
#endif
    if constexpr (kIncludeFingerprintsInOutputTMP2Test_) {
        mapper.AddSubClass<Network, Network> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
            {"fingerprint"sv,
             ObjectVariantMapper::TypeMappingDetails{
                 ObjectVariantMapper::FromObjectMapperType<Network> ([] (const ObjectVariantMapper&, const Network* objOfType) -> VariantValue {
                     return VariantValue{objOfType->GenerateFingerprintFromProperties ().As<String> ()};
                 }),
                 ObjectVariantMapper::ToObjectMapperType<Network> (nullptr)}},
        });
    }
    mapper.AddCommonType<Sequence<Network>> ();

    return mapper;
}();

/*
 ********************************************************************************
 *********************** Model::NetworkAttachmentInfo ***************************
 ********************************************************************************
 */
String NetworkAttachmentInfo::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "hardwareAddresses: "sv << hardwareAddresses << ", "sv;
    sb << "localAddresses: "sv << localAddresses;
    sb << "}"sv;
    return sb;
}

/*
 ********************************************************************************
 ***************************** Model::Device::SeenType **************************
 ********************************************************************************
 */
optional<Range<DateTime>> Model::Device::SeenType::EverSeen () const
{
    optional<Range<DateTime>> result{fARP};
    if (fCollector) {
        if (result) {
            result = result->UnionBounds (*fCollector);
        }
        else {
            result = fCollector;
        }
    }
    if (fICMP) {
        if (result) {
            result = result->UnionBounds (*fICMP);
        }
        else {
            result = fICMP;
        }
    }
    if (fTCP) {
        if (result) {
            result = result->UnionBounds (*fTCP);
        }
        else {
            result = fTCP;
        }
    }
    if (fUDP) {
        if (result) {
            result = result->UnionBounds (*fUDP);
        }
        else {
            result = fUDP;
        }
    }
    return result;
}

String Model::Device::SeenType::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "ARP: "sv << fARP << ", "sv;
    sb << "Collector: "sv << fCollector << ", "sv;
    sb << "ICMP: " << fICMP << ", "sv;
    sb << "TCP: "sv << fTCP << ", "sv;
    sb << "UDP: "sv << fUDP;
    sb << "}"sv;
    return sb;
}

const DataExchange::ObjectVariantMapper Model::Device::SeenType::kMapper = [] () {
    ObjectVariantMapper mapper;
    mapper.AddCommonType<Traversal::Range<DateTime>> ();
    mapper.AddClass<SeenType> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"ARP"sv, StructFieldMetaInfo{&SeenType::fARP}, kOptionalDateRangeMapper_, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"Collector"sv, StructFieldMetaInfo{&SeenType::fCollector}, kOptionalDateRangeMapper_, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"ICMP"sv, StructFieldMetaInfo{&SeenType::fICMP}, kOptionalDateRangeMapper_, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"UDP"sv, StructFieldMetaInfo{&SeenType::fUDP}, kOptionalDateRangeMapper_, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"TCP"sv, StructFieldMetaInfo{&SeenType::fTCP}, kOptionalDateRangeMapper_, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    return mapper;
}();

/*
 ********************************************************************************
 ********************** Model::Device::UserOverridesType ************************
 ********************************************************************************
 */
bool Model::Device::UserOverridesType::IsNonTrivial () const
{
    return fName or fTags or fNotes or fAggregateDevices or fAggregateDeviceHardwareAddresses;
}

String Model::Device::UserOverridesType::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    if (fName) {
        sb << "Name: "sv << fName << ", "sv;
    }
    if (fTags) {
        sb << "Tags: "sv << fTags << ", "sv;
    }
    if (fNotes) {
        sb << "Notes: "sv << fNotes << ", "sv;
    }
    if (fAggregateDevices) {
        sb << "AggregateDevices: "sv << fAggregateDevices << ", "sv;
    }
    if (fAggregateDeviceHardwareAddresses) {
        sb << "AggregateDeviceHardwareAddresses: "sv << fAggregateDeviceHardwareAddresses << ", "sv;
    }
    sb << "}"sv;
    return sb;
}

const DataExchange::ObjectVariantMapper Model::Device::UserOverridesType::kMapper = [] () {
    ObjectVariantMapper mapper;
    mapper.AddCommonType<optional<String>> ();
    mapper.AddCommonType<Set<String>> ();
    mapper.AddCommonType<optional<Set<String>>> ();
    mapper.AddCommonType<Set<GUID>> ();
    mapper.AddCommonType<optional<Set<GUID>>> ();
    mapper.AddClass<UserOverridesType> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"name"sv, StructFieldMetaInfo{&UserOverridesType::fName}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"tags"sv, StructFieldMetaInfo{&UserOverridesType::fTags}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"notes"sv, StructFieldMetaInfo{&UserOverridesType::fNotes}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"aggregateDevices"sv, StructFieldMetaInfo{&UserOverridesType::fAggregateDevices}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"aggregateDeviceHardwareAddresses"sv, StructFieldMetaInfo{&UserOverridesType::fAggregateDeviceHardwareAddresses},
         ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    return mapper;
}();

/*
 ********************************************************************************
 ********************************* Model::Device ********************************
 ********************************************************************************
 */
const ObjectVariantMapper Device::kMapper = [] () {
    ObjectVariantMapper mapper;

    mapper += OperatingSystem::kMapper;
    mapper.AddCommonType<optional<OperatingSystem>> ();
    mapper += Manufacturer::kMapper;
    mapper.AddCommonType<optional<Manufacturer>> ();
    mapper.AddCommonType<InternetAddress> ();
    mapper.AddCommonType<optional<InternetAddress>> ();
    mapper.AddCommonType<Sequence<InternetAddress>> ();
    mapper.AddCommonType<CIDR> ();
    mapper.AddCommonType<Sequence<CIDR>> ();
    mapper.AddCommonType<DateTime> ();
    mapper.AddCommonType<optional<DateTime>> ();
    mapper.AddCommonType<Set<GUID>> ();
    mapper.AddCommonType<optional<Set<GUID>>> ();
    mapper.AddCommonType<Device::DeviceType> ();
    mapper.AddCommonType<Set<Device::DeviceType>> ();
    mapper.AddCommonType<optional<Set<Device::DeviceType>>> ();
    mapper.AddCommonType<optional<String>> ();
    mapper.AddCommonType<optional<float>> ();
    mapper.AddCommonType<Collection<String>> ();
    mapper.AddCommonType<Sequence<String>> ();
    mapper.AddCommonType<Set<String>> ();
    mapper.AddCommonType<optional<Set<String>>> ();
    mapper.AddCommonType<URI> ();
    mapper.AddCommonType<optional<URI>> ();
    mapper.AddCommonType<Set<GUID>> ();
    mapper.AddCommonType<optional<Set<GUID>>> ();
    mapper += SeenType::kMapper;
    mapper += UserOverridesType::kMapper;
    mapper.AddCommonType<optional<UserOverridesType>> ();
    mapper += Common::PrioritizedNames::kMapper;

    mapper.AddClass<NetworkAttachmentInfo> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"hardwareAddresses"sv, StructFieldMetaInfo{&NetworkAttachmentInfo::hardwareAddresses}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"localAddresses"sv, StructFieldMetaInfo{&NetworkAttachmentInfo::localAddresses}},
    });
    mapper.AddCommonType<Mapping<GUID, NetworkAttachmentInfo>> ();

    mapper.AddClass<Device> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"id"sv, StructFieldMetaInfo{&Device::fID}},
        {"aggregatedBy"sv, StructFieldMetaInfo{&Device::fAggregatedBy}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"names"sv, StructFieldMetaInfo{&Device::fNames}},
        {"type"sv, StructFieldMetaInfo{&Device::fTypes}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"seen"sv, StructFieldMetaInfo{&Device::fSeen}},
        {"openPorts"sv, StructFieldMetaInfo{&Device::fOpenPorts}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"icon"sv, StructFieldMetaInfo{&Device::fIcon}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"manufacturer"sv, StructFieldMetaInfo{&Device::fManufacturer}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"attachedNetworks"sv, StructFieldMetaInfo{&Device::fAttachedNetworks}},
        {"attachedNetworkInterfaces"sv, StructFieldMetaInfo{&Device::fAttachedNetworkInterfaces}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"presentationURL"sv, StructFieldMetaInfo{&Device::fPresentationURL}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"operatingSystem"sv, StructFieldMetaInfo{&Device::fOperatingSystem}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"aggregatesReversibly"sv, StructFieldMetaInfo{&Device::fAggregatesReversibly}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"aggregatesIrreversibly"sv, StructFieldMetaInfo{&Device::fAggregatesIrreversibly}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"idIsPersistent"sv, StructFieldMetaInfo{&Device::fIDPersistent}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"userOverrides"sv, StructFieldMetaInfo{&Device::fUserOverrides}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
#if qDebug
    mapper.AddSubClass<Device, Device> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"debugProps"sv, StructFieldMetaInfo{&Device::fDebugProps}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
#endif
    mapper.AddCommonType<Sequence<Device>> ();
    return mapper;
}();

Set<String> Device::GetHardwareAddresses () const
{
    Set<String> result;
    for (const auto& iNet : fAttachedNetworks) {
        result += iNet.fValue.hardwareAddresses;
    }
    return result;
}

Set<InternetAddress> Device::GetInternetAddresses () const
{
    Set<InternetAddress> result;
    for (const auto& iNet : fAttachedNetworks) {
        result += iNet.fValue.localAddresses;
    }
    return result;
}

String Device::ToString () const
{
    return DataExchange::Variant::JSON::Writer{}.WriteAsString (Device::kMapper.FromObject (*this));
}

Device Device::Merge (const Device& baseDevice, const Device& priorityDevice)
{
    Device merged = baseDevice;
    for (const auto& i : priorityDevice.fNames) {
        merged.fNames.Add (i.fName, i.fPriority);
    }
    merged.fID = priorityDevice.fID;
    Memory::AccumulateIf (&merged.fTypes, priorityDevice.fTypes);
    Memory::CopyToIf (&merged.fIcon, priorityDevice.fIcon);
    MergeSeen_ (&merged.fSeen, priorityDevice.fSeen);
    Memory::CopyToIf (&merged.fManufacturer, priorityDevice.fManufacturer);
    merged.fAttachedNetworks.AddAll (priorityDevice.fAttachedNetworks); // @todo perhaps should MERGE these details...
    Memory::AccumulateIf (&merged.fOpenPorts, priorityDevice.fOpenPorts);
    Memory::CopyToIf (&merged.fPresentationURL, priorityDevice.fPresentationURL);
    Memory::AccumulateIf (&merged.fAttachedNetworkInterfaces, priorityDevice.fAttachedNetworkInterfaces);
    Memory::CopyToIf (&merged.fOperatingSystem, priorityDevice.fOperatingSystem);
    Memory::AccumulateIf (&merged.fAggregatesReversibly, priorityDevice.fAggregatesReversibly);
    Memory::AccumulateIf (&merged.fAggregatesIrreversibly, priorityDevice.fAggregatesIrreversibly);
    Memory::CopyToIf (&merged.fIDPersistent, priorityDevice.fIDPersistent);
    Memory::CopyToIf (&merged.fUserOverrides, priorityDevice.fUserOverrides); // for now, no need to look inside and accumulate because only one place can generate user-overrides - some special TBD database record - LGP 2022-09-14
#if qDebug
    if (priorityDevice.fDebugProps) {
        // copy sub-elements of debug props
        Mapping<String, VariantValue> newProps = NullCoalesce (merged.fDebugProps);
        for (auto i : *priorityDevice.fDebugProps) {
            newProps.Add (i);
        }
        merged.fDebugProps = newProps;
    }
#endif
    return merged;
}

Device Device::Rollup (const Device& rollupDevice, const Device& instanceDevice2Add)
{
    // Use seen.Ever() to decide which 'device' gets precedence in merging. Give the most
    // recent device precedence
    Device d = (rollupDevice.fSeen.EverSeen () == nullopt or
                rollupDevice.fSeen.EverSeen ()->GetUpperBound () < instanceDevice2Add.fSeen.EverSeen ()->GetUpperBound ())
                   ? Merge (rollupDevice, instanceDevice2Add)
                   : Merge (instanceDevice2Add, rollupDevice);
    d.fID    = rollupDevice.fID; // regardless of dates, keep the rollupDevice GUID
    if (d.fAggregatesReversibly.has_value ()) {
        d.fAggregatesReversibly->Add (instanceDevice2Add.fID);
    }
    else {
        d.fAggregatesReversibly = Set<GUID>{instanceDevice2Add.fID};
    }
    d.fAggregatesIrreversibly = nullopt;
    d.fIDPersistent           = false;
    return d;
}

/*
 ********************************************************************************
 ******************* DeviceSortParameters::SearchTerm ***************************
 ********************************************************************************
 */
String DeviceSortParameters::SearchTerm::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "by: "sv << Characters::ToString (fBy) << ", "sv;
    if (fAscending) {
        sb << "ascending: "sv << fAscending;
    }
    sb << "}"sv;
    return sb;
}

/*
 ********************************************************************************
 *************************** DeviceSortParameters *******************************
 ********************************************************************************
 */
String DeviceSortParameters::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "searchTerms: "sv << fSearchTerms << ", "sv;
    if (fCompareNetwork) {
        sb << "compareNetwork: "sv << fCompareNetwork;
    }
    sb << "}"sv;
    return sb;
}

const ObjectVariantMapper DeviceSortParameters::kMapper = [] () {
    ObjectVariantMapper mapper;

    mapper.AddCommonType<DeviceSortParameters::SearchTerm::By> ();
    mapper.AddCommonType<optional<bool>> ();
    mapper.AddCommonType<optional<String>> ();
    mapper.AddClass<DeviceSortParameters::SearchTerm> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"by"sv, StructFieldMetaInfo{&DeviceSortParameters::SearchTerm::fBy}},
        {"ascending"sv, StructFieldMetaInfo{&DeviceSortParameters::SearchTerm::fAscending}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<Sequence<DeviceSortParameters::SearchTerm>> ();
    mapper.AddClass<DeviceSortParameters> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"searchTerms"sv, StructFieldMetaInfo{&DeviceSortParameters::fSearchTerms}},
        {"compareNetwork"sv, StructFieldMetaInfo{&DeviceSortParameters::fCompareNetwork}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    return mapper;
}();

/*
 ********************************************************************************
 ****************************** Model::Operations *******************************
 ********************************************************************************
 */
const ObjectVariantMapper Operations::kMapper = [] () {
    ObjectVariantMapper mapper;

    mapper.AddCommonType<optional<String>> ();
    mapper.AddCommonType<Sequence<double>> ();
    mapper.AddCommonType<Time::Duration> ();
    mapper.AddClass<Operations::TraceRouteResults::Hop> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"timeToHop"sv, StructFieldMetaInfo{&Operations::TraceRouteResults::Hop::fTime}},
        {"address"sv, StructFieldMetaInfo{&Operations::TraceRouteResults::Hop::fAddress}},
    });
    mapper.AddCommonType<Sequence<Operations::TraceRouteResults::Hop>> ();
    mapper.AddClass<Operations::TraceRouteResults> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"hops"sv, StructFieldMetaInfo{&Operations::TraceRouteResults::fHops}},
    });
    mapper.AddClass<Operations::DNSLookupResults> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"result"sv, StructFieldMetaInfo{&Operations::DNSLookupResults::fResult}},
        {"lookup-time"sv, StructFieldMetaInfo{&Operations::DNSLookupResults::fLookupTime}},
    });
    return mapper;
}();

/*
 ********************************************************************************
 ********************************* Model::About *********************************
 ********************************************************************************
 */
String About::APIServerInfo::ComponentInfo::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "Name: "sv << fName << ", "sv;
    sb << "Version: "sv << fVersion << ", "sv;
    sb << "URL: "sv << fURL << ", "sv;
    sb << "}"sv;
    return sb;
}

String About::APIServerInfo::CurrentMachine::ToString () const
{
    StringBuilder sb;
    sb << "{";
    sb << "Operating-System: " << fOperatingSystem << ", "sv;
    sb << "Machine-Uptime: " << fMachineUptime << ", "sv;
    sb << "Total-CPU-Usage: " << fTotalCPUUsage << ", "sv;
    sb << "Run-Q-Length: " << fRunQLength << ", "sv;
    sb << "}"sv;
    return sb;
}

String About::APIServerInfo::CurrentProcess::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "ProcessUptime: "sv << fProcessUptime << ", "sv;
    sb << "AverageCPUTimeUsed: "sv << fAverageCPUTimeUsed << ", "sv;
    sb << "WorkingOrResidentSetSize: "sv << fWorkingOrResidentSetSize << ", "sv;
    sb << "CombinedIOReadRate: "sv << fCombinedIOReadRate << ", "sv;
    sb << "CombinedIOWriteRate: "sv << fCombinedIOWriteRate;
    sb << "}"sv;
    return sb;
}

String About::APIServerInfo::APIEndpoint::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "CallsCompleted: "sv << fCallsCompleted << ", "sv;
    sb << "Errors: "sv << fErrors << ", "sv;
    sb << "MedianDuration: "sv << fMedianDuration << ", "sv;
    sb << "MeanDuration: "sv << fMeanDuration << ", "sv;
    sb << "MaxDuration: "sv << fMaxDuration << ", "sv;
    sb << "MedianWebServerConnections: "sv << fMedianWebServerConnections << ", "sv;
    sb << "MedianProcessingWebServerConnections: "sv << fMedianProcessingWebServerConnections << ", "sv;
    sb << "MedianRunningAPITasks: "sv << fMedianRunningAPITasks;
    sb << "}"sv;
    return sb;
}

String About::APIServerInfo::WebServer::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "ThreadPool: {"sv;
    sb << "Threads: "sv << fThreadPool.fThreads << ", "sv;
    sb << "TasksStillQueued: "sv << fThreadPool.fTasksStillQueued << ", "sv;
    sb << "AverageTaskRunTime: "sv << fThreadPool.fAverageTaskRunTime;
    sb << "}"sv;
    return sb;
}

String About::APIServerInfo::Database::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "Reads: "sv << fReads << ", "sv;
    sb << "Writes: "sv << fWrites << ", "sv;
    sb << "Errors: "sv << fErrors << ", "sv;
    sb << "MeanReadDuration: "sv << fMeanReadDuration << ", "sv;
    sb << "MeanReadDuration: "sv << fMedianReadDuration << ", "sv;
    sb << "MeanWriteDuration: "sv << fMeanWriteDuration << ", "sv;
    sb << "MeanWriteDuration: "sv << fMedianWriteDuration << ", "sv;
    sb << "MaxDuration: "sv << fMaxDuration << ", "sv;
    sb << "FileSize: "sv << fFileSize;
    sb << "}"sv;
    return sb;
}

String About::APIServerInfo::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "Version: "sv << fVersion << ", "sv;
    sb << "Component-Versions: "sv << fComponentVersions << ", "sv;
    sb << "Current-Machine: "sv << fCurrentMachine << ", "sv;
    sb << "Current-Process: "sv << fCurrentProcess << ", "sv;
    sb << "API-Endpoint: "sv << fAPIEndpoint << ", "sv;
    sb << "WebServer: "sv << fWebServer << ", "sv;
    sb << "Database: "sv << fDatabase;
    sb << "}"sv;
    return sb;
}

String About::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "Overall-Application-Version: "sv << fOverallApplicationVersion << ", "sv;
    sb << "API-Server-Info: "sv << fAPIServerInfo;
    sb << "}"sv;
    return sb;
}

const ObjectVariantMapper About::kMapper = [] () {
    ObjectVariantMapper mapper;

    mapper += OperatingSystem::kMapper;

    mapper.AddCommonType<optional<double>> ();

    mapper.Add<Configuration::Version> (
        [] ([[maybe_unused]] const ObjectVariantMapper& mapper, const Configuration::Version* obj) -> VariantValue {
            return obj->AsPrettyVersionString ();
        },
        [] ([[maybe_unused]] const ObjectVariantMapper& mapper, const VariantValue& d, Configuration::Version* intoObj) -> void {
            *intoObj = Configuration::Version::FromPrettyVersionString (d.As<String> ());
        });

    mapper.AddClass<About::APIServerInfo::ComponentInfo> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"name"sv, StructFieldMetaInfo{&About::APIServerInfo::ComponentInfo::fName}},
        {"version"sv, StructFieldMetaInfo{&About::APIServerInfo::ComponentInfo::fVersion}},
        {"URL"sv, StructFieldMetaInfo{&About::APIServerInfo::ComponentInfo::fURL}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<Sequence<About::APIServerInfo::ComponentInfo>> ();

    mapper.AddClass<About::APIServerInfo::CurrentMachine> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"operatingSystem"sv, StructFieldMetaInfo{&About::APIServerInfo::CurrentMachine::fOperatingSystem}},
        {"machineUptime"sv, StructFieldMetaInfo{&About::APIServerInfo::CurrentMachine::fMachineUptime}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"totalCPUUsage"sv, StructFieldMetaInfo{&About::APIServerInfo::CurrentMachine::fTotalCPUUsage}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"runQLength"sv, StructFieldMetaInfo{&About::APIServerInfo::CurrentMachine::fRunQLength}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });

    mapper.AddClass<About::APIServerInfo::CurrentProcess> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"processUptime"sv, StructFieldMetaInfo{&About::APIServerInfo::CurrentProcess::fProcessUptime}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"averageCPUTimeUsed"sv, StructFieldMetaInfo{&About::APIServerInfo::CurrentProcess::fAverageCPUTimeUsed},
         ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"workingOrResidentSetSize"sv, StructFieldMetaInfo{&About::APIServerInfo::CurrentProcess::fWorkingOrResidentSetSize},
         ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"combinedIOReadRate"sv, StructFieldMetaInfo{&About::APIServerInfo::CurrentProcess::fCombinedIOReadRate},
         ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"combinedIOWriteRate"sv, StructFieldMetaInfo{&About::APIServerInfo::CurrentProcess::fCombinedIOWriteRate},
         ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });

    mapper.AddCommonType<optional<float>> ();
    mapper.AddClass<About::APIServerInfo::APIEndpoint> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"callsCompleted"sv, StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fCallsCompleted}},
        {"errors"sv, StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fErrors}},
        {"medianDuration"sv, StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fMedianDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"meanDuration"sv, StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fMeanDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"maxDuration"sv, StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fMaxDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"medianWebServerConnections"sv, StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fMedianWebServerConnections},
         ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"medianProcessingWebServerConnections"sv, StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fMedianProcessingWebServerConnections},
         ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"medianRunningAPITasks"sv, StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fMedianRunningAPITasks},
         ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<optional<About::APIServerInfo::APIEndpoint>> ();

    mapper.AddClass<About::APIServerInfo::WebServer::ThreadPool> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"threads"sv, StructFieldMetaInfo{&About::APIServerInfo::WebServer::ThreadPool::fThreads}},
        {"tasksStillQueued"sv, StructFieldMetaInfo{&About::APIServerInfo::WebServer::ThreadPool::fTasksStillQueued}},
        {"averageTaskRunTime"sv, StructFieldMetaInfo{&About::APIServerInfo::WebServer ::ThreadPool ::fAverageTaskRunTime}},
    });

    mapper.AddClass<About::APIServerInfo::WebServer> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"threadPool"sv, StructFieldMetaInfo{&About::APIServerInfo::WebServer::fThreadPool}},
    });
    mapper.AddCommonType<optional<About::APIServerInfo::WebServer>> ();

    mapper.AddClass<About::APIServerInfo::Database> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"reads"sv, StructFieldMetaInfo{&About::APIServerInfo::Database::fReads}},
        {"writes"sv, StructFieldMetaInfo{&About::APIServerInfo::Database::fWrites}},
        {"errors"sv, StructFieldMetaInfo{&About::APIServerInfo::Database::fErrors}},
        {"meanReadDuration"sv, StructFieldMetaInfo{&About::APIServerInfo::Database::fMeanReadDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"medianReadDuration"sv, StructFieldMetaInfo{&About::APIServerInfo::Database::fMedianReadDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"meanWriteDuration"sv, StructFieldMetaInfo{&About::APIServerInfo::Database::fMeanWriteDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"medianWriteDuration"sv, StructFieldMetaInfo{&About::APIServerInfo::Database::fMedianWriteDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"maxDuration"sv, StructFieldMetaInfo{&About::APIServerInfo::Database::fMaxDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {"fileSize"sv, StructFieldMetaInfo{&About::APIServerInfo::Database::fFileSize}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<optional<About::APIServerInfo::Database>> ();

    mapper.AddClass<About::APIServerInfo> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"version"sv, StructFieldMetaInfo{&About::APIServerInfo::fVersion}},
        {"componentVersions"sv, StructFieldMetaInfo{&About::APIServerInfo::fComponentVersions}},
        {"currentMachine"sv, StructFieldMetaInfo{&About::APIServerInfo::fCurrentMachine}},
        {"currentProcess"sv, StructFieldMetaInfo{&About::APIServerInfo::fCurrentProcess}},
        {"apiEndpoint"sv, StructFieldMetaInfo{&About::APIServerInfo::fAPIEndpoint}},
        {"webServer"sv, StructFieldMetaInfo{&About::APIServerInfo::fWebServer}},
        {"database"sv, StructFieldMetaInfo{&About::APIServerInfo::fDatabase}},
    });

    mapper.AddClass<About> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {"applicationVersion"sv, StructFieldMetaInfo{&About::fOverallApplicationVersion}},
        {"serverInfo"sv, StructFieldMetaInfo{&About::fAPIServerInfo}},
    });

    return mapper;
}();
