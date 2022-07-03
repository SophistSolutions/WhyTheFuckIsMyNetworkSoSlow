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

    private:
        Version                      fTargetDBVersion_;
        Iterable<ORM::Schema::Table> fTables_;
    };

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "DB.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_DB_h_*/
