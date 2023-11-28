/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Private_DBAccess_inl_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Private_DBAccess_inl_ 1

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "Stroika/Foundation/Debug/TimingTrace.h"

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::IntegratedModel::Private_::DBAccess {

    /*
     ********************************************************************************
     ****************** IntegratedModel::Private_::DBAccess::Mgr ********************
     ********************************************************************************
     */
    inline Mapping<GUID, Device::UserOverridesType> Mgr::GetDeviceUserSettings () const
    {
        Debug::TimingTrace ttrc{L"IntegratedModel...GetDeviceUserSettings ()", 1ms};
        return fCachedDeviceUserSettings_.cget ().cref ();
    }
    inline optional<Device::UserOverridesType> Mgr::LookupDevicesUserSettings (const GUID& guid) const
    {
        Debug::TimingTrace ttrc{L"IntegratedModel...LookupDevicesUserSettings ()", 1ms};
        return fCachedDeviceUserSettings_.cget ().cref ().Lookup (guid);
    }
    inline Mapping<GUID, Network::UserOverridesType> Mgr::GetNetworkUserSettings () const
    {
        Debug::TimingTrace ttrc{L"IntegratedModel...GetNetworkUserSettings ()", 1ms};
        return fCachedNetworkUserSettings_.cget ().cref ();
    }
    inline optional<Network::UserOverridesType> Mgr::LookupNetworkUserSettings (const GUID& guid) const
    {
        Debug::TimingTrace ttrc{L"IntegratedModel...LookupNetworkUserSettings ()", 1ms};
        return fCachedNetworkUserSettings_.cget ().cref ().Lookup (guid);
    }
    inline NetworkInterfaceCollection Mgr::GetRawNetworkInterfaces () const
    {
        return fDBNetworkInterfaces_;
    }
    inline NetworkCollection Mgr::GetRawNetworks () const
    {
        return fDBNetworks_;
    }
    inline DeviceCollection Mgr::GetRawDevices () const
    {
        return fDBDevices_;
    }

}

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Private_DBAccess_inl_*/
