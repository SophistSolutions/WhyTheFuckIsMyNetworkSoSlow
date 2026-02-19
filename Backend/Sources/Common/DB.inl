/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_inl_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_inl_ 1

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Common/StroikaVersion.h"
#include "Stroika/Foundation/Debug/Assertions.h"
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/Execution/Logger.h"

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common {

    /*
 ********************************************************************************
 *************************************** DB *************************************
 ********************************************************************************
 */
    template <typename T>
    T DB::AddOrMergeUpdate (Document::ObjectCollection::Ptr<T> dbCollection, const T& d)
    {
        // @todo ADD TRANSACTION HERE
        using namespace Stroika::Foundation;
        using namespace Stroika::Foundation::Characters;
        Debug::TraceContextBumper ctx{"DB::AddOrMergeUpdate", "...,d={}"_f, d};
        std::optional<T>          result;
        // @todo transaction here...
        if (auto dbObj = dbCollection.Get (d.fID.template As<String> ())) {
            result = T::Merge (*dbObj, d);
            dbCollection.Replace (*result);
        }
        else {
            result = d;
            dbCollection.Add (d);
        }
        Ensure (result.has_value ());
        return *result;
    }

    /*
     ********************************************************************************
     ***************************** DB::ReadStatsContext *****************************
     ********************************************************************************
     */
    inline DB::ReadStatsContext::ReadStatsContext ()
        : ProcessDBCmd{OperationalStatisticsMgr::DBCommandType::eRead}
    {
    }

    /*
     ********************************************************************************
     ****************************** DB::WriteStatsContext ***************************
     ********************************************************************************
     */
    inline DB::WriteStatsContext::WriteStatsContext ()
        : ProcessDBCmd{OperationalStatisticsMgr::DBCommandType::eWrite}
    {
    }

}

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_inl_*/
