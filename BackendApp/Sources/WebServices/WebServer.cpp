/*
* Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/String_Constant.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Cryptography/Digest/Algorithm/MD5.h"
#include "Stroika/Foundation/Cryptography/Format.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Writer.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/HTTP/Exception.h"
#include "Stroika/Foundation/IO/Network/HTTP/Headers.h"
#include "Stroika/Foundation/IO/Network/HTTP/Methods.h"
#include "Stroika/Foundation/Streams/TextReader.h"

#include "Stroika/Frameworks/WebServer/ConnectionManager.h"
#include "Stroika/Frameworks/WebServer/Router.h"
#include "Stroika/Frameworks/WebService/Basic.h"

#include "WebServer.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Execution;
using namespace Stroika::Foundation::IO::Network;

using namespace Stroika::Frameworks::WebServer;
using namespace Stroika::Frameworks::WebService;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;

/*
 ********************************************************************************
 ********************************** WebServer ***********************************
 ********************************************************************************
 */
class WebServer::Rep_ {
public:
    static const WebServiceMethodDescription kDevices_;

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
               *      o   curl  http://localhost:8080/ -- to see a list of available web-methods
               */
              Route{
                  RegularExpression (IO::Network::HTTP::Methods::kOptions, RegularExpression::eECMAScript),
                  RegularExpression::kAny,
                  [](Message* m) {}},
              Route{RegularExpression (L"", RegularExpression::eECMAScript), DefaultPage_},
              Route{RegularExpression (L"Devices", RegularExpression::eECMAScript), [=](Message* m) { GetDevices_ (m); }},
          }}
        , fConnectionMgr_{SocketAddress (Network::V4::kAddrAny, 8080), kRouter_} // listen and dispatch while this object exists
    {
        fConnectionMgr_.SetServerHeader (String_Constant{L"Why-The-Fuck-Is-My-Network-So-Slow/1.0"});
    }
    static void DefaultPage_ (Request* request, Response* response)
    {
        WriteDocsPage (
            response,
            Sequence<WebServiceMethodDescription>{
                kDevices_,
            },
            String_Constant{L"Web Methods"});
    }
    void GetDevices_ (Message* m)
    {
        WriteResponse (m->PeekResponse (), kDevices_, Device::kMapper.FromObject (fWSAPI->GetDevices ()));
    }
};
const WebServiceMethodDescription WebServer::Rep_::kDevices_{
    String_Constant{L"Devices"},
    Set<String>{String_Constant{IO::Network::HTTP::Methods::kGet}},
    DataExchange::PredefinedInternetMediaType::JSON_CT (),
    {},
    Sequence<String>{L"curl http://localhost:8080/Devices"},
    Sequence<String>{L"Fetch the list of known devices for the currently connected network.",
                     L"@todo - in the future - add support for parameters to this fetch - which can be used to filter/subset etc"},
};

WebServer::WebServer (const shared_ptr<IWSAPI>& wsImpl)
    : fRep_ (make_shared<Rep_> (wsImpl))
{
}