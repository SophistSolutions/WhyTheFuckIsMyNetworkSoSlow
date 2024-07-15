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
#include "Stroika/Foundation/Execution/Logger.h"
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

#include "../WebServices/WSImpl.h"

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
        "config"sv,
        Set<String>{IO::Network::HTTP::Methods::kGet},
        DataExchange::InternetMediaTypes::kJSON,
        "GUI config"sv,
        Sequence<String>{},
        Sequence<String>{"GUI config."sv},
    };
    const ObjectVariantMapper Config_::kMapper = [] () {
        ObjectVariantMapper mapper;
        mapper.AddCommonType<optional<String>> ();
        mapper.AddCommonType<optional<unsigned int>> ();
        mapper.AddClass<Config_> ({
            {"API_ROOT"sv, &Config_::API_ROOT},
            {"DEFAULT_API_PORT"sv, &Config_::DEFAULT_API_PORT},
        });
        return mapper;
    }();
    Config_ GetConfig_ ()
    {
        return Config_{nullopt, gAppConfiguration.Get ().WebServerPort.value_or (AppConfigurationType::kWebServerPort_Default)};
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
    constexpr unsigned int kMaxWSConnectionsPerUser_{4};   // empirically derived from looking at chrome --LGP 2021-01-14
    constexpr unsigned int kMaxGUIConnectionssPerUser_{6}; // ''
    constexpr unsigned int kMaxUsersSupported_{5};         // how many simultaneous users to support?
}

namespace {
    const ConstantProperty<Headers> kDefaultResponseHeadersStaticSite_{[] () {
        const String kServerString_ = "Why-The-Fuck-Is-My-Network-So-Slow/"sv + AppVersion::kVersion.AsMajorMinorString ();
        Headers      h;
        h.server = kServerString_;
        //h.cacheControl = HTTP::CacheControl::kMustRevalidatePrivate;
        h.cacheControl = HTTP::CacheControl{/*.fCacheability=*/HTTP::CacheControl::ePublic};
        return h;
    }};
    CacheControl                    mkCacheControlForAPI_ (Duration ttl)
    {
        //auto cc    = HTTP::CacheControl::kMustRevalidatePrivate;
        auto cc    = HTTP::CacheControl{/*.fCacheability=*/HTTP::CacheControl::ePrivate};
        cc.fMaxAge = static_cast<uint32_t> (ttl.As<int> ());
        return cc;
    }
}

namespace {
    const ConstantProperty<FileSystemRequestHandler::Options> kStaticSiteHandlerOptions_{[] () {
        Sequence<pair<RegularExpression, CacheControl>> kFSCacheControlSettings_{
            pair<RegularExpression, CacheControl>{RegularExpression{".*[0-9a-fA-F]+\\.(js|css|js\\.map)"sv, CompareOptions::eCaseInsensitive},
                                                  CacheControl::kImmutable},
            pair<RegularExpression, CacheControl>{RegularExpression::kAny,
                                                  CacheControl{.fCacheability = CacheControl::ePublic, .fMaxAge = Duration{24h}.As<int32_t> ()}},
        };
        return FileSystemRequestHandler::Options{nullopt, Sequence<String>{"index.html"_k}, nullopt, kFSCacheControlSettings_};
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
    shared_ptr<IWSAPI>                                fWSAPI_;
    const Sequence<Route>                             fWSRoutes_;
    const Sequence<Route>                             fStaticRoutes_;
    optional<DeclareActivity<Activity<wstring_view>>> fEstablishActivity1_{&kContructing_WebServer_};
    ConnectionManager                                 fConnectionMgr_;
    [[no_unique_address]] EmptyObjectForSideEffects   fIgnore1_{[this] () { fEstablishActivity1_.reset (); }};

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
    Rep_ ()
        : fWSAPI_{make_shared<WSImpl> ([this] () -> About::APIServerInfo::WebServer {
                About::APIServerInfo::WebServer r;
                auto rr = this->fConnectionMgr_.statistics();
                r.fThreadPool.fThreads             = kMaxThreads_; // todo begingings of data to report
                r.fThreadPool.fTasksStillQueued = rr.fThreadPoolStatistics.fNumberOfTasksAdded - rr.fThreadPoolStatistics.fNumberOfTasksCompleted;
                r.fThreadPool.fAverageTaskRunTime = rr.fThreadPoolStatistics.GetMeanTimeConsumed ();
                return r;
            })}
        , fWSRoutes_{
              /*
               *  To test this example:
               *      o   Run the service (under the debugger if you wish)
               *      o   curl  http://localhost/ -- to see web GUI
               *      o   curl  http://localhost/api -- to see a list of available web-methods
               */
              Route{
                  "api"_RegEx,
                  DefaultPage_},

              Route{
                  "api/v1/about"_RegEx,
                  mkRequestHandler (kAbout_, About::kMapper, function<About (void)>{[this] () { ActiveCallCounter_ acc{*this}; return fWSAPI_->GetAbout (); }})},

              Route{
                  "api/v1/blob/(.+)"_RegEx,
                  [this] (Message* m, const String& id) {
                      ActiveCallCounter_                                             acc{*this};
                      tuple<Memory::BLOB, optional<DataExchange::InternetMediaType>> b = fWSAPI_->GetBLOB (id);
                      if (get<1> (b)) {
                          m->rwResponse ().contentType = *get<1> (b);
                      }
                      m->rwResponse ().write (get<0> (b));
                  }},

              Route{
                 "api/v1/devices(/?)"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_                          acc{*this};
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (&m->rwRequest ());
                      optional<DeviceSortParameters>              sort;
                      if (auto o = args.Lookup ("sort"sv)) {
                          ClientErrorException::TreatExceptionsAsClientError ([&] () {
                              sort = DeviceSortParameters::kMapper.ToObject<DeviceSortParameters> (
                                  DataExchange::Variant::JSON::Reader{}.Read (o->As<String> ()));
                          });
                      }
                      if (auto o = args.Lookup ("sortBy"sv)) {
                          ClientErrorException::TreatExceptionsAsClientError ([&] () {
                              sort = sort.value_or (DeviceSortParameters{});
                              sort->fSearchTerms +=
                                  DeviceSortParameters::SearchTerm{Configuration::DefaultNames<DeviceSortParameters::SearchTerm::By>{}.GetValue (
                                      o->As<String> ().c_str (), ClientErrorException{
                                                                                                                                              "Invalid argument to query string sortBy"sv})};
                          });
                      }
                      optional<Set<GUID>> ids = nullopt;
                      if (auto o = args.Lookup ("ids"sv)) {
                          ids = kSequenceOfGUIDMapper_.ToObject<Set<GUID>> (DataExchange::Variant::JSON::Reader{}.Read (o->As<String> ()));
                      }
                      if (args.LookupValue ("recurse"sv, false).As<bool> ()) {
                          WriteResponse (&m->rwResponse (), kDevices_, Device::kMapper.FromObject (fWSAPI_->GetDevices_Recurse (ids, sort)));
                      }
                      else {
                          WriteResponse (&m->rwResponse (), kDevices_, kBasicsMapper_.FromObject (fWSAPI_->GetDevices (ids, sort)));
                      }
                  }},
              Route{
                  "api/v1/devices/(.+)"_RegEx,
                  [this] (Message* m, const String& id) {
                      ActiveCallCounter_ acc{*this};
                      auto [device, ttl]                         = fWSAPI_->GetDevice (id);
                      m->rwResponse ().rwHeaders ().cacheControl = mkCacheControlForAPI_ (ttl);
                      WriteResponse (&m->rwResponse (), kDevices_, Device::kMapper.FromObject (device));
                  }},
              Route{
                  IO::Network::HTTP::MethodsRegEx::kPatch,
                "api/v1/devices/(.+)"_RegEx,
                  [this] (Message* m, const String& id) {
                      ActiveCallCounter_ acc{*this};
                      fWSAPI_->PatchDevice (id, DataExchange::JSON::Patch::OperationItemsType::kMapper.ToObject<DataExchange::JSON::Patch::OperationItemsType> (DataExchange::Variant::JSON::Reader{}.Read (m->rwRequest ().GetBody ())));
                      m->rwResponse ().status = IO::Network::HTTP::StatusCodes::kNoContent;
                  }},

              Route{
                 "api/v1/network-interfaces(/?)"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_                          acc{*this};
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (&m->rwRequest ());
                      if (args.LookupValue ("recurse"sv, false).As<bool> ()) {
                          WriteResponse (&m->rwResponse (), kNetworkInterfaces_, NetworkInterface::kMapper.FromObject (fWSAPI_->GetNetworkInterfaces_Recurse ()));
                      }
                      else {
                          WriteResponse (&m->rwResponse (), kNetworkInterfaces_, kBasicsMapper_.FromObject (fWSAPI_->GetNetworkInterfaces ()));
                      }
                  }},
              Route{
                 "api/v1/network-interfaces/(.+)"_RegEx,
                  [this] (Message* m, const String& id) {
                      ActiveCallCounter_ acc{*this};
                      auto [networkInterface, ttl]               = fWSAPI_->GetNetworkInterface (id);
                      m->rwResponse ().rwHeaders ().cacheControl = mkCacheControlForAPI_ (ttl);
                      WriteResponse (&m->rwResponse (), kNetworkInterfaces_, NetworkInterface::kMapper.FromObject (networkInterface));
                  }},

              Route{
                 "api/v1/networks(/?)"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_                          acc{*this};
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (&m->rwRequest ());
                      optional<Set<GUID>>                         ids  = nullopt;
                      if (auto o = args.Lookup ("ids"sv)) {
                          ids = kSequenceOfGUIDMapper_.ToObject<Set<GUID>> (DataExchange::Variant::JSON::Reader{}.Read (o->As<String> ()));
                      }
                      if (args.LookupValue ("recurse"sv, false).As<bool> ()) {
                          WriteResponse (&m->rwResponse (), kNetworks_, Network::kMapper.FromObject (fWSAPI_->GetNetworks_Recurse (ids)));
                      }
                      else {
                          WriteResponse (&m->rwResponse (), kNetworks_, kBasicsMapper_.FromObject (fWSAPI_->GetNetworks (ids)));
                      }
                  }},
              Route{
                  "api/v1/networks/(.+)"_RegEx,
                  [this] (Message* m, const String& id) {
                      ActiveCallCounter_ acc{*this};
                      auto [network, ttl]                        = fWSAPI_->GetNetwork (id);
                      m->rwResponse ().rwHeaders ().cacheControl = mkCacheControlForAPI_ (ttl);
                      WriteResponse (&m->rwResponse (), kNetworks_, Network::kMapper.FromObject (network));
                  }},
              Route{
                  IO::Network::HTTP::MethodsRegEx::kPatch,
                 "api/v1/networks/(.+)"_RegEx,
                  [this] (Message* m, const String& id) {
                      ActiveCallCounter_ acc{*this};
                      fWSAPI_->PatchNetwork (id, DataExchange::JSON::Patch::OperationItemsType::kMapper.ToObject<DataExchange::JSON::Patch::OperationItemsType> (DataExchange::Variant::JSON::Reader{}.Read (m->rwRequest ().GetBody ())));
                      m->rwResponse ().status = IO::Network::HTTP::StatusCodes::kNoContent;
                  }},

              Route{
                  "api/v1/operations/ping"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_                          acc{*this};
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (&m->rwRequest ());
                      if (auto address = args.Lookup ("target"sv)) {
                          ExpectedMethod (m->request, kOperations_);
                          WriteResponse (&m->rwResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_Ping (address->As<String> ())));
                      }
                      else {
                          Execution::Throw (ClientErrorException{"missing target argument"sv});
                      }
                  }},
              Route{
                  "api/v1/operations/traceroute"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_                          acc{*this};
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (&m->rwRequest ());
                      optional<bool>                              reverseDNSResult;
                      if (auto rdr = args.Lookup ("reverse-dns-result"sv)) {
                          reverseDNSResult = rdr->As<bool> ();
                      }
                      if (auto address = args.Lookup ("target"sv)) {
                          ExpectedMethod (m->request, kOperations_);
                          WriteResponse (&m->rwResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_TraceRoute (address->As<String> (), reverseDNSResult)));
                      }
                      else {
                          static const auto kException_ = ClientErrorException{"missing target argument"sv};
                          Execution::Throw (kException_);
                      }
                  }},
              Route{
                  "api/v1/operations/dns/calculate-negative-lookup-time"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_ acc{*this};
                      ExpectedMethod (m->request, kOperations_);
                      optional<unsigned int>                      samples;
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (&m->rwRequest ());
                      if (auto rdr = args.Lookup ("samples"sv)) {
                          samples = rdr->As<unsigned int> ();
                      }
                      WriteResponse (&m->rwResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_DNS_CalculateNegativeLookupTime (samples)));
                  }},
              Route{
                  "api/v1/operations/dns/lookup"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_ acc{*this};
                      ExpectedMethod (m->request, kOperations_);
                      String                                      name;
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (&m->rwRequest ());
                      if (auto rdr = args.Lookup ("name"sv)) {
                          name = rdr->As<String> ();
                      }
                      else {
                          static const auto kException_ = ClientErrorException{"missing name argument"sv};
                          Execution::Throw (kException_);
                      }
                      WriteResponse (&m->rwResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_DNS_Lookup (name)));
                  }},
              Route{
                 "api/v1/operations/dns/calculate-score"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_ acc{*this};
                      ExpectedMethod (m->request, kOperations_);
                      WriteResponse (&m->rwResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_DNS_CalculateScore ()));
                  }},
              Route{
                  "api/v1/operations/scan/FullRescan"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_                          acc{*this};
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (&m->rwRequest ());
                      ExpectedMethod (m->request, kOperations_);
                      if (auto rdr = args.Lookup ("deviceID"sv)) {
                          WriteResponse (&m->rwResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_Scan_FullRescan (rdr->As<String> ())));
                      }
                      else {
                          static const auto kException_ = ClientErrorException{"missing deviceID argument"sv};
                          Execution::Throw (kException_);
                      }
                  }},
              Route{
                  "api/v1/operations/scan/Scan"_RegEx,
                  [this] (Message* m) {
                      ActiveCallCounter_                          acc{*this};
                      Mapping<String, DataExchange::VariantValue> args = PickoutParamValues (&m->rwRequest ());
                      ExpectedMethod (m->request, kOperations_);
                      if (auto rdr = args.Lookup ("addr"sv)) {
                          WriteResponse (&m->rwResponse (), kOperations_, Operations::kMapper.FromObject (fWSAPI_->Operation_Scan_Scan (rdr->As<String> ())));
                      }
                      else {
                          static const auto kException_ = ClientErrorException{"missing addr argument"sv};
                          Execution::Throw (kException_);
                      }
                  }},
          }
        , fStaticRoutes_{
              Route{"config.json"_RegEx, mkRequestHandler (kGUIConfig_, Config_::kMapper, function<Config_ (void)>{[=] () { return GetConfig_ (); }})},
              Route{RegularExpression::kAny, FileSystemRequestHandler{Execution::GetEXEDir () / "html"sv, kStaticSiteHandlerOptions_}},
          }
        , fConnectionMgr_{
            SocketAddresses (InternetAddresses_Any (), gAppConfiguration.Get ().WebServerPort.value_or (AppConfigurationType::kWebServerPort_Default)), 
            fWSRoutes_ + fStaticRoutes_, 
            ConnectionManager::Options{
                .fMaxConnections = kMaxWebServerConcurrentConnections_
                , .fMaxConcurrentlyHandledConnections = kMaxThreads_
                , .fBindFlags = Socket::BindFlags{.fSO_REUSEADDR = true}
                , .fDefaultResponseHeaders = kDefaultResponseHeadersStaticSite_
             ,.fCollectStatistics = true
            }}
        , fIntervalTimerAdder_{[this] () {
                                   Debug::TraceContextBumper ctx{"webserver status gather TIMER HANDLER"}; // to debug https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/78
                                   OperationalStatisticsMgr::sThe.RecordActiveRunningTasksCount (fActiveCallCnt_);
                                   OperationalStatisticsMgr::sThe.RecordOpenConnectionCount (fConnectionMgr_.connections ().length ());
                                   OperationalStatisticsMgr::sThe.RecordActiveRunningTasksCount (fConnectionMgr_.activeConnections ().length ());
                               },
                               15s, IntervalTimer::Adder::eRunImmediately}
    {
        using Stroika::Frameworks::WebServer::DefaultFaultInterceptor;
        DefaultFaultInterceptor defaultHandler;
        fConnectionMgr_.defaultErrorHandler = DefaultFaultInterceptor{[defaultHandler] (Message* m, const exception_ptr& e) {
            // Unsure if we should bother recording 404s
            DbgTrace ("faulting on request {}"_f, Characters::ToString (m->request ()));
            if (m->request ().url ().GetPath ().StartsWith ("/api"sv, CompareOptions::eCaseInsensitive)) {
                OperationalStatisticsMgr::ProcessAPICmd::NoteError ();
            }
            defaultHandler.HandleFault (m, e);
        }};
        Logger::sThe.Log (Logger::eInfo, "Started WebServices on {}"_f, fConnectionMgr_.bindings ());
    }
    static void DefaultPage_ ([[maybe_unused]] Request* request, Response* response)
    {
        WriteDocsPage (response,
                       Sequence<WebServiceMethodDescription>{
                           kAbout_,
                           kBlob_,
                           kDevices_,
                           kNetworkInterfaces_,
                           kNetworks_,
                           kOperations_,
                       },
                       DocsOptions{"Web Methods"sv});
    }
};

const WebServiceMethodDescription WebServer::Rep_::kAbout_{
    "api/v1/about"sv,
    Set<String>{IO::Network::HTTP::Methods::kGet},
    DataExchange::InternetMediaTypes::kJSON,
    "Data about the WTF application, version etc"sv,
    Sequence<String>{
        "curl http://localhost/api/v1/about"sv,
    },
    Sequence<String>{"Fetch the component versions, etc."sv},
};
const WebServiceMethodDescription WebServer::Rep_::kBlob_{
    "api/v1/blob"sv,
    Set<String>{IO::Network::HTTP::Methods::kGet},
    nullopt,
    "BLOBs (and their associated media type) generally sourced from other computers, but cached here so they will be available when those other computers are not (like icons from SSDP)"sv,
    Sequence<String>{
        "curl http://localhost/api/v1/blob/{ID}"sv,
    },
    Sequence<String>{"Fetch the blob by value (generally these links appear in GET /devices/{x} etc output)."sv},
};
const WebServiceMethodDescription WebServer::Rep_::kDevices_{
    "api/v1/devices"sv,
    Set<String>{IO::Network::HTTP::Methods::kGet, IO::Network::HTTP::Methods::kPatch},
    DataExchange::InternetMediaTypes::kJSON,
    {},
    Sequence<String>{
        "curl http://localhost/api/v1/devices"sv, "curl http://localhost/api/v1/devices?recurse=true"sv,
        "curl 'http://localhost/api/v1/devices?recurse=true&sort=%7b\"searchTerms\":[%7b\"by\":\"Address\"%7d],\"compareNetwork\":\"192.168.244.0/24\"%7d'"sv,
        "curl 'http://localhost/api/v1/devices?recurse=true&sort={\"searchTerms\":[{\"by\":\"Address\"},{\"by\":\"Priority\"}],\"compareNetwork\":\"192.168.244.0/24\"}'"sv,
        "curl http://localhost/api/v1/devices?recurse=true&sortBy=Address&sortCompareNetwork=192.168.244.0/24"sv,
        "curl http://localhost/api/v1/devices/60c59f9c-9a69-c89e-9d99-99c7976869c5"sv, "curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"add\",\"path\":\"/userOverrides/name\",\"value\":\"PROTY\"}]' http://localhost/api/v1/devices/a1fe525c-6bb7-271a-0f57-70d50f889dd3", "curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"add\",\"path\":\"/userOverrides/notes\",\"value\":\"## Note1\\n##Note2\"}]' http://localhost/api/v1/devices/a1fe525c-6bb7-271a-0f57-70d50f889dd3", "curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"add\",\"path\":\"/userOverrides/tags\",\"value\":[\"tag1\",\"tag2\"]}]' http://localhost/api/v1/devices/a1fe525c-6bb7-271a-0f57-70d50f889dd3",
        "curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"remove\",\"path\":\"/userOverrides/tags\"}]' "
        "http://localhost/api/v1/devices/a1fe525c-6bb7-271a-0f57-70d50f889dd3"},
    Sequence<String>{
        "Fetch the list of known devices for the currently connected network. By default, this list is sorted so the most interesting devices come first (like this machine is first)"sv,
        "query-string: sort={[by: Address|Priority|Name|Type, ascending: true|false]+, compareNetwork?: CIDR|network-id}; sort=ARG is JSON encoded SearchTerm={by: string, ascending?: bool}, {searchTerms: SearchTerm[], compareNetwork: string}"sv,
        "query-string: sortBy=Address|Priority|Name|Type sortAscending=true|false (requires sortBy); both are aliases for sort=...)"sv,
        "query-string: ids=[a,b,c] - optional - if omitted returns all)"sv,
        "Note: sorts are stable, so they can be combined one after the other. To get a GroupBy, just do the grouping as the final 'sort'."sv,
        "For PATCH API, only supported operations are 'add /userSettings/name' and 'delete /userSettings/name' (and /userSettings/tags, /userSettings/notes)"sv},
};
const WebServiceMethodDescription WebServer::Rep_::kNetworks_{
    "api/v1/networks"sv,
    Set<String>{IO::Network::HTTP::Methods::kGet},
    DataExchange::InternetMediaTypes::kJSON,
    {},
    Sequence<String>{
        "curl http://localhost/api/v1/networks"sv, "curl http://localhost/api/v1/networks?recurse=true"sv,
        "curl http://localhost/api/v1/networks/{ID}"sv, "curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"add\",\"path\":\"/userOverrides/name\",\"value\":\"34churchst\"}]' http://localhost/api/v1/networks/d4a16729-ba16-5c2f-6187-0925a2f7025d", "curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"add\",\"path\":\"/userOverrides/notes\",\"value\":\"## Note1\\n##Note2\"}]' http://localhost/api/v1/networks/d4a16729-ba16-5c2f-6187-0925a2f7025d", "curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"add\",\"path\":\"/userOverrides/tags\",\"value\":[\"tag1\",\"tag2\"]}]' http://localhost/api/v1/networks/d4a16729-ba16-5c2f-6187-0925a2f7025d", "curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"remove\",\"path\":\"/userOverrides/tags\"}]' http://localhost/api/v1/networks/d4a16729-ba16-5c2f-6187-0925a2f7025d", "curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d '[{\"op\":\"add\",\"path\":\"/userOverrides/aggregateFingerprints\",\"value\":[\"449b7f39-0cba-6cf6-dc7d-1b1998566098\",\"27ea14ab-04c8-e8f5-ddff-a3bd24503497\"]}]' http://localhost/api/v1/networks/4b0103ed-5dfc-8639-0fb4-7dbcd62fb9fb",
        "curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d "
        "'[{\"op\":\"remove\",\"path\":\"/userOverrides/aggregateFingerprints\"}]' "
        "http://localhost/api/v1/networks/1e0dba3d-55f9-cdee-5d81-0a83aae5ab43",
        "curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d "
        "'[{\"op\":\"add\",\"path\":\"/userOverrides/aggregateGatewayHardwareAddresses\",\"value\":[\"70:97:41:94:1a:00\"]}]' "
        "http://localhost/api/v1/networks/d4a16729-ba16-5c2f-6187-0925a2f7025d",
        "curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d "
        "'[{\"op\":\"remove\",\"path\":\"/userOverrides/aggregateGatewayHardwareAddresses\"}]' "
        "http://localhost/api/v1/networks/d4a16729-ba16-5c2f-6187-0925a2f7025d",
        "curl -v -X PATCH --output - -H \"Content-Type: application/json\" -d "
        "'[{\"op\":\"add\",\"path\":\"/userOverrides/aggregateNetworkInterfacesMatching\",\"value\":[{\"fingerprint\": "
        "\"9da26b0d-0ff5-8e69-5610-c4fe39f84794\", \"interfaceType\": \"Device-Virtual-Internal-Network\" } ]}]' "
        "http://localhost/api/v1/networks/a4fbd565-cea8-3885-8393-2d7ac6954457"},
    Sequence<String>{"Fetch the list of known Networks."sv, "query-string: ids=[a,b,c] - optional - if omitted returns all)"sv,
                     "@todo - in the future - add support for parameters to this fetch - which can be used to filter/subset etc"sv,
                     "For PATCH API, only supported operations are 'add /userSettings/name' and 'delete /userSettings/name' (and /userSettings/tags, /userSettings/notes)"sv},
};
const WebServiceMethodDescription WebServer::Rep_::kNetworkInterfaces_{
    "api/v1/network-interfaces"sv,
    Set<String>{IO::Network::HTTP::Methods::kGet},
    DataExchange::InternetMediaTypes::kJSON,
    {},
    Sequence<String>{"curl http://localhost/api/v1/network-interfaces", "curl http://localhost/api/v1/network-interfaces?recurse=true"sv},
    Sequence<String>{"Fetch the list of known Network Interfaces."sv, "[recurse=true|false]?"sv},
};
const WebServiceMethodDescription WebServer::Rep_::kOperations_{
    "api/v1/operations"sv,
    Set<String>{IO::Network::HTTP::Methods::kGet},
    DataExchange::InternetMediaTypes::kJSON,
    {},
    Sequence<String>{
        "curl http://localhost/api/v1/operations/ping?target=www.google.com"sv,
        "curl http://localhost/api/v1/operations/traceroute?target=www.sophists.com"sv,
        "curl http://localhost/api/v1/operations/dns/calculate-negative-lookup-time"sv,
        "curl http://localhost/api/v1/operations/dns/lookup?name=www.youtube.com"sv,
        "curl http://localhost/api/v1/operations/dns/calculate-score"sv,
        "curl http://localhost/api/v1/operations/scan/FullRescan?device=ID"sv,
        "curl http://localhost/api/v1/operations/scan/Scan?addr=hostOrIPAddr"sv,
    },
    Sequence<String>{
        "perform a wide variety of operations - mostly for debugging for now but may stay around."sv,
        "/operations/ping?target=address; (address can be ipv4, ipv6 address, or dnsname)"sv,
        "/operations/traceroute?target=address[&reverse-dns-result=bool]?; (address can be ipv4, ipv6 address, or dnsname)"sv,
        "/operations/dns/calculate-negative-lookup-time[&samples=uint]?"sv,
        "/operations/dns/lookup[&name=string]"sv,
        "/operations/dns/calculate-score; returns number 0 (worst) to 1.0 (best)"sv,
        "/operations/scan/FullRescan?device=ID; clears found ports for deviceID, and immediately rescans before returning; returns summary"sv,
        "/operations/scan/operations/scan/Scan?addr=hostOrIPAddr; doesnt affect internal strucutres, and just runs scan process on given IP and returns result"sv,
    },
};

WebServer::WebServer ()
    : fRep_{make_shared<Rep_> ()}
{
}
