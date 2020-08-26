/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2020.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/Transfer/Connection.h"

#include "BLOBMgr.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Execution;

using Memory::BLOB;
using Stroika::Foundation::Common::GUID;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common;

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

// @todo Lose DIGEST code and use new UUID::CreateNew () method when available.

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
    if (auto id = fStorage_.cget ()->InverseLookup (make_tuple (b, ct))) {
        return *id;
    }
    GUID g = GUID::GenerateNew ();
    fStorage_.rwget ()->Add (g, make_tuple (b, ct));
    return g;
}

GUID BLOBMgr::AddBLOBFromURL (const URI& url)
{
    using namespace IO::Network::Transfer;
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
    return AddBLOB (data.first, data.second);
}

optional<GUID> BLOBMgr::AsyncAddBLOBFromURL (const URI& url, bool allowExpired)
{
    // create mapping of URL to guid, and if not presnt, add task to threadpool to AddBLOBFromURL and store mapping into mapping object

    // @todo add logic for checking if expired and refetch then too
    auto r = fURI2BLOBMap_.load ().Lookup (url);
    if (not r.has_value ()) {
        fThreadPool_.rwget ().rwref ()->AddTask ([=] () {
            auto guid = AddBLOBFromURL (url); // @todo if this fails (CATCH) - then negative cache, so we dont try too often
            auto l    = fURI2BLOBMap_.rwget ();
            DbgTrace (L"Added icon mapping: %s maps to blobid %s", Characters::ToString (url).c_str (), Characters::ToString (guid).c_str ());
            l.rwref ().Add (url, guid);
        });
    }
    return r;
}

optional<GUID> BLOBMgr::Lookup (const URI& url, bool allowExpired)
{
    auto r = fURI2BLOBMap_.load ().Lookup (url);
    if (r) {
        return *r;
    }
    return nullopt;
}

tuple<BLOB, InternetMediaType> BLOBMgr::GetBLOB (const GUID& id) const
{
    if (auto r = fStorage_.cget ()->Lookup (id)) {
        return *r;
    }
    Execution::Throw (Execution::Exception<>{L"No such blob"sv});
}
