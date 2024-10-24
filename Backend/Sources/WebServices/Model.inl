/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_Model_inl_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_Model_inl_ 1

/*
********************************************************************************
***************************** Implementation Details ***************************
********************************************************************************
*/

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model {

    /*
     ********************************************************************************
     ************************ WebServices::Model::Network ***************************
     ********************************************************************************
     */
    inline Network::Network (const Set<CIDR>& nas)
        : fNetworkAddresses{nas}
    {
    }

}

namespace Stroika::Foundation::Common {
    template <>
    constexpr EnumNames<WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType>
        DefaultNames<WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType>::k{
            EnumNames<WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType>::BasicArrayInitializer{{
                {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::eMediaPlayer, L"Media-Player"},
                {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::eNetworkAttachedStorage,
                 L"Network-Attached-Storage"},
                {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::eNetworkInfrastructure,
                 L"Network-Infrastructure"},
                {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::ePC, L"Personal-Computer"},
                {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::ePhone, L"Phone"},
                {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::ePrinter, L"Printer"},
                {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::eRouter, L"Router"},
                {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::eSpeaker, L"Speaker"},
                {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::eTablet, L"Tablet"},
                {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::eTV, L"TV"},
                {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::eVirtualMachine, L"Virtual-Machine"},
                {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::eWTFCollector, L"WTFIMNSS-Collector"},
            }}};

    template <>
    constexpr EnumNames<WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::DeviceSortParameters::SearchTerm::By>
        DefaultNames<WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::DeviceSortParameters::SearchTerm::By>::k{
            EnumNames<WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::DeviceSortParameters::SearchTerm::By>::BasicArrayInitializer{{
                {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::DeviceSortParameters::SearchTerm::By::eAddress, L"Address"},
                {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::DeviceSortParameters::SearchTerm::By::ePriority, L"Priority"},
                {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::DeviceSortParameters::SearchTerm::By::eName, L"Name"},
                {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::DeviceSortParameters::SearchTerm::By::eType, L"Type"},
            }}};
}

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_Model_inl_*/
