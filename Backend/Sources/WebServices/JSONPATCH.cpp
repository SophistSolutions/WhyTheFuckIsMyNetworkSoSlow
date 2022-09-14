/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "JSONPATCH.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::DataExchange;

using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::JSONPATCH;

/*
 ********************************************************************************
 ********************** JSONPATCH::OperationItemType ****************************
 ********************************************************************************
 */
String JSONPATCH::OperationItemType::ToString () const
{
    Characters::StringBuilder sb;
    sb += L"{";
    sb += L"op: " + Characters::ToString (op) + L", ";
    sb += L"path: " + Characters::ToString (path) + L", ";
    if (value) {
        sb += L"value: " + Characters::ToString (value);
    }
    sb += L"}";
    return sb.str ();
}

const DataExchange::ObjectVariantMapper JSONPATCH::OperationItemType::kMapper = [] () {
    ObjectVariantMapper mapper;
    mapper.AddCommonType<OperationType> ();
    mapper.AddCommonType<String> ();
    mapper.AddCommonType<optional<String>> ();
    mapper.AddCommonType<optional<VariantValue>> ();
    mapper.AddClass<OperationItemType> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
        {L"op"sv, StructFieldMetaInfo{&OperationItemType::op}},
        {L"path"sv, StructFieldMetaInfo{&OperationItemType::path}},
        {L"value"sv, StructFieldMetaInfo{&OperationItemType::value}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
    });
    return mapper;
}();

/*
 ********************************************************************************
 ********************** JSONPATCH::OperationItemsType ***************************
 ********************************************************************************
 */
const DataExchange::ObjectVariantMapper JSONPATCH::OperationItemsType::kMapper = [] () {
    ObjectVariantMapper mapper;
    mapper += OperationItemType::kMapper;
    mapper.AddCommonType<OperationItemsType> ();
    return mapper;
}();
