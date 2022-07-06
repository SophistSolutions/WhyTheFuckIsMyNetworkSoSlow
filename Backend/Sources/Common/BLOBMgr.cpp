/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Common/Property.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/Transfer/Connection.h"

#include "DB.h"

#include "BLOBMgr.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Execution;

using Memory::BLOB;
using Stroika::Foundation::Common::ConstantProperty;
using Stroika::Foundation::Common::GUID;
using Stroika::Foundation::Database::SQL::ORM::Schema::CatchAllField;
using Stroika::Foundation::Database::SQL::ORM::Schema::Field;
using Stroika::Foundation::Database::SQL::ORM::Schema::Table;
using Stroika::Foundation::DataExchange::ObjectVariantMapper;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common;

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

// @todo Lose DIGEST code and use new UUID::CreateNew () method when available.

namespace {

    namespace DBRecs_ {
        struct BLOB_ {
            GUID              fID;
            BLOB              fBLOB;
            InternetMediaType fContentType;

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
            mapper.AddClass<BLOB_> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
                {L"id", StructFieldMetaInfo{&BLOB_::fID}},
                {L"blob", StructFieldMetaInfo{&BLOB_::fBLOB}},
                {L"contentType", StructFieldMetaInfo{&BLOB_::fContentType}},
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
            mapper.AddClass<BLOBURL_> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
                {L"uri", StructFieldMetaInfo{&BLOBURL_::fURI}},
                {L"blobid", StructFieldMetaInfo{&BLOBURL_::fBLOBID}},
                {L"etag", StructFieldMetaInfo{&BLOBURL_::fETag}},
            });
            return mapper;
        }();
    }

    constexpr VariantValue::Type kRepresentIDAs_ = VariantValue::Type::eBLOB; // else as string

    /*
     *  Combined mapper for objects we write to the database. Contains all the objects mappers we need merged together,
     *  and any touchups on represenation we need (like writing GUID as BLOB rather than string).
     */
    const ConstantProperty<ObjectVariantMapper> kDBObjectMapper_{[] () {
        ObjectVariantMapper mapper;
        mapper += DBRecs_::BLOB_::kMapper;
        mapper += DBRecs_::BLOBURL_::kMapper;
        mapper.AddCommonType<GUID> (kRepresentIDAs_); // store GUIDs as BLOBs - at least for database interactions - cuz more efficient
        return mapper;
    }};

    const Table kBLOBTableSchema_{
        L"BLOB",
        Collection<Field>{
#if __cpp_designated_initializers
            {.fName = L"id"sv, .fRequired = true, .fVariantValueType = kRepresentIDAs_, .fIsKeyField = true, .fDefaultExpression = L"randomblob(16)"sv},
            {.fName = L"blob"sv, .fRequired = true, .fVariantValueType = VariantValue::eBLOB},
            {.fName = L"contentType"sv, .fRequired = true, .fVariantValueType = VariantValue::eString},
#else
            {L"id", nullopt, true, kRepresentIDAs_, nullopt, true, nullopt, L"randomblob(16)"sv},
            {L"blob", nullopt, true, VariantValue::eBLOB},
            {L"contentType", nullopt, true, VariantValue::eString},
#endif
        }};

    const Table kBLOBURLTableSchema_{
        L"BLOBURL",
        Collection<Field>{
#if __cpp_designated_initializers
            {.fName = L"uri"sv, .fRequired = true, .fVariantValueType = VariantValue::eString, .fIsKeyField = true},
            {.fName = L"blobid"sv, .fRequired = true, .fVariantValueType = VariantValue::eBLOB},
            {.fName = L"etag"sv, .fVariantValueType = VariantValue::eString},
#else
            {L"uri", nullopt, true, VariantValue::eString, nullopt, true},
            {L"blobid", nullopt, true, VariantValue::eBLOB},
            {L"etag", nullopt, false, VariantValue::eString},
#endif
        }};

    struct DBConn_ {
        using BLOBURLTableConnection_ = SQL::ORM::TableConnection<DBRecs_::BLOBURL_, SQL::ORM::TableConnectionTraits<DBRecs_::BLOBURL_, IO::Network::URI>>;
        DBConn_ ()
        {
            constexpr Configuration::Version kCurrentVersion_ = Configuration::Version{1, 0, Configuration::VersionStage::Alpha, 0};
            BackendApp::Common::DB           db{
                kCurrentVersion_,
                Traversal::Iterable<Database::SQL::ORM::Schema::Table>{kBLOBTableSchema_, kBLOBURLTableSchema_}};
            SQL::Connection::Ptr conn        = db.NewConnection ();
            fBLOBs                           = make_shared<SQL::ORM::TableConnection<DBRecs_::BLOB_>> (conn, kBLOBTableSchema_, kDBObjectMapper_);
            fBLOBURLs                        = make_shared<BLOBURLTableConnection_> (conn, kBLOBURLTableSchema_, kDBObjectMapper_);
            fLookupBLOBByValueAndContentType = make_shared<SQL::Statement> (conn.mkStatement (L"SELECT * from BLOB where blob=:b and contentType=:ct;"));
        }
        shared_ptr<SQL::ORM::TableConnection<DBRecs_::BLOB_>> fBLOBs;
        shared_ptr<BLOBURLTableConnection_>                   fBLOBURLs;
        shared_ptr<SQL::Statement>                            fLookupBLOBByValueAndContentType;

        optional<GUID> Lookup (const BLOB& b, const InternetMediaType& ct) const
        {
            fLookupBLOBByValueAndContentType->Reset ();
            fLookupBLOBByValueAndContentType->Bind (L"b", b);
            fLookupBLOBByValueAndContentType->Bind (L"ct", ct.As<String> ());
            if (auto row = fLookupBLOBByValueAndContentType->GetNextRow ()) {
                return row->Lookup (L"id")->As<BLOB> ();
            }
            return nullopt;
        }
    };

    // NB: Make on db-Connection per thread, so no need for locks
    static thread_local shared_ptr<DBConn_> sConn_; // might want to shut these down on BLOBMgr::Activator::~Activator()?

}

/*
 ********************************************************************************
 ******************* BackendApp::Common::BLOBMgr::Activator *********************
 ********************************************************************************
 */
BLOBMgr::Activator::Activator ()
{
    BLOBMgr::sThe.fThreadPool_.store (make_unique<Execution::ThreadPool> (1, L"URLBLOBFetcher"_k));
}

BLOBMgr::Activator::~Activator ()
{
    BLOBMgr::sThe.fThreadPool_.store (nullptr);
}

/*
 ********************************************************************************
 ************************* BackendApp::Common::BLOBMgr **************************
 ********************************************************************************
 */
GUID BLOBMgr::AddBLOB (const BLOB& b, const InternetMediaType& ct)
{
    if (!sConn_) {
        sConn_ = make_shared<DBConn_> ();
    }
    if (auto id = sConn_->Lookup (b, ct)) {
        return *id;
    }
    GUID g = GUID::GenerateNew ();
    sConn_->fBLOBs->AddNew (DBRecs_::BLOB_{g, b, ct});
    return g;
}

GUID BLOBMgr::AddBLOBFromURL (const URI& url, bool recheckIfExpired)
{
    using IO::Network::Transfer::Cache;
    using IO::Network::Transfer::Connection;
    auto fetchData = [] (const URI& url) {
        // fetch the data from the given URI, maintaining a cache, so we don't needlessly ping remote servers for icons etc.
        static Cache::Ptr sHttpCache_ = [] () {
            Cache::DefaultOptions options;
            options.fCacheSize          = 25;
            options.fDefaultResourceTTL = 60min;
            return Cache::CreateDefault (options);
        }();
        Connection::Options options{};
        options.fCache           = sHttpCache_;
        Connection::Ptr conn     = Connection::New (options);
        auto&&          response = conn.GET (url);
        return make_pair (response.GetData (), response.GetContentType ().value_or (InternetMediaType{}));
    };
    auto data = fetchData (url);
    GUID guid = AddBLOB (data.first, data.second);
    if (!sConn_) {
        sConn_ = make_shared<DBConn_> ();
    }
    sConn_->fBLOBURLs->AddOrUpdate (DBRecs_::BLOBURL_{url, guid});
    DbgTrace (L"Added blob mapping: %s maps to blobid %s", Characters::ToString (url).c_str (), Characters::ToString (guid).c_str ());
    return guid;
}

optional<GUID> BLOBMgr::AsyncAddBLOBFromURL (const URI& url, bool recheckIfExpired)
{
    // create mapping of URL to guid, and if not presnt, add task to threadpool to AddBLOBFromURL and store mapping into mapping object

    // @todo add logic for checking if expired and refetch then too
    // Use Stroika HTTP-Cache object support to handle age/etag stuff automatically

    if (!sConn_) {
        sConn_ = make_shared<DBConn_> ();
    }

    optional<GUID> storeGUID;
    if (optional<DBRecs_::BLOBURL_> cachedURLObj = sConn_->fBLOBURLs->GetByID (url)) {
        storeGUID = cachedURLObj->fBLOBID;
    }
    if (not storeGUID.has_value ()) {
        fThreadPool_.rwget ().rwref ()->AddTask ([=] () {
            AddBLOBFromURL (url, recheckIfExpired); // @todo if this fails (CATCH) - then negative cache, so we dont try too often
        });
    }
    return storeGUID;
}

optional<GUID> BLOBMgr::Lookup (const URI& url)
{
    if (!sConn_) {
        sConn_ = make_shared<DBConn_> ();
    }
    if (optional<DBRecs_::BLOBURL_> cachedURLObj = sConn_->fBLOBURLs->GetByID (url)) {
        return cachedURLObj->fBLOBID;
    }
    return nullopt;
}

tuple<BLOB, InternetMediaType> BLOBMgr::GetBLOB (const GUID& id) const
{
    if (!sConn_) {
        sConn_ = make_shared<DBConn_> ();
    }
    optional<DBRecs_::BLOB_> ob = sConn_->fBLOBs->GetByID (id);
    if (ob) {
        return make_tuple (ob->fBLOB, ob->fContentType);
    }
    Execution::Throw (Execution::Exception<>{L"No such blob"sv});
}
