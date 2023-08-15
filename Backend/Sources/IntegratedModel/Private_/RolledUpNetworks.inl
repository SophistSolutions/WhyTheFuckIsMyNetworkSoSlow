/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Private_RolledUpNetworks_inl_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Private_RolledUpNetworks_inl_ 1

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::IntegratedModel::Private_ {

    /*
     ********************************************************************************
     ************** IntegratedModel::Private_::RolledUpNetworks ************
     ********************************************************************************
     */

    inline NetworkCollection RolledUpNetworks::GetNetworks () const
    {
        return fRolledUpNetworks_;
    }

}

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Private_RolledUpNetworks_inl_*/
