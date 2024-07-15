/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_AppConfiguration_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_AppConfiguration_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/DataExchange/OptionsFile.h"
#include "Stroika/Foundation/Execution/ModuleGetterSetter.h"
#include "Stroika/Foundation/IO/Network/Port.h"

/**
 *
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common {

    using namespace std;
    using namespace Stroika::Foundation;

    /**
     *  This is the type of object stored in configuration.
     */
    struct AppConfigurationType {
        optional<IO::Network::PortType>        WebServerPort;
        static constexpr IO::Network::PortType kWebServerPort_Default = 80;

        struct Logging {
            optional<bool>        ToStdOut;
            static constexpr bool kToStdOut_Default = false;
#if qPlatform_POSIX
            optional<bool>        ToSysLog;
            static constexpr bool kToSysLog_Default = true;
#endif
#if qPlatform_Windows
            optional<bool>        ToWindowsEventLog;
            static constexpr bool kToWindowsEventLog_Default = true;
#endif
        };
        optional<Logging> fLogging;

        static const DataExchange::ObjectVariantMapper kMapper;
    };

    namespace Private_ {
        struct AppConfiguration_Storage_IMPL_ {
            AppConfiguration_Storage_IMPL_ ();
            AppConfigurationType      Get () const;
            void                      Set (const AppConfigurationType& v);
            DataExchange::OptionsFile fOptionsFile_;
            AppConfigurationType      fActualCurrentConfigData_;
        };
    }
    /**
     *  gAppConfiguration is automatically internally synchronized ... - just call update / set / get to access options freely.
     */
    inline Execution::ModuleGetterSetter<AppConfigurationType, Private_::AppConfiguration_Storage_IMPL_> gAppConfiguration;

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "AppConfiguration.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_AppConfiguration_h_*/
