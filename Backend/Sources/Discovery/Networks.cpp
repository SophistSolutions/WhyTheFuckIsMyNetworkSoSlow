/*
* Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
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
#include "Stroika/Foundation/IO/Network/Transfer/Client.h"

#include "NetworkInterfaces.h"

#include "Networks.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::IO::Network;

using DataExchange::VariantValue;
using IO::Network::URL;
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
        using namespace Stroika::Foundation::Cryptography;
        using DIGESTER_ = Digest::Digester<Digest::Algorithm::MD5>;
        sb += Characters::ToString (nw.fGateways);         // NO WHERE NEAR GOOD ENUF - take into account public IP ADDR and hardware address of router - but still ALLOW for any of these to float
        sb += Characters::ToString (nw.fNetworkAddresses); // ""
        string tmp = sb.str ().AsUTF8 ();
        return Cryptography::Format<GUID> (DIGESTER_::ComputeDigest ((const std::byte*)tmp.c_str (), (const std::byte*)tmp.c_str () + tmp.length ()));
    }
}

namespace {
    optional<InternetAddress> LookupExternalInternetAddress_ (optional<Time::DurationSecondsType> allowedStaleness = {})
    {
        using Cache::SynchronizedCallerStalenessCache;
        static SynchronizedCallerStalenessCache<void, optional<InternetAddress>> sCache_;
        return sCache_.LookupValue (sCache_.Ago (allowedStaleness.value_or (30)), []() -> optional<InternetAddress> {
            /*
             * Alternative sources for this information:
             *
             *  o   http://api.ipify.org/
             *  o   http://myexternalip.com/raw
             */
            static const URL kSources_[]{
                URL{L"http://api.ipify.org/"_k, URL::ParseOptions::eAsFullURL},
                URL{L"http://myexternalip.com/raw"_k, URL::ParseOptions::eAsFullURL},
            };
            // @todo - when one fails, we should try the other first next time
            for (auto&& url : kSources_) {
                try {
                    // this goes throug the gateway, not necesarily this network, if we had multiple networks with gateways!
                    auto&& connection = IO::Network::Transfer::CreateConnection ();
                    connection.SetURL (url);
                    return IO::Network::InternetAddress{connection.GET ().GetDataTextInputStream ().ReadAll ().Trim ()};
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
    sb += L"}";
    return sb.str ();
}

/*
 ********************************************************************************
 ******************** Discovery::CollectActiveNetworks **************************
 ********************************************************************************
 */
Sequence<Network> Discovery::CollectActiveNetworks ()
{
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    Debug::TraceContextBumper ctx{L"Discovery::CollectActiveNetworks"};
#endif
    Mapping<CIDR, Network> accumResults;
    MultiSet<CIDR>         cidrScores; // primitive attempt to find best interface to display
    for (NetworkInterface i : CollectActiveNetworkInterfaces ()) {
        if (not i.fBindings.empty ()) {
            Interface::Binding useBinding = i.fBindings.Nth (0);
            i.fBindings.Apply ([&](Interface::Binding i) {
                if (useBinding.fInternetAddress.GetAddressFamily () != InternetAddress::AddressFamily::V4 or useBinding.fInternetAddress.IsMulticastAddress ()) {
                    if (i.fInternetAddress.GetAddressFamily () == InternetAddress::AddressFamily::V4 and not i.fInternetAddress.IsMulticastAddress ()) {
                        useBinding = i;
                    }
                }
            });
            if (useBinding.fInternetAddress.GetAddressSize ()) {
                // Guess CIDR prefix is max (so one address - bad guess) - if we cannot read from adapter
                CIDR    cidr{useBinding.fInternetAddress, useBinding.fOnLinkPrefixLength.value_or (*useBinding.fInternetAddress.GetAddressSize () * 8)};
                Network nw = accumResults.LookupValue (cidr, Network{{cidr}});

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
                    nw.fGEOLocInfo = LookupGEOLocation ((*nw.fExternalAddresses)[0]);
                    nw.fISP        = LookupInternetServiceProvider ((*nw.fExternalAddresses)[0]);
                }

                // Stroika IO::Network::Interface::fNetworkGUID field appears useless - since only defined on windows and not really documented what it means
                // and doesn't appear to vary interestingly (maybe didnt test enough) - like my virtual adapters and localhost adapter alll have same network as
                // the real ethernet adapter).
                // -- LGP 2018-12-16
                nw.fGUID = ComputeGUIDForNetwork_ (nw);

                accumResults.Add (cidr, nw);

                // put most important interfaces at top of list
                if ((i.fType == Interface::Type::eWiredEthernet or i.fType == Interface::Type::eWIFI) and i.fDescription and not i.fDescription->Contains (L"virtual", CompareOptions::eCaseInsensitive)) {
                    score += 1;
                }
                cidrScores.Add (cidr, score);
            }
            else {
                DbgTrace (L"Skipping interface %s - cuz bindings bad address", Characters::ToString (i).c_str ());
            }
        }
    }

    // now patch missing GUID if not already known
    for (auto i = accumResults.begin (); i != accumResults.end (); ++i) {
        if (i->fValue.fGUID == GUID::Zero ()) {
            StringBuilder sb;
            using namespace Stroika::Foundation::Cryptography;
            sb += Characters::ToString (i->fValue.fGateways);         // NO WHERE NEAR GOOD ENUF - take into account public IP ADDR and hardware address of router - but still ALLOW for any of these to float
            sb += Characters::ToString (i->fValue.fNetworkAddresses); // ""
            using DIGESTER_ = Digest::Digester<Digest::Algorithm::MD5>;
            string tmp      = sb.str ().AsUTF8 ();
            auto   v        = i->fValue;
            v.fGUID         = Cryptography::Format<GUID> (DIGESTER_::ComputeDigest ((const std::byte*)tmp.c_str (), (const std::byte*)tmp.c_str () + tmp.length ()));
            accumResults.Add (i->fKey, v);
        }
    }

    Sequence<Network> results;
    for (auto i : cidrScores.OrderBy ([](const CountedValue<CIDR>&lhs, const CountedValue<CIDR>&rhs) -> bool { return lhs.fCount > rhs.fCount; })) {
        results += *accumResults.Lookup (i.fValue);
    }
    Assert (results.size () == cidrScores.size ());
    Assert (results.size () == accumResults.size ());
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace (L"returns: %s", Characters::ToString (results).c_str ());
#endif
    return results;
}
