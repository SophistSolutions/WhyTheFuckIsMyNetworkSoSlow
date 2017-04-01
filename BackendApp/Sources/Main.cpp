/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2017.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include <iostream>

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Execution/SignalHandlers.h"
#include "Stroika/Foundation/Execution/WaitableEvent.h"

#include "Discovery/Devices.h"

#include "WebServices/WSImpl.h"
#include "WebServices/WebServer.h"

using namespace std;

using namespace Stroika::Foundation;

using Characters::String;
using Memory::BLOB;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::WebServices::Model;

int main (int argc, const char* argv[])
{
    Execution::SignalHandlerRegistry::SafeSignalsManager safeSignals;
#if qPlatform_POSIX
    Execution::SignalHandlerRegistry::Get ().SetSignalHandlers (SIGPIPE, Execution::SignalHandlerRegistry::kIGNORED);
#endif

    try {
        WebServices::TmpHackAssureStartedMonitoring ();
        WebServer webServer{make_shared<WSImpl> ()};
        Execution::WaitableEvent (Execution::WaitableEvent::eAutoReset).Wait (); // wait forever - til user hits ctrl-c
    }
    catch (...) {
        cerr << "Error encountered: "
             << Characters::ToString (current_exception ()).AsNarrowSDKString ()
             << " - terminating..." << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
