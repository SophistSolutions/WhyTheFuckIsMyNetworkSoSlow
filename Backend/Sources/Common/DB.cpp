/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Cache/TimedCache.h"
#include "Stroika/Foundation/Common/GUID.h"
#include "Stroika/Foundation/Common/KeyValuePair.h"
#include "Stroika/Foundation/Common/Property.h"
#include "Stroika/Foundation/Containers/KeyedCollection.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/Database/Document/LocalDocumentDB.h"
#include "Stroika/Foundation/Database/Document/SQLite.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Execution/Sleep.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/FileSystem/WellKnownLocations.h"

#include "../Common/AppConfiguration.h"

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

using AppConfigurationType      = WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::AppConfigurationType;
using DatabaseConfigurationType = AppConfigurationType::DatabaseConfigurationType;
using DirectoryJSONStorage      = AppConfigurationType::DirectoryJSONStorage;
using SingleFileJSONStorage     = AppConfigurationType::SingleFileJSONStorage;
using SQLiteStorage             = AppConfigurationType::SQLiteStorage;

namespace {
    filesystem::path RegularizeLocalFilePathFromConfigs_ (const filesystem::path& p)
    {
        if (p.empty ()) {
            return p;
        }
        if (p.is_relative ()) {
            return IO::FileSystem::WellKnownLocations::GetApplicationData () / "WhyTheFuckIsMyNetworkSoSlow" / p;
        }
        return p;
    }
}

/*
 ********************************************************************************
 ********* WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::DB ******************
 ********************************************************************************
 */
const ReadOnlyProperty<filesystem::path> WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::DB::pFileName{
    [] ([[maybe_unused]] const auto* property) -> filesystem::path {
        DatabaseConfigurationType dbConfig{BackendApp::Common::gAppConfiguration->fDatabase};
        if (SingleFileJSONStorage* osjs = get_if<SingleFileJSONStorage> (&dbConfig)) {
            filesystem::path p = osjs->fFile;
            if (p.empty ()) {
                p = "db-fs-single-v1";
            }
            return RegularizeLocalFilePathFromConfigs_ (p);
        }
        else if (DirectoryJSONStorage* odjs = get_if<DirectoryJSONStorage> (&dbConfig)) {
            filesystem::path p = odjs->fRoot;
            if (p.empty ()) {
                p = "db-fs-v1";
            }
            return RegularizeLocalFilePathFromConfigs_ (p);
        }
        else if (SQLiteStorage* osqlite = get_if<SQLiteStorage> (&dbConfig)) {
            filesystem::path p = osqlite->fFile;
            if (p.empty ()) {
                p = "db-doc-v1.db";
            }
            return RegularizeLocalFilePathFromConfigs_ (p);
        }
        AssertNotReached ();
        return RegularizeLocalFilePathFromConfigs_ ("db-fs-v1");
    }};

uintmax_t WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::DB::GetFileSize () const
{
    return GetInternallySynchronizedConnection ().GetSpaceConsumed ();
}

namespace {
    void LoadFromStartupFile_ (const filesystem::path& loadFromStartupFile, Database::Document::Connection::Ptr copyTo)
    {
        IgnoreExceptionsExceptThreadAbortForCall ([&] () {
            using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common;
            Logger::sThe.Log (Logger::eInfo, "Updating configuration to lose startup/loadfile reference"_f);
            AppConfigurationType appCfg = gAppConfiguration.Get ();
            if (appCfg.fStartupLoadFrom.has_value ()) {
                appCfg.fStartupLoadFrom->fFile = nullopt;
            }
            gAppConfiguration.Set (appCfg);
        }());
        static bool sTried_ = false;
        Assert (not sTried_); // don't accidentally trigger twice
        sTried_ = true;
        try {
            // load file as tmp DocumentDB (copyFrom)
            // clear all collections in current DB (copyTo)
            // iterate over each collection in local document DB, and copy data to new DB
            // @todo - IDEALLY - this should be done as a transaction!
            Database::Document::Connection::Ptr copyFrom = Database::Document::LocalDocumentDB::New (Database::Document::LocalDocumentDB::Options{
                .fStorage = Database::Document::LocalDocumentDB::Options::SingleFileStorage{.fFile = loadFromStartupFile, .fReadOnly = true}});
            for (const auto& collectionName : copyTo.GetCollections ()) {
                copyTo.DropCollection (collectionName);
            }
            for (const auto& collectionName : copyFrom.GetCollections ()) {
                Database::Document::Collection::Ptr toCollection   = copyTo.CreateCollection (collectionName);
                Database::Document::Collection::Ptr fromCollection = copyFrom.GetCollection (collectionName);
                for (const auto& doc : fromCollection.GetAll ()) {
                    toCollection.Add (doc);
                }
            }
        }
        catch (const exception& e) {
            // NOTE this situation could leave the DB corrupted
            Logger::sThe.Log (Logger::eError, "Failed to load database from startup file '{}': {}"_f, loadFromStartupFile, e);
        }
    }
}

Database::Document::Connection::Ptr WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::DB::GetInternallySynchronizedConnection () const
{
    auto rwLock = sConn_.rwget ();
    if (rwLock.rwref () == nullptr) {
        rwLock.store (CreateCachedInternallySynchronizedConnection_ ());
    }
    EnsureNotNull (rwLock.cref ());
    return rwLock.cref ();
}

Database::Document::Connection::Ptr WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::DB::CreateCachedInternallySynchronizedConnection_ () const
{
    Debug::TraceContextBumper ctx{"BackendApp::Common::DB::CreateCachedInternallySynchronizedConnection_"};
    using namespace Database::Document;
    DatabaseConfigurationType dbConfig{BackendApp::Common::gAppConfiguration->fDatabase};

    // For default configuration, if no db specified, fill in sensible default.
    if (get_if<monostate> (&dbConfig)) {
        dbConfig = DirectoryJSONStorage{.fRoot = "db-localDBDir-v1"}; // @todo see if this is best or SQLITE
        // update App Config
        AppConfigurationType appCfg = BackendApp::Common::gAppConfiguration.Get ();
        appCfg.fDatabase            = dbConfig;
        BackendApp::Common::gAppConfiguration.Set (appCfg);
    }

    Database::Document::Connection::Ptr result = nullptr;

    optional<filesystem::path> loadFromStartupFile = [] () -> optional<filesystem::path> {
        AppConfigurationType fullCfg = BackendApp::Common::gAppConfiguration.Get ();
        if (fullCfg.fStartupLoadFrom and fullCfg.fStartupLoadFrom->fFile) {
            auto p = RegularizeLocalFilePathFromConfigs_ (*fullCfg.fStartupLoadFrom->fFile);
            if (not filesystem::exists (p)) {
                Logger::sThe.Log (Logger::eWarning, "StartupLoadFrom file specified but does not exist: {}"_f, p);
                return nullopt;
            };
            return p;
        }
        return nullopt;
    }();

    auto f = pFileName ();
    if (get_if<SingleFileJSONStorage> (&dbConfig)) {
        LocalDocumentDB::Options options;
        options = LocalDocumentDB::Options{.fInternallySynchronizedLetter = Execution::eInternallySynchronized,
                                           .fStorage                      = LocalDocumentDB::Options::SingleFileStorage{
                                                                    .fFile = f, .fForceCreateNew = loadFromStartupFile.has_value (), .fFlushOnEachWrite = true}};
#if qStroika_Foundation_Common_Platform_Windows
        // Could avoid the need for this by excluding the location of the DB from antivirus tools, but this is more
        // general and shouldn't cause any problems if the user does that.
        get<LocalDocumentDB::Options::SingleFileStorage> (options.fStorage).fRetryOnSharingViolationFor = 5s;
#endif
        // track usage
#if qStroika_Foundation_Debug_AssertionsChecked
        options.fOperationLoggingCallback = BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd (/*true*/);
#else
        options.fOperationLoggingCallback = BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd ();
#endif
        result = LocalDocumentDB::New (options);
    }
    else if (get_if<DirectoryJSONStorage> (&dbConfig)) {
        LocalDocumentDB::Options options;
        options = LocalDocumentDB::Options{.fInternallySynchronizedLetter = Execution::eInternallySynchronized,
                                           .fStorage                      = LocalDocumentDB::Options::DirectoryFileStorage{.fRoot = f}};
#if qStroika_Foundation_Common_Platform_Windows
        // Could avoid the need for this by excluding the location of the DB from antivirus tools, but this is more
        // general and shouldn't cause any problems if the user does that.
        get<LocalDocumentDB::Options::DirectoryFileStorage> (options.fStorage).fRetryOnSharingViolationFor = 5s;
#endif
        // track usage
#if qStroika_Foundation_Debug_AssertionsChecked
        options.fOperationLoggingCallback = BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd (/*true*/);
#else
        options.fOperationLoggingCallback = BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd ();
#endif
        result = LocalDocumentDB::New (options);
    }
    else if (get_if<SQLiteStorage> (&dbConfig)) {
        Database::Document::SQLite::Connection::Options options;
        options = Database::Document::SQLite::Connection::Options{.fInternallySynchronizedLetter = Execution::eInternallySynchronized, .fDBPath = f};
#if qStroika_Foundation_Debug_AssertionsChecked
        options.fOperationLoggingCallback = BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd (/*true*/);
#else
        options.fOperationLoggingCallback = BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd ();
#endif
        result = SQLite::Connection::New (options);
    }

    AssertNotNull (result);
    if (loadFromStartupFile) {
        Logger::sThe.Log (Logger::eWarning, "Overwriting/Loading database from startup file: '{}'"_f, *loadFromStartupFile);
        LoadFromStartupFile_ (*loadFromStartupFile, result);
    }
    return result;
}

auto WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::mkOperationalStatisticsMgrProcessDBCmd (bool traceDB) -> Database::Document::Connection::OpertionCallbackPtr
{
    using namespace Characters;
    using Database::Document::Connection::Operation;
    shared_ptr<OperationalStatisticsMgr::ProcessDBCmd> tmp; // use shared_ptr in lambda so copies of lambda share same object
    // @todo note - COULD use same shared_ptr object to store a Debug::TraceContextBumper object so we get /DBRead messages elided from log most of the time (when quick and /DBWrite).
    auto r = [=] (Operation op, [[maybe_unused]] const Database::Document::Connection::Ptr& documentDBConnection,
                  const optional<String>& collectionName, const exception_ptr& e) mutable noexcept {
        switch (op) {
            case Operation::eStartingRead:
                if (traceDB) {
                    DbgTrace ("<DBRead: {}>"_f, collectionName);
                }
                IgnoreExceptionsExceptThreadAbortForCall (tmp = make_shared<DB::ReadStatsContext> ());
                break;
            case Operation::eCompletedRead:
                if (traceDB) {
                    DbgTrace ("</DBRead>"_f);
                }
                tmp.reset ();
                break;
            case Operation::eStartingWrite:
                if (traceDB) {
                    DbgTrace ("<DBWrite: {}>"_f, collectionName);
                }
                IgnoreExceptionsExceptThreadAbortForCall (tmp = make_shared<DB::WriteStatsContext> ());
                break;
            case Operation::eCompletedWrite:
                if (traceDB) {
                    DbgTrace ("</DBWrite>"_f);
                }
                tmp.reset ();
                break;
            case Operation::eNotifyError:
                Execution::Logger::sThe.Log (Execution::Logger::eWarning, "Database operation exception: {}"_f, e);
                tmp->NoteError ();
                break;
        }
    };
    return r;
}
