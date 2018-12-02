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
namespace {
    DISABLE_COMPILER_MSC_WARNING_START (4573);
    DISABLE_COMPILER_GCC_WARNING_START ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    const DataExchange::ObjectVariantMapper kBasicsMapper_ = []() {
        DataExchange::ObjectVariantMapper mapper;
        mapper.AddCommonType<Collection<String>> ();
        return mapper;
    }();
    DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    DISABLE_COMPILER_MSC_WARNING_END (4573);
}

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
              Route{
                  RegularExpression (L"", RegularExpression::eECMAScript),
                  DefaultPage_},

              Route{
                  RegularExpression (L"Devices", RegularExpression::eECMAScript),
                  mkRequestHandler (kDevices_, Device::kMapper, function<Collection<Device> (void)>{[=]() { return fWSAPI_->GetDevices (); }})},

              // This doesn't belong here - move to new WebService sample
              Route{
                  RegularExpression (L"plus", RegularExpression::eECMAScript),
                  mkRequestHandler (WebServiceMethodDescription{{}, {}, DataExchange::PredefinedInternetMediaType::JSON_CT ()}, Device::kMapper, Traversal::Iterable<String>{L"arg1", L"arg2"}, function<double(double, double)>{[=](double arg1, double arg2) { return arg1 + arg2; }})},
              Route{
                  RegularExpression (L"test-void-return", RegularExpression::eECMAScript),
                  mkRequestHandler (WebServiceMethodDescription{}, Device::kMapper, Traversal::Iterable<String>{L"err-if-more-than-10"}, function<void(double)>{[=](double check) { if (check > 10) {Execution::Throw (Execution::StringException (L"more than 10"));} }})},

              Route{
                  RegularExpression (L"NetworkInterfaces", RegularExpression::eECMAScript),
                  [=](Message* m) {
                      ExpectedMethod (m->GetRequestReference (), kNetworkInterfaces_);
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (m->PeekRequest ());
                      if (args.LookupValue (L"recurse", false).As<bool> ()) {
                          WriteResponse (m->PeekResponse (), kNetworkInterfaces_, NetworkInterface::kMapper.FromObject (fWSAPI_->GetNetworkInterfaces_Recurse ()));
                      }
                      else {
                          WriteResponse (m->PeekResponse (), kNetworkInterfaces_, kBasicsMapper_.FromObject (fWSAPI_->GetNetworkInterfaces ()));
                      }
                  }},
              Route{
                  RegularExpression (L"NetworkInterface/.*", RegularExpression::eECMAScript),
                  [=](Message* m) {
                      // @todo parse out of REGULAR EXPRESSION - id
                      //tmphack way to grab id arg (til stroika router supports this)
                      String tmp = m->PeekRequest ()->GetURL ().GetHostRelativePath ();
                      if (auto i = tmp.RFind ('/')) {
                          String id = L"1ac5b672-a496-45c4-86d8-1f4939f9a236";
                          id        = tmp.SubString (*i + 1);
                          ExpectedMethod (m->GetRequestReference (), kNetworkInterfaces_);
                          WriteResponse (m->PeekResponse (), kNetworkInterfaces_, NetworkInterface::kMapper.FromObject (fWSAPI_->GetNetworkInterface (id)));
                      }
                      else {
                          Execution::Throw (Execution::StringException (L"missing ID argument"));
                      }
                  }},

              Route{
                  RegularExpression (L"Networks", RegularExpression::eECMAScript),
                  mkRequestHandler (kNetworks_, Network::kMapper, function<Sequence<Network> (void)>{[=]() { return fWSAPI_->GetNetworks (); }})}}}
        , fWSConnectionMgr_{SocketAddresses (InternetAddresses_Any (), 8080), fWSRouter_, ConnectionManager::Options{kMaxConcurrentConnections_, Socket::BindFlags{true}, kServerString_}} // listen and dispatch while this object exists
        , fGUIWebRouter_{Sequence<Route>{
              Route{RegularExpression::kAny, FileSystemRouter{Execution::GetEXEDir () + L"html", {}, Sequence<String>{L"index.html"}}},
          }}
        , fGUIWebConnectionMgr_{SocketAddresses (InternetAddresses_Any (), 80), fGUIWebRouter_, ConnectionManager::Options{kMaxGUIWebServerConcurrentConnections_, Socket::BindFlags{true}, kServerString_}}
    {
        using VariantValue         = DataExchange::VariantValue;
        Sequence<VariantValue> tmp = OrderParamValues (Iterable<String>{L"page", L"xxx"}, PickoutParamValuesFromURL (URL (L"http://www.sophist.com?page=5", URL::eFlexiblyAsUI)));
        Assert (tmp.size () == 2);
        Assert (tmp[0] == 5);
        Assert (tmp[1] == nullptr);
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
    Sequence<String>{L"curl http://localhost:8080/NetworkInterfaces", L"curl http://localhost:8080/NetworkInterfaces?recurse=true"},
    Sequence<String>{L"Fetch the list of known Network Interfaces.",
                     L"@todo - in the future - add support for parameters to this fetch - which can be used to filter/subset etc"},
};
const String_Constant WebServer::Rep_::kServerString_{L"Why-The-Fuck-Is-My-Network-So-Slow/1.0"};

WebServer::WebServer (const shared_ptr<IWSAPI>& wsImpl)
    : fRep_ (make_shared<Rep_> (wsImpl))
{
}