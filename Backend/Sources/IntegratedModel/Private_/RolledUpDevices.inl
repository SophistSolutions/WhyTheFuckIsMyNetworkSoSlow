/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Mgr_Private_RolledUpDevices_inl_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Mgr_Private_RolledUpDevices_inl_ 1

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::IntegratedModel::Private_ {

    /*
     ********************************************************************************
     **************** IntegratedModel::Private_::RolledUpNetworks *******************
     ********************************************************************************
     */
    inline DeviceCollection RolledUpDevices::GetDevices () const
    {
        return fRolledUpDevices;
    }

}

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Mgr_Private_RolledUpDevices_inl_*/
