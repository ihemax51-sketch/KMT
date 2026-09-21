//
// Created by YUMBUL on 13.10.2023.
//

#include "IFMacroSlot.h"
#include <GInterface.h>
#include <GlobalDataManager.h>

GFX_IMPLEMENT_DYNCREATE(CIFMacroSlot, CIFWnd)

GFX_BEGIN_MESSAGE_MAP(CIFMacroSlot, CIFWnd)

GFX_END_MESSAGE_MAP()


CIFMacroSlot::CIFMacroSlot(void) {

}

CIFMacroSlot::~CIFMacroSlot(void) {

}

bool CIFMacroSlot::OnCreate(long ln) {
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
void CIFMacroSlot::ClearSlot() {
    m_pMySlot->m_pSlot->SetSlotData(NULL);
    m_pMySlot->m_pSlot->SetSlotType(0);
    m_pMySlot->m_pSlot->SetInventorySlotType(0);
    m_pMySlot->m_pSlot->TB_Func_13("", 0, 0);
}

void CIFMacroSlot::OnUpdate() {
    if(m_pMySlot != NULL)
    {
        if(m_pMySlot->m_pSlot != NULL)
        {
            m_pMySlot->m_pSlot->ShowGWnd(true);
            m_pMySlot->m_pSlot->BringToFront();
        }
    }
}
void CIFMacroSlot::LoadSlot(int SlotSetq, int SlotType, int Data) {
    m_pMySlot->m_pSlot->SetSlot(SlotSetq);
    m_pMySlot->m_pSlot->GetIconSlot(SlotType, Data);
    m_pMySlot->m_pSlot->SetInventorySlotType(Data);
}


void CIFMacroSlot::UseItem(int Slot) {
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
void CIFMacroSlot::SendPacketMoveItem(BYTE SLot, BYTE SlotTo) {
    NEWMSG(0x7034)
        pReq << BYTE(0x00) << BYTE(SLot + 0xD) << BYTE(SlotTo) << UINT16(0x00);
    SENDMSG()
}
void CIFMacroSlot::Func_26(int a1) {
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


bool CIFMacroSlot::Func_28(CGWnd* a1, int a2, int a3) {
    if (a2 == 0) {
        return true;
    }
    CIFSlotWithHelp *birakilanslot = (CIFSlotWithHelp *) a1;
    int SlotType = birakilanslot->GetParentWindowId();
    switch (SlotType) {
        case 0x46:
        {

            if(this->m_pMySlot->m_pSlot->GetSlot() == 141)
            {
                if(birakilanslot->ItemInfo->GetItemData()->IsHPPotion())
                {
                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);

                    SaveSlotData(m_pMySlot->m_pSlot);
                }

            }
            else if(this->m_pMySlot->m_pSlot->GetSlot() == 142)
            {
                if(birakilanslot->ItemInfo->GetItemData()->IsMPPotion())
                {
                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);

                    SaveSlotData(m_pMySlot->m_pSlot);
                }

            }
            else if(this->m_pMySlot->m_pSlot->GetSlot() == 143)
            {
                if(birakilanslot->ItemInfo->GetItemData()->IsPill())
                {
                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);

                    SaveSlotData(m_pMySlot->m_pSlot);
                }

            }
            else if (this->m_pMySlot->m_pSlot->GetSlot() == 144) {

                if (birakilanslot->ItemInfo->GetItemData()->IsPuriPill()) {
                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);

                    SaveSlotData(m_pMySlot->m_pSlot);
                }
            }
            else if(this->m_pMySlot->m_pSlot->GetSlot() == 145)
            {

                if(birakilanslot->ItemInfo->GetItemData()->m_desc1_128.find(L"SPEED") != std::n_wstring::npos)
                {
                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);
                    SaveSlotData(m_pMySlot->m_pSlot);
                }

            }
            else if (this->m_pMySlot->m_pSlot->GetSlot() == 146) {

                if (birakilanslot->ItemInfo->GetItemData()->IsVIGOR()) {
                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);

                    SaveSlotData(m_pMySlot->m_pSlot);
                }
            }
            else if (this->m_pMySlot->m_pSlot->GetSlot() == 147) {

                if (birakilanslot->ItemInfo->GetItemData()->IsVIGOR()) {
                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);

                    SaveSlotData(m_pMySlot->m_pSlot);
                }
            }
            else if (this->m_pMySlot->m_pSlot->GetSlot() == 148) {

                if (birakilanslot->ItemInfo->GetItemData()->IsPETHpPotion()) {
                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);

                    SaveSlotData(m_pMySlot->m_pSlot);
                }
            }
            else if (this->m_pMySlot->m_pSlot->GetSlot() == 149) {

                if (birakilanslot->ItemInfo->GetItemData()->IsPETMpPotion()) {
                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);

                    SaveSlotData(m_pMySlot->m_pSlot);
                }
            }
            else if (this->m_pMySlot->m_pSlot->GetSlot() == 150) {

                if (birakilanslot->ItemInfo->GetItemData()->IsCureCOS()) {
                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);

                    SaveSlotData(m_pMySlot->m_pSlot);
                }
            }
            else if (this->m_pMySlot->m_pSlot->GetSlot() == 151) {

                if (birakilanslot->ItemInfo->GetItemData()->IsPetRes()) {
                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);

                    SaveSlotData(m_pMySlot->m_pSlot);
                }
            }
            else if (this->m_pMySlot->m_pSlot->GetSlot() == 152) {

                if (birakilanslot->ItemInfo->GetItemData()->IsAttackPet()) {
                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);

                    SaveSlotData(m_pMySlot->m_pSlot);
                }
            }
        } break;
        default:
            break;
    }
    return true;
}
void CIFMacroSlot::RenderMyself(){
    CIFSlotWithHelp *pWnd = (CIFSlotWithHelp *) GetWndByGID(m_nMagicCubeSlotGID);
    if (pWnd != NULL)
        pWnd->Func_24(this->GetPos().x, this->GetPos().y);
}

void CIFMacroSlot::UpdatehgWndSlot(CIFSlotWithHelp *pSlot) {
    if (pSlot == NULL) {
        m_nMagicCubeSlotGID = 0;
        return;
    }
    m_nMagicCubeSlotGID = pSlot->GethgWnd();
    pSlot->ShowGWnd(true);
}

void CIFMacroSlot::SaveSlotData(CIFSlotWithHelp* Slot) {
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
