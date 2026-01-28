/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Common/Property.h"
#include "Stroika/Foundation/Common/Version.h"
#include "Stroika/Foundation/Execution/Thread.h"
#include "Stroika/Foundation/Execution/TimeOutException.h"

#include "OperationalStatistics.h"

#ifndef qUseNewDocumentDBAPI
#define qUseNewDocumentDBAPI 0
#endif

#if qUseNewDocumentDBAPI
#include "Stroika/Foundation/Database/Document/Collection.h"
#include "Stroika/Foundation/Database/Document/Connection.h"
#include "Stroika/Foundation/Database/Document/ObjectCollection.h"
#endif
#if !qUseNewDocumentDBAPI
#include "Stroika/Foundation/Database/SQL/Connection.h"
#include "Stroika/Foundation/Database/SQL/ORM/Schema.h"
#include "Stroika/Foundation/Database/SQL/ORM/TableConnection.h"
#endif

/**
 *  Wrapper on persistence.
 */
namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common {

#if !qUseNewDocumentDBAPI
    using namespace Stroika::Foundation::Database::SQL;
#endif
    using Stroika::Foundation::Common::ReadOnlyProperty;
    using Stroika::Foundation::Common::Version;
    using Stroika::Foundation::Traversal::Iterable;
    using namespace Stroika::Foundation::Database;

    /**
     *  There is one database. Each DB object maps to the same underlying database. There can be as many as you want. The underlying
     *  database is multithreaded, but each 'DB' object ???
     */
    class DB {
#if !qUseNewDocumentDBAPI
    public:
        //constexpr VariantValue::Type kRepresentIDAs_ = VariantValue::Type::eBLOB;     // probably more performant
        static constexpr VariantValue::Type kRepresentIDAs_ = VariantValue::Type::eString; // more readable in DB tool
#endif

    public:
        DB () = default;
#if !qUseNewDocumentDBAPI
        DB (Version targetDBVersion, const Iterable<ORM::Schema::Table>& tables);
#endif
        DB (const DB&) = default;
        DB (DB&&)      = default;

#if qUseNewDocumentDBAPI
    public:
        /**
         *
         */
        nonvirtual Database::Document::Connection::Ptr GetInternallySynchronizedConnection ();
#endif
#if !qUseNewDocumentDBAPI
    public:
        /**
         *  Note - each Connection::Ptr can be used from any thread, but is not internally synchronized and must be used from one thread at a time.
         */
        nonvirtual SQL::Connection::Ptr NewConnection ();
#endif

#if qUseNewDocumentDBAPI
    public:
        template <typename T>
        nonvirtual T AddOrMergeUpdate (Document::ObjectCollection::Ptr<T> dbCollection, const T& d);
#endif

#if !qUseNewDocumentDBAPI
    public:
        template <typename T>
        nonvirtual T AddOrMergeUpdate (ORM::TableConnection<T>* dbConnTable, const T& d);
#endif

    public:
        static const ReadOnlyProperty<filesystem::path> pFileName;

    public:
        static const ReadOnlyProperty<uintmax_t> pFileSize;

    public:
        struct ReadStatsContext;

    public:
        struct WriteStatsContext;

#if qUseNewDocumentDBAPI
    private:
        /**
         *
         */
        Execution::Synchronized<Database::Document::Connection::Ptr> fConn_;
#endif

#if !qUseNewDocumentDBAPI
    private:
        Version                      fTargetDBVersion_;
        Iterable<ORM::Schema::Table> fTables_;
#endif
    };

    struct DB::ReadStatsContext : OperationalStatisticsMgr::ProcessDBCmd {
        ReadStatsContext ();
    };

    struct DB::WriteStatsContext : OperationalStatisticsMgr::ProcessDBCmd {
        WriteStatsContext ();
    };

#if !qUseNewDocumentDBAPI
    /**
     *  Define callback function used for logging/reporting status in DB access code.
     *  Set traceSQL = true here (or in particular calls for just those tables) to see logging of reads and writes.
     */
    template <typename TABLE_CONNECTION>
    auto mkOperationalStatisticsMgrProcessDBCmd (bool traceSQL = false) -> typename TABLE_CONNECTION::OpertionCallbackPtr;
#endif
}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "DB.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_h_*/
