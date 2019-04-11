/*
* Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Writer.h"
#include "Stroika/Foundation/IO/Network/CIDR.h"
#include "Stroika/Foundation/IO/Network/InternetAddress.h"

#include "Model.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::DataExchange;
using namespace Stroika::Foundation::IO::Network;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model;

#if qCompilerAndStdLib_lambda_expand_in_namespace_Buggy
const ObjectVariantMapper klambda_expand_in_namespace_Buggy_workaround_Mapper_ = [] () {
    using IO::Network::CIDR;
    ObjectVariantMapper mapper;
    mapper.Add<CIDR> ([] (const ObjectVariantMapper& mapper, const CIDR* obj) -> VariantValue { return obj->ToString (); },
                      [] (const ObjectVariantMapper& mapper, const VariantValue& d, CIDR* intoObj) -> void { *intoObj = CIDR{d.As<String> ()}; });
    return mapper;
}();
#endif

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
    sb += L"fFullVersionedOSName: " + Characters::ToString (fFullVersionedOSName) + L", ";
    sb += L"}";
    return sb.str ();
}

DISABLE_COMPILER_MSC_WARNING_START (4573);
DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
const ObjectVariantMapper OperatingSystem::kMapper = [] () {
    ObjectVariantMapper mapper;

    mapper.AddClass<OperatingSystem> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"fullVersionedName", Stroika_Foundation_DataExchange_StructFieldMetaInfo (OperatingSystem, fFullVersionedOSName), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });

    return mapper;
}();
DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
DISABLE_COMPILER_MSC_WARNING_END (4573);

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

DISABLE_COMPILER_MSC_WARNING_START (4573);
DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
const ObjectVariantMapper Network::kMapper = [] () {
    ObjectVariantMapper mapper;

    using IO::Network::CIDR;
#if qCompilerAndStdLib_lambda_expand_in_namespace_Buggy
    mapper.Add (klambda_expand_in_namespace_Buggy_workaround_Mapper_);
#else
    mapper.Add<CIDR> ([] (const ObjectVariantMapper& mapper, const CIDR* obj) -> VariantValue { return obj->ToString (); },
                      [] (const ObjectVariantMapper& mapper, const VariantValue& d, CIDR* intoObj) -> void { *intoObj = CIDR{d.As<String> ()}; });
#endif
    mapper.AddCommonType<Set<CIDR>> ();
    mapper.AddCommonType<InternetAddress> ();
    mapper.AddCommonType<Sequence<InternetAddress>> ();
    mapper.AddCommonType<optional<InternetAddress>> ();
    mapper.AddCommonType<optional<Sequence<InternetAddress>>> ();
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

    mapper.AddClass<Common::InternetServiceProvider> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"name", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Common::InternetServiceProvider, name), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<optional<Common::InternetServiceProvider>> ();

    mapper.AddClass<Common::GEOLocationInformation> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"countryCode"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Common::GEOLocationInformation, fCountryCode), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"city"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Common::GEOLocationInformation, fCity), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"regionCode"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Common::GEOLocationInformation, fRegionCode), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"postalCode"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Common::GEOLocationInformation, fPostalCode), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"coordinates"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Common::GEOLocationInformation, fLatitudeAndLongitude), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<optional<Common::GEOLocationInformation>> ();

    mapper.AddClass<Network> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"friendlyName"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fFriendlyName), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"networkAddresses"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fNetworkAddresses)},
        {L"attachedInterfaces"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fAttachedInterfaces)},
        {L"gateways"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fGateways)},
        {L"DNSServers"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fDNSServers)},
        {L"externalAddresses"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fExternalAddresses), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"geographicLocation"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fGEOLocInformation), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"internetServiceProvider"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fInternetServiceProvider), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"id"sv, Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fGUID)},
    });
    mapper.AddCommonType<Sequence<Network>> ();

    return mapper;
}();
DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
DISABLE_COMPILER_MSC_WARNING_END (4573);

/*
 ********************************************************************************
 **************************** Model::NetworkInterface ***************************
 ********************************************************************************
 */
Model::NetworkInterface::NetworkInterface (const IO::Network::Interface& src)
    : Interface (src)
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

DISABLE_COMPILER_MSC_WARNING_START (4573);
DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
const ObjectVariantMapper NetworkInterface::kMapper = [] () {
    ObjectVariantMapper mapper;

    mapper.AddCommonType<NetworkInterface::Type> ();
    mapper.AddCommonType<optional<NetworkInterface::Type>> ();
    mapper.AddCommonType<InternetAddress> ();
    mapper.AddCommonType<optional<InternetAddress>> ();
    mapper.AddCommonType<Sequence<InternetAddress>> ();
    mapper.AddCommonType<optional<Sequence<InternetAddress>>> ();

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
    mapper.AddClass<NetworkInterface::Binding> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"internetAddress", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::Binding, fInternetAddress)},
        {L"onLinkPrefixLength", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::Binding, fOnLinkPrefixLength), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<Collection<NetworkInterface::Binding>> ();
    mapper.AddCommonType<NetworkInterface::Status> ();
    mapper.AddCommonType<Set<NetworkInterface::Status>> ();
    mapper.AddCommonType<optional<Set<NetworkInterface::Status>>> ();
    mapper.AddClass<NetworkInterface> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"platformInterfaceID", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fInternalInterfaceID)},
        {L"id", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fGUID)},
        {L"friendlyName", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fFriendlyName)},
        {L"description", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fDescription), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        // fNetworkGUID INTENTIONALLY OMITTED because doesn't correspond to our network ID, misleading, and unhelpful
        {L"type", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fType)},
        {L"hardwareAddress", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fHardwareAddress)},
        {L"transmitSpeedBaud", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fTransmitSpeedBaud), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"receiveLinkSpeedBaud", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fReceiveLinkSpeedBaud), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"bindings", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fBindings)},
        {L"gateways", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fGateways), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"DNSServers", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fDNSServers), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"wirelessInformation", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fWirelessInfo), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"status", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fStatus)},
    });
    mapper.AddCommonType<Collection<NetworkInterface>> ();

    return mapper;
}();
DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
DISABLE_COMPILER_MSC_WARNING_END (4573);

/*
 ********************************************************************************
 ************************************* Device ***********************************
 ********************************************************************************
 */

DISABLE_COMPILER_MSC_WARNING_START (4573);
DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
const ObjectVariantMapper Device::kMapper = [] () {
    ObjectVariantMapper mapper;

    //mapper += OperatingSystem::kMapper;
    mapper.Add (OperatingSystem::kMapper);
    mapper.AddCommonType<optional<OperatingSystem>> ();

    mapper.AddCommonType<InternetAddress> ();
    mapper.AddCommonType<optional<InternetAddress>> ();
    mapper.AddCommonType<Sequence<InternetAddress>> ();
    mapper.AddCommonType<Set<GUID>> ();
    mapper.AddCommonType<optional<Set<GUID>>> ();
    mapper.AddCommonType<Device::DeviceType> ();
    mapper.AddCommonType<Set<Device::DeviceType>> ();
    mapper.AddCommonType<optional<Set<Device::DeviceType>>> ();
    mapper.AddCommonType<optional<String>> ();
    mapper.AddCommonType<optional<float>> ();
    mapper.AddCommonType<Collection<String>> ();
    mapper.AddCommonType<Sequence<String>> ();
    mapper.AddCommonType<URL> ();
    mapper.AddCommonType<optional<URL>> ();

    mapper.AddClass<Device> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"id", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, fGUID)},
        {L"name", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, name)},
        {L"internetAddresses", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, ipAddresses)},
        {L"type", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, fTypes), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"attachedNetworks", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, fAttachedNetworks)},
        {L"attachedNetworkInterfaces", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, fAttachedNetworkInterfaces), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"presentationURL", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, fPresentationURL), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"operatingSystem", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, fOperatingSystem), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });

    mapper.AddCommonType<Collection<Device>> ();
    return mapper;
}();
DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
DISABLE_COMPILER_MSC_WARNING_END (4573);

String Device::ToString () const
{
    return DataExchange::Variant::JSON::Writer ().WriteAsString (Device::kMapper.FromObject (*this));
}

/*
 ********************************************************************************
 ********************************** Model::Operations ***************************
 ********************************************************************************
 */

DISABLE_COMPILER_MSC_WARNING_START (4573);
DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
const ObjectVariantMapper Operations::kMapper = [] () {
    ObjectVariantMapper mapper;

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

    return mapper;
}();
DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
DISABLE_COMPILER_MSC_WARNING_END (4573);

/*
 ********************************************************************************
 ********************************* Model::About *********************************
 ********************************************************************************
 */
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

DISABLE_COMPILER_MSC_WARNING_START (4573);
DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
const ObjectVariantMapper About::kMapper = [] () {
    ObjectVariantMapper mapper;

    //mapper += OperatingSystem::kMapper;
    mapper.Add (OperatingSystem::kMapper);

    mapper.Add<Configuration::Version> (
        [] (const ObjectVariantMapper& mapper, const Configuration::Version* obj) -> VariantValue { return obj->AsPrettyVersionString (); },
        [] (const ObjectVariantMapper& mapper, const VariantValue& d, Configuration::Version* intoObj) -> void { *intoObj = Configuration::Version::FromPrettyVersionString (d.As<String> ()); });
    mapper.AddCommonType<Mapping<String, Configuration::Version>> ();
    mapper.AddClass<About> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"applicationVersion", Stroika_Foundation_DataExchange_StructFieldMetaInfo (About, fOverallApplicationVersion)},
        {L"componentVersions", Stroika_Foundation_DataExchange_StructFieldMetaInfo (About, fComponentVersions)},
        {L"operatingSystem", Stroika_Foundation_DataExchange_StructFieldMetaInfo (About, fOperatingSystem)},
    });

    return mapper;
}();
DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
DISABLE_COMPILER_MSC_WARNING_END (4573);
