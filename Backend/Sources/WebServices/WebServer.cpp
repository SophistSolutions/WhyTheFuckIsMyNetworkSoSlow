/*
* Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/String_Constant.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Common/EmptyObjectForConstructorSideEffect.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Cryptography/Digest/Algorithm/MD5.h"
#include "Stroika/Foundation/Cryptography/Format.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Writer.h"
#include "Stroika/Foundation/Execution/Activity.h"
#include "Stroika/Foundation/Execution/Module.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/HTTP/ClientErrorException.h"
#include "Stroika/Foundation/IO/Network/HTTP/Exception.h"
#include "Stroika/Foundation/IO/Network/HTTP/Headers.h"
#include "Stroika/Foundation/IO/Network/HTTP/Methods.h"
#include "Stroika/Foundation/Streams/TextReader.h"

#include "Stroika/Frameworks/WebServer/ConnectionManager.h"
#include "Stroika/Frameworks/WebServer/FileSystemRouter.h"
#include "Stroika/Frameworks/WebServer/Router.h"
#include "Stroika/Frameworks/WebService/Server/Basic.h"
#include "Stroika/Frameworks/WebService/Server/VariantValue.h"

#include "AppVersion.h"

#include "WebServer.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Execution;
using namespace Stroika::Foundation::IO::Network;

using Stroika::Foundation::Common::EmptyObjectForConstructorSideEffect;
using Stroika::Foundation::IO::Network::HTTP::ClientErrorException;

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
    const DataExchange::ObjectVariantMapper kBasicsMapper_ = [] () {
        DataExchange::ObjectVariantMapper mapper;
        mapper.AddCommonType<Collection<String>> ();
        mapper.AddCommonType<Sequence<String>> ();
        return mapper;
    }();
    DISABLE_COMPILER_GCC_WARNING_END ("GCC diagnostic ignored \"-Winvalid-offsetof\"");
    DISABLE_COMPILER_MSC_WARNING_END (4573);
}

namespace {
    constexpr Activity kContructing_GUI_WebServer_{L"constructing static content webserver"sv};
    constexpr Activity kContructing_WSAPI_WebServer_{L"constructing WSAPI webserver"sv};
}

class WebServer::Rep_ {
public:
    static const WebServiceMethodDescription kAbout_;
    static const WebServiceMethodDescription kDevices_;
    static const WebServiceMethodDescription kNetworks_;
    static const WebServiceMethodDescription kNetworkInterfaces_;
    static const WebServiceMethodDescription kOperations_;

private:
    static constexpr unsigned int kMaxConcurrentConnections_{5};
    static constexpr unsigned int kMaxGUIWebServerConcurrentConnections_{5};
    static const inline String    kServerString_ = L"Why-The-Fuck-Is-My-Network-So-Slow/"sv + AppVersion::kVersion.AsMajorMinorString ();

private:
    shared_ptr<IWSAPI>                                             fWSAPI_;
    const Router                                                   fWSRouter_;
    optional<DeclareActivity<Activity<wstring_view>>>              fEstablishActivity1_{&kContructing_WSAPI_WebServer_};
    ConnectionManager                                              fWSConnectionMgr_;
    [[NO_UNIQUE_ADDRESS_ATTR]] EmptyObjectForConstructorSideEffect fIgnore1_{[this] () { fEstablishActivity1_.reset (); }};
    const Router                                                   fGUIWebRouter_;
    optional<DeclareActivity<Activity<wstring_view>>>              fEstablishActivity2_{&kContructing_GUI_WebServer_};
    ConnectionManager                                              fGUIWebConnectionMgr_;
    [[NO_UNIQUE_ADDRESS_ATTR]] EmptyObjectForConstructorSideEffect fIgnore2_{[this] () { fEstablishActivity2_.reset (); }};

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
                  [] (Message* m) {}},
              Route{
                  L""_RegEx,
                  DefaultPage_},

#if 0
              Route{
                  RegularExpression (L"Devices", RegularExpression::eECMAScript),
                  mkRequestHandler (kDevices_, Device::kMapper, function<Collection<Device> (void)>{[=]() { return fWSAPI_->GetDevices_Recurse (); }})},
#endif

              Route{
                  L"devices(/?)"_RegEx,
                  [=] (Message* m) {
                      constexpr bool kDefault_FilterRunningOnly_{true};
                      ExpectedMethod (m->GetRequestReference (), kNetworkInterfaces_);
                      Mapping<String, DataExchange::VariantValue> args              = PickoutParamValues (m->PeekRequest ());
                      bool                                        filterRunningOnly = args.LookupValue (L"filter-only-running", DataExchange::VariantValue{kDefault_FilterRunningOnly_}).As<bool> ();
                      if (args.LookupValue (L"recurse", false).As<bool> ()) {
                          WriteResponse (m->PeekResponse (), kDevices_, Device::kMapper.FromObject (fWSAPI_->GetDevices_Recurse ()));
                      }
                      else {
                          WriteResponse (m->PeekResponse (), kDevices_, kBasicsMapper_.FromObject (fWSAPI_->GetDevices ()));
                      }
                  }},
              Route{
                  L"devices/(.+)"_RegEx,
                  [=] (Message* m, const Containers::Sequence<Characters::String>& matches) {
                      String id = matches[0];
                      ExpectedMethod (m->GetRequestReference (), kDevices_);
                      WriteResponse (m->PeekResponse (), kDevices_, Device::kMapper.FromObject (fWSAPI_->GetDevice (id)));
                  }},

              Route{
                  L"network-interfaces(/?)"_RegEx,
                  [=] (Message* m) {
                      constexpr bool kDefault_FilterRunningOnly_{true};
                      ExpectedMethod (m->GetRequestReference (), kNetworkInterfaces_);
                      Mapping<String, DataExchange::VariantValue> args              = PickoutParamValues (m->PeekRequest ());
                      bool                                        filterRunningOnly = args.LookupValue (L"filter-only-running"sv, DataExchange::VariantValue{kDefault_FilterRunningOnly_}).As<bool> ();
                      if (args.LookupValue (L"recurse", false).As<bool> ()) {
                          WriteResponse (m->PeekResponse (), kNetworkInterfaces_, NetworkInterface::kMapper.FromObject (fWSAPI_->GetNetworkInterfaces_Recurse (filterRunningOnly)));
                      }
                      else {
                          WriteResponse (m->PeekResponse (), kNetworkInterfaces_, kBasicsMapper_.FromObject (fWSAPI_->GetNetworkInterfaces (filterRunningOnly)));
                      }
                  }},
              Route{
                  L"network-interfaces/(.+)"_RegEx,
                  [=] (Message* m, const Containers::Sequence<Characters::String>& matches) {
                      String id = matches[0];
                      ExpectedMethod (m->GetRequestReference (), kNetworkInterfaces_);
                      WriteResponse (m->PeekResponse (), kNetworkInterfaces_, NetworkInterface::kMapper.FromObject (fWSAPI_->GetNetworkInterface (id)));
                  }},

              Route{
                  L"networks(/?)"_RegEx,
                  [=] (Message* m) {
                      ExpectedMethod (m->GetRequestReference (), kNetworks_);
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (m->PeekRequest ());
                      if (args.LookupValue (L"recurse"sv, false).As<bool> ()) {
                          WriteResponse (m->PeekResponse (), kNetworkInterfaces_, Network::kMapper.FromObject (fWSAPI_->GetNetworks_Recurse ()));
                      }
                      else {
                          WriteResponse (m->PeekResponse (), kNetworkInterfaces_, kBasicsMapper_.FromObject (fWSAPI_->GetNetworks ()));
                      }
                  }},
              Route{
                  L"networks/(.+)"_RegEx,
                  [=] (Message* m, const Containers::Sequence<Characters::String>& matches) {
                      String id = matches[0];
                      ExpectedMethod (m->GetRequestReference (), kNetworks_);
                      WriteResponse (m->PeekResponse (), kNetworks_, Network::kMapper.FromObject (fWSAPI_->GetNetwork (id)));
                  }},

              Route{
                  L"operations/ping"_RegEx,
                  [=] (Message* m) {
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (m->PeekRequest ());
                      if (auto address = args.Lookup (L"target"sv)) {
                          ExpectedMethod (m->GetRequestReference (), kOperations_);
                          WriteResponse (m->PeekResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_Ping (address->As<String> ())));
                      }
                      else {
                          Execution::Throw (ClientErrorException (L"missing target argument"sv));
                      }
                  }},
              Route{
                  L"operations/traceroute"_RegEx,
                  [=] (Message* m) {
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (m->PeekRequest ());
                      optional<bool>                              reverseDNSResult;
                      if (auto rdr = args.Lookup (L"reverse-dns-result"sv)) {
                          reverseDNSResult = rdr->As<bool> ();
                      }
                      if (auto address = args.Lookup (L"target"sv)) {
                          ExpectedMethod (m->GetRequestReference (), kOperations_);
                          WriteResponse (m->PeekResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_TraceRoute (address->As<String> (), reverseDNSResult)));
                      }
                      else {
                          Execution::Throw (ClientErrorException (L"missing target argument"sv));
                      }
                  }},
              Route{
                  L"operations/dns/calculate-negative-lookup-time"_RegEx,
                  [=] (Message* m) {
                      ExpectedMethod (m->GetRequestReference (), kOperations_);
                      optional<unsigned int>                      samples;
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (m->PeekRequest ());
                      if (auto rdr = args.Lookup (L"samples")) {
                          samples = rdr->As<unsigned int> ();
                      }
                      WriteResponse (m->PeekResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_DNS_CalculateNegativeLookupTime (samples)));
                  }},
              Route{
                  L"operations/dns/lookup"_RegEx,
                  [=] (Message* m) {
                      ExpectedMethod (m->GetRequestReference (), kOperations_);
                      String                                      name;
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (m->PeekRequest ());
                      if (auto rdr = args.Lookup (L"name")) {
                          name = rdr->As<String> ();
                      }
                      else {
                          Execution::Throw (ClientErrorException (L"missing name argument"sv));
                      }
                      WriteResponse (m->PeekResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_DNS_Lookup (name)));
                  }},
              Route{
                  L"operations/dns/calculate-score"_RegEx,
                  [=] (Message* m) {
                      ExpectedMethod (m->GetRequestReference (), kOperations_);
                      WriteResponse (m->PeekResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_DNS_CalculateScore ()));
                  }},

              Route{
                  L"about"_RegEx,
                  mkRequestHandler (kAbout_, About::kMapper, function<About (void)>{[=] () { return fWSAPI_->GetAbout (); }})},

          }}
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
                kAbout_,
                kDevices_,
                kNetworkInterfaces_,
                kNetworks_,
                kOperations_,
            },
            DocsOptions{L"Web Methods"sv});
    }
};
const WebServiceMethodDescription WebServer::Rep_::kDevices_{
    L"devices"sv,
    Set<String>{String_Constant{IO::Network::HTTP::Methods::kGet}},
    DataExchange::PredefinedInternetMediaType::kJSON,
    {},
    Sequence<String>{L"curl http://localhost:8080/devices"sv, L"curl http://localhost:8080/devices?recurse=true"sv, L"curl http://localhost:8080/devices/{ID}"sv},
    Sequence<String>{L"Fetch the list of known devices for the currently connected network. By default, this list is sorted so the most interesting devices come first (like this machine is first)"sv,
                     L"@todo - in the future - add support for parameters to this fetch - which can be used to filter/subset etc"sv},
};
const WebServiceMethodDescription WebServer::Rep_::kNetworks_{
    L"networks"sv,
    Set<String>{String_Constant{IO::Network::HTTP::Methods::kGet}},
    DataExchange::PredefinedInternetMediaType::kJSON,
    {},
    Sequence<String>{L"curl http://localhost:8080/networks"sv, L"curl http://localhost:8080/networks?recurse=true"sv, L"curl http://localhost:8080/networks/{ID}"sv},
    Sequence<String>{L"Fetch the list of known Networks."sv,
                     L"@todo - in the future - add support for parameters to this fetch - which can be used to filter/subset etc"sv},
};
const WebServiceMethodDescription WebServer::Rep_::kNetworkInterfaces_{
    L"network-interfaces"sv,
    Set<String>{String_Constant{IO::Network::HTTP::Methods::kGet}},
    DataExchange::PredefinedInternetMediaType::kJSON,
    {},
    Sequence<String>{L"curl http://localhost:8080/network-interfaces", L"curl http://localhost:8080/network-interfaces?recurse=true"sv, L"curl http://localhost:8080/network-interfaces?filter-only-running=true"sv},
    Sequence<String>{L"Fetch the list of known Network Interfaces."sv,
                     L"[filter-only-running=true|false]?, recurse=true|false]?"sv},
};
const WebServiceMethodDescription WebServer::Rep_::kOperations_{
    L"operations"sv,
    Set<String>{String_Constant{IO::Network::HTTP::Methods::kGet}},
    DataExchange::PredefinedInternetMediaType::kJSON,
    {},
    Sequence<String>{
        L"curl http://localhost:8080/operations/ping?target=www.google.com"sv,
        L"curl http://localhost:8080/operations/traceroute?target=www.sophists.com"sv,
        L"curl http://localhost:8080/operations/dns/calculate-negative-lookup-time"sv,
        L"curl http://localhost:8080/operations/dns/lookup?name=www.youtube.com"sv,
        L"curl http://localhost:8080/operations/dns/calculate-score"sv,
    },
    Sequence<String>{
        L"perform a wide variety of operations - mostly for debugging for now but may stay around."sv,
        L"/operations/ping?target=address; (address can be ipv4, ipv6 address, or dnsname)"sv,
        L"/operations/traceroute?target=address[&reverse-dns-result=bool]?; (address can be ipv4, ipv6 address, or dnsname)"sv,
        L"/operations/dns/calculate-negative-lookup-time[&samples=uint]?"sv,
        L"/operations/dns/lookup[&name=string]"sv,
        L"/operations/dns/calculate-score; returns number 0 (worst) to 1.0 (best)"sv,
    },
};
const WebServiceMethodDescription WebServer::Rep_::kAbout_{
    L"about"sv,
    Set<String>{String_Constant{IO::Network::HTTP::Methods::kGet}},
    DataExchange::PredefinedInternetMediaType::kJSON,
    {},
    Sequence<String>{
        L"curl http://localhost:8080/about"sv,
    },
    Sequence<String>{L"Fetch the component versions, etc."sv},
};

WebServer::WebServer (const shared_ptr<IWSAPI>& wsImpl)
    : fRep_ (make_shared<Rep_> (wsImpl))
{
}
