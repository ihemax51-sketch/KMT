///******************************************************************************
/// \File ReferenceDataMgr.cpp
///
/// \Desc
///
///******************************************************************************
#include "ReferenceDataMgr.h"

#include <algorithm>

RefObjCommon *CRefDataManager::GetRefObj(DWORD dwObjID) const {
    std::list<std::pair<DWORD, RefObjCommon *>>::const_iterator it = m_listRefGameObj.begin();
    for (; it != m_listRefGameObj.end(); it++) {
        if ((*it).first == dwObjID)
            return (*it).second;
    }

    return NULL;
}
