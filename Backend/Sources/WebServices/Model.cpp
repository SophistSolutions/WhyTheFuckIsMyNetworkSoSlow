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
    const ObjectVariantMapper::TypeMappingDetails kDateRangeMapper_ = ObjectVariantMapper::MakeCommonSerializer<Range<DateTime>> (ObjectVariantMapper::RangeSerializerOptions{L"lowerBound"sv, L"upperBound"sv});

#if kStroika_Version_FullVersion <= Stroika_Make_FULL_VERSION(2, 1, kStroika_Version_Stage_Release, 5, 1)
    const ObjectVariantMapper::TypeMappingDetails kOptionalDateRangeMapper_ = [] () {
        using T                                                                 = Range<DateTime>;
        ObjectVariantMapper::FromObjectMapperType<optional<T>> fromObjectMapper = [] (const ObjectVariantMapper& mapper, const optional<T>* fromObjOfTypeT) -> VariantValue {
            RequireNotNull (fromObjOfTypeT);
            if (fromObjOfTypeT->has_value ()) {
                return kDateRangeMapper_.FromObjectMapper<T> () (mapper, &**fromObjOfTypeT);
            }
            else {
                return VariantValue{};
            }
        };
        ObjectVariantMapper::ToObjectMapperType<optional<T>> toObjectMapper = [] (const ObjectVariantMapper& mapper, const VariantValue& d, optional<T>* intoObjOfTypeT) -> void {
            RequireNotNull (intoObjOfTypeT);
            if (d.GetType () == VariantValue::eNull) {
                *intoObjOfTypeT = nullopt;
            }
            else {
                // SEE https://stroika.atlassian.net/browse/STK-910
                // fix here - I KNOW I have something there, but how to construct
                T tmp;
                kDateRangeMapper_.ToObjectMapper<T> () (mapper, d, &tmp);
                *intoObjOfTypeT = tmp;
            }
        };
        return ObjectVariantMapper::TypeMappingDetails{typeid (optional<T>), fromObjectMapper, toObjectMapper};
    }();
#else
    const ObjectVariantMapper::TypeMappingDetails kOptionalDateRangeMapper_ = ObjectVariantMapper::MakeCommonSerializer<optional<Range<DateTime>>> (ObjectVariantMapper::OptionalSerializerOptions{kDateRangeMapper_});
#endif
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
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"majorOSCategory: " + Characters::ToString (fMajorOSCategory) + L", ";
    sb += L"fullVersionedOSName: " + Characters::ToString (fFullVersionedOSName) + L", ";
    sb += L"}";
    return sb.str ();
}

const ObjectVariantMapper OperatingSystem::kMapper = [] () {
    ObjectVariantMapper mapper;
    mapper.AddClass<OperatingSystem> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"majorOSCategory", StructFieldMetaInfo{&OperatingSystem::fMajorOSCategory}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"fullVersionedName", StructFieldMetaInfo{&OperatingSystem::fFullVersionedOSName}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
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
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"shortName: " + Characters::ToString (fShortName) + L", ";
    sb += L"fullName: " + Characters::ToString (fFullName) + L", ";
    sb += L"webSiteURL: " + Characters::ToString (fWebSiteURL) + L", ";
    sb += L"}";
    return sb.str ();
}

const ObjectVariantMapper Model::Manufacturer::kMapper = [] () {
    ObjectVariantMapper mapper;
    mapper.AddCommonType<optional<String>> ();
    mapper.AddCommonType<URI> ();
    mapper.AddCommonType<optional<URI>> ();
    mapper.AddClass<Manufacturer> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"shortName", StructFieldMetaInfo{&Manufacturer::fShortName}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"fullName", StructFieldMetaInfo{&Manufacturer::fFullName}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"webSiteURL", StructFieldMetaInfo{&Manufacturer::fWebSiteURL}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    return mapper;
}();

/*
 ********************************************************************************
 ********************** Model::Network::UserOverridesType ***********************
 ********************************************************************************
 */
String Model::Network::UserOverridesType::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    if (fName) {
        sb += L"Name: " + Characters::ToString (fName) + L", ";
    }
    if (fTags) {
        sb += L"Tags: " + Characters::ToString (fTags) + L", ";
    }
    if (fNotes) {
        sb += L"Notes: " + Characters::ToString (fNotes);
    }
    if (fAggregateNetworks) {
        sb += L"AggregateNetworks: " + Characters::ToString (fAggregateNetworks);
    }
    if (fDontAggregateNetworks) {
        sb += L"DontAggregateNetworks: " + Characters::ToString (fDontAggregateNetworks);
    }
    if (fAggregateFingerprint) {
        sb += L"AggregateFingerprint: " + Characters::ToString (fAggregateFingerprint);
    }
    if (fDontAggregateFingerprint) {
        sb += L"DontAggregateFingerprint: " + Characters::ToString (fDontAggregateFingerprint);
    }
    if (fAggregateGatewayHardwareAddresses) {
        sb += L"AggregateGatewayHardwareAddresses: " + Characters::ToString (fAggregateGatewayHardwareAddresses);
    }
    sb += L"}";
    return sb.str ();
}

const DataExchange::ObjectVariantMapper Model::Network::UserOverridesType::kMapper = [] () {
    ObjectVariantMapper mapper;
    mapper.AddCommonType<optional<String>> ();
    mapper.AddCommonType<Set<String>> ();
    mapper.AddCommonType<optional<Set<String>>> ();
    mapper.AddCommonType<Set<GUID>> ();
    mapper.AddCommonType<optional<Set<GUID>>> ();
    mapper.AddClass<UserOverridesType> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"name"sv, StructFieldMetaInfo{&UserOverridesType::fName}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"tags"sv, StructFieldMetaInfo{&UserOverridesType::fTags}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"notes"sv, StructFieldMetaInfo{&UserOverridesType::fNotes}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"aggregateNetworks"sv, StructFieldMetaInfo{&UserOverridesType::fAggregateNetworks}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"dontAggregateNetworks"sv, StructFieldMetaInfo{&UserOverridesType::fDontAggregateNetworks}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"aggregateFingerprint"sv, StructFieldMetaInfo{&UserOverridesType::fAggregateFingerprint}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"dontAggregateFingerprint"sv, StructFieldMetaInfo{&UserOverridesType::fDontAggregateFingerprint}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"aggregateGatewayHardwareAddresses"sv, StructFieldMetaInfo{&UserOverridesType::fAggregateGatewayHardwareAddresses}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
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
    merged.fGUID   = priorityNetwork.fGUID;
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
    Memory::CopyToIf (&merged.fHistoricalSnapshot, priorityNetwork.fHistoricalSnapshot);
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
    // Use seen.Ever() to decide which 'device' gets precedence in merging. Give the most
    // recent device precedence
    Network merged = (rollupNetwork.fSeen.empty () or rollupNetwork.fSeen.GetUpperBound () < instanceNetwork2Add.fSeen.GetUpperBound ())
                         ? Merge (rollupNetwork, instanceNetwork2Add)
                         : Merge (instanceNetwork2Add, rollupNetwork);
    merged.fGUID   = rollupNetwork.fGUID; // regardless of dates, keep the rollupDevice GUID
    if (merged.fAggregatesReversibly.has_value ()) {
        merged.fAggregatesReversibly->Add (instanceNetwork2Add.fGUID);
    }
    else {
        merged.fAggregatesReversibly = Set<GUID>{instanceNetwork2Add.fGUID};
    }
    merged.fAggregatesIrreversibly = nullopt;
    merged.fIDPersistent           = false;
    merged.fHistoricalSnapshot     = false;
    Memory::CopyToIf (&merged.fUserOverrides, instanceNetwork2Add.fUserOverrides); // for now, no need to look inside and accumulate because only one place can generate user-overrides - some special TBD database record - LGP 2022-09-14
    return merged;
}

Network::FingerprintType Network::GenerateFingerprintFromProperties () const
{
    // LOGIC UP TIL 2022-09-25
#if 0
            /*
             *  A network is not a super-well defined concept, so deciding if two instances of a network refer to the same
             *  network is a bit of a judgement call.
             * 
             *  BUt a few key things probably make sense:
             *      >   Same ISP
             *      >   Same GeoLoc (with exceptions)
             *      >   Same IPv4 CIDR
             *      >   Same Gateway addresses
             * 
             *  Things we allow to differ:
             *      >   details of any IP-v6 network addresses (if there were IPV4 CIDRs agreed upon).
             * 
             *  At least thats by best guess to start as of 2021-08-29
             */
#endif
    auto useCIDRs = [] (const Set<CIDR>& ias) {
        // for some reason, gateway list sometimes contains IPv4 only, and sometimes IPv4 and IPv6 addresses
        // treat the list the same if the gateway list ipv4s at least are the same (and non-empty)
        // if IPv4 CIDRs same (and non-empty), then ignore differences in IPv4 addressses
        Set<CIDR> ipv4s{ias.Where ([] (const CIDR& i) { return i.GetBaseInternetAddress ().GetAddressFamily () == InternetAddress::AddressFamily::V4; })};

        // If we got here, they differ in IPv6 (or other) address. If they matched on IPV4 (not trivially - because there were none)
        // ignore the (ipv6) differences
        return ipv4s.empty () ? ias : ipv4s;
    };
    auto useAddresses = [] (const Set<InternetAddress>& ias) {
        // for some reason, gateway list sometimes contains IPv4 only, and sometimes IPv4 and IPv6 addresses
        // treat the list the same if the gateway list ipv4s at least are the same (and non-empty)
        // if IPv4 CIDRs same (and non-empty), then ignore differences in IPv4 addressses
        Set<InternetAddress> ipv4s{ias.Where ([] (const InternetAddress& i) { return i.GetAddressFamily () == InternetAddress::AddressFamily::V4; })};

        // If we got here, they differ in IPv6 (or other) address. If they matched on IPV4 (not trivially - because there were none)
        // ignore the (ipv6) differences
        return ipv4s.empty () ? ias : ipv4s;
    };

    // combine hardare addresses of gateway with gateway address, and external ip address (and hash)
    // into a single ID that probably mostly uniquely ids a network.

    // note - if external id floats, this will change, and it will look like a new network. NOT SURE best to include
    // network id, but often a good idea, and not hard to add records allowing combine of networks (more of a PITA than
    // other way where we didn't include and had to force separate).
    StringBuilder sb;
    if (fExternalAddresses and fExternalAddresses->size () > 1) {
        for (const InternetAddress& i : SortedSet<InternetAddress>{useAddresses (*fExternalAddresses)}) {
            sb += i.As<String> ();
        }
    }
    else if (fExternalAddresses and fExternalAddresses->size () == 1) {
        for (const InternetAddress& i : *fExternalAddresses) {
            sb += i.As<String> ();
        }
    }
    sb += L"/";
    for (const CIDR& i : useCIDRs (fNetworkAddresses)) {
        sb += i.As<String> ();
    }
    sb += L"/";
    if (fGateways.size () > 1) {
        for (const InternetAddress& i : SortedSet<InternetAddress>{useAddresses (fGateways)}) {
            sb += i.As<String> ();
        }
    }
    else {
        for (const InternetAddress& i : fGateways) {
            sb += i.As<String> ();
        }
    }
    sb += L"/";
    if (fGatewayHardwareAddresses.size () > 1) {
        for (const String& i : SortedSet<String>{fGatewayHardwareAddresses}) {
            sb += i;
        }
    }
    else {
        for (const String& i : fGatewayHardwareAddresses) {
            sb += i;
        }
    }
#if kStroika_Version_FullVersion <= Stroika_Make_FULL_VERSION(2, 1, kStroika_Version_Stage_Release, 5, 1)
    return Cryptography::Digest::ComputeDigest<Cryptography::Digest::Algorithm::MD5> ((const std::byte*)sb.begin (), (const std::byte*)sb.end ());
#else
    // Could use other algorithms, but easiest to stick with MD5 for compat with 2.1.5 Stroika
    return Cryptography::Digest::ComputeDigest<Cryptography::Digest::Algorithm::MD5> (sb.str ());
#endif
}

String Network::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"Network-Addresses: " + Characters::ToString (fNetworkAddresses) + L", ";
    sb += L"Names: " + Characters::ToString (fNames) + L", ";
    sb += L"GUID: " + Characters::ToString (fGUID) + L", ";
    sb += L"Attached-Interfaces: " + Characters::ToString (fAttachedInterfaces) + L", ";
    sb += L"Gateways: " + Characters::ToString (fGateways) + L", ";
    sb += L"GatewayHardwareAddresses: " + Characters::ToString (fGatewayHardwareAddresses) + L", ";
    sb += L"DNS-Servers: " + Characters::ToString (fDNSServers) + L", ";
    sb += L"Seen: " + Characters::ToString (fSeen) + L", ";
    sb += L"Aggregates-Reversibly: " + Characters::ToString (fAggregatesReversibly) + L", ";
    sb += L"Aggregates-Irreverisbly: " + Characters::ToString (fAggregatesIrreversibly) + L", ";
    sb += L"Aggregates-Fingerprints: " + Characters::ToString (fAggregatesFingerprints) + L", ";
    sb += L"IDPersistent: " + Characters::ToString (fIDPersistent) + L", ";
    sb += L"HistoricalSnapshot: " + Characters::ToString (fHistoricalSnapshot) + L", ";
    sb += L"}";
    return sb.str ();
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
            {L"latitude"sv, StructFieldMetaInfo{&laglon_::lat}},
            {L"longitude"sv, StructFieldMetaInfo{&laglon_::lon}},
        });
        mapper.Add<tuple<float, float>> (
            [] (const ObjectVariantMapper& mapper, const tuple<float, float>* obj) -> VariantValue { return mapper.FromObject (laglon_{get<0> (*obj), get<1> (*obj)}); },
            [] (const ObjectVariantMapper& mapper, const VariantValue& d, tuple<float, float>* intoObj) -> void { auto tmp{ mapper.ToObject<laglon_> (d) }; *intoObj = make_tuple (tmp.lat, tmp.lon); });
    }
    else {
        mapper.AddCommonType<tuple<float, float>> (); // works but represents as an array
    }

    mapper.AddCommonType<optional<tuple<float, float>>> ();

    mapper.AddClass<InternetServiceProvider> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"name", StructFieldMetaInfo{&InternetServiceProvider::name}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<optional<InternetServiceProvider>> ();

    mapper.AddClass<GEOLocationInformation> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"countryCode"sv, StructFieldMetaInfo{&GEOLocationInformation::fCountryCode}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"city"sv, StructFieldMetaInfo{&GEOLocationInformation::fCity}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"regionCode"sv, StructFieldMetaInfo{&GEOLocationInformation::fRegionCode}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"postalCode"sv, StructFieldMetaInfo{&GEOLocationInformation::fPostalCode}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"coordinates"sv, StructFieldMetaInfo{&GEOLocationInformation::fLatitudeAndLongitude}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<optional<GEOLocationInformation>> ();

    mapper += UserOverridesType::kMapper;
    mapper.AddCommonType<optional<UserOverridesType>> ();

    mapper.AddClass<Network> (initializer_list<ObjectVariantMapper::StructFieldInfo> {
        {L"names"sv, StructFieldMetaInfo{&Network::fNames}},
            {L"networkAddresses"sv, StructFieldMetaInfo{&Network::fNetworkAddresses}},
            {L"attachedInterfaces"sv, StructFieldMetaInfo{&Network::fAttachedInterfaces}},
            {L"gateways"sv, StructFieldMetaInfo{&Network::fGateways}},
            {L"gatewayHardwareAddresses"sv, StructFieldMetaInfo{&Network::fGatewayHardwareAddresses}},
            {L"DNSServers"sv, StructFieldMetaInfo{&Network::fDNSServers}},
            {L"externalAddresses"sv, StructFieldMetaInfo{&Network::fExternalAddresses}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"geographicLocation"sv, StructFieldMetaInfo{&Network::fGEOLocInformation}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"internetServiceProvider"sv, StructFieldMetaInfo{&Network::fInternetServiceProvider}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"id"sv, StructFieldMetaInfo{&Network::fGUID}},
            {L"seen"sv, StructFieldMetaInfo{&Network::fSeen}, kDateRangeMapper_},
            {L"aggregatesReversibly"sv, StructFieldMetaInfo{&Network::fAggregatesReversibly}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"aggregatesIrreversibly"sv, StructFieldMetaInfo{&Network::fAggregatesIrreversibly}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"aggregatesFingerprints"sv, StructFieldMetaInfo{&Network::fAggregatesFingerprints}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"idIsPersistent"sv, StructFieldMetaInfo{&Network::fIDPersistent}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"historicalSnapshot"sv, StructFieldMetaInfo{&Network::fHistoricalSnapshot}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"userOverrides"sv, StructFieldMetaInfo{&Network::fUserOverrides}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
#if qDebug
            {L"debugProps", StructFieldMetaInfo{&Network::fDebugProps}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
#endif
    });
    mapper.AddCommonType<Sequence<Network>> ();

    return mapper;
}();

/*
 ********************************************************************************
 **************************** Model::NetworkInterface ***************************
 ********************************************************************************
 */
Model::NetworkInterface::NetworkInterface (const IO::Network::Interface& src)
    : Interface{src}
{
}

String NetworkInterface::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"GUID: " + Characters::ToString (fGUID) + L", ";
    sb += Interface::ToString ().SafeSubString (1, -1);
    sb += L"}";
    return sb.str ();
}

const ObjectVariantMapper NetworkInterface::kMapper = [] () {
    ObjectVariantMapper mapper;

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
        {L"SSID", StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fSSID}},
        {L"state", StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fState}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"connectionMode", StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fConnectionMode}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"profileName", StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fProfileName}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"BSSType", StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fBSSType}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"MACAddress", StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fMACAddress}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"physicalConnectionType", StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fPhysicalConnectionType}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"signalQuality", StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fSignalQuality}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"securityEnabled", StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fSecurityEnabled}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"802.1XEnabled", StructFieldMetaInfo{&NetworkInterface::WirelessInfo::f8021XEnabled}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"authAlgorithm", StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fAuthAlgorithm}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"cipher", StructFieldMetaInfo{&NetworkInterface::WirelessInfo::fCipher}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<optional<NetworkInterface::WirelessInfo>> ();
    mapper.AddCommonType<NetworkInterface::Status> ();
    mapper.AddCommonType<Set<NetworkInterface::Status>> ();
    mapper.AddCommonType<optional<Set<NetworkInterface::Status>>> ();

    {
        mapper.AddClass<NetworkInterface> (initializer_list<ObjectVariantMapper::StructFieldInfo> {
            {L"platformInterfaceID", StructFieldMetaInfo{&NetworkInterface::fInternalInterfaceID}},
                {L"id", StructFieldMetaInfo{&NetworkInterface::fGUID}},
                {L"friendlyName", StructFieldMetaInfo{&NetworkInterface::fFriendlyName}},
                {L"description", StructFieldMetaInfo{&NetworkInterface::fDescription}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
                // fNetworkGUID INTENTIONALLY OMITTED because doesn't correspond to our network ID, misleading, and unhelpful
                {L"type", StructFieldMetaInfo{&NetworkInterface::fType}},
                {L"hardwareAddress", StructFieldMetaInfo{&NetworkInterface::fHardwareAddress}},
                {L"transmitSpeedBaud", StructFieldMetaInfo{&NetworkInterface::fTransmitSpeedBaud}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
                {L"receiveLinkSpeedBaud", StructFieldMetaInfo{&NetworkInterface::fReceiveLinkSpeedBaud}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
                //SEE OVERRIDE BELOW {L"boundAddressRanges", StructFieldMetaInfo{&NetworkInterface::fBindings.fAddressRanges}},
                //SEE OVERRIDE BELOW {L"boundAddresses", StructFieldMetaInfo{&NetworkInterface::fBindings.fAddresses}},
                {L"gateways", StructFieldMetaInfo{&NetworkInterface::fGateways}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
                {L"DNSServers", StructFieldMetaInfo{&NetworkInterface::fDNSServers}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
                {L"wirelessInformation", StructFieldMetaInfo{&NetworkInterface::fWirelessInfo}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
                {L"status", StructFieldMetaInfo{&NetworkInterface::fStatus}},
#if qDebug
                {L"debugProps", StructFieldMetaInfo{&NetworkInterface::fDebugProps}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
#endif
        });
        // StructFieldMetaInfo{} doesn't work with nested members - https://stackoverflow.com/questions/1929887/is-pointer-to-inner-struct-member-forbidden
        ObjectVariantMapper::TypeMappingDetails originalTypeMapper = *mapper.GetTypeMappingRegistry ().Lookup (typeid (NetworkInterface));
        mapper.Add<NetworkInterface> (
            // Do base mappings, and map
            //      {L"boundAddressRanges", StructFieldMetaInfo{offsetof (NetworkInterface, fBindings.fAddressRanges), typeid (NetworkInterface::fBindings.fAddressRanges)}},
            //      {L"boundAddresses", StructFieldMetaInfo{offsetof (NetworkInterface, fBindings.fAddresses), typeid (NetworkInterface::fBindings.fAddressRanges)}},
            [=] (const ObjectVariantMapper& mapper, const NetworkInterface* obj) -> VariantValue {
                Mapping<String, VariantValue> resultMap = originalTypeMapper.fFromObjectMapper (mapper, obj).As<Mapping<String, VariantValue>> ();
                resultMap.Add (L"boundAddressRanges", mapper.FromObject (obj->fBindings.fAddressRanges));
                resultMap.Add (L"boundAddresses", mapper.FromObject (obj->fBindings.fAddresses));
                return VariantValue{resultMap};
            },
            [=] (const ObjectVariantMapper& mapper, const VariantValue& d, NetworkInterface* intoObj) -> void {
                originalTypeMapper.fToObjectMapper (mapper, d, intoObj);
                Mapping<String, VariantValue> fromMap = d.As<Mapping<String, VariantValue>> ();
                if (auto o = fromMap.Lookup (L"boundAddressRanges")) {
                    intoObj->fBindings.fAddressRanges = mapper.ToObject<Containers::Collection<CIDR>> (*o);
                }
                if (auto o = fromMap.Lookup (L"boundAddresses")) {
                    intoObj->fBindings.fAddresses = mapper.ToObject<Containers::Collection<InternetAddress>> (*o);
                }
            });
    }
    mapper.AddCommonType<Collection<NetworkInterface>> ();
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
    sb += L"{";
    sb += L"hardwareAddresses: " + Characters::ToString (hardwareAddresses) + L", ";
    sb += L"localAddresses: " + Characters::ToString (localAddresses) + L", ";
    sb += L"}";
    return sb.str ();
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
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"ARP: " + Characters::ToString (fARP) + L", ";

    sb += L"Collector: " + Characters::ToString (fCollector) + L", ";
    sb += L"ICMP: " + Characters::ToString (fICMP) + L", ";
    sb += L"TCP: " + Characters::ToString (fTCP) + L", ";
    sb += L"UDP: " + Characters::ToString (fUDP) + L", ";
    sb += L"}";
    return sb.str ();
}

const DataExchange::ObjectVariantMapper Model::Device::SeenType::kMapper = [] () {
    ObjectVariantMapper mapper;
    mapper.AddCommonType<Traversal::Range<DateTime>> ();
    mapper.AddClass<SeenType> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"ARP"sv, StructFieldMetaInfo{&SeenType::fARP}, kOptionalDateRangeMapper_, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"Collector"sv, StructFieldMetaInfo{&SeenType::fCollector}, kOptionalDateRangeMapper_, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"ICMP"sv, StructFieldMetaInfo{&SeenType::fICMP}, kOptionalDateRangeMapper_, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"UDP"sv, StructFieldMetaInfo{&SeenType::fUDP}, kOptionalDateRangeMapper_, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"TCP"sv, StructFieldMetaInfo{&SeenType::fTCP}, kOptionalDateRangeMapper_, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    return mapper;
}();

/*
 ********************************************************************************
 ********************** Model::Device::UserOverridesType ************************
 ********************************************************************************
 */
String Model::Device::UserOverridesType::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    if (fName) {
        sb += L"Name: " + Characters::ToString (fName) + L", ";
    }
    if (fTags) {
        sb += L"Tags: " + Characters::ToString (fTags) + L", ";
    }
    if (fNotes) {
        sb += L"Notes: " + Characters::ToString (fNotes);
    }
    if (fAggregateDevices) {
        sb += L"AggregateDevices: " + Characters::ToString (fAggregateDevices);
    }
    if (fDontAggregateDevices) {
        sb += L"DontAggregateDevices: " + Characters::ToString (fDontAggregateDevices);
    }
    if (fAggregateDeviceHardwareAddresses) {
        sb += L"AggregateDeviceHardwareAddresses: " + Characters::ToString (fAggregateDeviceHardwareAddresses);
    }
    sb += L"}";
    return sb.str ();
}

const DataExchange::ObjectVariantMapper Model::Device::UserOverridesType::kMapper = [] () {
    ObjectVariantMapper mapper;
    mapper.AddCommonType<optional<String>> ();
    mapper.AddCommonType<Set<String>> ();
    mapper.AddCommonType<optional<Set<String>>> ();
    mapper.AddCommonType<Set<GUID>> ();
    mapper.AddCommonType<optional<Set<GUID>>> ();
    mapper.AddClass<UserOverridesType> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"name"sv, StructFieldMetaInfo{&UserOverridesType::fName}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"tags"sv, StructFieldMetaInfo{&UserOverridesType::fTags}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"notes"sv, StructFieldMetaInfo{&UserOverridesType::fNotes}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"aggregateDevices"sv, StructFieldMetaInfo{&UserOverridesType::fAggregateDevices}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"dontAggregateDevices"sv, StructFieldMetaInfo{&UserOverridesType::fDontAggregateDevices}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"aggregateDeviceHardwareAddresses"sv, StructFieldMetaInfo{&UserOverridesType::fAggregateDeviceHardwareAddresses}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
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
        {L"hardwareAddresses", StructFieldMetaInfo{&NetworkAttachmentInfo::hardwareAddresses}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"localAddresses", StructFieldMetaInfo{&NetworkAttachmentInfo::localAddresses}},
    });
    mapper.AddCommonType<Mapping<GUID, NetworkAttachmentInfo>> ();

    mapper.AddClass<Device> (initializer_list<ObjectVariantMapper::StructFieldInfo> {
        {L"id", StructFieldMetaInfo{&Device::fGUID}},

#if 0
            //tmphack list name/names for bacward compat, then just names simple way
            {L"name", StructFieldMetaInfo{&Device::fNames},
             ObjectVariantMapper::TypeMappingDetails{
                 typeid (Common::PrioritizedNames),
                 ObjectVariantMapper::FromObjectMapperType<Common::PrioritizedNames>{[] (const ObjectVariantMapper&, const Common::PrioritizedNames* objOfType) -> VariantValue { return VariantValue{objOfType->GetName ()}; }},
                 ObjectVariantMapper::ToObjectMapperType<Common::PrioritizedNames>{[] (const ObjectVariantMapper&, const VariantValue& d, Common::PrioritizedNames* into) -> void {
                     if (not d.As<String> ().empty ()) {
                         into->Add (d.As<String> (), 1);
                     } 
                 }}}},
            {L"names", StructFieldMetaInfo{&Device::fNames},
             ObjectVariantMapper::TypeMappingDetails{
                 typeid (Common::PrioritizedNames),
                 ObjectVariantMapper::FromObjectMapperType<Common::PrioritizedNames>{[] (const ObjectVariantMapper& mapper, const Common::PrioritizedNames* fromObjOfTypeT) -> VariantValue {
                     RequireNotNull (fromObjOfTypeT);
                     Sequence<VariantValue> s;
                     if (not fromObjOfTypeT->empty ()) {
                         using T = Common::PrioritizedName;
                         ObjectVariantMapper::FromObjectMapperType<T> valueMapper{mapper.FromObjectMapper<T> ()};
                         for (const auto& i : *fromObjOfTypeT) {
                             s.Append (mapper.FromObject<T> (valueMapper, i));
                         }
                     }
                     return VariantValue{s};
                 }},
                 ObjectVariantMapper::ToObjectMapperType<Common::PrioritizedNames>{[] (const ObjectVariantMapper& mapper, const VariantValue& d, Common::PrioritizedNames* intoObjOfTypeT) -> void {
                     RequireNotNull (intoObjOfTypeT);
                     // Require (intoObjOfTypeT->empty ()); override to avoid this and use Add
                     Sequence<VariantValue> s = d.As<Sequence<VariantValue>> ();
                     if (not s.empty ()) {
                         ObjectVariantMapper::ToObjectMapperType<Common::PrioritizedName> valueMapper{mapper.ToObjectMapper<Common::PrioritizedName> ()};
                         for (const auto& i : s) {
                             auto obj2Add = mapper.ToObject<Common::PrioritizedName> (valueMapper, i);
                             if (not obj2Add.fName.empty ()) {
                                 intoObjOfTypeT->Add (obj2Add.fName, obj2Add.fPriority);
                             }
                         }
                     }
                 }}}},
#endif
            {L"names", StructFieldMetaInfo{&Device::fNames}},
            {L"type", StructFieldMetaInfo{&Device::fTypes}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"seen", StructFieldMetaInfo{&Device::fSeen}},
            {L"openPorts", StructFieldMetaInfo{&Device::fOpenPorts}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"icon", StructFieldMetaInfo{&Device::fIcon}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"manufacturer", StructFieldMetaInfo{&Device::fManufacturer}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"attachedNetworks", StructFieldMetaInfo{&Device::fAttachedNetworks}},
            {L"attachedNetworkInterfaces", StructFieldMetaInfo{&Device::fAttachedNetworkInterfaces}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"presentationURL", StructFieldMetaInfo{&Device::fPresentationURL}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"operatingSystem", StructFieldMetaInfo{&Device::fOperatingSystem}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"aggregatesReversibly", StructFieldMetaInfo{&Device::fAggregatesReversibly}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"aggregatesIrreversibly"sv, StructFieldMetaInfo{&Device::fAggregatesIrreversibly}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"idIsPersistent"sv, StructFieldMetaInfo{&Device::fIDPersistent}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"historicalSnapshot"sv, StructFieldMetaInfo{&Device::fHistoricalSnapshot}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"userOverrides"sv, StructFieldMetaInfo{&Device::fUserOverrides}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},

#if qDebug
            {L"debugProps", StructFieldMetaInfo{&Device::fDebugProps}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
#endif
    });
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
    merged.fGUID = priorityDevice.fGUID;
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
    Memory::CopyToIf (&merged.fHistoricalSnapshot, priorityDevice.fHistoricalSnapshot);
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
    Device d = rollupDevice.fSeen.EverSeen ()->GetUpperBound () < instanceDevice2Add.fSeen.EverSeen ()->GetUpperBound ()
                   ? Merge (rollupDevice, instanceDevice2Add)
                   : Merge (instanceDevice2Add, rollupDevice);
    d.fGUID  = rollupDevice.fGUID; // regardless of dates, keep the rollupDevice GUID
    if (d.fAggregatesReversibly.has_value ()) {
        d.fAggregatesReversibly->Add (instanceDevice2Add.fGUID);
    }
    else {
        d.fAggregatesReversibly = Set<GUID>{instanceDevice2Add.fGUID};
    }
    d.fAggregatesIrreversibly = nullopt;
    d.fIDPersistent           = false;
    d.fHistoricalSnapshot     = false;
    return d;
}

/*
 ********************************************************************************
 ******************* DeviceSortParamters::SearchTerm ****************************
 ********************************************************************************
 */
String DeviceSortParamters::SearchTerm::ToString () const
{
    StringBuilder sb;
    sb += L"{";
    sb += L"by: " + Characters::ToString (fBy) + L", ";
    if (fAscending) {
        sb += L"ascending: " + Characters::ToString (fAscending) + L", ";
    }
    sb += L"}";
    return sb.str ();
}

/*
 ********************************************************************************
 *************************** DeviceSortParamters ********************************
 ********************************************************************************
 */
String DeviceSortParamters::ToString () const
{
    StringBuilder sb;
    sb += L"{";
    sb += L"searchTerms: " + Characters::ToString (fSearchTerms) + L", ";
    if (fCompareNetwork) {
        sb += L"compareNetwork: " + Characters::ToString (fCompareNetwork) + L", ";
    }
    sb += L"}";
    return sb.str ();
}

const ObjectVariantMapper DeviceSortParamters::kMapper = [] () {
    ObjectVariantMapper mapper;

    mapper.AddCommonType<DeviceSortParamters::SearchTerm::By> ();
    mapper.AddCommonType<optional<bool>> ();
    mapper.AddCommonType<optional<String>> ();
    mapper.AddClass<DeviceSortParamters::SearchTerm> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"by", StructFieldMetaInfo{&DeviceSortParamters::SearchTerm::fBy}},
        {L"ascending", StructFieldMetaInfo{&DeviceSortParamters::SearchTerm::fAscending}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<Sequence<DeviceSortParamters::SearchTerm>> ();
    mapper.AddClass<DeviceSortParamters> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"searchTerms", StructFieldMetaInfo{&DeviceSortParamters::fSearchTerms}},
        {L"compareNetwork", StructFieldMetaInfo{&DeviceSortParamters::fCompareNetwork}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    return mapper;
}();

/*
 ********************************************************************************
 ****************************** Model::Operations *******************************
 ********************************************************************************
 */
#if qCompilerAndStdLib_static_initializer_lambda_funct_init_Buggy
namespace {
    ObjectVariantMapper mkMapper_ ()
    {
        ObjectVariantMapper mapper;

        mapper.AddCommonType<optional<String>> ();
        mapper.AddCommonType<Sequence<double>> ();
        mapper.AddCommonType<Time::Duration> ();
        mapper.AddClass<Operations::TraceRouteResults::Hop> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
            {L"timeToHop", StructFieldMetaInfo{&Operations::TraceRouteResults::Hop::fTime}},
            {L"address", StructFieldMetaInfo{&Operations::TraceRouteResults::Hop::fAddress}},
        });
        mapper.AddCommonType<Sequence<Operations::TraceRouteResults::Hop>> ();
        mapper.AddClass<Operations::TraceRouteResults> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
            {L"hops", StructFieldMetaInfo{&Operations::TraceRouteResults::fHops}},
        });
        mapper.AddClass<Operations::DNSLookupResults> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
            {L"result", StructFieldMetaInfo{&Operations::DNSLookupResults::fResult}},
            {L"lookup-time", StructFieldMetaInfo{&Operations::DNSLookupResults::fLookupTime}},
        });
        return mapper;
    }
}
const ObjectVariantMapper Operations::kMapper = mkMapper_ ();
#else
const ObjectVariantMapper Operations::kMapper = [] () {
    ObjectVariantMapper mapper;

    mapper.AddCommonType<optional<String>> ();
    mapper.AddCommonType<Sequence<double>> ();
    mapper.AddCommonType<Time::Duration> ();
    mapper.AddClass<Operations::TraceRouteResults::Hop> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"timeToHop", StructFieldMetaInfo{&Operations::TraceRouteResults::Hop::fTime}},
        {L"address", StructFieldMetaInfo{&Operations::TraceRouteResults::Hop::fAddress}},
    });
    mapper.AddCommonType<Sequence<Operations::TraceRouteResults::Hop>> ();
    mapper.AddClass<Operations::TraceRouteResults> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"hops", StructFieldMetaInfo{&Operations::TraceRouteResults::fHops}},
    });
    mapper.AddClass<Operations::DNSLookupResults> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"result", StructFieldMetaInfo{&Operations::DNSLookupResults::fResult}},
        {L"lookup-time", StructFieldMetaInfo{&Operations::DNSLookupResults::fLookupTime}},
    });
    return mapper;
}();
#endif

/*
 ********************************************************************************
 ********************************* Model::About *********************************
 ********************************************************************************
 */
String About::APIServerInfo::ComponentInfo::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"Name: " + Characters::ToString (fName) + L", ";
    sb += L"Version: " + Characters::ToString (fVersion) + L", ";
    sb += L"URL: " + Characters::ToString (fURL) + L", ";
    sb += L"}";
    return sb.str ();
}

String About::APIServerInfo::CurrentMachine::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"Operating-System: " + Characters::ToString (fOperatingSystem) + L", ";
    sb += L"Machine-Uptime: " + Characters::ToString (fMachineUptime) + L", ";
    sb += L"Total-CPU-Usage: " + Characters::ToString (fTotalCPUUsage) + L", ";
    sb += L"Run-Q-Length: " + Characters::ToString (fRunQLength) + L", ";
    sb += L"}";
    return sb.str ();
}

String About::APIServerInfo::CurrentProcess::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"ProcessUptime: " + Characters::ToString (fProcessUptime) + L", ";
    sb += L"AverageCPUTimeUsed: " + Characters::ToString (fAverageCPUTimeUsed) + L", ";
    sb += L"WorkingOrResidentSetSize: " + Characters::ToString (fWorkingOrResidentSetSize) + L", ";
    sb += L"CombinedIOReadRate: " + Characters::ToString (fCombinedIOReadRate) + L", ";
    sb += L"CombinedIOWriteRate: " + Characters::ToString (fCombinedIOWriteRate) + L", ";
    sb += L"}";
    return sb.str ();
}

String About::APIServerInfo::APIEndpoint::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"CallsCompleted: " + Characters::ToString (fCallsCompleted) + L", ";
    sb += L"Errors: " + Characters::ToString (fErrors) + L", ";
    sb += L"MedianDuration: " + Characters::ToString (fMedianDuration) + L", ";
    sb += L"MeanDuration: " + Characters::ToString (fMeanDuration) + L", ";
    sb += L"MaxDuration: " + Characters::ToString (fMaxDuration) + L", ";
    sb += L"MedianWebServerConnections: " + Characters::ToString (fMedianWebServerConnections) + L", ";
    sb += L"MedianProcessingWebServerConnections: " + Characters::ToString (fMedianProcessingWebServerConnections) + L", ";
    sb += L"MedianRunningAPITasks: " + Characters::ToString (fMedianRunningAPITasks) + L", ";
    sb += L"}";
    return sb.str ();
}

String About::APIServerInfo::Database::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"Reads: " + Characters::ToString (fReads) + L", ";
    sb += L"Writes: " + Characters::ToString (fWrites) + L", ";
    sb += L"Errors: " + Characters::ToString (fErrors) + L", ";
    sb += L"MeanReadDuration: " + Characters::ToString (fMeanReadDuration) + L", ";
    sb += L"MeanReadDuration: " + Characters::ToString (fMedianReadDuration) + L", ";
    sb += L"MeanWriteDuration: " + Characters::ToString (fMeanWriteDuration) + L", ";
    sb += L"MeanWriteDuration: " + Characters::ToString (fMedianWriteDuration) + L", ";
    sb += L"MaxDuration: " + Characters::ToString (fMaxDuration) + L", ";
    sb += L"FileSize: " + Characters::ToString (fFileSize) + L", ";
    sb += L"}";
    return sb.str ();
}

String About::APIServerInfo::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"Version: " + Characters::ToString (fVersion) + L", ";
    sb += L"Component-Versions: " + Characters::ToString (fComponentVersions) + L", ";
    sb += L"Current-Machine: " + Characters::ToString (fCurrentMachine) + L", ";
    sb += L"Current-Process: " + Characters::ToString (fCurrentProcess) + L", ";
    sb += L"API-Endpoint: " + Characters::ToString (fAPIEndpoint) + L", ";
    sb += L"Database: " + Characters::ToString (fDatabase) + L", ";
    sb += L"}";
    return sb.str ();
}

String About::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"Overall-Application-Version: " + Characters::ToString (fOverallApplicationVersion) + L", ";
    sb += L"API-Server-Info: " + Characters::ToString (fAPIServerInfo) + L", ";
    sb += L"}";
    return sb.str ();
}

const ObjectVariantMapper About::kMapper = [] () {
    ObjectVariantMapper mapper;

    mapper += OperatingSystem::kMapper;

    mapper.AddCommonType<optional<double>> ();

    mapper.Add<Configuration::Version> (
        [] ([[maybe_unused]] const ObjectVariantMapper& mapper, const Configuration::Version* obj) -> VariantValue { return obj->AsPrettyVersionString (); },
        [] ([[maybe_unused]] const ObjectVariantMapper& mapper, const VariantValue& d, Configuration::Version* intoObj) -> void { *intoObj = Configuration::Version::FromPrettyVersionString (d.As<String> ()); });

    mapper.AddClass<About::APIServerInfo::ComponentInfo> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"name", StructFieldMetaInfo{&About::APIServerInfo::ComponentInfo::fName}},
        {L"version", StructFieldMetaInfo{&About::APIServerInfo::ComponentInfo::fVersion}},
        {L"URL", StructFieldMetaInfo{&About::APIServerInfo::ComponentInfo::fURL}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<Sequence<About::APIServerInfo::ComponentInfo>> ();

    mapper.AddClass<About::APIServerInfo::CurrentMachine> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"operatingSystem", StructFieldMetaInfo{&About::APIServerInfo::CurrentMachine::fOperatingSystem}},
        {L"machineUptime", StructFieldMetaInfo{&About::APIServerInfo::CurrentMachine::fMachineUptime}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"totalCPUUsage", StructFieldMetaInfo{&About::APIServerInfo::CurrentMachine::fTotalCPUUsage}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"runQLength", StructFieldMetaInfo{&About::APIServerInfo::CurrentMachine::fRunQLength}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });

    mapper.AddClass<About::APIServerInfo::CurrentProcess> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"processUptime", StructFieldMetaInfo{&About::APIServerInfo::CurrentProcess::fProcessUptime}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"averageCPUTimeUsed", StructFieldMetaInfo{&About::APIServerInfo::CurrentProcess::fAverageCPUTimeUsed}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"workingOrResidentSetSize", StructFieldMetaInfo{&About::APIServerInfo::CurrentProcess::fWorkingOrResidentSetSize}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"combinedIOReadRate", StructFieldMetaInfo{&About::APIServerInfo::CurrentProcess::fCombinedIOReadRate}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"combinedIOWriteRate", StructFieldMetaInfo{&About::APIServerInfo::CurrentProcess::fCombinedIOWriteRate}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });

    mapper.AddCommonType<optional<float>> ();
    mapper.AddClass<About::APIServerInfo::APIEndpoint> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"callsCompleted", StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fCallsCompleted}},
        {L"errors", StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fErrors}},
        {L"medianDuration", StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fMedianDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"meanDuration", StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fMeanDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"maxDuration", StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fMaxDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"medianWebServerConnections", StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fMedianWebServerConnections}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"medianProcessingWebServerConnections", StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fMedianProcessingWebServerConnections}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"medianRunningAPITasks", StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fMedianRunningAPITasks}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });

    mapper.AddCommonType<optional<About::APIServerInfo::APIEndpoint>> ();

    mapper.AddClass<About::APIServerInfo::Database> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"reads", StructFieldMetaInfo{&About::APIServerInfo::Database::fReads}},
        {L"writes", StructFieldMetaInfo{&About::APIServerInfo::Database::fWrites}},
        {L"errors", StructFieldMetaInfo{&About::APIServerInfo::Database::fErrors}},
        {L"meanReadDuration", StructFieldMetaInfo{&About::APIServerInfo::Database::fMeanReadDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"medianReadDuration", StructFieldMetaInfo{&About::APIServerInfo::Database::fMedianReadDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"meanWriteDuration", StructFieldMetaInfo{&About::APIServerInfo::Database::fMeanWriteDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"medianWriteDuration", StructFieldMetaInfo{&About::APIServerInfo::Database::fMedianWriteDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"maxDuration", StructFieldMetaInfo{&About::APIServerInfo::Database::fMaxDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"fileSize", StructFieldMetaInfo{&About::APIServerInfo::Database::fFileSize}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<optional<About::APIServerInfo::Database>> ();

    mapper.AddClass<About::APIServerInfo> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"version", StructFieldMetaInfo{&About::APIServerInfo::fVersion}},
        {L"componentVersions", StructFieldMetaInfo{&About::APIServerInfo::fComponentVersions}},
        {L"currentMachine", StructFieldMetaInfo{&About::APIServerInfo::fCurrentMachine}},
        {L"currentProcess", StructFieldMetaInfo{&About::APIServerInfo::fCurrentProcess}},
        {L"apiEndpoint", StructFieldMetaInfo{&About::APIServerInfo::fAPIEndpoint}},
        {L"database", StructFieldMetaInfo{&About::APIServerInfo::fDatabase}},
    });

    mapper.AddClass<About> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"applicationVersion", StructFieldMetaInfo{&About::fOverallApplicationVersion}},
        {L"serverInfo", StructFieldMetaInfo{&About::fAPIServerInfo}},
    });

    return mapper;
}();
