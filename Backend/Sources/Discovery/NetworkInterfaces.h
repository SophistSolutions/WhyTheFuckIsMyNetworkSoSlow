/*
* Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
*/
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_NetworkInterfaces_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_NetworkInterfaces_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Containers/Collection.h"
#include "Stroika/Foundation/Containers/Sequence.h"
#include "Stroika/Foundation/Containers/Set.h"
#include "Stroika/Foundation/DataExchange/VariantValue.h"
#include "Stroika/Foundation/IO/Network/Interface.h"

/**
 *
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Discovery {

    using namespace Stroika;
    using namespace Stroika::Foundation;

    using Characters::String;
    using Containers::Collection;
    using Containers::Mapping;
    using Containers::Set;
    using Stroika::Foundation::Common::GUID;


    /**
     *  OK to inherit from Stroika interface (subject to change) because this is not externalized - just
     *  used internally).
     */
    struct NetworkInterface : IO::Network::Interface {
        /**
         *  GUID for this interface - MANUFACTURED by WTF
         // @todo rename fID, and actually auto-generate it uniquly somehow (or OK to use iwndows based one)
         */
        GUID fGUID;

#if qDebug
        Mapping<String, DataExchange::VariantValue> fDebugProps;
#endif

        NetworkInterface (const NetworkInterface& src) = default;
        NetworkInterface (const IO::Network::Interface& src);

        /**
         *  @see Characters::ToString ();
         */
        nonvirtual String ToString () const;
    };

    /**
     *  Public APIs fully <a href='ThirdPartyComponents/Stroika/StroikaRoot/Documentation/Thread-Safety.md#Internally-Synchronized-Thread-Safety'>Internally-Synchronized-Thread-Safety</a>
     *
     *  Only legal to call NetworkInterfacesMgr while its active (checked with assertions). Callers responsability to assure, but checked with assertions.
     */
    class NetworkInterfacesMgr {
    public:
        NetworkInterfacesMgr ()                            = default;
        NetworkInterfacesMgr (const NetworkInterfacesMgr&) = delete;

    public:
        /**
         *  At most one such object may exist. When it does, the NetworkInterfacesMgr is active and usable. Its illegal to call otherwise.
         */
        struct Activator {
            Activator ();
            ~Activator ();
        };

    public:
        /**
         */
        nonvirtual Collection<NetworkInterface> CollectAllNetworkInterfaces (optional<Time::DurationSecondsType> allowedStaleness = {}) const;

    public:
        /**
         */
        nonvirtual Collection<NetworkInterface> CollectActiveNetworkInterfaces (optional<Time::DurationSecondsType> allowedStaleness = {}) const;

    public:
        static NetworkInterfacesMgr sThe;
    };

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "NetworkInterfaces.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Discovery_NetworkInterfaces_h_*/
