//
// Created by YUMBUL on 19.03.2023.
//

#include <Game.h>
#include "GObjPC.h"
#include "GObjMob.h"
#include "RgnTerrain.h"
#include <sstream>
#include <memory/MemoryUtility.h>
#include <BSObj/BSObj.h>
#include <Objects/GItem.h>
#include <SqlConnection/sqlCon.h>
#include <SettingMgr/NewSettings.h>
#include <time.h>
#include <ReferenceData/ReferenceDataMgr.h>
#include <NetHelper.h>
#include <ctime>
#include <KMTGuardCustom/GameServerTelemetry.h>
#include <KMTGuardCustom/GameServerRuntimeSafety.h>
#include <GameServerCommandContract.h>


CGObjPC::FN_GETITEMATSTORAGESLOT CGObjPC::s_pfnGetItemAtStorageSlot;
CGObjPC::FN_OFFSET_EXP_POINT CGObjPC::s_pfnOffsetExpPoint;
#define OFFSET_EXP_POINT_FN_OFFSET					0x004E5250
#define GET_ITEM_AT_MAIN_STORAGE_SLOT_FN_OFFSET		0x004A68E0
#define BROADCAST_MSG_TO_NEARBY_PLAYERS_FN_OFFSET	0x00484D90
#define UPDATE_SILK_FN_OFFSET						0x004F0940
#define TELEPORT_TO_TARGET_POINT_FN_OFFSET			0x004DF290

IGObj* CGObjPC::GetItemAtStorageSlot(int nSlotIndex)
{
    return s_pfnGetItemAtStorageSlot(this, nSlotIndex);
}
void CGObjPC::Setup()
{
    s_pfnGetItemAtStorageSlot = reinterpret_cast<FN_GETITEMATSTORAGESLOT>(GET_ITEM_AT_MAIN_STORAGE_SLOT_FN_OFFSET);
    s_pfnOffsetExpPoint = reinterpret_cast<FN_OFFSET_EXP_POINT>(OFFSET_EXP_POINT_FN_OFFSET);
}
int CGObjPC::GetDBID() const
{
    if (this->m_pObjDataInstance == NULL)
        return -1;

    return this->m_pObjDataInstance->CharDBID;
}
BOOL CGObjPC::CanSendMessage()
{
    if (this->m_pObjDataInstance == NULL || this->m_pLifeState == NULL)
        return FALSE;

    //Get msg block state.

    SObjectStateFlags* state = MEMUTIL_READ_BY_PTR_OFFSET(this, 0x30, SObjectStateFlags*);
    if (state == NULL)
        return FALSE;

    //TEST
    if (this->m_pLifeState->m_btLifeState == LIFESTATE_ALIVE
        && this->m_pLifeState->m_btTeleportState == TELEPORTSTATE_NONE
        && state->btMsgBlockState != 1)
        return TRUE;

    return FALSE;
}

#pragma region restrict attack mobs

// restrict attack
enum AttackBlockReason : BYTE {
    REASON_UNIQUE_STR = 0,
    REASON_UNIQUE_INT = 1,

    REASON_ONLY_ON_JOB = 2,
    REASON_ONLY_OFF_JOB = 3,
    REASON_JOB_MASK = 4,
    REASON_CAPE_MASK = 5,
    REASON_RACE_MASK = 6,
    REASON_REQUIRE_PARTY = 7,
    REASON_REQUIRE_GUILD = 8
};

namespace
{
    enum UniqueStatType : BYTE
    {
        UNIQUE_STAT_NONE = 0,
        UNIQUE_STAT_STR = 1,
        UNIQUE_STAT_INT = 2
    };

    const DWORD ATTACK_BLOCK_NOTICE_INTERVAL_MS = 1000;
    std::map<int, BYTE> s_uniqueStatTypeByRefId;
    std::map<DWORD, DWORD> s_attackBlockNoticeTicks;
    CRITICAL_SECTION s_attackRestrictionCacheLock;

    struct AttackRestrictionCacheInitializer
    {
        AttackRestrictionCacheInitializer()
        {
            InitializeCriticalSection(&s_attackRestrictionCacheLock);
        }

        ~AttackRestrictionCacheInitializer()
        {
            DeleteCriticalSection(&s_attackRestrictionCacheLock);
        }
    } s_attackRestrictionCacheInitializer;

    class ScopedAttackRestrictionLock
    {
    public:
        ScopedAttackRestrictionLock()
        {
            EnterCriticalSection(&s_attackRestrictionCacheLock);
        }

        ~ScopedAttackRestrictionLock()
        {
            LeaveCriticalSection(&s_attackRestrictionCacheLock);
        }

    private:
        ScopedAttackRestrictionLock(const ScopedAttackRestrictionLock&);
        ScopedAttackRestrictionLock& operator=(const ScopedAttackRestrictionLock&);
    };

    BYTE GetUniqueStatType(CGObjMob* mob)
    {
        if (mob == NULL || mob->Monsterclass != 3)
            return UNIQUE_STAT_NONE;

        const int refId = mob->GetRefObjID();
        {
            ScopedAttackRestrictionLock lock;
            std::map<int, BYTE>::const_iterator cached = s_uniqueStatTypeByRefId.find(refId);
            if (cached != s_uniqueStatTypeByRefId.end())
                return cached->second;
        }

        BYTE result = UNIQUE_STAT_NONE;
        RefObjCommon* ref = REFDATA_MGR.GetRefObj(refId);
        if (ref != NULL)
        {
            const std::string& name = ref->m_strObjectName;
            if (name.find("_STR") != std::string::npos)
                result = UNIQUE_STAT_STR;
            else if (name.find("_INT") != std::string::npos)
                result = UNIQUE_STAT_INT;
        }

        size_t cacheSize = 0;
        {
            ScopedAttackRestrictionLock lock;
            s_uniqueStatTypeByRefId[refId] = result;
            cacheSize = s_uniqueStatTypeByRefId.size();
        }
        GameServerTelemetry::SetUniqueTypeCacheSize(cacheSize);
        return result;
    }

    bool GetUniqueStatAttackBlockReason(CGObjMob* mob, CGObjPC* player, BYTE& reason)
    {
        if (player == NULL || player->m_pObjDataInstance == NULL)
            return false;

        const BYTE statType = GetUniqueStatType(mob);
        const int strength = player->m_pObjDataInstance->Strength;
        const int intellect = player->m_pObjDataInstance->Intellect;
        if (statType == UNIQUE_STAT_STR && strength < intellect)
        {
            reason = REASON_UNIQUE_STR;
            return true;
        }
        if (statType == UNIQUE_STAT_INT && strength > intellect)
        {
            reason = REASON_UNIQUE_INT;
            return true;
        }
        return false;
    }

    void SendAttackBlockMessage(CGObjPC* player, BYTE reason)
    {
        if (player == NULL)
            return;

        const DWORD playerId = player->GetGameID();
        const DWORD nowTick = GetTickCount();
        bool shouldSend = false;

        {
            ScopedAttackRestrictionLock lock;
            std::map<DWORD, DWORD>::iterator last = s_attackBlockNoticeTicks.find(playerId);
            if (last == s_attackBlockNoticeTicks.end() ||
                (nowTick - last->second) >= ATTACK_BLOCK_NOTICE_INTERVAL_MS)
            {
                s_attackBlockNoticeTicks[playerId] = nowTick;
                shouldSend = true;
            }
        }

        if (!shouldSend)
            return;

        CMsg* message = player->AllocMsg(0x3563);
        if (message == NULL)
            return;
        *message << reason;
        player->SendMsg(message);
    }
}

static inline BYTE MapJobStateToBit(int jobState) {
    // Game states: 1=Trader, 2=Thief, 3=Hunter, 4=None
    // Mask bits  : Trader=1, Hunter=2, Thief=4
    switch (jobState) {
    case 1: return 1; // Trader -> bit 1
    case 3: return 2; // Hunter -> bit 2
    case 2: return 4; // Thief  -> bit 4
    default: return 0; // None/unknown
    }
}

static inline BYTE MapCapeToBit(int cape) {
    if (cape == 1) return 1; // Cape1
    if (cape == 3) return 2; // Cape3
    return 0;
}
static inline BYTE GetRaceBitSafe(CGObjPC* pc) {
    if (!pc) return 0;
    const int refId = pc->GetRefObjID();

    // CH: 1907..1932 (male 1907..1919, female 1920..1932)
    if (refId >= 1907 && refId <= 1932)
        return 1;

    // EU: 14875..14900 (male 14875..14887, female 14888..14900)
    if (refId >= 14875 && refId <= 14900)
        return 2;

    return 0; // close it
}

static inline int GetGenderFromRef(CGObjPC* pc) {
    if (!pc) return -1;
    const int refId = pc->GetRefObjID();
    if ((refId >= 1907 && refId <= 1919) || (refId >= 14875 && refId <= 14887))
        return 0; // male
    if ((refId >= 1920 && refId <= 1932) || (refId >= 14888 && refId <= 14900))
        return 1; // female
    return -1; // unknown
}

static inline bool HasGuild(CGObjPC* pc) {
    return (pc && pc->MyGuild && pc->MyGuild->IntanceGuild);
}
static inline bool HasParty(CGObjPC* pc) {
    return pc != NULL && pc->MyParty != NULL;
}
//(RefObjID)
static inline bool EvaluateAttackRestriction(CGObjPC* pPC, CGObjMob* pMob, BYTE& outReason)
{
    outReason = 0xFF;
    if (!pPC || !pMob) return true;

    const int mobRefId = pMob->GetRefObjID();
    SAttackRestrictionByMob r;
    if (!CSqlCon::TryGetAttackRestriction(mobRefId, r))
        return true;
    const bool isOnJob = (pPC->GetJobState() != 4 /*ADHOC_JOB_STATE_NONE*/);

    // (1) OnlyOnJob / OnlyOffJob
    if (r.OnlyOnJob && !isOnJob) { outReason = REASON_ONLY_ON_JOB; return false; }
    if (r.OnlyOffJob && isOnJob) { outReason = REASON_ONLY_OFF_JOB; return false; }

    // (2) STR / INT
    if (r.OnlyStrPlayer || r.OnlyIntPlayer) {
        const int Str = (pPC->m_pObjDataInstance ? pPC->m_pObjDataInstance->Strength : 0);
        const int Int = (pPC->m_pObjDataInstance ? pPC->m_pObjDataInstance->Intellect : 0);
        if (r.OnlyStrPlayer && Str < Int) { outReason = REASON_UNIQUE_STR; return false; }
        if (r.OnlyIntPlayer && Int < Str) { outReason = REASON_UNIQUE_INT; return false; }
    }

    // (3) Job mask 
    if (r.AllowedJobMask != 0) {
        BYTE jbit = MapJobStateToBit(pPC->GetJobState());
        if ((jbit & r.AllowedJobMask) == 0) { outReason = REASON_JOB_MASK; return false; }
    }
    else {
        if (r.OnlyByTrader || r.OnlyByThief) {
            BYTE jbit = MapJobStateToBit(pPC->GetJobState());
            if (r.OnlyByTrader && (jbit & 1) == 0) { outReason = REASON_JOB_MASK; return false; }
            if (r.OnlyByThief && (jbit & 4) == 0) { outReason = REASON_JOB_MASK; return false; }
        }
    }

    // (4) Cape mask
    if (r.AllowedCapeMask != 0) {
        BYTE cbit = MapCapeToBit(pPC->PvpCape);
        if ((cbit & r.AllowedCapeMask) == 0) { outReason = REASON_CAPE_MASK; return false; }
    }

    // (5) Race mask
    if (r.AllowedRaceMask != 0) {
        BYTE rbit = GetRaceBitSafe(pPC);
        if (rbit == 0 || (rbit & r.AllowedRaceMask) == 0) { outReason = REASON_RACE_MASK; return false; }
    }

    // (6) Party/Guild
    if (r.RequireGuild == 1 && !HasGuild(pPC)) { outReason = REASON_REQUIRE_GUILD; return false; }
    if (r.RequireParty == 1 && !HasParty(pPC)) { outReason = REASON_REQUIRE_PARTY; return false; }

    return true;
}

bool CRegionAttackRestrictionsMgr::CanMobInteractWithPlayer(CGObjMob* pMob, CGObjPC* pPC, BYTE* pReason)
{
    if (pMob != NULL && pPC != NULL && InterlockedCompareExchange(&s_towerDefenseEnabled, 0, 0) == 1)
    {
        SWorldID worldId;
        SPosInfo position;
        pPC->GetWorldID(worldId);
        pPC->GetPosInfo(position);

        const int configuredWorldId = InterlockedCompareExchange(&s_towerDefenseWorldId, 0, 0);
        const int configuredRegionId = InterlockedCompareExchange(&s_towerDefenseRegionId, 0, 0);
        const int currentWorldId = worldId.wWorldID % 65536;
        if (currentWorldId == configuredWorldId && position.wRegionID == configuredRegionId)
        {
            const int mobRefId = pMob->GetRefObjID();
            const int team1MobId = InterlockedCompareExchange(&s_towerDefenseTeam1MobId, 0, 0);
            const int team1Cape = InterlockedCompareExchange(&s_towerDefenseTeam1Cape, 0, 0);
            const int team2MobId = InterlockedCompareExchange(&s_towerDefenseTeam2MobId, 0, 0);
            const int team2Cape = InterlockedCompareExchange(&s_towerDefenseTeam2Cape, 0, 0);

            if ((mobRefId == team1MobId && pPC->PvpCape == team1Cape) ||
                (mobRefId == team2MobId && pPC->PvpCape == team2Cape))
            {
                if (pReason != NULL)
                    *pReason = REASON_CAPE_MASK;
                return false;
            }
        }
    }

    BYTE reason = 0xFF;
    const bool allowed = EvaluateAttackRestriction(pPC, pMob, reason);
    if (pReason != NULL)
        *pReason = reason;
    return allowed;
}

void CRegionAttackRestrictionsMgr::ForgetPlayer(CGObjPC* pPC)
{
    if (pPC == NULL)
        return;

    ScopedAttackRestrictionLock lock;
    s_attackBlockNoticeTicks.erase(pPC->GetGameID());
}
#pragma endregion

void CGObjPC::GiveExp(DWORD dwExpSourceObjUniqueID, long long qwLevelExp, long long qwSkillExp, DWORD& dwUnk)
{
    if (qwLevelExp < 0 && this->GetLevel() == 101)
    {
        qwLevelExp = 0;
    }
    reinterpret_cast<void(__thiscall*)(CGObjPC*, DWORD, long long, long long, DWORD&)>(0x004E5250)(this, dwExpSourceObjUniqueID, qwLevelExp, qwSkillExp, dwUnk);
}
void CGObjPC::Ressurect(float fRecoverHpMult, float fRecoverMpMult)
{
    ////Ressurect first..
    const DWORD dwCallAddr = TELEPORT_TO_TARGET_POINT_FN_OFFSET;

    __asm pushad;
    __asm pushfd;

    __asm push 1;
    __asm mov eax, dword ptr[this];
    __asm call dwCallAddr;

    __asm popfd;
    __asm popad;

    //Update HP MP
    this->UpdateHpMp(this->GetMaxHealth() * fRecoverHpMult, this->GetMaxMana() * fRecoverMpMult);
}
void CGObjPC::UpdateHpMp(int nHp, int nMp)
{
    reinterpret_cast<void(__stdcall*)(CGObjPC*, int, int, int)>(0x004A86A0)(this, nHp, nMp, 8);
}
unsigned int CGObjPC::CharacterSpawn_VfTable391()
{
    // BODYMODE_GM_INVISIBLE is serialized by the native spawn routine. Clear
    // only that state before the original call so an enabled server always
    // publishes GM characters as visible without affecting normal invisibility.
    if (CNewSettings::m_Settings != NULL &&
        CNewSettings::m_Settings->ForceGmVisibleOnSpawn &&
        this->m_pLifeState != NULL &&
        this->m_pLifeState->m_btBodyState == BODYMODE_GM_INVISIBLE)
    {
        this->m_pLifeState->m_btBodyState = BODYMODE_NORMAL;
    }

    unsigned int a = reinterpret_cast<unsigned int(__thiscall*)(CGObjPC*)>(0x004df9e0)(this);
    int is = m_PCInventory.m_nSlotsCount;
    for (int i = 0; i < is; i++)
    {
        CGItem* item = this->GetItemChar(i);
        if (item != NULL)
        {
            INT64 ID64 = item->ID64;
            if (CSqlCon::IsItemLocked(ID64))
            {
                CMsg* newpMsg = this->AllocMsg(0xF200);
                if (newpMsg == NULL)
                    continue;
                *newpMsg << byte(0x0);
                *newpMsg << byte(i);
                this->SendMsg(newpMsg);
            }
        }
    }
    int irr = 0;
    for (std::vector<CGItem*>::iterator it = m_PCAvatarInventory.m_vecItems.begin(); it != m_PCAvatarInventory.m_vecItems.end(); it++)
    {
        irr++;
        if ((*it) != NULL && (*it)->InstanceItem != NULL)
        {


            if (CSqlCon::IsItemLocked((*it)->InstanceItem->ID64))
            {
                CMsg* newpMsg = this->AllocMsg(0xF200);
                if (newpMsg == NULL)
                    continue;
                byte type = 1;
                *newpMsg << (byte)type;
                *newpMsg << irr;
                this->SendMsg(newpMsg);
            }
        }

    }

    SWorldID mineWorldID;
    this->GetWorldID(mineWorldID);
    CMsg* info = this->AllocMsg(0x5035);
    if (info == NULL)
        return a;
    *info << mineWorldID.wLayerID;
    *info << mineWorldID.wWorldID;
    byte jobstate = this->GetJobState();
    *info << jobstate;
    *info << this->RegionID;
    if(this->GetJobState() != 4)
    {
        std::string Nicknm = this->GetNickName();
        *info << Nicknm;
    }
    this->SendMsg(info);
    return a;
}
///\GetRgnTerrain - Contains all the information about player current region;
CRgnTerrain *CGObjPC::GetRgnTerrain() const {
    return MEMUTIL_READ_BY_PTR_OFFSET(this, 0x128, CRgnTerrain *);
}

bool CGObjPC::IsInBattleField() const {

    if (this->GetRgnTerrain() != NULL) {
        return this->GetRgnTerrain()->GetZoneState();
    }

    return false;
}
void CGObjPC::CharacterStats()
{
    reinterpret_cast<void(__thiscall*)(CGObjPC*)>(0x004ebe30)(this);
}

void CGObjPC::FUNCMovement(unsigned int param)
{
    //printf("%p 22222222222222222\n", param);
    reinterpret_cast<void(__thiscall*)(CGObjPC*, unsigned int)>(0x004b0ea0)(this, param);
}
void CGObjPC::SetCharState(char a2) {
    reinterpret_cast<void(__thiscall *)(CGObjPC *, char, int, float)>(0x004A9C80)(this, a2, 255, 0.f);
}
void CGObjPC::FuncJobExp(unsigned int p1, unsigned int p2, unsigned int p3)
{
    reinterpret_cast<void(__thiscall*)(CGObjPC*, unsigned int, unsigned int, unsigned int)>(0x004e27c0)(this, p1, p2, p3);
}
void CGObjPC::FuncJobExpTest2(unsigned int p1)
{
    reinterpret_cast<void(__thiscall*)(CGObjPC*, unsigned int)>(0x004e6820)(this, p1);
}
void CGObjPC::Func_201() {
    reinterpret_cast<void(__thiscall*)(CGObjPC*)>(0x004f1640)(this);
}
void CGObjPC::GroupSpawn(unsigned int p1, int p2)
{
    reinterpret_cast<void(__thiscall*)(CGObjPC*, unsigned int, int)>(0x004e5b00)(this, p1, p2);
}
void CGObjPC::DealDamage(int nReasonMask, int nAmount)
{
    reinterpret_cast<void(__thiscall*)(CGObjPC*,LPVOID , int , int unkZero_1, DWORD , int unkZero_2)>(0x0052AA30)(this, NULL, nAmount, 0, nReasonMask, 0);
}
void CGObjPC::KillLoggerFunction(CGObjPC* pKillerPC, int param_2)
{
    if (param_2 == 53 &&
        (pKillerPC == NULL || this->GetDBID() <= 0 || pKillerPC->GetDBID() <= 0))
    {
        GameServerTelemetry::RecordRuntimeError();
        return reinterpret_cast<void(__thiscall*)(CGObjPC*, CGObjPC*, int)>(0x004a7540)(this, pKillerPC, param_2);
    }

    if (param_2 == 53 && pKillerPC != NULL)
    {
        CGObjPC * pVictim = this;
        CMsg* pMsg = pVictim->AllocMsg(0x5013);
        if (pMsg == NULL)
        {
            GameServerTelemetry::RecordRuntimeError();
            return reinterpret_cast<void(__thiscall*)(CGObjPC*, CGObjPC*, int)>(0x004a7540)(this, pKillerPC, param_2);
        }

        SPosInfo mineposbaby;
        pVictim->GetPosInfo(mineposbaby);

        SWorldID mineworldbaby;
        pVictim->GetWorldID(mineworldbaby);

        *pMsg << (int)mineworldbaby.dwWorldID;
        *pMsg << (int)mineposbaby.wRegionID;

        *pMsg << (int)pKillerPC->GetDBID();
        std::string killerCharName= pKillerPC->GetCharName();
        *pMsg << killerCharName;
        *pMsg << (byte)(pKillerPC->PvpCape);
        *pMsg << (byte)(pKillerPC->GetJobState());
        std::string EmptyGuildName = "dummy";
        if(pKillerPC->MyGuild != NULL && pKillerPC->MyGuild->IntanceGuild != NULL) {

            *pMsg << (int)(pKillerPC->MyGuild->IntanceGuild->GuildID);
            std::string pKillerGuildName = pKillerPC->MyGuild->IntanceGuild->MyGuildName;
            *pMsg << pKillerGuildName;
        }
        else
        {
            *pMsg << (int)(0);
            *pMsg << EmptyGuildName;
        }


        *pMsg << (int)pVictim->GetDBID();
        std::string pVictimCN = pVictim->GetCharName();

        *pMsg << pVictimCN;
        *pMsg << (byte)(pVictim->PvpCape);
        *pMsg << (byte)(pVictim->GetJobState());

        if(pVictim->MyGuild != NULL && pVictim->MyGuild->IntanceGuild != NULL) {

            *pMsg << (int)(pVictim->MyGuild->IntanceGuild->GuildID);
            std::string pKillerGuildName = pVictim->MyGuild->IntanceGuild->MyGuildName;
            *pMsg << pKillerGuildName;
        }
        else
        {
            *pMsg << (int)(0);
            *pMsg << EmptyGuildName;
        }


        this->SendMsg(pMsg); /// kesene paket gönder

    }
    return reinterpret_cast<void(__thiscall*)(CGObjPC*, CGObjPC*, int)>(0x004a7540)(this, pKillerPC, param_2);
}


void CGObjPC::UpdateHwan(char hwanLevel)
{
    reinterpret_cast<void(__thiscall*)(CGObjPC*, char)>(0x004A9F40)(this, hwanLevel);
}
void CGObjPC::UpdatePVPCapeType(BYTE CapeType)
{
    // Avoid unnecesary updates
    if (CapeType <= 5)
        reinterpret_cast<void(__thiscall*)(CGObjPC*, BYTE)>(0x004F0E90)(this, CapeType);
}
bool CGObjPC::MoveTo(uint32_t param_1, unsigned short param_2, float param_3, float param_4, float param_5, unsigned int param_6)
{
    return reinterpret_cast<bool(__thiscall*)(CGObjPC*, uint32_t*, unsigned short, float, float, float, unsigned int)>(0x004df590)(this, &param_1, param_2, param_3, param_4, param_5, param_6);
}
int CGObjPC::SetLiveDeleteItem(int slot, int reducecount)
{
    int operation_result = 0;
    CallVirtual<void(__thiscall*)(CGObjPC*, int, int, int, int*, int, int)>(this, 140)(this, 0, slot, reducecount, &operation_result, 4, 0);
    return operation_result;
}
void CGObjPC::SetLiveItem(int slot, const char* itemcode)
{
    return CallVirtual<void(__thiscall*)(CGObjPC*, int, const char*)>(this, 141)(this, slot, itemcode);
}
void CGObjPC::UpdateSilk(int nSilk, int nSilkGift, int nSilkPoint, bool bSendPacket)
{
    const DWORD dwCallAddr = UPDATE_SILK_FN_OFFSET;

    __asm movzx eax, bSendPacket;

    __asm push eax;
    __asm mov esi, this;
    __asm mov ecx, nSilkPoint;
    __asm mov edx, nSilkGift;
    __asm mov eax, nSilk;

    __asm call dwCallAddr;
}
void CGObjPC::EngageBuffSkill(CGObjPC* pPC, const char* szSkillCodeName128)
{
    const DWORD dwCGObjPC_EngageBuffSkill_Addr = 0x004F1800;

    const double dblUnk1 = 1.00;
    const int nUnk2 = 1;

    __asm push nUnk2;
    __asm push szSkillCodeName128;
    __asm fld dblUnk1;
    __asm mov ecx, pPC;
    __asm call dwCGObjPC_EngageBuffSkill_Addr;
}
void CGObjPC::LiveSkill(int SkillID)
{
    DWORD32* Skillinfo = reinterpret_cast<DWORD32 * (__stdcall*)()>(0x5AA0B0)();
    Skillinfo[2] = SkillID;
    reinterpret_cast<int(__stdcall*)(DWORD32, DWORD32*)>(0x59B480)((DWORD32)this + 0xA30, Skillinfo);
}
unsigned __int16 CGObjPC::AddItem(const char* Codename, __int32 Amount, bool RandomizeStats, __int8 OptLevel)
{
    __int16 operation_result = 0;
    CallVirtual<void(__thiscall*)(CGObjPC*, __int32, const char*, __int32, __int16*, __int8, __int32, __int32, __int8, __int32, __int32, __int8, __int32)>(this, 139)(this, 0, Codename, Amount, &operation_result, 0, RandomizeStats ? 1 : 0, 0, OptLevel, 0, 1, 0, 0);
    return operation_result;
}
void CGObjPC::UpdateGold(__int64 Offset)
{
    // Check if offset is higher than int.MaxValue to avoid send a bugged message
    if (Offset > 0x7FFFFFFF)
        UpdateGold(Offset, 25, true, false);
    else
        UpdateGold(Offset, 25, true, true);
}
void CGObjPC::RemoveGold(__int64 Offset)
{
    // Check if offset is higher than int.MaxValue to avoid send a bugged message
    if (Offset > 0x7FFFFFFF)
        UpdateGold(-Offset, 25, true, false);
    else
        UpdateGold(-Offset, 25, true, true);
}
void CGObjPC::UpdateGold(__int64 Offset, __int32 Unknown, bool Realtime, bool ShowMessage)
{
    CallVirtual<void(__thiscall*)(CGObjPC*, __int64, __int32, __int32, __int32)>(this, 91)(this, Offset, Unknown, (Realtime ? 1 : 0), (ShowMessage ? 1 : 0));
}
void CGObjPC::TeleportToTown()
{
    try
    {
        const DWORD dwCallAddr = TELEPORT_TO_TARGET_POINT_FN_OFFSET;

        __asm pushad;
        __asm pushfd;

        __asm push 0;
        __asm mov eax, dword ptr[this];
        __asm call dwCallAddr;

        __asm popfd;
        __asm popad;
    }
    catch (...)
    {
        BS_INFO("%s - exception occoured on teleport to town", __FUNCTION__);
    }
}

CMsg* CGObjPC::AllocMsg(unsigned int param_1)
{
    CMsg* message = CallVirtual<CMsg*(__thiscall*)(CGObjPC*, unsigned int)>(this, 158)(this, param_1);
    if (message != NULL) message->ResetWriteState();
    return message;
}
void CGObjPC::SendMsg(CMsg* param_1)
{
    if (param_1 == NULL) return;
    if (param_1->HasWriteOverflow())
    {
        GameServerTelemetry::RecordMalformedPacket();
        CNetHelper::FreeMsg(param_1);
        return;
    }
    reinterpret_cast<void(__thiscall*)(CGObjPC*, CMsg*)>(0x004E0860)(this, param_1);
}
void CGObjPC::BroadcastMsgToNearbyPlayers(CMsg* pMsg)
{
    if (pMsg == NULL || pMsg->HasWriteOverflow())
    {
        if (pMsg != NULL)
        {
            GameServerTelemetry::RecordMalformedPacket();
            CNetHelper::FreeMsg(pMsg);
        }
        return;
    }
    const DWORD dwCallAddr = BROADCAST_MSG_TO_NEARBY_PLAYERS_FN_OFFSET;
    SWorldID worldID;
    SWorldID* pstWorldID = &worldID;

    __asm mov esi, this;
    __asm mov edi, pMsg;
    __asm mov ecx, pstWorldID;
    __asm call dwCallAddr;
}

#define ON_IGOBJ_ATTACK_REQ_FN_OFFSET        0x004aa640

CRegionAttackRestrictionsMgr::FN_IOBJ_CAN_ATTACK CRegionAttackRestrictionsMgr::s_pfnIGObj_CanAttack;
namespace { bool s_attackRestrictionDetourInstalled = false; }
volatile LONG CRegionAttackRestrictionsMgr::s_towerDefenseEnabled = 0;
volatile LONG CRegionAttackRestrictionsMgr::s_towerDefenseWorldId = 0;
volatile LONG CRegionAttackRestrictionsMgr::s_towerDefenseRegionId = 0;
volatile LONG CRegionAttackRestrictionsMgr::s_towerDefenseTeam1MobId = 0;
volatile LONG CRegionAttackRestrictionsMgr::s_towerDefenseTeam1Cape = 0;
volatile LONG CRegionAttackRestrictionsMgr::s_towerDefenseTeam2MobId = 0;
volatile LONG CRegionAttackRestrictionsMgr::s_towerDefenseTeam2Cape = 0;
volatile LONG CRegionAttackRestrictionsMgr::s_freeForAllEnabled = 0;
volatile LONG CRegionAttackRestrictionsMgr::s_freeForAllWorldId = 0;
volatile LONG CRegionAttackRestrictionsMgr::s_freeForAllRegionId = 0;

void CRegionAttackRestrictionsMgr::ConfigureTowerDefense(bool enabled, int worldId, int regionId,
                                                          int team1MobId, int team1Cape,
                                                          int team2MobId, int team2Cape)
{
    if (!enabled)
    {
        InterlockedExchange(&s_towerDefenseEnabled, 0);
        return;
    }

    InterlockedExchange(&s_towerDefenseWorldId, worldId);
    InterlockedExchange(&s_towerDefenseRegionId, KmtGameServerCommand::NormalizeRegionIdForCompare(regionId));
    InterlockedExchange(&s_towerDefenseTeam1MobId, team1MobId);
    InterlockedExchange(&s_towerDefenseTeam1Cape, team1Cape);
    InterlockedExchange(&s_towerDefenseTeam2MobId, team2MobId);
    InterlockedExchange(&s_towerDefenseTeam2Cape, team2Cape);
    InterlockedExchange(&s_towerDefenseEnabled, 1);
}

void CRegionAttackRestrictionsMgr::ConfigureFreeForAll(bool enabled, int worldId, int regionId)
{
    if (!enabled)
    {
        InterlockedExchange(&s_freeForAllEnabled, 0);
        GameServerTelemetry::SetFreeForAllState(false);
        return;
    }
    InterlockedExchange(&s_freeForAllWorldId, worldId);
    InterlockedExchange(&s_freeForAllRegionId, KmtGameServerCommand::NormalizeRegionIdForCompare(regionId));
    InterlockedExchange(&s_freeForAllEnabled, 1);
    GameServerTelemetry::SetFreeForAllState(true);
}

bool CRegionAttackRestrictionsMgr::Initialize()
{
    s_pfnIGObj_CanAttack = reinterpret_cast<FN_IOBJ_CAN_ATTACK>(
            ON_IGOBJ_ATTACK_REQ_FN_OFFSET
    );

    s_attackRestrictionDetourInstalled = GameServerRuntimeSafety::AttachDetour(
        reinterpret_cast<PVOID*>(&s_pfnIGObj_CanAttack),
        reinterpret_cast<PVOID>(CRegionAttackRestrictionsMgr::MyIGObj_CanAttack),
        "attack restriction detour");
    return s_attackRestrictionDetourInstalled;
}

void CRegionAttackRestrictionsMgr::Shutdown()
{
    ConfigureTowerDefense(false, 0, 0, 0, 0, 0, 0);
    ConfigureFreeForAll(false, 0, 0);
    if (s_attackRestrictionDetourInstalled)
    {
        GameServerRuntimeSafety::DetachDetour(
            reinterpret_cast<PVOID*>(&s_pfnIGObj_CanAttack),
            reinterpret_cast<PVOID>(CRegionAttackRestrictionsMgr::MyIGObj_CanAttack),
            "attack restriction detour");
        s_attackRestrictionDetourInstalled = false;
    }
}


BOOL __fastcall CRegionAttackRestrictionsMgr::MyIGObj_CanAttack(IGObj* pObjFirst, void* /* dummy edx */, IGObj* pObjSecond, int a3, WORD& pwResultCode)
{
    //Dunno if this ever happens, just in case better check !
    if (pObjSecond == NULL)
        return s_pfnIGObj_CanAttack(pObjFirst, pObjSecond, a3, pwResultCode);

    try
    {
        IGObj* pAttacker = NULL;
        IGObj* pVictim = NULL;
        CGObjPC* pPC = NULL;

        if (pObjFirst != NULL)
            pAttacker = (pObjFirst->IsCOS() ? pObjFirst->GetOwner() : pObjFirst);

        if (pObjSecond != NULL)
            pVictim = (pObjSecond->IsCOS() ? pObjSecond->GetOwner() : pObjSecond);

        if (pVictim == NULL)
            return s_pfnIGObj_CanAttack(pObjFirst, pObjSecond, a3, pwResultCode);

        if (pAttacker == NULL)
            return s_pfnIGObj_CanAttack(pObjFirst, pObjSecond, a3, pwResultCode);

        // Native cape rules reject cape-less participants. During an active
        // FFA round, override only direct PC-to-PC eligibility inside the
        // exact configured arena. Other target types and locations continue
        // through the native decision path below.
        if (InterlockedCompareExchange(&s_freeForAllEnabled, 0, 0) == 1 &&
            pObjFirst->IsPC() && pObjSecond->IsPC())
        {
            CGObjPC* attackerPC = reinterpret_cast<CGObjPC*>(pObjFirst);
            CGObjPC* victimPC = reinterpret_cast<CGObjPC*>(pObjSecond);
            if (pObjFirst != pObjSecond && attackerPC->GetHealth() > 0 && victimPC->GetHealth() > 0 &&
                attackerPC->CanSendMessage() && victimPC->CanSendMessage())
            {
                SWorldID attackerWorld;
                SWorldID victimWorld;
                SPosInfo attackerPos;
                SPosInfo victimPos;
                attackerPC->GetWorldID(attackerWorld);
                victimPC->GetWorldID(victimWorld);
                attackerPC->GetPosInfo(attackerPos);
                victimPC->GetPosInfo(victimPos);

                const LONG configuredWorld = InterlockedCompareExchange(&s_freeForAllWorldId, 0, 0);
                const LONG configuredRegion = InterlockedCompareExchange(&s_freeForAllRegionId, 0, 0);
                const LONG attackerWorldId = attackerWorld.wWorldID % 65536;
                const LONG victimWorldId = victimWorld.wWorldID % 65536;
                if (attackerWorldId == configuredWorld &&
                    victimWorldId == configuredWorld &&
                    attackerPos.wRegionID == configuredRegion &&
                    victimPos.wRegionID == configuredRegion)
                {
                    pwResultCode = 0;
                    return TRUE;
                }
            }
            // Only override the native target decision inside the configured
            // FFA arena. Outside that exact match, preserve the native path;
            // forcing FALSE here makes valid native PvP targets report
            // "Invalid target" whenever the runtime world/region snapshot
            // differs from the event configuration.
        }

        // Apply the immutable STR/INT unique rule once at the authoritative
        // combat decision point. ReaderPacket no longer parses attack packets.
        if (pAttacker->IsPC() && pVictim->IsMonster())
        {
            CGObjPC* attackerPC = reinterpret_cast<CGObjPC*>(pAttacker);
            CGObjMob* victimMob = reinterpret_cast<CGObjMob*>(pVictim);
            BYTE statReason = 0xFF;
            if (GetUniqueStatAttackBlockReason(victimMob, attackerPC, statReason))
            {
                SendAttackBlockMessage(attackerPC, statReason);
                pwResultCode = 0x3006;
                return FALSE;
            }
        }

        // Apply __AttackRestrictions in both directions. COS attacks are
        // evaluated using their owner so pets cannot bypass the same rule.
        CGObjMob* restrictedMob = NULL;
        CGObjPC* restrictedPC = NULL;

        if (pAttacker->IsPC() && pVictim->IsMonster())
        {
            restrictedPC = reinterpret_cast<CGObjPC*>(pAttacker);
            restrictedMob = reinterpret_cast<CGObjMob*>(pVictim);
        }
        else if (pAttacker->IsMonster() && pVictim->IsPC())
        {
            restrictedMob = reinterpret_cast<CGObjMob*>(pAttacker);
            restrictedPC = reinterpret_cast<CGObjPC*>(pVictim);
        }

        BYTE restrictionReason = 0xFF;
        if (restrictedMob != NULL && restrictedPC != NULL &&
            !CanMobInteractWithPlayer(restrictedMob, restrictedPC, &restrictionReason))
        {
            if (pAttacker->IsPC())
                SendAttackBlockMessage(restrictedPC, restrictionReason);
            pwResultCode = 0x3006;
            return FALSE;
        }

        // This hook is intentionally scoped to __AttackRestrictions. Legacy
        // code below is retained for source compatibility but is not part of
        // the active decision path.
        return s_pfnIGObj_CanAttack(pObjFirst, pObjSecond, a3, pwResultCode);

#if 0
        if (pObjFirst->IsPC() && pObjSecond->IsMonster())
        {
            CGObjMob* Mob = reinterpret_cast<CGObjMob*>(
                    g_pCGame->GetObjByGameID(pObjSecond->GetGameID()));
            CGObjPC* pPC = reinterpret_cast<CGObjPC*>(
                    g_pCGame->GetObjByGameID(pObjFirst->GetGameID()));
            if (Mob != NULL) {
                std::string charname = Mob->GetCodeName();
                if (Mob->Monsterclass == 3) {
                    size_t pos_f = charname.find("_STR");
                    size_t pos_f2 = charname.find("_INT");
                    int Str = pPC->m_pObjDataInstance->Strength;
                    int Int = pPC->m_pObjDataInstance->Intellect;
                    if (pos_f != -1 && Str < Int)/// e�er mob str ise ve
                    {
                        pwResultCode = 0x3006;
                        return FALSE;
                    }
                    else if (pos_f2 != -1 && Str > Int) {
                        pwResultCode = 0x3006;
                        return FALSE;
                    }
                }
            }

            if (pPC != NULL && Mob != NULL)
            {
                if (pPC->PvpCape == 3)
                {
                    std::string strUniqueCodeName;
                    RefObjCommon* pRefObj = REFDATA_MGR.GetRefObj(Mob->GetRefObjID());
                    if (pRefObj != NULL)
                        strUniqueCodeName = pRefObj->m_strObjectName;

                    if (strUniqueCodeName.find("cape3") != std::string::npos)
                    {
                        pwResultCode = 0x3006;
                        return FALSE;
                    }

                }
                else if (pPC->PvpCape == 1)
                {
                    std::string strUniqueCodeName;
                    RefObjCommon* pRefObj = REFDATA_MGR.GetRefObj(Mob->GetRefObjID());
                    if (pRefObj != NULL)
                        strUniqueCodeName = pRefObj->m_strObjectName;

                    if (strUniqueCodeName.find("cape1") != std::string::npos)
                    {
                        pwResultCode = 0x3006;
                        return FALSE;
                    }
                }

            }

        }

        if (pObjFirst->IsMonster() && pObjSecond->IsPC())
        {
            //CGObjMob* mob = (CGObjMob*)g_pCGame->GetGameObjById(pObjSecond->GetGameID());
            CGObjPC* pPC = reinterpret_cast<CGObjPC*>(
                    g_pCGame->GetObjByGameID(pObjSecond->GetGameID()));

            CGObjMob* Mob = reinterpret_cast<CGObjMob*>(
                    g_pCGame->GetObjByGameID(pObjFirst->GetGameID()));


            if (pPC != NULL && Mob != NULL)
            {
                if (pPC->PvpCape == 3)
                {
                    std::string strUniqueCodeName;
                    RefObjCommon* pRefObj = REFDATA_MGR.GetRefObj(Mob->GetRefObjID());
                    if (pRefObj != NULL)
                        strUniqueCodeName = pRefObj->m_strObjectName;

                    if (strUniqueCodeName.find("cape3") != std::string::npos)
                    {
                        pwResultCode = 0x3006;
                        return FALSE;
                    }

                }
                else if (pPC->PvpCape == 1)
                {
                    std::string strUniqueCodeName;
                    RefObjCommon* pRefObj = REFDATA_MGR.GetRefObj(Mob->GetRefObjID());
                    if (pRefObj != NULL)
                        strUniqueCodeName = pRefObj->m_strObjectName;

                    if (strUniqueCodeName.find("cape1") != std::string::npos)
                    {
                        pwResultCode = 0x3006;
                        return FALSE;
                    }
                }

            }

        }
        /*
        this will print invaild target so you cant attack it
            pwResultCode = 0x3006;
                        return FALSE;

        */
#endif

    }
    catch (std::exception& e)
    {
        BS_ERROR("%s - attack restriction exception: %s", __FUNCTION__, e.what());
    }
    catch (...)
    {
        BS_ERROR("%s - unknown attack restriction exception", __FUNCTION__);
    }


    return s_pfnIGObj_CanAttack(pObjFirst, pObjSecond, a3, pwResultCode);
}
void CGObjPC::SetGrantName(std::string* grantname)
{
    unsigned int CharID = this->GetDBID();
    __asm
    {
    push grantname;
    push 0x25;
    push this;

    mov ecx, CharID;
    xor edx, edx;

    mov eax, 0x005C80A0;
    call eax;
    }
}

CSkillManager* CGObjPC::GetSkillManager()
{
    return (CSkillManager*)MEMUTIL_ADD_PTR(this, 0xA30);
}

void CGObjPC::CancelBuff(int nRefSkillID)
{
    const DWORD dwCancelBuffCallAddr = 0x0059F0C0;

    __asm pushad;
    __asm pushfd;

    //pSkillMgr -> EDX
    __asm mov edx, this;
    __asm add edx, 0xA30;

    //Call function
    __asm mov ebx, nRefSkillID;
    __asm mov eax, edx;
    __asm call dwCancelBuffCallAddr;

    __asm popfd;
    __asm popad;
}

