/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Common/Property.h"
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/Transfer/Connection.h"

#include "AppConfiguration.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::DataExchange;
using namespace Stroika::Foundation::Execution;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common;

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

using WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::AppConfigurationType;
using WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common::Private_::AppConfiguration_Storage_IMPL_;

/*
 ********************************************************************************
 *************************** AppConfigurationType *******************************
 ********************************************************************************
 */
const ObjectVariantMapper AppConfigurationType::kMapper = [] () {
    // note - set fOmitNullEntriesInFromObject=false so easier review config file and see where values are defaulted/defaulting
    ObjectVariantMapper mapper;
    mapper.AddCommonType<optional<IO::Network::PortType>> ();
    mapper.AddClass<AppConfigurationType::Logging> (
        {
            {"ToStdOut"sv, &AppConfigurationType::Logging::ToStdOut},
#if qStroika_HasComponent_syslog
            {"ToSysLog"sv, &AppConfigurationType::Logging::ToSysLog},
#endif
#if qStroika_Foundation_Common_Platform_Windows
            {"ToWindowsEventLog"sv, &AppConfigurationType::Logging::ToWindowsEventLog},
#endif
        },
        {.fOmitNullEntriesInFromObject = false});
    mapper.AddCommonType<optional<AppConfigurationType::Logging>> ();

    mapper.AddClass<SingleFileJSONStorage> ({{"File"sv, &SingleFileJSONStorage::fFile}}, {.fOmitNullEntriesInFromObject = false});
    mapper.AddClass<DirectoryJSONStorage> ({{"Root"sv, &DirectoryJSONStorage::fRoot}}, {.fOmitNullEntriesInFromObject = false});
    mapper.AddClass<SQLiteStorage> ({{"File"sv, &SQLiteStorage::fFile}}, {.fOmitNullEntriesInFromObject = false});

    // Treat a VARIANT as a Mapping (regular object) - but with only one of the values possible
    mapper.Add<DatabaseConfigurationType> (
        [] (const ObjectVariantMapper& mapper, const DatabaseConfigurationType* obj) -> VariantValue {
            if (auto sfj = get_if<SingleFileJSONStorage> (obj)) {
                Mapping<String, VariantValue> t;
                t.Add ("SingleFileJSONStorage"sv, mapper.FromObject (*sfj));
                return VariantValue{t};
            }
            else if (auto dfj = get_if<DirectoryJSONStorage> (obj)) {
                Mapping<String, VariantValue> t;
                t.Add ("DirectoryJSONStorage"sv, mapper.FromObject (*dfj));
                return VariantValue{t};
            }
            else if (auto msql = get_if<SQLiteStorage> (obj)) {
                Mapping<String, VariantValue> t;
                t.Add ("SQLiteStorage"sv, mapper.FromObject (*msql));
                return VariantValue{t};
            }
            return VariantValue{}; // monostate => empty/missing data
        },
        [] (const ObjectVariantMapper& mapper, const VariantValue& d, DatabaseConfigurationType* intoObj) -> void {
            Mapping<String, VariantValue> vv = d.As<Mapping<String, VariantValue>> ();
            if (auto sfj = vv.Lookup ("SingleFileJSONStorage"sv)) {
                *intoObj = mapper.ToObject<SingleFileJSONStorage> (*sfj);
            }
            else if (auto dfj = vv.Lookup ("DirectoryJSONStorage"sv)) {
                *intoObj = mapper.ToObject<DirectoryJSONStorage> (*dfj);
            }
            else if (auto msql = vv.Lookup ("SQLiteStorage"sv)) {
                *intoObj = mapper.ToObject<SQLiteStorage> (*msql);
            }
            else {
                *intoObj = DatabaseConfigurationType{}; // monostate
            }
        });

    mapper.AddClass<AppConfigurationType::BackupData> (
        {
            {"File"sv, &AppConfigurationType::BackupData::fFile},
        },
        {.fOmitNullEntriesInFromObject = false});
    mapper.AddCommonType<optional<AppConfigurationType::BackupData>> ();

    mapper.AddClass<AppConfigurationType> ({{"Logging"sv, &AppConfigurationType::fLogging},
                                            {"WebServerPort"sv, &AppConfigurationType::WebServerPort},
                                            {"Database"sv, &AppConfigurationType::fDatabase},
                                            {"BackupData"sv, &AppConfigurationType::fBackupData}},
                                           {.fOmitNullEntriesInFromObject = false});

    return mapper;
}();

/*
 ********************************************************************************
 ******************** Private_::AppConfiguration_Storage_IMPL_ ******************
 ********************************************************************************
 */
AppConfiguration_Storage_IMPL_::AppConfiguration_Storage_IMPL_ ()
    : fOptionsFile_{"AppSettings"sv, AppConfigurationType::kMapper, OptionsFile::kDefaultUpgrader,
                    OptionsFile::mkFilenameMapper ("WhyTheFuckIsMyNetworkSoSlow"sv)}
    , fActualCurrentConfigData_{fOptionsFile_.Read<AppConfigurationType> (AppConfigurationType{})}
{
    Set (fActualCurrentConfigData_); // assure derived data (and changed fields etc) up to date
}

AppConfigurationType AppConfiguration_Storage_IMPL_::Get () const
{
    return fActualCurrentConfigData_;
}

void AppConfiguration_Storage_IMPL_::Set (const AppConfigurationType& v)
{
    fActualCurrentConfigData_ = v;
    fOptionsFile_.Write (v);
}
