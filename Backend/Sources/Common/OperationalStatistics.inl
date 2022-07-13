/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_OperationalStatistics_inl_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_OperationalStatistics_inl_ 1

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common {

    /*
     ********************************************************************************
     ***************** OperationalStatisticsMgr::ProcessAPICmd **********************
     ********************************************************************************
     */
    inline OperationalStatisticsMgr::ProcessAPICmd::ProcessAPICmd ()
        : fStart_{Time::GetTickCount ()}
    {
    }

    /*
     ********************************************************************************
     ***************** OperationalStatisticsMgr::ProcessDBCmd **********************
     ********************************************************************************
     */
    inline OperationalStatisticsMgr::ProcessDBCmd::ProcessDBCmd (DBCommandType cmdType)
        : fStart_{Time::GetTickCount ()}
    {
        switch (cmdType) {
            case DBCommandType::eRead:
                fKind_ = Rec_::Kind::eDBRead;
                break;
            case DBCommandType::eWrite:
                fKind_ = Rec_::Kind::eDBWrite;
                break;
            default:
                RequireNotReached ();
        }
    }

    /*
     ********************************************************************************
     ************************** OperationalStatisticsMgr ****************************
     ********************************************************************************
     */
    inline void OperationalStatisticsMgr::Add_ (const Rec_& r)
    {
        lock_guard lk{fMutex_};
        ++fNextHistory_;
        if (fNextHistory_ == Memory::NEltsOf (fRollingHistory_)) {
            fNextHistory_ = 0;
        }
        Assert (fNextHistory_ < Memory::NEltsOf (fRollingHistory_));
        fRollingHistory_[fNextHistory_] = r;
    }

}

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_OperationalStatistics_inl_*/
