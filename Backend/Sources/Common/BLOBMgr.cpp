/*
* Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Bijection.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/Transfer/Client.h"

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
 ************************* BackendApp::Common::BLOBMgr **************************
 ********************************************************************************
 */
BLOBMgr BLOBMgr::sThe;

namespace {
    Synchronized<Bijection<GUID, tuple<BLOB, InternetMediaType>>> sStorage_;
}

GUID BLOBMgr::AddBLOB (const BLOB& b, const InternetMediaType& ct)
{
    if (auto id = sStorage_.cget ()->InverseLookup (make_tuple (b, ct))) {
        return *id;
    }
    GUID g = GUID::GenerateNew ();
    sStorage_.rwget ()->Add (g, make_tuple (b, ct));
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
        options.fCache      = sHttpCache_;
        Connection conn     = CreateConnection (options);
        auto&&     response = conn.GET (url);
        return make_pair (response.GetData (), response.GetContentType ().value_or (InternetMediaType{}));
    };
    auto data = fetchData (url);
    return AddBLOB (data.first, data.second);
}

tuple<BLOB, InternetMediaType> BLOBMgr::GetBLOB (const GUID& id) const
{
    if (auto r = sStorage_.cget ()->Lookup (id)) {
        return *r;
    }
    Execution::Throw (Execution::Exception<> (L"No such blob"sv));
}
