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
            mapper.AddClass<BLOB_> (initializer_list<ObjectVariantMapper::StructFieldInfo>{
                {L"id", StructFieldMetaInfo{&BLOB_::fID}},
                {L"blob", StructFieldMetaInfo{&BLOB_::fBLOB}},
                {L"contentType", StructFieldMetaInfo{&BLOB_::fContentType}, ObjectVariantMapper::StructFieldInfo::eOmitNullFields},
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
            {.fName = L"contentType"sv, .fRequired = false, .fVariantValueType = VariantValue::eString},
#else
            {L"id", nullopt, true, kRepresentIDAs_, nullopt, true, nullopt, L"randomblob(16)"sv},
            {L"blob", nullopt, true, VariantValue::eBLOB},
            {L"contentType", nullopt, false, VariantValue::eString},
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
            fBLOBs                           = make_shared<SQL::ORM::TableConnection<DBRecs_::BLOB_>> (conn, kBLOBTableSchema_, kDBObjectMapper_, mkOperationalStatisticsMgrProcessDBCmd<SQL::ORM::TableConnection<DBRecs_::BLOB_>> ());
            fBLOBURLs                        = make_shared<BLOBURLTableConnection_> (conn, kBLOBURLTableSchema_, kDBObjectMapper_, mkOperationalStatisticsMgrProcessDBCmd<BLOBURLTableConnection_> ());
            fLookupBLOBByValueAndContentType = make_shared<SQL::Statement> (conn.mkStatement (L"SELECT * from BLOB where blob=:b and contentType=:ct;"sv));
        }
        shared_ptr<SQL::ORM::TableConnection<DBRecs_::BLOB_>> fBLOBs;
        shared_ptr<BLOBURLTableConnection_>                   fBLOBURLs;
        shared_ptr<SQL::Statement>                            fLookupBLOBByValueAndContentType;

        optional<GUID> Lookup (const BLOB& b, const optional<InternetMediaType>& ct) const
        {
            fLookupBLOBByValueAndContentType->Reset ();
            fLookupBLOBByValueAndContentType->Bind (L"b"sv, b);
            fLookupBLOBByValueAndContentType->Bind (L"ct"sv, ct ? ct->As<String> () : VariantValue{});
            DB::ReadStatsContext readStats; // explicit stats cuz not read through TableORM code
            if (auto row = fLookupBLOBByValueAndContentType->GetNextRow ()) {
                return row->Lookup (L"id"sv)->As<BLOB> ();
            }
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
    Debug::TraceContextBumper ctx{L"BLOBMgr::Activator::Activator"};
    BLOBMgr::sThe.fThreadPool_.store (make_unique<Execution::ThreadPool> (1, L"URLBLOBFetcher"_k));
    sConn_.store (make_shared<DBConn_> ());
}

BLOBMgr::Activator::~Activator ()
{
    Debug::TraceContextBumper ctx{L"BLOBMgr::Activator::~Activator"};
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
    sConn_.rwget ().rwref ()->fBLOBs->AddNew (DBRecs_::BLOB_{g, b, ct});
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
        return make_pair (response.GetData (), response.GetContentType ());
    };
    auto data = fetchData (url);
    GUID guid = AddBLOB (data.first, data.second);
    sConn_.rwget ().rwref ()->fBLOBURLs->AddOrUpdate (DBRecs_::BLOBURL_{url, guid});
    DbgTrace (L"Added blob mapping: %s maps to blobid %s", Characters::ToString (url).c_str (), Characters::ToString (guid).c_str ());
    return guid;
}

optional<GUID> BLOBMgr::AsyncAddBLOBFromURL (const URI& url, bool recheckIfExpired)
{
    // create mapping of URL to guid, and if not present, add task to threadpool to AddBLOBFromURL and store mapping into mapping object

    // @todo add logic for checking if expired and refetch then too
    // Use Stroika HTTP-Cache object support to handle age/etag stuff automatically
    optional<GUID> storeGUID;
    {
        if (optional<DBRecs_::BLOBURL_> cachedURLObj = sConn_.rwget ().rwref ()->fBLOBURLs->GetByID (url)) {
            storeGUID = cachedURLObj->fBLOBID;
        }
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
    if (optional<DBRecs_::BLOBURL_> cachedURLObj = sConn_.rwget ().rwref ()->fBLOBURLs->GetByID (url)) {
        return cachedURLObj->fBLOBID;
    }
    return nullopt;
}

tuple<BLOB, optional<InternetMediaType>> BLOBMgr::GetBLOB (const GUID& id) const
{
    optional<DBRecs_::BLOB_> ob = sConn_.rwget ().rwref ()->fBLOBs->GetByID (id);
    if (ob) {
        return make_tuple (ob->fBLOB, ob->fContentType);
    }
    Execution::Throw (Execution::Exception<>{L"No such blob"sv});
}
