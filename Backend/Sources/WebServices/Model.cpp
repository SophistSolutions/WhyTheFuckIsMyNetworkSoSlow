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
        CIDR tmp{InternetAddress{}, 32};
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

    DISABLE_COMPILER_MSC_WARNING_START (4573);
    DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    mapper.AddClass<OperatingSystem> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"fullVersionedName", Stroika_Foundation_DataExchange_StructFieldMetaInfo (OperatingSystem, fFullVersionedOSName), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    DISABLE_COMPILER_MSC_WARNING_END (4573);

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
    DISABLE_COMPILER_MSC_WARNING_START (4573);
    DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    ObjectVariantMapper mapper;
    mapper.AddCommonType<optional<String>> ();
    mapper.AddCommonType<URI> ();
    mapper.AddCommonType<optional<URI>> ();
    mapper.AddClass<Manufacturer> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"shortName", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Manufacturer, fShortName), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"fullName", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Manufacturer, fFullName), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"webSiteURL", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Manufacturer, fWebSiteURL), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    DISABLE_COMPILER_MSC_WARNING_END (4573);
    return mapper;
}();

/*
 ********************************************************************************
 ********************************** Model::Network ******************************
 ********************************************************************************
 */
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
    sb += L"}";
    return sb.str ();
}

const ObjectVariantMapper Network::kMapper = [] () {
    using namespace BackendApp::Common;

    ObjectVariantMapper mapper;

    DISABLE_COMPILER_MSC_WARNING_START (4573);
    DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
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

    if (true) {
        // looks better as an object, than as an array
        struct laglon_ {
            float lat;
            float lon;
        };
        mapper.AddClass<laglon_> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
            {L"latitude"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (laglon_, lat)},
            {L"longitude"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (laglon_, lon)},
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
        {L"name", Stroika_Foundation_DataExchange_StructFieldMetaInfo (InternetServiceProvider, name), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<optional<InternetServiceProvider>> ();

    mapper.AddClass<GEOLocationInformation> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"countryCode"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (GEOLocationInformation, fCountryCode), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"city"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (GEOLocationInformation, fCity), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"regionCode"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (GEOLocationInformation, fRegionCode), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"postalCode"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (GEOLocationInformation, fPostalCode), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"coordinates"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (GEOLocationInformation, fLatitudeAndLongitude), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<optional<GEOLocationInformation>> ();

    mapper.AddClass<Network> (initializer_list<ObjectVariantMapper::StructFieldInfo> {
        {L"friendlyName"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fFriendlyName), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"networkAddresses"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fNetworkAddresses)},
            {L"attachedInterfaces"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fAttachedInterfaces)},
            {L"gateways"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fGateways)},
            {L"DNSServers"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fDNSServers)},
            {L"externalAddresses"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fExternalAddresses), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"geographicLocation"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fGEOLocInformation), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"internetServiceProvider"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fInternetServiceProvider), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"id"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fGUID)},
#if qDebug
            {L"debugProps", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fDebugProps), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
#endif
    });
    mapper.AddCommonType<Sequence<Network>> ();
    DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    DISABLE_COMPILER_MSC_WARNING_END (4573);

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

    DISABLE_COMPILER_MSC_WARNING_START (4573);
    DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
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
        {L"SSID", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fSSID)},
        {L"state", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fState), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"connectionMode", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fConnectionMode), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"profileName", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fProfileName), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"BSSType", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fBSSType), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"MACAddress", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fMACAddress), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"physicalConnectionType", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fPhysicalConnectionType), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"signalQuality", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fSignalQuality), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"securityEnabled", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fSecurityEnabled), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"802.1XEnabled", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, f8021XEnabled), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"authAlgorithm", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fAuthAlgorithm), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"cipher", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fCipher), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<optional<NetworkInterface::WirelessInfo>> ();
    mapper.AddCommonType<NetworkInterface::Status> ();
    mapper.AddCommonType<Set<NetworkInterface::Status>> ();
    mapper.AddCommonType<optional<Set<NetworkInterface::Status>>> ();
    mapper.AddClass<NetworkInterface> (initializer_list<ObjectVariantMapper::StructFieldInfo> {
        {L"platformInterfaceID", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fInternalInterfaceID)},
            {L"id", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fGUID)},
            {L"friendlyName", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fFriendlyName)},
            {L"description", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fDescription), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            // fNetworkGUID INTENTIONALLY OMITTED because doesn't correspond to our network ID, misleading, and unhelpful
            {L"type", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fType)},
            {L"hardwareAddress", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fHardwareAddress)},
            {L"transmitSpeedBaud", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fTransmitSpeedBaud), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"receiveLinkSpeedBaud", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fReceiveLinkSpeedBaud), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"boundAddressRanges", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fBindings.fAddressRanges)},
            {L"boundAddresses", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fBindings.fAddresses)},
            {L"gateways", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fGateways), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"DNSServers", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fDNSServers), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"wirelessInformation", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fWirelessInfo), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"status", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fStatus)},
#if qDebug
            {L"debugProps", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fDebugProps), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
#endif
    });
    mapper.AddCommonType<Collection<NetworkInterface>> ();
    DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    DISABLE_COMPILER_MSC_WARNING_END (4573);

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

    DISABLE_COMPILER_MSC_WARNING_START (4573);
    DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
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
    mapper.AddClass<NetworkAttachmentInfo> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"hardwareAddresses", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkAttachmentInfo, hardwareAddresses), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"localAddresses", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkAttachmentInfo, localAddresses)},
    });
    mapper.AddCommonType<Mapping<GUID, NetworkAttachmentInfo>> ();
    mapper.AddClass<Device> (initializer_list<ObjectVariantMapper::StructFieldInfo> {
        {L"id", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, fGUID)},
            {L"name", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, name)},
            {L"type", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, fTypes), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"lastSeenAt", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, fLastSeenAt), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"openPorts", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, fOpenPorts), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"icon", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, fIcon), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"manufacturer", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, fManufacturer), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"attachedNetworks", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, fAttachedNetworks)},
            {L"attachedNetworkInterfaces", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, fAttachedNetworkInterfaces), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"presentationURL", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, fPresentationURL), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
            {L"operatingSystem", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, fOperatingSystem), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
#if qDebug
            {L"debugProps", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, fDebugProps), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
#endif
    });
    mapper.AddCommonType<Sequence<Device>> ();
    DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    DISABLE_COMPILER_MSC_WARNING_END (4573);
    return mapper;
}();

Set<InternetAddress> Device::GetInternetAddresses () const
{
    Set<InternetAddress> result;
    for (auto iNet : fAttachedNetworks) {
        result += iNet.fValue.localAddresses;
    }
    return result;
}

String Device::ToString () const
{
    return DataExchange::Variant::JSON::Writer{}.WriteAsString (Device::kMapper.FromObject (*this));
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

    DISABLE_COMPILER_MSC_WARNING_START (4573);
    DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    mapper.AddCommonType<DeviceSortParamters::SearchTerm::By> ();
    mapper.AddCommonType<optional<bool>> ();
    mapper.AddCommonType<optional<String>> ();
    mapper.AddClass<DeviceSortParamters::SearchTerm> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"by", Stroika_Foundation_DataExchange_StructFieldMetaInfo (DeviceSortParamters::SearchTerm, fBy)},
        {L"ascending", Stroika_Foundation_DataExchange_StructFieldMetaInfo (DeviceSortParamters::SearchTerm, fAscending), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<Sequence<DeviceSortParamters::SearchTerm>> ();
    mapper.AddClass<DeviceSortParamters> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"searchTerms", Stroika_Foundation_DataExchange_StructFieldMetaInfo (DeviceSortParamters, fSearchTerms)},
        {L"compareNetwork", Stroika_Foundation_DataExchange_StructFieldMetaInfo (DeviceSortParamters, fCompareNetwork), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    DISABLE_COMPILER_MSC_WARNING_END (4573);
    return mapper;
}();

/*
 ********************************************************************************
 ********************************** Model::Operations ***************************
 ********************************************************************************
 */
#if qCompilerAndStdLib_static_initializer_lambda_funct_init_Buggy
namespace {
    ObjectVariantMapper mkMapper_ ()
    {
        ObjectVariantMapper mapper;

        DISABLE_COMPILER_MSC_WARNING_START (4573);
        DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
        mapper.AddCommonType<optional<String>> ();
        mapper.AddCommonType<Sequence<double>> ();
        mapper.AddCommonType<Time::Duration> ();
        mapper.AddClass<Operations::TraceRouteResults::Hop> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
            {L"timeToHop", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Operations::TraceRouteResults::Hop, fTime)},
            {L"address", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Operations::TraceRouteResults::Hop, fAddress)},
        });
        mapper.AddCommonType<Sequence<Operations::TraceRouteResults::Hop>> ();
        mapper.AddClass<Operations::TraceRouteResults> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
            {L"hops", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Operations::TraceRouteResults, fHops)},
        });
        mapper.AddClass<Operations::DNSLookupResults> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
            {L"result", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Operations::DNSLookupResults, fResult)},
            {L"lookup-time", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Operations::DNSLookupResults, fLookupTime)},
        });
        DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
        DISABLE_COMPILER_MSC_WARNING_END (4573);
        return mapper;
    }
}
const ObjectVariantMapper Operations::kMapper = mkMapper_ ();
#else
const ObjectVariantMapper Operations::kMapper = [] () {
    ObjectVariantMapper mapper;

    DISABLE_COMPILER_MSC_WARNING_START (4573);
    DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    mapper.AddCommonType<optional<String>> ();
    mapper.AddCommonType<Sequence<double>> ();
    mapper.AddCommonType<Time::Duration> ();
    mapper.AddClass<Operations::TraceRouteResults::Hop> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"timeToHop", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Operations::TraceRouteResults::Hop, fTime)},
        {L"address", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Operations::TraceRouteResults::Hop, fAddress)},
    });
    mapper.AddCommonType<Sequence<Operations::TraceRouteResults::Hop>> ();
    mapper.AddClass<Operations::TraceRouteResults> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"hops", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Operations::TraceRouteResults, fHops)},
    });
    mapper.AddClass<Operations::DNSLookupResults> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"result", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Operations::DNSLookupResults, fResult)},
        {L"lookup-time", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Operations::DNSLookupResults, fLookupTime)},
    });

    DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    DISABLE_COMPILER_MSC_WARNING_END (4573);
    return mapper;
}();
#endif

/*
 ********************************************************************************
 ********************************* Model::About *********************************
 ********************************************************************************
 */
String About::ComponentInfo::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"Name: " + Characters::ToString (fName) + L", ";
    sb += L"Version: " + Characters::ToString (fVersion) + L", ";
    sb += L"URL: " + Characters::ToString (fURL) + L", ";
    sb += L"}";
    return sb.str ();
}

String About::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"Overall-Application-Version: " + Characters::ToString (fOverallApplicationVersion) + L", ";
    sb += L"Component-Versions: " + Characters::ToString (fComponentVersions) + L", ";
    sb += L"Operating-System: " + Characters::ToString (fOperatingSystem) + L", ";
    sb += L"}";
    return sb.str ();
}

const ObjectVariantMapper About::kMapper = [] () {
    ObjectVariantMapper mapper;

    mapper += OperatingSystem::kMapper;

    DISABLE_COMPILER_MSC_WARNING_START (4573);
    DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    mapper.AddClass<About::ComponentInfo> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"name", Stroika_Foundation_DataExchange_StructFieldMetaInfo (About::ComponentInfo, fName)},
        {L"version", Stroika_Foundation_DataExchange_StructFieldMetaInfo (About::ComponentInfo, fVersion)},
        {L"URL", Stroika_Foundation_DataExchange_StructFieldMetaInfo (About::ComponentInfo, fURL), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.Add<Configuration::Version> (
        [] ([[maybe_unused]] const ObjectVariantMapper& mapper, const Configuration::Version* obj) -> VariantValue { return obj->AsPrettyVersionString (); },
        [] ([[maybe_unused]] const ObjectVariantMapper& mapper, const VariantValue& d, Configuration::Version* intoObj) -> void { *intoObj = Configuration::Version::FromPrettyVersionString (d.As<String> ()); });
    mapper.AddCommonType<Sequence<About::ComponentInfo>> ();
    mapper.AddClass<About> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"applicationVersion", Stroika_Foundation_DataExchange_StructFieldMetaInfo (About, fOverallApplicationVersion)},
        {L"componentVersions", Stroika_Foundation_DataExchange_StructFieldMetaInfo (About, fComponentVersions)},
        {L"operatingSystem", Stroika_Foundation_DataExchange_StructFieldMetaInfo (About, fOperatingSystem)},
    });
    DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    DISABLE_COMPILER_MSC_WARNING_END (4573);

    return mapper;
}();
