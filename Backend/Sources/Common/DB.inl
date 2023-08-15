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
#include "Stroika/Foundation/Configuration/StroikaVersion.h"
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
        Debug::TraceContextBumper ctx{
            Stroika_Foundation_Debug_OptionalizeTraceArgs (L"DB::AddOrMergeUpdate", L"...,d=%s", Characters::ToString (d).c_str ())};
        RequireNotNull (dbConnTable);
#if kStroika_Version_Major >= 3
        SQL::Transaction t{dbConnTable->connection ()->mkTransaction ()};
#else
        SQL::Transaction t{dbConnTable->pConnection ()->mkTransaction ()};
#endif
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
                        DbgTrace (L"<DBRead: %s>", s->GetSQL (Statement::WhichSQLFlag::eExpanded).c_str ());
                    }
                    if (kIncludeLastSQK_) {
                        lastSQL = s->GetSQL (Statement::WhichSQLFlag::eExpanded);
                    }
                    IgnoreExceptionsExceptThreadAbortForCall (tmp = make_shared<DB::ReadStatsContext> ());
                    break;
                case TABLE_CONNECTION::Operation::eCompletedRead:
                    if (traceSQL) {
                        DbgTrace (L"</DBRead>");
                    }
                    tmp.reset ();
                    break;
                case TABLE_CONNECTION::Operation::eStartingWrite:
                    RequireNotNull (s);
                    if (traceSQL) {
                        DbgTrace (L"<DBWrite: %s>", s->GetSQL (Statement::WhichSQLFlag::eExpanded).c_str ());
                    }
                    if (kIncludeLastSQK_) {
                        lastSQL = s->GetSQL (Statement::WhichSQLFlag::eExpanded);
                    }
                    IgnoreExceptionsExceptThreadAbortForCall (tmp = make_shared<DB::WriteStatsContext> ());
                    break;
                case TABLE_CONNECTION::Operation::eCompletedWrite:
                    if (traceSQL) {
                        DbgTrace (L"</DBWrite>");
                    }
                    tmp.reset ();
                    break;
                case TABLE_CONNECTION::Operation::eNotifyError:
                    if (kIncludeLastSQK_) {
                        Execution::Logger::sThe.Log (Execution::Logger::eWarning, L"Database operation exception: %s (lastsql %s)",
                                                     Characters::ToString (e).c_str (), Characters::ToString (lastSQL).c_str ());
                    }
                    else {
                        Execution::Logger::sThe.Log (Execution::Logger::eWarning, L"Database operation exception: %s",
                                                     Characters::ToString (e).c_str ());
                    }
                    tmp->NoteError ();
                    break;
            }
        };
        return r;
    }

}

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_inl_*/
