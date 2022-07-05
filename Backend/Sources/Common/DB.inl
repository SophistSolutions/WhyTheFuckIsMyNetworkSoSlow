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
#include "Stroika/Foundation/Database/SQL/Transaction.h"
#include "Stroika/Foundation/Debug/Assertions.h"
#include "Stroika/Foundation/Debug/Trace.h"

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common {

    /*
     ********************************************************************************
     *************************************** DB *************************************
     ********************************************************************************
     */
    inline DB::DB (Version targetDBVersion, const Iterable<ORM::Schema::Table>& tables)
        : fTargetDBVersion_{targetDBVersion}
        , fTables_{tables}
    {
    }
    template <typename T>
    T DB::AddOrMergeUpdate (ORM::TableConnection<T>* dbConnTable, const T& d)
    {
        using namespace Stroika::Foundation;
        Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"DB::AddOrMergeUpdate", L"...,d=%s", Characters::ToString (d).c_str ())};
        RequireNotNull (dbConnTable);
        SQL::Transaction t{dbConnTable->pConnection ()->mkTransaction ()};
        std::optional<T> result;
        if (auto dbObj = dbConnTable->GetByID (d.fGUID)) {
            result = T::Merge (*dbObj, d);
            dbConnTable->Update (*result);
        }
        else {
            result = d;
            dbConnTable->AddNew (d);
        }
        t.Commit ();
        Ensure (result.has_value ());
        return *result;
    }

}

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_inl_*/
