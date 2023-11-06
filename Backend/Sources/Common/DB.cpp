/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Cache/SynchronizedCallerStalenessCache.h"
#include "Stroika/Foundation/Common/GUID.h"
#include "Stroika/Foundation/Common/KeyValuePair.h"
#include "Stroika/Foundation/Common/Property.h"
#include "Stroika/Foundation/Containers/KeyedCollection.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/Database/SQL/ORM/Schema.h"
#include "Stroika/Foundation/Database/SQL/ORM/TableConnection.h"
#include "Stroika/Foundation/Database/SQL/ORM/Versioning.h"
#include "Stroika/Foundation/Database/SQL/SQLite.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Execution/Sleep.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/FileSystem/WellKnownLocations.h"

#include "DB.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Common;
using namespace Stroika::Foundation::Database;
using namespace Stroika::Foundation::DataExchange;
using namespace Stroika::Foundation::Execution;
using namespace Stroika::Foundation::Memory;
using namespace Stroika::Foundation::IO::Network;
using namespace Stroika::Foundation::IO::Network::HTTP;

using namespace SQL::ORM;
using namespace SQL::SQLite;

/*
 ********************************************************************************
 ********* WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::DB ******************
 ********************************************************************************
 */
ReadOnlyProperty<filesystem::path> WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::DB::pFileName{
    [qStroika_Foundation_Common_Property_ExtraCaptureStuff] ([[maybe_unused]] const auto* property) -> filesystem::path {
        return IO::FileSystem::WellKnownLocations::GetApplicationData () / "WhyTheFuckIsMyNetworkSoSlow" / "db-v16.db";
    }};

ReadOnlyProperty<uintmax_t> WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::DB::pFileSize{
    [qStroika_Foundation_Common_Property_ExtraCaptureStuff] ([[maybe_unused]] const auto* property) -> uintmax_t {
        // add sizes of various component files (WAL, etc)
        uintmax_t szTotal{};
        auto      incSize = [&] (const filesystem::path& p) {
            error_code ec{};
            uintmax_t  sz = filesystem::file_size (p, ec);
            if (!ec) {
                szTotal += sz;
            }
        };
        filesystem::path p = pFileName ();
        incSize (p);
        p = pFileName ();
        p += "-journal";
        incSize (p);
        p = pFileName ();
        p += "-shm";
        incSize (p);
        p = pFileName ();
        p += "-wal";
        incSize (p);
        return szTotal;
    }};

WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::DB::DB (Version targetDBVersion, const Iterable<ORM::Schema::Table>& tables)
    : fTargetDBVersion_{targetDBVersion}
    , fTables_{tables}
{
}

SQL::Connection::Ptr WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::DB::NewConnection ()
{
    using SQLite::Connection::Options;
    // Logically I THINK (from docs) - should use eMultiThread but experiemnt with eSerialized to see if fixes sporadic failures (mostly on unix)- but maybe just now saw on windows - flying...--LGP 2023-03-92
    // Serialized didn't hlep - still get tons of 'Device or resource busy on database extion lastsql select * from BLOBURL'...., (at least wtih stk 2.1) - so go back to multithread - makes more sense -- LGP 2023-09-13
    constexpr auto kThreadModel_ = Options::ThreadingMode::eMultiThread;
    //constexpr auto kThreadModel_ = Options::ThreadingMode::eSerialized;
    auto           dbPath        = pFileName ();
    filesystem::create_directories (dbPath.parent_path ());
#if __cpp_designated_initializers
    auto options = Options{.fDBPath = dbPath, .fThreadingMode = kThreadModel_};
#else
    auto options = Options{dbPath, true, nullopt, nullopt, kThreadModel_};
#endif

    /*
     *  Software doing database accesses must handle busy timeout exceptions, but
     *  reducing thier frequency probably produces better, smoother operation.
     * 
     *  We get TONS of SQLITE_BUSY errors using the default JournalMode, but if you read
     *  https://sqlite.org/wal.html, you will see WAL is recommended for multiple readers/writers on DB.
     * 
     *  NOTE - though much better, still not working perfectly with these settings. Sometimes gets SQLITE_BUSY.
     *  Our rollup code sometimes calls through to the database (from calling webservice thread), and we have background requests to add records
     *  from the AddOrMergeUpdate () calls (another thread). These sometimes contend and at least on raspberrypi
     *  the conflict can exceed 1 second. Don't want the timeout too long, cuz better to see warnings in the logs
     *  than sluggish operation and no warnings.
     *
     *  As of 2022-09-08 experimenting with 2.5s;
     * 
     *  See https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/35
     *  COULD possibly redo this using much smaller timeout if I used a SINGLE SHARED connection (in eMultiThreaded mode as above).
     *  Unclear how that would affect checking on underlying statement objects (I think fine but need to review).
     */
    options.fBusyTimeout = 2.5s;
    options.fJournalMode = JournalModeType::eWAL2;

    auto conn = SQLite::Connection::New (options);

    Database::SQL::ORM::ProvisionForVersion (conn, fTargetDBVersion_, fTables_);

    return conn;
}
