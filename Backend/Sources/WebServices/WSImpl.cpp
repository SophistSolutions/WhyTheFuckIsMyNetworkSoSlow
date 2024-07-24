/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#if qHasFeature_boost
#include <boost/version.hpp>
#endif
#if qHasFeature_LibCurl
// For CURLCode define
#include <curl/curl.h>
#endif
#if qHasFeature_sqlite
#include <sqlite/sqlite3.h>
#endif
#if qHasFeature_OpenSSL
#include <openssl/opensslv.h>
#endif

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Configuration/SystemConfiguration.h"
#include "Stroika/Foundation/Containers/Sequence.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/Database/SQL/SQLite.h"
#include "Stroika/Foundation/Debug/TimingTrace.h"
#include "Stroika/Foundation/Execution/Process.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/DNS.h"
#include "Stroika/Foundation/IO/Network/HTTP/ClientErrorException.h"
#include "Stroika/Foundation/IO/Network/Interface.h"
#include "Stroika/Foundation/IO/Network/LinkMonitor.h"
#include "Stroika/Foundation/Time/DateTime.h"

#include "Stroika/Frameworks/NetworkMonitor/Ping.h"
#include "Stroika/Frameworks/NetworkMonitor/Traceroute.h"

#include "Stroika/Frameworks/SystemPerformance/Capturer.h"
#include "Stroika/Frameworks/SystemPerformance/Instruments/CPU.h"
#include "Stroika/Frameworks/SystemPerformance/Instruments/Memory.h"
#include "Stroika/Frameworks/SystemPerformance/Instruments/Process.h"
#include "Stroika/Frameworks/SystemPerformance/Measurement.h"

//tmphack
#include "Stroika/Foundation/Execution/IntervalTimer.h"

#include "Stroika-Current-Version.h"

#include "../Common/BLOBMgr.h"
#include "../Common/DB.h"
#include "../Common/OperationalStatistics.h"
#include "../Discovery/Devices.h"
#include "../IntegratedModel/Mgr.h"

#include "AppVersion.h"

#include "WSImpl.h"

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Execution;
using namespace Stroika::Foundation::Time;

using namespace Stroika::Frameworks::SystemPerformance;

using IO::Network::URI;
using Stroika::Foundation::Common::GUID;
using Stroika::Foundation::IO::Network::HTTP::ClientErrorException;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;

namespace {

    static const Duration kCaptureFrequency_ = 30s;

    struct MyCapturer_ final : Capturer {
    public:
        Instruments::CPU::Instrument     fCPUInstrument{};
        Instruments::Process::Instrument fProcessInstrument{
            Instruments::Process::Options{.fRestrictToPIDs = Set<pid_t>{Execution::GetCurrentProcessID ()}}};
        MyCapturer_ ()
        {
            AddCaptureSet (CaptureSet{kCaptureFrequency_, {fCPUInstrument, fProcessInstrument}});
        }
    };
}

/*
 ********************************************************************************
 ************************************* WSImpl ***********************************
 ********************************************************************************
 */
struct WSImpl::Rep_ {
    MyCapturer_                                  fMyCapturer;
    function<About::APIServerInfo::WebServer ()> fWebServerStatsFetcher;
};
WSImpl::WSImpl (function<About::APIServerInfo::WebServer ()> webServerStatsFetcher)
    : fRep_{make_shared<Rep_> ()}
{
    fRep_->fWebServerStatsFetcher = webServerStatsFetcher;
}

About WSImpl::GetAbout () const
{
    DbgTrace ("intervalutimertasks={}"_f, Execution::IntervalTimer::Manager::sThe.GetAllRegisteredTasks ()); // to debug https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/78
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;
    using APIServerInfo  = About::APIServerInfo;
    using ComponentInfo  = APIServerInfo::ComponentInfo;
    using CurrentMachine = APIServerInfo::CurrentMachine;
    using CurrentProcess = APIServerInfo::CurrentProcess;
    using APIEndpoint    = APIServerInfo::APIEndpoint;
    using Database       = APIServerInfo::Database;
    static const Sequence<ComponentInfo> kAPIServerComponents_{initializer_list<ComponentInfo>{
        ComponentInfo{"Stroika"sv, Configuration::Version{kStroika_Version_FullVersion}.AsPrettyVersionString (), URI{"https://github.com/SophistSolutions/Stroika"}}
#if qHasFeature_OpenSSL
        ,
        ComponentInfo{"OpenSSL"sv, OPENSSL_VERSION_TEXT, URI{"https://www.openssl.org/"}}
#endif
#if qHasFeature_LibCurl
        ,
        ComponentInfo{"libcurl"sv, LIBCURL_VERSION, URI{"https://curl.se/"}}
#endif
#if qHasFeature_boost
        ,
        ComponentInfo{"boost"sv, String{BOOST_LIB_VERSION}}
#endif
#if qHasFeature_sqlite
        ,
        ComponentInfo{"sqlite"sv, SQLITE_VERSION, URI{"https://www.sqlite.org"}}
#endif
    }};
    auto now = DateTime::Now ();
    auto measurements = fRep_->fMyCapturer.pMostRecentMeasurements (); // capture results on a regular cadence with MyCapturer, and just report the latest stats

    CurrentMachine machineInfo = [this, now, &measurements] () {
        CurrentMachine    result;
        static const auto kOS_  = OperatingSystem{Configuration::GetSystemConfiguration_ActualOperatingSystem ().fTokenName,
                                                 Configuration::GetSystemConfiguration_ActualOperatingSystem ().fPrettyNameWithVersionDetails};
        result.fOperatingSystem = kOS_;
        if (auto o = Configuration::GetSystemConfiguration_BootInformation ().fBootedAt) {
            result.fMachineUptime = now - *o;
        }
        if (auto om = fRep_->fMyCapturer.fCPUInstrument.MeasurementAs<Instruments::CPU::Info> (measurements)) {
            result.fRunQLength    = om->fRunQLength;
            result.fTotalCPUUsage = om->fTotalCPUUsage;
        }
        return result;
    }();

    CurrentProcess processInfo = [this, now, &measurements] () {
        CurrentProcess result;
        if (auto om = fRep_->fMyCapturer.fProcessInstrument.MeasurementAs<Instruments::Process::Info> (measurements)) {
            Assert (om->size () == 1);
            Instruments::Process::ProcessType thisProcess = (*om)[Execution::GetCurrentProcessID ()];
            if (auto o = thisProcess.fProcessStartedAt) {
                result.fProcessUptime = now - *o;
            }
            result.fAverageCPUTimeUsed = thisProcess.fAverageCPUTimeUsed ? thisProcess.fAverageCPUTimeUsed->count () : optional<double>{};
            result.fWorkingOrResidentSetSize = Memory::NullCoalesce (thisProcess.fWorkingSetSize, thisProcess.fResidentMemorySize);
            result.fCombinedIOReadRate       = thisProcess.fCombinedIOReadRate;
            result.fCombinedIOWriteRate      = thisProcess.fCombinedIOWriteRate;
        }
        return result;
    }();

    Common::OperationalStatisticsMgr::Statistics stats    = Common::OperationalStatisticsMgr::sThe.GetStatistics ();
    APIEndpoint                                  apiStats = [&] () {
        APIEndpoint r;
        r.fCallsCompleted                       = stats.fRecentAPI.fCallsCompleted;
        r.fMeanDuration                         = stats.fRecentAPI.fMeanDuration;
        r.fMedianDuration                       = stats.fRecentAPI.fMedianDuration;
        r.fMaxDuration                          = stats.fRecentAPI.fMaxDuration;
        r.fErrors                               = stats.fRecentAPI.fErrors;
        r.fMedianWebServerConnections           = stats.fRecentAPI.fMedianWebServerConnections;
        r.fMedianProcessingWebServerConnections = stats.fRecentAPI.fMedianProcessingWebServerConnections;
        r.fMedianRunningAPITasks                = stats.fRecentAPI.fMedianRunningAPITasks;
        return r;
    }();
    APIServerInfo::WebServer webServerStats = [&] () { return fRep_->fWebServerStatsFetcher (); }();
    Database                 dbStats        = [&] () {
        Database r;
        r.fReads               = stats.fRecentDB.fReads;
        r.fWrites              = stats.fRecentDB.fWrites;
        r.fErrors              = stats.fRecentDB.fErrors;
        r.fMeanReadDuration    = stats.fRecentDB.fMeanReadDuration;
        r.fMedianReadDuration  = stats.fRecentDB.fMedianReadDuration;
        r.fMeanWriteDuration   = stats.fRecentDB.fMeanWriteDuration;
        r.fMedianWriteDuration = stats.fRecentDB.fMedianWriteDuration;
        r.fMaxDuration         = stats.fRecentDB.fMaxDuration;
        r.fFileSize            = WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::DB::pFileSize ();
        return r;
    }();

    return About{AppVersion::kVersion,
                 APIServerInfo{AppVersion::kVersion, kAPIServerComponents_, machineInfo, processInfo, apiStats, webServerStats, dbStats}};
}

tuple<Memory::BLOB, optional<DataExchange::InternetMediaType>> WSImpl::GetBLOB (const GUID& guid) const
{
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;
    return Common::BLOBMgr::sThe.GetBLOB (guid);
}

Sequence<String> WSImpl::GetDevices (const optional<Set<GUID>>& ids, const optional<DeviceSortParameters>& sort) const
{
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;
    return GetDevices_Recurse (ids, sort).Map<Sequence<String>> ([] (const WebServices::Device& n) { return n.fID.As<String> (); });
}

Sequence<BackendApp::WebServices::Device> WSImpl::GetDevices_Recurse (const optional<Set<GUID>>& ids, const optional<DeviceSortParameters>& sort) const
{
    using BackendApp::WebServices::Device;
    Debug::TraceContextBumper                       ctx{"WSImpl::GetDevices_Recurse", "sort={}"_f, sort};
    Debug::TimingTrace                              ttrc{L"WSImpl::GetDevices_Recurse", 100ms};
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;

    // Compute effective sort Search Terms - filling in optional values
    Sequence<DeviceSortParameters::SearchTerm> searchTerms;
    {
        if (sort) {
            searchTerms = sort->fSearchTerms;
        }
        if (searchTerms.empty ()) {
            searchTerms += DeviceSortParameters::SearchTerm{DeviceSortParameters::SearchTerm::By::ePriority, false};
        }
        for (auto i = searchTerms.begin (); i != searchTerms.end (); ++i) {
            if (not i->fAscending.has_value ()) {
                auto p = *i;
                switch (p.fBy) {
                    case DeviceSortParameters::SearchTerm::By::ePriority:
                        p.fAscending = false;
                        break;
                    default:
                        p.fAscending = true;
                        break;
                }
                searchTerms.Update (i, p);
            }
        }
    }

    // Compute effective sortCompareNetwork - as a set of CIDRs
    optional<Set<CIDR>> sortCompareNetwork;
    if (sort and sort->fCompareNetwork) {
        // CIDR will contain a / and GUID won't so use that to distinguish
        if (sort->fCompareNetwork->Contains (L"/"_k)) {
            sortCompareNetwork = Set<CIDR>{CIDR{*sort->fCompareNetwork}}; // OK to throw if invalid
        }
        else {
            if (auto n = IntegratedModel::Mgr::sThe.GetNetwork (*sort->fCompareNetwork)) {
                sortCompareNetwork = n->fNetworkAddresses;
            }
        }
    }

    Sequence<Device> devices;
    if (ids) {
        for (auto i : *ids) {
            if (auto d = IntegratedModel::Mgr::sThe.GetDevice (i)) {
                devices += *d;
            }
        }
    }
    else {
        devices = IntegratedModel::Mgr::sThe.GetDevices ();
    }

    // Sort them
    for (const DeviceSortParameters::SearchTerm& st : searchTerms) {
        switch (st.fBy) {
            case DeviceSortParameters::SearchTerm::By::ePriority: {
                devices = devices.OrderBy ([st] (const BackendApp::WebServices::Device& lhs, const BackendApp::WebServices::Device& rhs) -> bool {
                    // super primitive sort strategy...
                    auto priFun = [] (const BackendApp::WebServices::Device& d) {
                        int pri = 0;
                        if (d.fTypes and d.fTypes->Contains (Device::DeviceType::ePC)) {
                            pri += 20;
                        }
                        if (d.fTypes and d.fTypes->Contains (Device::DeviceType::eRouter)) {
                            pri += 10;
                        }
                        if (d.fTypes) {
                            pri += 2 * static_cast<int> (d.fTypes->length ());
                        }
                        if (d.fOperatingSystem) {
                            pri += 2;
                        }
                        if (d.fOpenPorts) {
                            pri += static_cast<int> (d.fOpenPorts->length ());
                        }
                        return pri;
                    };
                    int lPri = priFun (lhs);
                    int rPri = priFun (rhs);
                    Assert (st.fAscending);
                    bool ascending = *st.fAscending;
                    return ascending ? (lPri < rPri) : (lPri > rPri);
                });
                break;
                case DeviceSortParameters::SearchTerm::By::eAddress: {
                    devices = devices.OrderBy ([st, sortCompareNetwork] (const BackendApp::WebServices::Device& lhs,
                                                                         const BackendApp::WebServices::Device& rhs) -> bool {
                        Assert (st.fAscending);
                        bool ascending = *st.fAscending;
                        auto lookup    = [=] (const BackendApp::WebServices::Device& d) -> InternetAddress {
                            if (sortCompareNetwork) {
                                // if multiple, grab the first (somewhat arbitrary) - maybe should grab the least?
                                for (const InternetAddress& ia : d.GetInternetAddresses ()) {
                                    if (sortCompareNetwork->Any ([&] (const CIDR& cidr) { return cidr.GetRange ().Contains (ia); })) {
                                        return ia;
                                    }
                                }
                                return ascending ? InternetAddress::max () : InternetAddress::min (); // not matching always show up at end of list
                            }
                            else {
                                // @todo - consider which address to use? maybe least if ascending, and max if decesnding?
                                return d.GetInternetAddresses ().NthValue (0);
                            }
                        };
                        InternetAddress l = lookup (lhs);
                        InternetAddress r = lookup (rhs);
                        return ascending ? (l < r) : (l > r);
                    });
                } break;
                case DeviceSortParameters::SearchTerm::By::eName: {
                    devices = devices.OrderBy ([st, sortCompareNetwork] (const BackendApp::WebServices::Device& lhs,
                                                                         const BackendApp::WebServices::Device& rhs) -> bool {
                        Assert (st.fAscending);
                        bool ascending = *st.fAscending;
                        return ascending ? (lhs.fNames.GetName () < rhs.fNames.GetName ()) : (lhs.fNames.GetName () > rhs.fNames.GetName ());
                    });
                } break;
                case DeviceSortParameters::SearchTerm::By::eType: {
                    devices = devices.OrderBy ([st, sortCompareNetwork] (const BackendApp::WebServices::Device& lhs,
                                                                         const BackendApp::WebServices::Device& rhs) -> bool {
                        Assert (st.fAscending);
                        bool ascending = *st.fAscending;
                        // tricky to compare types cuz we have a set of types. And types in those sets have subtypes
                        // If TS is a subtype (more specific type) from T, then treat TS > T
                        auto mapTypeToOrder = [] (Device::DeviceType dt) -> double {
                            switch (dt) {
                                case Device::DeviceType::ePC:
                                    return 1;
                                case Device::DeviceType::eNetworkInfrastructure:
                                    return 2;
                                case Device::DeviceType::eRouter:
                                    return 2.1;
                                case Device::DeviceType::eTablet:
                                    return 3;
                                case Device::DeviceType::ePhone:
                                    return 4;
                                case Device::DeviceType::eMediaPlayer:
                                    return 5;
                                case Device::DeviceType::eSpeaker:
                                    return 5.1;
                                case Device::DeviceType::eTV:
                                    return 5.2;
                                default:
                                    return 99;
                            }
                        };
                        auto mapTypeToOrder2 = [&] (const optional<Set<Device::DeviceType>> dt) -> double {
                            double f = 99;
                            if (dt) {
                                for (const auto& d : *dt) {
                                    f = min (f, mapTypeToOrder (d));
                                }
                            }
                            return f;
                        };
                        return ascending ? (mapTypeToOrder2 (lhs.fTypes) < mapTypeToOrder2 (rhs.fTypes))
                                         : (mapTypeToOrder2 (lhs.fTypes) > mapTypeToOrder2 (rhs.fTypes));
                    });
                } break;
                default: {
                    Execution::Throw (ClientErrorException{"missing or invalid By in search specification"_k});
                } break;
            }
        }
    }
    return devices;
}

tuple<Device, Duration> WSImpl::GetDevice (const String& id) const
{
    Debug::TimingTrace                              ttrc{L"WSImpl::GetDevice", 100ms};
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;
    GUID               compareWithID = ClientErrorException::TreatExceptionsAsClientError ([&] () { return GUID{id}; });
    optional<Duration> ttl;
    if (auto d = IntegratedModel::Mgr::sThe.GetDevice (compareWithID, &ttl)) {
        return make_tuple (*d, Memory::ValueOf (ttl));
    }
    Execution::Throw (ClientErrorException{"no such id"sv});
}

void WSImpl::PatchDevice (const String& id, const DataExchange::JSON::Patch::OperationItemsType& patchDoc) const
{
    Debug::TraceContextBumper ctx{"WSImpl::PatchDevice", "id={}, patchDoc={}"_f, id, patchDoc};
    GUID                      objID = ClientErrorException::TreatExceptionsAsClientError ([&] () { return GUID{id}; });
    for (auto op : patchDoc) {
        switch (op.op) {
            case DataExchange::JSON::Patch::OperationType::eAdd: {
                Device::UserOverridesType updateVal =
                    IntegratedModel::Mgr::sThe.GetDeviceUserSettings (objID).value_or (Device::UserOverridesType{});
                if (not op.value) {
                    Execution::Throw (ClientErrorException{"JSON-Patch add requires a value"_k});
                }
                if (op.path == "/userOverrides/name"sv) {
                    updateVal.fName = op.value->As<String> ();
                }
                else if (op.path == "/userOverrides/notes"sv) {
                    updateVal.fNotes = op.value->As<String> ();
                }
                else if (op.path == "/userOverrides/tags"sv) {
                    // for now only support replacing the whole array at a time
                    updateVal.fTags =
                        op.value->As<Sequence<VariantValue>> ().Map<Set<String>> ([] (const VariantValue& vv) { return vv.As<String> (); });
                }
                else {
                    Execution::Throw (ClientErrorException{"JSON-Patch add of unsupported op.path"_k});
                }
                if (updateVal.IsNonTrivial ()) {
                    IntegratedModel::Mgr::sThe.SetDeviceUserSettings (objID, updateVal);
                }
                else {
                    IntegratedModel::Mgr::sThe.SetDeviceUserSettings (objID, nullopt);
                }
            } break;
            case DataExchange::JSON::Patch::OperationType::eRemove: {
                Device::UserOverridesType updateVal =
                    IntegratedModel::Mgr::sThe.GetDeviceUserSettings (objID).value_or (Device::UserOverridesType{});
                if (op.path == "/userOverrides/name") {
                    updateVal.fName = optional<String>{};
                }
                else if (op.path == "/userOverrides/notes") {
                    updateVal.fNotes = optional<String>{};
                }
                else if (op.path == L"/userOverrides/tags") {
                    // for now only support replacing the whole array at a time
                    updateVal.fTags = optional<Set<String>>{};
                }
                else {
                    Execution::Throw (ClientErrorException{"JSON-Patch remove of unsupported op.path"_k});
                }
                if (updateVal.IsNonTrivial ()) {
                    IntegratedModel::Mgr::sThe.SetDeviceUserSettings (objID, updateVal);
                }
                else {
                    IntegratedModel::Mgr::sThe.SetDeviceUserSettings (objID, nullopt);
                }
            } break;
        }
    }
}

Sequence<String> WSImpl::GetNetworks (const optional<Set<GUID>>& ids) const
{
    Debug::TimingTrace                              ttrc{L"WSImpl::GetNetworks", 100ms};
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;
    if (ids) {
        Sequence<String> result;
        for (auto i : *ids) {
            if (auto n = IntegratedModel::Mgr::sThe.GetNetwork (i)) {
                result += n->fID.ToString ();
            }
            else {
                // should drop on floor or throw? - but if throw whats the point of thie API taking guids and returning guids? Maybe none
            }
        }
        return result;
    }
    else {
        return IntegratedModel::Mgr::sThe.GetNetworks ().Map<Sequence<String>> ([] (const auto& n) { return n.fID.ToString (); });
    }
}

Sequence<BackendApp::WebServices::Network> WSImpl::GetNetworks_Recurse (const optional<Set<GUID>>& ids) const
{
    Debug::TimingTrace                              ttrc{L"WSImpl::GetNetworks_Recurse", 100ms};
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;
    if (ids) {
        Sequence<BackendApp::WebServices::Network> result;
        for (auto i : *ids) {
            if (auto d = IntegratedModel::Mgr::sThe.GetNetwork (i)) {
                result += *d;
            }
            else {
                // should drop on floor or throw?
            }
        }
        return result;
    }
    else {
        return IntegratedModel::Mgr::sThe.GetNetworks ();
    }
}

tuple<Network, Duration> WSImpl::GetNetwork (const String& id) const
{
    Debug::TimingTrace                              ttrc{L"WSImpl::GetNetwork", 100ms};
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;
    GUID               compareWithID = ClientErrorException::TreatExceptionsAsClientError ([&] () { return GUID{id}; });
    optional<Duration> ttl;
    if (auto d = IntegratedModel::Mgr::sThe.GetNetwork (compareWithID, &ttl)) {
        return make_tuple (*d, Memory::ValueOf (ttl));
    }
    Execution::Throw (ClientErrorException{"no such id"sv});
}

void WSImpl::PatchNetwork (const String& id, const DataExchange::JSON::Patch::OperationItemsType& patchDoc) const
{
    Debug::TraceContextBumper ctx{"WSImpl::PatchNetwork", "id={}, patchDoc={}"_f, id, patchDoc};
    GUID                      objID = ClientErrorException::TreatExceptionsAsClientError ([&] () { return GUID{id}; });
    for (auto op : patchDoc) {
        switch (op.op) {
            case DataExchange::JSON::Patch::OperationType::eAdd: {
                Network::UserOverridesType updateVal =
                    IntegratedModel::Mgr::sThe.GetNetworkUserSettings (objID).value_or (Network::UserOverridesType{});
                if (not op.value) {
                    Execution::Throw (ClientErrorException{"JSON-Patch add requires a value"_k});
                }
                if (op.path == "/userOverrides/name"sv) {
                    updateVal.fName = op.value->As<String> ();
                }
                else if (op.path == "/userOverrides/notes"sv) {
                    updateVal.fNotes = op.value->As<String> ();
                }
                else if (op.path == "/userOverrides/tags"sv) {
                    // for now only support replacing the whole array at a time
                    updateVal.fTags =
                        op.value->As<Sequence<VariantValue>> ().Map<Set<String>> ([] (const VariantValue& vv) { return vv.As<String> (); });
                }
                else if (op.path == "/userOverrides/aggregateFingerprints"sv) {
                    // for now only support replacing the whole array at a time
                    updateVal.fAggregateFingerprints =
                        op.value->As<Sequence<VariantValue>> ().Map<Set<GUID>> ([] (const VariantValue& vv) { return vv.As<String> (); });
                }
                else if (op.path == "/userOverrides/aggregateGatewayHardwareAddresses"sv) {
                    // for now only support replacing the whole array at a time
                    updateVal.fAggregateGatewayHardwareAddresses =
                        op.value->As<Sequence<VariantValue>> ().Map<Set<String>> ([] (const VariantValue& vv) { return vv.As<String> (); });
                }
                else if (op.path == "/userOverrides/aggregateNetworkInterfacesMatching"sv) {
                    // for now only support replacing the whole array at a time
                    updateVal.fAggregateNetworkInterfacesMatching =
                        Model::Network::UserOverridesType::kMapper.ToObject<Sequence<Model::Network::UserOverridesType::NetworkInterfaceAggregateRule>> (
                            *op.value);
                }
                else {
                    Execution::Throw (ClientErrorException{"JSON-Patch add of unsupported op.path"_k});
                }
                if (updateVal.IsNonTrivial ()) {
                    IntegratedModel::Mgr::sThe.SetNetworkUserSettings (objID, updateVal);
                }
                else {
                    IntegratedModel::Mgr::sThe.SetNetworkUserSettings (objID, nullopt);
                }
            } break;
            case DataExchange::JSON::Patch::OperationType::eRemove: {
                Network::UserOverridesType updateVal =
                    IntegratedModel::Mgr::sThe.GetNetworkUserSettings (objID).value_or (Network::UserOverridesType{});
                if (op.path == "/userOverrides/name"sv) {
                    updateVal.fName = optional<String>{};
                }
                else if (op.path == "/userOverrides/notes"sv) {
                    updateVal.fNotes = optional<String>{};
                }
                else if (op.path == "/userOverrides/tags"sv) {
                    // for now only support replacing the whole array at a time
                    updateVal.fTags = optional<Set<String>>{};
                }
                else if (op.path == "/userOverrides/aggregateFingerprints"sv) {
                    updateVal.fAggregateFingerprints = nullopt;
                }
                else if (op.path == "/userOverrides/aggregateGatewayHardwareAddresses"sv) {
                    updateVal.fAggregateGatewayHardwareAddresses = nullopt;
                }
                else if (op.path == "/userOverrides/aggregateNetworkInterfacesMatching"sv) {
                    updateVal.fAggregateNetworkInterfacesMatching = nullopt;
                }
                else {
                    Execution::Throw (ClientErrorException{"JSON-Patch remove of unsupported op.path"_k});
                }
                if (updateVal.IsNonTrivial ()) {
                    IntegratedModel::Mgr::sThe.SetNetworkUserSettings (objID, updateVal);
                }
                else {
                    IntegratedModel::Mgr::sThe.SetNetworkUserSettings (objID, nullopt);
                }
            } break;
        }
    }
}

Collection<String> WSImpl::GetNetworkInterfaces () const
{
    Debug::TimingTrace                              ttrc{L"WSImpl::GetNetworkInterfaces_Recurse", 100ms};
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;
    return IntegratedModel::Mgr::sThe.GetNetworkInterfaces ().Map<Collection<String>> (
        [=] (const auto& ni) -> optional<String> { return ni.fID.ToString (); });
}

Collection<BackendApp::WebServices::NetworkInterface> WSImpl::GetNetworkInterfaces_Recurse () const
{
    Debug::TimingTrace                              ttrc{L"WSImpl::GetNetworkInterfaces_Recurse", 100ms};
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;
    return IntegratedModel::Mgr::sThe.GetNetworkInterfaces ();
}

tuple<NetworkInterface, Duration> WSImpl::GetNetworkInterface (const String& id) const
{
    Debug::TimingTrace                              ttrc{L"WSImpl::GetNetworkInterface", 100ms};
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;
    GUID               compareWithID = ClientErrorException::TreatExceptionsAsClientError ([&] () { return GUID{id}; });
    optional<Duration> ttl;
    if (auto ni = IntegratedModel::Mgr::sThe.GetNetworkInterface (compareWithID, &ttl)) {
        return make_tuple (*ni, Memory::ValueOf (ttl));
    }
    Execution::Throw (ClientErrorException{"no such id"sv});
}

double WSImpl::Operation_Ping (const String& address) const
{
    Debug::TraceContextBumper                       ctx{"WSImpl::Operation_Ping", "address={}"_f, address};
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;

    using namespace Stroika::Foundation::IO::Network;
    using namespace Stroika::Foundation::IO::Network::InternetProtocol;
    using namespace Stroika::Foundation::Time;
    using namespace Stroika::Frameworks;
    using namespace Stroika::Frameworks::NetworkMonitor;

    size_t packetSize = Ping::Options::kDefaultPayloadSize + sizeof (ICMP::V4::PacketHeader); // historically, the app ping has measured this including ICMP packet header, but not ip packet header size
    unsigned int          maxHops     = Ping::Options::kDefaultMaxHops;
    unsigned int          sampleCount = 3;
    static const Duration kInterSampleTime_{"PT.1S"};

    Ping::Options options{};
    options.fPacketPayloadSize = Ping::Options::kAllowedICMPPayloadSizeRange.Pin (packetSize - sizeof (ICMP::V4::PacketHeader));
    options.fMaxHops           = maxHops;
    //   options.fSampleInfo                   = Ping::Options::SampleInfo{kInterSampleTime_, sampleCount};

    // write GetHostAddress () function in DNS that throws if not at least one
    auto addrs = DNS::kThe.GetHostAddresses (address, InternetAddress::AddressFamily::V4);
    if (addrs.size () < 1) {
        Execution::Throw (Execution::Exception{L"no addr"sv});
    }

    NetworkMonitor::Ping::SampleResults t = NetworkMonitor::Ping::Sample (addrs[0], Ping::SampleOptions{kInterSampleTime_, sampleCount}, options);
    if (t.fMedianPingTime) {
        return t.fMedianPingTime->As<double> ();
    }

    return numeric_limits<double>::infinity ();
}

Operations::TraceRouteResults WSImpl::Operation_TraceRoute (const String& address, optional<bool> reverseDNSResults) const
{
    Debug::TraceContextBumper ctx{"WSImpl::Operation_TraceRoute", "address={}, reverseDNSResults={}"_f, address, reverseDNSResults};
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;

    using namespace Stroika::Foundation::IO::Network;
    using namespace Stroika::Foundation::IO::Network::InternetProtocol;
    using namespace Stroika::Foundation::Time;
    using namespace Stroika::Frameworks;
    using namespace Stroika::Frameworks::NetworkMonitor;

    bool revDNS = reverseDNSResults.value_or (true);

    size_t packetSize = Ping::Options::kDefaultPayloadSize + sizeof (ICMP::V4::PacketHeader); // historically, the app ping has measured this including ICMP packet header, but not ip packet header size
    unsigned int          maxHops     = Ping::Options::kDefaultMaxHops;
    unsigned int          sampleCount = 3;
    static const Duration kInterSampleTime_{"PT.1S"};

    Traceroute::Options options{};
    options.fPacketPayloadSize = Ping::Options::kAllowedICMPPayloadSizeRange.Pin (packetSize - sizeof (ICMP::V4::PacketHeader));
    options.fMaxHops           = maxHops;

    options.fTimeout = Duration{5.0};
    //   options.fSampleInfo                   = Ping::Options::SampleInfo{kInterSampleTime_, sampleCount};

    // write GetHostAddress () function in DNS that throws if not at least one

    Model::Operations::TraceRouteResults results;

    Sequence<Traceroute::Hop> hops = Traceroute::Run (DNS::kThe.GetHostAddress (address), options);
    // unsigned int              hopIdx{1};
    for (Traceroute::Hop h : hops) {
        String hopName = [=] () {
            String addrStr = h.fAddress.As<String> ();
            if (revDNS) {
                if (auto rdnsName = DNS::kThe.QuietReverseLookup (h.fAddress)) {
                    return *rdnsName;
                }
            }
            return addrStr;
        }();
        results.fHops += Operations::TraceRouteResults::Hop{h.fTime, hopName};
        // cout << hopIdx++ << "\t" << h.fTime.PrettyPrint ().AsNarrowSDKString () << "\t" << hopName.AsNarrowSDKString () << endl;
    }

    return results;
}

Time::Duration WSImpl::Operation_DNS_CalculateNegativeLookupTime (optional<unsigned int> samples) const
{
    Debug::TraceContextBumper ctx{Stroika_Foundation_Debug_OptionalizeTraceArgs (L"WSImpl::Operation_DNS_CalculateNegativeLookupTime")};
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;
    constexpr unsigned int                          kDefault_Samples = 7;
    unsigned int                                    useSamples       = samples.value_or (kDefault_Samples);
    if (useSamples == 0) {
        Execution::Throw (ClientErrorException{"samples must be > 0"sv});
    }
    uniform_int_distribution<mt19937::result_type> allUInt16Distribution{0, numeric_limits<uint32_t>::max ()};
    static mt19937                                 sRng_{std::random_device{}()};
    Sequence<Time::DurationSeconds>                measurements;
    for (unsigned int i = 0; i < useSamples; ++i) {
        String                 randomAddress = Characters::Format ("www.xxxabc{}.com"_f, allUInt16Distribution (sRng_));
        Time::TimePointSeconds startAt       = Time::GetTickCount ();
        IgnoreExceptionsForCall (IO::Network::DNS::kThe.GetHostAddress (randomAddress));
        measurements += Time::GetTickCount () - startAt;
    }
    Assert (measurements.Median ().has_value ());
    return Time::Duration{*measurements.Median ()};
}

Operations::DNSLookupResults WSImpl::Operation_DNS_Lookup (const String& name) const
{
    Debug::TraceContextBumper                       ctx{"WSImpl::Operation_DNS_Lookup", "name={}"_f, name};
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;
    Operations::DNSLookupResults                    result;
    Time::TimePointSeconds                          startAt = Time::GetTickCount ();
    IgnoreExceptionsForCall (result.fResult = Characters::ToString (IO::Network::DNS::kThe.GetHostAddress (name)));
    result.fLookupTime = Time::Duration{Time::GetTickCount () - startAt};
    return result;
}

double WSImpl::Operation_DNS_CalculateScore () const
{
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;
    // decent estimate of score is (weighted) sum of these numbers - capped at some maximum, and then 1-log of that number (log to skew so mostly sensative to small differences around small numbers and big is big, and 1- to get 1 better score than zero)
    double           totalWeightedTime{};
    constexpr double kNegLookupWeight = 2.5;
    totalWeightedTime += kNegLookupWeight * Operation_DNS_CalculateNegativeLookupTime ({}).As<double> ();
    constexpr double kPosLookupWeight = 25; // much higher than kNegLookupWeight because this is the time for cached entries lookup which will naturally be much smaller
    totalWeightedTime += kPosLookupWeight * (0 + Operation_DNS_Lookup (L"www.google.com"sv).fLookupTime.As<double> () +
                                             Operation_DNS_Lookup (L"www.amazon.com"sv).fLookupTime.As<double> () +
                                             Operation_DNS_Lookup (L"www.youtube.com"sv).fLookupTime.As<double> ());
    Assert (totalWeightedTime >= 0);
    constexpr double kScoreCutOff_               = 10.0;
    constexpr double kShiftAndScaleVerticallyBy_ = 10;
    double           score{(kShiftAndScaleVerticallyBy_ - log (totalWeightedTime / (kScoreCutOff_ / 10))) / kShiftAndScaleVerticallyBy_};

    //DbgTrace (L"totalWeightedTime=%f", totalWeightedTime);
    //DbgTrace (L"log=%f", log (totalWeightedTime / (kScoreCutOff_ / 10)));
    //DbgTrace (L"score=%f", score);

    score = Math::PinInRange<double> (score, 0, 1);
    Ensure (0 <= score and score <= 1.0);
    return score;
}

DataExchange::VariantValue WSImpl::Operation_Scan_FullRescan (const String& deviceID) const
{
    Debug::TraceContextBumper                       ctx{"WSImpl::Operation_Scan_FullRescan"};
    DataExchange::VariantValue                      x;
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;
    GUID useDeviceID = ClientErrorException::TreatExceptionsAsClientError ([&] () { return GUID{deviceID}; });
    // @todo if the device has no dynamic device (cuz it hasn't been discovered - yet) - we don't force an attempt to rediscover
    // because Discovery::DevicesMgr doesn't have API for this. Maybe add one --LGP 2022-06-22
    if (auto useDevID = IntegratedModel::Mgr::sThe.GetCorrespondingDynamicDeviceID (useDeviceID)) {
        Discovery::DevicesMgr::sThe.ReScan (*useDevID);
    }
    return x;
}

DataExchange::VariantValue WSImpl::Operation_Scan_Scan (const String& addr) const
{
    Debug::TraceContextBumper                       ctx{"WSImpl::Operation_Scan_Scan"};
    Common::OperationalStatisticsMgr::ProcessAPICmd statsGather;
    InternetAddress                                 useAddr =
        ClientErrorException::TreatExceptionsAsClientError ([&] () { return IO::Network::DNS::kThe.GetHostAddress (addr); });
    return Discovery::DevicesMgr::sThe.ScanAndReturnReport (useAddr);
}
