/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/ToString.h"
#include "Stroika/Foundation/Common/Property.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/IO/Network/Transfer/Connection.h"

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

auto OperationalStatisticsMgr::GetStatistics () const -> Statistics
{
    Statistics   result;
    unsigned int apiCallsCompleted{};
    //unsigned int apiCallsCompletedSuccessfully;-- NYI
    double       totalAPIDuration{0};
    double       maxAPIDuration{0};
    unsigned int dbReads{0};
    unsigned int dbWrites{0};
    //unsigned int       dbErrors{0};  -- NYI
    double totalDBReadDuration{0};
    double totalDBWriteDuration{0};
    double maxDBDuration{0};
    auto   doOne = [&] (const Rec_& r) {
        switch (r.fKind) {
            case Rec_::Kind::eAPI: {
                ++apiCallsCompleted;
                totalAPIDuration += r.fDuration;
                maxAPIDuration = max (maxAPIDuration, r.fDuration);
            } break;
            case Rec_::Kind::eDBRead: {
                ++dbReads;
                totalDBReadDuration += r.fDuration;
                maxDBDuration = max (maxDBDuration, r.fDuration);
            } break;
            case Rec_::Kind::eDBWrite: {
                ++dbWrites;
                totalDBWriteDuration += r.fDuration;
                maxDBDuration = max (maxDBDuration, r.fDuration);
            } break;
        }
    };
    lock_guard lk{fMutex_};
    // hit every entry and just skip those with null events
    Time::DurationSecondsType skipBefore = Time::GetTickCount () - kLookbackInterval.As<Time::DurationSecondsType> ();
    for (size_t i = 0; i < Memory::NEltsOf (fRollingHistory_); ++i) {
        // could optimize slightly and skip a bunch in a row, but not worht the trouble probably
        if (fRollingHistory_[i].fAt >= skipBefore) {
            doOne (fRollingHistory_[i]);
        }
    }

    result.fRecentAPI.fCallsCompleted             = apiCallsCompleted;
    result.fRecentAPI.fCallsCompletedSuccessfully = apiCallsCompleted; // @todo fix
    if (apiCallsCompleted > 0) {
        result.fRecentAPI.fMeanDuration = Duration{totalAPIDuration / apiCallsCompleted};
        result.fRecentAPI.fMaxDuration  = Duration{maxAPIDuration};
    }

    result.fRecentDB.fReads  = dbReads;
    result.fRecentDB.fWrites = dbWrites;
    result.fRecentDB.fErrors = 0; // @todo fix
    if (dbReads > 0) {
        result.fRecentDB.fMeanReadDuration = Duration{totalDBReadDuration / dbReads};
    }
    if (dbWrites > 0) {
        result.fRecentDB.fMeanWriteDuration = Duration{totalDBWriteDuration / dbWrites};
    }
    if (dbReads > 0 or dbWrites > 0) {
        result.fRecentDB.fMaxDuration = Duration{maxDBDuration};
    }
    return result;
}
