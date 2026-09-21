//
// Created by YUMBUL on 13.10.2023.
//

#include "IFMacroSlotWep.h"
#include <GInterface.h>
#include <GlobalDataManager.h>

namespace
{
bool IsLiveInventoryItem(const CSOItem* item)
{
    return item != NULL && item->m_blValid != 0 && item->GetItemData() != NULL;
}

bool IsSameMacroEquipment(const CSOItem* configuredItem, const CSOItem* liveItem, bool exactMatch)
{
    if (!configuredItem || !IsLiveInventoryItem(liveItem) ||
        configuredItem->m_refObjItemId == 0 ||
        configuredItem->m_refObjItemId != liveItem->m_refObjItemId)
    {
        return false;
    }

    if (!exactMatch)
        return true;

    return configuredItem->m_OptLevel == liveItem->m_OptLevel &&
           configuredItem->Variance == liveItem->Variance;
}

int FindMacroEquipmentInventorySlot(CIFInventory* inventory,
                                    const CSOItem* configuredItem,
                                    int preferredSlot)
{
    if (!inventory || !configuredItem)
        return -1;

    const int slotCount = inventory->InventorySlotCount();
    if (preferredSlot >= 0 && preferredSlot < slotCount &&
        IsSameMacroEquipment(configuredItem, inventory->GetItemBySlot(preferredSlot), false))
    {
        return preferredSlot;
    }

    int compatibleSlot = -1;
    for (int slot = 0; slot < slotCount; ++slot)
    {
        CSOItem* liveItem = inventory->GetItemBySlot(slot);
        if (IsSameMacroEquipment(configuredItem, liveItem, true))
            return slot;

        if (compatibleSlot == -1 && IsSameMacroEquipment(configuredItem, liveItem, false))
            compatibleSlot = slot;
    }

    return compatibleSlot;
}
}

GFX_IMPLEMENT_DYNCREATE(CIFMacroSlotWep, CIFWnd)

GFX_BEGIN_MESSAGE_MAP(CIFMacroSlotWep, CIFWnd)

GFX_END_MESSAGE_MAP()


CIFMacroSlotWep::CIFMacroSlotWep(void) {

}

CIFMacroSlotWep::~CIFMacroSlotWep(void) {

}

bool CIFMacroSlotWep::OnCreate(long ln) {
    CIFWnd::OnCreate(ln);
    RECT m_pSlotRect = { this->GetPos().x , this->GetPos().y, 32, 32 };
    m_pMySlot = (CIFSlotWithHelpEx*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSlotWithHelpEx), m_pSlotRect, this->UniqueID(), 0);
    m_pMySlot->m_pSlot->SetUniqueID(10006);
    m_pMySlot->m_pSlot->SetSlotData(NULL);
    m_pMySlot->m_pSlot->SetSlotType(0);
    m_pMySlot->m_pSlot->SetInventorySlotType(0);

    m_pMySlot->MoveGWnd(this->GetPos().x, this->GetPos().y);
    m_pMySlot->m_pSlot->BringToFront();
    this->ShowGWnd(true);
    return true;
}
#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1 : 0)
void CIFMacroSlotWep::ClearSlot() {
    m_pMySlot->m_pSlot->SetSlotData(NULL);
    m_pMySlot->m_pSlot->SetSlotType(0);
    m_pMySlot->m_pSlot->SetInventorySlotType(0);
    m_pMySlot->m_pSlot->TB_Func_13("", 0, 0);
}

void CIFMacroSlotWep::OnUpdate() {
    if(m_pMySlot != NULL)
    {
        if(m_pMySlot->m_pSlot != NULL)
        {
            m_pMySlot->m_pSlot->ShowGWnd(true);
            m_pMySlot->m_pSlot->BringToFront();
        }
    }
}
void CIFMacroSlotWep::LoadSlot(int SlotSetq, int SlotType, int Data) {
    m_pMySlot->m_pSlot->SetSlot(SlotSetq);
    m_pMySlot->m_pSlot->GetIconSlot(SlotType, Data);
    m_pMySlot->m_pSlot->SetInventorySlotType(Data);
}


void CIFMacroSlotWep::UseItem(int Slot) {
    if (!m_pMySlot || !m_pMySlot->m_pSlot || !m_pMySlot->m_pSlot->ItemInfo)
        return;

    const SItemData* configuredData = m_pMySlot->m_pSlot->ItemInfo->GetItemData();
    if (!configuredData)
        return;

    if (m_pMySlot->m_pSlot->GetSlotType() == 0x46 ||
        m_pMySlot->m_pSlot->GetSlotType() == 0x47)
    {
        CIFMainPopup* popup = g_pCGInterface ? g_pCGInterface->GetMainPopup() : NULL;
        CIFInventory* inventory = popup ? popup->GetInventory() : NULL;
        const int inventorySlot = FindMacroEquipmentInventorySlot(
            inventory,
            m_pMySlot->m_pSlot->ItemInfo,
            m_pMySlot->m_pSlot->GetInventorySlotType());

        if (inventorySlot < 0)
            return;

        // Equipment swaps move the previously worn item into the source slot.
        // Keep the macro slot synchronized with the item's current live position.
        m_pMySlot->m_pSlot->SetInventorySlotType(inventorySlot);

        if (configuredData->m_typeId.getTypeID1() == 3 &&
            configuredData->m_typeId.getTypeID2() == 1 &&
            configuredData->m_typeId.getTypeID3() == 6)
        {
            SendPacketMoveItem((BYTE)inventorySlot, 6);
        }
        else if (configuredData->m_typeId.getTypeID1() == 3 &&
                 configuredData->m_typeId.getTypeID2() == 1 &&
                 configuredData->m_typeId.getTypeID3() == 4)
        {
            SendPacketMoveItem((BYTE)inventorySlot, 7);
        }
        return;
    }

    if (m_pMySlot->m_pSlot->GetSlotType() != 0x47) {
        if (m_pMySlot->m_pSlot->GetSlotType() == 0x4E) {
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
void CIFMacroSlotWep::SendPacketMoveItem(BYTE SLot, BYTE SlotTo) {
    NEWMSG(0x7034)
        pReq << BYTE(0x00) << BYTE(SLot + 0xD) << BYTE(SlotTo) << UINT16(0x00);
    SENDMSG()
}
void CIFMacroSlotWep::Func_26(int a1) {
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


bool CIFMacroSlotWep::Func_28(CGWnd* a1, int a2, int a3) {
    if (a2 == 0) {
        return true;
    }
    CIFSlotWithHelp *birakilanslot = (CIFSlotWithHelp *) a1;
    if (birakilanslot == NULL || birakilanslot->ItemInfo == NULL || birakilanslot->ItemInfo->GetItemData() == NULL) {
        return true;
    }

    const SItemData* itemData = birakilanslot->ItemInfo->GetItemData();
    int SlotType = birakilanslot->GetParentWindowId();
    switch (SlotType) {
        case 0x46:
        case 0x47:
        {
            if(this->m_pMySlot->m_pSlot->GetSlot() == 153 || this->m_pMySlot->m_pSlot->GetSlot() == 154)
            {
                if(itemData->m_typeId.getTypeID1() == 3
                   && itemData->m_typeId.getTypeID2() == 1
                   && itemData->m_typeId.getTypeID3() == 6)
                {
                    m_pMySlot->m_pSlot->CopySlot(birakilanslot);
                     SaveSlotData(m_pMySlot->m_pSlot);
                }

            }
            else if(this->m_pMySlot->m_pSlot->GetSlot() == 155 || this->m_pMySlot->m_pSlot->GetSlot() == 156)
            {
                if(itemData->m_typeId.getTypeID1() == 3
                   && itemData->m_typeId.getTypeID2() == 1
                   && itemData->m_typeId.getTypeID3() == 4)
                {
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
void CIFMacroSlotWep::RenderMyself(){
    CIFSlotWithHelp *pWnd = (CIFSlotWithHelp *) GetWndByGID(m_nMagicCubeSlotGID);
    if (pWnd != NULL)
        pWnd->Func_24(this->GetPos().x, this->GetPos().y);
}

void CIFMacroSlotWep::UpdatehgWndSlot(CIFSlotWithHelp *pSlot) {
    if (pSlot == NULL) {
        m_nMagicCubeSlotGID = 0;
        return;
    }
    m_nMagicCubeSlotGID = pSlot->GethgWnd();
    pSlot->ShowGWnd(true);
}

void CIFMacroSlotWep::SaveSlotData(CIFSlotWithHelp* Slot) {
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
