#include "ItemRegionTravelGuard.h"

#include <Objects/GObjPC.h>
#include <Objects/GItem.h>
#include <SqlConnection/sqlCon.h>
#include <KMTGuardCustom/GameServerRuntimeSafety.h>
#include <BSObj/BSObj.h>

#include <map>
#include <windows.h>

namespace
{
    const DWORD MOVE_TO_ADDRESS = 0x004DF590;
    const DWORD PENDING_ITEM_TIMEOUT_MS = 120000;
    const WORD ITEM_REGION_BLOCKED_PACKET = 0x3572;

    typedef bool (__thiscall* MoveToFunction)(
        CGObjPC*, uint32_t*, unsigned short, float, float, float, unsigned int);

    MoveToFunction s_originalMoveTo = reinterpret_cast<MoveToFunction>(MOVE_TO_ADDRESS);
    bool s_moveToDetourInstalled = false;
    CRITICAL_SECTION s_pendingItemLock;

    struct PendingItemUse
    {
        int itemId;
        DWORD observedAt;

        PendingItemUse()
            : itemId(0), observedAt(0) {}
        PendingItemUse(int value, DWORD tick)
            : itemId(value), observedAt(tick) {}
    };

    std::map<DWORD, PendingItemUse> s_pendingItems;

    struct PendingItemLockInitializer
    {
        PendingItemLockInitializer()
        {
            InitializeCriticalSection(&s_pendingItemLock);
        }

        ~PendingItemLockInitializer()
        {
            DeleteCriticalSection(&s_pendingItemLock);
        }
    } s_pendingItemLockInitializer;

    class ScopedPendingItemLock
    {
    public:
        ScopedPendingItemLock() { EnterCriticalSection(&s_pendingItemLock); }
        ~ScopedPendingItemLock() { LeaveCriticalSection(&s_pendingItemLock); }

    private:
        ScopedPendingItemLock(const ScopedPendingItemLock&);
        ScopedPendingItemLock& operator=(const ScopedPendingItemLock&);
    };

    void ClearPendingItem(CGObjPC* player)
    {
        if (player == NULL)
            return;
        ScopedPendingItemLock lock;
        s_pendingItems.erase(player->GetGameID());
    }

    void RememberPendingItem(CGObjPC* player, int itemId)
    {
        if (player == NULL || itemId <= 0)
            return;
        ScopedPendingItemLock lock;
        s_pendingItems[player->GetGameID()] = PendingItemUse(
            itemId,
            GetTickCount());
    }

    bool TryGetPendingItem(CGObjPC* player, PendingItemUse& pending)
    {
        pending = PendingItemUse();
        if (player == NULL)
            return false;

        ScopedPendingItemLock lock;
        std::map<DWORD, PendingItemUse>::iterator entry =
            s_pendingItems.find(player->GetGameID());
        if (entry == s_pendingItems.end())
            return false;

        if ((GetTickCount() - entry->second.observedAt) > PENDING_ITEM_TIMEOUT_MS)
        {
            s_pendingItems.erase(entry);
            return false;
        }

        pending = entry->second;
        return pending.itemId > 0;
    }

    void KeepPendingForRetry(CGObjPC* player)
    {
        if (player == NULL)
            return;
        ScopedPendingItemLock lock;
        std::map<DWORD, PendingItemUse>::iterator entry =
            s_pendingItems.find(player->GetGameID());
        if (entry != s_pendingItems.end())
            entry->second.observedAt = GetTickCount() - (PENDING_ITEM_TIMEOUT_MS - 2000);
    }

    void SendBlockedNotice(CGObjPC* player)
    {
        if (player == NULL)
            return;
        CMsg* message = player->AllocMsg(ITEM_REGION_BLOCKED_PACKET);
        if (message != NULL)
            player->SendMsg(message);
    }

    int NormalizeRegionId(unsigned short regionId)
    {
        return static_cast<int>(static_cast<short>(regionId));
    }

    int NormalizeWorldId(uint32_t rawWorldId)
    {
        return static_cast<int>(rawWorldId & 0xFFFF);
    }

    bool IsReverseReturnScroll(CGItem* item)
    {
        return item != NULL && item->InstanceItem != NULL &&
            item->InstanceItem->pCRefObjItem != NULL &&
            (item->InstanceItem->pCRefObjItem->TID.m_type_id_value == 6636 ||
             item->InstanceItem->pCRefObjItem->TID.m_type_id_value == 6637);
    }

    bool __fastcall MoveToGuard(
        CGObjPC* player,
        void*,
        uint32_t* targetWorld,
        unsigned short targetRegion,
        float x,
        float y,
        float z,
        unsigned int mode)
    {
        if (player == NULL || targetWorld == NULL)
            return s_originalMoveTo(player, targetWorld, targetRegion, x, y, z, mode);

        PendingItemUse pending;
        if (TryGetPendingItem(player, pending))
        {
            const int worldId = NormalizeWorldId(*targetWorld);
            const int regionId = NormalizeRegionId(targetRegion);

            if (CSqlCon::IsItemBlockedInRegion(worldId, regionId, pending.itemId))
            {
                KeepPendingForRetry(player);
                SendBlockedNotice(player);
                BS_INFO(
                    "[KMTGuard][ItemRegion] Blocked pre-move travel game_id=%u item_id=%d world_id=%d region_id=%d",
                    static_cast<unsigned int>(player->GetGameID()), pending.itemId, worldId, regionId);
                return false;
            }
            ClearPendingItem(player);
        }

        return s_originalMoveTo(player, targetWorld, targetRegion, x, y, z, mode);
    }
}

bool CItemRegionTravelGuard::Initialize()
{
    if (s_moveToDetourInstalled)
        return true;

    s_originalMoveTo = reinterpret_cast<MoveToFunction>(MOVE_TO_ADDRESS);
    s_moveToDetourInstalled = GameServerRuntimeSafety::AttachDetour(
        reinterpret_cast<PVOID*>(&s_originalMoveTo),
        reinterpret_cast<PVOID>(&MoveToGuard),
        "item-region pre-move guard");
    return s_moveToDetourInstalled;
}

void CItemRegionTravelGuard::Shutdown()
{
    if (s_moveToDetourInstalled)
    {
        GameServerRuntimeSafety::DetachDetour(
            reinterpret_cast<PVOID*>(&s_originalMoveTo),
            reinterpret_cast<PVOID>(&MoveToGuard),
            "item-region pre-move guard");
        s_moveToDetourInstalled = false;
    }

    ScopedPendingItemLock lock;
    s_pendingItems.clear();
}

bool CItemRegionTravelGuard::InspectItemUse(CGObjPC* player, CMsg* message)
{
    if (player == NULL || message == NULL ||
        message->m_wWriteDataArrayPos - message->m_wReadDataArrayPos < 3)
    {
        ClearPendingItem(player);
        return true;
    }

    const int originalReadPosition = message->m_wReadDataArrayPos;
    BYTE slot = 0;
    *message >> slot;
    message->m_wReadDataArrayPos = originalReadPosition;

    if (slot >= player->m_PCInventory.m_nSlotsCount)
    {
        ClearPendingItem(player);
        return true;
    }

    CGItem* item = player->GetItemChar(slot);
    if (item == NULL || item->InstanceItem == NULL ||
        item->InstanceItem->pCRefObjItem == NULL)
    {
        ClearPendingItem(player);
        return true;
    }

    const int itemId = item->InstanceItem->RefItemID;
    const bool isReverseReturnScroll = IsReverseReturnScroll(item);
    if (itemId <= 0 || (!isReverseReturnScroll && !CSqlCon::IsItemTrackedForRegion(itemId)))
    {
        ClearPendingItem(player);
        return true;
    }

    RememberPendingItem(player, itemId);
    return true;
}

void CItemRegionTravelGuard::ForgetPlayer(CGObjPC* player)
{
    ClearPendingItem(player);
}
