/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include <cstdlib>
#include <iostream>

#include "Stroika/Foundation/Characters/String_Constant.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/Execution/Finally.h"
#include "Stroika/Foundation/Execution/Logger.h"
#include "Stroika/Foundation/Execution/Sleep.h"
#include "Stroika/Foundation/Execution/Thread.h"
#include "Stroika/Foundation/Execution/WaitableEvent.h"
#include "Stroika/Frameworks/Service/Main.h"

#include "Discovery/Devices.h"

#include "WebServices/WSImpl.h"
#include "WebServices/WebServer.h"

#include "AppVersion.h"

#include "Service.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Frameworks::Service;

using Characters::String_Constant;
using Execution::Thread;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;

using Execution::Logger;

namespace {
    const Main::ServiceDescription kServiceDescription_{
        String_Constant (L"WhyTheFuckIsMyNetworkSoSlow-Service"),
        String_Constant (L"WhyTheFuckIsMyNetworkSoSlow Service")};
}

void WTFAppServiceRep::MainLoop (const std::function<void()>& startedCB)
{
    Debug::TraceContextBumper ctx{"WTFAppServiceRep::MainLoop"};
    auto&&                    cleanup = Execution::Finally ([&]() {
        Thread::SuppressInterruptionInContext suppressSoWeActuallyShutDownOtherTaskWhenWereBeingShutDown;
        Logger::Get ().Log (Logger::Priority::eInfo, L"Beginning service shutdown");
    });
    IgnoreExceptionsForCall (WebServices::TmpHackAssureStartedMonitoring ());
    WebServer webServer{make_shared<WSImpl> ()};
    startedCB (); // Notify service control mgr that the service has started
    Logger::Get ().Log (Logger::Priority::eInfo, L"%s (version %s) service started successfully", kServiceDescription_.fPrettyName.c_str (), Characters::ToString (AppVersion::kVersion).c_str ());

    // Wait here until a 'service stop' command sends a thread-abort, and that will cause this wait to be abandoned and this stackframe to unwind
    Execution::WaitableEvent (Execution::WaitableEvent::eAutoReset).Wait (); // wait til service shutdown ThreadAbortException
}

Main::ServiceDescription WTFAppServiceRep::GetServiceDescription () const
{
    return kServiceDescription_;
}
