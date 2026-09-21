//
// Created by YUMBUL on 17.06.2024.
//
#include <BSLib/multibyte.h>
#include "IFItemMallZzim.h"
#include "GInterface.h"
#include "InterfaceNetSender.h"
#include "GlobalDataManager.h"


void CIFItemMallZzim::OnBuyAll_BtnClick() {
    if (!g_pCGInterface || g_pCGInterface->m_lstReservedItemMallData.empty()) {
        return;
    }

    if(g_CGlobalDataManager->GetEmptyInventorySlots() >=  g_pCGInterface->m_lstReservedItemMallData.size())
    {
        CIFItemMall* pItemMall = g_pCGInterface->GetItemMall();
        if (!pItemMall || !pItemMall->m_pMessageBox) {
            return;
        }

        m_pMessageBox = pItemMall->m_pMessageBox;

        if(!pItemMall->m_bIsMessageBoxCreated) {
            m_pMessageBox->SetMessageBoxParent(this);

            BringToFront();

            g_pCGInterface->sub_79A620();

            pItemMall->m_bIsMessageBoxCreated = true;

            int nTotalSilkPrice = 0;

            std::list<SReservedItemMallData>::const_iterator  it = g_pCGInterface->m_lstReservedItemMallData.begin();
            for(; it != g_pCGInterface->m_lstReservedItemMallData.end(); ++it) {
                CItemMallData *pItemData = (*it).pItemMallData;
                if (pItemData && pItemData->m_pSOPackage && pItemData->m_pSOPackage->GetPackageItemData()) {
                    nTotalSilkPrice += pItemData->m_pSOPackage->GetPackageItemData()->m_nSilkPrice;
                }
            }

            m_pMessageBox->CreateMessageBox(2);
            m_pMessageBox->ShowGWnd(true);
            m_pMessageBox->SetMessageBoxStyle(15);

            CIFStatic* pPriceStatic = m_pMessageBox->GetGuiFromList<CIFStatic>(2);
            if (pPriceStatic) {
                pPriceStatic->SetTextFormatted(L"%d",nTotalSilkPrice);
                pPriceStatic->m_FontTexture.SetColor(0xFFFFD953);
                pPriceStatic->MoveGWnd(pPriceStatic->GetPos().x - 30, pPriceStatic->GetPos().y + 4);
            }
        }
    }
}

void CIFItemMallZzim::OnBuyAllCallBack() {
    if (!g_pCGInterface) {
        return;
    }

    std::list<SReservedItemMallData>::const_iterator it = g_pCGInterface->m_lstReservedItemMallData.begin();

    if(it != g_pCGInterface->m_lstReservedItemMallData.end()) {
        CItemMallData *pItemData = it->pItemMallData;
        if (!pItemData || !pItemData->m_pSOPackage ||
            !pItemData->m_pSOPackage->GetPackageItemData() || !g_pCInterfaceNetSender) {
            return;
        }

        std::n_string strCodeName = TO_NSTRING(
                pItemData->m_pSOPackage->GetPackageItemData()->m_codeName128).c_str();

        CStringData data;
        data.str = strCodeName;

        const int nRealIndex = g_CGlobalDataManager->GetSOPackageIndexByShop(
                pItemData->m_nShopId,
                pItemData->m_btTabIndex,
                pItemData->m_pSOPackage);
        if (nRealIndex < 0 || nRealIndex > 0xFF) {
            return;
        }

        g_pCInterfaceNetSender->BuyItemMallItem(
                pItemData->m_pSOPackage->GetPackageItemData()->m_id,
                static_cast<unsigned short>(pItemData->m_nShopId),
                pItemData->m_btTabIndex,
                static_cast<BYTE>(nRealIndex),
                1,
                0,
                0,
                0,
                data);

        if (!g_pCGInterface->m_lstReservedItemMallData.empty()) {
            g_pCGInterface->m_lstReservedItemMallData.pop_front();
        }

        RefreshReservedItems();
    }

}

void CIFItemMallZzim::RefreshReservedItems() {
    reinterpret_cast<int(__thiscall *)(CIFItemMallZzim *, int)>(0x007D0EA0)(this, 0);
}
