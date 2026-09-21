//
// Created by YUMBUL on 13.10.2023.
//

#include "IFItemLockSlot.h"
#include "TextStringManager.h"
#include <GInterface.h>
#include <GlobalDataManager.h>

GFX_IMPLEMENT_DYNCREATE(CIFItemLockSlot, CIFWnd)

GFX_BEGIN_MESSAGE_MAP(CIFItemLockSlot, CIFWnd)

GFX_END_MESSAGE_MAP()


CIFItemLockSlot::CIFItemLockSlot(void) {

}

CIFItemLockSlot::~CIFItemLockSlot(void) {

}

bool CIFItemLockSlot::OnCreate(long ln) {
    CIFWnd::OnCreate(ln);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifmacroslot.txt");
    m_IRM.CreateInterfaceSection("Create", this);


    RECT m_pSlotRect = { this->GetPos().x , this->GetPos().y, 32, 32 };
    m_pMySlot = m_IRM.GetResObj<CIFSlotWithHelpEx>(1, 0);
    m_pMySlot->m_pSlot->SetSlotData(NULL);
    m_pMySlot->m_pSlot->SetSlotType(0);
    m_pMySlot->m_pSlot->SetInventorySlotType(0);

    m_pMySlot->MoveGWnd(this->GetPos().x, this->GetPos().y);
    this->ShowGWnd(true);
    m_pMySlot->m_pSlot->BringToFront();

    return true;
}
#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1 : 0)
void CIFItemLockSlot::ClearSlot() {
    m_pMySlot->m_pSlot->SetSlotData(NULL);
    m_pMySlot->m_pSlot->SetSlotType(0);
    m_pMySlot->m_pSlot->SetInventorySlotType(0);
    m_pMySlot->m_pSlot->TB_Func_13("", 0, 0);
    //SaveSlot(this->m_pMySlot->m_pSlot->GetSlotType());
}

void CIFItemLockSlot::OnUpdate() {
    if(m_pMySlot != NULL)
    {
        if(m_pMySlot->m_pSlot != NULL)
        {
            m_pMySlot->m_pSlot->ShowGWnd(true);
            m_pMySlot->m_pSlot->BringToFront();
        }
    }
    //if (KEY_DOWN(VK_LBUTTON)) {
    //    if (g_CurrentIfUnderCursor->UniqueID() == g_pCGInterface->UniqueID()) {
    //        if (m_pMySlot->m_pSlot && g_CurrentIfUnderCursor->GethgWnd() != this->GethgWnd())
    //            m_pMySlot->m_pSlot = NULL;
    //        if (m_pMySlot->m_pSlot) {
    //            ClearSlot();
    //            m_pMySlot->m_pSlot = NULL;
    //        }
    //    } else {
    //        if (!strcmp(g_CurrentIfUnderCursor->GetRuntimeClass()->m_lpszClassName, "CIFSlotWithHelp")) {
    //            CIFSlotWithHelp *CurSlot = (CIFSlotWithHelp *) g_CurrentIfUnderCursor;
    //            if (CurSlot->Type == 0x46 || CurSlot->Type == 0x47 || CurSlot->Type == 0x4A || CurSlot->Type == 0x28 || CurSlot->Type == 0x19 || CurSlot->Type == 0x49 || CurSlot->Type == 0x4E && CurSlot->ItemInfo) {
    //                m_pMySlot->m_pSlot = CurSlot;
    //            }
    //            if (CurSlot->Type == 0xC && CurSlot->GethgWnd() == m_pMySlot->m_pSlot->GethgWnd()) {
    //                m_pMySlot->m_pSlot = CurSlot;
    //            }
    //        }
    //    }
    //}
}
void CIFItemLockSlot::LoadSlot(int SlotSetq, int SlotType, int Data) {
    m_pMySlot->m_pSlot->SetSlot(SlotSetq);
    m_pMySlot->m_pSlot->SetSlotType(SlotType);
    m_pMySlot->m_pSlot->GetIconSlot(SlotType, Data);

}


void CIFItemLockSlot::UseItem(int Slot) {

}
void CIFItemLockSlot::SendPacketMoveItem(BYTE SLot, BYTE SlotTo) {
    NEWMSG(0x7034)
        pReq << BYTE(0x00) << BYTE(SLot + 0xD) << BYTE(SlotTo) << UINT16(0x00);
    SENDMSG()
}
void CIFItemLockSlot::Func_26(int a1) {
    //printf("%p \n", a1);
    CIFSlotWithHelp *pWnd = (CIFSlotWithHelp *) a1;
    if (pWnd != (CIFSlotWithHelp*) 0x0) {
        pWnd->ClearSlot();
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

int CIFItemLockSlot::OnMouseLeftUp(int a1, int x, int y)
{
    //printf("a1 a2 a3 %d %d %d \n", a1, x, y);
    return this->OnMouseLeftUp(a1, x, y);
}

int CIFItemLockSlot::OnMouseLeftDown(int a1, int x, int y) {
    //printf("a1 a2 a3 %d %d %d \n", a1, x, y);
    return this->OnMouseLeftDown(a1, x, y);
}

bool CIFItemLockSlot::Func_28(CGWnd* a1, int a2, int a3) {
    if (a2 == 0) {
        return true;
    }
    CIFSlotWithHelp *birakilanslot = (CIFSlotWithHelp *) a1;
    int SlotType = birakilanslot->GetParentWindowId();
    switch (SlotType) {
        case 70:
        {
            if(birakilanslot->ItemInfo->GetItemData()->m_typeId.m_type_id_value != 0xCEED &&
               (birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID1() != 3 || birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID2() != 3)
               && birakilanslot->ItemInfo->ItemLocked != 1)
            {
                m_pMySlot->m_pSlot->CopySlot(birakilanslot);
            }
            else
            {
                g_pCGInterface->ShowMessage_Warning(TSM_GETTEXTPTR(L"LEXA_ITEM_LOCK_IMPOSSIBLE"));
            }

        } break;
        default:
            break;
    }
    return true;
}
void CIFItemLockSlot::RenderMyself(){
    CIFSlotWithHelp *pWnd = (CIFSlotWithHelp *) GetWndByGID(m_nMagicCubeSlotGID);
    if (pWnd != NULL)
        pWnd->Func_24(this->GetPos().x, this->GetPos().y);
}

void CIFItemLockSlot::UpdatehgWndSlot(CIFSlotWithHelp *pSlot) {
    if (pSlot == NULL) {
        m_nMagicCubeSlotGID = 0;
        return;
    }
    m_nMagicCubeSlotGID = pSlot->GethgWnd();
    pSlot->ShowGWnd(true);
}
