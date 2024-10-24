/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Common/Property.h"
#include "Stroika/Foundation/Common/Version.h"
#include "Stroika/Foundation/Database/SQL/Connection.h"
#include "Stroika/Foundation/Database/SQL/ORM/Schema.h"
#include "Stroika/Foundation/Database/SQL/ORM/TableConnection.h"
#include "Stroika/Foundation/Execution/Thread.h"
#include "Stroika/Foundation/Execution/TimeOutException.h"

#include "OperationalStatistics.h"

/**
 *  Wrapper on persistence.
 */
namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common {

    using namespace Stroika::Foundation::Database::SQL;
    using Stroika::Foundation::Common::ReadOnlyProperty;
    using Stroika::Foundation::Common::Version;
    using Stroika::Foundation::Traversal::Iterable;
    using namespace Stroika::Foundation::Database;

    /**
     *  There is one database. Each DB object maps to the same underlying database. There can be as many as you want. The underlying
     *  database is multithreaded, but each 'DB' object ???
     */
    class DB {
    public:
        //constexpr VariantValue::Type kRepresentIDAs_ = VariantValue::Type::eBLOB;     // probably more performant
        static constexpr VariantValue::Type kRepresentIDAs_ = VariantValue::Type::eString; // more readable in DB tool

    public:
        DB () = delete;
        DB (Version targetDBVersion, const Iterable<ORM::Schema::Table>& tables);
        DB (const DB&) = default;
        DB (DB&&)      = default;

    public:
        /**
         *  Note - each Connection::Ptr can be used from any thread, but is not internally syncrhonized and must be used from one thread at a time.
         */
        nonvirtual SQL::Connection::Ptr NewConnection ();

    public:
        template <typename T>
        nonvirtual T AddOrMergeUpdate (ORM::TableConnection<T>* dbConnTable, const T& d);

    public:
        static ReadOnlyProperty<filesystem::path> pFileName;

    public:
        static ReadOnlyProperty<uintmax_t> pFileSize;

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

    /**
     *  Define callback function used for logging/reporting status in DB access code.
     *  Set traceSQL = true here (or in particular calls for just those tables) to see logging of reads and writes.
     */
    template <typename TABLE_CONNECTION>
    auto mkOperationalStatisticsMgrProcessDBCmd (bool traceSQL = false) -> typename TABLE_CONNECTION::OpertionCallbackPtr;

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "DB.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_h_*/
