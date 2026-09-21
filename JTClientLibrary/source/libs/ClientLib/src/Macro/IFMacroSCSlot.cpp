//
// Created by YUMBUL on 13.10.2023.
//

#include "IFMacroSCSlot.h"
#include <GInterface.h>
#include <GlobalDataManager.h>

GFX_IMPLEMENT_DYNCREATE(CIFMacroSCSlot, CIFWnd)

GFX_BEGIN_MESSAGE_MAP(CIFMacroSCSlot, CIFWnd)

GFX_END_MESSAGE_MAP()


CIFMacroSCSlot::CIFMacroSCSlot(void) {

}

CIFMacroSCSlot::~CIFMacroSCSlot(void) {

}

bool CIFMacroSCSlot::OnCreate(long ln) {
    CIFWnd::OnCreate(ln);
    RECT m_pSlotRect = { this->GetPos().x , this->GetPos().y, 32, 32 };
    m_pMySlot = (CIFSlotWithHelpEx*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSlotWithHelpEx), m_pSlotRect, this->UniqueID(), 0);
    m_pMySlot->m_pSlot->SetSlotData(NULL);
    m_pMySlot->m_pSlot->SetSlotType(0);
    m_pMySlot->m_pSlot->SetInventorySlotType(0);

    m_pMySlot->MoveGWnd(this->GetPos().x, this->GetPos().y);
    m_pMySlot->m_pSlot->BringToFront();
    this->ShowGWnd(true);
    return true;
}
#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1 : 0)
void CIFMacroSCSlot::ClearSlot() {
    m_pMySlot->m_pSlot->SetSlotData(NULL);
    m_pMySlot->m_pSlot->SetSlotType(0);
    m_pMySlot->m_pSlot->SetInventorySlotType(0);
    m_pMySlot->m_pSlot->TB_Func_13("", 0, 0);
}

void CIFMacroSCSlot::OnUpdate() {

}
void CIFMacroSCSlot::LoadSlot(int SlotSetq, int SlotType, int Data) {
    m_pMySlot->m_pSlot->SetSlot(SlotSetq);
    m_pMySlot->m_pSlot->GetIconSlot(SlotType, Data);
    m_pMySlot->m_pSlot->SetInventorySlotType(Data);
}


void CIFMacroSCSlot::UseItem(int Slot) {
    if(this->m_pMySlot->m_pSlot->ItemInfo != NULL)
    {
        CIFMainPopup *popup = g_pCGInterface->GetMainPopup();
        CIFInventory *inventory = popup->GetInventory();

        CSOItem *item = inventory->GetItemBySlot(Slot);

        if (item->m_blValid == 0) {
            return;
        }

        const SItemData *pItemData = item->GetItemData();
        NEWMSG(0x704C)
            pReq << (BYTE)(Slot+13) << (UINT16)(pItemData->m_typeId.m_type_id_value);
        SENDMSG()
    }
}
void CIFMacroSCSlot::SendPacketMoveItem(BYTE SLot, BYTE SlotTo) {
    NEWMSG(0x7034)
        pReq << BYTE(0x00) << BYTE(SLot + 0xD) << BYTE(SlotTo) << UINT16(0x00);
    SENDMSG()
}
void CIFMacroSCSlot::Func_26(int a1) {
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


bool CIFMacroSCSlot::Func_28(CGWnd* a1, int a2, int a3) {
    if (a2 == 0) {
        return true;
    }
    CIFSlotWithHelp *birakilanslot = (CIFSlotWithHelp *) a1;
    if (!birakilanslot || !birakilanslot->ItemInfo ||
        !birakilanslot->ItemInfo->GetItemData())
        return false;

    int SlotType = birakilanslot->GetParentWindowId();
    switch (SlotType) {
        case 0x46:
        {

            int typeID4 = birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID4();

            if (birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID1() == 3 &&
                birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID2() == 3 &&
                birakilanslot->ItemInfo->GetItemData()->m_typeId.getTypeID3() == 13 &&
                typeID4 != 4 && typeID4 != 5 &&
                typeID4 != 14 && typeID4 != 8 &&
                typeID4 != 7 && typeID4 != 9 &&
                typeID4 != 12 && typeID4 != 13 &&
                typeID4 != 15)
            {
                m_pMySlot->m_pSlot->CopySlot(birakilanslot);
                SaveSlotData(m_pMySlot->m_pSlot);
            }
            else if(birakilanslot->ItemInfo->GetItemData()->IsZerkScroll())
            {
                m_pMySlot->m_pSlot->CopySlot(birakilanslot);
                SaveSlotData(m_pMySlot->m_pSlot);
            }
        } break;
        default:
            break;
    }
    return true;
}
void CIFMacroSCSlot::RenderMyself(){
    CIFSlotWithHelp *pWnd = (CIFSlotWithHelp *) GetWndByGID(m_nMagicCubeSlotGID);
    if (pWnd != NULL)
        pWnd->Func_24(this->GetPos().x, this->GetPos().y);
}

void CIFMacroSCSlot::UpdatehgWndSlot(CIFSlotWithHelp *pSlot) {
    if (pSlot == NULL) {
        m_nMagicCubeSlotGID = 0;
        return;
    }
    m_nMagicCubeSlotGID = pSlot->GethgWnd();
    pSlot->ShowGWnd(true);
}

void CIFMacroSCSlot::SaveSlotData(CIFSlotWithHelp* Slot) {
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
