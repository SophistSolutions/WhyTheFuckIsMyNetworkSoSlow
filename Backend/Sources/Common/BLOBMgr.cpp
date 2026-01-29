/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Common/StroikaVersion.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/Transfer/Connection.h"

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
#if !qUseNewDocumentDBAPI
using Stroika::Foundation::Database::SQL::ORM::Schema::CatchAllField;
using Stroika::Foundation::Database::SQL::ORM::Schema::Field;
using Stroika::Foundation::Database::SQL::ORM::Schema::Table;
#endif

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

#if qUseNewDocumentDBAPI
    constexpr VariantValue::Type kRepresentIDAs_ = VariantValue::Type::eString;
#else
    constexpr VariantValue::Type kRepresentIDAs_ = VariantValue::Type::eBLOB; // else as string
#endif

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

#if !qUseNewDocumentDBAPI
    const Table kBLOBTableSchema_{
        "BLOB"sv, Collection<Field>{
                      {.fName = "id"sv, .fRequired = true, .fVariantValueType = kRepresentIDAs_, .fIsKeyField = true, .fDefaultExpression = "randomblob(16)"sv},
                      {.fName = "blob"sv, .fRequired = true, .fVariantValueType = VariantValue::eBLOB},
                      {.fName = "contentType"sv, .fRequired = false, .fVariantValueType = VariantValue::eString},
                  }};

    const Table kBLOBURLTableSchema_{"BLOBURL"sv,
                                     Collection<Field>{
                                         {.fName = "uri"sv, .fRequired = true, .fVariantValueType = VariantValue::eString, .fIsKeyField = true},
                                         {.fName = "blobid"sv, .fRequired = true, .fVariantValueType = VariantValue::eBLOB},
                                         {.fName = "etag"sv, .fVariantValueType = VariantValue::eString},
                                     }};
#endif

    struct DBConn_ {
#if !qUseNewDocumentDBAPI
        using BLOBURLTableConnection_ =
            SQL::ORM::TableConnection<DBRecs_::BLOBURL_, SQL::ORM::TableConnectionTraits<DBRecs_::BLOBURL_, IO::Network::URI>>;
#endif
        DBConn_ ()
        {
#if qUseNewDocumentDBAPI
            BackendApp::Common::DB    db{};
            Document::Connection::Ptr conn = db.GetInternallySynchronizedConnection ();
            fBLOBs    = Document::ObjectCollection::New<DBRecs_::BLOB_> (conn.CreateCollection ("BLOBs"), kDBObjectMapper_);
            fBLOBURLs = Document::ObjectCollection::New<DBRecs_::BLOBURL_> (conn.CreateCollection ("BLOB-URLs"), kDBObjectMapper_);
#else
            constexpr Version kCurrentVersion_ = Version{1, 0, VersionStage::Alpha, 0};
            BackendApp::Common::DB db{kCurrentVersion_, Traversal::Iterable<Database::SQL::ORM::Schema::Table>{kBLOBTableSchema_, kBLOBURLTableSchema_}};
            SQL::Connection::Ptr conn = db.NewConnection ();
            fBLOBs                    = make_shared<SQL::ORM::TableConnection<DBRecs_::BLOB_>> (
                conn, kBLOBTableSchema_, kDBObjectMapper_, mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<DBRecs_::BLOB_>> ());
            fBLOBURLs = make_shared<BLOBURLTableConnection_> (conn, kBLOBURLTableSchema_, kDBObjectMapper_,
                                                              mkOperationalStatisticsMgrProcessDBCmd<BLOBURLTableConnection_> ());
            fLookupBLOBByValueAndContentType =
                make_shared<SQL::Statement> (conn.mkStatement ("SELECT * from BLOB where blob=:b and contentType=:ct;"sv));
#endif
        }
#if qUseNewDocumentDBAPI
        Document::ObjectCollection::Ptr<DBRecs_::BLOB_>    fBLOBs;
        Document::ObjectCollection::Ptr<DBRecs_::BLOBURL_> fBLOBURLs;
#endif
#if !qUseNewDocumentDBAPI
        shared_ptr<SQL::ORM::TableConnection<DBRecs_::BLOB_>> fBLOBs;
        shared_ptr<BLOBURLTableConnection_>                   fBLOBURLs;
        shared_ptr<SQL::Statement>                            fLookupBLOBByValueAndContentType;
#endif

        optional<GUID> Lookup (const BLOB& b, const optional<InternetMediaType>& ct) const
        {
#if qUseNewDocumentDBAPI
            for (DBRecs_::BLOB_ i : fBLOBs.GetAll ()) {
                if (ct != nullopt and i.fContentType != nullopt and *ct != *i.fContentType) {
                    continue;
                }
                if (i.fBLOB == b) {
                    return i.fID;
                }
            }
#else
            fLookupBLOBByValueAndContentType->Reset ();
            fLookupBLOBByValueAndContentType->Bind ("b"sv, b);
            fLookupBLOBByValueAndContentType->Bind ("ct"sv, ct ? ct->As<String> () : VariantValue{});
            DB::ReadStatsContext readStats; // explicit stats cuz not read through TableORM code
            if (auto row = fLookupBLOBByValueAndContentType->GetNextRow ()) {
                return row->Lookup ("id"sv)->As<BLOB> ();
            }
#endif
            return nullopt;
        }
    };

    // Could use Synchronized<shared_ptr<DBConn_>> or thread_local, but if using thread_local, harder to set/unset in
    // BLOBMgr::Activator CTOR/DTOR, and this is so fast, no need to allow multiple simultaneous readers
    static Synchronized<shared_ptr<DBConn_>> sConn_; // Set/Unset in  BLOBMgr::Activator CTOR/DTOR

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
    if (auto id = sConn_.rwget ().rwref ()->Lookup (b, ct)) {
        return *id;
    }
    GUID g = GUID::GenerateNew ();
#if qUseNewDocumentDBAPI
    sConn_.rwget ().rwref ()->fBLOBs.Add (DBRecs_::BLOB_{.fID = g, .fBLOB = b, .fContentType = ct});
#else
    sConn_.rwget ().rwref ()->fBLOBs->AddNew (DBRecs_::BLOB_{g, b, ct});
#endif
    return g;
}

GUID BLOBMgr::AddBLOBFromURL (const URI& url, bool recheckIfExpired)
{
    using namespace IO::Network;
    auto fetchData = [] (const URI& url) {
        // fetch the data from the given URI, maintaining a cache, so we don't needlessly ping remote servers for icons etc.
        static Transfer::Cache::Ptr sHttpCache_ = [] () {
            Transfer::Cache::DefaultOptions options;
            options.fCacheSize          = 25;
            options.fDefaultResourceTTL = 60min;
            return Transfer::Cache::CreateDefault (options);
        }();
        using IO::Network::Transfer::Connection::Options;
        Options options{};
        options.fCache                     = sHttpCache_;
        Transfer::Connection::Ptr conn     = Transfer::Connection::New (options);
        auto&&                    response = conn.GET (url);
        return make_pair (response.GetData (), response.GetContentType ());
    };
    auto       data = fetchData (url);
    GUID       guid = AddBLOB (data.first, data.second);
    lock_guard lock{sConn_};
#if qUseNewDocumentDBAPI
    sConn_.rwget ().rwref ()->fBLOBURLs.AddOrUpdate (DBRecs_::BLOBURL_{.fURI = url, .fBLOBID = guid});
#else
    sConn_.rwget ().rwref ()->fBLOBURLs->AddOrUpdate (DBRecs_::BLOBURL_{.fURI = url, .fBLOBID = guid});
#endif
    DbgTrace ("Added blob mapping: {} maps to blobid {}"_f, url, guid);
    return guid;
}

optional<GUID> BLOBMgr::AsyncAddBLOBFromURL (const URI& url, bool recheckIfExpired)
{
    // create mapping of URL to guid, and if not present, add task to threadpool to AddBLOBFromURL and store mapping into mapping object

    // @todo add logic for checking if expired and refetch then too
    // Use Stroika HTTP-Cache object support to handle age/etag stuff automatically
    optional<GUID> storeGUID;
    {
#if qUseNewDocumentDBAPI
        using Document::Filter;
        using namespace Database::Document;
        if (optional<DBRecs_::BLOBURL_> cachedURLObj = sConn_.rwget ().rwref ()->fBLOBURLs.Get (
                Filter{{FilterElements::Equals{.fLHS = FilterElements::FieldName{"uri"sv}, .fRHS = FilterElements::Value{url.As<String> ()}}}})) {
            storeGUID = cachedURLObj->fBLOBID;
        }
#else
        if (optional<DBRecs_::BLOBURL_> cachedURLObj = sConn_.rwget ().rwref ()->fBLOBURLs->Get (url)) {
            storeGUID = cachedURLObj->fBLOBID;
        }
#endif
    }
    if (not storeGUID.has_value ()) {
        fThreadPool_.rwget ().rwref ()->AddTask ([url, recheckIfExpired, this] () {
            AddBLOBFromURL (url, recheckIfExpired); // @todo if this fails (CATCH) - then negative cache, so we dont try too often
        });
    }
    return storeGUID;
}

optional<GUID> BLOBMgr::Lookup (const URI& url)
{
#if qUseNewDocumentDBAPI
    using Document::Filter;
    using namespace Database::Document;
    if (optional<DBRecs_::BLOBURL_> cachedURLObj = sConn_.rwget ().rwref ()->fBLOBURLs.Get (
            Filter{{FilterElements::Equals{.fLHS = FilterElements::FieldName{"uri"sv}, .fRHS = FilterElements::Value{url.As<String> ()}}}})) {
        return cachedURLObj->fBLOBID;
    }
#else
    if (optional<DBRecs_::BLOBURL_> cachedURLObj = sConn_.rwget ().rwref ()->fBLOBURLs->Get (url)) {
        return cachedURLObj->fBLOBID;
    }
#endif
    return nullopt;
}

tuple<BLOB, optional<InternetMediaType>> BLOBMgr::GetBLOB (const GUID& id) const
{
#if qUseNewDocumentDBAPI
    optional<DBRecs_::BLOB_> ob = sConn_.rwget ().rwref ()->fBLOBs.Get (id.As<String> ());
#else
    optional<DBRecs_::BLOB_> ob = sConn_.rwget ().rwref ()->fBLOBs->Get (id);
#endif
    if (ob) {
        return make_tuple (ob->fBLOB, ob->fContentType);
    }
    Execution::Throw (Execution::Exception<>{"No such blob"sv});
}
