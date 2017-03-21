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

/*
 ********************************************************************************
 **************************** Configuration::DefaultNames ***********************
 ********************************************************************************
 */
namespace Stroika {
    namespace Foundation {
        namespace Configuration {
            constexpr EnumNames<WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType> DefaultNames<WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType>::k;
        }
    }
}

/*
 ********************************************************************************
 ************************************* Device ***********************************
 ********************************************************************************
 */
const DataExchange::ObjectVariantMapper Device::kMapper = []() {
    using DataExchange::ObjectVariantMapper;
    ObjectVariantMapper mapper;
    mapper.AddCommonType<Device::DeviceType> ();
    mapper.AddCommonType<Optional<Device::DeviceType>> ();
    mapper.AddCommonType<Optional<String>> ();
    mapper.AddCommonType<Optional<float>> ();
    mapper.AddCommonType<Sequence<String>> ();
    DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    mapper.AddClass<Device> ({
        ObjectVariantMapper::StructFieldInfo{L"persistentID", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, persistentID)},
        ObjectVariantMapper::StructFieldInfo{L"name", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, name)},
        ObjectVariantMapper::StructFieldInfo{L"ipAddresses", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, ipAddresses)},
        ObjectVariantMapper::StructFieldInfo{L"type", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, type)},
        ObjectVariantMapper::StructFieldInfo{L"network", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, network)},
        ObjectVariantMapper::StructFieldInfo{L"signalStrength", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, signalStrength), ObjectVariantMapper::StructFieldInfo::NullFieldHandling::eOmit},
        ObjectVariantMapper::StructFieldInfo{L"connected", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, connected)},
        ObjectVariantMapper::StructFieldInfo{L"important", Stroika_Foundation_DataExchange_StructFieldMetaInfo (Device, important)},
    });
    DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    mapper.AddCommonType<Collection<Device>> ();
    return mapper;
}();

String Device::ToString () const
{
    return DataExchange::Variant::JSON::Writer ().WriteAsString (Device::kMapper.FromObject (*this));
}
