/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2020.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/String_Constant.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Common/EmptyObjectForConstructorSideEffect.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Cryptography/Digest/Algorithm/MD5.h"
#include "Stroika/Foundation/Cryptography/Format.h"
#include "Stroika/Foundation/DataExchange/InternetMediaTypeRegistry.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Reader.h"
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
using namespace Stroika::Foundation::Memory;
using namespace Stroika::Foundation::IO::Network;
using namespace Stroika::Foundation::IO::Network::HTTP;

using Stroika::Foundation::Common::EmptyObjectForConstructorSideEffect;

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
    const DataExchange::ObjectVariantMapper kBasicsMapper_ = [] () {
        DataExchange::ObjectVariantMapper mapper;
        mapper.AddCommonType<Collection<String>> ();
        mapper.AddCommonType<Sequence<String>> ();
        return mapper;
    }();
}

namespace {
    constexpr Activity kContructing_GUI_WebServer_{L"constructing static content webserver"sv};
    constexpr Activity kContructing_WSAPI_WebServer_{L"constructing WSAPI webserver"sv};
}

class WebServer::Rep_ {
public:
    static const WebServiceMethodDescription kAbout_;
    static const WebServiceMethodDescription kBlob_;
    static const WebServiceMethodDescription kDevices_;
    static const WebServiceMethodDescription kNetworks_;
    static const WebServiceMethodDescription kNetworkInterfaces_;
    static const WebServiceMethodDescription kOperations_;

private:
    static constexpr unsigned int kMaxWSConcurrentConnections_{25};
    static constexpr unsigned int kMaxWSThreads_{3};
    static constexpr unsigned int kMaxGUIWebServerConcurrentConnections_{25};
    static constexpr unsigned int kMaxGUIThreads_{1};
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
        : fWSAPI_{wsImpl}
        , fWSRouter_
    {
        Sequence<Route>
        {
            /*
               *  To test this example:
               *      o   Run the service (under the debugger if you wish)
               *      o   curl  http://localhost/ -- to see web GUI
               *      o   curl  http://localhost:8080/ -- to see a list of available web-methods
               */
            Route{
                MethodsRegularExpressions::kOptions,
                RegularExpression::kAny,
                [] ([[maybe_unused]] Message* m) {}},

                Route{
                    L""_RegEx,
                    DefaultPage_},

                Route{
                    L"about"_RegEx,
                    mkRequestHandler (kAbout_, About::kMapper, function<About (void)>{[=] () { return fWSAPI_->GetAbout (); }})},

                Route{
                    L"blob/(.+)"_RegEx,
                    [=] (Message* m, const String& id) {
                        tuple<Memory::BLOB, DataExchange::InternetMediaType> b = fWSAPI_->GetBLOB (id);
                        m->PeekResponse ()->SetContentType (get<1> (b));
                        m->PeekResponse ()->write (get<0> (b));
                    }},

                Route{
                    MethodsRegularExpressions::kGet,
                    L"devices(/?)"_RegEx,
                    [=] (Message* m) {
                        constexpr bool                              kDefault_FilterRunningOnly_{true};
                        Mapping<String, DataExchange::VariantValue> args              = PickoutParamValues (m->PeekRequest ());
                        bool                                        filterRunningOnly = args.LookupValue (L"filter-only-running"sv, DataExchange::VariantValue{kDefault_FilterRunningOnly_}).As<bool> ();
                        optional<DeviceSortParamters>               sort;
                        if (auto o = args.Lookup (L"sort"sv)) {
                            ClientErrorException::TreatExceptionsAsClientError ([&] () {
                                sort = DeviceSortParamters::kMapper.ToObject<DeviceSortParamters> (DataExchange::Variant::JSON::Reader{}.Read (o->As<String> ()));
                            });
                        }
                        if (auto o = args.Lookup (L"sortBy"sv)) {
                            ClientErrorException::TreatExceptionsAsClientError ([&] () {
                                sort = sort.value_or (DeviceSortParamters{});
                                sort->fSearchTerms += DeviceSortParamters::SearchTerm{
                                    Configuration::DefaultNames<DeviceSortParamters::SearchTerm::By>{}.GetValue (o->As<String> ().c_str (), ClientErrorException{
                                                                                                                                                L"Invalid argument to query string sortBy"sv})};
                            });
                        }
                        if (args.LookupValue (L"recurse"sv, false).As<bool> ()) {
                            WriteResponse (m->PeekResponse (), kDevices_, Device::kMapper.FromObject (fWSAPI_->GetDevices_Recurse (sort)));
                        }
                        else {
                            WriteResponse (m->PeekResponse (), kDevices_, kBasicsMapper_.FromObject (fWSAPI_->GetDevices (sort)));
                        }
                    }},
                Route{
                    MethodsRegularExpressions::kGet,
                    L"devices/(.+)"_RegEx,
                    [=] (Message* m, const String& id) {
                        WriteResponse (m->PeekResponse (), kDevices_, Device::kMapper.FromObject (fWSAPI_->GetDevice (id)));
                    }},

                Route{
                    MethodsRegularExpressions::kGet,
                    L"network-interfaces(/?)"_RegEx,
                    [=] (Message* m) {
                        constexpr bool                              kDefault_FilterRunningOnly_{true};
                        Mapping<String, DataExchange::VariantValue> args              = PickoutParamValues (m->PeekRequest ());
                        bool                                        filterRunningOnly = args.LookupValue (L"filter-only-running"sv, DataExchange::VariantValue{kDefault_FilterRunningOnly_}).As<bool> ();
                        if (args.LookupValue (L"recurse"sv, false).As<bool> ()) {
                            WriteResponse (m->PeekResponse (), kNetworkInterfaces_, NetworkInterface::kMapper.FromObject (fWSAPI_->GetNetworkInterfaces_Recurse (filterRunningOnly)));
                        }
                        else {
                            WriteResponse (m->PeekResponse (), kNetworkInterfaces_, kBasicsMapper_.FromObject (fWSAPI_->GetNetworkInterfaces (filterRunningOnly)));
                        }
                    }},
                Route{
                    MethodsRegularExpressions::kGet,
                    L"network-interfaces/(.+)"_RegEx,
                    [=] (Message* m, const String& id) {
                        WriteResponse (m->PeekResponse (), kNetworkInterfaces_, NetworkInterface::kMapper.FromObject (fWSAPI_->GetNetworkInterface (id)));
                    }},

                Route{
                    MethodsRegularExpressions::kGet,
                    L"networks(/?)"_RegEx,
                    [=] (Message* m) {
                        Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (m->PeekRequest ());
                        if (args.LookupValue (L"recurse"sv, false).As<bool> ()) {
                            WriteResponse (m->PeekResponse (), kNetworkInterfaces_, Network::kMapper.FromObject (fWSAPI_->GetNetworks_Recurse ()));
                        }
                        else {
                            WriteResponse (m->PeekResponse (), kNetworkInterfaces_, kBasicsMapper_.FromObject (fWSAPI_->GetNetworks ()));
                        }
                    }},
                Route{
                    MethodsRegularExpressions::kGet,
                    L"networks/(.+)"_RegEx,
                    [=] (Message* m, const String& id) {
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
                            Execution::Throw (ClientErrorException{L"missing target argument"sv});
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
                            Execution::Throw (ClientErrorException{L"missing target argument"sv});
                        }
                    }},
                Route{
                    L"operations/dns/calculate-negative-lookup-time"_RegEx,
                    [=] (Message* m) {
                        ExpectedMethod (m->GetRequestReference (), kOperations_);
                        optional<unsigned int>                      samples;
                        Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (m->PeekRequest ());
                        if (auto rdr = args.Lookup (L"samples"sv)) {
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
                        if (auto rdr = args.Lookup (L"name"sv)) {
                            name = rdr->As<String> ();
                        }
                        else {
                            Execution::Throw (ClientErrorException{L"missing name argument"sv});
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
                    L"operations/scan/FullRescan"_RegEx,
                    [=] (Message* m) {
                        Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (m->PeekRequest ());
                        ExpectedMethod (m->GetRequestReference (), kOperations_);
                        if (auto rdr = args.Lookup (L"deviceID"sv)) {
                            WriteResponse (m->PeekResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_Scan_FullRescan (rdr->As<String> ())));
                        }
                        else {
                            Execution::Throw (ClientErrorException{L"missing deviceID argument"sv});
                        }
                    }},
                Route{
                    L"operations/scan/Scan"_RegEx,
                    [=] (Message* m) {
                        Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (m->PeekRequest ());
                        ExpectedMethod (m->GetRequestReference (), kOperations_);
                        if (auto rdr = args.Lookup (L"addr"sv)) {
                            WriteResponse (m->PeekResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_Scan_Scan (rdr->As<String> ())));
                        }
                        else {
                            Execution::Throw (ClientErrorException{L"missing deviceID argument"sv});
                        }
                    }},
        }
    }
#if __cpp_designated_initializers
    , fWSConnectionMgr_
    {
        SocketAddresses (InternetAddresses_Any (), 8080),
            fWSRouter_,
            ConnectionManager::Options { .fMaxConnections = kMaxWSConcurrentConnections_, .fMaxConcurrentlyHandledConnections = kMaxWSThreads_, .fBindFlags = Socket::BindFlags{true}, .fServerHeader = kServerString_ }
    } // listen and dispatch while this object exists
#else
    , fWSConnectionMgr_
    {
        SocketAddresses (InternetAddresses_Any (), 8080), fWSRouter_, ConnectionManager::Options { kMaxWSConcurrentConnections_, kMaxWSThreads_, Socket::BindFlags{true}, kServerString_ }
    } // listen and dispatch while this object exists
#endif
    , fGUIWebRouter_
    {
        Sequence<Route>
        {
            Route{RegularExpression::kAny, FileSystemRouter{Execution::GetEXEDir () / "html", {}, Sequence<String>{L"index.html"sv}}},
        }
    }
#if __cpp_designated_initializers
    , fGUIWebConnectionMgr_
    {
        SocketAddresses (InternetAddresses_Any (), 80),
            fGUIWebRouter_,
            ConnectionManager::Options { .fMaxConnections = kMaxGUIWebServerConcurrentConnections_, .fMaxConcurrentlyHandledConnections = kMaxGUIThreads_, .fBindFlags = Socket::BindFlags{true}, .fServerHeader = kServerString_ }
    }
#else
    , fGUIWebConnectionMgr_
    {
        SocketAddresses (InternetAddresses_Any (), 80),
            fGUIWebRouter_,
            ConnectionManager::Options { kMaxGUIWebServerConcurrentConnections_, kMaxGUIThreads_, Socket::BindFlags{true}, kServerString_ }
    }
#endif
    {
    }
    static void DefaultPage_ ([[maybe_unused]] Request* request, Response* response)
    {
        WriteDocsPage (
            response,
            Sequence<WebServiceMethodDescription>{
                kAbout_,
                kBlob_,
                kDevices_,
                kNetworkInterfaces_,
                kNetworks_,
                kOperations_,
            },
            DocsOptions{L"Web Methods"sv});
    }
};
const WebServiceMethodDescription WebServer::Rep_::kAbout_{
    L"about"sv,
    Set<String>{String_Constant{IO::Network::HTTP::Methods::kGet}},
    DataExchange::InternetMediaTypes::kJSON,
    L"Data about the WTF application, version etc"sv,
    Sequence<String>{
        L"curl http://localhost:8080/about"sv,
    },
    Sequence<String>{L"Fetch the component versions, etc."sv},
};
const WebServiceMethodDescription WebServer::Rep_::kBlob_{
    L"blob"sv,
    Set<String>{String_Constant{IO::Network::HTTP::Methods::kGet}},
    nullopt,
    L"BLOBs (and their associated media type) generally sourced from other computers, but cached here so they will be available when those other computers are not (like icons from SSDP)"sv,
    Sequence<String>{
        L"curl http://localhost:8080/blob/{ID}"sv,
    },
    Sequence<String>{L"Fetch the blob by value (generally these links appear in GET /devices/{x} etc output)."sv},
};
const WebServiceMethodDescription WebServer::Rep_::kDevices_{
    L"devices"sv,
    Set<String>{String_Constant{IO::Network::HTTP::Methods::kGet}},
    DataExchange::InternetMediaTypes::kJSON,
    {},
    Sequence<String>{
        L"curl http://localhost:8080/devices"sv,
        L"curl http://localhost:8080/devices?recurse=true"sv,
        L"curl 'http://localhost:8080/devices?recurse=true&sort=%7b\"searchTerms\":[%7b\"by\":\"Address\"%7d],\"compareNetwork\":\"192.168.244.0/24\"%7d'"sv,
        L"curl 'http://localhost:8080/devices?recurse=true&sort={\"searchTerms\":[{\"by\":\"Address\"},{\"by\":\"Priority\"}],\"compareNetwork\":\"192.168.244.0/24\"}'"sv,
        L"curl http://localhost:8080/devices?recurse=true&sortBy=Address&sortCompareNetwork=192.168.244.0/24"sv,
        L"curl http://localhost:8080/devices/60c59f9c-9a69-c89e-9d99-99c7976869c5"sv},
    Sequence<String>{
        L"Fetch the list of known devices for the currently connected network. By default, this list is sorted so the most interesting devices come first (like this machine is first)"sv,
        L"query-string: sort={[by: Address|Priority|Name|Type, ascending: true|false]+, compareNetwork?: CIDR|network-id}; sort=ARG is JSON encoded SearchTerm={by: string, ascending?: bool}, {searchTerms: SearchTerm[], compareNetwork: string}"sv,
        L"query-string: sortBy=Address|Priority|Name|Type sortAscending=true|false (requires sortBy); both are aliases for sort=...)"sv,
        L"Note: sorts are stable, so they can be combined one after the other. To get a GroupBy, just do the grouping as the final 'sort'."sv,
    },
};
const WebServiceMethodDescription WebServer::Rep_::kNetworks_{
    L"networks"sv,
    Set<String>{String_Constant{IO::Network::HTTP::Methods::kGet}},
    DataExchange::InternetMediaTypes::kJSON,
    {},
    Sequence<String>{L"curl http://localhost:8080/networks"sv, L"curl http://localhost:8080/networks?recurse=true"sv, L"curl http://localhost:8080/networks/{ID}"sv},
    Sequence<String>{L"Fetch the list of known Networks."sv,
                     L"@todo - in the future - add support for parameters to this fetch - which can be used to filter/subset etc"sv},
};
const WebServiceMethodDescription WebServer::Rep_::kNetworkInterfaces_{
    L"network-interfaces"sv,
    Set<String>{String_Constant{IO::Network::HTTP::Methods::kGet}},
    DataExchange::InternetMediaTypes::kJSON,
    {},
    Sequence<String>{L"curl http://localhost:8080/network-interfaces", L"curl http://localhost:8080/network-interfaces?recurse=true"sv, L"curl http://localhost:8080/network-interfaces?filter-only-running=true"sv},
    Sequence<String>{L"Fetch the list of known Network Interfaces."sv,
                     L"[filter-only-running=true|false]?, recurse=true|false]?"sv},
};
const WebServiceMethodDescription WebServer::Rep_::kOperations_{
    L"operations"sv,
    Set<String>{String_Constant{IO::Network::HTTP::Methods::kGet}},
    DataExchange::InternetMediaTypes::kJSON,
    {},
    Sequence<String>{
        L"curl http://localhost:8080/operations/ping?target=www.google.com"sv,
        L"curl http://localhost:8080/operations/traceroute?target=www.sophists.com"sv,
        L"curl http://localhost:8080/operations/dns/calculate-negative-lookup-time"sv,
        L"curl http://localhost:8080/operations/dns/lookup?name=www.youtube.com"sv,
        L"curl http://localhost:8080/operations/dns/calculate-score"sv,
        L"curl http://localhost:8080/operations/scan/FullRescan?device=ID"sv,
        L"curl http://localhost:8080/operations/scan/Scan?addr=hostOrIPAddr"sv,
    },
    Sequence<String>{
        L"perform a wide variety of operations - mostly for debugging for now but may stay around."sv,
        L"/operations/ping?target=address; (address can be ipv4, ipv6 address, or dnsname)"sv,
        L"/operations/traceroute?target=address[&reverse-dns-result=bool]?; (address can be ipv4, ipv6 address, or dnsname)"sv,
        L"/operations/dns/calculate-negative-lookup-time[&samples=uint]?"sv,
        L"/operations/dns/lookup[&name=string]"sv,
        L"/operations/dns/calculate-score; returns number 0 (worst) to 1.0 (best)"sv,
        L"/operations/scan/FullRescan?device=ID; clears found ports for deviceID, and immediately rescans before returning; returns summary"sv,
        L"/operations/scan/operations/scan/Scan?addr=hostOrIPAddr; doesnt affect internal strucutres, and just runs scan process on given IP and returns result"sv,
    },
};

WebServer::WebServer (const shared_ptr<IWSAPI>& wsImpl)
    : fRep_ (make_shared<Rep_> (wsImpl))
{
}
