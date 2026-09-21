//
// Created by kyuubi09 on 3/30/2023.
//

#include "Game.h"

#include "Helpers.h"

#include "ServerFramework/ServerFramework.h"
#include "BSObj/BSObj.h"

#include "World/GameWorldMgr.h"
#include "ReferenceData/ReferenceDataMgr.h"
#include <Objects/GObjMob.h>
#include <SqlConnection/sqlCon.h>
#include <NetHelper.h>
#include <KMTGuardCustom/GameServerTelemetry.h>
#include <GameServerCommandContract.h>
#include <algorithm>
#include <limits>
#include <vector>

namespace
{
    bool IsValidDestination(int worldId, int regionId, int x, int y, int z)
    {
        return KmtGameServerCommand::IsValidDestination(worldId, regionId, x, y, z);
    }

    bool TryBuildRuntimeWorldId(int baseWorldId, uint32_t& runtimeWorldId)
    {
        if (!KmtGameServerCommand::IsValidWorldId(baseWorldId))
            return false;
        runtimeWorldId = static_cast<uint32_t>(baseWorldId) + 0x10000u;
        return true;
    }

    CGItem* GetInventoryItemSafe(CGObjPC* player, int slot)
    {
        if (player == NULL || slot < 0 || slot >= player->m_PCInventory.m_nSlotsCount)
            return NULL;
        return player->GetItemChar(slot);
    }

    bool IsUsableItem(CGItem* item)
    {
        return item != NULL && item->InstanceItem != NULL &&
               item->InstanceItem->pCRefObjItem != NULL;
    }

    bool ItemHasCode(CGItem* item, const std::string& code)
    {
        return IsUsableItem(item) &&
               item->InstanceItem->pCRefObjItem->m_strObjectCode == code;
    }

    bool WasItemAmountConsumed(CGObjPC* player, int slot, int beforeAmount, int consumeAmount)
    {
        CGItem* current = GetInventoryItemSafe(player, slot);
        if (beforeAmount == consumeAmount)
            return current == NULL || current->InstanceItem == NULL || current->InstanceItem->Data == 0;
        return IsUsableItem(current) && current->InstanceItem->Data == beforeAmount - consumeAmount;
    }

    void RessurectFixed(CGObjPC* player, int health, int mana)
    {
        if (player == NULL) return;
        const int maxHealth = player->GetMaxHealth();
        const int maxMana = player->GetMaxMana();
        const float healthRatio = maxHealth > 0
            ? static_cast<float>(health) / maxHealth : 0.0f;
        const float manaRatio = maxMana > 0
            ? static_cast<float>(mana) / maxMana : 0.0f;
        const float healthMultiplier = healthRatio < 1.0f ? healthRatio : 1.0f;
        const float manaMultiplier = manaRatio < 1.0f ? manaRatio : 1.0f;
        player->Ressurect(healthMultiplier, manaMultiplier);
    }
}

CGObjPC *CGame::GetCharObjSTAT(const char *pName) {
    if (g_pCGame == NULL)
        return NULL;

    return g_pCGame->GetCharObjByName(pName);
}

CGObjPC *CGame::GetCharObjSTAT(DWORD dwCharId) {
    if (g_pCGame == NULL)
        return NULL;

    return g_pCGame->GetCharObjById(dwCharId);
}

CGObjPC *CGame::GetCharObjJID_STAT(DWORD dwAccountJID) {
    if (g_pCGame == NULL)
        return NULL;

    return g_pCGame->GetCharObjByJID(dwAccountJID);
}

CGObj *CGame::GetGameObjSTAT(DWORD dwGameId) {
    if (g_pCGame == NULL)
        return NULL;

    return g_pCGame->GetGameObjById(dwGameId);
}

CGObjPC *CGame::GetCharObjByName(const char *pName) {
    std::list<std::pair<std::string, CGObjPC *>>::const_iterator it = m_listPcCharName.begin();
    for (; it != m_listPcCharName.end(); it++) {
        if ((*it).first == pName) // Doğru karşılaştırma
            return ((*it).second);
    }

    return NULL;
}
const DWORD FN_DESPAWN_OBJ_FN_OFFSET = 0x00485E50;

void CGame::RemoveGameObjByObjId(DWORD ObjID) {
    std::vector<DWORD> gameIds;
    std::list<std::pair<DWORD, CGObj *>>::const_iterator it = m_listObjGameId.begin();
    for (; it != m_listObjGameId.end(); it++) {
        if ((*it).second != NULL && (*it).second->GetRefObjID() == ObjID)
            gameIds.push_back((*it).first);
    }
    for (size_t index = 0; index < gameIds.size(); ++index) {
        CGObj* object = GetGameObjById(gameIds[index]);
        if (object != NULL) DespawnGObj(object);
    }
}
void CGame::RemoveGameObjByWorldId(DWORD ObjID, DWORD dwWorldID)
{
    std::vector<DWORD> gameIds;
    std::list<std::pair<DWORD, CGObj *>>::const_iterator it = m_listObjGameId.begin();
    for (; it != m_listObjGameId.end(); it++) {
        if ((*it).second != NULL && (*it).second->GetRefObjID() == ObjID)
        {
            SWorldID mineWorldID;
            (*it).second->GetWorldID(mineWorldID);
            if (mineWorldID.wLayerID == dwWorldID)
                gameIds.push_back((*it).first);
        }
    }
    for (size_t index = 0; index < gameIds.size(); ++index) {
        CGObj* object = GetGameObjById(gameIds[index]);
        if (object != NULL) DespawnGObj(object);
    }
}
void CGame::DespawnGObj(IGObj* pObj)
{
    __asm pushad;
    __asm pushfd;
    __asm mov esi, pObj;
    __asm call FN_DESPAWN_OBJ_FN_OFFSET;
    __asm popfd;
    __asm popad;
}
CGObjPC *CGame::GetCharObjById(DWORD dwCharId) {
    std::map<DWORD, CGObjPC *>::const_iterator it = m_mapPcCharId.find(dwCharId);
    // If we got it
    if (it != m_mapPcCharId.end())
        return ((*it).second);

    return NULL;
}

void CGame::ToTownObjByWorldId(DWORD dwWorldID) {
   
    for (std::map<DWORD, CGObjPC*>::const_iterator it = m_mapPcCharId.begin(); it != m_mapPcCharId.end(); it++)
    {
        SWorldID mineWorldID;
        (*it).second->GetWorldID(mineWorldID);

        if (mineWorldID.wLayerID == dwWorldID)
        {
            CGObjPC* pC = (*it).second;
            if (pC != NULL)
            {
                pC->TeleportToTown();

            }
         
        }

    }
}
CGObjPC* CGame::GetCharObjByIdNew(DWORD CharId)
{
    // 0x00400000 (gs base addr) + 0x85E20 (offset);
    return reinterpret_cast<CGObjPC * (__cdecl*)(DWORD)>(0x00485df0)(CharId);
}
void CGame::ToTownbyWorldAndLayerID(DWORD dwWorldID, WORD LayerID) {

    if (!KmtGameServerCommand::IsValidWorldId(static_cast<int>(dwWorldID)))
        return;

    for (std::map<DWORD, CGObjPC*>::const_iterator it = m_mapPcCharId.begin(); it != m_mapPcCharId.end(); it++)
    {
        CGObjPC* pC = (*it).second;
        if (pC == NULL)
            continue;
        SWorldID mineWorldID;
        pC->GetWorldID(mineWorldID);

        if (mineWorldID.wWorldID == dwWorldID && mineWorldID.wLayerID == LayerID)
        {
            pC->TeleportToTown();

        }

    }
}

CGObjPC *CGame::GetCharObjByJID(DWORD dwAccountJID) {
    std::map<DWORD, CGObjPC *>::const_iterator it = m_mapPcJID.find(dwAccountJID);
    // If we got it
    if (it != m_mapPcJID.end())
        return ((*it).second);

    return NULL;
}
IGObj* CGame::GetObjByGameID(DWORD dwObjID)
{
    return reinterpret_cast<IGObj * (__cdecl*)(DWORD)>(0x00485D90)(dwObjID);
}
CGObj *CGame::GetGameObjById(DWORD dwGameId) {
    std::list<std::pair<DWORD, CGObj *>>::const_iterator it = m_listObjGameId.begin();
    for (; it != m_listObjGameId.end(); it++) {
        if ((*it).first == dwGameId)
            return ((*it).second);
    }

    return NULL;
}
#define ADHOC_MAX_SLOTS (13 + 96)
#define ADHOC_MAX_PLUS 250

std::map<int, int> SpawnedMobList;

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
#define READ_LOCK_INFO_FROM_SHARD 0x5061
#define READ_UNLOCK_INFO_FROM_SHARD 0x5063

enum CommandType : byte
{
    CHANGE_GRANT_NAME = KmtGameServerCommand::ActionGrantName,
    SPAWN_NPC = KmtGameServerCommand::ActionSpawnAtPosition,
    SPAWN_NPC_NEAR_CHAR = KmtGameServerCommand::ActionSpawnNearPlayer,
    REMOVE_MONSTER_BY_ID = KmtGameServerCommand::ActionRemoveMonster,
    REMOVE_NPC_BY_WORLD_ID = KmtGameServerCommand::ActionRemoveMonsterByWorld,
    TELEPORT_TO_POSITION_SAFE_ZONE_NO_JOB = KmtGameServerCommand::ActionMovePlayerAtPosition,
    GETUP_AT_POS = KmtGameServerCommand::ActionGetUpAtPosition,
    TELEPORT_TO_TOWN = KmtGameServerCommand::ActionTownPlayer,
    CHANGE_PVP_CAPE_TYPE = KmtGameServerCommand::ActionCape,
    TO_TOWN_ALL_BY_WORLDID = KmtGameServerCommand::ActionTownWorld,
    CONSUME_ITEM = KmtGameServerCommand::ActionConsumeItem,
    SPAWN_MONSTER_NEARBY_CHAR = KmtGameServerCommand::ActionSpawnNearPlayer,
    SPAWN_MONSTER_AT_POS = KmtGameServerCommand::ActionSpawnAtPosition,
    ADD_SKILL_POINT = KmtGameServerCommand::ActionExperienceRate,
    TELEPORT_TO_POSITION = KmtGameServerCommand::ActionMovePlayer,
    ADD_GOLD = KmtGameServerCommand::ActionGold,
    MUTATE_ITEM = KmtGameServerCommand::ActionChangeItem,
    LIVE_SKILL = KmtGameServerCommand::ActionAddSkill,
    TELEPORT_TO_OWN_POS = KmtGameServerCommand::ActionGetUpAtPosition,
    ADD_BLUE_TO_ITEM = 25,
    CONFIGURE_TOWER_DEFENSE = KmtGameServerCommand::ActionTowerCombat,
    CONFIGURE_FREE_FOR_ALL = KmtGameServerCommand::ActionFreeForAllCombat,
    SPAWN_NPC_IN_PLAYER_WORLD = KmtGameServerCommand::ActionSpawnAtPositionInPlayerWorld,

};
void CGame::ProcessMessage(CMsg *pMsg) {
    if (pMsg == NULL ||
        pMsg->m_wpMsgId == NULL ||
        pMsg->m_pMsgBuffer == NULL ||
        pMsg->m_wReadDataArrayPos > pMsg->m_wWriteDataArrayPos ||
        pMsg->m_wWriteDataArrayPos > pMsg->m_dwArrayDataSize)
    {
        GameServerTelemetry::RecordMalformedPacket();
        return;
    }

    GameServerTelemetry::ScopedPacketTimer telemetryTimer(false, pMsg->GetMsgId());
    try {
        // Handle the server packets here
        switch (pMsg->GetMsgId()) {
        case 0x8888:
        {
            byte Type;
            *pMsg >> Type;
            if (Type == CHANGE_GRANT_NAME) {
                int CharID;
                std::string GrantName;
                *pMsg >> CharID >> GrantName;
                CGObjPC * TargetChar = g_pCGame->GetCharObjById(CharID);
                if (TargetChar != NULL && TargetChar != 0)
                {
                    TargetChar->SetGrantName(&GrantName);
                }
            }
            else if (Type == SPAWN_NPC) {
                int RefObjId;
                int GameWorldId;
                int RegionId;
                int PosX, PosY, PosZ, GenerateRadius;
                *pMsg >> RefObjId >> GameWorldId >> RegionId >> PosX >> PosY >> PosZ >> GenerateRadius;
                uint32_t runtimeWorldId = 0;
                if (RefObjId > 0 && IsValidDestination(GameWorldId, RegionId, PosX, PosY, PosZ) &&
                    KmtGameServerCommand::IsValidSpawnRadius(GenerateRadius) &&
                    TryBuildRuntimeWorldId(GameWorldId, runtimeWorldId))
                {
                    CGObjMob::CreateMob(RefObjId, runtimeWorldId, KmtGameServerCommand::ToWireRegionId(RegionId),
                                      static_cast<float>(PosX), static_cast<float>(PosY),
                                      static_cast<float>(PosZ), static_cast<float>(GenerateRadius));
                }
            }
            else if (Type == SPAWN_NPC_NEAR_CHAR) {
                int CharID2;
                int RefObjId2;
               
                *pMsg >> CharID2 >> RefObjId2;
                CGObjPC * TargetPC = g_pCGame->GetCharObjById(CharID2);
                if (TargetPC != NULL)
                {
                    SPosInfo minePos;
                    TargetPC->GetPosInfo(minePos);
                    SWorldID mineWorldID;
                    TargetPC->GetWorldID(mineWorldID);

                    CGObjMob::CreateMob(RefObjId2, mineWorldID.wWorldID + 0x10000, (uint16_t)minePos.wRegionID, (float)minePos.fltX, (float)minePos.fltY, (float)minePos.fltZ, (float)5);
                }
            }
            else if (Type == SPAWN_NPC_IN_PLAYER_WORLD) {
                int AnchorCharID;
                int RefObjId;
                int RegionId;
                int PosX, PosY, PosZ, GenerateRadius;
                *pMsg >> AnchorCharID >> RefObjId >> RegionId >> PosX >> PosY >> PosZ >> GenerateRadius;

                CGObjPC* anchor = g_pCGame->GetCharObjById(AnchorCharID);
                if (anchor != NULL && RefObjId > 0 &&
                    KmtGameServerCommand::IsValidRegionId(RegionId) &&
                    KmtGameServerCommand::IsValidCoordinate(PosX) &&
                    KmtGameServerCommand::IsValidCoordinate(PosY) &&
                    KmtGameServerCommand::IsValidCoordinate(PosZ) &&
                    KmtGameServerCommand::IsValidSpawnRadius(GenerateRadius))
                {
                    SPosInfo anchorPos;
                    anchor->GetPosInfo(anchorPos);
                    if (static_cast<int>(anchorPos.wRegionID) == KmtGameServerCommand::NormalizeRegionIdForCompare(RegionId))
                    {
                        SWorldID anchorWorldID;
                        anchor->GetWorldID(anchorWorldID);
                        CGObjMob* createdMob = CGObjMob::CreateMob(
                            RefObjId, anchorWorldID.dwWorldID, KmtGameServerCommand::ToWireRegionId(RegionId),
                            static_cast<float>(PosX), static_cast<float>(PosY),
                            static_cast<float>(PosZ), static_cast<float>(GenerateRadius));
                        if (createdMob == NULL)
                        {
                            ServerFramework::ReportLog(
                                LOG_TYPE_NOTIFY,
                                " DTT tower spawn failed. RefObjID=%d WorldID=%u RegionID=%d Position=%d,%d,%d Radius=%d",
                                RefObjId, anchorWorldID.dwWorldID, RegionId, PosX, PosY, PosZ, GenerateRadius);
                        }
                    }
                }
            }
            else if (Type == REMOVE_MONSTER_BY_ID) {
                int RefObjID;
                *pMsg >> RefObjID;
                RemoveGameObjByObjId(RefObjID);
            }
            else if (Type == REMOVE_NPC_BY_WORLD_ID) {
                int WorldID, MobID;
                *pMsg >> WorldID >> MobID;
                RemoveGameObjByWorldId(MobID, WorldID);
            }
            else if (Type == CONFIGURE_TOWER_DEFENSE) {
                int Enabled, WorldID, RegionID;
                int Team1MobID, Team1Cape, Team2MobID, Team2Cape;
                *pMsg >> Enabled >> WorldID >> RegionID
                      >> Team1MobID >> Team1Cape >> Team2MobID >> Team2Cape;
                const bool valid = Enabled == 1 && KmtGameServerCommand::IsValidWorldId(WorldID) &&
                                   KmtGameServerCommand::IsValidRegionId(RegionID) &&
                                   Team1MobID > 0 && Team2MobID > 0 &&
                                   Team1Cape >= 0 && Team1Cape <= 5 &&
                                   Team2Cape >= 0 && Team2Cape <= 5;
                if (Enabled == 0 || valid)
                    CRegionAttackRestrictionsMgr::ConfigureTowerDefense(
                        valid, WorldID, RegionID, Team1MobID, Team1Cape, Team2MobID, Team2Cape);
            }
            else if (Type == CONFIGURE_FREE_FOR_ALL) {
                int Enabled, WorldID, RegionID;
                *pMsg >> Enabled >> WorldID >> RegionID;
                const bool valid = Enabled == 1 && KmtGameServerCommand::IsValidWorldId(WorldID) &&
                                   KmtGameServerCommand::IsValidRegionId(RegionID);
                if (Enabled == 0 || valid)
                    CRegionAttackRestrictionsMgr::ConfigureFreeForAll(valid, WorldID, RegionID);
            }
            else if (Type == KmtGameServerCommand::ActionAddSkill) {
                int CharID;
                int SkillID;
                *pMsg >> CharID >> SkillID;
                CGObjPC* TargetChar = g_pCGame->GetCharObjById(CharID);
                if (TargetChar != NULL && TargetChar != 0)
                {
                    TargetChar->LiveSkill(SkillID); // limitless
                }
            }
            else if (Type == KmtGameServerCommand::ActionRemoveSkill) {
                int CharID;
                int SkillID;
                *pMsg >> CharID >> SkillID;
                CGObjPC* TargetChar = g_pCGame->GetCharObjById(CharID);
                if (TargetChar != NULL && TargetChar != 0)
                {
                    TargetChar->CancelBuff(SkillID); // limitless
                }
            }
            else if (Type == KmtGameServerCommand::ActionAddSkillByCode) {
                int CharID;
                std::string SkillCodeName;
                *pMsg >> CharID;
                pMsg->ReadString(SkillCodeName, 127);
                CGObjPC* TargetChar = g_pCGame->GetCharObjById(CharID);
                if (TargetChar != NULL && !SkillCodeName.empty())
                {
                    TargetChar->EngageBuffSkill(TargetChar, SkillCodeName.c_str()); // limitless
                }
            }
            else if (Type == KmtGameServerCommand::ActionTownPlayer) {
                int CharID;
                *pMsg >> CharID;
                CGObjPC* TargetChar = g_pCGame->GetCharObjById(CharID);
                if (TargetChar != NULL && TargetChar != 0)
                {
                    TargetChar->TeleportToTown();
                }
            }
            else if (Type == KmtGameServerCommand::ActionTownWorld) {
                int WorldID;
                *pMsg >> WorldID;
                if (KmtGameServerCommand::IsValidWorldId(WorldID))
                    ToTownObjByWorldId(WorldID);
            }
            else if (Type == KmtGameServerCommand::ActionPetSkill)
            {
                unsigned int PetUQID;
                int PetSkillID;
                *pMsg >> PetUQID >> PetSkillID;
                
                IGObj* petObject = GetObjByGameID(PetUQID);
                IGObj* ownerObject = petObject != NULL && petObject->IsCOS()
                    ? petObject->GetOwner() : NULL;
                CGObjPC* owner = ownerObject != NULL && ownerObject->IsPC()
                    ? reinterpret_cast<CGObjPC*>(ownerObject) : NULL;
                if (owner != NULL &&
                    owner->GoldPetPtr == reinterpret_cast<CGObjCOS_GoldPet*>(petObject))
                {
                    CGObjCOS_GoldPet* pc = reinterpret_cast<CGObjCOS_GoldPet*>(petObject);
                    pc->LiveSkill2(PetSkillID);
                }
            }
            else if (Type == KmtGameServerCommand::ActionCape) {
                int CharID;
                int CapeID;
                *pMsg >> CharID >> CapeID;
                CGObjPC* pPC = g_pCGame->GetCharObjById(CharID);
                if (pPC != NULL)
                {
                    pPC->UpdatePVPCapeType(CapeID);
                }
                
            }
            else if (Type == KmtGameServerCommand::ActionMovePlayer) {
                int CharID = 0;
                int GameWorldId = 0;
                int RegionId = 0;
                int PosX = 0;
                int PosY = 0;
                int PosZ = 0;
                *pMsg >> CharID >> GameWorldId >> RegionId >> PosX >> PosY >> PosZ;
                CGObjPC* pPC = g_pCGame->GetCharObjById(CharID);
                if (pPC != NULL)
                {
                    uint32_t runtimeWorldId = 0;
                    if (IsValidDestination(GameWorldId, RegionId, PosX, PosY, PosZ) &&
                        TryBuildRuntimeWorldId(GameWorldId, runtimeWorldId) &&
                        !pPC->MoveTo(runtimeWorldId, KmtGameServerCommand::ToWireRegionId(RegionId), PosX, PosY, PosZ, 2)) {
                        pPC->MoveTo(runtimeWorldId, KmtGameServerCommand::ToWireRegionId(RegionId), PosX, PosY, PosZ, 1);
                    }
                }
            }
            else if (Type == KmtGameServerCommand::ActionGetUp) {
                int CharID = 0;
                *pMsg >> CharID;
                CGObjPC* pPC = g_pCGame->GetCharObjById(CharID);
                if (pPC != NULL)
                {
                    RessurectFixed(pPC, 3000, 3000);

                }
            
            }
            else if (Type == KmtGameServerCommand::ActionExperienceRate)
            {
                int CharIDs;
                unsigned int Exp;
                *pMsg >> CharIDs >> Exp;
                CGObjPC* pPC = g_pCGame->GetCharObjById(CharIDs);
                if (pPC != NULL)
                {
                    pPC->FuncJobExpTest(Exp);
                }
            }
            else if (Type == KmtGameServerCommand::ActionChangeItem)
            {
                int CharID;
                int Slot;
                std::string ItemCode;
                *pMsg >> CharID;
                *pMsg >> Slot;
                *pMsg >> ItemCode;

                const char* p = ItemCode.c_str();
                CGObjPC * pPC = g_pCGame->GetCharObjById(CharID);
                if (pPC != NULL) {
                    if (!ItemCode.empty() && ItemCode.size() <= 128 && IsUsableItem(GetInventoryItemSafe(pPC, Slot))) {
                        pPC->SetLiveItem(Slot, p);
                        if (!ItemHasCode(GetInventoryItemSafe(pPC, Slot), ItemCode))
                            BS_ERROR("Live item mutation verification failed (char=%d slot=%d)", CharID, Slot);
                    }
                }
            }
            else if (Type == KmtGameServerCommand::ActionConsumeItem)
            {
                int CharID;
                int Slot;
                int Amount;
                *pMsg >> CharID;
                *pMsg >> Slot;
                *pMsg >> Amount;
                CGObjPC * pPC = g_pCGame->GetCharObjById(CharID);
                if (pPC != NULL)
                {
                    CGItem* item = GetInventoryItemSafe(pPC, Slot);
                    if(Amount > 0 && IsUsableItem(item) && item->InstanceItem->Data >= Amount)
                    {
                        const int beforeAmount = item->InstanceItem->Data;
                        const int deleteResult = pPC->SetLiveDeleteItem(Slot, Amount);
                        if (!WasItemAmountConsumed(pPC, Slot, beforeAmount, Amount))
                            BS_ERROR("Live item consumption verification failed (char=%d slot=%d amount=%d result=%d)", CharID, Slot, Amount, deleteResult);
                    }

                }
            }
            else if (Type == KmtGameServerCommand::ActionConsumeAndChangeItem)
            {
                int CharID;
                int MutateSlot;
                std::string ItemCode;
                int ConsumeSlot;
                int ConsumeAmount;
                *pMsg >> CharID;

                *pMsg >> MutateSlot;
                *pMsg >> ItemCode;
                *pMsg >> ConsumeSlot;
                *pMsg >> ConsumeAmount;
                const char* p = ItemCode.c_str();
                CGObjPC * pPC = g_pCGame->GetCharObjById(CharID);
                if (pPC != NULL)
                {
                    CGItem* consumeItem = GetInventoryItemSafe(pPC, ConsumeSlot);
                    CGItem* mutateItem = GetInventoryItemSafe(pPC, MutateSlot);
                    if(MutateSlot != ConsumeSlot && ConsumeAmount > 0 &&
                       !ItemCode.empty() && ItemCode.size() <= 128 &&
                       IsUsableItem(consumeItem) && IsUsableItem(mutateItem) &&
                       consumeItem->InstanceItem->Data >= ConsumeAmount)
                    {
                        const int beforeAmount = consumeItem->InstanceItem->Data;
                        const std::string originalCode = mutateItem->InstanceItem->pCRefObjItem->m_strObjectCode;
                        pPC->SetLiveItem(MutateSlot, p);
                        if (!ItemHasCode(GetInventoryItemSafe(pPC, MutateSlot), ItemCode))
                        {
                            BS_ERROR("Live consume-and-mutate change failed (char=%d slot=%d)", CharID, MutateSlot);
                        }
                        else
                        {
                            const int deleteResult = pPC->SetLiveDeleteItem(ConsumeSlot, ConsumeAmount);
                            if (!WasItemAmountConsumed(pPC, ConsumeSlot, beforeAmount, ConsumeAmount))
                            {
                                pPC->SetLiveItem(MutateSlot, originalCode.c_str());
                                if (!ItemHasCode(GetInventoryItemSafe(pPC, MutateSlot), originalCode))
                                {
                                    BS_ERROR("CRITICAL: live item mutation rollback failed (char=%d slot=%d)", CharID, MutateSlot);
                                }
                                else
                                {
                                    BS_WARNING("Live item mutation rolled back after consumption failure (char=%d result=%d)", CharID, deleteResult);
                                }
                            }
                        }
                    }

                }
            }
            else if (Type == KmtGameServerCommand::ActionSilk)
            {
                int CharID;
                int nSilk, nSilkGift, nSilkPoint;
                *pMsg >> CharID;
                *pMsg >> nSilk >> nSilkGift >> nSilkPoint;

                CGObjPC * pPC = g_pCGame->GetCharObjById(CharID);
                if (pPC != NULL)
                {
                    pPC->UpdateSilk(nSilk, nSilkGift, nSilkPoint, true);
                }
            }
            else if (Type == KmtGameServerCommand::ActionGold)
            {
                int CharID;
                __int64 nGold;
                int AddOrRemove;
                *pMsg >> CharID;
                *pMsg >> nGold;
                *pMsg >> AddOrRemove;
                CGObjPC * pPC = g_pCGame->GetCharObjById(CharID);
                if (pPC != NULL && pPC->m_pObjDataInstance != NULL && nGold > 0)
                {
                    if(AddOrRemove == 1)
                    {
                        const UINT64 currentGold = pPC->m_pObjDataInstance->Gold;
                        if (currentGold <= static_cast<UINT64>(0x7FFFFFFFFFFFFFFFLL - nGold))
                            pPC->UpdateGold(nGold);
                    }
                    else if(AddOrRemove == 0)
                    {
                        const UINT64 availablegold = pPC->m_pObjDataInstance->Gold;
                        if (availablegold >= static_cast<UINT64>(nGold)) {
                            pPC->RemoveGold(nGold);
                        }
                    }
                }
            }
            else if (Type == KmtGameServerCommand::ActionTownWorldLayer)
            {
                int WorldID;
                int LayerID;
                *pMsg >> WorldID;
                *pMsg >> LayerID;
                if (KmtGameServerCommand::IsValidWorldId(WorldID) &&
                    KmtGameServerCommand::IsValidLayerId(LayerID))
                    ToTownbyWorldAndLayerID(static_cast<DWORD>(WorldID), static_cast<WORD>(LayerID));
            }
            else if (Type == GETUP_AT_POS) {
                int CharID = 0;
                int GameWorldId = 0;
                int RegionId = 0;
                int PosX = 0;
                int PosY = 0;
                int PosZ = 0;
                *pMsg >> CharID >> GameWorldId >> RegionId >> PosX >> PosY >> PosZ;
                CGObjPC* pPC = g_pCGame->GetCharObjById(CharID);
                if (pPC != NULL)
                {
                    uint32_t runtimeWorldId = 0;
                    if (IsValidDestination(GameWorldId, RegionId, PosX, PosY, PosZ) &&
                        TryBuildRuntimeWorldId(GameWorldId, runtimeWorldId))
                    {
                        RessurectFixed(pPC, 3000, 3000);
                        if (!pPC->MoveTo(runtimeWorldId, KmtGameServerCommand::ToWireRegionId(RegionId), PosX, PosY, PosZ, 2))
                            pPC->MoveTo(runtimeWorldId, KmtGameServerCommand::ToWireRegionId(RegionId), PosX, PosY, PosZ, 1);
                    }
                }

            }
            else if (Type == TELEPORT_TO_POSITION_SAFE_ZONE_NO_JOB) {
                int CharID = 0;
                int GameWorldId = 0;
                int RegionId = 0;
                int PosX = 0;
                int PosY = 0;
                int PosZ = 0;
                *pMsg >> CharID >> GameWorldId >> RegionId >> PosX >> PosY >> PosZ;
                CGObjPC* pPC = g_pCGame->GetCharObjById(CharID);
                if (pPC != NULL)
                {
                    //printf("%d %p %d\n", pPC->IsInBattleField(), pPC->GetRgnTerrain(), pPC->GetJobState());
                    uint32_t runtimeWorldId = 0;
                    if(pPC->IsInBattleField() == 0 && pPC->GetJobState() == 4 &&
                       IsValidDestination(GameWorldId, RegionId, PosX, PosY, PosZ) &&
                       TryBuildRuntimeWorldId(GameWorldId, runtimeWorldId))
                    {
                        if (!pPC->MoveTo(runtimeWorldId, KmtGameServerCommand::ToWireRegionId(RegionId), PosX, PosY, PosZ, 2)) {
                            pPC->MoveTo(runtimeWorldId, KmtGameServerCommand::ToWireRegionId(RegionId), PosX, PosY, PosZ, 1);
                        }
                    }
                }

            }

#if 0 // Action 131 is retired; retained only as historical protocol documentation.
            else if (Type == KmtGameServerCommand::ActionRetiredItemChange)
            {
                int CharID;
                *pMsg >> CharID;

                int ItemID;
                *pMsg >> ItemID;

                byte nPlus;
                *pMsg >> nPlus;

                byte ItemSlot;
                *pMsg >> ItemSlot;

                byte AdvLevel;
                *pMsg >> AdvLevel;


                CGObjPC * pPC = g_pCGame->GetCharObjById(CharID);
                if(pPC != NULL)
                {
                    CGItem* pItem = pPC->GetItemChar(ItemSlot);
                    if (pItem != NULL)
                    {
                        if(pItem->InstanceItem->RefItemID == ItemID)
                        {
                            CMsg* pTmpMsg = CNetHelper::AllocMsg(0x0000, false);
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
                            const int nLen = pTmpMsg->m_wWriteDataArrayPos - MSG_HEADER_SIZE;

                            CMsg* pSmMsg = pPC->AllocMsg(0x5034);

                            pSmMsg->Write<int>(nLen);
                            if (nLen > 0)
                                pSmMsg->Write(pTmpMsg->m_pMsgBuffer + MSG_HEADER_SIZE, nLen);
                            *pSmMsg << pItem->InstanceItem->RefItemID;
                            *pSmMsg << nPlus;
                            *pSmMsg << AdvLevel;
                            pPC->SendMsg(pSmMsg);
                            CNetHelper::FreeMsg(pTmpMsg);

                        }

                    }
                }

            }
#endif
        }
        break;
            case READ_LOCK_INFO_FROM_SHARD: // locked item added all gameservers
            {
                INT64 ItemID64;
                *pMsg >> ItemID64;
                CSqlCon::AddLockedItem(ItemID64);
            }
                break;
            case READ_UNLOCK_INFO_FROM_SHARD: // unlocked items removed all
            {
                INT64 ItemID64;
                *pMsg >> ItemID64;
                CSqlCon::RemoveLockedItem(ItemID64);
            }
                break;
            case 0x5066:
            {
                // Retired custom timed-plus channel. Consume the old opcode
                // without retaining item state in GameServer memory.
            }
                break;
            case 0x5068:
            {
                // Retired custom timed-plus removal channel.
            }
                break;
            case 0x5069:
            {
                int SenderCharID;
                pMsg->Read<int>(SenderCharID);


                byte eChatType;
                pMsg->Read<BYTE>(eChatType);

                byte btChatIndex;
                pMsg->Read<BYTE>(btChatIndex);


                std::string strReceiverName;
                if(eChatType == PRIVATE) {
                    pMsg->ReadString(strReceiverName);

                }

                std::string strAsciiText;
                pMsg->ReadString(strAsciiText);


                byte LinkedItemSlot;
                pMsg->Read<BYTE>(LinkedItemSlot);
/*
                if (eChatType == E_CHAT_TYPE::PRIVATE)
                {
                    wchar_t szBuffer[512] = { 0 };
                    wsprintf(szBuffer, L"%s(TO):%s", std::wstring(strReceiverName.begin(), strReceiverName.end()).c_str(), strMsgText.c_str());
                    strMsgText = szBuffer;
                }

                CMsgStreamBuffer* pClientMsg = BuildChatItemLinkClientMsg(eChatType, strSenderName, strMsgText, nGameID, vItemInfo);

                if (eChatType == E_CHAT_TYPE::LOCAL_ALL || eChatType == E_CHAT_TYPE::LOCAL_GM)
                {
                    CGObjPC* pPC = GetCharObjById(nDBID);

                    if(pPC != NULL)
                        pPC->BroadcastMsgToNearbyPlayers(pClientMsg);

                }

                if (eChatType == E_CHAT_TYPE::PARTY)
                {
                    __OBJ_MGR_PLAYER_LST lstParty = CObjMgr::GetPartyMembers(nIdentifier);
                    __OBJ_MGR_PLAYER_LST_IT itMember = lstParty.begin();

                    for (; itMember != lstParty.end(); itMember++)
                    {
                        CGObjPC* pMember = *itMember;
                        CMsgStreamBuffer* pMemberMsg = CNetHelper::CopyMsg(pClientMsg, pMember);
                        pMember->SendMsgToPeer(pMemberMsg);


                        CMsgStreamBuffer* pBotMsg = pMember->AllocMsgForPeer(0x3026);
                        pBotMsg->Write(eChatType);
                        pBotMsg->WriteStringA(strSenderName);
                        pBotMsg->WriteStringA(std::string(strMsgTextOriginal.begin(), strMsgTextOriginal.end()));
                        pMember->SendMsgToPeer(pBotMsg);
                    }
                }

                if (eChatType == E_CHAT_TYPE::GUILD)
                {
                    __OBJ_MGR_PLAYER_LST lstGuild = CObjMgr::GetGuildMembers(nIdentifier);
                    __OBJ_MGR_PLAYER_LST_IT itMember = lstGuild.begin();

                    for (; itMember != lstGuild.end(); itMember++)
                    {
                        CGObjPC* pMember = *itMember;
                        CMsgStreamBuffer* pMemberMsg = CNetHelper::CopyMsg(pClientMsg, pMember);
                        pMember->SendMsgToPeer(pMemberMsg);
                    }
                }

                if (eChatType == E_CHAT_TYPE::UNION)
                {
                    __OBJ_MGR_PLAYER_LST lstAlliance = CObjMgr::GetAllianceMembers(nIdentifier);
                    __OBJ_MGR_PLAYER_LST_IT itMember = lstAlliance.begin();

                    for (; itMember != lstAlliance.end(); itMember++)
                    {
                        CGObjPC* pMember = *itMember;
                        CMsgStreamBuffer* pMemberMsg = CNetHelper::CopyMsg(pClientMsg, pMember);
                        pMember->SendMsgToPeer(pMemberMsg);
                    }
                }

                if (eChatType == E_CHAT_TYPE::ACADEMY)
                {
                    __OBJ_MGR_PLAYER_LST lstAcademy = CObjMgr::GetTrainingCampMembers(nIdentifier);
                    __OBJ_MGR_PLAYER_LST_IT itMember = lstAcademy.begin();

                    for (; itMember != lstAcademy.end(); itMember++)
                    {
                        CGObjPC* pMember = *itMember;
                        CMsgStreamBuffer* pMemberMsg = CNetHelper::CopyMsg(pClientMsg, pMember);
                        pMember->SendMsgToPeer(pMemberMsg);
                    }
                }

                if (eChatType == E_CHAT_TYPE::GLOBAL)
                {
                    //*must* use CObjMgr to get by gid only on this GS
                    IGObj* pPC = CObjMgr::GetPlayerByGameID(nGameID);
                    int nLastGlobalRefItemID = nIdentifier;

                    if (pPC != NULL)
                    {
                        if (CRefDataManager::IsGlobalChatMultiShard(nLastGlobalRefItemID))
                        {
                            CMiscDBSet* pDbSet = CDbMgr::GetMiscDbSet();

                            WORD wOldReadPos = pClientMsg->GetReadPos();
                            pClientMsg->SetReadPos(6);
                            int nDataLen = pClientMsg->GetWritePos() - pClientMsg->GetReadPos();
                            BYTE* pBuffer = new BYTE[nDataLen];
                            pClientMsg->Read(pBuffer, nDataLen);
                            pClientMsg->SetReadPos(wOldReadPos);

                            pDbSet->AddAutoNetMsgBroadcastRecord(pClientMsg->GetID(), false, false, pBuffer, nDataLen);

                            delete[] pBuffer;
                        }
                    }

                    //Not a multi-shard global, so simply send msg to all players
                    if (!CRefDataManager::IsGlobalChatMultiShard(nLastGlobalRefItemID))
                    {
                        CObjMgr::GetPlayerListACS()->Enter();

                        __OBJ_MGR_PLAYER_LST& lstPlayers = CObjMgr::GetPlayers();
                        __OBJ_MGR_PLAYER_LST_IT itPlayer = lstPlayers.begin();

                        for (; itPlayer != lstPlayers.end(); itPlayer++)
                        {
                            CGObjPC* pPlayer = *itPlayer;
                            CMsgStreamBuffer* pMemberMsg = CNetHelper::CopyMsg(pClientMsg, pPlayer);
                            pPlayer->SendMsgToPeer(pMemberMsg);
                        }

                        CObjMgr::GetPlayerListACS()->Leave();
                    }
                }

                if (eChatType == E_CHAT_TYPE::PRIVATE)
                {
                    CGObjPC* pSender = CObjMgr::GetPlayerByDBID(nDBID);
                    CGObjPC* pTarget = CObjMgr::GetPlayerByCharName(strReceiverName);

                    if (pSender != NULL)
                        pSender->SendMsgToPeer(CNetHelper::CopyMsg(pClientMsg, pSender));

                    if (pTarget != NULL)
                    {
                        wchar_t szBuffer[512] = { 0 };
                        wsprintf(szBuffer, L"%s(FROM):%s", std::wstring(strSenderName.begin(), strSenderName.end()).c_str(), strMsgTextOriginal.c_str());

                        CMsgStreamBuffer* pClientMsgForTarget = BuildChatItemLinkClientMsg(eChatType, strSenderName, szBuffer, nGameID, vItemInfo);
                        pTarget->SendMsgToPeer(CNetHelper::CopyMsg(pClientMsgForTarget, pTarget));
                    }
                }

                if (eChatType == E_CHAT_TYPE::STALL)
                {
                    __OBJ_MGR_PLAYER_LST lstStallMembers = CObjMgr::GetStallMembers(nIdentifier);
                    __OBJ_MGR_PLAYER_LST_IT itMember = lstStallMembers.begin();

                    for (; itMember != lstStallMembers.end(); itMember++)
                    {
                        CGObjPC* pMember = *itMember;
                        CMsgStreamBuffer* pMemberMsg = CNetHelper::CopyMsg(pClientMsg, pMember);
                        pMember->SendMsgToPeer(pMemberMsg);
                    }
                }
*/
         /*       CGObjPC *Sender = GetCharObjById(SenderCharID);
                if (Sender != NULL) {


                    if(eChatType == PRIVATE) {
                        CGObjPC *TargetChar = GetCharObjByName(strReceiverName.c_str());
                        if(TargetChar != NULL) {
                            printf("TargetCharIs Found %p\n", TargetChar);

                            CGItem* pItem = Sender->GetItemChar(LinkedItemSlot);
                            if (pItem != NULL) {
                                printf("Item found in slot %d\n", LinkedItemSlot);

                                CMsg* pTmpMsg = CNetHelper::AllocMsg(0x0000, false);
                                CNetHelper::BindStreamBufferWithMsg(pTmpMsg);

                                if ((pItem->InstanceItem->pCRefObjItem->TID.getTypeID1() == 3 &&
                                     pItem->InstanceItem->pCRefObjItem->TID.getTypeID2() == 2 &&
                                     pItem->InstanceItem->pCRefObjItem->TID.getTypeID3() == 1 &&
                                     pItem->InstanceItem->pCRefObjItem->TID.getTypeID4() == 1) ||
                                    (pItem->InstanceItem->pCRefObjItem->TID.getTypeID1() == 3 &&
                                     pItem->InstanceItem->pCRefObjItem->TID.getTypeID2() == 3 &&
                                     pItem->InstanceItem->pCRefObjItem->TID.getTypeID3() == 3 &&
                                     pItem->InstanceItem->pCRefObjItem->TID.getTypeID4() == 7) ||
                                    (pItem->InstanceItem->pCRefObjItem->TID.getTypeID1() == 3 &&
                                     pItem->InstanceItem->pCRefObjItem->TID.getTypeID2() == 2 &&
                                     pItem->InstanceItem->pCRefObjItem->TID.getTypeID3() == 1 &&
                                     pItem->InstanceItem->pCRefObjItem->TID.getTypeID4() == 2))
                                {
                                    pItem->WriteItemCosDataMsg((void*)g_CStreamBufferUnk, 1);
                                } else {
                                    pItem->WriteItemDataToMsg((void*)g_CStreamBufferUnk, 1);
                                }

                                CNetHelper::FlushStreamBufferMsg(pTmpMsg);
                                int nLen = pTmpMsg->m_wWriteDataArrayPos - 6;
                                printf("nLen: %d\n", nLen);

                                // Read item data from dummy msg
                                BYTE* pItemDataBuf = new BYTE[nLen];
                                pTmpMsg->SetReadPos(6);
                                pTmpMsg->Read(pItemDataBuf, nLen);

                                std::string sendercname = Sender->GetCharName();
                                CMsg* PacketForTarget = TargetChar->AllocMsg(0x705E);
                                *PacketForTarget << eChatType;
                                PacketForTarget->WriteString(sendercname);
                                PacketForTarget->WriteString(strAsciiText);
                                PacketForTarget->Write<int>(nLen);
                                PacketForTarget->Write(pItemDataBuf, nLen);
                                TargetChar->SendMsg(PacketForTarget);
                                printf("Sent packet to target.\n");


                                std::string sendercnametarget = TargetChar->GetCharName();
                                CMsg* PacketForTargetX = Sender->AllocMsg(0x705E);
                                *PacketForTargetX << eChatType;
                                PacketForTargetX->WriteString(sendercnametarget);
                                PacketForTargetX->WriteString(strAsciiText);
                                PacketForTargetX->Write<int>(nLen);
                                PacketForTargetX->Write(pItemDataBuf, nLen);
                                Sender->SendMsg(PacketForTargetX);
                                printf("Sent packet to target.\n");

                                delete[] pItemDataBuf;
                                CNetHelper::FreeMsg(pTmpMsg);
                            }
                        }
                    }
                } */
            }


            break;
        }
        reinterpret_cast<void (__stdcall *)(CGame *, CMsg *)>(0x00414260)(this, pMsg);
    }
    catch (...) {
        GameServerTelemetry::RecordMalformedPacket();
        pMsg->FlushRemainingBytes();
        WORD wMsgId = pMsg->GetMsgId();

        ServerFramework::ReportLog(LOG_TYPE_NOTIFY, " An Exception occurred in CGame::ProcessMessage() MsgID : %x",
                                   wMsgId);
        return;
    }
}
