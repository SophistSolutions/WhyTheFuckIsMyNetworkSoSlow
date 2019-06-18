/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2019.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_EthernetMACAddressOUIPrefixes_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_EthernetMACAddressOUIPrefixes_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/IO/Network/InternetAddress.h"

/**
 *
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common {

    using namespace Stroika;
    using namespace Stroika::Foundation;

    using Characters::String;

    /**
     *  For the purpose of this API, a hardware address is a series of HEX digits, most significant first. These digits may or may
     *  not be '-' or ':' separated. They can be any case. Only the first 6 hex digits are examined.
     *
     *  This function (typically) returns the (printable ui) name of the manufacturer of the device with this hardware address
     *  (e.g. 'Dell', 'Apple', 'AsusTek', 'NetGear' etc).
     */
    optional<String> LookupEthernetMACAddressOUIFromPrefix (const String& hardware);

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "EthernetMACAddressOUIPrefixes.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_EthernetMACAddressOUIPrefixes_h_*/
