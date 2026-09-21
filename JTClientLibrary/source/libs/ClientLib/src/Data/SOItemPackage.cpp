//
// Created by YUMBUL on 15.06.2024.
//

#include "SOItemPackage.h"


CRefPackageItemData *CSOItemPackage::GetPackageItemData() const {
    return m_pPackageItemData;
}
CSOItem *CSOItemPackage::GetSOItem() const {
    return GetSOItem(0);
}

CSOItem *CSOItemPackage::GetSOItem(size_t index) const {
    if (index < m_vPSOItem.size()) return m_vPSOItem[index];

    return NULL;
}

size_t CSOItemPackage::GetSOItemCount() const {
    return m_vPSOItem.size();
}
