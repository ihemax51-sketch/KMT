#pragma once

#include "Data/ItemMallData.h"
#include "IFWnd.h"
class CStringData {
public:
    std::n_string str;
};
class CIFItemMallShopSlot : public CIFWnd{
GFX_DECLARE_DYNAMIC_EXISTING(CIFItemMallShopSlot, 0x00eec488)
public:
    CItemMallData* GetItemMallData() const;
    void OnConfirmBuySectionControl(bool bCreate);
    bool OnCreateIMPL(long ln);

    void OnUpdateIMPL();
private:
    char pad_036C[4];
    CItemMallData* m_pItemMallData;

};
