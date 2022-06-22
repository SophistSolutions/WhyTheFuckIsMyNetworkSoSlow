/*
* Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include <algorithm>
#include <vector>

#include "Stroika/Foundation/Cache/SynchronizedCallerStalenessCache.h"
#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
//#include "Stroika/Foundation/Cryptography/Digest/Algorithm/MD5.h"
//#include "Stroika/Foundation/Cryptography/Digest/Hash.h"
#include "Stroika/Foundation/Cryptography/Format.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/LinkMonitor.h"

#include "NetworkInterfaces.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::DataExchange;
using namespace Stroika::Foundation::IO::Network;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Discovery;

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

/*
 ********************************************************************************
 ************************ Discovery::NetworkInterface ***************************
 ********************************************************************************
 */
NetworkInterface::NetworkInterface (const IO::Network::Interface& src)
    : Interface{src}
{
}

/*
 ********************************************************************************
 ****************** Discovery::NetworkInterfacesMgr::Activator ******************
 ********************************************************************************
 */
namespace {
    constexpr Time::DurationSecondsType kDefaultItemCacheLifetime_{15};
    bool                                sActive_{false};
}
Discovery::NetworkInterfacesMgr::Activator::Activator ()
{
    DbgTrace (L"Discovery::NetworkInterfacesMgr::Activator::Activator: activating network discovery");
    Require (not sActive_);
    sActive_ = true;
}
Discovery::NetworkInterfacesMgr::Activator::~Activator ()
{
    DbgTrace (L"Discovery::NetworkInterfacesMgr::Activator::~Activator: deactivating network discovery");
    Require (sActive_);
    sActive_ = false;
    // @todo must shutdown any background threads
}

namespace {
    /*
     *  Generate a UNIQUE ID for each local interface (so unique for lifetime of this process). Will use IntegratedModel::Mgr
     *  to generate/map to longer-lived IDs.
     */
#if 0
OLD
     *  For now, always hash the InternalInterfaceID into a new ID. But later will want to somehow match
     *  against data in a database to assure we are always appropriately re-using interface ids (by making them
     *  relative to some host id perhaps).
     *
     *  We used to just re-use the GUID if it happened to be already in the form of a GUID (generally true on windows)
     *  but appears no guarantee even there, and if we want to track connection to that low level id, we should capture it
     *  as a separate datum.
#endif
    Common::GUID MapCurrentProcessInternalInterfaceIDToReportedProcessLifetimeGUID_ (const String& internalInterfaceID)
    {
#if 1
        static Execution::Synchronized<Mapping<String, Common::GUID>> sCacheOfAllGUIDs_;
        auto                                                          l = sCacheOfAllGUIDs_.rwget ();
        if (auto o = l->Lookup (internalInterfaceID)) {
            return *o;
        }
        else {
            auto result = Common::GUID::GenerateNew ();
            l->Add (internalInterfaceID, result);
            return result;
        }
#else
        using namespace Stroika::Foundation::Cryptography::Digest;
        // @todo must try to re-use IDs (during run session) but not this strongly - so keep cache of map
        // and add new random GUID when internalInterface not found.
        // So keep list of all iterface IDs EVER seen (during this process run)
        return Hash<String, Digester<Algorithm::MD5>, Common::GUID>{}(internalInterfaceID);
#endif
    }
}

namespace {
    Collection<NetworkInterface> CollectAllNetworkInterfaces_ ()
    {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
        Debug::TraceContextBumper ctx{L"CollectAllNetworkInterfaces_"};
#endif
        Require (sActive_);
        SystemInterfacesMgr      sysNetInterfaces;
        vector<NetworkInterface> results;
        for (const Interface& i : sysNetInterfaces.GetAll ()) {
            NetworkInterface ni{i};
            {
                ni.fGUID = MapCurrentProcessInternalInterfaceIDToReportedProcessLifetimeGUID_ (i.fInternalInterfaceID);
#if qDebug
                // nothing useful to add yet
                ni.fDebugProps.Add (L"test"sv,
                                    VariantValue{
                                        Mapping<String, VariantValue>{
                                            pair<String, VariantValue>{L"updatedAt"sv, Time::DateTime::Now ()}}});
#endif
            }
            results.push_back (ni);
        }
#if USE_NOISY_TRACE_IN_THIS_MODULE_
        DbgTrace (L"returns: %s", Characters::ToString (results).c_str ());
#endif
        return results;
    }
}

/*
 ********************************************************************************
 *********************** Discovery::NetworkInterfacesMgr ************************
 ********************************************************************************
 */
NetworkInterfacesMgr NetworkInterfacesMgr::sThe;

Collection<NetworkInterface> Discovery::NetworkInterfacesMgr::CollectAllNetworkInterfaces (optional<Time::DurationSecondsType> allowedStaleness) const
{
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    Debug::TraceContextBumper ctx{L"Discovery::CollectAllNetworkInterfaces"};
#endif
    Require (sActive_);
    Collection<NetworkInterface> results;
    using Cache::SynchronizedCallerStalenessCache;
    static SynchronizedCallerStalenessCache<void, Collection<NetworkInterface>> sCache_;
    results = sCache_.LookupValue (sCache_.Ago (allowedStaleness.value_or (kDefaultItemCacheLifetime_)), [] () -> Collection<NetworkInterface> {
        return CollectAllNetworkInterfaces_ ();
    });
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace (L"returns: %s", Characters::ToString (results).c_str ());
#endif
    return results;
}

Collection<NetworkInterface> Discovery::NetworkInterfacesMgr::CollectActiveNetworkInterfaces (optional<Time::DurationSecondsType> allowedStaleness) const
{
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    Debug::TraceContextBumper ctx{L"Discovery::CollectActiveNetworkInterfaces"};
#endif
    Require (sActive_);
    Collection<NetworkInterface> results = CollectAllNetworkInterfaces (allowedStaleness).Where ([] (const NetworkInterface& i) { return i.fType != Interface::Type::eLoopback and i.fStatus and i.fStatus->Contains (Interface::Status::eRunning); });
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace (L"returns: %s", Characters::ToString (results).c_str ());
#endif
    return results;
}
