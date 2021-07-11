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
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/LinkMonitor.h"
#include "Stroika/Foundation/IO/Network/Transfer/Connection.h"

#include "NetworkInterfaces.h"

#include "Networks.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::IO::Network;

using DataExchange::VariantValue;
using IO::Network::URI;
using Stroika::Foundation::Common::GUID;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Discovery;

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

namespace {
    // for now, use the CIDR, @todo - but this needs TONS OF WORK - and probably persistence
    GUID ComputeGUIDForNetwork_ (const Discovery::Network& nw)
    {
        StringBuilder sb;
        sb += Characters::ToString (nw.fGateways);         // NO WHERE NEAR GOOD ENUF - take into account public IP ADDR and hardware address of router - but still ALLOW for any of these to float
        sb += Characters::ToString (nw.fNetworkAddresses); // ""
        using namespace Stroika::Foundation::Cryptography::Digest;
        return Hash<String, Digester<Algorithm::MD5>, GUID>{}(sb.str ());
    }
}

namespace {
    optional<InternetAddress> LookupExternalInternetAddress_ (optional<Time::DurationSecondsType> allowedStaleness = {})
    {
        using Cache::SynchronizedCallerStalenessCache;
        static SynchronizedCallerStalenessCache<void, optional<InternetAddress>> sCache_;
        return sCache_.LookupValue (sCache_.Ago (allowedStaleness.value_or (30)), [] () -> optional<InternetAddress> {
            /*
             * Alternative sources for this information:
             *
             *  o   http://api.ipify.org/
             *  o   http://myexternalip.com/raw
             */
            static const URI kSources_[]{
                URI{L"http://api.ipify.org/"sv},
                URI{L"http://myexternalip.com/raw"sv},
            };
            // @todo - when one fails, we should try the other first next time
            for (auto&& url : kSources_) {
                try {
                    // this goes through the gateway, not necesarily this network, if we had multiple networks with gateways!
                    auto&& connection = IO::Network::Transfer::Connection::New ();
                    return IO::Network::InternetAddress{connection.GET (url).GetDataTextInputStream ().ReadAll ().Trim ()};
                }
                catch (...) {
                    DbgTrace (L"ignore exception fetching public(external) IP address: %s", Characters::ToString (current_exception ()).c_str ());
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
    for (auto&& nwa : fNetworkAddresses) {
        if (nwa.GetRange ().Contains (i)) {
            return true;
        }
    }
    return false;
}

String Discovery::Network::ToString () const
{
    StringBuilder sb;
    sb += L"{";
    sb += L"GUID: " + Characters::ToString (fGUID) + L", ";
    sb += L"IP-Address: " + Characters::ToString (fNetworkAddresses) + L", ";
    sb += L"Friendly-Name: " + Characters::ToString (fFriendlyName) + L", ";
    sb += L"Gateways: " + Characters::ToString (fGateways) + L", ";
    sb += L"DNS-Servers: " + Characters::ToString (fDNSServers) + L", ";
    sb += L"Attached-Network-Interfaces: " + Characters::ToString (fAttachedNetworkInterfaces) + L", ";
#if qDebug
    sb += L"fDebugProps: " + Characters::ToString (fDebugProps) + L", ";
#endif
    sb += L"}";
    return sb.str ();
}

/*
 ********************************************************************************
 ********************** Discovery::NetworksMgr::Activator ***********************
 ********************************************************************************
 */
namespace {
    constexpr Time::DurationSecondsType kDefaultItemCacheLifetime_{20};
    bool                                sActive_{false};
}

Discovery::NetworksMgr::Activator::Activator ()
{
    DbgTrace (L"Discovery::NetworksMgr::Activator::Activator: activating network discovery");
    Require (not sActive_);
    sActive_ = true;
}

Discovery::NetworksMgr::Activator::~Activator ()
{
    DbgTrace (L"Discovery::NetworksMgr::Activator::~Activator: deactivating network discovery");
    Require (sActive_);
    sActive_ = false;
    // @todo must shutdown any background threads
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
        Debug::TraceContextBumper ctx{L"Discovery::CollectActiveNetworks"};
#endif
        Require (sActive_);
        Collection<Network> accumResults;
        for (NetworkInterface i : Discovery::NetworkInterfacesMgr::sThe.CollectActiveNetworkInterfaces ()) {
            if (not i.fBindings.fAddressRanges.empty ()) {
                Network nw;
                {
                    auto genCIDRsFromBindings = [] (const Iterable<CIDR>& bindings) {
                        Set<CIDR> cidrs;
                        for (CIDR nib : bindings) {
                            if (not kIncludeMulticastAddressesInDiscovery) {
                                if (nib.GetBaseInternetAddress ().IsMulticastAddress ()) {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
                                    DbgTrace (L"CollectActiveNetworks_: interface=%s; ia=%s binding ignored because IsMulticastAddress", Characters::ToString (i.fGUID).c_str (), Characters::ToString (nib.fInternetAddress).c_str ());
#endif
                                    continue; // skip multicast addresses, because they don't really refer to a device
                                }
                            }
                            if (not kIncludeLinkLocalAddressesInDiscovery) {
                                if (nib.GetBaseInternetAddress ().IsLinkLocalAddress ()) {
#if USE_NOISY_TRACE_IN_THIS_MODULE_
                                    DbgTrace (L"CollectActiveNetworks_: interface=%s; ia=%s binding ignored because IsLinkLocalAddress", Characters::ToString (i.fGUID).c_str (), Characters::ToString (nib.fInternetAddress).c_str ());
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
                            for (CIDR maybeSubsumerCIDR : cidrs) {
                                if (maybeSubsumerCIDR.GetNumberOfSignificantBits () > ci->GetNumberOfSignificantBits ()) {
                                    if (maybeSubsumerCIDR.GetRange ().Contains (ci->GetRange ())) {
                                        DbgTrace ("Removing subsumed CIDR %s inside %s", Characters::ToString (*ci).c_str (), Characters::ToString (maybeSubsumerCIDR).c_str ());
                                        cidrs.Remove (ci);
                                    }
                                }
                            }
                        }
                        return cidrs;
                    };
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
                    unsigned int score{};
                    if (i.fGateways) {
                        for (auto gw : *i.fGateways) {
                            if (not nw.fGateways.Contains (gw)) {
                                score += 20;
                                nw.fGateways.Append (gw);
                            }
                        }
                    }
                    if (i.fDNSServers) {
                        for (auto dnss : *i.fDNSServers) {
                            if (not nw.fDNSServers.Contains (dnss)) {
                                score += 5;
                                nw.fDNSServers.Append (dnss);
                            }
                        }
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
                    nw.fGUID = ComputeGUIDForNetwork_ (nw);

                    nw.fFriendlyName = i.fFriendlyName; // if multiple, pick arbitrarily

#if qDebug
                    // nothing useful to add yet
                    nw.fDebugProps.Add (L"test"sv,
                                        VariantValue{
                                            Mapping<String, VariantValue>{
                                                pair<String, VariantValue>{L"updatedAt"sv, Time::DateTime::Now ()}}});
#endif

                    accumResults += nw;
                }
                else {
                    DbgTrace (L"Skipping interface %s - cuz bindings bad address", Characters::ToString (i).c_str ());
                }
            }
        }

        // Assure all networks have an ID
        for (auto i = accumResults.begin (); i != accumResults.end (); ++i) {
            if (i->fGUID == GUID{}) {
                StringBuilder sb;
                sb += Characters::ToString (i->fGateways);         // NO WHERE NEAR GOOD ENUF - take into account public IP ADDR and hardware address of router - but still ALLOW for any of these to float
                sb += Characters::ToString (i->fNetworkAddresses); // ""
                auto v = *i;
                using namespace Stroika::Foundation::Cryptography::Digest;
                v.fGUID = Hash<String, Digester<Algorithm::MD5>, GUID>{}(sb.str ());
                accumResults.Update (i, v);
            }
        }

        // Score guess best network
        Mapping<GUID, float> netScores; // primitive attempt to find best interface to display
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

        Sequence<Network> results = Sequence<Network>{accumResults.OrderBy ([&] (const Network& l, const Network& r) {
            return netScores.Lookup (l.fGUID) > netScores.Lookup (r.fGUID);
        })};
        Assert (results.size () == accumResults.size ());
        Assert (results.size () == netScores.size ());
#if USE_NOISY_TRACE_IN_THIS_MODULE_
        DbgTrace (L"returns: %s", Characters::ToString (results).c_str ());
#endif
        return results;
    }

}
NetworksMgr NetworksMgr::sThe;

Sequence<Network> Discovery::NetworksMgr::CollectActiveNetworks (optional<Time::DurationSecondsType> allowedStaleness) const
{
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    Debug::TraceContextBumper ctx{L"Discovery::CollectAllNetworkInterfaces"};
#endif
    Require (sActive_);
    Sequence<Network> results;
    using Cache::SynchronizedCallerStalenessCache;
    static SynchronizedCallerStalenessCache<void, Sequence<Network>> sCache_;
    results = sCache_.LookupValue (sCache_.Ago (allowedStaleness.value_or (kDefaultItemCacheLifetime_)), [] () {
        return CollectActiveNetworks_ ();
    });
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace (L"returns: %s", Characters::ToString (results).c_str ());
#endif
    return results;
}

Network Discovery::NetworksMgr::GetNetworkByID (const GUID& id, optional<Time::DurationSecondsType> allowedStaleness) const
{
    for (Network i : CollectActiveNetworks (allowedStaleness)) {
        if (i.fGUID == id) {
            return i;
        }
    }
    Execution::Throw (Execution::Exception<> (L"No such network id"sv));
}