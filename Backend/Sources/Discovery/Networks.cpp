/*
* Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include <algorithm>
#include <vector>

#include "Stroika/Foundation/Cache/Memoizer.h"
#include "Stroika/Foundation/Cache/SynchronizedCallerStalenessCache.h"
#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Mapping.h"
#include "Stroika/Foundation/Containers/MultiSet.h"
#include "Stroika/Foundation/Cryptography/Digest/Algorithm/MD5.h"
#include "Stroika/Foundation/Cryptography/Format.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Reader.h"
#include "Stroika/Foundation/Execution/IntervalTimer.h"
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/LinkMonitor.h"
#include "Stroika/Foundation/IO/Network/Neighbors.h"
#include "Stroika/Foundation/IO/Network/Transfer/Connection.h"

#include "NetworkInterfaces.h"

#include "Networks.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::IO::Network;

using DataExchange::VariantValue;
using Execution::IntervalTimer;
using Execution::Synchronized;
using IO::Network::URI;
using Stroika::Foundation::Common::GUID;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Discovery;

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

namespace {
    optional<InternetAddress> LookupExternalInternetAddress_ (optional<Time::DurationSeconds> allowedStaleness = {})
    {
        using Cache::SynchronizedCallerStalenessCache;
        static SynchronizedCallerStalenessCache<void, optional<InternetAddress>> sCache_;
        return sCache_.LookupValue (sCache_.Ago (allowedStaleness.value_or (30s)), [] () -> optional<InternetAddress> {
            /*
             * Alternative sources for this information:
             *
             *  o   http://api.ipify.org/
             *  o   http://myexternalip.com/raw
             */
            static const URI kSources_[]{
                URI{"http://api.ipify.org/"sv},
                URI{"http://myexternalip.com/raw"sv},
            };
            // @todo - when one fails, we should try the other first next time
            for (const auto& url : kSources_) {
                try {
                    // this goes through the gateway, not necesarily this network, if we had multiple networks with gateways!
                    auto&& connection = IO::Network::Transfer::Connection::New ();
                    return IO::Network::InternetAddress{connection.GET (url).GetDataTextInputStream ().ReadAll ().Trim ()};
                }
                catch (...) {
                    DbgTrace ("ignore exception fetching public(external) IP address: {}"_f, current_exception ());
                }
            }
            return nullopt;
        });
    }
}

/*
 ********************************************************************************
 ***************************** Discovery::Network *******************************
 ********************************************************************************
 */
bool Discovery::Network::Contains (const InternetAddress& i) const
{
    for (const auto& nwa : fNetworkAddresses) {
        if (nwa.GetRange ().Contains (i)) {
            return true;
        }
    }
    return false;
}

String Discovery::Network::ToString () const
{
    StringBuilder sb;
    sb << "{"sv;
    sb << "GUID: "sv << fGUID << ", "sv;
    sb << "IP-Address: "sv << fNetworkAddresses << ", "sv;
    sb << "Names: "sv << fNames << ", "sv;
    sb << "Seen: "sv << fSeen << ", "sv;
    sb << "Gateways: "sv << fGateways << ", "sv;
    sb << "GatewayHardwareAddresses: "sv << fGatewayHardwareAddresses << ", "sv;
    sb << "DNS-Servers: "sv << fDNSServers << ", "sv;
    sb << "Attached-Network-Interfaces: "sv << fAttachedNetworkInterfaces << ", "sv;
#if qDebug
    sb << "DebugProps: "sv << fDebugProps;
#endif
    sb << "}"sv;
    return sb;
}

namespace {
    // when monitors code improved (https://stroika.atlassian.net/browse/STK-937)
    // fix this to use a shared intstance - shared with devices montitor
    Synchronized<Collection<NeighborsMonitor::Neighbor>> sCachedNeighbors_;
    struct KeepCachedMonitorsUpToDate_ {
        NeighborsMonitor fMonitor_{};
        void             DoOnce ()
        {
            Debug::TraceContextBumper ctx{"KeepCachedMonitorsUpToDate_ TIMER HANDLER"}; // to debug https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/78
            sCachedNeighbors_.store (fMonitor_.GetNeighbors ());
        }
    };
    KeepCachedMonitorsUpToDate_ sKeepCachedMonitorsUpToDate_;
    optional<String>            LookupHardwareAddress_ (const InternetAddress& ia)
    {
        for (auto i : sCachedNeighbors_.load ()) {
            if (i.fInternetAddress == ia) {
                return i.fHardwareAddress;
            }
        }
        return nullopt;
    }
}

/*
 ********************************************************************************
 ********************** Discovery::NetworksMgr::Activator ***********************
 ********************************************************************************
 */
namespace {
    constexpr Time::DurationSeconds  kDefaultItemCacheLifetime_{20s};
    bool                             sActive_{false};
    unique_ptr<IntervalTimer::Adder> sIntervalTimerAdder_;
}

Discovery::NetworksMgr::Activator::Activator ()
{
    DbgTrace ("Discovery::NetworksMgr::Activator::Activator: activating network discovery"_f);
    Require (not sActive_);
    sActive_ = true;
    sIntervalTimerAdder_ =
        make_unique<IntervalTimer::Adder> ([] () { sKeepCachedMonitorsUpToDate_.DoOnce (); }, 1min, IntervalTimer::Adder::eRunImmediately);
}

Discovery::NetworksMgr::Activator::~Activator ()
{
    DbgTrace ("Discovery::NetworksMgr::Activator::~Activator: deactivating network discovery"_f);
    Require (sActive_);
    sActive_ = false;
    sIntervalTimerAdder_.release ();
}

/*
 ********************************************************************************
 **************************** Discovery::NetworksMgr ****************************
 ********************************************************************************
 */
namespace {
    Sequence<Network> CollectActiveNetworks_ ()
    {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
        Debug::TraceContextBumper ctx{"Discovery::CollectActiveNetworks"};
#endif
        Require (sActive_);

        auto genProperNetworkIDAndSeen = [] (Network* nw) {
            static Execution::Synchronized<Collection<Network>> sExistingNetworks_;
            RequireNotNull (nw);
            Require (nw->fGUID == GUID{});
            auto existingNWsLock = sExistingNetworks_.rwget ();
            for (const Network& nwi : existingNWsLock.load ()) {
                if (nwi.fGateways == nw->fGateways and nwi.fNetworkAddresses == nw->fNetworkAddresses) {
                    nw->fGUID = nwi.fGUID;
                    nw->fSeen = nwi.fSeen;
                    return;
                }
            }
            nw->fGUID = GUID::GenerateNew ();
            auto now  = Time::DateTime::Now ();
            nw->fSeen = Range<DateTime>{now, now};
            existingNWsLock->Add (*nw);
        };
        auto genCIDRsFromBindings = [] (const Iterable<CIDR>& bindings) -> Set<CIDR> {
            Set<CIDR> cidrs;
            for (const CIDR& nib : bindings) {
                if (not kIncludeMulticastAddressesInDiscovery) {
                    if (nib.GetBaseInternetAddress ().IsMulticastAddress ()) {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
                        DbgTrace ("CollectActiveNetworks_: interface={}; ia={} binding ignored because IsMulticastAddress"_f, i.fGUID, nib.fInternetAddress);
#endif
                        continue; // skip multicast addresses, because they don't really refer to a device
                    }
                }
                if (not kIncludeLinkLocalAddressesInDiscovery) {
                    if (nib.GetBaseInternetAddress ().IsLinkLocalAddress ()) {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
                        DbgTrace ("CollectActiveNetworks_: interface={}; ia={} binding ignored because IsLinkLocalAddress"_f, i.fGUID, nib.fInternetAddress);
#endif
                        continue; // skip link-local addresses, they are only used for special purposes like discovery, and aren't part of the network
                    }
                }
                if (nib.GetBaseInternetAddress ().GetAddressSize ().has_value ()) {
                    // Guess CIDR prefix is max (so one address - bad guess) - if we cannot read from adapter
                    cidrs += CIDR{nib.GetBaseInternetAddress (), nib.GetNumberOfSignificantBits ()};
                }
            }
            // You CAN generate two CIDRs, one which subsumes the other. If so, lose any subsumed CIDRs from the list
            // Use simple quadradic algorithm, since we can never have very many CIDRs in a network
            for (Iterator<CIDR> ci = cidrs.begin (); ci != cidrs.end (); ++ci) {
                for (const CIDR& maybeSubsumerCIDR : cidrs) {
                    if (maybeSubsumerCIDR.GetNumberOfSignificantBits () > ci->GetNumberOfSignificantBits ()) {
                        if (maybeSubsumerCIDR.GetRange ().Contains (ci->GetRange ())) {
                            DbgTrace ("Removing subsumed CIDR {} inside {}"_f, *ci, maybeSubsumerCIDR);
                            cidrs.Remove (ci);
                        }
                    }
                }
            }
            return cidrs;
        };

        Collection<Network> accumResults;
        for (const NetworkInterface& i : Discovery::NetworkInterfacesMgr::sThe.CollectActiveNetworkInterfaces ()) {
            if (not i.fBindings.fAddressRanges.empty ()) {
                Network nw;
                {
                    Set<CIDR> cidrs = genCIDRsFromBindings (i.fBindings.fAddressRanges);

                    // See if the network has already been found. VERY TRICKY - cuz ambiguous concept a network. Mostly follow my
                    // intuition - now - that a network CAN comprise multiple CIDRs - like a WIFI card and ETHERNET all with the same V4 and V6 scopes - they
                    // are one network.

                    // erase for now cuz added at the end (and copied all the data over so effectively merging)
                    for (Iterator<Network> ni = accumResults.begin (); ni != accumResults.end (); ++ni) {
                        if (cidrs == ni->fNetworkAddresses) {
                            nw = *ni;
                            accumResults.erase (ni);
                            break;
                        }
                    }
                    nw.fNetworkAddresses = cidrs;
                }
                if (not nw.fNetworkAddresses.empty ()) {
                    if (i.fGateways) {
                        for (const InternetAddress& gw : *i.fGateways) {
                            nw.fGateways += gw;
                            // @todo add HW addr or gateway - if that address is there now.
                            if (auto hwa = LookupHardwareAddress_ (gw)) {
                                nw.fGatewayHardwareAddresses += *hwa;
                            }
                        }
                    }
                    if (i.fDNSServers) {
                        nw.fDNSServers += *i.fDNSServers;
                    }
                    nw.fAttachedNetworkInterfaces += i.fGUID;

                    if (not nw.fGateways.empty ()) {
                        ///tmphack - eventually support doing lookup through gateway from this network (all gateways on this network)
                        Memory::AccumulateIf (&nw.fExternalAddresses, LookupExternalInternetAddress_ ());
                    }

                    if (nw.fExternalAddresses and not nw.fExternalAddresses->empty ()) {
                        nw.fGEOLocInfo = LookupGEOLocation ((*nw.fExternalAddresses).Nth (0));
                        nw.fISP        = LookupInternetServiceProvider ((*nw.fExternalAddresses).Nth (0));
                    }

                    // Stroika IO::Network::Interface::fNetworkGUID field appears useless - since only defined on windows and not really documented what it means
                    // and doesn't appear to vary interestingly (maybe didnt test enough) - like my virtual adapters and localhost adapter alll have same network as
                    // the real ethernet adapter).
                    // -- LGP 2018-12-16
                    if (nw.fGUID == GUID{}) {
                        genProperNetworkIDAndSeen (&nw);
                    }
                    nw.fSeen = nw.fSeen.Extend (Time::DateTime::Now ());

                    nw.fNames.Add (i.fFriendlyName, 25); // unsure how to sort out priorities and/or dups if same net on multiple interfaces...

#if qDebug
                    // nothing useful to add yet
                    nw.fDebugProps.Add (
                        "test"sv, VariantValue{Mapping<String, VariantValue>{pair<String, VariantValue>{"updatedAt"sv, Time::DateTime::Now ()}}});
#endif

                    accumResults += nw;
                }
                else {
                    DbgTrace ("Skipping interface {} - cuz bindings bad address"_f, i);
                }
            }
        }

        // foreach network, replace friendly name (computed from arbitrarily chosen interface name) with info about
        // geoloc
        for (Iterator<Network> nwi = accumResults.begin (); nwi != accumResults.end (); ++nwi) {
            if (nwi->fGEOLocInfo and nwi->fGEOLocInfo->fCity) {
                Network nw = *nwi;
                nw.fNames.Add (*nwi->fGEOLocInfo->fCity, 50);
                accumResults.Update (nwi, nw, &nwi);
            }
        }

        // Score guess best network
        Mapping<GUID, float> netScores; // primitive attempt to find best network to display
        for (auto i = accumResults.begin (); i != accumResults.end (); ++i) {
            float score{};
            if (not i->fGateways.empty ()) {
                score += 20;
            }
            if (not i->fDNSServers.empty ()) {
                score += 5;
            }

            if (i->fExternalAddresses) {
                score += 30;
            }

// put most important interfaces at top of list
#if 0
            // could figure from interfaceids, but probably not worth it
            if ((i->fType == Interface::Type::eWiredEthernet or i->fType == Interface::Type::eWIFI)) {
                score += 3;
            }
#endif
            netScores.Add (i->fGUID, score);
        }
        Sequence<Network> results = Sequence<Network>{accumResults.OrderBy (
            [&] (const Network& l, const Network& r) { return netScores.Lookup (l.fGUID) > netScores.Lookup (r.fGUID); })};
        Assert (results.size () == accumResults.size ());
        Assert (results.size () == netScores.size ());
#if USE_NOISY_TRACE_IN_THIS_MODULE_
        DbgTrace ("returns: {}"_f, results);
#endif
        return results;
    }
}
NetworksMgr NetworksMgr::sThe;

Sequence<Network> Discovery::NetworksMgr::CollectActiveNetworks (optional<Time::DurationSeconds> allowedStaleness) const
{
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    Debug::TraceContextBumper ctx{"Discovery::CollectAllNetworkInterfaces"};
#endif
    Require (sActive_);
    Sequence<Network>                                                       results;
    static Cache::SynchronizedCallerStalenessCache<void, Sequence<Network>> sCache_;
    results = sCache_.LookupValue (sCache_.Ago (allowedStaleness.value_or (kDefaultItemCacheLifetime_)),
                                   [] () { return CollectActiveNetworks_ (); });
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace ("returns: {}"_f, results);
#endif
    return results;
}

Network Discovery::NetworksMgr::GetNetworkByID (const GUID& id, optional<Time::DurationSeconds> allowedStaleness) const
{
    for (const Network& i : CollectActiveNetworks (allowedStaleness)) {
        if (i.fGUID == id) {
            return i;
        }
    }
    Execution::Throw (Execution::Exception<>{"No such network id"sv});
}
