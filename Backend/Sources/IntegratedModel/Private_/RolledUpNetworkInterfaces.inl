/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Private_RolledUpNetworkInterfaces_inl_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Private_RolledUpNetworkInterfaces_inl_ 1

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::IntegratedModel::Private_ {

    /*
     ********************************************************************************
     ************** IntegratedModel::Private_::RolledUpNetworkInterfaces ************
     ********************************************************************************
     */
    inline NetworkInterfaceCollection RolledUpNetworkInterfaces::GetNetworkInterfacess () const
    {
        return fRolledUpNetworkInterfaces_;
    }
    inline NetworkInterface RolledUpNetworkInterfaces::GetRollupNetworkInterface (const GUID& id) const
    {
        return Memory::ValueOf (fRolledUpNetworkInterfaces_.Lookup (id));
    }
    inline NetworkInterfaceCollection RolledUpNetworkInterfaces::GetRollupNetworkInterfaces (const Set<GUID>& rollupIDs) const
    {
        Require (Set<GUID>{fRolledUpNetworkInterfaces_.Keys ()}.ContainsAll (rollupIDs));
        return fRolledUpNetworkInterfaces_.Where ([&rollupIDs] (const auto& i) { return rollupIDs.Contains (i.fID); });
    }
    inline NetworkInterfaceCollection RolledUpNetworkInterfaces::GetRawNetworkInterfaces () const
    {
        return fRawNetworkInterfaces_;
    }
    inline NetworkInterfaceCollection RolledUpNetworkInterfaces::GetRawNetworkInterfaces (const Set<GUID>& rawIDs) const
    {
        Require (Set<GUID>{fRawNetworkInterfaces_.Keys ()}.ContainsAll (rawIDs));
        return fRawNetworkInterfaces_.Where ([&rawIDs] (const auto& i) { return rawIDs.Contains (i.fID); });
    }
    inline auto RolledUpNetworkInterfaces::MapAggregatedNetInterfaceID2ItsRollupID (const GUID& netID) const -> optional<GUID>
    {
        return fMapAggregatedNetInterfaceID2RollupID_.Lookup (netID);
    }
    inline auto RolledUpNetworkInterfaces::MapAggregatedNetInterfaceID2ItsRollupID (const Set<GUID>& netIDs) const -> Set<GUID>
    {
        return netIDs.Map<GUID, Set<GUID>> ([this] (const auto& i) { return MapAggregatedNetInterfaceID2ItsRollupID (i); });
    }

}

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_IntegratedModel_Private_RolledUpNetworkInterfaces_inl_*/
