//
// Created by YUMBUL on 13.10.2023.
//

#include "IFItemTranslationSlot.h"
#include "IFItemTranslationWnd.h"
#include <GInterface.h>
#include <GlobalDataManager.h>

GFX_IMPLEMENT_DYNCREATE(CIFItemTranslationSlot, CIFWnd)

GFX_BEGIN_MESSAGE_MAP(CIFItemTranslationSlot, CIFWnd)

GFX_END_MESSAGE_MAP()


CIFItemTranslationSlot::CIFItemTranslationSlot(void) {

}

CIFItemTranslationSlot::~CIFItemTranslationSlot(void) {

}

bool CIFItemTranslationSlot::OnCreate(long ln) {
    CIFWnd::OnCreate(ln);
    RECT m_pSlotRect = { this->GetPos().x , this->GetPos().y, 32, 32 };
    m_pMySlot = (CIFSlotWithHelpEx*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSlotWithHelpEx), m_pSlotRect, this->UniqueID(), 0);
    m_pMySlot->m_pSlot->SetSlotData(NULL);
    m_pMySlot->m_pSlot->SetSlotType(0);
    m_pMySlot->m_pSlot->SetInventorySlotType(0);


    copyslot = (CIFSlotWithHelp *) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSlotWithHelp), m_pSlotRect, 950, 0);
    copyslot->ShowGWnd(false);
    copyslot->SetType(0xC);
    m_pMySlot->MoveGWnd(this->GetPos().x, this->GetPos().y);
    m_pMySlot->m_pSlot->BringToFront();
    this->ShowGWnd(true);
    return true;
}
#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1 : 0)
void CIFItemTranslationSlot::ClearSlot() {
    m_pMySlot->m_pSlot->SetSlotData(NULL);
    m_pMySlot->m_pSlot->SetSlotType(0);
    m_pMySlot->m_pSlot->SetInventorySlotType(0);
    m_pMySlot->m_pSlot->TB_Func_13("", 0, 0);

}

void CIFItemTranslationSlot::OnUpdate() {

}
void CIFItemTranslationSlot::LoadSlot(int SlotSetq, int SlotType, int Data) {
    m_pMySlot->m_pSlot->SetSlot(SlotSetq);
    m_pMySlot->m_pSlot->SetSlotType(SlotType);
    m_pMySlot->m_pSlot->GetIconSlot(SlotType, Data);

}

void CIFItemTranslationSlot::Func_26(int a1) {
    //printf("%p \n", a1);
    CIFSlotWithHelp *pWnd = (CIFSlotWithHelp *) a1;
    if (pWnd != (CIFSlotWithHelp*) 0x0) {
        pWnd->ClearSlot();
        g_pCGInterface->m_IRM.GetResObj<CIFItemTranslationWnd>(1361, 1)->ClearTexts();
    }
}

//bool CIFMySlot::Func_25(int a1) {
//    //printf("%p \n", a1);
//    //CIFSlotWithHelp *pWnd = (CIFSlotWithHelp *) a1;
//    //if (pWnd == (CIFSlotWithHelp *) 0x0) {
//    //    ClearSlot();
//    //}
//    return false;
//}
bool CIFItemTranslationSlot::IsChineseWeapon(TypeId TID)
{
    return false;
}
bool CIFItemTranslationSlot::Func_28(CGWnd* a1, int a2, int a3) {
    if (a2 == 0) {
        return true;
    }
    CIFSlotWithHelp *birakilanslot = (CIFSlotWithHelp *) a1;
    int SlotType = birakilanslot->GetParentWindowId();
    switch (SlotType) {
        case 0x46:
        {
            if(this->m_pMySlot->m_pSlot->GetSlot() == 510)
            {
                if(birakilanslot->ItemInfo->GetItemData()->CanTrade != 0  && birakilanslot->ItemInfo->GetItemData()->CanBorrow != 0 && birakilanslot->ItemInfo->GetItemData()->CanDrop != 0)
                {
                    if(birakilanslot->ItemInfo->GetItemData()->IsChSword() || birakilanslot->ItemInfo->GetItemData()->IsChBlade() ||
                       birakilanslot->ItemInfo->GetItemData()->IsChSpear() || birakilanslot->ItemInfo->GetItemData()->IsChTBlade() ||
                       birakilanslot->ItemInfo->GetItemData()->IsChBow())
                    {
                        m_pMySlot->m_pSlot->CopySlot(birakilanslot);
                        g_pCGInterface->m_IRM.GetResObj<CIFItemTranslationWnd>(1361, 1)->ChineseWeaponIsPuttedSlot();
                    }
                    else if(birakilanslot->ItemInfo->GetItemData()->IsEuSword() || birakilanslot->ItemInfo->GetItemData()->IsEuTSword()
                            || birakilanslot->ItemInfo->GetItemData()->IsEuAxe() || birakilanslot->ItemInfo->GetItemData()->IsEuDarkStaff()
                            || birakilanslot->ItemInfo->GetItemData()->IsEuTStaff() || birakilanslot->ItemInfo->GetItemData()->IsEuCrossbow()
                            || birakilanslot->ItemInfo->GetItemData()->IsEuDagger() || birakilanslot->ItemInfo->GetItemData()->IsEuHarp()
                            || birakilanslot->ItemInfo->GetItemData()->IsEuStaff())
                    {
                        m_pMySlot->m_pSlot->CopySlot(birakilanslot);
                        g_pCGInterface->m_IRM.GetResObj<CIFItemTranslationWnd>(1361, 1)->EuropeWeaponIsPuttedSlot();
                    }
                    else
                    {
                        g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_EXCHANGE_FUNCTION_IS_WORK_ONLY_WEAPONS"));
                    }
                }
                else
                {
                    g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_EXCHANGE_FUNCTION_IS_NOT_WORK_FOR_THIS_ITEM"));
                }

            }
        }
        break;
        default:
            break;
    }


    return true;
}
void CIFItemTranslationSlot::RenderMyself(){
    CIFSlotWithHelp *pWnd = (CIFSlotWithHelp *) GetWndByGID(m_nMagicCubeSlotGID);
    if (pWnd != NULL)
        pWnd->Func_24(this->GetPos().x, this->GetPos().y);
}

void CIFItemTranslationSlot::UpdatehgWndSlot(CIFSlotWithHelp *pSlot) {
    if (pSlot == NULL) {
        m_nMagicCubeSlotGID = 0;
        return;
    }
    m_nMagicCubeSlotGID = pSlot->GethgWnd();
    pSlot->ShowGWnd(true);
}
