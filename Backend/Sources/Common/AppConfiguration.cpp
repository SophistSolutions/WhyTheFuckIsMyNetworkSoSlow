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
#if qPlatform_POSIX
                {"ToSysLog"sv, &AppConfigurationType::Logging::ToSysLog},
#endif
#if qPlatform_Windows
                {"ToWindowsEventLog"sv, &AppConfigurationType::Logging::ToWindowsEventLog},
#endif
        },
        {.fOmitNullEntriesInFromObject = false});
    mapper.AddCommonType<optional<AppConfigurationType::Logging>> ();

    mapper.AddClass<AppConfigurationType> ({{"Logging"sv, &AppConfigurationType::fLogging}, {"WebServerPort"sv, &AppConfigurationType::WebServerPort}},
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
