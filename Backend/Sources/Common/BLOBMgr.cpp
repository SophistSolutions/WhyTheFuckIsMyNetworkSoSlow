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
    // @todo must add logic to Stroika IO::Network::Transfer::Connection to handle last-modified/ETAGs etc so this can be
    // more efficient. Then Cache this 'connection' object (or cache/maintain common cache object)
    // here so uses that and doesn't fetch for each
    IO::Network::Transfer::Connection conn = IO::Network::Transfer::CreateConnection ();
    conn.SetURL (url);
    auto&& response = conn.GET ();
    return AddBLOB (response.GetData (), response.GetContentType ().value_or (InternetMediaType{}));
}

tuple<BLOB, InternetMediaType> BLOBMgr::GetBLOB (const GUID& id) const
{
    if (auto r = sStorage_.cget ()->Lookup (id)) {
        return *r;
    }
    Execution::Throw (Execution::Exception<> (L"No such blob"sv));
}
