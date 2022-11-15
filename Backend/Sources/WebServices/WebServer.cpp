/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Common/ObjectForSideEffects.h"
#include "Stroika/Foundation/Common/Property.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Cryptography/Digest/Algorithm/MD5.h"
#include "Stroika/Foundation/Cryptography/Format.h"
#include "Stroika/Foundation/DataExchange/InternetMediaTypeRegistry.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Reader.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Writer.h"
#include "Stroika/Foundation/Execution/Activity.h"
#include "Stroika/Foundation/Execution/IntervalTimer.h"
#include "Stroika/Foundation/Execution/Module.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/HTTP/ClientErrorException.h"
#include "Stroika/Foundation/IO/Network/HTTP/Exception.h"
#include "Stroika/Foundation/IO/Network/HTTP/Headers.h"
#include "Stroika/Foundation/IO/Network/HTTP/Methods.h"
#include "Stroika/Foundation/Streams/TextReader.h"
#include "Stroika/Foundation/Time/Duration.h"

#include "Stroika/Frameworks/WebServer/ConnectionManager.h"
#include "Stroika/Frameworks/WebServer/DefaultFaultInterceptor.h"
#include "Stroika/Frameworks/WebServer/FileSystemRequestHandler.h"
#include "Stroika/Frameworks/WebServer/Router.h"
#include "Stroika/Frameworks/WebService/Server/Basic.h"
#include "Stroika/Frameworks/WebService/Server/VariantValue.h"

#include "../Common/AppConfiguration.h"
#include "../Common/OperationalStatistics.h"

#include "AppVersion.h"

#include "WebServer.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Execution;
using namespace Stroika::Foundation::Memory;
using namespace Stroika::Foundation::IO::Network;

using DataExchange::ObjectVariantMapper;
using IO::Network::HTTP::ClientErrorException;
using Stroika::Foundation::Common::ConstantProperty;
using Stroika::Foundation::Common::EmptyObjectForSideEffects;
using Stroika::Foundation::Common::GUID;
using Stroika::Foundation::Time::Duration;

using namespace Stroika::Frameworks::WebServer;
using namespace Stroika::Frameworks::WebService;
using namespace Stroika::Frameworks::WebService::Server;
using namespace Stroika::Frameworks::WebService::Server::VariantValue;

using Stroika::Frameworks::WebServer::Request;
using Stroika::Frameworks::WebServer::Response;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;

using WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::AppConfigurationType;
using WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::gAppConfiguration;
using WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::OperationalStatisticsMgr;

// Configuration object passed to GUI as startup parameters/configuration
namespace {
    struct Config_ {
        optional<String>       API_ROOT;         // if specified takes precedence over DEFAULT_API_PORT
        optional<unsigned int> DEFAULT_API_PORT; // added to remote host used in web browser for accessing API

        static const ObjectVariantMapper kMapper;
    };
    const WebServiceMethodDescription kGUIConfig_{
        L"config"sv,
        Set<String>{IO::Network::HTTP::Methods::kGet},
        DataExchange::InternetMediaTypes::kJSON,
        L"GUI config"sv,
        Sequence<String>{},
        Sequence<String>{L"GUI config."sv},
    };
    const ObjectVariantMapper Config_::kMapper = [] () {
        ObjectVariantMapper mapper;
        mapper.AddCommonType<optional<String>> ();
        mapper.AddCommonType<optional<unsigned int>> ();
        mapper.AddClass<Config_> ({
            ObjectVariantMapper::StructFieldInfo{L"API_ROOT", DataExchange::StructFieldMetaInfo{&Config_::API_ROOT}},
            ObjectVariantMapper::StructFieldInfo{L"DEFAULT_API_PORT", DataExchange::StructFieldMetaInfo{&Config_::DEFAULT_API_PORT}},
        });
        return mapper;
    }();
    Config_ GetConfig_ ()
    {
        return Config_{
            nullopt,
            gAppConfiguration.Get ().WebServerPort.value_or (AppConfigurationType::kWebServerPort_Default)};
    }
}

namespace {
    const ObjectVariantMapper kSequenceOfGUIDMapper_ = [] () {
        using namespace BackendApp::Common;
        ObjectVariantMapper mapper;
        mapper.AddCommonType<GUID> ();
        mapper.AddCommonType<Set<GUID>> ();
        return mapper;
    }();
}

/*
 ********************************************************************************
 ********************************** WebServer ***********************************
 ********************************************************************************
 */
namespace {
    const ObjectVariantMapper kBasicsMapper_ = [] () {
        ObjectVariantMapper mapper;
        mapper.AddCommonType<Collection<String>> ();
        mapper.AddCommonType<Sequence<String>> ();
        return mapper;
    }();
}

namespace {
    constexpr Activity kContructing_WebServer_{L"constructing webserver"sv};
}

namespace {
    const unsigned int kMaxWSConnectionsPerUser_{4};   // empirically derived from looking at chrome --LGP 2021-01-14
    const unsigned int kMaxGUIConnectionssPerUser_{6}; // ''
    const unsigned int kMaxUsersSupported_{5};         // how many simultaneous users to support?
}

namespace {
    const ConstantProperty<Headers> kDefaultResponseHeadersStaticSite_{[] () {
        const String kServerString_ = L"Why-The-Fuck-Is-My-Network-So-Slow/"sv + AppVersion::kVersion.AsMajorMinorString ();
        Headers      h;
        h.server       = kServerString_;
        h.cacheControl = HTTP::CacheControl::kMustRevalidatePrivate;
        return h;
    }};
}

namespace {
    const ConstantProperty<FileSystemRequestHandler::Options> kStaticSiteHandlerOptions_{[] () {
        Sequence<pair<RegularExpression, CacheControl>> kFSCacheControlSettings_
        {
#if __cpp_designated_initializers
            pair<RegularExpression, CacheControl>{RegularExpression{L".*[0-9a-fA-F]+\\.(js|css|js\\.map)", CompareOptions::eCaseInsensitive}, CacheControl::kImmutable},
                pair<RegularExpression, CacheControl>{RegularExpression::kAny, CacheControl{.fCacheability = CacheControl::ePublic, .fMaxAge = Duration{24h}.As<int32_t> ()}},
#else
            pair<RegularExpression, CacheControl>{RegularExpression{L".*[0-9a-fA-F]+\\.(js|css|js\\.map)", CompareOptions::eCaseInsensitive}, CacheControl::kImmutable},
                pair<RegularExpression, CacheControl>{RegularExpression::kAny, CacheControl{CacheControl::ePublic, Duration{24h}.As<int32_t> ()}},
#endif
        };
        return FileSystemRequestHandler::Options{nullopt, Sequence<String>{L"index.html"_k}, nullopt, kFSCacheControlSettings_};
    }};
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
    static constexpr unsigned int kMaxWebServerConcurrentConnections_{kMaxUsersSupported_ * (kMaxWSConnectionsPerUser_ + kMaxGUIConnectionssPerUser_)};
    static constexpr unsigned int kMaxThreads_{kMaxWSConnectionsPerUser_ + kMaxGUIConnectionssPerUser_ + 1}; // handle the BURST quickly of requests at start, but then no need (just reduces startup latency), plus one just in case...

private:
    shared_ptr<IWSAPI>                                   fWSAPI_;
    const Sequence<Route>                                fWSRoutes_;
    const Sequence<Route>                                fStaticRoutes_;
    optional<DeclareActivity<Activity<wstring_view>>>    fEstablishActivity1_{&kContructing_WebServer_};
    ConnectionManager                                    fConnectionMgr_;
    [[NO_UNIQUE_ADDRESS_ATTR]] EmptyObjectForSideEffects fIgnore1_{[this] () { fEstablishActivity1_.reset (); }};

    atomic<unsigned int> fActiveCallCnt_{0};
    struct ActiveCallCounter_ {
        ActiveCallCounter_ (Rep_& r)
            : fRep_{r}
        {
            ++r.fActiveCallCnt_;
        }
        ~ActiveCallCounter_ ()
        {
            --fRep_.fActiveCallCnt_;
        }
        Rep_& fRep_;
    };
    IntervalTimer::Adder fIntervalTimerAdder_;

public:
    Rep_ (const shared_ptr<IWSAPI>& wsImpl)
        : fWSAPI_{wsImpl}
        , fWSRoutes_{
              /*
               *  To test this example:
               *      o   Run the service (under the debugger if you wish)
               *      o   curl  http://localhost/ -- to see web GUI
               *      o   curl  http://localhost/api -- to see a list of available web-methods
               */
              Route{
                  L"api"_RegEx,
                  DefaultPage_},

              Route{
                  L"api/v1/about"_RegEx,
                  mkRequestHandler (kAbout_, About::kMapper, function<About (void)>{[this] () { ActiveCallCounter_ acc{*this}; return fWSAPI_->GetAbout (); }})},

              Route{
                  L"api/v1/blob/(.+)"_RegEx,
                  [this] (Message* m, const String& id) {
                      ActiveCallCounter_                                             acc{*this};
                      tuple<Memory::BLOB, optional<DataExchange::InternetMediaType>> b = fWSAPI_->GetBLOB (id);
                      if (get<1> (b)) {
                          m->rwResponse ().contentType = *get<1> (b);
                      }
                      m->rwResponse ().write (get<0> (b));
                  }},

              Route{
                  L"api/v1/devices(/?)"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_                          acc{*this};
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (&m->rwRequest ());

                      DbgTrace (L"args=%s", Characters::ToString (args).c_str ());
                      optional<DeviceSortParamters> sort;
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
                      optional<Set<GUID>> ids = nullopt;
                      if (auto o = args.Lookup (L"ids"sv)) {
                          ids = kSequenceOfGUIDMapper_.ToObject<Set<GUID>> (DataExchange::Variant::JSON::Reader{}.Read (o->As<String> ()));
                      }
                      if (args.LookupValue (L"recurse"sv, false).As<bool> ()) {
                          WriteResponse (&m->rwResponse (), kDevices_, Device::kMapper.FromObject (fWSAPI_->GetDevices_Recurse (ids, sort)));
                      }
                      else {
                          WriteResponse (&m->rwResponse (), kDevices_, kBasicsMapper_.FromObject (fWSAPI_->GetDevices (ids, sort)));
                      }
                  }},
              Route{
                  L"api/v1/devices/(.+)"_RegEx,
                  [this] (Message* m, const String& id) {
                      ActiveCallCounter_ acc{*this};
                      WriteResponse (&m->rwResponse (), kDevices_, Device::kMapper.FromObject (fWSAPI_->GetDevice (id)));
                  }},
              Route{
                  IO::Network::HTTP::MethodsRegEx::kPatch,
                  L"api/v1/devices/(.+)"_RegEx,
                  [this] (Message* m, const String& id) {
                      ActiveCallCounter_ acc{*this};
                      fWSAPI_->PatchDevice (id, JSONPATCH::OperationItemsType::kMapper.ToObject<JSONPATCH::OperationItemsType> (DataExchange::Variant::JSON::Reader{}.Read (m->rwRequest ().GetBody ())));
                      m->rwResponse ().status = IO::Network::HTTP::StatusCodes::kNoContent;
                  }},

              Route{
                  L"api/v1/network-interfaces(/?)"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_                          acc{*this};
                      Mapping<String, DataExchange::VariantValue> args              = PickoutParamValues (&m->rwRequest ());
                      if (args.LookupValue (L"recurse"sv, false).As<bool> ()) {
                          WriteResponse (&m->rwResponse (), kNetworkInterfaces_, NetworkInterface::kMapper.FromObject (fWSAPI_->GetNetworkInterfaces_Recurse ()));
                      }
                      else {
                          WriteResponse (&m->rwResponse (), kNetworkInterfaces_, kBasicsMapper_.FromObject (fWSAPI_->GetNetworkInterfaces ()));
                      }
                  }},
              Route{
                  L"api/v1/network-interfaces/(.+)"_RegEx,
                  [this] (Message* m, const String& id) {
                      ActiveCallCounter_ acc{*this};
                      WriteResponse (&m->rwResponse (), kNetworkInterfaces_, NetworkInterface::kMapper.FromObject (fWSAPI_->GetNetworkInterface (id)));
                  }},

              Route{
                  L"api/v1/networks(/?)"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_                          acc{*this};
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (&m->rwRequest ());
                      optional<Set<GUID>>                         ids  = nullopt;
                      if (auto o = args.Lookup (L"ids"sv)) {
                          ids = kSequenceOfGUIDMapper_.ToObject<Set<GUID>> (DataExchange::Variant::JSON::Reader{}.Read (o->As<String> ()));
                      }
                      if (args.LookupValue (L"recurse"sv, false).As<bool> ()) {
                          WriteResponse (&m->rwResponse (), kNetworks_, Network::kMapper.FromObject (fWSAPI_->GetNetworks_Recurse (ids)));
                      }
                      else {
                          WriteResponse (&m->rwResponse (), kNetworks_, kBasicsMapper_.FromObject (fWSAPI_->GetNetworks (ids)));
                      }
                  }},
              Route{
                  L"api/v1/networks/(.+)"_RegEx,
                  [this] (Message* m, const String& id) {
                      ActiveCallCounter_ acc{*this};
                      WriteResponse (&m->rwResponse (), kNetworks_, Network::kMapper.FromObject (fWSAPI_->GetNetwork (id)));
                  }},
              Route{
                  IO::Network::HTTP::MethodsRegEx::kPatch,
                  L"api/v1/networks/(.+)"_RegEx,
                  [this] (Message* m, const String& id) {
                      ActiveCallCounter_ acc{*this};
                      fWSAPI_->PatchNetwork (id, JSONPATCH::OperationItemsType::kMapper.ToObject<JSONPATCH::OperationItemsType> (DataExchange::Variant::JSON::Reader{}.Read (m->rwRequest ().GetBody ())));
                      m->rwResponse ().status = IO::Network::HTTP::StatusCodes::kNoContent;
                  }},

              Route{
                  L"api/v1/operations/ping"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_                          acc{*this};
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (&m->rwRequest ());
                      if (auto address = args.Lookup (L"target"sv)) {
                          ExpectedMethod (m->request, kOperations_);
                          WriteResponse (&m->rwResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_Ping (address->As<String> ())));
                      }
                      else {
                          Execution::Throw (ClientErrorException{L"missing target argument"sv});
                      }
                  }},
              Route{
                  L"api/v1/operations/traceroute"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_                          acc{*this};
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (&m->rwRequest ());
                      optional<bool>                              reverseDNSResult;
                      if (auto rdr = args.Lookup (L"reverse-dns-result"sv)) {
                          reverseDNSResult = rdr->As<bool> ();
                      }
                      if (auto address = args.Lookup (L"target"sv)) {
                          ExpectedMethod (m->request, kOperations_);
                          WriteResponse (&m->rwResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_TraceRoute (address->As<String> (), reverseDNSResult)));
                      }
                      else {
                          Execution::Throw (ClientErrorException{L"missing target argument"sv});
                      }
                  }},
              Route{
                  L"api/v1/operations/dns/calculate-negative-lookup-time"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_ acc{*this};
                      ExpectedMethod (m->request, kOperations_);
                      optional<unsigned int>                      samples;
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (&m->rwRequest ());
                      if (auto rdr = args.Lookup (L"samples"sv)) {
                          samples = rdr->As<unsigned int> ();
                      }
                      WriteResponse (&m->rwResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_DNS_CalculateNegativeLookupTime (samples)));
                  }},
              Route{
                  L"api/v1/operations/dns/lookup"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_ acc{*this};
                      ExpectedMethod (m->request, kOperations_);
                      String                                      name;
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (&m->rwRequest ());
                      if (auto rdr = args.Lookup (L"name"sv)) {
                          name = rdr->As<String> ();
                      }
                      else {
                          Execution::Throw (ClientErrorException{L"missing name argument"sv});
                      }
                      WriteResponse (&m->rwResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_DNS_Lookup (name)));
                  }},
              Route{
                  L"api/v1/operations/dns/calculate-score"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_ acc{*this};
                      ExpectedMethod (m->request, kOperations_);
                      WriteResponse (&m->rwResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_DNS_CalculateScore ()));
                  }},
              Route{
                  L"api/v1/operations/scan/FullRescan"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_                          acc{*this};
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (&m->rwRequest ());
                      ExpectedMethod (m->request, kOperations_);
                      if (auto rdr = args.Lookup (L"deviceID"sv)) {
                          WriteResponse (&m->rwResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_Scan_FullRescan (rdr->As<String> ())));
                      }
                      else {
                          Execution::Throw (ClientErrorException{L"missing deviceID argument"sv});
                      }
                  }},
              Route{
                  L"api/v1/operations/scan/Scan"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_                          acc{*this};
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (&m->rwRequest ());
                      ExpectedMethod (m->request, kOperations_);
                      if (auto rdr = args.Lookup (L"addr"sv)) {
                          WriteResponse (&m->rwResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_Scan_Scan (rdr->As<String> ())));
                      }
                      else {
                          Execution::Throw (ClientErrorException{L"missing deviceID argument"sv});
                      }
                  }},
          }
        , fStaticRoutes_{
              Route{L"config.json"_RegEx, mkRequestHandler (kGUIConfig_, Config_::kMapper, function<Config_ (void)>{[=] () { return GetConfig_ (); }})},
              Route{RegularExpression::kAny, FileSystemRequestHandler{Execution::GetEXEDir () / "html", kStaticSiteHandlerOptions_}},
          }
#if __cpp_designated_initializers
        , fConnectionMgr_{SocketAddresses (InternetAddresses_Any (), gAppConfiguration.Get ().WebServerPort.value_or (AppConfigurationType::kWebServerPort_Default)), fWSRoutes_ + fStaticRoutes_, ConnectionManager::Options{.fMaxConnections = kMaxWebServerConcurrentConnections_, .fMaxConcurrentlyHandledConnections = kMaxThreads_, .fBindFlags = Socket::BindFlags{.fSO_REUSEADDR = true}, .fDefaultResponseHeaders = kDefaultResponseHeadersStaticSite_}}
#else
        , fConnectionMgr_{SocketAddresses (InternetAddresses_Any (), gAppConfiguration.Get ().WebServerPort.value_or (AppConfigurationType::kWebServerPort_Default)), fWSRoutes_ + fStaticRoutes_, ConnectionManager::Options{kMaxWebServerConcurrentConnections_, kMaxThreads_, Socket::BindFlags{true}, kDefaultResponseHeadersStaticSite_}}
#endif
        , fIntervalTimerAdder_{[this] () {
                                   OperationalStatisticsMgr::sThe.RecordActiveRunningTasksCount (fActiveCallCnt_);
                                   OperationalStatisticsMgr::sThe.RecordOpenConnectionCount (fConnectionMgr_.pConnections ().length ());
                                   OperationalStatisticsMgr::sThe.RecordActiveRunningTasksCount (fConnectionMgr_.pActiveConnections ().length ());
                               },
                               15s, IntervalTimer::Adder::eRunImmediately}
    {
        using Stroika::Frameworks::WebServer::DefaultFaultInterceptor;
        DefaultFaultInterceptor defaultHandler;
        fConnectionMgr_.defaultErrorHandler = DefaultFaultInterceptor{[defaultHandler] (Message* m, const exception_ptr& e) {
            // Unsure if we should bother recording 404s
            DbgTrace (L"faulting on request %s", Characters::ToString (m->request ()).c_str ());
            if (m->request ().url ().GetPath ().StartsWith (L"/api"sv, CompareOptions::eCaseInsensitive)) {
                OperationalStatisticsMgr::ProcessAPICmd::NoteError ();
            }
            defaultHandler.HandleFault (m, e);
        }};
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
    L"api/v1/about"sv,
    Set<String>{IO::Network::HTTP::Methods::kGet},
    DataExchange::InternetMediaTypes::kJSON,
    L"Data about the WTF application, version etc"sv,
    Sequence<String>{
        L"curl http://localhost/api/v1/about"sv,
    },
    Sequence<String>{L"Fetch the component versions, etc."sv},
};
const WebServiceMethodDescription WebServer::Rep_::kBlob_{
    L"api/v1/blob"sv,
    Set<String>{IO::Network::HTTP::Methods::kGet},
    nullopt,
    L"BLOBs (and their associated media type) generally sourced from other computers, but cached here so they will be available when those other computers are not (like icons from SSDP)"sv,
    Sequence<String>{
        L"curl http://localhost/api/v1/blob/{ID}"sv,
    },
    Sequence<String>{L"Fetch the blob by value (generally these links appear in GET /devices/{x} etc output)."sv},
};
const WebServiceMethodDescription WebServer::Rep_::kDevices_{
    L"api/v1/devices"sv,
    Set<String>{IO::Network::HTTP::Methods::kGet, IO::Network::HTTP::Methods::kPatch},
    DataExchange::InternetMediaTypes::kJSON,
    {},
    Sequence<String>{
        L"curl http://localhost/api/v1/devices"sv,
        L"curl http://localhost/api/v1/devices?recurse=true"sv,
        L"curl 'http://localhost/api/v1/devices?recurse=true&sort=%7b\"searchTerms\":[%7b\"by\":\"Address\"%7d],\"compareNetwork\":\"192.168.244.0/24\"%7d'"sv,
        L"curl 'http://localhost/api/v1/devices?recurse=true&sort={\"searchTerms\":[{\"by\":\"Address\"},{\"by\":\"Priority\"}],\"compareNetwork\":\"192.168.244.0/24\"}'"sv,
        L"curl http://localhost/api/v1/devices?recurse=true&sortBy=Address&sortCompareNetwork=192.168.244.0/24"sv,
        L"curl http://localhost/api/v1/devices/60c59f9c-9a69-c89e-9d99-99c7976869c5"sv,
        L"curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"add\",\"path\":\"/userOverrides/name\",\"value\":\"PROTY\"}]' http://localhost/api/v1/devices/a1fe525c-6bb7-271a-0f57-70d50f889dd3",
        L"curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"add\",\"path\":\"/userOverrides/notes\",\"value\":\"## Note1\\n##Note2\"}]' http://localhost/api/v1/devices/a1fe525c-6bb7-271a-0f57-70d50f889dd3",
        L"curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"add\",\"path\":\"/userOverrides/tags\",\"value\":[\"tag1\",\"tag2\"]}]' http://localhost/api/v1/devices/a1fe525c-6bb7-271a-0f57-70d50f889dd3",
        L"curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"remove\",\"path\":\"/userOverrides/tags\"}]' http://localhost/api/v1/devices/a1fe525c-6bb7-271a-0f57-70d50f889dd3"},
    Sequence<String>{
        L"Fetch the list of known devices for the currently connected network. By default, this list is sorted so the most interesting devices come first (like this machine is first)"sv,
        L"query-string: sort={[by: Address|Priority|Name|Type, ascending: true|false]+, compareNetwork?: CIDR|network-id}; sort=ARG is JSON encoded SearchTerm={by: string, ascending?: bool}, {searchTerms: SearchTerm[], compareNetwork: string}"sv,
        L"query-string: sortBy=Address|Priority|Name|Type sortAscending=true|false (requires sortBy); both are aliases for sort=...)"sv,
        L"query-string: ids=[a,b,c] - optional - if omitted returns all)"sv,
        L"Note: sorts are stable, so they can be combined one after the other. To get a GroupBy, just do the grouping as the final 'sort'."sv,
        L"For PATCH API, only supported operations are 'add /userSettings/name' and 'delete /userSettings/name' (and /userSettings/tags, /userSettings/notes)"sv},
};
const WebServiceMethodDescription WebServer::Rep_::kNetworks_{
    L"api/v1/networks"sv,
    Set<String>{IO::Network::HTTP::Methods::kGet},
    DataExchange::InternetMediaTypes::kJSON,
    {},
    Sequence<String>{L"curl http://localhost/api/v1/networks"sv,
                     L"curl http://localhost/api/v1/networks?recurse=true"sv,
                     L"curl http://localhost/api/v1/networks/{ID}"sv,
                     L"curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"add\",\"path\":\"/userOverrides/name\",\"value\":\"34churchst\"}]' http://localhost/api/v1/networks/d4a16729-ba16-5c2f-6187-0925a2f7025d",
                     L"curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"add\",\"path\":\"/userOverrides/notes\",\"value\":\"## Note1\\n##Note2\"}]' http://localhost/api/v1/networks/d4a16729-ba16-5c2f-6187-0925a2f7025d",
                     L"curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"add\",\"path\":\"/userOverrides/tags\",\"value\":[\"tag1\",\"tag2\"]}]' http://localhost/api/v1/networks/d4a16729-ba16-5c2f-6187-0925a2f7025d",
                     L"curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"remove\",\"path\":\"/userOverrides/tags\"}]' http://localhost/api/v1/networks/d4a16729-ba16-5c2f-6187-0925a2f7025d",
                     L"curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"add\",\"path\":\"/userOverrides/aggregateFingerprints\",\"value\":[\"449b7f39-0cba-6cf6-dc7d-1b1998566098\",\"27ea14ab-04c8-e8f5-ddff-a3bd24503497\"]}]' http://localhost/api/v1/networks/4b0103ed-5dfc-8639-0fb4-7dbcd62fb9fb",
                     L"curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"remove\",\"path\":\"/userOverrides/aggregateFingerprints\"}]' http://localhost/api/v1/networks/1e0dba3d-55f9-cdee-5d81-0a83aae5ab43",
                     L"curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"add\",\"path\":\"/userOverrides/aggregateGatewayHardwareAddresses\",\"value\":[\"70:97:41:94:1a:00\"]}]' http://localhost/api/v1/networks/d4a16729-ba16-5c2f-6187-0925a2f7025d",
                     L"curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"remove\",\"path\":\"/userOverrides/aggregateGatewayHardwareAddresses\"}]' http://localhost/api/v1/networks/d4a16729-ba16-5c2f-6187-0925a2f7025d"},
    Sequence<String>{L"Fetch the list of known Networks."sv,
                     L"query-string: ids=[a,b,c] - optional - if omitted returns all)"sv,
                     L"@todo - in the future - add support for parameters to this fetch - which can be used to filter/subset etc"sv,
                     L"For PATCH API, only supported operations are 'add /userSettings/name' and 'delete /userSettings/name' (and /userSettings/tags, /userSettings/notes)"sv},
};
const WebServiceMethodDescription WebServer::Rep_::kNetworkInterfaces_{
    L"api/v1/network-interfaces"sv,
    Set<String>{IO::Network::HTTP::Methods::kGet},
    DataExchange::InternetMediaTypes::kJSON,
    {},
    Sequence<String>{L"curl http://localhost/api/v1/network-interfaces", L"curl http://localhost/api/v1/network-interfaces?recurse=true"sv},
    Sequence<String>{L"Fetch the list of known Network Interfaces."sv,
                     L"[recurse=true|false]?"sv},
};
const WebServiceMethodDescription WebServer::Rep_::kOperations_{
    L"api/v1/operations"sv,
    Set<String>{IO::Network::HTTP::Methods::kGet},
    DataExchange::InternetMediaTypes::kJSON,
    {},
    Sequence<String>{
        L"curl http://localhost/api/v1/operations/ping?target=www.google.com"sv,
        L"curl http://localhost/api/v1/operations/traceroute?target=www.sophists.com"sv,
        L"curl http://localhost/api/v1/operations/dns/calculate-negative-lookup-time"sv,
        L"curl http://localhost/api/v1/operations/dns/lookup?name=www.youtube.com"sv,
        L"curl http://localhost/api/v1/operations/dns/calculate-score"sv,
        L"curl http://localhost/api/v1/operations/scan/FullRescan?device=ID"sv,
        L"curl http://localhost/api/v1/operations/scan/Scan?addr=hostOrIPAddr"sv,
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
    : fRep_{make_shared<Rep_> (wsImpl)}
{
}
