/*
 * Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
 */
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_PrioritizedName_h_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_PrioritizedName_h_ 1

#include "Stroika/Frameworks/StroikaPreComp.h"

#include "Stroika/Foundation/Characters/String.h"
#include "Stroika/Foundation/Common/Compare.h"
#include "Stroika/Foundation/Containers/SortedCollection.h"
#include "Stroika/Foundation/DataExchange/ObjectVariantMapper.h"

/**
 *
 */

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common {

    using namespace Stroika;
    using namespace Stroika::Foundation;

    using Characters::String;

    struct PrioritizedName {
        String       fName;
        unsigned int fPriority{};

        bool operator== (const PrioritizedName& rhs) const;
#if __cpp_impl_three_way_comparison < 201711
        bool operator!= (const PrioritizedName& rhs) const
        {
            return not(*this == rhs);
        }
#endif

        String ToString () const;
    };

    /*
     * highest priority name kept first
     */
    constexpr auto kDefaultPrioritizedName_OrderByDefault_Less =
        Stroika::Foundation::Common::DeclareInOrderComparer ([] (const PrioritizedName& lhs, const PrioritizedName& rhs) -> bool {
            if (lhs.fPriority > rhs.fPriority) {
                return true;
            }
            else if (lhs.fPriority < rhs.fPriority) {
                return false;
            }
            return lhs.fName < rhs.fName;
        });

    struct PrioritizedNames : Containers::SortedCollection<PrioritizedName> {
        PrioritizedNames ();
        PrioritizedNames (const PrioritizedNames& src) = default;

        nonvirtual String GetName () const;
        using SortedCollection<PrioritizedName>::Add;
        nonvirtual void Add (const String& name, unsigned int priority);

        static const DataExchange::ObjectVariantMapper kMapper;
    };

}

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */
#include "PrioritizedName.inl"

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_PrioritizedName_h_*/
