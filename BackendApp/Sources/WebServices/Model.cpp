/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Writer.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Containers;

#include "Model.h"

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model;

const DataExchange::ObjectVariantMapper Device::kMapper = []() {
    using DataExchange::ObjectVariantMapper;
    ObjectVariantMapper mapper;
    mapper.AddCommonType<Optional<String>> ();
    mapper.AddCommonType<Optional<float>> ();
    mapper.AddClass<Device> ({
        ObjectVariantMapper::StructFieldInfo{L"name", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, name)},
        ObjectVariantMapper::StructFieldInfo{L"ipAddress", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, ipAddress)},
        ObjectVariantMapper::StructFieldInfo{L"ipv4", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, ipv4), ObjectVariantMapper::StructFieldInfo::NullFieldHandling::eOmit},
        ObjectVariantMapper::StructFieldInfo{L"ipv6", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, ipv6), ObjectVariantMapper::StructFieldInfo::NullFieldHandling::eOmit},
        ObjectVariantMapper::StructFieldInfo{L"type", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, type)},
        ObjectVariantMapper::StructFieldInfo{L"network", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, network)},
        ObjectVariantMapper::StructFieldInfo{L"networkMask", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, networkMask)},
        ObjectVariantMapper::StructFieldInfo{L"signalStrength", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, signalStrength)},
        ObjectVariantMapper::StructFieldInfo{L"connected", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, connected)},
        ObjectVariantMapper::StructFieldInfo{L"important", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, important)},
    });
    mapper.AddCommonType<Collection<Device>> ();
    return mapper;
}();

String Device::ToString () const
{
    return DataExchange::Variant::JSON::Writer ().WriteAsString (Device::kMapper.FromObject (*this));
}
