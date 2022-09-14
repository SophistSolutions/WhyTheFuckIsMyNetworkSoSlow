/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _TMPHACK_JSONPATCH_h_
#define _TMPHACK_JSONPATCH_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"

/**
 *  TMPHACK - this belongs in stroika - but tmp code til Stroika v3
 */
namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::JSONPATCH {

    using namespace Stroika::Foundation;

    using Characters::String;
    using DataExchange::VariantValue;

    // SEE https://jsonpatch.com/

    enum class OperationType {
        eAdd,
        eRemove,

        Stroika_Define_Enum_Bounds (eAdd, eRemove)
    };

    using JSONPointerType = String;

    struct OperationItemType {
        OperationType          op;
        JSONPointerType        path;
        optional<VariantValue> value;

        /**
         *  @see Characters::ToString ();
         */
        nonvirtual String ToString () const;

        static const DataExchange::ObjectVariantMapper kMapper;
    };

    struct OperationItemsType : Containers::Sequence<OperationItemType> {
        static const DataExchange::ObjectVariantMapper kMapper;
    };

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "JSONPATCH.inl"

#endif /*_TMPHACK_JSONPATCH_h_*/
