//
// Created by YUMBUL on 13.10.2023.
//

#include "IFMySlot.h"
#include <GInterface.h>
#include <GlobalDataManager.h>

GFX_IMPLEMENT_DYNCREATE(CIFMySlot, CIFWnd)

GFX_BEGIN_MESSAGE_MAP(CIFMySlot, CIFWnd)

GFX_END_MESSAGE_MAP()


CIFMySlot::CIFMySlot(void) {

}

CIFMySlot::~CIFMySlot(void) {

}

bool CIFMySlot::OnCreate(long ln) {
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
    this->ShowGWnd(true);
    return true;
}
#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1 : 0)
void CIFMySlot::ClearSlot() {
    m_pMySlot->m_pSlot->SetSlotData(NULL);
    m_pMySlot->m_pSlot->SetSlotType(0);
    m_pMySlot->m_pSlot->SetInventorySlotType(0);
    m_pMySlot->m_pSlot->TB_Func_13("", 0, 0);
    //SaveSlot(this->m_pMySlot->m_pSlot->GetSlotType());
}

void CIFMySlot::OnUpdate() {
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
void CIFMySlot::LoadSlot(int SlotSetq, int SlotType, int Data) {
    m_pMySlot->m_pSlot->SetSlot(SlotSetq);
    m_pMySlot->m_pSlot->SetSlotType(SlotType);
    m_pMySlot->m_pSlot->GetIconSlot(SlotType, Data);

}


void CIFMySlot::UseItem(int Slot) {
    if (m_pMySlot->m_pSlot->GetSlotType() != 0x47) {
        if (m_pMySlot->m_pSlot->GetSlotType() == 0x46) {
            int TID = g_CGlobalDataManager->GetItem(m_pMySlot->m_pSlot->ItemInfo->m_refObjItemId)->GetData().m_typeId.m_type_id_value;
            if (TID == 25388 || TID == 4908 || TID == 6956 || TID == 9004 || TID == 11052 || TID == 13100 || TID == 21292 || TID == 31532 || TID == 17196 || TID == 15148 || TID == 23340 || TID == 27436 || TID == 19244 || TID == 29484)
                SendPacketMoveItem(m_pMySlot->m_pSlot->GetInventorySlotType(), 6);
            else if (TID == 2604 || TID == 4652)
                SendPacketMoveItem(m_pMySlot->m_pSlot->GetInventorySlotType(), 7);
            else {
                m_pMySlot->m_pSlot->SetSlot(m_pMySlot->m_pSlot->GetInventorySlotType());
                m_pMySlot->m_pSlot->SetType(m_pMySlot->m_pSlot->GetSlotType());
                m_pMySlot->m_pSlot->FUN_00682040();
            }

        } else if (m_pMySlot->m_pSlot->GetSlotType() == 0x4E) {
            m_pMySlot->m_pSlot->SetSlot(m_pMySlot->m_pSlot->GetInventorySlotType());
            m_pMySlot->m_pSlot->SetType(m_pMySlot->m_pSlot->GetSlotType());
            m_pMySlot->m_pSlot->FUN_00682040();

        } else {
            m_pMySlot->m_pSlot->SetType(m_pMySlot->m_pSlot->GetSlotType());
            m_pMySlot->m_pSlot->SetSlot(m_pMySlot->m_pSlot->m_SkillID);
            m_pMySlot->m_pSlot->FUN_00682040();
        }
        m_pMySlot->m_pSlot->SetType(0xC);
        m_pMySlot->m_pSlot->SetSlot(Slot);
    }
}
void CIFMySlot::SendPacketMoveItem(BYTE SLot, BYTE SlotTo) {
    NEWMSG(0x7034)
        pReq << BYTE(0x00) << BYTE(SLot + 0xD) << BYTE(SlotTo) << UINT16(0x00);
    SENDMSG()
}
void CIFMySlot::Func_26(int a1) {
    //printf("%p \n", a1);
    CIFSlotWithHelp *pWnd = (CIFSlotWithHelp *) a1;
    if (pWnd != (CIFSlotWithHelp*) 0x0) {
        pWnd->ClearSlot();
        SaveSlotData(pWnd);
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

int CIFMySlot::OnMouseLeftUp(int a1, int x, int y)
{
    //printf("a1 a2 a3 %d %d %d \n", a1, x, y);
    return this->OnMouseLeftUp(a1, x, y);
}

int CIFMySlot::OnMouseLeftDown(int a1, int x, int y) {
    //printf("a1 a2 a3 %d %d %d \n", a1, x, y);
    return this->OnMouseLeftDown(a1, x, y);
}

bool CIFMySlot::Func_28(CGWnd* a1, int a2, int a3) {
    if (a2 == 0) {
        return true;
    }
    CIFSlotWithHelp *birakilanslot = (CIFSlotWithHelp *) a1;
    int SlotType = birakilanslot->GetParentWindowId();
    if(birakilanslot->UniqueID() == 10007 || birakilanslot->UniqueID() == 100005
       || birakilanslot->UniqueID() == 100006)
    {
        return true;
    }

    switch (SlotType) {
        case 0xc:
        {
            CIFSlotWithHelp *hedefslot = (CIFSlotWithHelp *) a2;
            copyslot->SetSlot(hedefslot->GetSlot());
            copyslot->CopySlot(hedefslot);// hedef slot kopyalandı.

            hedefslot->CopySlot(birakilanslot); // mousenin tuttuğu slot hedefe aktarıldı.

            birakilanslot->ClearSlot(); // mouseninn tuttuğu slot temizlendi.

            SetDraggedGWnd(this); // wtf?

            birakilanslot->CopySlot(copyslot); // mousenin tuttuğu slota kopyalanan slot aktarıldı.

            SaveSlotData(hedefslot);
            SaveSlotData(birakilanslot);


        }
            break;
        case 0x25:
        case 25:
        case 0x46:
        case 0x47:
        case 0x49:
        case 0x4a:
        case 0x4e: {

            m_pMySlot->m_pSlot->CopySlot(birakilanslot);

            int SlotTypex = birakilanslot->GetParentWindowId();

            SaveSlotData(m_pMySlot->m_pSlot);

            if ((0x45 < SlotTypex) && ((SlotTypex < 0x48 || (SlotTypex == 0x4e))))
            {

            }
        } break;
        default:
            break;
    }


    return true;
}
void CIFMySlot::RenderMyself(){
    CIFSlotWithHelp *pWnd = (CIFSlotWithHelp *) GetWndByGID(m_nMagicCubeSlotGID);
    if (pWnd != NULL)
        pWnd->Func_24(this->GetPos().x, this->GetPos().y);
}

void CIFMySlot::UpdatehgWndSlot(CIFSlotWithHelp *pSlot) {
    if (pSlot == NULL) {
        m_nMagicCubeSlotGID = 0;
        return;
    }
    m_nMagicCubeSlotGID = pSlot->GethgWnd();
    pSlot->ShowGWnd(true);
}

void CIFMySlot::SaveSlotData(CIFSlotWithHelp* Slot) {
    if (Slot->GetType() == 0xC) {
        CMsgStreamBuffer buf(0x7158);
        //printf("%d %d %d %d\n", Slot->GetSlotType(), Slot, m_pMySlot->m_pSlot->GetInventorySlotType(), m_pMySlot->m_pSlot->GetSkillSlotInDex());
        switch (Slot->GetSlotType()) {
            case 0x25:
            case 0x46:
            case 0x47:
            case 0x4a:
            case 0x4e: {
                buf << BYTE(0x01) << BYTE(Slot->GetSlot()) << BYTE(Slot->GetSlotType()) << UINT32(Slot->GetInventorySlotType());
                SendMsg(buf);
            } break;
            case 0x49:
            {
                buf << BYTE(0x01) << BYTE(Slot->GetSlot()) << BYTE(Slot->GetSlotType()) << UINT32(Slot->GetSkillSlotInDex());
                SendMsg(buf);
            }
                break;
            case 0:
            {
                buf << BYTE(0x01) << BYTE(Slot->GetSlot()) << BYTE(0x0) << UINT32(0x00);
                SendMsg(buf);
            } break;
        }
    }
}