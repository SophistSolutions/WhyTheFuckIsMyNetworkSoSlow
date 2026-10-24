/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Common/StroikaVersion.h"
#include "Stroika/Foundation/Containers/Mapping.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/Transfer/Connection.h"
#include "Stroika/Foundation/Time/Realtime.h"

#include "DB.h"

#include "BLOBMgr.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Common;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::DataExchange;
using namespace Stroika::Foundation::Execution;

using Memory::BLOB;
using Stroika::Foundation::Common::GUID;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common;

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

// @todo Lose DIGEST code and use new UUID::CreateNew () method when available.

namespace {

    namespace DBRecs_ {
        struct BLOB_ {
            GUID                        fID;
            BLOB                        fBLOB;
            optional<InternetMediaType> fContentType;

            static ObjectVariantMapper kMapper;
        };
        struct BLOBURL_ {
            URI              fURI;
            GUID             fBLOBID; // reference
            optional<String> fETag;   // maybe also should save expires

            static ObjectVariantMapper kMapper;
        };
        ObjectVariantMapper BLOB_::kMapper = [] () {
            using namespace DataExchange;
            ObjectVariantMapper mapper;
            mapper.AddCommonType<GUID> ();
            mapper.AddCommonType<BLOB> ();
            mapper.AddCommonType<InternetMediaType> ();
            mapper.AddCommonType<optional<InternetMediaType>> ();
            mapper.AddClass<BLOB_> ({
                {"id"sv, &BLOB_::fID},
                {"blob"sv, &BLOB_::fBLOB},
                {"contentType"sv, &BLOB_::fContentType},
            });
            return mapper;
        }();
        ObjectVariantMapper BLOBURL_::kMapper = [] () {
            using namespace DataExchange;
            ObjectVariantMapper mapper;
            mapper.AddCommonType<URI> ();
            mapper.AddCommonType<GUID> ();
            mapper.AddCommonType<String> ();
            mapper.AddCommonType<optional<String>> ();
            mapper.AddClass<BLOBURL_> ({
                {"uri"sv, &BLOBURL_::fURI},
                {"blobid"sv, &BLOBURL_::fBLOBID},
                {"etag"sv, &BLOBURL_::fETag},
            });
            return mapper;
        }();
    }

    constexpr VariantValue::Type kRepresentIDAs_ = VariantValue::Type::eString;

    /*
     *  Combined mapper for objects we write to the database. Contains all the objects mappers we need merged together,
     *  and any touchups on represenation we need (like writing GUID as BLOB rather than string).
     */
    const ObjectVariantMapper kDBObjectMapper_{[] () {
        ObjectVariantMapper mapper;
        mapper += DBRecs_::BLOB_::kMapper;
        mapper += DBRecs_::BLOBURL_::kMapper;
        mapper.AddCommonType<GUID> (kRepresentIDAs_); // store GUIDs as BLOBs - at least for database interactions - cuz more efficient
        return mapper;
    }()};

    struct DBConn_ {
        DBConn_ ()
        {
            BackendApp::Common::DB    db{};
            Document::Connection::Ptr conn = db.GetInternallySynchronizedConnection ();
            fBLOBs    = Document::ObjectCollection::New<DBRecs_::BLOB_> (conn.CreateCollection ("BLOBs"), kDBObjectMapper_);
            fBLOBURLs = Document::ObjectCollection::New<DBRecs_::BLOBURL_> (conn.CreateCollection ("BLOB-URLs"), kDBObjectMapper_);
        }
        Document::ObjectCollection::Ptr<DBRecs_::BLOB_>    fBLOBs;
        Document::ObjectCollection::Ptr<DBRecs_::BLOBURL_> fBLOBURLs;

        optional<GUID> Lookup (const BLOB& b, const optional<InternetMediaType>& ct) const
        {
            for (DBRecs_::BLOB_ i : fBLOBs.GetAll ()) {
                if (ct != nullopt and i.fContentType != nullopt and *ct != *i.fContentType) {
                    continue;
                }
                if (i.fBLOB == b) {
                    return i.fID;
                }
            }
            return nullopt;
        }
    };

    // Could use Synchronized<shared_ptr<DBConn_>> or thread_local, but if using thread_local, harder to set/unset in
    // BLOBMgr::Activator CTOR/DTOR, and this is so fast, no need to allow multiple simultaneous readers
    static Synchronized<shared_ptr<DBConn_>> sConn_; // Set/Unset in  BLOBMgr::Activator CTOR/DTOR

    // Called with sConn_ locked (lk), so the lookup and the add are one operation: a blob two threads add at once is stored once
    GUID AddBLOB_ (decltype (sConn_)::WritableReference& lk, const BLOB& b, const optional<InternetMediaType>& ct)
    {
        if (optional<GUID> id = lk.cref ()->Lookup (b, ct)) {
            return *id;
        }
        GUID g = GUID::GenerateNew ();
        lk.rwref ()->fBLOBs.Add (DBRecs_::BLOB_{.fID = g, .fBLOB = b, .fContentType = ct});
        return g;
    }

    // Selects url's BLOB-URLs records
    Database::Document::Filter URLFilter_ (const URI& url)
    {
        using namespace Database::Document;
        return Filter{{FilterElements::Equals{.fLHS = FilterElements::FieldName{"uri"sv}, .fRHS = FilterElements::Value{url.As<String> ()}}}};
    }

    // How long a URL's fetched content counts as current: the HTTP cache's lifetime for a resource that names none, and how soon
    // AsyncAddBLOBFromURL fetches a URL again
    constexpr Time::DurationSeconds kURLRecheckInterval_{60min};

    // When AsyncAddBLOBFromURL last queued a fetch of each URL, in this run: it queues at most one per kURLRecheckInterval_ -
    // so a failed fetch is not retried sooner either
    Synchronized<Mapping<String, Time::TimePointSeconds>> sURLFetchQueuedAt_;

}

/*
 ********************************************************************************
 ******************* BackendApp::Common::BLOBMgr::Activator *********************
 ********************************************************************************
 */
BLOBMgr::Activator::Activator ()
{
    Debug::TraceContextBumper ctx{"BLOBMgr::Activator::Activator"};
    BLOBMgr::sThe.fThreadPool_.store (make_unique<ThreadPool> (ThreadPool::Options{1, "URLBLOBFetcher"_k}));
    sConn_.store (make_shared<DBConn_> ());
}

BLOBMgr::Activator::~Activator ()
{
    Debug::TraceContextBumper ctx{"BLOBMgr::Activator::~Activator"};
    Execution::Thread::SuppressInterruptionInContext suppressInterruption; // must complete this abort and wait for done - this cannot abort/throw
    BLOBMgr::sThe.fThreadPool_.store (nullptr);
    sConn_.store (nullptr);
}

/*
 ********************************************************************************
 ************************* BackendApp::Common::BLOBMgr **************************
 ********************************************************************************
 */
GUID BLOBMgr::AddBLOB (const BLOB& b, const optional<InternetMediaType>& ct)
{
    auto lk = sConn_.rwget ();
    return AddBLOB_ (lk, b, ct);
}

GUID BLOBMgr::AddBLOBFromURL (const URI& url, bool recheckIfExpired)
{
    using namespace IO::Network;
    auto fetchData = [recheckIfExpired] (const URI& url) {
        // fetch the data from the given URI through a cache, so we don't needlessly ping remote servers for icons etc: within
        // its lifetime a resource is not fetched at all, after it with a conditional GET. Not recheckIfExpired: no cache, a full GET
        static Transfer::Cache::Ptr sHttpCache_ = [] () {
            Transfer::Cache::DefaultOptions options;
            options.fCacheSize          = 25;
            options.fDefaultResourceTTL = kURLRecheckInterval_;
            return Transfer::Cache::CreateDefault (options);
        }();
        using IO::Network::Transfer::Connection::Options;
        Options options{};
        if (recheckIfExpired) {
            options.fCache = sHttpCache_;
        }
        Transfer::Connection::Ptr conn     = Transfer::Connection::New (options);
        auto&&                    response = conn.GET (url);
        return make_pair (response.GetData (), response.GetContentType ());
    };
    auto data = fetchData (url); // the network GET - outside the lock
    // the blob, and the URL's mapping to it, as one operation: the URL maps to the blob of the same fetch
    auto lk   = sConn_.rwget ();
    GUID guid = AddBLOB_ (lk, data.first, data.second);
    // One BLOB-URLs record per URL: remove any it has, then add one - AddOrUpdate cannot replace a record, as BLOBURL_ has no
    // document ID
    const Database::Document::Collection::Ptr& urlRecords = lk.cref ()->fBLOBURLs;
    for (const Database::Document::Document& d : urlRecords.GetAll (URLFilter_ (url))) {
        if (optional<VariantValue> id = d.Lookup (Database::Document::kID)) {
            urlRecords.Remove (id->As<String> ());
        }
    }
    lk.rwref ()->fBLOBURLs.Add (DBRecs_::BLOBURL_{.fURI = url, .fBLOBID = guid});
    DbgTrace ("Added blob mapping: {} maps to blobid {}"_f, url, guid);
    return guid;
}

optional<GUID> BLOBMgr::AsyncAddBLOBFromURL (const URI& url, bool recheckIfExpired)
{
    // Return the blob stored for url, if any - even if out of date - and queue a fetch of url to the thread pool (AddBLOBFromURL):
    // if recheckIfExpired, only when none was queued in the last kURLRecheckInterval_. Refetching unchanged content is cheap - the
    // HTTP cache makes it a conditional GET, and AddBLOB_ reuses the stored blob - and a URL whose content changed (a device's new
    // icon) is mapped to the new blob.
    optional<GUID> storedGUID  = Lookup (url);
    bool           shouldQueue = [&] () {
        auto                   lk  = sURLFetchQueuedAt_.rwget ();
        Time::TimePointSeconds now = Time::GetTickCount ();
        if (recheckIfExpired) {
            if (optional<Time::TimePointSeconds> queuedAt = lk.cref ().Lookup (url.As<String> ()); queuedAt and now - *queuedAt < kURLRecheckInterval_) {
                return false;
            }
        }
        lk.rwref ().Add (url.As<String> (), now);
        return true;
    }();
    if (shouldQueue) {
        fThreadPool_.rwget ().rwref ()->AddTask ([url, recheckIfExpired, this] () { AddBLOBFromURL (url, recheckIfExpired); });
    }
    return storedGUID;
}

optional<GUID> BLOBMgr::Lookup (const URI& url)
{
    // GetAll, not Get - which throws if more than one record matches: a database written before WTF 1.0d21 may hold several
    // for a URL, each naming a fetch of its content, so any will do (the URL's next fetch leaves one)
    if (optional<DBRecs_::BLOBURL_> cachedURLObj = sConn_.rwget ().rwref ()->fBLOBURLs.GetAll (URLFilter_ (url)).First ()) {
        return cachedURLObj->fBLOBID;
    }
    return nullopt;
}

tuple<BLOB, optional<InternetMediaType>> BLOBMgr::GetBLOB (const GUID& id) const
{
    optional<DBRecs_::BLOB_> ob = sConn_.rwget ().rwref ()->fBLOBs.Get (id.As<String> ());
    if (ob) {
        return make_tuple (ob->fBLOB, ob->fContentType);
    }
    Execution::Throw (Execution::Exception<>{"No such blob"sv});
}

void BLOBMgr::BackupTo (const Database::Document::Connection::Ptr& dbConn)
{
    Database::Document::Collection::Ptr fromBlobs            = sConn_.rwget ().rwref ()->fBLOBs;
    Database::Document::Collection::Ptr fromBLOBURUls        = sConn_.rwget ().rwref ()->fBLOBURLs;
    Database::Document::Collection::Ptr toBlobsCollection    = dbConn.CreateCollection ("BLOBs"sv);
    Database::Document::Collection::Ptr toBlobURLsCollection = dbConn.CreateCollection ("BLOB-URLs"sv);
    fromBlobs.GetAll ().Apply ([&] (const Document::Document& i) { toBlobsCollection.Add (i); });
    fromBLOBURUls.GetAll ().Apply ([&] (const Document::Document& i) { toBlobURLsCollection.Add (i); });
}