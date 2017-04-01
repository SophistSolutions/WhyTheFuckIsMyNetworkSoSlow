/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Cryptography/Digest/Algorithm/MD5.h"
#include "Stroika/Foundation/Cryptography/Format.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Writer.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/HTTP/Exception.h"
#include "Stroika/Foundation/IO/Network/HTTP/Headers.h"
#include "Stroika/Foundation/Streams/TextReader.h"

#include "Stroika/Frameworks/WebServer/ConnectionManager.h"
#include "Stroika/Frameworks/WebServer/Router.h"

#include "WebServer.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Execution;
using namespace Stroika::Foundation::IO::Network;

using namespace Stroika::Frameworks::WebServer;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;

/*
 ********************************************************************************
 ********************************** WebServer ***********************************
 ********************************************************************************
 */
class WebServer::Rep_ {
private:
    shared_ptr<IWSAPI> fWSAPI;
    const Router       kRouter_;
    ConnectionManager  fConnectionMgr_;

public:
    Rep_ (const shared_ptr<IWSAPI>& wsImpl)
        : fWSAPI (wsImpl)
        , kRouter_{Sequence<Route>{
              /*
             *  To test this example:
             *      o   Run the service (under the debugger if you wish)
             *      o   curl  http://localhost:8080/ OR
             *      o   curl  http://localhost:8080/Devices
             *      o   curl  http://localhost:8080/FRED OR      (to see error handling)
             *      o   curl -H "Content-Type: application/json" -X POST -d
             * '{"AppState":"Start"}' http://localhost:8080/SetAppState
             */
              Route{RegularExpression (L"", RegularExpression::SyntaxType::eECMAScript), DefaultPage_},
              Route{RegularExpression (L"Devices", RegularExpression::SyntaxType::eECMAScript), [=](Request* request, Response * response) { GetDevices_ (request, response); }},
              Route{RegularExpression (L"POST", RegularExpression::SyntaxType::eECMAScript), RegularExpression (L"SetAppState", RegularExpression::SyntaxType::eECMAScript), SetAppState_},
          }}
        , fConnectionMgr_{SocketAddress (Network::V4::kAddrAny, 8080), kRouter_} // listen and dispatch while this object exists
    {
        fConnectionMgr_.SetServerHeader (String{L"Why-The-Fuck-Is-My-Network-So-Slow/1.0"});
    }
    static void DefaultPage_ (Request* request, Response* response)
    {
        response->writeln (L"<html><body><p>Hi Mom</p></body></html>");
        response->SetContentType (DataExchange::PredefinedInternetMediaType::Text_HTML_CT ());
    }
    void GetDevices_ (Request* request, Response* response)
    {
        response->write (DataExchange::Variant::JSON::Writer ().WriteAsBLOB (Device::kMapper.FromObject (fWSAPI->GetDevices ())));
        response->SetContentType (DataExchange::PredefinedInternetMediaType::JSON_CT ());
    }
    static void SetAppState_ (Message* message)
    {
        String argsAsString = Streams::TextReader (message->PeekRequest ()->GetBody ()).ReadAll ();
        message->PeekResponse ()->writeln (L"<html><body><p>Hi SetAppState (" +
                                           argsAsString.As<wstring> () +
                                           L")</p></body></html>");
        message->PeekResponse ()->SetContentType (DataExchange::PredefinedInternetMediaType::Text_HTML_CT ());
    }
};

WebServer::WebServer (const shared_ptr<IWSAPI>& wsImpl)
    : fRep_ (make_shared<Rep_> (wsImpl))
{
}