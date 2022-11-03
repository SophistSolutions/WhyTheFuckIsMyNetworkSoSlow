/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _TMPHACK_JSONPATCH_inl_
#define _TMPHACK_JSONPATCH_inl_ 1

/*
********************************************************************************
***************************** Implementation Details ***************************
********************************************************************************
*/

namespace Stroika::Foundation::Configuration {
    template <>
    constexpr EnumNames<WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::JSONPATCH::OperationType> DefaultNames<WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::JSONPATCH::OperationType>::k{
        EnumNames<WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::JSONPATCH::OperationType>::BasicArrayInitializer{{
            {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::JSONPATCH::OperationType::eAdd, L"add"},
            {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::JSONPATCH::OperationType::eRemove, L"remove"},
        }}};
}

#endif /*_TMPHACK_JSONPATCH_inl_*/
