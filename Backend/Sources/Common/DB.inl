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
#include "Stroika/Foundation/Database/SQL/Transaction.h"
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
    T DB::AddOrMergeUpdate (ORM::TableConnection<T>* dbConnTable, const T& d)
    {
        using namespace Stroika::Foundation;
        using namespace Stroika::Foundation::Characters;
        Debug::TraceContextBumper ctx{"DB::AddOrMergeUpdate", "...,d={}"_f, d};
        RequireNotNull (dbConnTable);
        SQL::Transaction t{dbConnTable->connection ()->mkTransaction ()};
        std::optional<T> result;
        Assert (kRepresentIDAs_ == VariantValue::Type::eString or kRepresentIDAs_ == VariantValue::Type::eBLOB);
        VariantValue id = (kRepresentIDAs_ == VariantValue::Type::eString) ? VariantValue{d.fID.template As<String> ()}
                                                                           : VariantValue{d.fID.template As<Memory::BLOB> ()};
        if (auto dbObj = dbConnTable->Get (id)) {
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

    /*
     ********************************************************************************
     ********************* mkOperationalStatisticsMgrProcessDBCmd *******************
     ********************************************************************************
     */
    template <typename TABLE_CONNECTION>
    auto mkOperationalStatisticsMgrProcessDBCmd (bool traceSQL) -> typename TABLE_CONNECTION::OpertionCallbackPtr
    {
        using namespace Characters;
        shared_ptr<OperationalStatisticsMgr::ProcessDBCmd>      tmp; // use shared_ptr in lambda so copies of lambda share same object
        constexpr bool                                          kIncludeLastSQK_ = true;
        conditional_t<kIncludeLastSQK_, optional<String>, void> lastSQL;
        // @todo note - COULD use same shared_ptr object to store a Debug::TraceContextBumper object so we get /DBRead messages elided from log most of the time (when quick and /DBWrite).
        auto r = [=] (typename TABLE_CONNECTION::Operation op, const TABLE_CONNECTION* /*tableConn*/, const Statement* s,
                      const exception_ptr& e) mutable noexcept {
            switch (op) {
                case TABLE_CONNECTION::Operation::eStartingRead:
                    RequireNotNull (s);
                    if (traceSQL) {
                        DbgTrace ("<DBRead: {}>"_f, s->GetSQL (Statement::WhichSQLFlag::eExpanded));
                    }
                    if (kIncludeLastSQK_) {
                        lastSQL = s->GetSQL (Statement::WhichSQLFlag::eExpanded);
                    }
                    IgnoreExceptionsExceptThreadAbortForCall (tmp = make_shared<DB::ReadStatsContext> ());
                    break;
                case TABLE_CONNECTION::Operation::eCompletedRead:
                    if (traceSQL) {
                        DbgTrace ("</DBRead>"_f);
                    }
                    tmp.reset ();
                    break;
                case TABLE_CONNECTION::Operation::eStartingWrite:
                    RequireNotNull (s);
                    if (traceSQL) {
                        DbgTrace ("<DBWrite: {}>"_f, s->GetSQL (Statement::WhichSQLFlag::eExpanded));
                    }
                    if (kIncludeLastSQK_) {
                        lastSQL = s->GetSQL (Statement::WhichSQLFlag::eExpanded);
                    }
                    IgnoreExceptionsExceptThreadAbortForCall (tmp = make_shared<DB::WriteStatsContext> ());
                    break;
                case TABLE_CONNECTION::Operation::eCompletedWrite:
                    if (traceSQL) {
                        DbgTrace ("</DBWrite>"_f);
                    }
                    tmp.reset ();
                    break;
                case TABLE_CONNECTION::Operation::eNotifyError:
                    if (kIncludeLastSQK_) {
                        Execution::Logger::sThe.Log (Execution::Logger::eWarning, "Database operation exception: {} (last sql {})"_f, e, lastSQL);
                    }
                    else {
                        Execution::Logger::sThe.Log (Execution::Logger::eWarning, "Database operation exception: {}"_f, e);
                    }
                    tmp->NoteError ();
                    break;
            }
        };
        return r;
    }

}

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_inl_*/
