/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/Containers/Set.h"
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

using Stroika::Foundation::Common::GUID;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model;

namespace Stroika::Foundation::DataExchange {
    template <>
    CIDR ObjectVariantMapper::ToObject (const ToObjectMapperType<CIDR>& toObjectMapper, const VariantValue& v) const
    {
        CIDR tmp{InternetAddress{}, 0};
        ToObject (toObjectMapper, v, &tmp);
        return tmp;
    }
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
    sb += L"fullVersionedOSName: " + Characters::ToString (fFullVersionedOSName) + L", ";
    sb += L"}";
    return sb.str ();
}

const ObjectVariantMapper OperatingSystem::kMapper = [] () {
    ObjectVariantMapper mapper;

    mapper.AddClass<OperatingSystem> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
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
 ********************************** Model::Network ******************************
 ********************************************************************************
 */
Network Network::Merge (const Network& databaseNetwork, const Network& priorityNetwork)
{
    Network merged = databaseNetwork;
    // name from DB takes precedence
    merged.fNetworkAddresses.AddAll (priorityNetwork.fNetworkAddresses);
    Memory::CopyToIf (&merged.fFriendlyName, priorityNetwork.fFriendlyName);
    merged.fAttachedInterfaces.AddAll (priorityNetwork.fAttachedInterfaces);
    priorityNetwork.fGateways.Apply ([&] (auto inetAddr) {  if (not merged.fGateways.Contains (inetAddr)) {merged.fGateways += inetAddr;} });
    priorityNetwork.fDNSServers.Apply ([&] (auto inetAddr) {  if (not merged.fDNSServers.Contains (inetAddr)) {merged.fDNSServers += inetAddr;} });
    Memory::AccumulateIf (&merged.fExternalAddresses, priorityNetwork.fExternalAddresses);
    Memory::CopyToIf (&merged.fGEOLocInformation, priorityNetwork.fGEOLocInformation);
    Memory::CopyToIf (&merged.fInternetServiceProvider, priorityNetwork.fInternetServiceProvider);
    Memory::CopyToIf (&merged.fLastSeenAt, priorityNetwork.fLastSeenAt);
    Memory::AccumulateIf (&merged.fAggregatesReversibly, priorityNetwork.fAggregatesReversibly); // @todo consider if this is right way to combine
    Memory::AccumulateIf (&merged.fAggregatesIrreversibly, priorityNetwork.fAggregatesIrreversibly);
    Memory::CopyToIf (&merged.fIDPersistent, priorityNetwork.fIDPersistent);
    Memory::CopyToIf (&merged.fHistoricalSnapshot, priorityNetwork.fHistoricalSnapshot);
#if qDebug
    Memory::CopyToIf (&merged.fDebugProps, priorityNetwork.fDebugProps);
#endif
    return merged;
}

Network Network::Rollup (const Network& rollupNetwork, const Network& instanceNetwork2Add)
{
    Network n = Merge (rollupNetwork, instanceNetwork2Add);

    // @todo make this smarter about which device/info is newest, so we know what takes precedence
    // but for now using fLastSeenAt as indicator; later - use globally (within WTF) consistent sequence # (not sure how todo)
    // or maybe use UTC datetime? - but alway want to consistently compare and prefer if no ties so we can consistent merges
    bool preferRHS = rollupNetwork.fLastSeenAt <= instanceNetwork2Add.fLastSeenAt;

    Memory::CopyToIf (&n.fLastSeenAt, preferRHS ? instanceNetwork2Add.fLastSeenAt : rollupNetwork.fLastSeenAt);

    if (n.fAggregatesReversibly.has_value ()) {
        n.fAggregatesReversibly->Add (instanceNetwork2Add.fGUID);
    }
    else {
        n.fAggregatesReversibly = Set<GUID>{instanceNetwork2Add.fGUID};
    }
    n.fAggregatesIrreversibly = nullopt;
    n.fIDPersistent           = false;
    n.fHistoricalSnapshot     = false;
    return n;
}

String Network::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"Network-Addresses: " + Characters::ToString (fNetworkAddresses) + L", ";
    sb += L"Friendly-Name: " + Characters::ToString (fFriendlyName) + L", ";
    sb += L"GUID: " + Characters::ToString (fGUID) + L", ";
    sb += L"Attached-Interfaces: " + Characters::ToString (fAttachedInterfaces) + L", ";
    sb += L"Gateways: " + Characters::ToString (fGateways) + L", ";
    sb += L"DNS-Servers: " + Characters::ToString (fDNSServers) + L", ";
    sb += L"LastSeenAt: " + Characters::ToString (fLastSeenAt) + L", ";
    sb += L"Aggregates-Reversibly: " + Characters::ToString (fAggregatesReversibly) + L", ";
    sb += L"Aggregates-Irreverisbly: " + Characters::ToString (fAggregatesIrreversibly) + L", ";
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

    if (true) {
        // looks better as an object, than as an array
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

    mapper.AddClass<Network> (initializer_list<ObjectVariantMapper::StructFieldInfo> {
        {L"friendlyName"sv, StructFieldMetaInfo{&Network::fFriendlyName}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"networkAddresses"sv, StructFieldMetaInfo{&Network::fNetworkAddresses}},
            {L"attachedInterfaces"sv, StructFieldMetaInfo{&Network::fAttachedInterfaces}},
            {L"gateways"sv, StructFieldMetaInfo{&Network::fGateways}},
            {L"DNSServers"sv, StructFieldMetaInfo{&Network::fDNSServers}},
            {L"externalAddresses"sv, StructFieldMetaInfo{&Network::fExternalAddresses}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"geographicLocation"sv, StructFieldMetaInfo{&Network::fGEOLocInformation}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"internetServiceProvider"sv, StructFieldMetaInfo{&Network::fInternetServiceProvider}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"id"sv, StructFieldMetaInfo{&Network::fGUID}},
            {L"lastSeenAt"sv, StructFieldMetaInfo{&Network::fLastSeenAt}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"aggregatesReversibly"sv, StructFieldMetaInfo{&Network::fAggregatesReversibly}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"aggregatesIrreversibly"sv, StructFieldMetaInfo{&Network::fAggregatesIrreversibly}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"idIsPersistent"sv, StructFieldMetaInfo{&Network::fIDPersistent}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"historicalSnapshot"sv, StructFieldMetaInfo{&Network::fHistoricalSnapshot}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
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
    sb += L"fGUID: " + Characters::ToString (fGUID) + L", ";
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
    mapper.AddClass<NetworkAttachmentInfo> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"hardwareAddresses", StructFieldMetaInfo{&NetworkAttachmentInfo::hardwareAddresses}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"localAddresses", StructFieldMetaInfo{&NetworkAttachmentInfo::localAddresses}},
    });
    mapper.AddCommonType<Mapping<GUID, NetworkAttachmentInfo>> ();
    mapper.AddClass<Device> (initializer_list<ObjectVariantMapper::StructFieldInfo> {
        {L"id", StructFieldMetaInfo{&Device::fGUID}},
            {L"name", StructFieldMetaInfo{&Device::name}},
            {L"type", StructFieldMetaInfo{&Device::fTypes}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"lastSeenAt", StructFieldMetaInfo{&Device::fLastSeenAt}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
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

Device Device::Merge (const Device& databaseDevice, const Device& priorityDevice)
{
    Device merged = databaseDevice;
    // name from databaseDevice takes precedence
    Memory::AccumulateIf (&merged.fTypes, priorityDevice.fTypes);
    Memory::CopyToIf (&merged.fIcon, priorityDevice.fIcon);
    Memory::CopyToIf (&merged.fLastSeenAt, priorityDevice.fLastSeenAt);
    Memory::CopyToIf (&merged.fManufacturer, priorityDevice.fManufacturer);
    merged.fAttachedNetworks.AddAll (priorityDevice.fAttachedNetworks); // @todo perhaps should MERGE these details...
    Memory::AccumulateIf (&merged.fOpenPorts, priorityDevice.fOpenPorts);
    Memory::CopyToIf (&merged.fPresentationURL, priorityDevice.fPresentationURL);
    Memory::AccumulateIf (&merged.fAttachedNetworkInterfaces, priorityDevice.fAttachedNetworkInterfaces);
    Memory::CopyToIf (&merged.fOperatingSystem, priorityDevice.fOperatingSystem);
    Memory::AccumulateIf (&merged.fAggregatesReversibly, priorityDevice.fAggregatesReversibly); // @todo UNSURE if this is right
    Memory::AccumulateIf (&merged.fAggregatesIrreversibly, priorityDevice.fAggregatesIrreversibly);
    Memory::CopyToIf (&merged.fIDPersistent, priorityDevice.fIDPersistent);
    Memory::CopyToIf (&merged.fHistoricalSnapshot, priorityDevice.fHistoricalSnapshot);
#if qDebug
    Memory::CopyToIf (&merged.fDebugProps, priorityDevice.fDebugProps);
#endif
    return merged;
}

Device Device::Rollup (const Device& rollupDevice, const Device& instanceDevice2Add)
{
    Device d = Merge (rollupDevice, instanceDevice2Add);

    // @todo make this smarter about which device/info is newest, so we know what takes precedence
    // but for now using fLastSeenAt as indicator; later - use globally (within WTF) consistent sequence # (not sure how todo)
    // or maybe use UTC datetime? - but alway want to consistently compare and prefer if no ties so we can consistent merges
    bool preferRHS = rollupDevice.fLastSeenAt <= instanceDevice2Add.fLastSeenAt;

    Memory::CopyToIf (&d.fLastSeenAt, preferRHS ? instanceDevice2Add.fLastSeenAt : rollupDevice.fLastSeenAt);

    if (d.fAggregatesReversibly.has_value ()) {
        d.fAggregatesReversibly->Add (instanceDevice2Add.fGUID);
    }
    else {
        d.fAggregatesReversibly = Set<GUID>{instanceDevice2Add.fGUID};
    }
    d.fAggregatesIrreversibly = nullopt;
    d.fIDPersistent           = false;
    d.fHistoricalSnapshot     = false;
    // for rollup, names can come in any order, and dont pick last, pick best; fix so smarter!!! @todo --LGP 2021-08-30
    // @todo respect preferRHS
    if (not instanceDevice2Add.name.empty () and instanceDevice2Add.name[0].IsAlphabetic ()) {
        d.name = instanceDevice2Add.name;
    }
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
    sb += L"fProcessUptime: " + Characters::ToString (fProcessUptime) + L", ";
    sb += L"fAverageCPUTimeUsed: " + Characters::ToString (fAverageCPUTimeUsed) + L", ";
    sb += L"fWorkingOrResidentSetSize: " + Characters::ToString (fWorkingOrResidentSetSize) + L", ";
    sb += L"fCombinedIOReadRate: " + Characters::ToString (fCombinedIOReadRate) + L", ";
    sb += L"fCombinedIOWriteRate: " + Characters::ToString (fCombinedIOWriteRate) + L", ";
    sb += L"}";
    return sb.str ();
}

String About::APIServerInfo::APIEndpoint::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"fCallsCompleted: " + Characters::ToString (fCallsCompleted) + L", ";
    sb += L"fMedianDuration: " + Characters::ToString (fMedianDuration) + L", ";
    sb += L"fMeanDuration: " + Characters::ToString (fMeanDuration) + L", ";
    sb += L"fMaxDuration: " + Characters::ToString (fMaxDuration) + L", ";
    sb += L"}";
    return sb.str ();
}

String About::APIServerInfo::Database::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"fReads: " + Characters::ToString (fReads) + L", ";
    sb += L"fWrites: " + Characters::ToString (fWrites) + L", ";
    sb += L"fErrors: " + Characters::ToString (fErrors) + L", ";
    sb += L"fMeanReadDuration: " + Characters::ToString (fMeanReadDuration) + L", ";
    sb += L"fMeanReadDuration: " + Characters::ToString (fMedianReadDuration) + L", ";
    sb += L"fMeanWriteDuration: " + Characters::ToString (fMeanWriteDuration) + L", ";
    sb += L"fMeanWriteDuration: " + Characters::ToString (fMedianWriteDuration) + L", ";
    sb += L"fMaxDuration: " + Characters::ToString (fMaxDuration) + L", ";
    sb += L"fFileSize: " + Characters::ToString (fFileSize) + L", ";
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

    mapper.AddClass<About::APIServerInfo::APIEndpoint> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"callsCompleted", StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fCallsCompleted}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"medianDuration", StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fMedianDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"meanDuration", StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fMeanDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"maxDuration", StructFieldMetaInfo{&About::APIServerInfo::APIEndpoint::fMaxDuration}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
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
