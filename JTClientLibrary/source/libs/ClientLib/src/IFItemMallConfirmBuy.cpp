//
// Created by maximus on 12/24/2023.
//

#include <CustomData/CustomSettingManager.h>
#include "IFItemMallConfirmBuy.h"
#include "BSLib/multibyte.h"
#include "GlobalDataManager.h"
#include "IFInventory.h"
#include "IFItemMallShopSlot.h"
#include "InterfaceNetSender.h"

CSOItemPackage *CIFItemMallConfirmBuy::GetPackageItem() const {
    return m_pSOPackageItem;
}

void CIFItemMallConfirmBuy::OnBuy_BtnClick() {
    if (!m_Settings->EnableOldItemMall) {
        reinterpret_cast<void (__thiscall *)(CIFItemMallConfirmBuy *)>(0x007c4880)(this);
        return;
    }

    CIFWnd *pParent = static_cast<CIFWnd *>(GetParentControl());

    // The renewed NPC shop embeds this confirmation control in CIFInventory.
    // OldItemMall hooks the shared Buy handler, so NPC purchases must be routed
    // to the original client implementation before applying any mall-specific
    // packet/index handling.
    if (pParent && pParent->IsSame(GFX_RUNTIME_CLASS(CIFInventory))) {
        reinterpret_cast<void (__thiscall *)(CIFItemMallConfirmBuy *)>(0x007c4880)(this);
        return;
    }

    if (!pParent || !pParent->IsSame(GFX_RUNTIME_CLASS(CIFItemMallShopSlot))) {
        reinterpret_cast<void (__thiscall *)(CIFItemMallConfirmBuy *)>(0x007c4880)(this);
        return;
    }

    CIFItemMallShopSlot *pSlot = static_cast<CIFItemMallShopSlot *>(pParent);
    CSOItemPackage *pPackage = GetPackageItem();
    if (!pSlot || !pPackage || !pPackage->GetPackageItemData() ||
        !pSlot->GetItemMallData() || !g_pCInterfaceNetSender) {
        return;
    }

    const CItemMallData *pItemMallData = pSlot->GetItemMallData();
    const int nRealIndex = GetRealItemIndex(
            pItemMallData->m_nShopId,
            pItemMallData->m_btTabIndex,
            pPackage);
    if (nRealIndex < 0 || nRealIndex > 0xFF) {
        return;
    }

    CStringData data;
    data.str = TO_NSTRING(pPackage->GetPackageItemData()->m_codeName128).c_str();

    const USHORT usCount = m_usCount > 0 ? m_usCount : 1;

    g_pCInterfaceNetSender->BuyItemMallItem(
            pPackage->GetPackageItemData()->m_id,
            pItemMallData->m_nShopId,
            pItemMallData->m_btTabIndex,
            static_cast<BYTE>(nRealIndex),
            usCount,
            0,
            m_nField7E8,
            m_nField7EC,
            data);

    pSlot->OnConfirmBuySectionControl(false);
}

int CIFItemMallConfirmBuy::GetRealItemIndex(int nShopId, int nTabIndex,
                                            CSOItemPackage *pPackageItem) {
    CRefShopdata *pShopData = g_CGlobalDataManager->m_refShopDataMap[nShopId];

    if (pShopData && !pShopData->m_vRefShopTabGroupData.empty()) {
        CRefShopTabGroupData *pTabGroupData = pShopData->m_vRefShopTabGroupData[0];

        if (pTabGroupData && nTabIndex >= 0 &&
            pTabGroupData->m_vRefShopTabData.size() > static_cast<size_t>(nTabIndex)) {
            CRefShopTabData *pTabData = pTabGroupData->m_vRefShopTabData[nTabIndex];

            if (pTabData) {
                for (size_t i = 0; i < pTabData->m_vPackageItems.size(); ++i) {
                    if (pTabData->m_vPackageItems[i] &&
                        pTabData->m_vPackageItems[i] == pPackageItem) {
                        return static_cast<int>(i);
                    }
                }
            }
        }
    }

    return -1;
}
