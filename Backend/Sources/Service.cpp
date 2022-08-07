/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include <cstdlib>

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Debug/Trace.h"
#include "Stroika/Foundation/Execution/Finally.h"
#include "Stroika/Foundation/Execution/Logger.h"
#include "Stroika/Foundation/Execution/Thread.h"
#include "Stroika/Foundation/Execution/WaitableEvent.h"
#include "Stroika/Frameworks/Service/Main.h"

#include "Common/BLOBMgr.h"

#include "Discovery/Devices.h"
#include "Discovery/NetworkInterfaces.h"

#include "IntegratedModel/Mgr.h"

#include "WebServices/WSImpl.h"
#include "WebServices/WebServer.h"

#include "AppVersion.h"

#include "Service.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Frameworks::Service;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;

using Execution::Logger;

namespace {
    const Main::ServiceDescription kServiceDescription_{
        L"WhyTheFuckIsMyNetworkSoSlow-Service"sv,
        L"WhyTheFuckIsMyNetworkSoSlow Service"sv};
}

void WTFAppServiceRep::MainLoop (const std::function<void ()>& startedCB)
{
    Debug::TraceContextBumper ctx{"WTFAppServiceRep::MainLoop"};
    // Activator objects cause the discovery modules to start/stop so RAAI controls startup/shutdown even with exceptions
    // deviceMgr calls NetworkMgr so order here is important. And webserver can call either. Allowing destruction to shutdown guarantees proper ordering
    // of dependencies on shutdown
    Common::BLOBMgr::Activator                 blobMgrActivator;
    Discovery::NetworkInterfacesMgr::Activator networkInterfacesMgrActivator;
    Discovery::NetworksMgr::Activator          networkMgrActivator;
    Discovery::DevicesMgr::Activator           devicesMgrActivator;
    IntegratedModel::Mgr::Activator            integratedModelMgrActivator;
    WebServer                                  webServer{make_shared<WSImpl> ()};
    startedCB (); // Notify service control mgr that the service has started
    Logger::sThe.Log (Logger::eInfo, L"%s (version %s) service started successfully", kServiceDescription_.fPrettyName.c_str (), Characters::ToString (AppVersion::kVersion).c_str ());

    [[maybe_unused]] auto&& cleanup = Execution::Finally ([&] () {
        Execution::Thread::SuppressInterruptionInContext suppressSoWeActuallyShutDownOtherTaskWhenWereBeingShutDown;
        Logger::sThe.Log (Logger::eInfo, L"Beginning service shutdown");
    });

    // Wait here until a 'service stop' command sends a thread-abort, and that will cause this wait to be abandoned and this stackframe to unwind
    Execution::WaitableEvent{Execution::WaitableEvent::eAutoReset}.Wait (); // wait til service shutdown ThreadAbortException
}

Main::ServiceDescription WTFAppServiceRep::GetServiceDescription () const
{
    return kServiceDescription_;
}
