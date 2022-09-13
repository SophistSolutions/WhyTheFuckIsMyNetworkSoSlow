/*
* Copyright(c) Sophist Solutions, Inc. 1990-2021.  All rights reserved
*/
#ifndef _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_PrioritizedName_inl_
#define _WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_PrioritizedName_inl_ 1

/*
 ********************************************************************************
 ***************************** Implementation Details ***************************
 ********************************************************************************
 */

#include "Stroika/Foundation/Characters/StringBuilder.h"

namespace WhyTheFuckIsMyNetworkSoSlow::BackendApp::Common {

    /*
     ********************************************************************************
     ******************************** PrioritizedName *******************************
     ********************************************************************************
     */
    inline bool PrioritizedName::operator== (const PrioritizedName& rhs) const
    {
        if (fName != rhs.fName) {
            return false;
        }
        if (fPriority != rhs.fPriority) {
            return false;
        }
        return true;
    }
    inline String PrioritizedName::ToString () const
    {
        Characters::StringBuilder sb;
        sb += L"{";
        sb += L"Name: " + Characters::ToString (fName) + L",";
        sb += L"Priority: " + Characters::ToString (fPriority);
        sb += L"}";
        return sb.str ();
    }

    /*
     ********************************************************************************
     ******************************* PrioritizedNames *******************************
     ********************************************************************************
     */
    inline PrioritizedNames::PrioritizedNames ()
        : SortedCollection<PrioritizedName>{kDefaultPrioritizedName_OrderByDefault_Less}
    {
    }
    inline String PrioritizedNames::GetName () const
    {
        for (auto i : *this) {
            return i.fName;
        }
        return L"Unknown"sv;
    }
    inline void PrioritizedNames::Add (const String& name, unsigned int priority)
    {
        Require (not name.empty ()); // maybe allow so dont have to check everywhere?
        for (auto i = begin (); i != end (); ++i) {
            if (i->fName == name) {
                if (priority > i->fPriority) {
                    Update (i, PrioritizedName{name, priority});
                }
                return;
            }
        }
        Add (PrioritizedName{name, priority});
    }

}

#endif /*_WhyTheFuckIsMyNetworkSoSlow_BackendApp_Common_PrioritizedName_inl_*/
