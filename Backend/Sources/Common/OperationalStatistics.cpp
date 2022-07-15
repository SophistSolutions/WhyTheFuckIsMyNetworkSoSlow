/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Common/Property.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/Transfer/Connection.h"
#include "Stroika/Foundation/Math/Statistics.h"
#include "Stroika/Foundation/Memory/Optional.h"

#include "DB.h"

#include "OperationalStatistics.h"

using namespace std;

using namespace Stroika::Foundation;
using namespace Stroika::Foundation::Characters;
using namespace Stroika::Foundation::Containers;
using namespace Stroika::Foundation::Execution;

using Memory::BLOB;
using Stroika::Foundation::Common::ConstantProperty;
using Stroika::Foundation::Common::GUID;
using Stroika::Foundation::Database::SQL::ORM::Schema::CatchAllField;
using Stroika::Foundation::Database::SQL::ORM::Schema::Field;
using Stroika::Foundation::Database::SQL::ORM::Schema::Table;
using Stroika::Foundation::DataExchange::ObjectVariantMapper;

using namespace WhyTheFuckIsMyNetworkSoSlow;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp;
using namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common;

// Comment this in to turn on aggressive noisy DbgTrace in this module
//#define USE_NOISY_TRACE_IN_THIS_MODULE_ 1

// @todo Lose DIGEST code and use new UUID::CreateNew () method when available.

/*
 ********************************************************************************
 ***************** OperationalStatisticsMgr::ProcessAPICmd **********************
 ********************************************************************************
 */
OperationalStatisticsMgr::ProcessAPICmd::~ProcessAPICmd ()
{
    Time::DurationSecondsType now{Time::GetTickCount ()};
    sThe.Add_ (Rec_{Rec_::Kind::eAPI, now, now - fStart_});
}

void OperationalStatisticsMgr::ProcessAPICmd::NoteError ()
{
    Time::DurationSecondsType now{Time::GetTickCount ()};
    sThe.Add_ (Rec_{Rec_::Kind::eAPIError, now, now});
}

/*
 ********************************************************************************
 ****************** OperationalStatisticsMgr::ProcessDBCmd **********************
 ********************************************************************************
 */
OperationalStatisticsMgr::ProcessDBCmd::~ProcessDBCmd ()
{
    Time::DurationSecondsType now{Time::GetTickCount ()};
    sThe.Add_ (Rec_{fKind_, now, now - fStart_});
}

void OperationalStatisticsMgr::ProcessDBCmd::NoteError ()
{
    Time::DurationSecondsType now{Time::GetTickCount ()};
    sThe.Add_ (Rec_{Rec_::Kind::eDBError, now, now});
}

void OperationalStatisticsMgr::RecordInputQLength (size_t length)
{
    Time::DurationSecondsType now{Time::GetTickCount ()};
    sThe.Add_ (Rec_{Rec_::Kind::eAPIInputQLength, now, static_cast<Time::DurationSecondsType> (length)});
}

auto OperationalStatisticsMgr::GetStatistics () const -> Statistics
{
    Statistics result;

    using Time::Duration;
    using Time::DurationSecondsType;
    // hit every entry and just skip those with null events
    DurationSecondsType skipBefore = Time::GetTickCount () - kLookbackInterval.As<DurationSecondsType> ();

    // could optimize slightly and skip a bunch in a row, but not worht the trouble probably
    Iterable<Rec_> allApplicable = [&] () {
        lock_guard lk{fMutex_};
        return Sequence<Rec_>{begin (fRollingHistory_), end (fRollingHistory_)}.Where ([&] (const Rec_& r) { return r.fAt >= skipBefore and r.fKind != Rec_::Kind::eNull; });
    }();

    {
        Iterable<DurationSecondsType> apiTimes = allApplicable.Select<DurationSecondsType> ([] (const Rec_& r) -> optional<DurationSecondsType> { if (r.fKind == Rec_::Kind::eAPI) return r.fDuration; return nullopt; });
        if (not apiTimes.empty ()) {
            result.fRecentAPI.fMeanDuration   = Duration{Math::Mean (apiTimes)};
            result.fRecentAPI.fMedianDuration = Duration{Math::Median (apiTimes)};
            result.fRecentAPI.fMaxDuration    = Duration{*apiTimes.Max ()};
        }
        result.fRecentAPI.fCallsCompleted = static_cast<unsigned int> (apiTimes.length ());
        result.fRecentAPI.fErrors         = static_cast<unsigned int> (allApplicable.Count ([] (const Rec_& r) { return r.fKind == Rec_::Kind::eAPIError; }));
    }
    {
        Iterable<float> apiQLength = allApplicable.Select<float> ([] (const Rec_& r) -> optional<float> { if (r.fKind == Rec_::Kind::eAPIInputQLength) return static_cast<float> (r.fDuration); return nullopt; });
        if (not apiQLength.empty ()) {
            result.fRecentAPI.fMeanQLength   = Math::Mean (apiQLength);
            result.fRecentAPI.fMedianQLength = Math::Median (apiQLength);
        }
    }
    {
        Iterable<DurationSecondsType> dbReadTimes = allApplicable.Select<DurationSecondsType> ([] (const Rec_& r) -> optional<DurationSecondsType> { if (r.fKind == Rec_::Kind::eDBRead) return r.fDuration; return nullopt; });
        if (not dbReadTimes.empty ()) {
            result.fRecentDB.fMeanReadDuration   = Duration{dbReadTimes.MeanValue ()};
            result.fRecentDB.fMedianReadDuration = Duration{dbReadTimes.MedianValue ()};
            result.fRecentDB.fMaxDuration        = Duration{*dbReadTimes.Max ()};
        }
        result.fRecentDB.fReads = static_cast<unsigned int> (dbReadTimes.length ());
    }
    {
        Iterable<DurationSecondsType> dbWriteTimes = allApplicable.Select<DurationSecondsType> ([] (const Rec_& r) -> optional<DurationSecondsType> { if (r.fKind == Rec_::Kind::eDBWrite) return r.fDuration; return nullopt; });
        if (not dbWriteTimes.empty ()) {
            result.fRecentDB.fMeanWriteDuration   = Duration{dbWriteTimes.MeanValue ()};
            result.fRecentDB.fMedianWriteDuration = Duration{dbWriteTimes.MedianValue ()};
            Memory::AccumulateIf (&result.fRecentDB.fMaxDuration, Duration{*dbWriteTimes.Max ()}, [] (Duration l, Duration r) { return max (l, r); });
        }
        result.fRecentDB.fWrites = static_cast<unsigned int> (dbWriteTimes.length ());
    }
    result.fRecentDB.fErrors = static_cast<unsigned int> (allApplicable.Count ([] (const Rec_& r) { return r.fKind == Rec_::Kind::eDBError; }));
    return result;
}