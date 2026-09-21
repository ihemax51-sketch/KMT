//
// Created by YUMBUL on 21.06.2025.
//

#include <Game.h>
#include <NetHelper.h>
#include <Objects/GObjMob.h>
#include "GObjPC.h"
#include "SqlConnection/sqlCon.h"
#include <ReferenceData/ReferenceDataMgr.h>
#include <map>
#include <set>
#include <float.h>
#include <KMTGuardCustom/GameServerTelemetry.h>
#include <KMTGuardCustom/InternalPacketAuth.h>
#include <KMTGuardCustom/ItemRegionTravelGuard.h>


SPosInfo CGObjPC::GetCurrentPosition()
{
    SPosInfo mypos;
    this->GetPosInfo(mypos);
    return mypos;
}
#define OFFSET			0x004e2830
void CGObjPC::FuncJobExpTest(unsigned int p1)
{
    DWORD dwCallAddr = OFFSET;

    __asm pushad;
    __asm pushfd;

    __asm push p1;
    __asm mov edi, dword ptr[this];
    __asm call dwCallAddr;

    __asm popfd;
    __asm popad;

}

#define KEY_OF_SECRET "B4517142409MG!NEWFILTER"

enum E_CHAT_TYPE : byte
{
    LOCAL_ALL               = 1,
    PRIVATE                 = 2,
    LOCAL_GM                = 3,
    PARTY                   = 4,
    GUILD                   = 5,
    GLOBAL                  = 6,
    NOTICE                  = 7,
    STALL                   = 9,
    UNION                   = 11,
    NPC                     = 13,
    ACADEMY                 = 16
};
int SuccesRateOptLevel_1 = 100;
int SuccesRateOptLevel_2 = 100;
int SuccesRateOptLevel_3 = 99;
int SuccesRateOptLevel_4 = 75;
int SuccesRateOptLevel_5 = 50;
int SuccesRateOptLevel_6 = 20;
int SuccesRateOptLevel_7 = 10;
int SuccesRateOptLevel_8 = 8;
int SuccesRateOptLevel_9 = 5;
int SuccesRateOptLevel_10 = 2;
int SuccesRateOptLevel_11 = 1;
int SuccesRateOptLevel_12 = 1;
int SuccesRateOptLevel_13 = 1;
int SuccesRateOptLevel_14 = 1;
int SuccesRateOptLevel_15 = 1;
enum eAlchemyResultType
{
    SUCCESS = 0,
    FAILED = 1,
    NOTHING = 2,
};
enum eAlchemyType
{
    WITH_PROOF = 0,
    WITHOUT_ROOF = 1
};

#define SEND_SHARD_TO_LOCK_INFO 0x5060
#define SEND_SHARD_TO_UNLOCK_INFO 0x5062
#define TYPE_OF_ITEM_LOCKED 0
#define TYPE_OF_WRONG_PASSWORD 1
#define ITEM_LOCK_RESULT_PACKET 0x5032

#define GRANT_NAME_PACKET 0x3500
#define CHANGE_TITLE_PACKET 0x3501
#define CHANGE_PVP_CAPE_PACKET 0x3502
#define USE_REVERSE_CUSTOM_PACKET 0x3504
#define ITEM_TRANSLATE_PACKET 0x3505
#define GIVE_LIVE_ITEM_PACKET 0x3506
#define FELLOW_SKILL_PACKET 0x3511
#define SILK_PACKET 0x3527
#define COS_SKILL_PACKET 0x3528
#define CUSTOM_SCROLL_USE_PACKET 0x3530
#define ITEM_LOCK_PACKET 0x3531
#define ITEM_UNLOCK_PACKET 0x3532
#define ALCHEMY_LINK_PACKET 0x3533
#define NEW_ALCHEMY_PACKET 0x3534
#define DISPLAY_CHAR_INFO_PACKET 0x3537
#define SELF_TELEPORT_PACKET 0x3539
#define FILTER_TELEPORT_PACKET 0x3540
#define SILK_STALL_PREPARE_BUY_PACKET 0x3541
#define PVP_CHALLENGE_GOLD_SYNC_PACKET 0x3542
#define FILTER_RETURN_TO_TOWN_PACKET 0x3543
#define GLOBAL_ITEM_LINK 0x705C
#define CLIENT_REINFORCE_REQUEST 0x7150

#define REGISTER_FILTER_SESSION_KEY_PACKET 0x35FE

namespace {
    struct FilterSessionKeyEntry
    {
        std::string key;
        const CGObjPC* owner;
        DWORD gameId;

        FilterSessionKeyEntry()
            : owner(NULL), gameId(0)
        {
        }
    };

    class ScopedCriticalSection
    {
    public:
        explicit ScopedCriticalSection(CRITICAL_SECTION& criticalSection)
            : m_criticalSection(criticalSection)
        {
            EnterCriticalSection(&m_criticalSection);
        }

        ~ScopedCriticalSection()
        {
            LeaveCriticalSection(&m_criticalSection);
        }

    private:
        CRITICAL_SECTION& m_criticalSection;
        ScopedCriticalSection(const ScopedCriticalSection&);
        ScopedCriticalSection& operator=(const ScopedCriticalSection&);
    };

    std::map<int, FilterSessionKeyEntry> g_FilterSessionKeys;
    std::set<INT64> g_PendingItemLockOperations;
    struct SilkStallBuyPreparation
    {
        __int64 transactionId;
        DWORD sellerGameId;
        BYTE stallSlot;
        __int64 silkPrice;
        const CGObjPC* owner;
        DWORD buyerGameId;
        DWORD createdTick;

        SilkStallBuyPreparation()
            : transactionId(0), sellerGameId(0), stallSlot(0), silkPrice(0),
              owner(NULL), buyerGameId(0), createdTick(0)
        {
        }
    };

    std::map<int, SilkStallBuyPreparation> g_SilkStallBuyPreparations;
    CRITICAL_SECTION g_FilterSessionKeysLock;

    struct FilterSessionKeyStoreInitializer {
        FilterSessionKeyStoreInitializer() {
            InitializeCriticalSection(&g_FilterSessionKeysLock);
        }
        ~FilterSessionKeyStoreInitializer() {
            DeleteCriticalSection(&g_FilterSessionKeysLock);
        }
    } g_FilterSessionKeyStoreInitializer;

    bool IsHexKey(const std::string& key) {
        if (key.length() != 64)
            return false;
        for (size_t i = 0; i < key.length(); ++i) {
            const char c = key[i];
            if (!((c >= '0' && c <= '9') ||
                  (c >= 'A' && c <= 'F') ||
                  (c >= 'a' && c <= 'f')))
                return false;
        }
        return true;
    }

    class ScopedItemLockOperation
    {
    public:
        explicit ScopedItemLockOperation(INT64 itemId)
            : m_itemId(itemId), m_acquired(false)
        {
            ScopedCriticalSection guard(g_FilterSessionKeysLock);
            m_acquired = g_PendingItemLockOperations.insert(itemId).second;
        }

        ~ScopedItemLockOperation()
        {
            if (!m_acquired) return;
            ScopedCriticalSection guard(g_FilterSessionKeysLock);
            g_PendingItemLockOperations.erase(m_itemId);
        }

        bool Acquired() const { return m_acquired; }

    private:
        INT64 m_itemId;
        bool m_acquired;
        ScopedItemLockOperation(const ScopedItemLockOperation&);
        ScopedItemLockOperation& operator=(const ScopedItemLockOperation&);
    };

    void SetFilterSessionKey(CGObjPC* player, const std::string& key) {
        if (player == NULL || player->GetDBID() <= 0)
            return;

        FilterSessionKeyEntry entry;
        entry.key = key;
        entry.owner = player;
        entry.gameId = player->GetGameID();

        size_t cacheSize = 0;
        {
            ScopedCriticalSection guard(g_FilterSessionKeysLock);
            g_FilterSessionKeys[player->GetDBID()] = entry;
            cacheSize = g_FilterSessionKeys.size();
        }
        GameServerTelemetry::SetSessionKeyCacheSize(cacheSize);
    }

    bool ValidateFilterSessionKey(CGObjPC* player, const std::string& key) {
        if (player == NULL || player->GetDBID() <= 0)
            return false;

        bool valid = false;
        ScopedCriticalSection guard(g_FilterSessionKeysLock);
        std::map<int, FilterSessionKeyEntry>::const_iterator it =
            g_FilterSessionKeys.find(player->GetDBID());
        if (it != g_FilterSessionKeys.end() &&
            it->second.owner == player &&
            it->second.gameId == player->GetGameID() &&
            it->second.key.size() == key.size())
        {
            unsigned char difference = 0;
            for (size_t index = 0; index < key.size(); ++index)
                difference |= static_cast<unsigned char>(it->second.key[index] ^ key[index]);
            valid = difference == 0;
        }
        return valid;
    }

    void RemoveFilterSessionKey(CGObjPC* player) {
        if (player == NULL)
            return;

        const int charId = player->GetDBID();
        size_t cacheSize = 0;
        {
            ScopedCriticalSection guard(g_FilterSessionKeysLock);
            if (charId > 0)
            {
                std::map<int, FilterSessionKeyEntry>::iterator entry =
                    g_FilterSessionKeys.find(charId);
                if (entry != g_FilterSessionKeys.end() &&
                    entry->second.owner == player &&
                    entry->second.gameId == player->GetGameID())
                {
                    g_FilterSessionKeys.erase(entry);
                }
            }
            g_SilkStallBuyPreparations.erase(charId);
            cacheSize = g_FilterSessionKeys.size();
        }
        GameServerTelemetry::SetSessionKeyCacheSize(cacheSize);
    }

    bool SetSilkStallBuyPreparation(CGObjPC* buyer, __int64 transactionId,
                                    DWORD sellerGameId, BYTE stallSlot, int silkPrice)
    {
        if (buyer == NULL || buyer->GetDBID() <= 0 || transactionId <= 0 ||
            sellerGameId == 0 || sellerGameId == buyer->GetGameID() ||
            stallSlot > 9 || silkPrice <= 0)
            return false;

        SilkStallBuyPreparation preparation;
        preparation.transactionId = transactionId;
        preparation.sellerGameId = sellerGameId;
        preparation.stallSlot = stallSlot;
        preparation.silkPrice = silkPrice;
        preparation.owner = buyer;
        preparation.buyerGameId = buyer->GetGameID();
        preparation.createdTick = GetTickCount();

        ScopedCriticalSection guard(g_FilterSessionKeysLock);
        g_SilkStallBuyPreparations[buyer->GetDBID()] = preparation;
        return true;
    }

    bool TakeSilkStallBuyPreparation(CGObjPC* buyer, BYTE requestedSlot,
                                     SilkStallBuyPreparation& preparation, bool& hadPreparation)
    {
        hadPreparation = false;
        if (buyer == NULL || buyer->GetDBID() <= 0)
            return false;

        ScopedCriticalSection guard(g_FilterSessionKeysLock);
        std::map<int, SilkStallBuyPreparation>::iterator entry =
            g_SilkStallBuyPreparations.find(buyer->GetDBID());
        if (entry == g_SilkStallBuyPreparations.end())
            return false;

        hadPreparation = true;
        preparation = entry->second;
        g_SilkStallBuyPreparations.erase(entry);

        return preparation.owner == buyer &&
               preparation.buyerGameId == buyer->GetGameID() &&
               preparation.stallSlot == requestedSlot &&
               GetTickCount() - preparation.createdTick <= 15000;
    }

    void RestoreGold(CGObjPC* player, UINT64 originalGold)
    {
        if (player == NULL || player->m_pObjDataInstance == NULL)
            return;

        const UINT64 currentGold = player->m_pObjDataInstance->Gold;
        if (currentGold > originalGold)
            player->RemoveGold(static_cast<__int64>(currentGold - originalGold));
        else if (currentGold < originalGold)
            player->UpdateGold(static_cast<__int64>(originalGold - currentGold));
    }

    class ScopedSilkStallGoldNeutralizer
    {
    public:
        ScopedSilkStallGoldNeutralizer(CGObjPC* buyer, CGObjPC* seller)
            : m_buyer(buyer), m_seller(seller), m_buyerGold(0), m_sellerGold(0), m_active(false)
        {
            if (buyer != NULL && seller != NULL &&
                buyer->m_pObjDataInstance != NULL && seller->m_pObjDataInstance != NULL)
            {
                m_buyerGold = buyer->m_pObjDataInstance->Gold;
                m_sellerGold = seller->m_pObjDataInstance->Gold;
                m_active = true;
            }
        }

        ~ScopedSilkStallGoldNeutralizer()
        {
            if (m_active)
            {
                RestoreGold(m_buyer, m_buyerGold);
                RestoreGold(m_seller, m_sellerGold);
            }
        }

        bool CanCredit(__int64 amount) const
        {
            return m_active && amount > 0 &&
                   m_buyerGold <= static_cast<UINT64>(0x7FFFFFFFFFFFFFFFLL - amount);
        }

    private:
        CGObjPC* m_buyer;
        CGObjPC* m_seller;
        UINT64 m_buyerGold;
        UINT64 m_sellerGold;
        bool m_active;
        ScopedSilkStallGoldNeutralizer(const ScopedSilkStallGoldNeutralizer&);
        ScopedSilkStallGoldNeutralizer& operator=(const ScopedSilkStallGoldNeutralizer&);
    };

    CGItem* GetInventoryItemSafe(CGObjPC* player, int slot)
    {
        if (player == NULL || slot < 0 || slot >= player->m_PCInventory.m_nSlotsCount)
            return NULL;
        return player->GetItemChar(slot);
    }

    bool IsUsableItem(CGItem* item)
    {
        return item != NULL &&
               item->InstanceItem != NULL &&
               item->InstanceItem->pCRefObjItem != NULL;
    }

    bool TryGetMessagePayload(CMsg* message, const char*& payload, int& length)
    {
        payload = NULL;
        length = 0;
        if (message == NULL ||
            message->m_pMsgBuffer == NULL ||
            message->m_wWriteDataArrayPos < MSG_HEADER_SIZE ||
            message->m_wWriteDataArrayPos > message->m_dwArrayDataSize)
            return false;

        payload = message->m_pMsgBuffer + MSG_HEADER_SIZE;
        length = static_cast<int>(message->m_wWriteDataArrayPos - MSG_HEADER_SIZE);
        return true;
    }

    bool IsInventoryItemLocked(CGObjPC* player, int slot)
    {
        CGItem* item = GetInventoryItemSafe(player, slot);
        return item != NULL && CSqlCon::IsItemLocked(item->ID64);
    }

    void SendItemStateNotice(CGObjPC* player, BYTE noticeType)
    {
        if (player == NULL)
            return;
        CMsg* message = player->AllocMsg(0x5015);
        if (message == NULL)
            return;
        *message << noticeType;
        player->SendMsg(message);
    }

    void SendItemLockFailure(CGObjPC* player, BYTE operation, BYTE result, BYTE slot)
    {
        if (player == NULL) return;
        CMsg* message = player->AllocMsg(ITEM_LOCK_RESULT_PACKET);
        if (message == NULL) return;
        *message << operation << result << slot;
        player->SendMsg(message);
    }
}

void CGObjPC::OnDeleteObjectCustom()
{
    RemoveFilterSessionKey(this);
    CItemRegionTravelGuard::ForgetPlayer(this);
    CRegionAttackRestrictionsMgr::ForgetPlayer(this);
    reinterpret_cast<void(__thiscall*)(CGObjPC*)>(0x004DE9B0)(this);
}

int GetDegreeLevel(int itemClass) {
    return ((itemClass - 1) / 3 + 1);
}
void CGObjPC::ReaderPacket(CMsg* pMsg) {
    if (pMsg == NULL ||
        pMsg->m_wpMsgId == NULL ||
        pMsg->m_pMsgBuffer == NULL ||
        pMsg->m_wReadDataArrayPos > pMsg->m_wWriteDataArrayPos ||
        pMsg->m_wWriteDataArrayPos > pMsg->m_dwArrayDataSize)
    {
        GameServerTelemetry::RecordMalformedPacket();
        return;
    }

    GameServerTelemetry::ScopedPacketTimer telemetryTimer(true, pMsg->GetMsgId());
    try
    {

    if (*pMsg->m_wpMsgId == 0x704C &&
        !CItemRegionTravelGuard::InspectItemUse(this, pMsg))
        return;

    if (*pMsg->m_wpMsgId == REGISTER_FILTER_SESSION_KEY_PACKET) {
        const DWORD registrationSize = 1 + 4 + 8 + 16 + 32 + 32;
        if (pMsg->m_wWriteDataArrayPos - pMsg->m_wReadDataArrayPos != registrationSize)
        {
            GameServerTelemetry::RecordMalformedPacket();
            return;
        }
        BYTE version = 0;
        DWORD gameId = 0;
        __int64 issuedAt = 0;
        BYTE nonce[16] = { 0 };
        BYTE sessionKeyBytes[32] = { 0 };
        BYTE mac[32] = { 0 };
        *pMsg >> version >> gameId >> issuedAt;
        pMsg->ReadBytes(nonce, sizeof(nonce));
        pMsg->ReadBytes(sessionKeyBytes, sizeof(sessionKeyBytes));
        pMsg->ReadBytes(mac, sizeof(mac));

        const InternalPacketAuth::ValidationResult authResult =
            InternalPacketAuth::ValidateRegistration(
                this->GetGameID(), version, gameId, issuedAt,
                nonce, sessionKeyBytes, mac);
        if (authResult == InternalPacketAuth::AUTH_VALID)
            SetFilterSessionKey(this, InternalPacketAuth::ToHex(sessionKeyBytes, sizeof(sessionKeyBytes)));
        else
        {
            GameServerTelemetry::RecordPacketAuthFailure(static_cast<int>(authResult));
            GameServerTelemetry::RecordMalformedPacket();
        }
        SecureZeroMemory(sessionKeyBytes, sizeof(sessionKeyBytes));
        SecureZeroMemory(mac, sizeof(mac));
        return;
    }
    if (*pMsg->m_wpMsgId == GRANT_NAME_PACKET) {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if(ValidateFilterSessionKey(this, Key))
        {
            HandleGrantNameRequest(pMsg);
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == CHANGE_TITLE_PACKET) {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if(ValidateFilterSessionKey(this, Key)) {
            HandleTitleChangeRequest(pMsg);
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == CHANGE_PVP_CAPE_PACKET) {

        std::string Key;
        pMsg->ReadString(Key, 64);
        if(ValidateFilterSessionKey(this, Key)) {
            HandleChangePvpCapeRequest(pMsg);

        }
        return;
    }
    else if (*pMsg->m_wpMsgId == USE_REVERSE_CUSTOM_PACKET)
    {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if(ValidateFilterSessionKey(this, Key)) {
            HandleCustomReverseUseRequest(pMsg);
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == ITEM_TRANSLATE_PACKET) {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if(ValidateFilterSessionKey(this, Key)) {
            HandleItemTranslateRequest(pMsg);
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == GIVE_LIVE_ITEM_PACKET) {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if(ValidateFilterSessionKey(this, Key)) {
            HandleLiveItemChestPacket(pMsg);
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == FELLOW_SKILL_PACKET) {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if(ValidateFilterSessionKey(this, Key)) {
            HandleFellowSkill(pMsg);
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == SILK_PACKET) {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if(ValidateFilterSessionKey(this, Key)) {
            HandleSilkPacket(pMsg);
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == COS_SKILL_PACKET)
    {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if(ValidateFilterSessionKey(this, Key)) {
            HandleCosSkill(pMsg);
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == CUSTOM_SCROLL_USE_PACKET)
    {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if(ValidateFilterSessionKey(this, Key)) {
            HandleCustomScrollUsage(pMsg);
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == ITEM_LOCK_PACKET)
    {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if(ValidateFilterSessionKey(this, Key)) {
            HandleItemLockRequest(pMsg);
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == ITEM_UNLOCK_PACKET)
    {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if(ValidateFilterSessionKey(this, Key)) {
            HandleItemUnlockRequest(pMsg);
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == ALCHEMY_LINK_PACKET)
    {   std::string Key;
        pMsg->ReadString(Key, 64);
        if(ValidateFilterSessionKey(this, Key)) {
            HandleAlchemyLinkRequest(pMsg);
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == NEW_ALCHEMY_PACKET) {
        // The custom New Alchemy pipeline is retired. Consume the private opcode
        // here so no Filter version or direct packet can reach its old SQL-heavy
        // implementation. Original vSRO alchemy opcodes remain unchanged.
        return;
    }
    else if (*pMsg->m_wpMsgId == 0x3538)
    {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if(ValidateFilterSessionKey(this, Key)) {
            byte uniquetype;
            *pMsg >> uniquetype;
            SPosInfo minePos;
            this->GetPosInfo(minePos);
            SWorldID mineWorldID;
            this->GetWorldID(mineWorldID);

            if (uniquetype == 0) {
                CGObjMob::CreateMob(46406, mineWorldID.dwWorldID, (uint16_t) minePos.wRegionID, (float) minePos.fltX,
                                    (float) minePos.fltY, (float) minePos.fltZ, (float) 5);
            } else if (uniquetype == 1) {
                CGObjMob::CreateMob(46407, mineWorldID.dwWorldID, (uint16_t) minePos.wRegionID, (float) minePos.fltX,
                                    (float) minePos.fltY, (float) minePos.fltZ, (float) 5);
            } else if (uniquetype == 2) {
                CGObjMob::CreateMob(46408, mineWorldID.dwWorldID, (uint16_t) minePos.wRegionID, (float) minePos.fltX,
                                    (float) minePos.fltY, (float) minePos.fltZ, (float) 5);
            } else if (uniquetype == 3) {
                CGObjMob::CreateMob(46409, mineWorldID.dwWorldID, (uint16_t) minePos.wRegionID, (float) minePos.fltX,
                                    (float) minePos.fltY, (float) minePos.fltZ, (float) 5);
            } else if (uniquetype == 4) {
                CGObjMob::CreateMob(46410, mineWorldID.dwWorldID, (uint16_t) minePos.wRegionID, (float) minePos.fltX,
                                    (float) minePos.fltY, (float) minePos.fltZ, (float) 5);
            }
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == DISPLAY_CHAR_INFO_PACKET)
    {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if(ValidateFilterSessionKey(this, Key)) {
            HandleDisplayCharInfoRequest(pMsg);
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == SELF_TELEPORT_PACKET)
    {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if (ValidateFilterSessionKey(this, Key)) {
            HandleSelfTeleportRequest();
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == FILTER_RETURN_TO_TOWN_PACKET)
    {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if (ValidateFilterSessionKey(this, Key)) {
            this->TeleportToTown();
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == FILTER_TELEPORT_PACKET)
    {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if (ValidateFilterSessionKey(this, Key)) {
            HandleFilterTeleportRequest(pMsg);
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == SILK_STALL_PREPARE_BUY_PACKET)
    {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if (ValidateFilterSessionKey(this, Key))
        {
            __int64 transactionId = 0;
            DWORD sellerGameId = 0;
            BYTE stallSlot = 0;
            int silkPrice = 0;
            *pMsg >> transactionId >> sellerGameId >> stallSlot >> silkPrice;
            SetSilkStallBuyPreparation(this, transactionId, sellerGameId, stallSlot, silkPrice);
        }
        return;
    }
    else if (*pMsg->m_wpMsgId == PVP_CHALLENGE_GOLD_SYNC_PACKET)
    {
        std::string Key;
        pMsg->ReadString(Key, 64);
        if (ValidateFilterSessionKey(this, Key))
            HandlePvpChallengeGoldSyncRequest(pMsg);
        return;
    }
    else if (*pMsg->m_wpMsgId == 0x70B4)
    {
        const int originalReadPosition = pMsg->m_wReadDataArrayPos;
        BYTE requestedSlot = 0;
        *pMsg >> requestedSlot;
        pMsg->m_wReadDataArrayPos = originalReadPosition;

        SilkStallBuyPreparation preparation;
        bool hadPreparation = false;
        if (TakeSilkStallBuyPreparation(this, requestedSlot, preparation, hadPreparation))
        {
            IGObj* sellerObject = g_pCGame != NULL
                ? g_pCGame->GetObjByGameID(preparation.sellerGameId)
                : NULL;
            if (sellerObject == NULL || !sellerObject->IsPC())
                return;

            CGObjPC* seller = static_cast<CGObjPC*>(sellerObject);
            ScopedSilkStallGoldNeutralizer neutralizer(this, seller);
            if (!neutralizer.CanCredit(preparation.silkPrice))
                return;

            // The original stall code still validates and transfers the item.
            // Gold exists only for the duration of that synchronous call; the
            // scope guard restores both exact balances on every exit path.
            this->UpdateGold(preparation.silkPrice);
            reinterpret_cast<void(__thiscall*)(CGObjPC*, CMsg*)>(0x0050eee0)(this, pMsg);
            return;
        }

        // A consumed but invalid/expired private preparation must never fall
        // through as a normal Gold purchase.
        if (hadPreparation)
            return;
    }
    else if (*pMsg->m_wpMsgId == 0x705D)
    {
        byte eChatType;
        pMsg->Read<BYTE>(eChatType);

        byte btChatIndex;
        pMsg->Read<BYTE>(btChatIndex);

        std::string strReceiverName;
        if(eChatType == PRIVATE)
        {
            pMsg->ReadString(strReceiverName, 64);
        }

        std::string strAsciiText;
        pMsg->ReadString(strAsciiText, 1024);

        byte LinkedItemSlot;
        pMsg->Read<BYTE>(LinkedItemSlot);


        int dbid = this->GetDBID();
        CMsg* pShardMsgFirst = NEWMSG(0x5068, false);
        if (pShardMsgFirst == NULL)
            return;
        pShardMsgFirst->Write<int>(dbid);
        pShardMsgFirst->Write<byte>(eChatType);
        pShardMsgFirst->Write<byte>(btChatIndex);
        if(eChatType == PRIVATE) {
            pShardMsgFirst->WriteString(strReceiverName);
        }
        pShardMsgFirst->WriteString(strAsciiText);
        pShardMsgFirst->Write<byte>(LinkedItemSlot);
        CNetHelper::SendMsgToSM(pShardMsgFirst);

        return;
    }
    else if (*pMsg->m_wpMsgId == GLOBAL_ITEM_LINK)
    {
        HandleGlobalItemLink(pMsg);

        return;
    }
    else if (*pMsg->m_wpMsgId == 0x7150)
    {
        int m_wReadDataArrayPos1 = pMsg->m_wReadDataArrayPos;
        byte type;

        *pMsg >> type;
        if (type == 2)
        {
            byte unk1;
            byte unk2;
            byte ItemSlot;
            byte StoneSlot;
            *pMsg >> unk1 >> unk2 >> ItemSlot >> StoneSlot;
            if (IsInventoryItemLocked(this, ItemSlot))
            {
                SendItemStateNotice(this, TYPE_OF_ITEM_LOCKED);
                return;
            }
        }
        pMsg->m_wReadDataArrayPos = m_wReadDataArrayPos1;
    }
    else if (*pMsg->m_wpMsgId == 0x716A)
    {
        int m_wReadDataArrayPos1 = pMsg->m_wReadDataArrayPos;
        byte type;
        *pMsg >> type;
        if (type == 1)
        {
            byte unk2;
            byte ItemSlot;
            byte StoneSlot;
            *pMsg >> unk2 >> ItemSlot >> StoneSlot;
            if (IsInventoryItemLocked(this, ItemSlot))
            {
                SendItemStateNotice(this, TYPE_OF_ITEM_LOCKED);
                return;
            }
        }
        else if (type == 2)
        {
            byte unk2;
            byte ItemSlot;
            byte StoneSlot;
            byte unks;
            *pMsg >> unk2 >> ItemSlot >> StoneSlot >> unks;
            if (IsInventoryItemLocked(this, ItemSlot))
            {
                SendItemStateNotice(this, TYPE_OF_ITEM_LOCKED);
                return;
            }
        }
        else if (type == 3)
        {
            byte unk2;
            byte ItemSlot;
            byte StoneSlot;
            *pMsg >> unk2 >> ItemSlot >> StoneSlot;
            if (IsInventoryItemLocked(this, ItemSlot))
            {
                SendItemStateNotice(this, TYPE_OF_ITEM_LOCKED);
                return;
            }
        }
        pMsg->m_wReadDataArrayPos = m_wReadDataArrayPos1;

    }
    else if (*pMsg->m_wpMsgId == 0x7151)
    {
        int m_wReadDataArrayPos1 = pMsg->m_wReadDataArrayPos;
        byte type;

        *pMsg >> type;
        if (type == 2)
        {
            byte unk1;
            byte unk2;
            byte ItemSlot;
            byte StoneSlot;
            *pMsg >> unk1 >> unk2 >> ItemSlot >> StoneSlot;
            if (IsInventoryItemLocked(this, ItemSlot))
            {
                SendItemStateNotice(this, TYPE_OF_ITEM_LOCKED);
                return;
            }
        }

        pMsg->m_wReadDataArrayPos = m_wReadDataArrayPos1;
    }
    else if (*pMsg->m_wpMsgId == 0x7157)
    {
        int m_wReadDataArrayPos1 = pMsg->m_wReadDataArrayPos;
        byte type;

        *pMsg >> type;
        if (type == 1)
        {

            byte ItemSlot;

            *pMsg >> ItemSlot;
            if (IsInventoryItemLocked(this, ItemSlot))
            {
                SendItemStateNotice(this, TYPE_OF_ITEM_LOCKED);
                return;
            }
        }

        pMsg->m_wReadDataArrayPos = m_wReadDataArrayPos1;
    }
    else if (*pMsg->m_wpMsgId == 0x7034)
    {
        int m_wReadDataArrayPos1 = pMsg->m_wReadDataArrayPos;

        E_INVENTORY_OP_TYPE type;
        *pMsg >> type;
        switch (type) {
            case E_INVENTORY_OP_TYPE::DEPOSIT_ITEM:
            {
                byte slot;
                byte slot_to;
                int movingcount;
                *pMsg >> slot >> slot_to >> movingcount;
                if (IsInventoryItemLocked(this, slot))
                {
                    SendItemStateNotice(this, TYPE_OF_ITEM_LOCKED);
                    return;
                }
            }
                break;
            case E_INVENTORY_OP_TYPE::ADD_EXCHANGE:
            {
                byte slot;
                *pMsg >> slot;
                if (IsInventoryItemLocked(this, slot))
                {
                    SendItemStateNotice(this, TYPE_OF_ITEM_LOCKED);
                    return;
                }
            }
                break;
            case E_INVENTORY_OP_TYPE::DROP_ITEM:
            {
                byte slot;
                *pMsg >> slot;
                if (IsInventoryItemLocked(this, slot))
                {
                    SendItemStateNotice(this, TYPE_OF_ITEM_LOCKED);
                    return;
                }
            }
                break;
            case E_INVENTORY_OP_TYPE::SELL_ITEM:
            {
                byte slot;
                byte slot_to;
                int movingcount;
                *pMsg >> slot >> slot_to >> movingcount;
                if (IsInventoryItemLocked(this, slot))
                {
                    SendItemStateNotice(this, TYPE_OF_ITEM_LOCKED);
                    return;
                }
            }
                break;

            case E_INVENTORY_OP_TYPE::MOVE_ITEM_PC_PET:
            {
                int petuqID;
                byte slot;
                byte petslot;
                *pMsg >> petuqID >> slot >> petslot;
                if (IsInventoryItemLocked(this, slot))
                {
                    SendItemStateNotice(this, TYPE_OF_ITEM_LOCKED);
                    return;
                }
            }
                break;

            case E_INVENTORY_OP_TYPE::PICK_ITEM_BY_OTHER:
            {
                byte slot;
                byte slot_to;
                int pp;
                *pMsg >> slot >> slot_to >> pp;
                if (IsInventoryItemLocked(this, slot))
                {
                    SendItemStateNotice(this, TYPE_OF_ITEM_LOCKED);
                    return;
                }
            }
                break;

            case E_INVENTORY_OP_TYPE::GUILD_CHEST_DEPOSIT_ITEM:
            {
                byte slot;
                byte slot_to;
                int pp;
                *pMsg >> slot >> slot_to >> pp;
                if (IsInventoryItemLocked(this, slot))
                {
                    SendItemStateNotice(this, TYPE_OF_ITEM_LOCKED);
                    return;
                }
            }
                break;
        }

        pMsg->m_wReadDataArrayPos = m_wReadDataArrayPos1;
    }
    else if(*pMsg->m_wpMsgId == 0x70BA)
    {
        int m_wReadDataArrayPos1 = pMsg->m_wReadDataArrayPos;
        byte updateType;
        *pMsg >> updateType;
        if(updateType == 2)
        {
            byte Slot;    //within Stall (0-9)
            byte SourceSlot; //from ownerInventory
            unsigned short StackCount;
            INT64 Price;
            unsigned int FleaMarketNetworkTidGroup;
            unsigned short unkUShort0;
            *pMsg >> Slot;
            *pMsg >> SourceSlot;
            *pMsg >> StackCount;
            *pMsg >> Price;
            *pMsg >> FleaMarketNetworkTidGroup;
            *pMsg >> unkUShort0;
            if (IsInventoryItemLocked(this, SourceSlot))
            {
                SendItemStateNotice(this, TYPE_OF_ITEM_LOCKED);
                return;
            }
        }


        pMsg->m_wReadDataArrayPos = m_wReadDataArrayPos1;
    }
    else if (*pMsg->m_wpMsgId == 0x704F)
    {
        int m_wReadDataArrayPos1 = pMsg->m_wReadDataArrayPos;
        byte state;
        *pMsg >> state;
        if (state == 2)
        {
            if (GoldPetPtr != NULL)
            {

                return;

            }
        }
        pMsg->m_wReadDataArrayPos = m_wReadDataArrayPos1;
    }

        reinterpret_cast<void(__thiscall*)(CGObjPC*, CMsg*)>(0x0050eee0)(this, pMsg);
    }
    catch (...)
    {
        GameServerTelemetry::RecordMalformedPacket();
        pMsg->FlushRemainingBytes();
    }
}
void CGObjPC::HandleGlobalItemLink(CMsg* pMsg)
{
    byte GlobalType;
    pMsg->Read<BYTE>(GlobalType);

    byte GlobalSlot;
    pMsg->Read<BYTE>(GlobalSlot);

    USHORT GlobalItemType;
    pMsg->Read<USHORT>(GlobalItemType);

    int GlobalItemID;
    pMsg->Read<int>(GlobalItemID);

    std::wstring Message;
    pMsg->ReadStringW(Message, 1024);

    if (GlobalType > 1)
        return;

    if (GlobalType == 0)
    {
        CMsg* NewMsg = this->AllocMsg(0x5033);
        if (NewMsg == NULL)
            return;
        NewMsg->Write<BYTE>(GlobalType);
        NewMsg->Write<BYTE>(GlobalSlot);

        NewMsg->Write<USHORT>(GlobalItemType);
        NewMsg->Write<int>(GlobalItemID);
        NewMsg->WriteStringW(Message);
        this->SendMsg(NewMsg);
    }
    else if (GlobalType == 1)
    {
        byte ItemSlot;
        *pMsg >> ItemSlot;
        CGItem* pItem = GetInventoryItemSafe(this, ItemSlot);

        if (IsUsableItem(pItem))
        {
            CMsg* pTmpMsg = CNetHelper::AllocMsg(0x0000, false);
            if (pTmpMsg == NULL)
                return;
            CNetHelper::BindStreamBufferWithMsg(pTmpMsg);
            if ((pItem->InstanceItem->pCRefObjItem->TID.getTypeID1() == 3 && pItem->InstanceItem->pCRefObjItem->TID.getTypeID2() == 2
                 && pItem->InstanceItem->pCRefObjItem->TID.getTypeID3() == 1 && pItem->InstanceItem->pCRefObjItem->TID.getTypeID4() == 1) ||
                (pItem->InstanceItem->pCRefObjItem->TID.getTypeID1() == 3 && pItem->InstanceItem->pCRefObjItem->TID.getTypeID2() == 3
                 && pItem->InstanceItem->pCRefObjItem->TID.getTypeID3() == 3 && pItem->InstanceItem->pCRefObjItem->TID.getTypeID4() == 7)
                || (pItem->InstanceItem->pCRefObjItem->TID.getTypeID1() == 3 && pItem->InstanceItem->pCRefObjItem->TID.getTypeID2() == 2
                    && pItem->InstanceItem->pCRefObjItem->TID.getTypeID3() == 1 && pItem->InstanceItem->pCRefObjItem->TID.getTypeID4() == 2))
            {
                pItem->WriteItemCosDataMsg((void*)g_CStreamBufferUnk, 1);
            }
            else
            {
                pItem->WriteItemDataToMsg((void*)g_CStreamBufferUnk, 1);
            }

            CNetHelper::FlushStreamBufferMsg(pTmpMsg);
            const char* itemPayload = NULL;
            int nLen = 0;
            if (!TryGetMessagePayload(pTmpMsg, itemPayload, nLen))
            {
                CNetHelper::FreeMsg(pTmpMsg);
                return;
            }

            CMsg* pSmMsg = this->AllocMsg(0x5033);
            if (pSmMsg == NULL)
            {
                CNetHelper::FreeMsg(pTmpMsg);
                return;
            }
            pSmMsg->Write<BYTE>(GlobalType);
            pSmMsg->Write<BYTE>(GlobalSlot);

            pSmMsg->Write<USHORT>(GlobalItemType);
            pSmMsg->Write<int>(GlobalItemID);
            pSmMsg->WriteStringW(Message);

            pSmMsg->Write<int>(nLen);
            if (nLen > 0)
                pSmMsg->Write(itemPayload, static_cast<size_t>(nLen));
            *pSmMsg << pItem->InstanceItem->RefItemID;
            this->SendMsg(pSmMsg);

            CNetHelper::FreeMsg(pTmpMsg);

        }
    }
}
void CGObjPC::HandleGrantNameRequest(CMsg* pMsg)
{
    std::string NewGrantName;
    pMsg->ReadString(NewGrantName, 64);
    if (this->MyGuild != NULL && this->MyGuild->IntanceGuild != NULL)
    {
        if (this->MyGuild->IntanceGuild->GuildLevel >= 4)
        {
            this->SetGrantName(&NewGrantName);
        }
    }

}
void CGObjPC::HandleTitleChangeRequest(CMsg* pMsg)
{
    BYTE TitleID;
    *pMsg >> TitleID;

    if (this->m_pLifeState == NULL) {
        return;
    }
    if (this->m_pLifeState->m_btBodyState == BODYMODE_HWAN) {
    }
    else {
        this->UpdateHwan(TitleID);
    }
}
void CGObjPC::HandleChangePvpCapeRequest(CMsg* pMsg)
{
    BYTE Cape;
    *pMsg >> Cape;

    if (this->CanSendMessage()) {

        this->UpdatePVPCapeType(Cape);
    }
}
void CGObjPC::HandleCustomReverseUseRequest(CMsg* pMsg) {
    int SlotID;
    *pMsg >> SlotID;

    BYTE WorldID;
    *pMsg >> WorldID;
    unsigned short wRegionID;
    *pMsg >> wRegionID;

    float X;
    float Y;
    float Z;
    *pMsg >> X;
    *pMsg >> Y;
    *pMsg >> Z;

    BYTE admissionApproved = 0;
    *pMsg >> admissionApproved;

    if (admissionApproved != 1 || WorldID == 0 || wRegionID == 0 ||
        !_finite(X) || !_finite(Y) || !_finite(Z) ||
        X < -1000000.0f || X > 1000000.0f ||
        Y < -1000000.0f || Y > 1000000.0f ||
        Z < -1000000.0f || Z > 1000000.0f)
        return;

    if (this->CanSendMessage()) {

        CGItem* reverseItem = GetInventoryItemSafe(this, SlotID);
        if (IsUsableItem(reverseItem)) {
            if (reverseItem->InstanceItem->pCRefObjItem->TID.m_type_id_value == 6636 ||
                reverseItem->InstanceItem->pCRefObjItem->TID.m_type_id_value == 6637) {
                CMsg *pMsg32 = this->AllocMsg(0x305C);
                if (pMsg32 == NULL)
                    return;
                const uint32_t targetWorldId = static_cast<uint32_t>(WorldID) + 0x10000;

                // Finish the Reverse item transaction before MoveTo starts the
                // world-transfer handshake.  Touching inventory state or sending
                // the use effect after mode 2 begins can leave a normal client in
                // its loading state and cause its Agent connection to be dropped.
                this->SetLiveDeleteItem(SlotID, 1);
                unsigned int pGameID = this->GetGameID();
                unsigned int ScID = 3769;
                *pMsg32 << pGameID; //flag opt lvl
                *pMsg32 << ScID;
                this->SendMsg(pMsg32);

                bool moved = this->MoveTo(targetWorldId, wRegionID, X, Y, Z, 2);
                if (!moved)
                    moved = this->MoveTo(targetWorldId, wRegionID, X, Y, Z, 1);
            }
        }
    }
}
void CGObjPC::HandleItemTranslateRequest(CMsg* pMsg)
{
    byte PaymentMethod;
    *pMsg >> PaymentMethod;

    byte ItemSlot;
    *pMsg >> ItemSlot;

    std::string ItemCodeName;
    std::string TargetItemCodeName;

    pMsg->ReadString(ItemCodeName, 128);
    pMsg->ReadString(TargetItemCodeName, 128);

    if (PaymentMethod > 1 ||
        ItemCodeName.empty() || ItemCodeName.length() > 128 ||
        TargetItemCodeName.empty() || TargetItemCodeName.length() > 128)
        return;

    int TargetGold;
    if(PaymentMethod == 1)
    {
        *pMsg >> TargetGold;
    }
    if (this->CanSendMessage()) {

        CGItem* sourceItem = GetInventoryItemSafe(this, ItemSlot);
        if(IsUsableItem(sourceItem))
        {
            if (CSqlCon::IsItemLocked(sourceItem->InstanceItem->ID64))
            {
                CMsg* pck = this->AllocMsg(0x5015);
                if (pck == NULL)
                    return;
                *pck << byte(TYPE_OF_ITEM_LOCKED);
                this->SendMsg(pck);
                return;
            }
            if(sourceItem->InstanceItem->pCRefObjItem->m_strObjectCode == ItemCodeName)
            {
                if(PaymentMethod == 1)
                {
                    if (this->m_pObjDataInstance == NULL || TargetGold <= 0)
                        return;
                    INT64 availablegold = this->m_pObjDataInstance->Gold;
                    if (availablegold >= TargetGold) {
                        this->RemoveGold(TargetGold);
                        this->SetLiveItem(ItemSlot, TargetItemCodeName.c_str());
                    }
                    else
                    {
                        CMsg* pck = this->AllocMsg(0x5015);
                        if (pck == NULL)
                            return;
                        *pck << byte(3);
                        this->SendMsg(pck);
                    }
                }
                else if(PaymentMethod == 0)
                {
                    this->SetLiveItem(ItemSlot, TargetItemCodeName.c_str());
                }

                /// SEND RESULT ?
            }
        }
    }
}
void CGObjPC::HandleLiveItemChestPacket(CMsg* pMsg)
{
    int ItemDBID;
    *pMsg >> ItemDBID;
    std::string ItemCodeName;
    pMsg->ReadString(ItemCodeName, 128);
    __int32 Amount;
    *pMsg >> Amount;

    bool RandomizeStats;
    *pMsg >> RandomizeStats;
    __int8 OptLevel;
    *pMsg >> OptLevel;

    byte response = 1;
    if (ItemDBID > 0 &&
        Amount > 0 &&
        Amount <= 1000000 &&
        !ItemCodeName.empty() &&
        ItemCodeName.length() <= 128 &&
        this->CanSendMessage())
    {
        char* itemCodeName = const_cast<char*>(ItemCodeName.c_str());
        unsigned __int16 addResult =
            this->AddItem(itemCodeName, Amount, RandomizeStats, OptLevel);
        if (addResult == 1)
            response = 0;
    }

    CMsg* resultMessage = this->AllocMsg(0xA405);
    if (resultMessage == NULL)
        return;
    *resultMessage << response << ItemDBID;
    this->SendMsg(resultMessage);
}
void CGObjPC::HandlePvpChallengeGoldSyncRequest(CMsg* pMsg)
{
    __int64 delta = 0;
    *pMsg >> delta;

    if (delta == 0 || this->m_pObjDataInstance == NULL || !this->CanSendMessage())
        return;

    const UINT64 currentGold = this->m_pObjDataInstance->Gold;
    const UINT64 debit = delta < 0
        ? static_cast<UINT64>(-(delta + 1)) + 1
        : 0;
    if ((delta > 0 && currentGold > static_cast<UINT64>(0x7FFFFFFFFFFFFFFFLL - delta)) ||
        (delta < 0 && currentGold < debit))
    {
        GameServerTelemetry::RecordMalformedPacket();
        return;
    }

    // The Filter has already committed the same delta atomically in the shard
    // database. Realtime=false updates the live object/client without applying
    // the database mutation a second time.
    this->UpdateGold(delta, 25, false, false);
}
void CGObjPC::HandleFellowSkill(CMsg* pMsg)
{
    int SkillID;
    *pMsg >> SkillID;
    unsigned int petuqID;
    *pMsg >> petuqID;
    byte AnimationID;
    *pMsg >> AnimationID;
    IGObj* pc = g_pCGame != NULL ? g_pCGame->GetObjByGameID(petuqID) : NULL;
    if (pc != NULL && pc->IsCOS() && pc->GetOwner() == this &&
        this->GoldPetPtr == reinterpret_cast<CGObjCOS_GoldPet*>(pc)) {
        if (this->CanSendMessage()) {


            std::list<CSkill*>::iterator it = pData.UsedSkillList.begin();
            for (; it != pData.UsedSkillList.end(); ++it) {
                if (*it == NULL)
                    continue;
                SSkillPreEngagementData* preEngagementData = (*it)->m_pSkillPreEngagementData;
                if (preEngagementData == NULL)
                    continue;
                if(SkillID == preEngagementData->m_nSkillId)
                    return;
            }

            CMsg* newmsg = CNetHelper::AllocMsg(0x182C, true);
            if (newmsg == NULL)
                return;
            *newmsg << petuqID;
            *newmsg << AnimationID;
            this->BroadcastMsgToNearbyPlayers(newmsg);

            this->LiveSkill(SkillID);
        }
    }
}
void CGObjPC::HandleSilkPacket(CMsg* pMsg)
{
	int silkOwn;
	int silkGift;
	int silkPoint;

	*pMsg >> silkOwn;
	*pMsg >> silkGift;
	*pMsg >> silkPoint;

    if (silkOwn < 0 || silkGift < 0 || silkPoint < 0)
        return;

	if (this->CanSendMessage()) {
		this->UpdateSilk(silkOwn, silkGift, silkPoint, true);
	}
}
void CGObjPC::HandleCosSkill(CMsg* pMsg)
{
    unsigned int PetUQID;
    int PetSkillID;
    *pMsg >> PetUQID >> PetSkillID;
    IGObj* petObject = g_pCGame != NULL ? g_pCGame->GetObjByGameID(PetUQID) : NULL;
    if (petObject != NULL && petObject->IsCOS() && petObject->GetOwner() == this &&
        this->GoldPetPtr == reinterpret_cast<CGObjCOS_GoldPet*>(petObject))
    {
        CGObjCOS_GoldPet* pc = reinterpret_cast<CGObjCOS_GoldPet*>(petObject);
        pc->LiveSkill2(PetSkillID);
    }
}
void CGObjPC::HandleCustomScrollUsage(CMsg * pMsg)
{
    int itemId;
    *pMsg >> itemId;

    int itemSlotId;
    *pMsg >> itemSlotId;

    unsigned short itemTypeId;
    *pMsg >> itemTypeId;

    CGItem* scrollItem = GetInventoryItemSafe(this, itemSlotId);
    if (IsUsableItem(scrollItem))
    {
        if (scrollItem->InstanceItem->RefItemID == itemId && scrollItem->InstanceItem->pCRefObjItem->TID.m_type_id_value == 0xC6ED)
        {
            this->SetLiveDeleteItem(itemSlotId, 1);

            CMsg* pMsgg = this->AllocMsg(0x210B);
            if (pMsgg == NULL)
                return;
            *pMsgg << itemId;
            this->SendMsg(pMsgg);

        }
    }
}
void CGObjPC::HandleItemLockRequest(CMsg* pMsg)
{
    int itemId;
    *pMsg >> itemId;

    int itemSlotId;
    *pMsg >> itemSlotId;

    unsigned short itemTypeId;
    *pMsg >> itemTypeId;

    int LockedItemSlot;
    *pMsg >> LockedItemSlot;


    CGItem* lockedItem = GetInventoryItemSafe(this, LockedItemSlot);
    CGItem* lockScroll = GetInventoryItemSafe(this, itemSlotId);
    if (IsUsableItem(lockedItem))
    {
        INT64 ID64 = lockedItem->ID64;
        ScopedItemLockOperation operation(ID64);
        if (!operation.Acquired())
        {
            SendItemLockFailure(this, 1, 2, static_cast<BYTE>(LockedItemSlot));
            return;
        }
        if (!CSqlCon::IsItemLocked(ID64))
        {
            if (IsUsableItem(lockScroll))
            {
                if (lockScroll->InstanceItem->RefItemID == itemId && lockScroll->InstanceItem->pCRefObjItem->TID.m_type_id_value == 0xCEED)
                {
                    CMsg* newpMsg = this->AllocMsg(0x5030);
                    CMsg* pMsg2 = this->AllocMsg(0x305C);
                    CMsg* pShardMsgFirst = NEWMSG(SEND_SHARD_TO_LOCK_INFO, false);
                    if (newpMsg == NULL || pMsg2 == NULL || pShardMsgFirst == NULL)
                    {
                        CNetHelper::FreeMsg(newpMsg);
                        CNetHelper::FreeMsg(pMsg2);
                        CNetHelper::FreeMsg(pShardMsgFirst);
                        SendItemLockFailure(this, 1, 2, static_cast<BYTE>(LockedItemSlot));
                        return;
                    }

                    const CSqlCon::ItemLockStateResult stateResult =
                        CSqlCon::SetItemLockState(ID64, true);
                    if (stateResult != CSqlCon::ITEM_LOCK_STATE_CHANGED)
                    {
                        CNetHelper::FreeMsg(newpMsg);
                        CNetHelper::FreeMsg(pMsg2);
                        CNetHelper::FreeMsg(pShardMsgFirst);
                        SendItemLockFailure(this, 1,
                            stateResult == CSqlCon::ITEM_LOCK_STATE_FAILED ? 1 : 2,
                            static_cast<BYTE>(LockedItemSlot));
                        return;
                    }

                    lockedItem = GetInventoryItemSafe(this, LockedItemSlot);
                    lockScroll = GetInventoryItemSafe(this, itemSlotId);
                    if (!IsUsableItem(lockedItem) || lockedItem->ID64 != ID64 ||
                        !IsUsableItem(lockScroll) || lockScroll->InstanceItem->RefItemID != itemId)
                    {
                        CSqlCon::SetItemLockState(ID64, false);
                        CNetHelper::FreeMsg(newpMsg);
                        CNetHelper::FreeMsg(pMsg2);
                        CNetHelper::FreeMsg(pShardMsgFirst);
                        SendItemLockFailure(this, 1, 2, static_cast<BYTE>(LockedItemSlot));
                        return;
                    }

                    CSqlCon::AddLockedItem(ID64);
                    this->SetLiveDeleteItem(itemSlotId, 1);

                    *newpMsg << LockedItemSlot << ID64;
                    *pMsg2 << unsigned int(this->GetGameID()); //flag opt lvl
                    *pMsg2 << unsigned int(3769);
                    *pShardMsgFirst << ID64;
                    CNetHelper::SendMsgToSM(pShardMsgFirst);
                    this->SendMsg(newpMsg);
                    this->SendMsg(pMsg2);
                }
            }
        }
        else
        {
            CMsg* pck = this->AllocMsg(0x5015);
            if (pck == NULL)
                return;
            *pck << byte(TYPE_OF_ITEM_LOCKED);
            this->SendMsg(pck);
            return;
        }
    }

}
void CGObjPC::HandleItemUnlockRequest(CMsg* pMsg)
{
    int itemId;
    *pMsg >> itemId;

    int itemSlotId;
    *pMsg >> itemSlotId;

    unsigned short itemTypeId;
    *pMsg >> itemTypeId;

    int LockedItemSlot;
    *pMsg >> LockedItemSlot;


    CGItem* lockedItem = GetInventoryItemSafe(this, LockedItemSlot);
    CGItem* unlockScroll = GetInventoryItemSafe(this, itemSlotId);
    if (IsUsableItem(lockedItem))
    {
        INT64 ID64 = lockedItem->ID64;
        ScopedItemLockOperation operation(ID64);
        if (!operation.Acquired())
        {
            SendItemLockFailure(this, 2, 2, static_cast<BYTE>(LockedItemSlot));
            return;
        }
        if (CSqlCon::IsItemLocked(ID64))
        {
            if (IsUsableItem(unlockScroll))
            {
                if (unlockScroll->InstanceItem->RefItemID == itemId && unlockScroll->InstanceItem->pCRefObjItem->TID.m_type_id_value == 0xD6ED)
                {
                    CMsg* newpMsg = this->AllocMsg(0x5031);
                    CMsg* pMsg2 = this->AllocMsg(0x305C);
                    CMsg* pShardMsgFirst = NEWMSG(SEND_SHARD_TO_UNLOCK_INFO, false);
                    if (newpMsg == NULL || pMsg2 == NULL || pShardMsgFirst == NULL)
                    {
                        CNetHelper::FreeMsg(newpMsg);
                        CNetHelper::FreeMsg(pMsg2);
                        CNetHelper::FreeMsg(pShardMsgFirst);
                        SendItemLockFailure(this, 2, 2, static_cast<BYTE>(LockedItemSlot));
                        return;
                    }

                    const CSqlCon::ItemLockStateResult stateResult =
                        CSqlCon::SetItemLockState(ID64, false);
                    if (stateResult != CSqlCon::ITEM_LOCK_STATE_CHANGED)
                    {
                        CNetHelper::FreeMsg(newpMsg);
                        CNetHelper::FreeMsg(pMsg2);
                        CNetHelper::FreeMsg(pShardMsgFirst);
                        SendItemLockFailure(this, 2,
                            stateResult == CSqlCon::ITEM_LOCK_STATE_FAILED ? 1 : 2,
                            static_cast<BYTE>(LockedItemSlot));
                        return;
                    }

                    lockedItem = GetInventoryItemSafe(this, LockedItemSlot);
                    unlockScroll = GetInventoryItemSafe(this, itemSlotId);
                    if (!IsUsableItem(lockedItem) || lockedItem->ID64 != ID64 ||
                        !IsUsableItem(unlockScroll) || unlockScroll->InstanceItem->RefItemID != itemId)
                    {
                        CSqlCon::SetItemLockState(ID64, true);
                        CNetHelper::FreeMsg(newpMsg);
                        CNetHelper::FreeMsg(pMsg2);
                        CNetHelper::FreeMsg(pShardMsgFirst);
                        SendItemLockFailure(this, 2, 2, static_cast<BYTE>(LockedItemSlot));
                        return;
                    }

                    CSqlCon::RemoveLockedItem(ID64);
                    this->SetLiveDeleteItem(itemSlotId, 1);

                    *newpMsg << LockedItemSlot << ID64;
                    *pMsg2 << unsigned int(this->GetGameID()); //flag opt lvl
                    *pMsg2 << unsigned int(3769);
                    *pShardMsgFirst << ID64;
                    CNetHelper::SendMsgToSM(pShardMsgFirst);
                    this->SendMsg(newpMsg);
                    this->SendMsg(pMsg2);
                }
            }
            return;
        }
        else
        {
            CMsg* pck = this->AllocMsg(0x5015);
            if (pck == NULL)
                return;
            *pck << byte(5);
            this->SendMsg(pck);
            return;
        }
    }
}
void CGObjPC::HandleAlchemyLinkRequest(CMsg* pMsg)
{
    byte ItemSlot;
    *pMsg >> ItemSlot;
    CGItem* pItem = GetInventoryItemSafe(this, ItemSlot);

    byte AdvPlus;
    *pMsg >> AdvPlus;
    if (IsUsableItem(pItem))
    {
        CMsg* pTmpMsg = CNetHelper::AllocMsg(0x0000, false);
        if (pTmpMsg == NULL)
            return;
        CNetHelper::BindStreamBufferWithMsg(pTmpMsg);
        if (pItem->InstanceItem->pCRefObjItem->TID.getTypeID1() == 3) {
            if ((pItem->InstanceItem->pCRefObjItem->TID.getTypeID2() == 2
                 && pItem->InstanceItem->pCRefObjItem->TID.getTypeID3() == 1 &&
                 pItem->InstanceItem->pCRefObjItem->TID.getTypeID4() == 1) ||
                (pItem->InstanceItem->pCRefObjItem->TID.getTypeID1() == 3 &&
                 pItem->InstanceItem->pCRefObjItem->TID.getTypeID2() == 3
                 && pItem->InstanceItem->pCRefObjItem->TID.getTypeID3() == 3 &&
                 pItem->InstanceItem->pCRefObjItem->TID.getTypeID4() == 7)
                || (pItem->InstanceItem->pCRefObjItem->TID.getTypeID1() == 3 &&
                    pItem->InstanceItem->pCRefObjItem->TID.getTypeID2() == 2
                    && pItem->InstanceItem->pCRefObjItem->TID.getTypeID3() == 1 &&
                    pItem->InstanceItem->pCRefObjItem->TID.getTypeID4() == 2)) {
                pItem->WriteItemCosDataMsg((void *) g_CStreamBufferUnk, 1);
            } else {
                pItem->WriteItemDataToMsg((void *) g_CStreamBufferUnk, 1);
            }
        } else {
            if (pItem->InstanceItem->pCRefObjItem->TID.getTypeID1() == 3 &&
                pItem->InstanceItem->pCRefObjItem->TID.getTypeID2() == 3
                && pItem->InstanceItem->pCRefObjItem->TID.getTypeID3() == 3 &&
                pItem->InstanceItem->pCRefObjItem->TID.getTypeID4() == 7
                || (pItem->InstanceItem->pCRefObjItem->TID.getTypeID1() == 3 &&
                    pItem->InstanceItem->pCRefObjItem->TID.getTypeID2() == 2
                    && pItem->InstanceItem->pCRefObjItem->TID.getTypeID3() == 1 &&
                    pItem->InstanceItem->pCRefObjItem->TID.getTypeID4() == 2)) {
                pItem->WriteItemCosDataMsg((void *) g_CStreamBufferUnk, 1);
            } else {
                pItem->WriteItemDataToMsg((void *) g_CStreamBufferUnk, 1);
            }
        }

        CNetHelper::FlushStreamBufferMsg(pTmpMsg);
        const char* itemPayload = NULL;
        int nLen = 0;
        if (!TryGetMessagePayload(pTmpMsg, itemPayload, nLen))
        {
            CNetHelper::FreeMsg(pTmpMsg);
            return;
        }

        CMsg* pSmMsg = this->AllocMsg(0x5034);
        if (pSmMsg == NULL)
        {
            CNetHelper::FreeMsg(pTmpMsg);
            return;
        }

        pSmMsg->Write<int>(nLen);
        if (nLen > 0)
            pSmMsg->Write(itemPayload, static_cast<size_t>(nLen));
        *pSmMsg << pItem->InstanceItem->RefItemID;
        *pSmMsg << pItem->InstanceItem->OptLevel;
        *pSmMsg << AdvPlus;
        this->SendMsg(pSmMsg);
        CNetHelper::FreeMsg(pTmpMsg);

    }

}



void CGObjPC::HandleDisplayCharInfoRequest(CMsg* pMsg)
{
    std::string SenderName;
    pMsg->ReadString(SenderName, 64);

    if (SenderName.empty() || SenderName.length() > 64)
        return;

    for (int i = 0; i < 13; i++)
    {
        if(i != 8)
        {
            CGItem* pItem = GetInventoryItemSafe(this, i);
            if (IsUsableItem(pItem))
            {
                    CMsg* pTmpMsg = CNetHelper::AllocMsg(0x0000, false);
                    if (pTmpMsg == NULL)
                        continue;
                    CNetHelper::BindStreamBufferWithMsg(pTmpMsg);
                    if ((pItem->InstanceItem->pCRefObjItem->TID.getTypeID1() == 3 && pItem->InstanceItem->pCRefObjItem->TID.getTypeID2() == 2
                         && pItem->InstanceItem->pCRefObjItem->TID.getTypeID3() == 1 && pItem->InstanceItem->pCRefObjItem->TID.getTypeID4() == 1) ||
                        (pItem->InstanceItem->pCRefObjItem->TID.getTypeID1() == 3 && pItem->InstanceItem->pCRefObjItem->TID.getTypeID2() == 3
                         && pItem->InstanceItem->pCRefObjItem->TID.getTypeID3() == 3 && pItem->InstanceItem->pCRefObjItem->TID.getTypeID4() == 7)
                        || (pItem->InstanceItem->pCRefObjItem->TID.getTypeID1() == 3 && pItem->InstanceItem->pCRefObjItem->TID.getTypeID2() == 2
                            && pItem->InstanceItem->pCRefObjItem->TID.getTypeID3() == 1 && pItem->InstanceItem->pCRefObjItem->TID.getTypeID4() == 2))
                    {
                        pItem->WriteItemCosDataMsg((void*)g_CStreamBufferUnk, 1);
                    }
                    else
                    {
                        pItem->WriteItemDataToMsg((void*)g_CStreamBufferUnk, 1);
                    }

                    CNetHelper::FlushStreamBufferMsg(pTmpMsg);
                    const char* itemPayload = NULL;
                    int nLen = 0;
                    if (!TryGetMessagePayload(pTmpMsg, itemPayload, nLen))
                    {
                        CNetHelper::FreeMsg(pTmpMsg);
                        continue;
                    }

                    CMsg* pSmMsg = this->AllocMsg(0x5038);
                    if (pSmMsg == NULL)
                    {
                        CNetHelper::FreeMsg(pTmpMsg);
                        continue;
                    }
                    pSmMsg->Write<byte>(i);
                    pSmMsg->Write<int>(nLen);
                    if (nLen > 0)
                        pSmMsg->Write(itemPayload, static_cast<size_t>(nLen));
                    *pSmMsg << pItem->InstanceItem->RefItemID;
                    pSmMsg->WriteString(SenderName);
                    this->SendMsg(pSmMsg);
                    CNetHelper::FreeMsg(pTmpMsg);
            }
        }

    }

}


void CGObjPC::HandleNewAlchemyRequest(CMsg *pMsg) {
#if 0
    // Retired custom New Alchemy implementation. Keep the historical source
    // available for reference, but do not emit its SQL-heavy machine code.
    // ReaderPacket consumes NEW_ALCHEMY_PACKET before this method can run.
    byte FuseType;
    pMsg->Read<BYTE>(FuseType);

    if (FuseType == 0) {
        byte ItemSlot;
        pMsg->Read<BYTE>(ItemSlot);

        byte EnhancerSlot;
        pMsg->Read<BYTE>(EnhancerSlot);

        byte ProofSlot;
        pMsg->Read<BYTE>(ProofSlot);

        if (GetInventoryItemSafe(this, ItemSlot) != NULL)
        {
            CGItem *pTargetItem = GetInventoryItemSafe(this, ItemSlot);
            if (CSqlCon::IsItemLocked(pTargetItem->InstanceItem->ID64))
            {
                CMsg* pck = this->AllocMsg(0x5015);
                *pck << byte(TYPE_OF_ITEM_LOCKED);
                this->SendMsg(pck);
                return;
            }
            if (pTargetItem->InstanceItem->pCRefObjItem->TID.getTypeID1() == 3 && pTargetItem->InstanceItem->pCRefObjItem->TID.getTypeID2() == 1 && pTargetItem->InstanceItem->pCRefObjItem->TID.getTypeID3() != 13)
            {
                if (GetInventoryItemSafe(this, EnhancerSlot) != NULL)
                {
                    if (this->ItemIsWeapon(pTargetItem->InstanceItem->pCRefObjItem->TID))
                    {
                        CGItem *pEnhancerItem = GetInventoryItemSafe(this, EnhancerSlot);
                        int Degree = GetDegreeLevel(pTargetItem->InstanceItem->pCRefObjItem->m_btItemClass);
                        if (this->ItemIsEnhancer(pEnhancerItem->InstanceItem->pCRefObjItem->TID) && pEnhancerItem->InstanceItem->pCRefObjItem->Param3 == 100663296 && pEnhancerItem->InstanceItem->pCRefObjItem->Param1 == Degree)
                        {
                            if (GetInventoryItemSafe(this, ProofSlot) != NULL)
                            {
                                CGItem *pRoofItem = GetInventoryItemSafe(this, ProofSlot);
                                if (this->ItemIsProofStone(pRoofItem->InstanceItem->pCRefObjItem->TID))
                                {
                                    if (pRoofItem->InstanceItem->pCRefObjItem->Param1 == pTargetItem->InstanceItem->pCRefObjItem->m_btItemClass)
                                    {
                                        SetLiveDeleteItem(EnhancerSlot, 1);
                                        /// FUSE VIA PROOF
                                        if (pTargetItem->InstanceItem->OptLevel == 0) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_1) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(eAlchemyType::WITH_PROOF); /// W PROOF
                                                NewMsg->Write<BYTE>(eAlchemyResultType::SUCCESS); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);
                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);

                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(eAlchemyType::WITH_PROOF); /// W PROOF
                                                NewMsg->Write<BYTE>(eAlchemyResultType::FAILED); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64);
                                                NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) 0);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,
                                                                           pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 1) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_2) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(eAlchemyType::WITH_PROOF); /// W PROOF
                                                NewMsg->Write<BYTE>(eAlchemyResultType::SUCCESS); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(eAlchemyType::WITH_PROOF); /// W PROOF
                                                NewMsg->Write<BYTE>(eAlchemyResultType::FAILED); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 2) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_3) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 3) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_4) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 4) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_5) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 5) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_6) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 6) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_7) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 7) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_8) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 8) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_9) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 9) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_10) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 10) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_11) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 11) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_12) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 12) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_13) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 13) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_14) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 14) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_15) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 15) {
                                            CMsg *NewMsg = this->AllocMsg(0x5017);
                                            NewMsg->Write<BYTE>(0); /// W PROOF
                                            NewMsg->Write<BYTE>(2); // FAIL RESULT
                                            this->SendMsg(NewMsg);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (this->ItemIsArmor(pTargetItem->InstanceItem->pCRefObjItem->TID)) {
                        CGItem *pEnhancerItem = GetInventoryItemSafe(this, EnhancerSlot);
                        int Degree = GetDegreeLevel(pTargetItem->InstanceItem->pCRefObjItem->m_btItemClass);
                        if (this->ItemIsEnhancer(pEnhancerItem->InstanceItem->pCRefObjItem->TID) && pEnhancerItem->InstanceItem->pCRefObjItem->Param1 == Degree) {
                            if (GetInventoryItemSafe(this, ProofSlot) != NULL) {
                                CGItem *pRoofItem = GetInventoryItemSafe(this, ProofSlot);
                                if (this->ItemIsProofStone(pRoofItem->InstanceItem->pCRefObjItem->TID)) {
                                    if (pRoofItem->InstanceItem->pCRefObjItem->Param1 ==
                                        pTargetItem->InstanceItem->pCRefObjItem->m_btItemClass) {

                                        SetLiveDeleteItem(EnhancerSlot, 1);
                                        /// FUSE VIA PROOF
                                        if (pTargetItem->InstanceItem->OptLevel == 0) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_1) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(eAlchemyType::WITH_PROOF); /// W PROOF
                                                NewMsg->Write<BYTE>(eAlchemyResultType::SUCCESS); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);
                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(eAlchemyType::WITH_PROOF); /// W PROOF
                                                NewMsg->Write<BYTE>(eAlchemyResultType::FAILED); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) 0);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,
                                                                           pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 1) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_2) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(eAlchemyType::WITH_PROOF); /// W PROOF
                                                NewMsg->Write<BYTE>(eAlchemyResultType::SUCCESS); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(eAlchemyType::WITH_PROOF); /// W PROOF
                                                NewMsg->Write<BYTE>(eAlchemyResultType::FAILED); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 2) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_3) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 3) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_4) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 4) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_5) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 5) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_6) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 6) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_7) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 7) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_8) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 8) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_9) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 9) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_10) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 10) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_11) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 11) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_12) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 12) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_13) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 13) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_14) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 14) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_15) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 15) {
                                            CMsg *NewMsg = this->AllocMsg(0x5017);
                                            NewMsg->Write<BYTE>(0); /// W PROOF
                                            NewMsg->Write<BYTE>(2); // FAIL RESULT
                                            this->SendMsg(NewMsg);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (this->ItemIsAccessory(pTargetItem->InstanceItem->pCRefObjItem->TID)){
                        CGItem *pEnhancerItem = GetInventoryItemSafe(this, EnhancerSlot);
                        int Degree = GetDegreeLevel(pTargetItem->InstanceItem->pCRefObjItem->m_btItemClass);
                        if (pEnhancerItem->InstanceItem->pCRefObjItem->Param3 == 83886080 &&
                            this->ItemIsEnhancer(pEnhancerItem->InstanceItem->pCRefObjItem->TID) && pEnhancerItem->InstanceItem->pCRefObjItem->Param1 == Degree)
                        {
                            if (GetInventoryItemSafe(this, ProofSlot) != NULL) {
                                CGItem *pRoofItem = GetInventoryItemSafe(this, ProofSlot);
                                if (this->ItemIsProofStone(pRoofItem->InstanceItem->pCRefObjItem->TID))
                                {
                                    if (pRoofItem->InstanceItem->pCRefObjItem->Param1 ==
                                        pTargetItem->InstanceItem->pCRefObjItem->m_btItemClass) {

                                        SetLiveDeleteItem(EnhancerSlot, 1);
                                        /// FUSE VIA PROOF
                                        if (pTargetItem->InstanceItem->OptLevel == 0) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_1) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(eAlchemyType::WITH_PROOF); /// W PROOF
                                                NewMsg->Write<BYTE>(eAlchemyResultType::SUCCESS); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);
                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(eAlchemyType::WITH_PROOF); /// W PROOF
                                                NewMsg->Write<BYTE>(eAlchemyResultType::FAILED); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) 0);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,
                                                                           pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 1) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_2) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(eAlchemyType::WITH_PROOF); /// W PROOF
                                                NewMsg->Write<BYTE>(eAlchemyResultType::SUCCESS); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(eAlchemyType::WITH_PROOF); /// W PROOF
                                                NewMsg->Write<BYTE>(eAlchemyResultType::FAILED); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 2) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_3) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 3) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_4) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 4) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_5) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 5) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_6) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 6) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_7) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 7) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_8) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 8) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_9) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 9) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_10) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 10) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_11) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 11) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_12) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 12) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_13) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 13) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_14) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 14) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_15) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 15) {
                                            CMsg *NewMsg = this->AllocMsg(0x5017);
                                            NewMsg->Write<BYTE>(0); /// W PROOF
                                            NewMsg->Write<BYTE>(2); // FAIL RESULT
                                            this->SendMsg(NewMsg);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (this->ItemIsShield(pTargetItem->InstanceItem->pCRefObjItem->TID)) {
                        CGItem *pEnhancerItem = GetInventoryItemSafe(this, EnhancerSlot);
                        int Degree = GetDegreeLevel(pTargetItem->InstanceItem->pCRefObjItem->m_btItemClass);
                        if (pEnhancerItem->InstanceItem->pCRefObjItem->Param3 == 67108864 && this->ItemIsEnhancer(pEnhancerItem->InstanceItem->pCRefObjItem->TID) && pEnhancerItem->InstanceItem->pCRefObjItem->Param1 == Degree) {
                            if (GetInventoryItemSafe(this, ProofSlot) != NULL) {
                                CGItem *pRoofItem = GetInventoryItemSafe(this, ProofSlot);
                                if (this->ItemIsProofStone(pRoofItem->InstanceItem->pCRefObjItem->TID)) {
                                    if (pRoofItem->InstanceItem->pCRefObjItem->Param1 ==
                                        pTargetItem->InstanceItem->pCRefObjItem->m_btItemClass) {
                                        SetLiveDeleteItem(EnhancerSlot, 1);
                                        /// FUSE VIA PROOF
                                        if (pTargetItem->InstanceItem->OptLevel == 0) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_1) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(eAlchemyType::WITH_PROOF); /// W PROOF
                                                NewMsg->Write<BYTE>(eAlchemyResultType::SUCCESS); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);
                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);
                                                this->SendMsg(NewMsg);
                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();
                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(eAlchemyType::WITH_PROOF); /// W PROOF
                                                NewMsg->Write<BYTE>(eAlchemyResultType::FAILED); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) 0);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,
                                                                           pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 1) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_2) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(eAlchemyType::WITH_PROOF); /// W PROOF
                                                NewMsg->Write<BYTE>(eAlchemyResultType::SUCCESS); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(eAlchemyType::WITH_PROOF); /// W PROOF
                                                NewMsg->Write<BYTE>(eAlchemyResultType::FAILED); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 2) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_3) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 3) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_4) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 4) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_5) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 5) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_6) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 6) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_7) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 7) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_8) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 8) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_9) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 9) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_10) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 10) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_11) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 11) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_12) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 12) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_13) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 13) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_14) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 14) {
                                            int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                            if (chance < SuccesRateOptLevel_15) {
                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            } else {

                                                SetLiveDeleteItem(ProofSlot, 1);

                                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                                NewMsg->Write<BYTE>(0); /// W PROOF
                                                NewMsg->Write<BYTE>(1); // FAIL RESULT
                                                NewMsg->Write<BYTE>(ItemSlot);
                                                NewMsg->Write<BYTE>(EnhancerSlot);
                                                NewMsg->Write<BYTE>(ProofSlot);

                                                NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel - 1);
                                                NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                                byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                                this->SendMsg(NewMsg);

                                                pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel-1);
                                                pTargetItem->RefreshItemStats();

                                                byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                            }

                                            this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                        }
                                        else if (pTargetItem->InstanceItem->OptLevel == 15) {
                                            CMsg *NewMsg = this->AllocMsg(0x5017);
                                            NewMsg->Write<BYTE>(0); /// W PROOF
                                            NewMsg->Write<BYTE>(2); // FAIL RESULT
                                            this->SendMsg(NewMsg);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    else if (FuseType == 1) {
        byte ItemSlot;
        pMsg->Read<BYTE>(ItemSlot);

        byte EnhancerSlot;
        pMsg->Read<BYTE>(EnhancerSlot);

        if (GetInventoryItemSafe(this, ItemSlot) != NULL) {
            CGItem *pTargetItem = GetInventoryItemSafe(this, ItemSlot);
            if (CSqlCon::IsItemLocked(pTargetItem->InstanceItem->ID64))
            {
                CMsg* pck = this->AllocMsg(0x5015);
                *pck << byte(TYPE_OF_ITEM_LOCKED);
                this->SendMsg(pck);
                return;
            }
            if (pTargetItem->InstanceItem->pCRefObjItem->TID.getTypeID1() == 3 && pTargetItem->InstanceItem->pCRefObjItem->TID.getTypeID2() == 1 && pTargetItem->InstanceItem->pCRefObjItem->TID.getTypeID3() != 13)
            {
                if (GetInventoryItemSafe(this, EnhancerSlot) != NULL)
                {
                    if (this->ItemIsWeapon(pTargetItem->InstanceItem->pCRefObjItem->TID)) {
                        CGItem *pEnhancerItem = GetInventoryItemSafe(this, EnhancerSlot);
                        int Degree = GetDegreeLevel(pTargetItem->InstanceItem->pCRefObjItem->m_btItemClass);
                        if (pEnhancerItem->InstanceItem->pCRefObjItem->Param3 == 100663296 && this->ItemIsEnhancer(pEnhancerItem->InstanceItem->pCRefObjItem->TID) && pEnhancerItem->InstanceItem->pCRefObjItem->Param1 == Degree)
                        {
                            SetLiveDeleteItem(EnhancerSlot, 1);
                            if (pTargetItem->InstanceItem->OptLevel == 0) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_1) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);

                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 1) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_2) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                }
                                else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 2) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_3) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 3) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_4) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 4) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_5) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);

                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus(
                                            (BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,
                                                                   pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 5) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_6) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);
                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 6) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_7) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 7) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_8) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 8) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_9) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 9) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_10) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 10) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_11) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 11) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_12) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 12) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_13) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);

                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 13) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_14) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 14) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_15) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 15) {
                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                NewMsg->Write<BYTE>(1); /// W PROOF
                                NewMsg->Write<BYTE>(2); // FAIL RESULT
                                this->SendMsg(NewMsg);
                            }
                        }
                    }
                    else if (this->ItemIsArmor(pTargetItem->InstanceItem->pCRefObjItem->TID))
                    {
                        CGItem *pEnhancerItem = GetInventoryItemSafe(this, EnhancerSlot);
                        int Degree = GetDegreeLevel(pTargetItem->InstanceItem->pCRefObjItem->m_btItemClass);
                        if (pEnhancerItem->InstanceItem->pCRefObjItem->Param3 == 16909056 && this->ItemIsEnhancer(pEnhancerItem->InstanceItem->pCRefObjItem->TID) && pEnhancerItem->InstanceItem->pCRefObjItem->Param1 == Degree)
                        {
                            SetLiveDeleteItem(EnhancerSlot, 1);
                            if (pTargetItem->InstanceItem->OptLevel == 0) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_1) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);

                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 1) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_2) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                }
                                else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 2) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_3) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 3) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_4) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 4) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_5) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);

                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus(
                                            (BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,
                                                                   pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 5) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_6) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);
                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 6) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_7) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 7) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_8) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 8) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_9) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 9) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_10) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 10) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_11) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 11) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_12) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 12) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_13) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);

                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 13) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_14) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 14) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_15) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 15) {
                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                NewMsg->Write<BYTE>(1); /// W PROOF
                                NewMsg->Write<BYTE>(2); // FAIL RESULT
                                this->SendMsg(NewMsg);
                            }
                        }
                    }
                    else if (this->ItemIsAccessory(pTargetItem->InstanceItem->pCRefObjItem->TID)) {
                        CGItem *pEnhancerItem = GetInventoryItemSafe(this, EnhancerSlot);
                        int Degree = GetDegreeLevel(pTargetItem->InstanceItem->pCRefObjItem->m_btItemClass);
                        if (pEnhancerItem->InstanceItem->pCRefObjItem->Param3 == 83886080 && this->ItemIsEnhancer(pEnhancerItem->InstanceItem->pCRefObjItem->TID) && pEnhancerItem->InstanceItem->pCRefObjItem->Param1 == Degree)
                        {
                            SetLiveDeleteItem(EnhancerSlot, 1);
                            if (pTargetItem->InstanceItem->OptLevel == 0) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_1) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);

                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 1) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_2) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                }
                                else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 2) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_3) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 3) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_4) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 4) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_5) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);

                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus(
                                            (BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,
                                                                   pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 5) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_6) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);
                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 6) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_7) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 7) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_8) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 8) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_9) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 9) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_10) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 10) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_11) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 11) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_12) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 12) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_13) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);

                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 13) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_14) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 14) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_15) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 15) {
                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                NewMsg->Write<BYTE>(1); /// W PROOF
                                NewMsg->Write<BYTE>(2); // FAIL RESULT
                                this->SendMsg(NewMsg);
                            }
                        }
                    }
                    else if (this->ItemIsShield(pTargetItem->InstanceItem->pCRefObjItem->TID)) {
                        CGItem *pEnhancerItem = GetInventoryItemSafe(this, EnhancerSlot);
                        int Degree = GetDegreeLevel(pTargetItem->InstanceItem->pCRefObjItem->m_btItemClass);
                        if (pEnhancerItem->InstanceItem->pCRefObjItem->Param3 == 67108864 && this->ItemIsEnhancer(pEnhancerItem->InstanceItem->pCRefObjItem->TID) && pEnhancerItem->InstanceItem->pCRefObjItem->Param1 == Degree)
                        {
                            SetLiveDeleteItem(EnhancerSlot, 1);
                            if (pTargetItem->InstanceItem->OptLevel == 0) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_1) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);

                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 1) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_2) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                }
                                else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 2) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_3) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 3) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_4) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 4) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_5) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);

                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus(
                                            (BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,
                                                                   pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 5) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_6) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);
                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 6) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_7) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 7) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_8) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 8) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_9) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 9) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_10) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 10) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_11) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 11) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_12) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 12) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_13) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);

                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 13) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_14) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 14) {
                                int chance = rand() % 100; // 0 - 99 arasında rastgele bir sayı
                                if (chance < SuccesRateOptLevel_15) {
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W PROOF
                                    NewMsg->Write<BYTE>(0); // SUCCESS RESULT
                                    NewMsg->Write<BYTE>(ItemSlot);
                                    NewMsg->Write<BYTE>(EnhancerSlot);


                                    NewMsg->Write<BYTE>(pTargetItem->InstanceItem->OptLevel + 1);
                                    NewMsg->Write<int>(pTargetItem->InstanceItem->RefItemID);
                                    byte AdvOptLevel = CSqlCon::GetItemBindingOpt(pTargetItem->ID64); NewMsg->Write<BYTE>(AdvOptLevel);

                                    this->SendMsg(NewMsg);

                                    pTargetItem->InstanceItem->SetPlus((BYTE) pTargetItem->InstanceItem->OptLevel + 1);
                                    pTargetItem->RefreshItemStats();

                                    byte NewPlus = pTargetItem->InstanceItem->OptLevel+AdvOptLevel; this->Send3040(ItemSlot, NewPlus);
                                    this->UpdateItemPlusInDatabase(pTargetItem->ID64,pTargetItem->InstanceItem->OptLevel);
                                } else {
                                    SetLiveDeleteItem(ItemSlot, 1);
                                    CMsg *NewMsg = this->AllocMsg(0x5017);
                                    NewMsg->Write<BYTE>(1); /// W/O PROOF
                                    NewMsg->Write<BYTE>(1); // FAIL RESULT
                                    this->SendMsg(NewMsg);
                                }
                            }
                            else if (pTargetItem->InstanceItem->OptLevel == 15) {
                                CMsg *NewMsg = this->AllocMsg(0x5017);
                                NewMsg->Write<BYTE>(1); /// W PROOF
                                NewMsg->Write<BYTE>(2); // FAIL RESULT
                                this->SendMsg(NewMsg);
                            }
                        }
                    }
                }
            }
        }
    }
    #endif
}


bool CGObjPC::ItemIsWeapon(TypeId TID)
{
    if(TID.getTypeID1() == 3 && TID.getTypeID2() == 1 && TID.getTypeID3() == 6)
    {
        return true;
    }
    return false;
}
bool CGObjPC::ItemIsArmor(TypeId TID)
{
    if (TID.getTypeID1() == 3 && TID.getTypeID2() == 1
        && (TID.getTypeID3() == 1 || TID.getTypeID3() == 2 || TID.getTypeID3() == 3 || TID.getTypeID3() == 9 || TID.getTypeID3() == 10  || TID.getTypeID3() == 11)) /// ARMOR
    {
        return true;
    }
    return false;
}
bool CGObjPC::ItemIsAccessory(TypeId TID)
{
    if(TID.getTypeID1() == 3 && TID.getTypeID2() == 1 && (TID.getTypeID3() == 5 || TID.getTypeID3() == 12))
    {
        return true;
    }
    return false;
}
bool CGObjPC::ItemIsShield(TypeId TID)
{
    if(TID.getTypeID1() == 3 && TID.getTypeID2() == 1 && TID.getTypeID3() == 4)
    {
        return true;
    }
    return false;
}
bool CGObjPC::ItemIsEnhancer(TypeId TID)
{
    if(TID.getTypeID1() == 3 && TID.getTypeID2() == 3 && TID.getTypeID3() == 10 && TID.getTypeID4() == 6)
    {
        return true;
    }
    return false;
}
void CGObjPC::Send3040(byte ItemSlot, byte NewOptLevel)
{
    CMsg *pMsg = this->AllocMsgForPeer(0x3040);
    if (pMsg == NULL)
        return;
    *pMsg << ItemSlot;
    *pMsg << BYTE(2); //flag opt lvl
    *pMsg << NewOptLevel;
    this->SendMsg(pMsg);
}
bool CGObjPC::ItemIsProofStone(TypeId TID)
{
    if(TID.getTypeID1() == 3 && TID.getTypeID2() == 3 && TID.getTypeID3() == 10 && TID.getTypeID4() == 8)
    {
        return true;
    }
    return false;
}

void CGObjPC::HandleSelfTeleportRequest()
{
    SPosInfo currentPosition;
    SWorldID currentWorld;
    this->GetPosInfo(currentPosition);
    this->GetWorldID(currentWorld);

    if (currentPosition.wRegionID == 0 || currentWorld.dwWorldID == 0)
        return;

    if (!this->MoveTo(
            currentWorld.dwWorldID,
            currentPosition.wRegionID,
            currentPosition.fltX,
            currentPosition.fltY,
            currentPosition.fltZ,
            2))
    {
        this->MoveTo(
            currentWorld.dwWorldID,
            currentPosition.wRegionID,
            currentPosition.fltX,
            currentPosition.fltY,
            currentPosition.fltZ,
            1);
    }
}

void CGObjPC::HandleFilterTeleportRequest(CMsg* pMsg)
{
    int gameWorldId = 0;
    int regionId = 0;
    int posX = 0;
    int posY = 0;
    int posZ = 0;

    *pMsg >> gameWorldId >> regionId >> posX >> posY >> posZ;

    const int maxCoordinate = 1000000;
    if (gameWorldId <= 0 || gameWorldId > 65535 ||
        regionId <= 0 || regionId > 65535 ||
        posX < -maxCoordinate || posX > maxCoordinate ||
        posY < -maxCoordinate || posY > maxCoordinate ||
        posZ < -maxCoordinate || posZ > maxCoordinate)
        return;

    const uint32_t runtimeWorldId = static_cast<uint32_t>(gameWorldId) + 0x10000u;

    // Keep the same transition contract used by the native administrative
    // movement paths. Mode 2 performs the complete world-transfer handshake;
    // mode 1 is only the established fallback when that transition is refused.
    // Forcing mode 1 here can leave a clientless session without the reset/data
    // cycle required to remain attached to its character.
    if (!this->MoveTo(
        runtimeWorldId,
        static_cast<unsigned short>(regionId),
        static_cast<float>(posX),
        static_cast<float>(posY),
        static_cast<float>(posZ),
        2))
    {
        this->MoveTo(
            runtimeWorldId,
            static_cast<unsigned short>(regionId),
            static_cast<float>(posX),
            static_cast<float>(posY),
            static_cast<float>(posZ),
            1);
    }
}
