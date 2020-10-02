/*
* Copyright(c) Sophist Solutions, Inc. 1990-2020.  All rights reserved
*/
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Cache/Memoizer.h"
#include "Stroika/Foundation/Cache/SynchronizedTimedCache.h"
#include "Stroika/Foundation/Characters/StringBuilder.h"
#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/DataExchange/Variant/JSON/Reader.h"
#include "Stroika/Foundation/IO/Network/Transfer/Connection.h"

#include "GeoLocAndISPLookup.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Cache;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;

using DataExchange::VariantValue;
using IO::Network::URI;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common;

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

namespace {
    String NormalizeISPName_ (const String& ispName)
    {
        if (ispName.Contains (L"d/b/a Verizon"_k)) {
            return L"Verizon"_k;
        }
        return ispName;
    }
    optional<String> NormalizeISPName_ (const optional<String>& ispName)
    {
        return ispName.has_value () ? NormalizeISPName_ (*ispName) : optional<String>{};
    }
}

/*
 ********************************************************************************
 ******************* BackendApp::Common::GEOLocAndISPLookup *********************
 ********************************************************************************
 */
#if qCompilerAndStdLib_template_template_argument_as_different_template_paramters_Buggy
namespace {
    template <typename T1, typename T2>
    using CACHE_BWA_ = SynchronizedTimedCache<T1, T2>;
}
#endif
optional<tuple<GEOLocationInformation, InternetServiceProvider>> BackendApp::Common::GEOLocAndISPLookup (InternetAddress ia)
{
    /*
     *  Could fetch from https://ipstack.com/documentation but required signup for account.
     *
     *  Instead - for now - went with:
     *          http://ip-api.com/docs/api:json
     *          EXAMPLE: http://ip-api.com/json/108.49.190.49
     */
#if USE_NOISY_TRACE_IN_THIS_MODULE_
    Debug::TraceContextBumper ctx{L"GEOLocAndISPLookup"};
#endif
    constexpr Time::DurationSecondsType kInfoTimeoutInSeconds_{10 * 60.0};
#if qCompilerAndStdLib_template_template_argument_as_different_template_paramters_Buggy
    static Memoizer<optional<tuple<GEOLocationInformation, InternetServiceProvider>>, CACHE_BWA_, InternetAddress> sMemoizeCache_ = {
#else
    static Memoizer<optional<tuple<GEOLocationInformation, InternetServiceProvider>>, SynchronizedTimedCache, InternetAddress> sMemoizeCache_ = {
#endif
        [] (InternetAddress ia) -> optional<tuple<GEOLocationInformation, InternetServiceProvider>> {
            using namespace DataExchange;
            using namespace IO::Network::Transfer;
            Debug::TraceContextBumper ctx{L"GEOLocAndISPLookup::{}... real lookup - cache miss"};

            auto&&                        connection = Connection::New ();
            Mapping<String, VariantValue> m          = Variant::JSON::Reader ().Read (connection.GET (URI{L"http://ip-api.com/json/" + ia.ToString ()}).GetDataTextInputStream ()).As<Mapping<String, VariantValue>> ();
            GEOLocationInformation        geoloc{};
            auto                          cvt = [] (optional<VariantValue> v) -> optional<String> { return v ? optional<String>{v->As<String> ()} : optional<String>{}; };
            geoloc.fRegionCode                = cvt (m.Lookup (L"region"_k));
            geoloc.fCountryCode               = cvt (m.Lookup (L"countryCode"_k));
            geoloc.fCity                      = cvt (m.Lookup (L"city"_k));
            geoloc.fPostalCode                = cvt (m.Lookup (L"zip"_k));
            optional<VariantValue> lat        = m.Lookup (L"lat"_k);
            optional<VariantValue> lon        = m.Lookup (L"lon"_k);
            if (lat and lon) {
                geoloc.fLatitudeAndLongitude = make_tuple (lat->As<float> (), lon->As<float> ());
            }
            InternetServiceProvider isp{};
            isp.name = NormalizeISPName_ (cvt (m.Lookup (L"isp"_k)));
            return make_tuple (geoloc, isp);
        },
        SynchronizedTimedCache<tuple<InternetAddress>, optional<tuple<GEOLocationInformation, InternetServiceProvider>>>{kInfoTimeoutInSeconds_}
    };
    return sMemoizeCache_.Compute (ia);
}
