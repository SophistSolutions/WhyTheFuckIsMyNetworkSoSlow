/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include <iostream>

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Debug/BackTrace.h"
#include "Stroika/Foundation/Debug/Fatal.h"
#include "Stroika/Foundation/Execution/CommandLine.h"
#include "Stroika/Foundation/Execution/IntervalTimer.h"
#include "Stroika/Foundation/Execution/Module.h"
#include "Stroika/Foundation/Execution/SignalHandlers.h"
#include "Stroika/Foundation/Execution/Thread.h"
#include "Stroika/Foundation/Execution/Users.h"
#include "Stroika/Foundation/Execution/WaitableEvent.h"
#if qPlatform_Windows
#include "Stroika/Foundation/Execution/Platform/Windows/Exception.h"
#include "Stroika/Foundation/Execution/Platform/Windows/StructuredException.h"
#endif
#include "Stroika/Foundation/IO/FileSystem/FileOutputStream.h"
#include "Stroika/Foundation/IO/Network/SystemFirewall.h"

#include "Common/AppConfiguration.h"

#include "AppVersion.h"

#include "Service.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Execution;

using Execution::Logger;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;

namespace {
    void FatalErorrHandler_ (const Characters::SDKChar* msg) noexcept
    {
        Thread::SuppressInterruptionInContext suppressCtx;
        DbgTrace ("Fatal Error {} encountered"_f, String::FromSDKString (msg));
        Logger::sThe.Log (Logger::eCriticalError, "Fatal Error: {}; Aborting..."_f, String::FromSDKString (msg));
        Logger::sThe.Log (Logger::eCriticalError, "Backtrace: {}"_f, Debug::BackTrace::Capture ());
        if (std::exception_ptr exc = std::current_exception ()) {
            Logger::sThe.Log (Logger::eCriticalError, "Uncaught exception: {}"_f, exc);
        }
        Logger::sThe.Flush ();
        std::_Exit (EXIT_FAILURE); // skip
    }
    void FatalSignalHandler_ (Execution::SignalID signal) noexcept
    {
        Thread::SuppressInterruptionInContext suppressCtx;
        DbgTrace ("Fatal Signal encountered: {}"_f, Execution::SignalToName (signal));
        Logger::sThe.Log (Logger::eCriticalError, "Fatal Signal: {}; Aborting..."_f, Execution::SignalToName (signal));
        Logger::sThe.Log (Logger::eCriticalError, "Backtrace: {}"_f, Debug::BackTrace::Capture ());
        Logger::sThe.Flush ();
        std::_Exit (EXIT_FAILURE); // skip
    }
}

namespace {
    void ShowUsage_ (const Main& m, const Execution::InvalidCommandLineArgument& e = {})
    {
        if (not e.fMessage.empty ()) {
            cerr << "Error: " << e.fMessage.AsNarrowSDKString () << endl;
            cerr << endl;
        }
        cerr << "Usage: " << m.GetServiceDescription ().fRegistrationName.AsNarrowSDKString () << " [options] where options can be :\n ";
        if (m.GetServiceIntegrationFeatures ().Contains (Main::ServiceIntegrationFeatures::eInstall)) {
            cerr << "\t--" << String{Main::CommandNames::kInstall}.AsNarrowSDKString ()
                 << "               /* Install service (only when debugging - should use real installer like WIX) */" << endl;
            cerr << "\t--" << String{Main::CommandNames::kUnInstall}.AsNarrowSDKString ()
                 << "             /* UnInstall service (only when debugging - should use real installer like WIX) */" << endl;
        }
        cerr << "\t--" << String{Main::CommandNames::kRunAsService}.AsNarrowSDKString ()
             << "        /* Run this process as a service (doesn't exit until the serivce is done ...) */" << endl;
        cerr << "\t--" << String{Main::CommandNames::kRunDirectly}.AsNarrowSDKString () << "          /* Run this process as a directly (doesn't exit until the service is done or ARGUMENT TIMEOUT seconds elapsed ...) but not using service infrastructure */"
             << endl;
        cerr << "\t--" << String{Main::CommandNames::kStart}.AsNarrowSDKString ()
             << "                 /* Service/Control Function: Start the service */" << endl;
        cerr << "\t--" << String{Main::CommandNames::kStop}.AsNarrowSDKString ()
             << "                  /* Service/Control Function: Stop the service */" << endl;
        cerr << "\t--" << String{Main::CommandNames::kForcedStop}.AsNarrowSDKString ()
             << "            /* Service/Control Function: Forced stop the service (after trying to normally stop) */" << endl;
        cerr << "\t--" << String{Main::CommandNames::kRestart}.AsNarrowSDKString ()
             << "               /* Service/Control Function: Stop and then re-start the service (ok if already stopped) */" << endl;
        cerr << "\t--" << String{Main::CommandNames::kForcedRestart}.AsNarrowSDKString ()
             << "         /* Service/Control Function: Stop (force if needed) and then re-start the service (ok if already stopped) */" << endl;
        cerr << "\t--" << String (Main::CommandNames::kReloadConfiguration).AsNarrowSDKString () << "  /* Reload service configuration */" << endl;
        cerr << "\t--" << String{Main::CommandNames::kPause}.AsNarrowSDKString ()
             << "                 /* Service/Control Function: Pause the service */" << endl;
        cerr << "\t--" << String{Main::CommandNames::kContinue}.AsNarrowSDKString ()
             << "              /* Service/Control Function: Continue the paused service */" << endl;
        cerr << "\t--Status                /* Service/Control Function: Print status of running service */ " << endl;
        cerr << "\t--Version               /* print this application version */ " << endl;
        cerr << "\t--help                  /* Print this help. */ " << endl;
        cerr << endl
             << "\tExtra unrecognized parameters for start/restart, and forcedrestart operations will be passed along to the actual "
                "service process"
             << endl;
        cerr << endl;
    }
}

int main (int argc, const char* argv[])
{
    Execution::CommandLine    args{argc, argv};
    Debug::TraceContextBumper ctx{"main", "argv={}"_f, args};
    DbgTrace ("Running as user {}"_f, GetCurrentUserName ());

#if qStroika_Foundation_Execution_Thread_SupportThreadStatistics
    [[maybe_unused]] auto&& cleanupReport = Execution::Finally (
        [] () { DbgTrace ("Exiting main with thread {} running"_f, Execution::Thread::GetStatistics ().fRunningThreads); });
#endif

    /*
     *  This allows for safe signals to be managed in a threadsafe way
     */
    SignalHandlerRegistry::SafeSignalsManager safeSignals;

    /*
     *  Setup basic (optional) error handling.
     */
#if qPlatform_Windows
    Execution::Platform::Windows::RegisterDefaultHandler_invalid_parameter ();
    Execution::Platform::Windows::RegisterDefaultHandler_StructuredException ();
#endif
    Debug::RegisterDefaultFatalErrorHandlers (FatalErorrHandler_);

    /*
     *  SetStandardCrashHandlerSignals not really needed, but helpful for many applications so you get a decent log message/debugging on crash.
     */
    SignalHandlerRegistry::Get ().SetStandardCrashHandlerSignals (SignalHandler{FatalSignalHandler_, SignalHandler::Type::eDirect});

    /*
     *  Ignore SIGPIPE is common practice/helpful in POSIX, but not required by the service manager.
     */
#if qPlatform_POSIX
    SignalHandlerRegistry::Get ().SetSignalHandlers (SIGPIPE, SignalHandlerRegistry::kIGNORED);
#endif

    /*
     *  Setup Logging to the OS logging facility.
     */
    Logger::Activator loggerActivation{Logger::Options{
        .fLogBufferingEnabled         = true,
        .fSuppressDuplicatesThreshold = 5min,
    }};
    Logger::sThe.SetAppenders ([] () {
        static const String kAppName_                            = "WhyTheFuckIsMyNetworkSoSlow"sv;
        using Logging                                            = BackendApp::Common::AppConfigurationType::Logging;
        Logging                                    loggingConfig = BackendApp::Common::gAppConfiguration->fLogging.value_or (Logging{});
        Sequence<shared_ptr<Logger::IAppenderRep>> appenders;
        Logger::sThe.SetAppenders (nullptr);
        if (loggingConfig.ToStdOut.value_or (Logging::kToStdOut_Default)) {
            appenders += make_shared<Logger::StreamAppender> (
                IO::FileSystem::FileOutputStream::New (1, IO::FileSystem::FileStream::AdoptFDPolicy::eDisconnectOnDestruction));
        }
#if qHas_Syslog
        if (loggingConfig.ToSysLog.value_or (Logging::kToSysLog_Default)) {
            appenders += make_shared<Logger::SysLogAppender> (kAppName_);
        }
#elif qPlatform_Windows
        if (loggingConfig.ToWindowsEventLog.value_or (Logging::kToWindowsEventLog_Default)) {
            appenders += make_shared<Logger::WindowsEventLogAppender> (kAppName_);
        }
#endif
        return appenders;
    }());

    /*
     *  Parse command line arguments, and start looking at options.
     */
    shared_ptr<Main::IServiceIntegrationRep> serviceIntegrationRep = Main::mkDefaultServiceIntegrationRep ();
    serviceIntegrationRep                                          = make_shared<Main::LoggerServiceWrapper> (serviceIntegrationRep);

    /*
     *  Without this firewall rule, on windows, SSDP 'listen' discovery doesn't work. The messages
     *  don't make it to this service. 
     *
     * Maybe best to do this with installer, not direct code here (see https://github.com/SophistSolutions/WhyTheFuckIsMyNetworkSoSlow/issues/13)
     */
#if qPlatform_Windows
    try {
        static constexpr Activity kSettingUpFirewall_{"setting up firewall"sv};
        DeclareActivity           da{&kSettingUpFirewall_};
        IO::Network::SystemFirewall::Manager{}.Register (IO::Network::SystemFirewall::Rule{
            "WhyTheFuckIsMyNetworkSoSlow Receive SSDP Notify UDP Access Allowed"sv,
            "Allow UDP/multicast (NOTIFY) traffic for WhyTheFuckIsMyNetworkSoSlow so SSDP listen works (search works without this)"sv,
            "WhyTheFuckIsMyNetworkSoSlow"sv, Execution::GetEXEPath (), NET_FW_PROFILE2_ALL, NET_FW_RULE_DIR_IN, NET_FW_IP_PROTOCOL_UDP,
            "1900"sv, "*"sv, NET_FW_ACTION_ALLOW, true});
    }
    catch (...) {
        String exceptMsg   = Characters::ToString (current_exception ());
        bool   warningOnly = false;
        if (auto errCode = GetAssociatedErrorCode (current_exception ())) {
            if (errCode == errc::permission_denied) {
                warningOnly = true;
                exceptMsg += " Some device discovery features (SSDP Listen) may not function properly. Run as administrator once, or "
                             "re-run the installer to fix this."sv;
            }
        }
        if (warningOnly) {
            Logger::sThe.Log (Logger::eWarning, "{}"_f, exceptMsg);
            cerr << "WARNING: " << exceptMsg.AsNarrowSDKString () << endl;
        }
        else {
            Logger::sThe.Log (Logger::eError, "{}"_f, exceptMsg);
            cerr << "FAILED: " << exceptMsg.AsNarrowSDKString () << endl;
            return EXIT_FAILURE;
        }
    }
#endif

    /*
     * Several components use interval timers, and this allows those modules to run (but have timer service started/shutdown in a controlled
     * fashion).
     */
    Execution::IntervalTimer::Manager::Activator intervalTimerMgrActivator;

    /*
     *  Create service handler instance.
     */
    Main m{make_shared<WTFAppServiceRep> (), serviceIntegrationRep};

    /*
     *  Run request.
     */
    try {
        const CommandLine::Option kStatusOpt_ = CommandLine::Option{.fLongName = "status"sv};
        if (args.Has (kStatusOpt_)) {
            cout << m.GetServiceStatusMessage ().AsUTF8<string> ();
            return EXIT_SUCCESS;
        }
        else if (args.Has (StandardCommandLineOptions::kHelp)) {
            ShowUsage_ (m);
            return EXIT_SUCCESS;
        }
        else if (args.Has (StandardCommandLineOptions::kVersion)) {
            cout << m.GetServiceDescription ().fPrettyName.AsNarrowSDKString () << ": "sv
                 << Characters::ToString (AppVersion::kVersion).AsNarrowSDKString () << endl;
            return EXIT_SUCCESS;
        }
        else {
            /*
             *  Run the commands, and capture/display exceptions
             */
            m.Run (args);
        }
    }
    catch (const Execution::InvalidCommandLineArgument& e) {
        ShowUsage_ (m, e);
    }
    catch (...) {
        String exceptMsg = Characters::ToString (current_exception ());
        Logger::sThe.Log (Logger::eError, "{}"_f, exceptMsg);
        cerr << "FAILED: " << exceptMsg.AsNarrowSDKString () << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
