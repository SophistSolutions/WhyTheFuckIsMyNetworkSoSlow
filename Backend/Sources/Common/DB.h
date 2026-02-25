/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Common/Property.h"
#include "Stroika/Foundation/Common/Version.h"
#include "Stroika/Foundation/Database/Document/Collection.h"
#include "Stroika/Foundation/Database/Document/Connection.h"
#include "Stroika/Foundation/Database/Document/ObjectCollection.h"
#include "Stroika/Foundation/Execution/Thread.h"
#include "Stroika/Foundation/Execution/TimeOutException.h"

#include "OperationalStatistics.h"

/**
 *  Wrapper on persistence.
 */
namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common {

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
        DB ()          = default;
        DB (const DB&) = default;
        DB (DB&&)      = default;

    public:
        /**
         *
         */
        nonvirtual Database::Document::Connection::Ptr GetInternallySynchronizedConnection () const;

    private:
        nonvirtual Database::Document::Connection::Ptr CreateCachedInternallySynchronizedConnection_ () const;

    public:
        template <typename T>
        nonvirtual T AddOrMergeUpdate (Document::ObjectCollection::Ptr<T> dbCollection, const T& d);

    public:
        static const ReadOnlyProperty<filesystem::path> pFileName;

    public:
        nonvirtual uintmax_t GetFileSize () const;

    public:
        struct ReadStatsContext;

    public:
        struct WriteStatsContext;

    private:
        /**
         * static so we construct once, do any initialization/setup once. Ptr references internally synchronized letter.
         */
        static inline Execution::Synchronized<Database::Document::Connection::Ptr> sConn_;
    };

    struct DB::ReadStatsContext : OperationalStatisticsMgr::ProcessDBCmd {
        ReadStatsContext ();
    };

    struct DB::WriteStatsContext : OperationalStatisticsMgr::ProcessDBCmd {
        WriteStatsContext ();
    };

    /**
     *  Define callback function used for logging/reporting status in DB access code.
     *  Set traceDB = true here (or in particular calls for just those tables) to see logging of reads and writes.
     */
    auto mkOperationalStatisticsMgrProcessDBCmd (bool traceDB = false) -> Database::Document::Connection::OpertionCallbackPtr;
}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "DB.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_h_*/
