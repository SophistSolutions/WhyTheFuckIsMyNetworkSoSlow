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

/*
 ********************************************************************************
 ************************************* Device ***********************************
 ********************************************************************************
 */

#if qCompilerAndStdLib_lambda_expand_in_namespace_Buggy
const ObjectVariantMapper klambda_expand_in_namespace_Buggy_workaround_Mapper_ = []() {
    using IO::Network::CIDR;
    ObjectVariantMapper mapper;
    mapper.Add<CIDR> ([](const ObjectVariantMapper& mapper, const CIDR* obj) -> VariantValue { return obj->ToString (); },
                      [](const ObjectVariantMapper& mapper, const VariantValue& d, CIDR* intoObj) -> void { *intoObj = CIDR{d.As<String> ()}; });
    return mapper;
}();
#endif

DISABLE_COMPILER_MSC_WARNING_START (4573);
DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
const ObjectVariantMapper Device::kMapper = []() {
    ObjectVariantMapper mapper;

    using IO::Network::CIDR;
#if qCompilerAndStdLib_lambda_expand_in_namespace_Buggy
    mapper.Add (klambda_expand_in_namespace_Buggy_workaround_Mapper_);
#else
    mapper.Add<CIDR> ([](const ObjectVariantMapper& mapper, const CIDR* obj) -> VariantValue { return obj->ToString (); },
                      [](const ObjectVariantMapper& mapper, const VariantValue& d, CIDR* intoObj) -> void { *intoObj = CIDR{d.As<String> ()}; });
#endif

    mapper.AddCommonType<NetworkInterface::Type> ();
    mapper.AddCommonType<optional<NetworkInterface::Type>> ();
    mapper.AddCommonType<InternetAddress> ();

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
        {L"GUID", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fGUID)},
        {L"Friendly-Name", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fFriendlyName)},
        {L"Description", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fDescription), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"Network-GUID", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fNetworkGUID), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"Type", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fType)},
        {L"Hardware-Address", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fHardwareAddress)},
        {L"Transmit-Speed-Baud", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fTransmitSpeedBaud), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"Receive-Link-Speed-Baud", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fReceiveLinkSpeedBaud), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
        {L"Bindings", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fBindings)},
        {L"Status", Stroika_Foundation_DataExchange_StructFieldMetaInfo (NetworkInterface, fStatus)},

    });
    mapper.AddCommonType<Collection<NetworkInterface>> ();

    // quickie draft
    mapper.AddClass<Network> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"Attached-Interface-Friendly-Name-SB-LISTOFOWNINGINTERFACES", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fFriendlyName)},
        {L"Network-Address", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fNetworkAddress)},

        {L"Network-GUID", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fNetworkGUID), ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    mapper.AddCommonType<Collection<Network>> ();

    mapper.AddCommonType<Device::DeviceType> ();
    mapper.AddCommonType<optional<Device::DeviceType>> ();
    mapper.AddCommonType<optional<String>> ();
    mapper.AddCommonType<optional<float>> ();
    mapper.AddCommonType<Sequence<String>> ();
    mapper.AddClass<Device> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"persistentID", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, persistentID)},
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

String NetworkInterface::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"Internal-Interface-ID: " + Characters::ToString (fInternalInterfaceID) + L", ";
    sb += L"Friendly-Name: " + Characters::ToString (fFriendlyName) + L", ";
    if (fDescription) {
        sb += L"Description: " + Characters::ToString (*fDescription) + L", ";
    }
    if (fNetworkGUID) {
        sb += L"Network-GUID: " + Characters::ToString (*fNetworkGUID) + L", ";
    }
    if (fType) {
        sb += L"Type: " + Characters::ToString (*fType) + L", ";
    }
    if (fHardwareAddress) {
        sb += L"Hardware-Address: " + Characters::ToString (*fHardwareAddress) + L", ";
    }
    if (fTransmitSpeedBaud) {
        sb += L"Transmit-Speed-Baud: " + Characters::ToString (*fTransmitSpeedBaud) + L", ";
    }
    if (fReceiveLinkSpeedBaud) {
        sb += L"Receive-Link-Speed-Baud: " + Characters::ToString (*fReceiveLinkSpeedBaud) + L", ";
    }
    sb += L"Bindings: " + Characters::ToString (fBindings) + L", ";
    if (fStatus) {
        sb += L"Status: " + Characters::ToString (*fStatus) + L", ";
    }
    sb += L"}";
    return sb.str ();
}
