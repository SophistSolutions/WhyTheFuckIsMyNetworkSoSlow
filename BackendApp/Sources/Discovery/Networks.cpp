/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include <algorithm>
#include <vector>

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/LinkMonitor.h"

#include "Networks.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::IO::Network;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Discovery;

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

/*
 ********************************************************************************
 ***************************** Discovery::Network *******************************
 ********************************************************************************
 */
String Discovery::Network::ToString () const
{
    StringBuilder sb;
    sb += L"{";
    sb += L"IP-Address: " + Characters::ToString (fIPAddress) + L", ";
    sb += L"Friendly-Name: " + Characters::ToString (fFriendlyName) + L", ";
    sb += L"SSIDs: " + Characters::ToString (fSSIDs) + L", ";
    sb += L"}";
    return sb.str ();
}

/*
 ********************************************************************************
 ******************** Discovery::CollectActiveNetworks **************************
 ********************************************************************************
 */
Collection<Network> Discovery::CollectActiveNetworks ()
{
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    Debug::TraceContextBumper ctx{L"Discovery::CollectActiveNetworks"};
#endif
    vector<Network> results;
    for (Interface i : IO::Network::GetInterfaces ()) {
        if (i.fType != Interface::Type::eLoopback and i.fStatus and i.fStatus->Contains (Interface::Status::eRunning)) {
            // prefer the v4 IP addr if any, and otherwise show v6
            if (not i.fBindings.empty ()) {
                // @todo REWRITE to use 'scoring' to pick BEST address not just v4/non-multicast
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
                    CIDR cidr{ useBinding.fInternetAddress, useBinding.fOnLinkPrefixLength.value_or (*useBinding.fInternetAddress.GetAddressSize () * 8) };
                    if (i.fType == Interface::Type::eWiredEthernet or i.fType == Interface::Type::eWIFI) {
                        results.insert (results.begin (), Network{cidr, {i.fFriendlyName}});
                    }
                    else {
                        results.push_back (Network{cidr, {i.fFriendlyName}});
                    }
                }
                else {
                    DbgTrace (L"Skipping interface %s - cuz bindings bad address", Characters::ToString (i).c_str ());
                }
            }
        }
    }
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    DbgTrace (L"returns: %s", Characters::ToString (results).c_str ());
#endif
    return results;
}
