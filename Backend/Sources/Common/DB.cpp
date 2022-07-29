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
        return IO::FileSystem::WellKnownLocations::GetApplicationData () / "WhyTheFuckIsMyNetworkSoSlow" / "db-v11.db";
    }};

ReadOnlyProperty<uintmax_t> WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::DB::pFileSize{[qStroika_Foundation_Common_Property_ExtraCaptureStuff] ([[maybe_unused]] const auto* property) -> uintmax_t {
    return filesystem::file_size (pFileName ());
}};

WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::DB::DB (Version targetDBVersion, const Iterable<ORM::Schema::Table>& tables)
    : fTargetDBVersion_{targetDBVersion}
    , fTables_{tables}
{
}

SQL::Connection::Ptr WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::DB::NewConnection ()
{
    auto dbPath = pFileName ();
    filesystem::create_directories (dbPath.parent_path ());
#if __cpp_designated_initializers
    auto options = Options{.fDBPath = dbPath, .fThreadingMode = Options::ThreadingMode::eMultiThread};
#else
    auto options = Options{dbPath, true, nullopt, nullopt, Options::ThreadingMode::eMultiThread};
#endif

    /*
     *  Software doing database accesses must handle busy timeout exceptions, but
     *  reducing thier frequency probably produces better, smoother operation.
     * 
     *  We get TONS of SQLITE_BUSY errors using the default JournalMode, but if you read
     *  https://sqlite.org/wal.html, you will see WAL is recommended for multiple readers/writers on DB.
     * 
     *  NOTE - though much better, still not working well with these settings. Sometimes fails.
     */
    options.fBusyTimeout = 1ms;
    options.fJournalMode = JournalModeType::eWAL;

    auto conn = SQLite::Connection::New (options);

    Database::SQL::ORM::ProvisionForVersion (conn, fTargetDBVersion_, fTables_);

    return conn;
}
