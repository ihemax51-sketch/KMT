//
// Created by YUMBUL on 24.08.2024.
//

#include "IFGhaCha.h"
#include "GInterface.h"
#include "IFInventory.h"
#include <CustomData/CustomCICPlayer.h>
#include <Windows.h>

namespace {

const int kWaitForResultTimer = 1313;
const int kStartNextPlayTimer = 1314;
const int kNoAutomationSetting = 700;
const int kAutoRefillSetting = 701;
const int kAutoPlaySetting = 702;
const DWORD kRoundResultSettleTimeMs = 1000;
const DWORD kRoundTimeoutMs = 20000;

int g_magicPopCardsBeforePlay = 0;
int g_magicPopWinCardsBeforePlay = 0;
DWORD g_magicPopRoundStartedAt = 0;
DWORD g_magicPopResultObservedAt = 0;

void ResetMagicPopRoundState()
{
    g_magicPopCardsBeforePlay = 0;
    g_magicPopWinCardsBeforePlay = 0;
    g_magicPopRoundStartedAt = 0;
    g_magicPopResultObservedAt = 0;
}

void StopMagicPopTimer(CIFGhaCha *window, int timerId)
{
    window->KillTimer(timerId);
    ResetMagicPopRoundState();
    if (m_Player != NULL)
        m_Player->m_MagicPopTimerRunning = false;
}

int CountMagicPopInventoryItems(bool winCards)
{
    if (g_pCGInterface == NULL || g_pCGInterface->GetMainPopup() == NULL)
        return 0;

    CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
    if (inventory == NULL)
        return 0;

    int count = 0;
    const int slotCount = inventory->InventorySlotCount();
    for (int slot = 0; slot < slotCount; ++slot) {
        CSOItem *item = inventory->GetItemBySlot(slot);
        if (item == NULL || item->m_blValid == 0 || item->GetItemData() == NULL)
            continue;

        const SItemData *itemData = item->GetItemData();
        const bool matches = winCards
                ? (itemData->IsMagicPopWinCard() || itemData->RefObjectId == 9239)
                : itemData->IsMagicPop();
        if (!matches)
            continue;

        const int quantity = item->GetQuantity();
        count += quantity > 0 ? quantity : 1;
    }

    return count;
}

CSOItem *FindMagicPopCard(int &inventorySlot)
{
    inventorySlot = -1;

    if (g_pCGInterface == NULL || g_pCGInterface->GetMainPopup() == NULL)
        return NULL;

    CIFInventory *inventory = g_pCGInterface->GetMainPopup()->GetInventory();
    if (inventory == NULL)
        return NULL;

    const int slotCount = inventory->InventorySlotCount();
    for (int slot = 0; slot < slotCount; ++slot) {
        CSOItem *item = inventory->GetItemBySlot(slot);
        if (item != NULL && item->m_blValid != 0 &&
            item->GetItemData() != NULL && item->GetItemData()->IsMagicPop()) {
            inventorySlot = slot;
            return item;
        }
    }

    return NULL;
}

} // namespace

void CIFGhaCha::PlayButton()
{
    if (m_Player != NULL && !m_Player->m_MagicPopTimerRunning &&
        m_Player->m_MagicPopSettings != kNoAutomationSetting &&
        MagicPopSlot != NULL && MagicPopSlot->ItemInfo != NULL &&
        MagicPopSlot->ItemInfo->GetItemData() != NULL &&
        MagicPopSlot->ItemInfo->GetItemData()->IsMagicPop()) {
        g_magicPopCardsBeforePlay = CountMagicPopInventoryItems(false);
        g_magicPopWinCardsBeforePlay = CountMagicPopInventoryItems(true);
        g_magicPopRoundStartedAt = GetTickCount();
        g_magicPopResultObservedAt = 0;
        m_Player->m_MagicPopTimerRunning = true;
        StartTimer(kWaitForResultTimer, 500);
    }

    reinterpret_cast<void(__thiscall *)(CIFGhaCha *)>(0x007459b0)(this);
}

void CIFGhaCha::OnTimerIMPL(int timerId)
{
    reinterpret_cast<void(__thiscall *)(CIFGhaCha *, int)>(0x00746c70)(this, timerId);

    if (timerId == kStartNextPlayTimer) {
        KillTimer(kStartNextPlayTimer);

        if (m_Player != NULL && m_Player->m_MagicPopSettings == kAutoPlaySetting)
            PlayButton();

        return;
    }

    if (timerId != kWaitForResultTimer || m_Player == NULL)
        return;

    if (m_Player->m_MagicPopSettings != kAutoRefillSetting &&
        m_Player->m_MagicPopSettings != kAutoPlaySetting) {
        StopMagicPopTimer(this, kWaitForResultTimer);
        return;
    }

    if (MagicPopSlot == NULL) {
        StopMagicPopTimer(this, kWaitForResultTimer);
        return;
    }

    const DWORD now = GetTickCount();
    if (g_magicPopRoundStartedAt == 0 ||
        now - g_magicPopRoundStartedAt >= kRoundTimeoutMs) {
        StopMagicPopTimer(this, kWaitForResultTimer);
        return;
    }

    const SItemData *selectedItemData = NULL;
    if (MagicPopSlot->ItemInfo != NULL)
        selectedItemData = MagicPopSlot->ItemInfo->GetItemData();

    const bool resultShownInSlot = selectedItemData != NULL &&
            !selectedItemData->IsMagicPop();
    const bool inputCardWasConsumed =
            CountMagicPopInventoryItems(false) < g_magicPopCardsBeforePlay;

    // A normal Magic Pop result consumes the input card and may leave the play
    // slot empty. Some client layouts briefly show the result in the slot, so
    // support both signals and allow the inventory update to settle.
    if (!inputCardWasConsumed && !resultShownInSlot)
        return;

    if (g_magicPopResultObservedAt == 0) {
        g_magicPopResultObservedAt = now;
        return;
    }

    if (now - g_magicPopResultObservedAt < kRoundResultSettleTimeMs)
        return;

    const bool wonInSlot = selectedItemData != NULL &&
            (selectedItemData->IsMagicPopWinCard() || selectedItemData->RefObjectId == 9239);
    const bool wonInInventory =
            CountMagicPopInventoryItems(true) > g_magicPopWinCardsBeforePlay;

    if (m_Player->m_MagicPopSettings == kAutoPlaySetting &&
        (wonInSlot || wonInInventory)) {
        StopMagicPopTimer(this, kWaitForResultTimer);
        return;
    }

    int inventorySlot = -1;
    CSOItem *nextCard = FindMagicPopCard(inventorySlot);
    if (nextCard == NULL) {
        StopMagicPopTimer(this, kWaitForResultTimer);
        return;
    }

    MagicPopSlot->SetSlotData(nextCard);
    MagicPopSlot->SetInventorySlotType(inventorySlot);

    if (PlayButtonMaybe != NULL)
        PlayButtonMaybe->SetEnabledState(true);

    StopMagicPopTimer(this, kWaitForResultTimer);

    if (m_Player->m_MagicPopSettings == kAutoPlaySetting)
        StartTimer(kStartNextPlayTimer, 1000);
}
