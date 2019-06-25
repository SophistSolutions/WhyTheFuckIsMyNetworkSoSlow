/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_BLOBMgr_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_BLOBMgr_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Common/GUID.h"
#include "Stroika/Foundation/DataExchange/InternetMediaType.h"
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
     */
    class BLOBMgr {
    public:
        static BLOBMgr sThe;

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
         */
        nonvirtual tuple<BLOB, InternetMediaType> GetBLOB (const GUID& id) const;
    };

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "BLOBMgr.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_BLOBMgr_h_*/
