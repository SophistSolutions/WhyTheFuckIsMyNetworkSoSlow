/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
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
const ObjectVariantMapper klambda_expand_in_namespace_Buggy_workaround_Mapper_ = []() {
    using IO::Network::CIDR;
    ObjectVariantMapper mapper;
    mapper.Add<CIDR> ([](const ObjectVariantMapper& mapper, const CIDR* obj) -> VariantValue { return obj->ToString (); },
                      [](const ObjectVariantMapper& mapper, const VariantValue& d, CIDR* intoObj) -> void { *intoObj = CIDR{d.As<String> ()}; });
    return mapper;
}();
#endif

/*
 ********************************************************************************
 ********************************** Model::Network ******************************
 ********************************************************************************
 */
String Network::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"fNetworkAddress: " + Characters::ToString (fNetworkAddress) + L", ";
    sb += L"fFriendlyName: " + Characters::ToString (fFriendlyName) + L", ";
    sb += L"fGUID: " + Characters::ToString (fGUID) + L", ";
    sb += L"fAttachedInterfaces: " + Characters::ToString (fAttachedInterfaces) + L", ";
    sb += L"fGateways: " + Characters::ToString (fGateways) + L", ";
    sb += L"fDNSServers: " + Characters::ToString (fDNSServers) + L", ";
    sb += L"}";
    return sb.str ();
}

DISABLE_COMPILER_MSC_WARNING_START (4573);
DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
const ObjectVariantMapper Network::kMapper = []() {
    ObjectVariantMapper mapper;

    using IO::Network::CIDR;
#if qCompilerAndStdLib_lambda_expand_in_namespace_Buggy
    mapper.Add (klambda_expand_in_namespace_Buggy_workaround_Mapper_);
#else
    mapper.Add<CIDR> ([](const ObjectVariantMapper& mapper, const CIDR* obj) -> VariantValue { return obj->ToString (); },
                      [](const ObjectVariantMapper& mapper, const VariantValue& d, CIDR* intoObj) -> void { *intoObj = CIDR{d.As<String> ()}; });
#endif

    mapper.AddCommonType<InternetAddress> ();
    mapper.AddCommonType<Sequence<InternetAddress>> ();
    mapper.AddCommonType<Set<Common::GUID>> ();
    mapper.AddClass<Network> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"Friendly-Name", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fFriendlyName), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"Network-Address", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fNetworkAddress)},
        {L"Attached-Interfaces", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fAttachedInterfaces)},
        {L"Gateways", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fGateways)},
        {L"DNS-Servers", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fDNSServers)},
        {L"ID", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fGUID)},
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
const ObjectVariantMapper NetworkInterface::kMapper = []() {
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
        {L"State", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fState), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"Connection-Mode", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fConnectionMode), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"Profile-Name", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fProfileName), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"BSS-Type", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fBSSType), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"MAC-Address", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fMACAddress), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"Physical-Connection-Type", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fPhysicalConnectionType), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"Signal-Quality", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fSignalQuality), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"Security-Enabled", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fSecurityEnabled), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"802.1X-Enabled", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, f8021XEnabled), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"Auth-Algorithm", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fAuthAlgorithm), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"Cipher", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::WirelessInfo, fCipher), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<optional<NetworkInterface::WirelessInfo>> ();
    mapper.AddClass<NetworkInterface::Binding> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"Internet-Address", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::Binding, fInternetAddress)},
        {L"On-Link-Prefix-Length", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface::Binding, fOnLinkPrefixLength), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<Collection<NetworkInterface::Binding>> ();
    mapper.AddCommonType<NetworkInterface::Status> ();
    mapper.AddCommonType<Set<NetworkInterface::Status>> ();
    mapper.AddCommonType<optional<Set<NetworkInterface::Status>>> ();
    mapper.AddClass<NetworkInterface> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"Internal-Interface-ID", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fInternalInterfaceID)},
        {L"ID", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fGUID)},
        {L"Friendly-Name", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fFriendlyName)},
        {L"Description", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fDescription), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        // fNetworkGUID INTENTIONALLY OMITTED because doesn't correspond to our network ID, misleading, and unhelpful
        {L"Type", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fType)},
        {L"Hardware-Address", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fHardwareAddress)},
        {L"Transmit-Speed-Baud", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fTransmitSpeedBaud), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"Receive-Link-Speed-Baud", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fReceiveLinkSpeedBaud), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"Bindings", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fBindings)},
        {L"Gateways", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fGateways), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"DNS-Servers", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fDNSServers), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"Wireless-Information", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fWirelessInfo), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"Status", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fStatus)},
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
const ObjectVariantMapper Device::kMapper = []() {
    ObjectVariantMapper mapper;

    mapper.AddCommonType<InternetAddress> ();
    mapper.AddCommonType<optional<InternetAddress>> ();
    mapper.AddCommonType<Sequence<InternetAddress>> ();
    mapper.AddCommonType<Set<String>> ();
    mapper.AddCommonType<Device::DeviceType> ();
    mapper.AddCommonType<optional<Device::DeviceType>> ();
    mapper.AddCommonType<optional<String>> ();
    mapper.AddCommonType<optional<float>> ();
    mapper.AddCommonType<Sequence<String>> ();
    mapper.AddClass<Device> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"ID", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, persistentID)},
        {L"name", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, name)},
        {L"ipAddresses", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, ipAddresses)},
        {L"type", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, type), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"network", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, network)},
        {L"signalStrength", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, signalStrength), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"connected", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, connected)},
        {L"important", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, important)},
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
