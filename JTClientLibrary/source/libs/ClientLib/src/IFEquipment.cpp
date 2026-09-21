#include <CustomData/CustomSettingManager.h>
#include <ctime>
#include "IFEquipment.h"
#include "Game.h"
#include "GInterface.h"
#include "TextStringManager.h"
#include <support/hook.h>
#define WeaponSlot 106
#define ShieldSlot 107
#define HeadSlot 100
#define ChestSlot 101
#define LegSlot 104
#define ShoulderSlot 102
#define HandSlot 103
#define FootSlot 105
#define EarringSlot 106
#define NecklaceSlot 107
#define JobSlot 108
#define LeftRingSlot 111
#define RightRingSlot 112
#define GDR_BUTTON_AUTO_SORT_INVENTORY 300
#define GDR_BUTTON_AUTO_ORDER_INVENTORY 301

namespace {

struct EquipmentControlLayout {
    int id;
    int x;
    int y;
    int width;
    int height;
};

const EquipmentControlLayout NEW_EQUIPMENT_LAYOUT[] = {
        {9999, 154, 278, 100, 14}, {99999, 154, 292, 100, 14},
        {500, 98, 239, 212, 50}, {117, 18, 154, 32, 32},
        {300, 260, 314, 88, 26}, {301, 60, 314, 88, 26},
        {116, 358, 70, 32, 32}, {115, 358, 112, 32, 32},
        {114, 18, 112, 32, 32}, {113, 18, 70, 32, 32},
        {112, 358, 238, 32, 32}, {111, 18, 238, 32, 32},
        {110, 358, 196, 32, 32}, {109, 18, 196, 32, 32},
        {108, 358, 280, 32, 32}, {107, 358, 18, 32, 32},
        {106, 18, 18, 32, 32}, {105, 358, 154, 32, 32},
        {104, 18, 154, 32, 32}, {103, 358, 112, 32, 32},
        {102, 358, 70, 32, 32}, {101, 18, 112, 32, 32},
        {100, 18, 70, 32, 32}, {14, 16, 292, 40, 40},
        {13, 16, 292, 40, 40}, {12, 196, 319, 16, 16},
        {11, 212, 319, 28, 16}, {10, 168, 319, 28, 16},
        {5, 12, 12, 384, 326}, {4, 0, 0, 408, 350}
};

const EquipmentControlLayout LEGACY_EQUIPMENT_LAYOUT[] = {
        {9999, 50, 242, 74, 14}, {99999, 50, 256, 74, 14},
        {500, 0, 240, 177, 50}, {117, 5, 172, 32, 32},
        {300, 56, 300, 66, 24}, {301, 56, 272, 66, 24},
        {116, 141, 86, 32, 32}, {115, 141, 129, 32, 32},
        {114, 5, 129, 32, 32}, {113, 5, 86, 32, 32},
        {112, 141, 258, 32, 32}, {111, 5, 258, 32, 32},
        {110, 141, 215, 32, 32}, {109, 5, 215, 32, 32},
        {108, 141, 308, 32, 32}, {107, 134, 12, 32, 32},
        {106, 12, 12, 32, 32}, {105, 141, 172, 32, 32},
        {104, 5, 172, 32, 32}, {103, 141, 129, 32, 32},
        {102, 141, 86, 32, 32}, {101, 5, 129, 32, 32},
        {100, 5, 86, 32, 32}, {14, 1, 305, 40, 40},
        {13, 1, 305, 40, 40}, {12, 81, 327, 16, 16},
        {11, 96, 327, 28, 16}, {10, 54, 327, 28, 16},
        {5, 12, 12, 154, 331}, {4, 0, 0, 178, 355}
};

bool IsNewInventoryDesignEnabled()
{
    return m_Settings == NULL || m_Settings->NewInventoryDesign;
}

} // namespace

void ConfigureInventoryDesignPatches(bool enableNewDesign)
{
    static const int newEquipmentSlotPositions[13][2] = {
            {14, 66}, {14, 108}, {354, 66}, {354, 108}, {14, 150}, {354, 150},
            {6, 6}, {346, 6}, {354, 276}, {14, 192}, {354, 192}, {14, 234}, {354, 234}
    };
    static const int legacyEquipmentSlotPositions[13][2] = {
            {1, 82}, {1, 125}, {137, 82}, {137, 125}, {1, 168}, {137, 168},
            {0, 0}, {122, 0}, {137, 304}, {1, 211}, {137, 211}, {1, 254}, {137, 254}
    };
    static const int newAvatarSlotPositions[5][2] = {
            {14, 66}, {14, 108}, {354, 108}, {354, 66}, {14, 151}
    };
    static const int legacyAvatarSlotPositions[5][2] = {
            {1, 82}, {1, 125}, {137, 125}, {137, 82}, {1, 169}
    };

    const int (*equipmentSlots)[2] = enableNewDesign
            ? newEquipmentSlotPositions : legacyEquipmentSlotPositions;
    const int (*avatarSlots)[2] = enableNewDesign
            ? newAvatarSlotPositions : legacyAvatarSlotPositions;

    for (int slot = 0; slot < 13; ++slot) {
        replaceAddr(0x00da6e20 + (slot * 8), equipmentSlots[slot][0]);
        replaceAddr(0x00da6e24 + (slot * 8), equipmentSlots[slot][1]);
    }
    for (int slot = 0; slot < 5; ++slot) {
        replaceAddr(0x00da6df8 + (slot * 8), avatarSlots[slot][0]);
        replaceAddr(0x00da6dfc + (slot * 8), avatarSlots[slot][1]);
    }

    PatchMe(0x006ad8d2, enableNewDesign ? 0xee : 0xfe);
    PatchMe(0x006ad8d5, enableNewDesign ? 0xce : 0xdb);
}

GFX_MSGMAP* CIFEquipment::MessageMap(){
    static const GFX_MSGMAP_ENTRY skillBoardMessageEntries[] =
            {
                    {GFX_WM_COMMAND, 0, GDR_BUTTON_AUTO_SORT_INVENTORY, GDR_BUTTON_AUTO_SORT_INVENTORY, BSSig_u12, 0,
                            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFEquipment::AutoSort))},
                    {GFX_WM_COMMAND, 0, GDR_BUTTON_AUTO_ORDER_INVENTORY, GDR_BUTTON_AUTO_ORDER_INVENTORY, BSSig_u12, 0,
                            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFEquipment::AutoOrder))},
                    {0, 0, 0, 0, 0, 0, (GFX_PMSG)0, 0, 0, 0},
                    // Diğer özel mesaj girişleri buraya eklenebilir
            };

    static GFX_MSGMAP newmap =
            {
                    reinterpret_cast<const GFX_MSGMAP *>(0x00da6e88), skillBoardMessageEntries,
            };
    return &newmap;
}
time_t lastUpdate;
static bool s_startAutoOrder = false;
static bool s_autoOrderRunning = false;
static clock_t s_lastAutoOrderUpdate = 0;

static bool IsAutoOrderSlotAvailable(CIFSlotWithHelp* slot)
{
    return slot != 0 && slot->SlotisLocked == 0;
}

static bool IsAutoOrderSlotEmpty(CIFSlotWithHelp* slot)
{
    return IsAutoOrderSlotAvailable(slot) &&
           (slot->ItemInfo == 0 || slot->ItemInfo->m_blValid == 0 || slot->ItemInfo->GetItemData() == 0);
}

static bool HasAutoOrderMovableItem(CIFSlotWithHelp* slot)
{
    return IsAutoOrderSlotAvailable(slot) &&
           slot->ItemInfo != 0 &&
           slot->ItemInfo->m_blValid != 0 &&
           slot->ItemInfo->GetItemData() != 0 &&
           slot->ItemInfo->ItemLocked != 1;
}

static void StopAutoOrder()
{
    s_startAutoOrder = false;
    s_autoOrderRunning = false;
}

static void SendInventoryMove(BYTE sourceSlot, BYTE destinationSlot, UINT16 quantity)
{
    CMsgStreamBuffer buf(0x7034);
    buf << BYTE(0x00)
        << BYTE(sourceSlot + 13)
        << BYTE(destinationSlot + 13)
        << quantity;
    SendMsg(buf);
}

template <typename T>
T my_min(const T& a, const T& b) {
    return (a < b) ? a : b;
}
#define STOP_AUTOSORT() \
    m_Settings->AutoSortRunning = false; \
    m_Settings->StartAutoSort = false;
void CIFEquipment::OnUpdateIMPL()
{

    if (s_startAutoOrder && float(clock() - s_lastAutoOrderUpdate) / CLOCKS_PER_SEC > 0.2)
    {
        D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 0);
        CIFSystemMessage* systemmessage = g_pCGInterface->GetSystemMessageView();

        if (!s_autoOrderRunning) {
            s_autoOrderRunning = true;
            systemmessage->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_AUTO_ORDERING_STARTED"), 7, 7);
        }

        CIFInventory* Inventory = g_pCGInterface->GetMainPopup()->GetInventory();
        if (!Inventory)
        {
            StopAutoOrder();
            return;
        }

        std::n_vector<CIFSlotWithHelp*>& slots = Inventory->pSlots;
        int slotCount = Inventory->InventorySlotCount();
        if (slotCount > (int)slots.size())
            slotCount = (int)slots.size();

        int targetIndex = -1;
        for (int i = 0; i < slotCount; ++i)
        {
            if (IsAutoOrderSlotEmpty(slots[i]))
            {
                targetIndex = i;
                break;
            }
        }

        if (targetIndex != -1)
        {
            for (int i = targetIndex + 1; i < slotCount; ++i)
            {
                if (!HasAutoOrderMovableItem(slots[i]))
                    continue;

                int quantity = slots[i]->ItemInfo->GetQuantity();
                if (quantity <= 0)
                    quantity = 1;

                SendInventoryMove((BYTE)slots[i]->GetSlot(), (BYTE)slots[targetIndex]->GetSlot(), (UINT16)quantity);
                s_lastAutoOrderUpdate = clock();
                return;
            }
        }

        StopAutoOrder();
        color = D3DCOLOR_ARGB(255, 0, 255, 0);
        systemmessage->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_AUTO_ORDERING_FINISHED"), 7, 7);
        s_lastAutoOrderUpdate = clock();
    }

    if (m_Settings->StartAutoSort && float(clock() - lastUpdate) / CLOCKS_PER_SEC > 0.2)
    {
        bool finished = false;
        bool isPacketSent = false;

        D3DCOLOR color = D3DCOLOR_ARGB(255, 255, 255, 0);
        CIFSystemMessage* systemmessage = g_pCGInterface->GetSystemMessageView();

        if (!m_Settings->AutoSortRunning) {
            m_Settings->AutoSortRunning = true;
            systemmessage->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_AUTO_SORTING_STARTED"), 7, 7);
        }

        CIFInventory* Inventory = g_pCGInterface->GetMainPopup()->GetInventory();
        if (!Inventory)
        {
            STOP_AUTOSORT();
            return;
        }


        std::n_vector<CIFSlotWithHelp*>& slots = Inventory->pSlots;

        for (std::n_vector<CIFSlotWithHelp*>::iterator it = slots.begin(); it != slots.end(); ++it) {
            CIFSlotWithHelp* pSlot = *it;
            if (!pSlot || !pSlot->ItemInfo || !pSlot->ItemInfo->GetItemData()) continue;

            int itemId = pSlot->ItemInfo->GetItemData()->RefObjectId;
            int itemCount = pSlot->ItemInfo->GetQuantity();
            int maxCount = pSlot->ItemInfo->GetItemData()->m_maxStack;

            if (itemCount < maxCount) {
                for (std::n_vector<CIFSlotWithHelp*>::iterator jt = it + 1; jt != slots.end(); ++jt) {
                    CIFSlotWithHelp* pSlotj = *jt;
                    if (!pSlotj || !pSlotj->ItemInfo || !pSlotj->ItemInfo->GetItemData()) continue;

                    int itemIdj = pSlotj->ItemInfo->GetItemData()->RefObjectId;
                    int itemCountj = pSlotj->ItemInfo->GetQuantity();

                    if (itemIdj == itemId && itemCountj < maxCount) {
                        int countToMove = (itemCount < (maxCount - itemCountj)) ? itemCount : (maxCount - itemCountj);

                        CMsgStreamBuffer buf(0x7034);
                        buf << BYTE(0x0)
                            << BYTE(pSlotj->GetSlot() + 13)
                            << BYTE(pSlot->GetSlot() + 13)
                            << UINT16(countToMove);
                        SendMsg(buf);
                        isPacketSent = true;
                        break;
                    }
                }
            }

            if (isPacketSent)
                break;
        }

        if (!isPacketSent) {
            finished = true;
            STOP_AUTOSORT();
            color = D3DCOLOR_ARGB(255, 0, 255, 0);
            systemmessage->WriteMessage(255, color, KmtGetText(L"UIIT_KMT_AUTO_SORTING_FINISHED"), 7, 7);

        }

        lastUpdate = clock();
    }
    if (g_pCGInterface)
      {
          int ItemsPoint = 0;
          for (size_t i = 0; i < 13; i++)
          {
              CSOItem* pItem = g_pCGInterface->GetMainPopup()->GetEquipment()->GetEquipmentObjectBySlot(i);

              if (pItem != 0x0)
              {
                  ObjectData* pData = pItem->GetObjectData();
                  if (pData)
                      ItemsPoint += (pItem->m_OptLevel + pData->ReqLevel1 + (pData->Rarity * 3));
              }
          }
          wchar_t Priceb[255];
          swprintf_s(Priceb, L"%d", ItemsPoint);
          CIFStatic* itemPointValue = m_IRM.GetResObj<CIFStatic>(99999, 1);
          if (itemPointValue) {
              itemPointValue->SetText(Priceb);
          }
        }
    reinterpret_cast<void(__thiscall*)(CIFEquipment*)>(0x006ab6b0)(this);
}
bool CIFEquipment::OnCreateIMPL(long ln) {

    bool a = reinterpret_cast<bool(__thiscall*)(CIFEquipment*, long)>(0x006ac1c0)(this, ln);
    ApplyInventoryDesignLayout();

    const bool useNewDesign = IsNewInventoryDesignEnabled();
    CIFStatic* itemPointLabel = m_IRM.GetResObj<CIFStatic>(9999, 1);
    CIFStatic* itemPointValue = m_IRM.GetResObj<CIFStatic>(99999, 1);
    if (itemPointLabel) {
        itemPointLabel->m_FontTexture.sub_8B4750(7);
        itemPointLabel->m_FontTexture.SetColor(useNewDesign
                ? D3DCOLOR_ARGB(255, 239, 218, 164)
                : D3DCOLOR_ARGB(255, 255, 255, 255));
        itemPointLabel->SetText(useNewDesign ? KmtGetText(L"UIIT_KMT_ITEM_POWER") : KmtGetText(L"UIIT_KMT_ITEM_POINTS"));
        if (useNewDesign) {
            itemPointLabel->BringToFront();
        }
    }
    if (itemPointValue) {
        itemPointValue->m_FontTexture.sub_8B4750(7);
        itemPointValue->m_FontTexture.SetColor(useNewDesign
                ? D3DCOLOR_ARGB(255, 132, 225, 221)
                : D3DCOLOR_ARGB(255, 255, 255, 255));
        if (useNewDesign) {
            itemPointValue->BringToFront();
        }
    }
    if(m_Settings->AutoSortButton)
    {
        class CIFButton* autoSortButton = m_IRM.GetResObj<class CIFButton>(GDR_BUTTON_AUTO_SORT_INVENTORY, 1);
        if (autoSortButton != 0)
        {
            autoSortButton->ShowGWnd(true);
            autoSortButton->TB_Func_13(useNewDesign
                    ? "clientlibrary\\mall\\mall_pre_big_button.ddj"
                    : "interface\\ifcommon\\ifcommon\\com_blu_button.ddj", 1, 1);
            autoSortButton->SetText(KmtGetText(L"UIIT_KMT_AUTO_SORT"));
            autoSortButton->m_FontTexture.SetColor(useNewDesign
                    ? D3DCOLOR_ARGB(255, 245, 242, 232)
                    : D3DCOLOR_ARGB(255, 255, 255, 255));
            autoSortButton->BringToFront();

            class CIFButton* autoOrderButton = m_IRM.GetResObj<class CIFButton>(GDR_BUTTON_AUTO_ORDER_INVENTORY, 1);
            if (autoOrderButton == 0)
            {
                wnd_rect orderRect;
                orderRect.pos.x = useNewDesign ? 60 : 56;
                orderRect.pos.y = useNewDesign ? 314 : 272;
                orderRect.size.width = useNewDesign ? 88 : 66;
                orderRect.size.height = useNewDesign ? 26 : 24;

                autoOrderButton = (class CIFButton*)CGWnd::CreateInstance(
                        this,
                        GFX_RUNTIME_CLASS(CIFButton),
                        orderRect,
                        GDR_BUTTON_AUTO_ORDER_INVENTORY,
                        0);
            }

            if (autoOrderButton != 0)
            {
                autoOrderButton->TB_Func_13(useNewDesign
                        ? "clientlibrary\\mall\\mall_pre_big_button.ddj"
                        : "interface\\ifcommon\\ifcommon\\com_blu_button.ddj", 1, 1);
                autoOrderButton->SetText(KmtGetText(L"UIIT_KMT_AUTO_ORDER"));
                autoOrderButton->m_FontTexture.SetColor(useNewDesign
                        ? D3DCOLOR_ARGB(255, 245, 242, 232)
                        : D3DCOLOR_ARGB(255, 255, 255, 255));
                autoOrderButton->ShowGWnd(true);
                autoOrderButton->BringToFront();
            }
        }
    }
    return a;
}

void CIFEquipment::ApplyInventoryDesignLayout()
{
    const bool useNewDesign = IsNewInventoryDesignEnabled();
    const EquipmentControlLayout* layout = useNewDesign
            ? NEW_EQUIPMENT_LAYOUT : LEGACY_EQUIPMENT_LAYOUT;
    const int layoutCount = useNewDesign
            ? sizeof(NEW_EQUIPMENT_LAYOUT) / sizeof(NEW_EQUIPMENT_LAYOUT[0])
            : sizeof(LEGACY_EQUIPMENT_LAYOUT) / sizeof(LEGACY_EQUIPMENT_LAYOUT[0]);

    SetGWndSize(useNewDesign ? 408 : 178, useNewDesign ? 350 : 355);
    const wnd_rect bounds = GetBounds();

    for (int i = 0; i < layoutCount; ++i) {
        CIFWnd* control = m_IRM.GetResObj<CIFWnd>(layout[i].id, 1);
        if (!control) {
            continue;
        }

        control->MoveGWnd(bounds.pos.x + layout[i].x, bounds.pos.y + layout[i].y);
        control->SetGWndSize(layout[i].width, layout[i].height);
    }

    CIFWnd* background = m_IRM.GetResObj<CIFWnd>(5, 1);
    if (background) {
        background->TB_Func_13(useNewDesign
                ? "interface\\ifcommon\\bg_tile\\com_bg_tile_p.ddj"
                : "interface\\ifcommon\\bg_tile\\com_bg_tile_d.ddj", 0, 0);
    }

    CIFWnd* frame = m_IRM.GetResObj<CIFWnd>(4, 1);
    if (frame) {
        frame->TB_Func_13(useNewDesign
                ? "interface\\frame\\mall_sub_wnd04_"
                : "interface\\equipment\\equip_window_", 0, 0);
    }
}
void CIFEquipment::AutoSort()
{
    m_Settings->StartAutoSort = true;
}
void CIFEquipment::AutoOrder()
{
    if (s_autoOrderRunning || s_startAutoOrder || m_Settings->StartAutoSort || m_Settings->AutoSortRunning)
        return;

    s_startAutoOrder = true;
    s_lastAutoOrderUpdate = 0;
}
void CIFEquipment::SetSlotLock(byte slot)
{
    if(slot == 0)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(HeadSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(HeadSlot, 1)->ItemInfo != NULL)
            {
                this->m_IRM.GetResObj<CIFSlotWithHelp>(HeadSlot, 1)->ItemInfo->ItemLocked = 1;
            }

        }
    }
    if(slot == 1)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ChestSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ChestSlot, 1)->ItemInfo != NULL)
            {
                this->m_IRM.GetResObj<CIFSlotWithHelp>(ChestSlot, 1)->ItemInfo->ItemLocked = 1;
            }
        }
    }
    if(slot == 2)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ShoulderSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ShoulderSlot, 1)->ItemInfo != NULL)
            {
                this->m_IRM.GetResObj<CIFSlotWithHelp>(ShoulderSlot, 1)->ItemInfo->ItemLocked = 1;
            }
        }
    }
    if(slot == 3)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(HandSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(HandSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(HandSlot, 1)->ItemInfo->ItemLocked = 1;
            }
        }
    }
    if(slot == 4)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(LegSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(LegSlot, 1)->ItemInfo != NULL)
            {
                this->m_IRM.GetResObj<CIFSlotWithHelp>(LegSlot, 1)->ItemInfo->ItemLocked = 1;
            }
        }
    }
    if(slot == 5)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(FootSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(FootSlot, 1)->ItemInfo != NULL)
            {
                this->m_IRM.GetResObj<CIFSlotWithHelp>(FootSlot, 1)->ItemInfo->ItemLocked = 1;
            }
        }
    }
    if(slot == 6)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(WeaponSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(WeaponSlot, 1)->ItemInfo != NULL)
            {
                this->m_IRM.GetResObj<CIFSlotWithHelp>(WeaponSlot, 1)->ItemInfo->ItemLocked = 1;
            }
        }
    }
    if(slot == 7)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ShieldSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ShieldSlot, 1)->ItemInfo != NULL)
            {
               this->m_IRM.GetResObj<CIFSlotWithHelp>(ShieldSlot, 1)->ItemInfo->ItemLocked = 1;
            }
        }
    }
    if(slot == 8)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(JobSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(JobSlot, 1)->ItemInfo != NULL)
            {
                   this->m_IRM.GetResObj<CIFSlotWithHelp>(JobSlot, 1)->ItemInfo->ItemLocked = 1;
            }
        }
    }
    if(slot == 9)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(EarringSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(EarringSlot, 1)->ItemInfo != NULL)
            {
                this->m_IRM.GetResObj<CIFSlotWithHelp>(EarringSlot, 1)->ItemInfo->ItemLocked = 1;
            }
        }
    }
    if(slot == 10)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(NecklaceSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(NecklaceSlot, 1)->ItemInfo != NULL)
            {
                this->m_IRM.GetResObj<CIFSlotWithHelp>(NecklaceSlot, 1)->ItemInfo->ItemLocked = 1;
            }
        }
    }
    if(slot == 11)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(LeftRingSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(LeftRingSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(LeftRingSlot, 1)->ItemInfo->ItemLocked = 1;
            }
        }
    }
    if(slot == 12)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(RightRingSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(RightRingSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(RightRingSlot, 1)->ItemInfo->ItemLocked = 1;
            }
        }
    }
}
void CIFEquipment::SetSlotUnLock(byte slot)
{
    if(slot == 0)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(HeadSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(HeadSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(HeadSlot, 1)->ItemInfo->ItemLocked = 0;
            }

        }
    }
    if(slot == 1)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ChestSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ChestSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(ChestSlot, 1)->ItemInfo->ItemLocked = 0;
            }
        }
    }
    if(slot == 2)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ShoulderSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ShoulderSlot, 1)->ItemInfo != NULL)
            {
                this->m_IRM.GetResObj<CIFSlotWithHelp>(ShoulderSlot, 1)->ItemInfo->ItemLocked = 0;
            }
        }
    }
    if(slot == 3)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(HandSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(HandSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(HandSlot, 1)->ItemInfo->ItemLocked = 0;
            }
        }
    }
    if(slot == 4)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(LegSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(LegSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(LegSlot, 1)->ItemInfo->ItemLocked = 0;
            }
        }
    }
    if(slot == 5)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(FootSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(FootSlot, 1)->ItemInfo != NULL)
            {
                this->m_IRM.GetResObj<CIFSlotWithHelp>(FootSlot, 1)->ItemInfo->ItemLocked = 0;
            }
        }
    }
    if(slot == 6)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(WeaponSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(WeaponSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(WeaponSlot, 1)->ItemInfo->ItemLocked = 0;
            }
        }
    }
    if(slot == 7)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ShieldSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ShieldSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(ShieldSlot, 1)->ItemInfo->ItemLocked = 0;
            }
        }
    }
    if(slot == 8)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(JobSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(JobSlot, 1)->ItemInfo != NULL)
            {
                  this->m_IRM.GetResObj<CIFSlotWithHelp>(JobSlot, 1)->ItemInfo->ItemLocked = 0;
            }
        }
    }
    if(slot == 9)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(EarringSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(EarringSlot, 1)->ItemInfo != NULL)
            {
                  this->m_IRM.GetResObj<CIFSlotWithHelp>(EarringSlot, 1)->ItemInfo->ItemLocked = 0;
            }
        }
    }
    if(slot == 10)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(NecklaceSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(NecklaceSlot, 1)->ItemInfo != NULL)
            {
                  this->m_IRM.GetResObj<CIFSlotWithHelp>(NecklaceSlot, 1)->ItemInfo->ItemLocked = 0;
            }
        }
    }
    if(slot == 11)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(LeftRingSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(LeftRingSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(LeftRingSlot, 1)->ItemInfo->ItemLocked = 0;
            }
        }
    }
    if(slot == 12)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(RightRingSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(RightRingSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(RightRingSlot, 1)->ItemInfo->ItemLocked = 0;
            }
        }
    }
}
void CIFEquipment::SetItemTime(byte slot, long min)
{
    if(slot == 0)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(HeadSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(HeadSlot, 1)->ItemInfo != NULL)
            {
                  this->m_IRM.GetResObj<CIFSlotWithHelp>(HeadSlot, 1)->ItemInfo->itemtimes = min;
            }

        }
    }
    if(slot == 1)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ChestSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ChestSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(ChestSlot, 1)->ItemInfo->itemtimes = min;
            }
        }
    }
    if(slot == 2)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ShoulderSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ShoulderSlot, 1)->ItemInfo != NULL)
            {
                this->m_IRM.GetResObj<CIFSlotWithHelp>(ShoulderSlot, 1)->ItemInfo->itemtimes = min;
            }
        }
    }
    if(slot == 3)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(HandSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(HandSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(HandSlot, 1)->ItemInfo->itemtimes = min;
            }
        }
    }
    if(slot == 4)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(LegSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(LegSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(LegSlot, 1)->ItemInfo->itemtimes = min;
            }
        }
    }
    if(slot == 5)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(FootSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(FootSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(FootSlot, 1)->ItemInfo->itemtimes = min;
            }
        }
    }
    if(slot == 6)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(WeaponSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(WeaponSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(WeaponSlot, 1)->ItemInfo->itemtimes = min;
            }
        }
    }
    if(slot == 7)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ShieldSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(ShieldSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(ShieldSlot, 1)->ItemInfo->itemtimes = min;
            }
        }
    }
    if(slot == 8)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(JobSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(JobSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(JobSlot, 1)->ItemInfo->itemtimes = min;
            }
        }
    }
    if(slot == 9)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(EarringSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(EarringSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(EarringSlot, 1)->ItemInfo->itemtimes = min;
            }
        }
    }
    if(slot == 10)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(NecklaceSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(NecklaceSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(NecklaceSlot, 1)->ItemInfo->itemtimes = min;
            }
        }
    }
    if(slot == 11)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(LeftRingSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(LeftRingSlot, 1)->ItemInfo != NULL)
            {
                 this->m_IRM.GetResObj<CIFSlotWithHelp>(LeftRingSlot, 1)->ItemInfo->itemtimes = min;
            }
        }
    }
    if(slot == 12)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(RightRingSlot, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(RightRingSlot, 1)->ItemInfo != NULL)
            {
                  this->m_IRM.GetResObj<CIFSlotWithHelp>(RightRingSlot, 1)->ItemInfo->itemtimes = min;
            }
        }
    }
    if(slot == 20)
    {
        if(this->m_IRM.GetResObj<CIFSlotWithHelp>(117, 1) != NULL)
        {
            if(this->m_IRM.GetResObj<CIFSlotWithHelp>(117, 1)->ItemInfo != NULL)
            {
                this->m_IRM.GetResObj<CIFSlotWithHelp>(117, 1)->ItemInfo->itemtimes = min;
            }
        }
    }
}
CSOItem* CIFEquipment::GetEquipmentObjectBySlot(BYTE SlotIndex)
{
    return reinterpret_cast<CSOItem*(__thiscall*)(CIFEquipment*, BYTE)>(0x006AB6E0)(this, SlotIndex);
}


CSOItem* CIFEquipment::Test4(BYTE param_1)
{
    return reinterpret_cast<CSOItem*(__thiscall*)(CIFEquipment*, BYTE)>(0x006ab800)(this, param_1);
}

void CIFEquipment::TakeItem(int param_1)
{
    reinterpret_cast<void*(__thiscall*)(CIFEquipment*, int)>(0x0068cb90)(this, param_1);
}
