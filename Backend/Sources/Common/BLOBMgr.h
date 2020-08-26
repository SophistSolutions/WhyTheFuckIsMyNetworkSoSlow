/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2020.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_BLOBMgr_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_BLOBMgr_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Common/GUID.h"
#include "Stroika/Foundation/Containers/Bijection.h"
#include "Stroika/Foundation/Containers/Mapping.h"
#include "Stroika/Foundation/DataExchange/InternetMediaType.h"
#include "Stroika/Foundation/Execution/Synchronized.h"
#include "Stroika/Foundation/Execution/ThreadPool.h"
#include "Stroika/Foundation/IO/Network/URI.h"
#include "Stroika/Foundation/Memory/BLOB.h"

/**
 *
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common {

    using namespace Stroika;
    using namespace Stroika::Foundation;

    using Characters::String;
    using DataExchange::InternetMediaType;
    using IO::Network::URI;
    using Memory::BLOB;
    using Stroika::Foundation::Common::GUID;

    /**
     *  Fully internally synchronized.
     *
     *  For now, this is application lifetime storage, but before long we will need to store in DB.
     * 
     *  This is not STRICTLY a BLOB-Manager, because it also stores external named KEYs to the BLOBs (URI).
     *  Merged into this class, and not layered elsewhere because we will want to persist that mapping too.
     */
    class BLOBMgr {
    public:
        static BLOBMgr sThe;

    public:
        /**
         *  At most one such object may exist. When it does, the BLOBMgr is active and usable. Its illegal to call otherwise.
         */
        struct Activator {
            Activator ();
            ~Activator ();
        };

    public:
        /**
         */
        nonvirtual GUID AddBLOB (const BLOB& b, const InternetMediaType& ct);

    public:
        /**
         *  This may cache (based on url) and not fetch each time...
         */
        nonvirtual GUID AddBLOBFromURL (const URI& url);

    public:
        /**
         *  Queue up work - fetching that url, and store it when it becomes available
         */
        nonvirtual optional<GUID> AsyncAddBLOBFromURL (const URI& url, bool allowExpired = false);

    public:
        /**
         *  Check if there is a BLOB stored associated with the given URL.
         */
        nonvirtual optional<GUID> Lookup (const URI& url, bool allowExpired = false);

    public:
        /**
         */
        nonvirtual tuple<BLOB, InternetMediaType> GetBLOB (const GUID& id) const;

    private:
        Execution::Synchronized<Containers::Bijection<GUID, tuple<BLOB, InternetMediaType>>> fStorage_;
        Execution::Synchronized<Containers::Mapping<URI, GUID>>                              fURI2BLOBMap_;
        Execution::Synchronized<unique_ptr<Execution::ThreadPool>>                           fThreadPool_;
    };
    inline BLOBMgr BLOBMgr::sThe; // @todo recondider if this follows new Stroika Singleton pattern -- LGP 2020-08-20

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "BLOBMgr.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_BLOBMgr_h_*/
