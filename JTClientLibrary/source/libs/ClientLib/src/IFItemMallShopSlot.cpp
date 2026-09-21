#include "IFItemMallShopSlot.h"
#include "IFButton.h"
GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFItemMallShopSlot, 0x00eec488)

CItemMallData *CIFItemMallShopSlot::GetItemMallData() const {
    return m_pItemMallData;
}

void CIFItemMallShopSlot::OnConfirmBuySectionControl(bool bCreate) {
    reinterpret_cast<void(__thiscall *)(CIFItemMallShopSlot *, bool)>(0x007CF210)(this, bCreate);
}


bool CIFItemMallShopSlot::OnCreateIMPL(long ln) {
    return reinterpret_cast<bool (__thiscall *)(CIFItemMallShopSlot *, long)>(0x007ceea0)(this, ln);
}

void CIFItemMallShopSlot::OnUpdateIMPL(){
    reinterpret_cast<bool (__thiscall *)(CIFItemMallShopSlot *)>(0x006528a0)(this);
}

