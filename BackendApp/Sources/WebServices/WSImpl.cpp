/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"

using namespace std;

using namespace Stroika::Foundation;

#include "WSImpl.h"

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;

Collection<Device> WSImpl::GetDevices () const
{
    //tmphack - initial hardwired data
    return Collection<Device>{
        Device{
            L"Robert's Phone",
            L"192.168.244.34",
            L"fe80::ec4:7aff:fec7:7f1c",
            L"Phone",
            L"./images/phone.png",
            L"192.168.244.0/24",
            L"255.255.255.0",
            67,
            true},
        Device{
            L"WAP - Main",
            L"192.168.244.87",
            L"fe80::ea3:5fef:fed7:98cc",
            L"WAP",
            L"./images/WAP.png",
            L"192.168.244.0/24",
            L"255.255.255.0",
            34,
            true},
    };
}