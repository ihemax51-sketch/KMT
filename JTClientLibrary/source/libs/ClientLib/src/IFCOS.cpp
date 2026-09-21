//
// Created by Admin on 21/11/2021.
//

#include "IFCOS.h"
#include "ICPlayer.h"
#include "GInterface.h"
#include "ICCos.h"
#include "IFCOSStatus.h"
#include "IFCOSManager.h"
#include "TextStringManager.h"
#include "IFButton.h"
#include <Game.h>
#include <IFNormalTile.h>
#include <GlobalDataManager.h>
#include <SRIFLib/NInterfaceResource.h>
#include <CustomData/CustomDataManager.h>
#include <GlobalHelpersThatHaveNoHomeYet.h>
#include <BSLib/multibyte.h>
#include <algorithm>
#include <ctime>
#include <vector>

#define GDR_TEST_BTN 13312321
#define GDR_TEST_BTN2 133123212
#define GDR_COS_AUTO_SORT_BTN 13312330
#define GDR_COS_CONVERT_BTN 13312331
#define GDR_COS_PET_FILTER_BTN 13312332
GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFCOS, 0x00EEC048)

namespace {
const int PET_INVENTORY_MAX_SLOTS = 28;
const double PET_ACTION_DELAY_SECONDS = 0.80;
const double PET_ACTION_TIMEOUT_SECONDS = 3.00;
bool s_petAutoSortRunning = false;
bool s_petConvertRunning = false;
clock_t s_lastPetActionTick = 0;
int s_pendingPetSourceSlot = -1;
int s_pendingPetSourceRefObjId = 0;
int s_pendingPetSourceQuantity = 0;
clock_t s_pendingPetActionTick = 0;

bool IsFellowPet(int uniqueId)
{
    if (uniqueId == 0)
        return false;

    CICharactor* pUser = GetCharacterObjectByID_MAYBE(uniqueId);
    if (pUser == NULL)
        return false;

    std::map<std::n_wstring, CustomDataManager::FellowPetStruct>::iterator fellow =
        m_CustomDataManager->m_RefFellowPetSystem.find(pUser->GetCommonData()->NameStrID);

    if (fellow == m_CustomDataManager->m_RefFellowPetSystem.end())
        return false;

    return !fellow->second.NameStrID.empty();
}

bool IsManagedCos(int uniqueId)
{
    if (uniqueId == 0 || g_pCGInterface == NULL)
        return false;

    CIFCOSManager* manager = g_pCGInterface->m_IRM.GetResObj<CIFCOSManager>(39, 1);
    if (manager == NULL)
        return false;

    for (std::list<CIFCOSStatus*>::iterator it = manager->N00000A57.begin();
         it != manager->N00000A57.end(); ++it)
    {
        if ((*it) != NULL && (*it)->m_petUniqueID == uniqueId)
            return true;
    }

    return false;
}

CCOSDataMgr::CosData* GetCosDataByUniqueId(int uniqueId)
{
    if (uniqueId == 0 || g_pMyPlayerObj == NULL || g_pMyPlayerObj->CCOSDataMgr == NULL)
        return NULL;

    std::map<int, CCOSDataMgr::CosData*>::iterator it = g_pMyPlayerObj->CCOSDataMgr->CosList.find(uniqueId);
    if (it == g_pMyPlayerObj->CCOSDataMgr->CosList.end())
        return NULL;

    return it->second;
}

bool IsPickupPetUniqueId(int uniqueId)
{
    CCOSDataMgr::CosData* cosData = GetCosDataByUniqueId(uniqueId);
    if (cosData == NULL || cosData->RefObjID == 0)
        return false;

    const CCharacterData* data = g_CGlobalDataManager->GetCharacter(cosData->RefObjID);
    return data != NULL && data->IsGrapPet();
}

CIFButton* CreateCosButton(CIFCOS* wnd, int id, int x, int y, const char* texturePath, const char* pressedTexturePath)
{
    if (wnd == NULL)
        return NULL;

    RECT rect = {x, y, 40, 32};
    CIFButton* button = (CIFButton*)CGWnd::CreateInstance(wnd, GFX_RUNTIME_CLASS(CIFButton), rect, id, 0);
    if (button == NULL)
        return NULL;

    button->TB_Func_13(texturePath, 0, 0);
    button->FUN_00656590(std::n_string(pressedTexturePath));
    button->SetText(L"");
    button->SetFont(theApp.GetFont(0));
    button->JustifyHorizontal(CTextBoard::JUSTIFY_CENTER);
    button->JustifyVertical(CTextBoard::JUSTIFY_MIDDLE);
    button->SetEnabledState(true);
    button->ShowGWnd(true);
    button->BringToFront();
    return button;
}

CIFCOSInventory* GetCosInventoryWnd(CIFCOS* wnd)
{
    if (wnd == NULL)
        return NULL;

    return wnd->m_COSInventory;
}

int GetCosWindowUniqueId(CIFCOS* wnd, CIFCOSInventory* inventory)
{
    if (inventory != NULL && inventory->m_UniqueID != 0)
        return inventory->m_UniqueID;

    if (wnd != NULL)
        return wnd->m_petUniqueID;

    return 0;
}

int GetCosSlotCount(CIFCOSInventory* inventory)
{
    if (inventory == NULL)
        return 0;

    int count = inventory->m_TotalSlot;
    if (count <= 0 || count > PET_INVENTORY_MAX_SLOTS)
        count = PET_INVENTORY_MAX_SLOTS;
    return count;
}

CIFSlotWithHelp* GetPetSlot(CIFCOSInventory* inventory, int index)
{
    if (inventory == NULL || index < 0 || index >= PET_INVENTORY_MAX_SLOTS)
        return NULL;
    return inventory->m_slots[index];
}

CIFSlotWithHelp* FindEmptyInventorySlot()
{
    if (g_pCGInterface == NULL || g_pCGInterface->GetMainPopup() == NULL)
        return NULL;

    CIFInventory* inventory = g_pCGInterface->GetMainPopup()->GetInventory();
    if (inventory == NULL)
        return NULL;

    const int slotCount = inventory->InventorySlotCount();
    for (int i = 0; i < slotCount && i < (int)inventory->pSlots.size(); ++i) {
        CIFSlotWithHelp* slot = inventory->pSlots[i];
        if (slot != NULL && slot->ItemInfo == NULL)
            return slot;
    }

    return NULL;
}

void SendPetInventoryMove(int petUniqueId, BYTE sourceSlot, BYTE destinationSlot)
{
    CMsgStreamBuffer buf(0x7034);
    buf << (BYTE)0x1A
        << petUniqueId
        << sourceSlot
        << destinationSlot;
    SendMsg(buf);
}

void SendPetStorageMove(int petUniqueId, BYTE sourceSlot, BYTE destinationSlot, UINT16 quantity)
{
    CMsgStreamBuffer buf(0x7034);
    buf << (BYTE)0x10
        << petUniqueId
        << sourceSlot
        << destinationSlot
        << quantity;
    SendMsg(buf);
}

int GetPetSlotItemRefObjId(CIFCOS* wnd, int slotIndex)
{
    if (wnd == NULL || wnd->m_COSInventory == NULL)
        return 0;

    CIFSlotWithHelp* slot = GetPetSlot(wnd->m_COSInventory, slotIndex);
    if (slot == NULL || slot->ItemInfo == NULL || slot->ItemInfo->GetItemData() == NULL)
        return 0;

    return slot->ItemInfo->GetItemData()->RefObjectId;
}

int GetPetSlotItemQuantity(CIFCOS* wnd, int slotIndex)
{
    if (wnd == NULL || wnd->m_COSInventory == NULL)
        return 0;

    CIFSlotWithHelp* slot = GetPetSlot(wnd->m_COSInventory, slotIndex);
    if (slot == NULL || slot->ItemInfo == NULL)
        return 0;

    return slot->ItemInfo->GetQuantity();
}

void ClearPetActionPending()
{
    s_pendingPetSourceSlot = -1;
    s_pendingPetSourceRefObjId = 0;
    s_pendingPetSourceQuantity = 0;
    s_pendingPetActionTick = 0;
}

void SetPetActionPending(CIFCOS* wnd, CIFSlotWithHelp* source)
{
    if (source == NULL || source->ItemInfo == NULL || source->ItemInfo->GetItemData() == NULL) {
        ClearPetActionPending();
        return;
    }

    s_pendingPetSourceSlot = source->GetSlot();
    s_pendingPetSourceRefObjId = source->ItemInfo->GetItemData()->RefObjectId;
    s_pendingPetSourceQuantity = source->ItemInfo->GetQuantity();
    s_pendingPetActionTick = clock();
}

bool IsPetActionPending(CIFCOS* wnd)
{
    if (s_pendingPetSourceSlot < 0)
        return false;

    if (GetPetSlotItemRefObjId(wnd, s_pendingPetSourceSlot) != s_pendingPetSourceRefObjId) {
        ClearPetActionPending();
        return false;
    }

    if (GetPetSlotItemQuantity(wnd, s_pendingPetSourceSlot) != s_pendingPetSourceQuantity) {
        ClearPetActionPending();
        return false;
    }

    if (s_pendingPetActionTick != 0 &&
        double(clock() - s_pendingPetActionTick) / CLOCKS_PER_SEC > PET_ACTION_TIMEOUT_SECONDS) {
        s_petAutoSortRunning = false;
        s_petConvertRunning = false;
        ClearPetActionPending();
        return false;
    }

    return true;
}

bool SendNextPetStackMerge(CIFCOS* wnd)
{
    if (wnd == NULL || wnd->m_COSInventory == NULL)
        return false;

    CIFCOSInventory* inventory = wnd->m_COSInventory;
    const int slotCount = GetCosSlotCount(inventory);

    for (int i = 0; i < slotCount; ++i) {
        CIFSlotWithHelp* target = GetPetSlot(inventory, i);
        if (target == NULL || target->ItemInfo == NULL || target->ItemInfo->GetItemData() == NULL)
            continue;

        const int targetItemId = target->ItemInfo->GetItemData()->RefObjectId;
        const int targetCount = target->ItemInfo->GetQuantity();
        const int maxStack = target->ItemInfo->GetItemData()->m_maxStack;
        if (maxStack <= 1 || targetCount >= maxStack)
            continue;

        for (int j = i + 1; j < slotCount; ++j) {
            CIFSlotWithHelp* source = GetPetSlot(inventory, j);
            if (source == NULL || source->ItemInfo == NULL || source->ItemInfo->GetItemData() == NULL)
                continue;

            if (source->ItemInfo->GetItemData()->RefObjectId != targetItemId)
                continue;

            const int sourceCount = source->ItemInfo->GetQuantity();
            if (sourceCount <= 0)
                continue;

            const int countToMove = sourceCount < (maxStack - targetCount) ? sourceCount : (maxStack - targetCount);
            SetPetActionPending(wnd, source);
            SendPetStorageMove(wnd->m_petUniqueID, (BYTE)source->GetSlot(), (BYTE)target->GetSlot(), (UINT16)countToMove);
            return true;
        }
    }

    return false;
}

bool SendNextPetCompactMove(CIFCOS* wnd)
{
    if (wnd == NULL || wnd->m_COSInventory == NULL)
        return false;

    CIFCOSInventory* inventory = wnd->m_COSInventory;
    const int slotCount = GetCosSlotCount(inventory);

    for (int i = 0; i < slotCount; ++i) {
        CIFSlotWithHelp* target = GetPetSlot(inventory, i);
        if (target == NULL || target->ItemInfo != NULL)
            continue;

        for (int j = i + 1; j < slotCount; ++j) {
            CIFSlotWithHelp* source = GetPetSlot(inventory, j);
            if (source == NULL || source->ItemInfo == NULL)
                continue;

            const int sourceCount = source->ItemInfo->GetQuantity();
            SetPetActionPending(wnd, source);
            SendPetStorageMove(wnd->m_petUniqueID,
                               (BYTE)source->GetSlot(),
                               (BYTE)target->GetSlot(),
                               (UINT16)(sourceCount > 0 ? sourceCount : 1));
            return true;
        }
    }

    return false;
}

bool SendNextPetItemToInventory(CIFCOS* wnd)
{
    if (wnd == NULL || wnd->m_COSInventory == NULL)
        return false;

    CIFSlotWithHelp* emptyInventorySlot = FindEmptyInventorySlot();
    if (emptyInventorySlot == NULL)
        return false;

    CIFCOSInventory* petInventory = wnd->m_COSInventory;
    const int slotCount = GetCosSlotCount(petInventory);

    for (int i = 0; i < slotCount; ++i) {
        CIFSlotWithHelp* source = GetPetSlot(petInventory, i);
        if (source == NULL || source->ItemInfo == NULL || source->ItemInfo->GetItemData() == NULL)
            continue;

        SetPetActionPending(wnd, source);
        SendPetInventoryMove(wnd->m_petUniqueID,
                             (BYTE)source->GetSlot(),
                             (BYTE)(emptyInventorySlot->GetSlot() + 13));
        return true;
    }

    return false;
}
}

void CIFCOS::ApplyFellowPetLayout(bool isFellowPet)
{
    if (isFellowPet)
    {
        this->m_SelectableArea->sub_64CE30("clientlibrary\\fellowpets\\pet_info_tab_on.ddj",
                                           "clientlibrary\\fellowpets\\pet_info_tab_off.ddj",
                                           "clientlibrary\\fellowpets\\pet_info_tab_disable.ddj");
        this->m_SelectableArea->sub_64CC30(1);
        this->m_SelectableArea->SetText(L"");
        this->m_SelectableArea1->ShowGWnd(false);
        this->m_SelectableArea2->ShowGWnd(false);
        this->SetGWndSize(355, 482);
        this->m_IRM.GetResObj<CIFFrame>(0, 1)->SetGWndSize(331, 403);
        this->m_IRM.GetResObj<CIFSelectableArea>(1000, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj<CIFSelectableArea>(1000, 1)->sub_64CC30(0);
        this->m_IRM.GetResObj<CIFSelectableArea>(1001, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj<CIFSelectableArea>(1001, 1)->sub_64CC30(0);
    }
    else
    {
        this->m_SelectableArea->sub_64CE30("interface\\ifcommon\\com_long_tab_on.ddj",
                                           "interface\\ifcommon\\com_long_tab_off.ddj",
                                           "interface\\ifcommon\\com_long_tab_disable.ddj");
        this->m_SelectableArea->sub_64CC30(1);
        this->m_SelectableArea->SetText(TSM_GETTEXTPTR(L"UIIT_STT_COSNEWUI_TABMENU_BASICINFO"));
        this->m_SelectableArea1->ShowGWnd(true);
        this->m_SelectableArea2->ShowGWnd(true);
        this->SetGWndSize(355, 390);
        this->m_IRM.GetResObj<CIFFrame>(0, 1)->SetGWndSize(331, 314);
        this->m_IRM.GetResObj<CIFSelectableArea>(1000, 1)->ShowGWnd(false);
        this->m_IRM.GetResObj<CIFSelectableArea>(1000, 1)->sub_64CC30(0);
        this->m_IRM.GetResObj<CIFSelectableArea>(1001, 1)->ShowGWnd(false);
        this->m_IRM.GetResObj<CIFSelectableArea>(1001, 1)->sub_64CC30(0);
    }
}

GFX_MSGMAP* CIFCOS::MessageMap(){
    static const GFX_MSGMAP_ENTRY skillBoardMessageEntries[] =
            {
                    {GFX_WM_COMMAND, 0, 1000, 1000, BSSig_u12, 0,
                            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFCOS::ActiveTab2))},
                    {GFX_WM_COMMAND, 0, 1001, 1001, BSSig_u12, 0,
                            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFCOS::ActiveTab3))},
                    {GFX_WM_COMMAND, 0, GDR_COS_AUTO_SORT_BTN, GDR_COS_AUTO_SORT_BTN, BSSig_u12, 0,
                            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFCOS::OnPetAutoSort))},
                    {GFX_WM_COMMAND, 0, GDR_COS_CONVERT_BTN, GDR_COS_CONVERT_BTN, BSSig_u12, 0,
                            (GFX_PMSG)(static_cast< void (GFX_MSG_CALL CGWndBase::*)() >(&CIFCOS::OnPetConvertToInventory))},
            };

    static GFX_MSGMAP newmap =
            {
                    reinterpret_cast<const GFX_MSGMAP *>(0x00dba074), skillBoardMessageEntries,
            };
    return &newmap;
}
void CIFCOS::PressTabs()
{
    int id = GetCurrentEventMsgCtrlId();

    reinterpret_cast<void(__thiscall*)(CIFCOS*)>(0x0079ff10)(this);

    if(id == 100 && IsFellowPet(this->m_petUniqueID))
    {
        this->m_IRM.GetResObj<CIFSelectableArea>(1000, 1)->sub_64CC30(0);
        this->m_IRM.GetResObj<CIFSelectableArea>(1001, 1)->sub_64CC30(0);
        this->m_SelectableArea->sub_64CC30(1);
        if(this->m_CosInfo != NULL)
        {
            this->m_CosInfo->ShowTab1();
        }
    }
}
void CIFCOS::ActiveTab2()
{
    int id = GetCurrentEventMsgCtrlId();
    if(id == 1000)
    {
        this->m_SelectableArea->sub_64CC30(0);
        this->m_IRM.GetResObj<CIFSelectableArea>(1000, 1)->sub_64CC30(1);
        this->m_IRM.GetResObj<CIFSelectableArea>(1001, 1)->sub_64CC30(0);
        if(this->m_CosInfo != NULL)
        {
            this->m_CosInfo->HideTab1();
        }
    }
}
void CIFCOS::ActiveTab3() {
    int id = GetCurrentEventMsgCtrlId();
    if(id == 1001)
    {
        this->m_SelectableArea->sub_64CC30(0);
        this->m_IRM.GetResObj<CIFSelectableArea>(1000, 1)->sub_64CC30(0);
        this->m_IRM.GetResObj<CIFSelectableArea>(1001, 1)->sub_64CC30(1);
        if(this->m_CosInfo != NULL)
        {
            this->m_CosInfo->HideTab1();
            this->m_CosInfo->HideTab2();
        }
    }
}
bool CIFCOS::OnCreateIMPL(long ln){
    bool b = reinterpret_cast<bool(__thiscall *)(CIFCOS *, long)>(0x0079f9a0)(this, ln);
    this->m_IRM.GetResObj<CIFSelectableArea>(1000, 1)->sub_64CE30("clientlibrary\\fellowpets\\pet_item_tab_on.ddj",
                                                                  "clientlibrary\\fellowpets\\pet_item_tab_off.ddj", "clientlibrary\\fellowpets\\pet_item_tab_disable.ddj");
    this->m_IRM.GetResObj<CIFSelectableArea>(1000, 1)->sub_64CC30(0);
    this->m_IRM.GetResObj<CIFSelectableArea>(1000, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFSelectableArea>(1001, 1)->sub_64CE30("clientlibrary\\fellowpets\\pet_skill_tab_on.ddj",
                                                                  "clientlibrary\\fellowpets\\pet_skill_tab_off.ddj", "clientlibrary\\fellowpets\\pet_skill_tab_disable.ddj");

    this->m_IRM.GetResObj<CIFSelectableArea>(1001, 1)->sub_64CC30(0);
    this->m_IRM.GetResObj<CIFSelectableArea>(1001, 1)->ShowGWnd(false);
    CreatePetInventoryButtons();

    if (IsManagedCos(this->m_petUniqueID))
    {
        ApplyFellowPetLayout(IsFellowPet(this->m_petUniqueID));
    }

    return b;
}

void CIFCOS::CreatePetInventoryButtons()
{
    if (GetChildControl(GDR_COS_AUTO_SORT_BTN) == NULL)
    {
        CreateCosButton(this,
                        GDR_COS_AUTO_SORT_BTN,
                        73,
                        285,
                        "clientlibrary\\fellowpets\\pet_auto_sort_button.ddj",
                        "clientlibrary\\fellowpets\\pet_auto_sort_button_pressed.ddj");
    }

    if (GetChildControl(GDR_COS_CONVERT_BTN) == NULL)
    {
        CreateCosButton(this,
                        GDR_COS_CONVERT_BTN,
                        238,
                        285,
                        "clientlibrary\\fellowpets\\pet_convert_button.ddj",
                        "clientlibrary\\fellowpets\\pet_convert_button_pressed.ddj");
    }

    UpdatePetInventoryButtons();
    UpdatePetSetupFilterButton();
}

void CIFCOS::UpdatePetInventoryButtons()
{
    CIFButton* sortButton = (CIFButton*)GetChildControl(GDR_COS_AUTO_SORT_BTN);
    CIFButton* convertButton = (CIFButton*)GetChildControl(GDR_COS_CONVERT_BTN);
    CIFCOSInventory* inventory = GetCosInventoryWnd(this);
    if (inventory == NULL)
        inventory = this->m_IRM.GetResObj<CIFCOSInventory>(122, 1);

    const int petUniqueId = GetCosWindowUniqueId(this, inventory);
    const bool showButtons = inventory != NULL && inventory->IsVisible() && IsPickupPetUniqueId(petUniqueId);
    if (sortButton != NULL) {
        sortButton->ShowGWnd(showButtons);
        sortButton->SetEnabledState(showButtons);
        sortButton->SetClickable(showButtons);
        if (showButtons) {
            sortButton->BringToFront();
        }
    }
    if (convertButton != NULL) {
        convertButton->ShowGWnd(showButtons);
        convertButton->SetEnabledState(showButtons);
        convertButton->SetClickable(showButtons);
        if (showButtons) {
            convertButton->BringToFront();
        }
    }
}

void CIFCOS::UpdatePetSetupFilterButton()
{
    CIFCOSSetup* setup = this->m_COSSetup;
    if (setup == NULL)
        setup = this->m_IRM.GetResObj<CIFCOSSetup>(123, 1);

    if (setup == NULL)
        return;

    setup->UpdatePetFilterButtonLayout(setup->IsVisible());
}

void CIFCOS::OnPetAutoSort()
{
    if (this->m_COSInventory == NULL ||
        !IsPickupPetUniqueId(GetCosWindowUniqueId(this, this->m_COSInventory)))
        return;

    s_petConvertRunning = false;
    s_petAutoSortRunning = true;
    s_lastPetActionTick = 0;
    ClearPetActionPending();
}

void CIFCOS::OnPetConvertToInventory()
{
    if (this->m_COSInventory == NULL ||
        !IsPickupPetUniqueId(GetCosWindowUniqueId(this, this->m_COSInventory)))
        return;

    s_petAutoSortRunning = false;
    s_petConvertRunning = true;
    s_lastPetActionTick = 0;
    ClearPetActionPending();
}

void CIFCOS::ProcessPetInventoryActions()
{
    if (!s_petAutoSortRunning && !s_petConvertRunning)
        return;

    if (!IsPickupPetUniqueId(GetCosWindowUniqueId(this, this->m_COSInventory))) {
        s_petAutoSortRunning = false;
        s_petConvertRunning = false;
        ClearPetActionPending();
        return;
    }

    if (IsPetActionPending(this))
        return;

    const clock_t now = clock();
    if (s_lastPetActionTick != 0 &&
        double(now - s_lastPetActionTick) / CLOCKS_PER_SEC < PET_ACTION_DELAY_SECONDS)
        return;

    bool sent = false;
    if (s_petAutoSortRunning)
        sent = SendNextPetStackMerge(this) || SendNextPetCompactMove(this);
    else if (s_petConvertRunning)
        sent = SendNextPetItemToInventory(this);

    if (!sent) {
        s_petAutoSortRunning = false;
        s_petConvertRunning = false;
        return;
    }

    s_lastPetActionTick = now;
}

void CIFCOS::Switch(int p1, int i)
{
    bool managedCos = IsManagedCos(p1);
    if (i == 3 && p1 != 0 && !managedCos)
    {
        return;
    }

    reinterpret_cast<void(__thiscall*)(CIFCOS*, int, int)>(0x0079ff50)(this, p1, i);

    if(i == 3 && p1 != 0 && managedCos)
    {
        ApplyFellowPetLayout(IsFellowPet(p1));
    }
}

void CIFCOS::FUN_0079fcd0(int p1)
{
    reinterpret_cast<void(__thiscall*)(CIFCOS*, int)>(0x007a0140)(this, p1);
}
void CIFCOS::FUN_0079fc20(int p1)
{
    reinterpret_cast<void(__thiscall*)(CIFCOS*, int)>(0x0079fc20)(this, p1);
}
void CIFCOS::OnUpdateIMPL() {
    reinterpret_cast<void(__thiscall*)(const CIFCOS*)>(0x006528a0)(this);
    UpdatePetInventoryButtons();
    UpdatePetSetupFilterButton();
    ProcessPetInventoryActions();

    CICCos* pUser = static_cast<CICCos*>(GetCharacterObjectByID_MAYBE(this->m_petUniqueID));
    if (pUser == NULL) {
        return;
    }
}
