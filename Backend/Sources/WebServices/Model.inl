/*
* Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
*/
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_Model_inl_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_Model_inl_ 1

/*
********************************************************************************
***************************** Implementation Details ***************************
********************************************************************************
*/

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model {
}

namespace Stroika::Foundation::Configuration {
    template <>
    struct DefaultNames<WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType> : EnumNames<WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType> {
        static constexpr EnumNames<WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType> k{
            EnumNames<WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType>::BasicArrayInitializer{
                {
                    {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::eLaptop, L"Laptop"},
                    {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::eDesktop, L"Desktop"},
                    {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::eTablet, L"Tablet"},
                    {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::ePhone, L"Phone"},
                    {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::eRouter, L"Router"},
                    {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::eSpeaker, L"Speaker"},
                    {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::ePrinter, L"Printer"},
                    {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::eInfrastructureDevice, L"Infrastructure-Device"},
                    {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::eMediaPlayer, L"Media-Player"},
                    {WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType::eTV, L"TV"},
                }}};
        DefaultNames ()
            : EnumNames<WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model::Device::DeviceType> (k)
        {
        }
    };
}

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_WebServices_Model_inl_*/
