/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include <cstdlib>
#include <iostream>

#include "Stroika/Foundation/Characters/String_Constant.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/Execution/Finally.h"
#include "Stroika/Foundation/Execution/Sleep.h"
#include "Stroika/Foundation/Execution/Thread.h"
#include "Stroika/Foundation/Execution/WaitableEvent.h"
#include "Stroika/Frameworks/Service/Main.h"

#include "Discovery/Devices.h"

#include "WebServices/WSImpl.h"
#include "WebServices/WebServer.h"

#include "Service.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Frameworks::Service;

using Characters::String_Constant;
using Execution::Thread;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;

#include "Stroika/Foundation/Execution/Logger.h"
using Execution::Logger;

namespace {
    const Main::ServiceDescription kServiceDescription_{
        String_Constant (L"Test-Service"),
        String_Constant (L"Test Service")};
}

void WTFAppServiceRep::MainLoop (const std::function<void()>& startedCB)
{
	Debug::TraceContextBumper ctx{ "WTFAppServiceRep::MainLoop" };
	auto&& cleanup = Execution::Finally ([&]() {
        Thread::SuppressInterruptionInContext suppressSoWeActuallyShutDownOtherTaskWhenWereBeingShutDown;
        Logger::Get ().Log (Logger::Priority::eInfo, L"Beginning service shutdown");
    });
    WebServices::TmpHackAssureStartedMonitoring ();
    WebServer webServer{make_shared<WSImpl> ()};
    startedCB (); // Notify service control mgr that the service has started
    Logger::Get ().Log (Logger::Priority::eInfo, L"Service started successfully");
    Execution::WaitableEvent (Execution::WaitableEvent::eAutoReset).Wait (); // wait til service shutdown ThreadAbortException
}

Main::ServiceDescription WTFAppServiceRep::GetServiceDescription () const
{
    return kServiceDescription_;
}
