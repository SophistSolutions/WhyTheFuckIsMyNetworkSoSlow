/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Writer.h"

#include "Model.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Containers;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model;

/*
 ********************************************************************************
 ************************************* Device ***********************************
 ********************************************************************************
 */
DISABLE_COMPILER_MSC_WARNING_START (4573);
DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
const DataExchange::ObjectVariantMapper Device::kMapper = []() {
    using DataExchange::ObjectVariantMapper;
    ObjectVariantMapper mapper;
    mapper.AddCommonType<Device::DeviceType> ();
    mapper.AddCommonType<optional<Device::DeviceType>> ();
    mapper.AddCommonType<optional<String>> ();
    mapper.AddCommonType<optional<float>> ();
    mapper.AddCommonType<Sequence<String>> ();
    mapper.AddClass<Device> (initializer_list<ObjectVariantMapper::StructFieldInfo> {
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
