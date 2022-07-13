/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Configuration/Version.h"
#include "Stroika/Foundation/Database/SQL/Connection.h"
#include "Stroika/Foundation/Database/SQL/ORM/Schema.h"
#include "Stroika/Foundation/Database/SQL/ORM/TableConnection.h"
#include "Stroika/Foundation/Execution/Thread.h"

#include "OperationalStatistics.h"

/**
 *  Wrapper on persistence.
 */
namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common {

    using namespace Stroika::Foundation::Database::SQL;
    using Stroika::Foundation::Configuration::Version;
    using Stroika::Foundation::Traversal::Iterable;
    using namespace Stroika::Foundation::Database;

    /**
     *  There is one database. Each DB object maps to the same underlying database. There can be as many as you want. The underlying
     *  database is multithreaded, but each 'DB' object ???
     */
    class DB {
    public:
        DB (Version targetDBVersion, const Iterable<ORM::Schema::Table>& tables);

    public:
        nonvirtual SQL::Connection::Ptr NewConnection ();

    public:
        template <typename T>
        nonvirtual T AddOrMergeUpdate (ORM::TableConnection<T>* dbConnTable, const T& d);

    public:
        struct ReadStatsContext;

    public:
        struct WriteStatsContext;

    private:
        Version                      fTargetDBVersion_;
        Iterable<ORM::Schema::Table> fTables_;
    };

    struct DB::ReadStatsContext : OperationalStatisticsMgr::ProcessDBCmd {
        ReadStatsContext ();
    };

    struct DB::WriteStatsContext : OperationalStatisticsMgr::ProcessDBCmd {
        WriteStatsContext ();
    };

    template <typename TABLE_CONNECTION>
    auto mkOperationalStatisticsMgrProcessDBCmd ()
    {
        shared_ptr<OperationalStatisticsMgr::ProcessDBCmd> tmp; // use shared_ptr in lambda so copies of lambda share same object
        auto                                               r = [=] (typename TABLE_CONNECTION::Operation op, const TABLE_CONNECTION* /*tableConn*/, const Statement* /*s*/) mutable noexcept {
            switch (op) {
                case TABLE_CONNECTION::Operation::eStartingRead:
                    IgnoreExceptionsExceptThreadAbortForCall (tmp = make_shared<DB::ReadStatsContext> ());
                    break;
                case TABLE_CONNECTION::Operation::eCompletedRead:
                    tmp.reset ();
                    break;
                case TABLE_CONNECTION::Operation::eStartingWrite:
                    IgnoreExceptionsExceptThreadAbortForCall (tmp = make_shared<DB::WriteStatsContext> ());
                    break;
                case TABLE_CONNECTION::Operation::eCompletedWrite:
                    tmp.reset ();
                    break;
            }
        };
        return r;
    }

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "DB.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_h_*/
