/*
* Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Characters/RegularExpression.h"
#include "Stroika/Foundation/Containers/Mapping.h"
#include "Stroika/Foundation/Execution/Logger.h"
#include "Stroika/Foundation/Execution/Module.h"
#include "Stroika/Foundation/IO/FileSystem/FileInputStream.h"
#include "Stroika/Foundation/Streams/TextReader.h"

#include "GeoLocAndISPLookup.h"

#include "EthernetMACAddressOUIPrefixes.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Streams;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common;

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

/*
 ********************************************************************************
 ******** BackendApp::Common::LookupEthernetMACAddressOUIFromPrefix *************
 ********************************************************************************
 */
optional<String> BackendApp::Common::LookupEthernetMACAddressOUIFromPrefix (const String& hardware)
{
    // case insensitive lookup, and cache mapping from config file
    static Mapping<String, String> sMap_ = [] () {
        Mapping<String, String> tmp;
        try {
            for (String line : TextReader::New (IO::FileSystem::FileInputStream::New (Execution::GetEXEDir () + L"data/OSI-MAC-PREFIXES.txt")).ReadLines ()) {
                String macaddrprefix = line.SafeSubString (0, 6).ToLowerCase ();
                if (macaddrprefix.length () == 6) {
                    tmp.Add (macaddrprefix, line.SafeSubString (7).RTrim ());
                }
            }
        }
        catch (...) {
            using Execution::Logger;
            Logger::Get ().Log (Logger::Priority::eError, L"Error encountered reading OSI-MAC-PREFIXES: %s", Characters::ToString (current_exception ()).c_str ());
        }
        return tmp;
    }();
    //String token2Lookup = hardware.ReplaceAll (L"-", L"").ReplaceAll (L":", L"").SafeSubString (0, 6).ToLowerCase ();
    String token2Lookup = hardware.ReplaceAll (L"[\-:]"_RegEx, L"").SafeSubString (0, 6).ToLowerCase ();
    return sMap_.Lookup (token2Lookup);
}
