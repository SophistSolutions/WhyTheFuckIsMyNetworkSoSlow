/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Collection.h"
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

// @todo figure out why cannot do these inline in Add<CIDR> call??
ObjectVariantMapper::FromObjectMapperType<CIDR> kCIDR_FROMOBJMAPPER_ = [](const ObjectVariantMapper& mapper, const CIDR* obj) -> VariantValue {
    return obj->ToString ();
};
ObjectVariantMapper::ToObjectMapperType<CIDR> kCIDR_TOOBJMAPPER_ = [](const ObjectVariantMapper& mapper, const VariantValue& d, CIDR* intoObj) -> void {
    *intoObj = CIDR{d.As<String> ()};
};

DISABLE_COMPILER_MSC_WARNING_START (4573);
DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
const ObjectVariantMapper Device::kMapper = []() {
    ObjectVariantMapper mapper;

    mapper.Add<CIDR> (kCIDR_FROMOBJMAPPER_,
                      kCIDR_TOOBJMAPPER_);

    // quickie draft
    mapper.AddClass<Network> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"Attached-Interface-Friendly-Name", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Network, fFriendlyName)},
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
