/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_OperationalStatistics_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_OperationalStatistics_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Time/Duration.h"

/**
 *
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common {

    using namespace Stroika;
    using namespace Stroika::Foundation;

    using Time::Duration;

    /**
     *  Fully internally synchronized.
     *
     *  Simple API to track recent application statistics.
     */
    class OperationalStatisticsMgr {
    public:
        static OperationalStatisticsMgr sThe;

    public:
        enum DBCommandType { eRead,
                             eWrite };

    public:
        static inline const Time::Duration kLookbackInterval{5min};

    public:
        /**
         */
        class ProcessAPICmd;

    public:
        /**
         */
        class ProcessDBCmd;

    public:
        struct Statistics;

    public:
        /**
         */
        nonvirtual Statistics GetStatistics () const;

    private:
        mutable mutex fMutex_; // protect all data with single quick access mutex
        struct Rec_ {
            enum Kind { eNull,
                        eDBRead,
                        eDBWrite,
                        eDBError,
                        eAPI };
            Kind                      fKind;
            Time::DurationSecondsType fAt;
            Time::DurationSecondsType fDuration;
        };
        Rec_   fRollingHistory_[1024]; // @todo see https://stroika.atlassian.net/browse/STK-174 - redo as circular q when available
        size_t fNextHistory_{0};       // circular - can be < first. - first==last implies zero length q

        void Add_ (const Rec_& r);
    };

    /**
         */
    class OperationalStatisticsMgr::ProcessAPICmd {
    public:
        ProcessAPICmd ();
        ~ProcessAPICmd ();

    private:
        Time::DurationSecondsType fStart_;
    };

    /**
         */
    class OperationalStatisticsMgr::ProcessDBCmd {
    public:
        ProcessDBCmd (DBCommandType cmdType);
        ~ProcessDBCmd ();

    private:
        Rec_::Kind                fKind_;
        Time::DurationSecondsType fStart_;
    };

    struct OperationalStatisticsMgr::Statistics {
        struct WSAPI {
            unsigned int       fCallsCompleted;
            unsigned int       fCallsCompletedSuccessfully;
            optional<Duration> fMeanDuration;
            optional<Duration> fMaxDuration;
        };
        struct DB {
            unsigned int       fReads;
            unsigned int       fWrites;
            unsigned int       fErrors;
            optional<Duration> fMeanReadDuration;
            optional<Duration> fMeanWriteDuration;
            optional<Duration> fMaxDuration;
        };

        WSAPI fRecentAPI;
        DB    fRecentDB;
    };

    inline OperationalStatisticsMgr OperationalStatisticsMgr::sThe; // @todo recondider if this follows new Stroika Singleton pattern -- LGP 2020-08-20

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "OperationalStatistics.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_OperationalStatistics_h_*/
