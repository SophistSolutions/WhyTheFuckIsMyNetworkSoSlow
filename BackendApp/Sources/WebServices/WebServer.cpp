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
#include "Stroika/Foundation/Execution/Module.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/HTTP/Exception.h"
#include "Stroika/Foundation/IO/Network/HTTP/Headers.h"
#include "Stroika/Foundation/IO/Network/HTTP/Methods.h"
#include "Stroika/Foundation/Streams/TextReader.h"

#include "Stroika/Frameworks/WebServer/ConnectionManager.h"
#include "Stroika/Frameworks/WebServer/FileSystemRouter.h"
#include "Stroika/Frameworks/WebServer/Router.h"
#include "Stroika/Frameworks/WebService/Server/Basic.h"
#include "Stroika/Frameworks/WebService/Server/VariantValue.h"

#include "WebServer.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Execution;
using namespace Stroika::Foundation::IO::Network;

using namespace Stroika::Frameworks::WebServer;
using namespace Stroika::Frameworks::WebService;
using namespace Stroika::Frameworks::WebService::Server;
using namespace Stroika::Frameworks::WebService::Server::VariantValue;

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
    static const WebServiceMethodDescription kNetworks_;
    static const WebServiceMethodDescription kNetworkInterfaces_;

private:
    static constexpr unsigned int kMaxConcurrentConnections_{5};
    static constexpr unsigned int kMaxGUIWebServerConcurrentConnections_{5};
    static const String_Constant  kServerString_;

private:
    shared_ptr<IWSAPI> fWSAPI_;
    const Router       fWSRouter_;
    ConnectionManager  fWSConnectionMgr_;
    const Router       fGUIWebRouter_;
    ConnectionManager  fGUIWebConnectionMgr_;

public:
    Rep_ (const shared_ptr<IWSAPI>& wsImpl)
        : fWSAPI_ (wsImpl)
        , fWSRouter_{Sequence<Route>{
              /*
               *  To test this example:
               *      o   Run the service (under the debugger if you wish)
               *      o   curl  http://localhost/ -- to see web GUI
               *      o   curl  http://localhost:8080/ -- to see a list of available web-methods
               */
              Route{
                  RegularExpression (IO::Network::HTTP::Methods::kOptions, RegularExpression::eECMAScript),
                  RegularExpression::kAny,
                  [](Message* m) {}},
              Route{RegularExpression (L"", RegularExpression::eECMAScript), DefaultPage_},
              Route{RegularExpression (L"Devices", RegularExpression::eECMAScript), mkRequestHandler (kDevices_, Device::kMapper, function<Collection<BackendApp::WebServices::Device> (void)>{[=]() { return fWSAPI_->GetDevices (); }})},
              Route{RegularExpression (L"NetworkInterfaces", RegularExpression::eECMAScript), mkRequestHandler (kNetworkInterfaces_, Device::kMapper, function<Collection<BackendApp::WebServices::NetworkInterface> (void)>{[=]() { return fWSAPI_->GetNetworkInterfaces (); }})},
              Route{RegularExpression (L"Networks", RegularExpression::eECMAScript), mkRequestHandler (kNetworks_, Device::kMapper, function<Sequence<BackendApp::WebServices::Network> (void)>{[=]() { return fWSAPI_->GetNetworks (); }})}}}
        , fWSConnectionMgr_{SocketAddresses (InternetAddresses_Any (), 8080), fWSRouter_, ConnectionManager::Options{kMaxConcurrentConnections_, Socket::BindFlags{true}, kServerString_}} // listen and dispatch while this object exists
        , fGUIWebRouter_{Sequence<Route>{
              Route{RegularExpression::kAny, FileSystemRouter{Execution::GetEXEDir () + L"html", {}, Sequence<String>{L"index.html"}}},
          }}
        , fGUIWebConnectionMgr_{SocketAddresses (InternetAddresses_Any (), 80), fGUIWebRouter_, ConnectionManager::Options{kMaxGUIWebServerConcurrentConnections_, Socket::BindFlags{true}, kServerString_}}
    {
    }
    static void DefaultPage_ (Request* request, Response* response)
    {
        WriteDocsPage (
            response,
            Sequence<WebServiceMethodDescription>{
                kDevices_,
                kNetworkInterfaces_,
                kNetworks_,
            },
            DocsOptions{String_Constant{L"Web Methods"}});
    }
};
const WebServiceMethodDescription WebServer::Rep_::kDevices_{
    String_Constant{L"Devices"},
    Set<String>{String_Constant{IO::Network::HTTP::Methods::kGet}},
    DataExchange::PredefinedInternetMediaType::kJSON,
    {},
    Sequence<String>{L"curl http://localhost:8080/Devices"},
    Sequence<String>{L"Fetch the list of known devices for the currently connected network.",
                     L"@todo - in the future - add support for parameters to this fetch - which can be used to filter/subset etc"},
};
const WebServiceMethodDescription WebServer::Rep_::kNetworks_{
    String_Constant{L"Networks"},
    Set<String>{String_Constant{IO::Network::HTTP::Methods::kGet}},
    DataExchange::PredefinedInternetMediaType::kJSON,
    {},
    Sequence<String>{L"curl http://localhost:8080/Networks"},
    Sequence<String>{L"Fetch the list of known Networks.",
                     L"@todo - in the future - add support for parameters to this fetch - which can be used to filter/subset etc"},
};
const WebServiceMethodDescription WebServer::Rep_::kNetworkInterfaces_{
    String_Constant{L"NetworkInterfaces"},
    Set<String>{String_Constant{IO::Network::HTTP::Methods::kGet}},
    DataExchange::PredefinedInternetMediaType::kJSON,
    {},
    Sequence<String>{L"curl http://localhost:8080/NetworkInterfaces"},
    Sequence<String>{L"Fetch the list of known Network Interfaces.",
                     L"@todo - in the future - add support for parameters to this fetch - which can be used to filter/subset etc"},
};
const String_Constant WebServer::Rep_::kServerString_{L"Why-The-Fuck-Is-My-Network-So-Slow/1.0"};

WebServer::WebServer (const shared_ptr<IWSAPI>& wsImpl)
    : fRep_ (make_shared<Rep_> (wsImpl))
{
}