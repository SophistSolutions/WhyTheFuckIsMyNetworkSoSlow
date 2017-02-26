/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include <iostream>

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Writer.h"
#include "Stroika/Foundation/Execution/SignalHandlers.h"
#include "Stroika/Foundation/Execution/WaitableEvent.h"
#include "Stroika/Foundation/IO/Network/HTTP/Exception.h"
#include "Stroika/Foundation/IO/Network/HTTP/Headers.h"
#include "Stroika/Foundation/Streams/TextReader.h"

#include "Stroika/Frameworks/WebServer/ConnectionManager.h"
#include "Stroika/Frameworks/WebServer/Router.h"

#include "WebServices/WSImpl.h"
using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::IO::Network;
using namespace Stroika::Frameworks::WebServer;

using Characters::String;
using Memory::BLOB;

/*
 *  To test this example:
 *      o   Run the service (under the debugger if you wish)
 *      o   curl  http://localhost:8080/ OR
 *      o   curl  http://localhost:8080/Devices
 *      o   curl  http://localhost:8080/FRED OR      (to see error handling)
 *      o   curl -H "Content-Type: application/json" -X POST -d
 * '{"AppState":"Start"}' http://localhost:8080/SetAppState
 */

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model;

namespace {
    // Can declare arguments as Request*,Response*
    void DefaultPage_ (Request*, Response* response)
    {
        response->writeln (L"<html><body><p>Hi Mom</p></body></html>");
        response->SetContentType (
            DataExchange::PredefinedInternetMediaType::Text_HTML_CT ());
    }
    void GetDevices_ (Request*, Response* response)
    {
        WSImpl wsimpl;
        response->write (DataExchange::Variant::JSON::Writer ().WriteAsBLOB (Device::kMapper.FromObject (wsimpl.GetDevices ())));
        //      response->writeln (L"<html><body><p>getdevices</p></body></html>");
        response->SetContentType (DataExchange::PredefinedInternetMediaType::JSON_CT ());
    }
    // Can declare arguments as Message* message
    void SetAppState_ (Message* message)
    {
        String argsAsString =
            Streams::TextReader (message->PeekRequest ()->GetBody ()).ReadAll ();
        message->PeekResponse ()->writeln (L"<html><body><p>Hi SetAppState (" +
                                           argsAsString.As<wstring> () +
                                           L")</p></body></html>");
        message->PeekResponse ()->SetContentType (
            DataExchange::PredefinedInternetMediaType::Text_HTML_CT ());
    }
    const Router kRouter_{Sequence<Route>{
        Route{RegularExpression (L"", RegularExpression::SyntaxType::eECMAScript), DefaultPage_},
        Route{RegularExpression (L"Devices", RegularExpression::SyntaxType::eECMAScript), GetDevices_},
        Route{
            RegularExpression (L"POST", RegularExpression::SyntaxType::eECMAScript),
            RegularExpression (L"SetAppState",
                               RegularExpression::SyntaxType::eECMAScript),
            SetAppState_},
    }};
}

int main (int argc, const char* argv[])
{
    Execution::SignalHandlerRegistry::SafeSignalsManager safeSignals;
    try {
        ConnectionManager cm{
            SocketAddress (Network::V4::kAddrAny, 8080),
            kRouter_}; // listen and dispatch while this object exists
        cm.SetServerHeader (String{L"Why-The-Fuck-Is-My-Network-So-Slow/1.0"});
        Execution::WaitableEvent (Execution::WaitableEvent::eAutoReset)
            .Wait (); // wait forever - til user hits ctrl-c
    }
    catch (...) {
        cerr << "Error encountered: "
             << Characters::ToString (current_exception ()).AsNarrowSDKString ()
             << " - terminating..." << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
