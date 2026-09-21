#pragma once

#include "Data/RefPackageItemData.h"
#include "SOItem.h"

class CSOItemPackage {
public:
    CRefPackageItemData* GetPackageItemData() const;
    CSOItem* GetSOItem() const;
    CSOItem* GetSOItem(size_t index) const;
    size_t GetSOItemCount() const;

public:
    char pad_0000[0x28]; //0x0000
    std::vector<CSOItem*> m_vPSOItem; //0x0028
    CRefPackageItemData* m_pPackageItemData; //0x0038
};
