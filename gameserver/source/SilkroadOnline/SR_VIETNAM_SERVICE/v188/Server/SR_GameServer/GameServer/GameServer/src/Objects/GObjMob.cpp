//
// Created by YUMBUL on 19.03.2023.
//

#include <cstdio>
#include <Game.h>
#include "GObjMob.h"
#include "GObjPC.h"
#include "RegionManagerBody.h"
#include <SqlConnection/sqlCon.h>
#include <KMTGuardCustom/DamageMeter.h>
#include <KMTGuardCustom/GameServerTelemetry.h>
#include <algorithm>
#include <set>

#define UNIQUE_DPS_MAX_RECORDS 8
#define UNIQUE_DPS_BATCH_INTERVAL_MS 1000
#define UNIQUE_DPS_DEFAULT_MAX_MOBS_PER_TICK 16
#define UNIQUE_DPS_DEFAULT_BUDGET_US 2000

namespace
{
    struct DamageDescending
    {
        bool operator()(const std::pair<DWORD, DWORD>& left,
                        const std::pair<DWORD, DWORD>& right) const
        {
            if (left.second != right.second) return left.second > right.second;
            return left.first < right.first;
        }
    };

    struct UniqueDpsSnapshotRecord
    {
        DWORD playerGameId;
        DWORD damage;
    };

    static std::set<DWORD> s_uniqueDpsPendingMobs;
    static CRITICAL_SECTION s_uniqueDpsStateLock;
    static DWORD s_uniqueDpsLastBatchTick = 0;
    static unsigned int ReadBoundedEnvironment(const char* name, unsigned int fallback, unsigned int maximum)
    {
        char text[16] = { 0 };
        if (GetEnvironmentVariableA(name, text, sizeof(text)) == 0) return fallback;
        const unsigned long value = strtoul(text, NULL, 10);
        return value > 0 && value <= maximum ? static_cast<unsigned int>(value) : fallback;
    }

    struct UniqueDpsStateInitializer
    {
        UniqueDpsStateInitializer()
        {
            InitializeCriticalSection(&s_uniqueDpsStateLock);
        }

        ~UniqueDpsStateInitializer()
        {
            DeleteCriticalSection(&s_uniqueDpsStateLock);
        }
    } s_uniqueDpsStateInitializer;

    class UniqueDpsLock
    {
    public:
        UniqueDpsLock()
        {
            EnterCriticalSection(&s_uniqueDpsStateLock);
        }

        ~UniqueDpsLock()
        {
            LeaveCriticalSection(&s_uniqueDpsStateLock);
        }

    private:
        UniqueDpsLock(const UniqueDpsLock&);
        UniqueDpsLock& operator=(const UniqueDpsLock&);
    };

    static void UpdatePendingDpsTelemetryLocked()
    {
        GameServerTelemetry::SetLiveDpsPendingCacheSize(s_uniqueDpsPendingMobs.size());
    }

    static void QueueLiveUniqueDpsSnapshot(CGObjMob* pMob)
    {
        if (pMob == NULL || pMob->Monsterclass != 3)
            return;

        try
        {
            UniqueDpsLock guard;
            const std::pair<std::set<DWORD>::iterator, bool> inserted =
                s_uniqueDpsPendingMobs.insert(pMob->GetGameID());
            if (inserted.second)
                UpdatePendingDpsTelemetryLocked();
        }
        catch (...)
        {
            GameServerTelemetry::RecordRuntimeError();
        }
    }

    static bool SendLiveUniqueDpsSnapshot(CGObjMob* pMob)
    {
        if (pMob == NULL || pMob->Monsterclass != 3 || g_pCGame == NULL)
            return false;

        if (pMob->MyMap.empty())
            return false;

        std::pair<DWORD, DWORD> records[UNIQUE_DPS_MAX_RECORDS];
        size_t recordCount = 0;
        CGObjPC* pSender = NULL;
        std::map<DWORD, SAggroMapSecondPairItem>::iterator itCur = pMob->MyMap.begin();
        while (itCur != pMob->MyMap.end())
        {
            const SAggroMapSecondPairItem& gidDmgPair = itCur->second;
            IGObj* candidate = g_pCGame->GetObjByGameID(gidDmgPair.dwPlayerGID);
            if (candidate != NULL && candidate->IsPC())
            {
                if (pSender == NULL)
                    pSender = reinterpret_cast<CGObjPC*>(candidate);

                const std::pair<DWORD, DWORD> candidateRecord(gidDmgPair.dwPlayerGID, gidDmgPair.dwDamage);
                size_t insertAt = 0;
                DamageDescending descending;
                while (insertAt < recordCount && !descending(candidateRecord, records[insertAt])) ++insertAt;
                if (insertAt < UNIQUE_DPS_MAX_RECORDS)
                {
                    const size_t newCount = recordCount < UNIQUE_DPS_MAX_RECORDS ? recordCount + 1 : recordCount;
                    for (size_t move = newCount - 1; move > insertAt; --move) records[move] = records[move - 1];
                    records[insertAt] = candidateRecord; recordCount = newCount;
                }
            }

            ++itCur;
        }

        if (recordCount == 0 || pSender == NULL)
            return false;

        CMsg* pMsg = pSender->AllocMsg(0x5010);
        if (pMsg == NULL)
            return false;
        *pMsg << pMob->GetRefObjID();
        *pMsg << static_cast<int>(recordCount);

        for (size_t i = 0; i < recordCount; ++i)
        {
            *pMsg << records[i].first;
            *pMsg << records[i].second;
        }

        pSender->SendMsg(pMsg);
        return true;
    }

    static void ForgetUniqueDpsMob(DWORD mobGameId)
    {
        UniqueDpsLock guard;
        s_uniqueDpsPendingMobs.erase(mobGameId);
        UpdatePendingDpsTelemetryLocked();
    }
}

void CGObjMob::FlushLiveDpsBatch()
{
    try
    {
        const DWORD nowTick = GetTickCount();
        const unsigned int maxMobs = ReadBoundedEnvironment("KMT_DPS_MAX_MOBS_PER_TICK", UNIQUE_DPS_DEFAULT_MAX_MOBS_PER_TICK, 256);
        const unsigned int budgetUs = ReadBoundedEnvironment("KMT_DPS_BUDGET_US", UNIQUE_DPS_DEFAULT_BUDGET_US, 20000);
        DWORD pendingMobIds[256] = { 0 };
        size_t pendingMobCount = 0;

        {
            UniqueDpsLock guard;
            if ((nowTick - s_uniqueDpsLastBatchTick) < UNIQUE_DPS_BATCH_INTERVAL_MS ||
                s_uniqueDpsPendingMobs.empty())
                return;

            s_uniqueDpsLastBatchTick = nowTick;
            std::set<DWORD>::iterator it = s_uniqueDpsPendingMobs.begin();
            while (it != s_uniqueDpsPendingMobs.end() &&
                   pendingMobCount < maxMobs)
            {
                pendingMobIds[pendingMobCount++] = *it;
                s_uniqueDpsPendingMobs.erase(it++);
            }
            UpdatePendingDpsTelemetryLocked();
        }

        unsigned int sentPackets = 0;
        if (g_pCGame == NULL)
        {
            UniqueDpsLock guard;
            for (size_t i = 0; i < pendingMobCount; ++i) s_uniqueDpsPendingMobs.insert(pendingMobIds[i]);
            UpdatePendingDpsTelemetryLocked();
        }
        else
        {
            LARGE_INTEGER frequency = { 0 }, started = { 0 }, current = { 0 };
            QueryPerformanceFrequency(&frequency); QueryPerformanceCounter(&started);
            size_t i = 0;
            for (; i < pendingMobCount; ++i)
            {
                IGObj* object = g_pCGame->GetObjByGameID(pendingMobIds[i]);
                if (object != NULL && object->IsMonster() &&
                    SendLiveUniqueDpsSnapshot(reinterpret_cast<CGObjMob*>(object)))
                {
                    ++sentPackets;
                }
                QueryPerformanceCounter(&current);
                if (frequency.QuadPart > 0 &&
                    ((current.QuadPart - started.QuadPart) * 1000000LL / frequency.QuadPart) >= budgetUs)
                { ++i; break; }
            }
            if (i < pendingMobCount)
            {
                UniqueDpsLock guard;
                for (; i < pendingMobCount; ++i) s_uniqueDpsPendingMobs.insert(pendingMobIds[i]);
                UpdatePendingDpsTelemetryLocked();
            }
        }

        GameServerTelemetry::RecordLiveDpsBatch(sentPackets);
    }
    catch (...)
    {
        GameServerTelemetry::RecordRuntimeError();
    }
}
void CGObjMob::LivedpsMobAttackRecorder(void* attacker, int p2, int p3, int p4, int p5) {
    reinterpret_cast<void(__thiscall*)(CGObjMob*, void*, int, int, int, int)>(0x0052a8e0)(this, attacker, p2, p3, p4, p5);
    QueueLiveUniqueDpsSnapshot(this);
}

CGObjPC *CGObjMob::MobDamageMeter(unsigned int a2) {
    try {
        //std::cout << __FUNCTION__ << " -> ID " << pObj->GetRefObjID() << " CGObjNPC" << std::endl;
        CGObjPC *aa = reinterpret_cast<CGObjPC *(__thiscall *) (CGObjMob *, unsigned int)>(0x004c44a0)(this, a2);
        /*       bool isUniqueMonster = CRegionRestrictionMgr::IsFortressHeartStruct(this->GetRefObjID());

        if (!isUniqueMonster)
            return aa;*/
        if (this->Monsterclass != 3)
            return aa;
        std::map<DWORD, SAggroMapSecondPairItem>::iterator itCur = MyMap.begin();
        std::map<DWORD, SAggroMapSecondPairItem>::iterator itEnd = MyMap.end();

        if (itCur == itEnd)
            return aa;

        if (g_pCGame == NULL)
            return aa;

        CGObjPC *pFirstObj = NULL;
        int sayi = 0;
        while (itCur != itEnd && pFirstObj == NULL) {
            const SAggroMapSecondPairItem& gid_dmg_pair = itCur->second;
            IGObj* candidate = g_pCGame->GetObjByGameID(gid_dmg_pair.dwPlayerGID);
            if (candidate != NULL && candidate->IsPC())
                pFirstObj = reinterpret_cast<CGObjPC *>(candidate);
            else
            {
                ++itCur;
                ++sayi;
            }
        }

        if (pFirstObj == NULL)
            return aa;


        CMsg *pMsg = pFirstObj->AllocMsg(0x5010);
        if (pMsg == NULL)
            return aa;

        *pMsg << this->GetRefObjID();
        *pMsg << MyMap.size() - sayi;
        while (itCur != itEnd) {
            std::pair<DWORD, SAggroMapSecondPairItem> aggro_info = *itCur;

            SAggroMapSecondPairItem gid_dmg_pair = aggro_info.second;
            *pMsg << gid_dmg_pair.dwPlayerGID;
            *pMsg << gid_dmg_pair.dwDamage;


            ++itCur;
        }
        pFirstObj->SendMsg(pMsg);
        pMsg->m_wWriteDataArrayPos = 0;


        return aa;
    } catch (...) {
        CGObjPC *aa = reinterpret_cast<CGObjPC *(__thiscall *) (CGObjMob *, unsigned int)>(0x004c44a0)(this, a2);
        return aa;
    }
}

void CGObjMob::Mobkillquest(CGObjPC* param_1, int param_2)
{
    reinterpret_cast<void(__thiscall*)(CGObjMob*, CGObjPC*, int)>(0x004a7540)(this, param_1, param_2);
}

unsigned int CGObjMob::HandleMobKilled(IGObj *pKiller)
{
    const DWORD killedMobGameId = this->GetGameID();
    const int killedMobRefObjId = this->GetRefObjID();
    const int killedMonsterClass = this->Monsterclass;
    SPosInfo killedPosition;
    this->GetPosInfo(killedPosition);
    SWorldID killedWorld;
    this->GetWorldID(killedWorld);

    std::vector<std::pair<DWORD, DWORD> > damageSnapshot;
    if (killedMonsterClass == 3)
    {
        damageSnapshot.reserve(MyMap.size());
        std::map<DWORD, SAggroMapSecondPairItem>::const_iterator it = MyMap.begin();
        for (; it != MyMap.end(); ++it)
        {
            damageSnapshot.push_back(std::make_pair(
                    it->second.dwPlayerGID,
                    it->second.dwDamage));
        }
        std::sort(damageSnapshot.begin(), damageSnapshot.end(), DamageDescending());
        if (damageSnapshot.size() > UNIQUE_DPS_MAX_RECORDS)
            damageSnapshot.resize(UNIQUE_DPS_MAX_RECORDS);
    }

    IGObj* resolvedKiller = pKiller;
    if (resolvedKiller != NULL && resolvedKiller->IsCOS())
        resolvedKiller = resolvedKiller->GetOwner();
    DWORD killerGameId = 0;
    if (resolvedKiller != NULL && resolvedKiller->IsPC())
        killerGameId = resolvedKiller->GetGameID();

    SendLiveUniqueDpsSnapshot(this);
    ForgetUniqueDpsMob(killedMobGameId);
    CDamageMeter::ForgetMob(killedMobGameId);
    unsigned int aa = reinterpret_cast<unsigned int(__thiscall*)(CGObjMob*, IGObj*)>(0x004c1c80)(this, pKiller);

    if (killerGameId != 0)
    {
        IGObj* killerObject = g_pCGame->GetObjByGameID(killerGameId);
        if(killerObject != NULL && killerObject->IsPC())
        {
            CGObjPC * pPC = reinterpret_cast<CGObjPC *>(killerObject);
            CMsg *pMsg = pPC->AllocMsg(0x5014);
            if (pMsg == NULL)
                return aa;
            *pMsg << killedMonsterClass;
            *pMsg << killedMobRefObjId;
            *pMsg << (unsigned short) killedPosition.wRegionID;
            *pMsg << killedPosition.fltX;
            *pMsg << killedPosition.fltY;
            *pMsg << killedPosition.fltZ;
            *pMsg << killedWorld.dwWorldID;

            if(killedMonsterClass == 3)
            {
                *pMsg << static_cast<unsigned int>(damageSnapshot.size());
                for (size_t i = 0; i < damageSnapshot.size(); ++i)
                {
                    *pMsg << damageSnapshot[i].first;
                    *pMsg << damageSnapshot[i].second;
                }
            }
            pPC->SendMsg(pMsg);
        }

    }

    return aa;
}

struct CreatedMobList
{
    uint32_t WorldID;
    int MobID;
    int UniqueID;
};
std::vector<CreatedMobList>MobList;

namespace
{
    const size_t MOB_TRACKER_PRUNE_INTERVAL = 64;
    size_t s_mobTrackerSpawnCount = 0;

    void PruneTrackedMobs()
    {
        std::vector<CreatedMobList>::iterator it = MobList.begin();
        while (it != MobList.end())
        {
            if (g_pCGame->GetObjByGameID(it->UniqueID) == NULL)
                it = MobList.erase(it);
            else
                ++it;
        }
    }
}

CGObjMob* CGObjMob::MonsterAdd(unsigned int p1, unsigned int p2, unsigned int p3, unsigned int p4, unsigned int p5)
{
    //printf("%d ! %d ! %d ! %d ! %d \n", p1, p2, p3, p4, p5);
    return reinterpret_cast<CGObjMob*(__thiscall*)(CGObjMob*, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int)>(0x004c10c0)(this, p1, p2, p3, p4, p5);
}
void CGObjMob::MonsterRemove(CGObjMob* p1)
{
    //printf("%d ! \n", p1);
    reinterpret_cast<void*(__thiscall*)(CGObjMob*, CGObjMob*)>(0x004c10c0)(this, p1);
}
CGObjMob* CGObjMob::CreateMob(uint32_t RefObjId, uint32_t GameWorldId, uint16_t RegionId, float PosX, float PosY, float PosZ, float GenerateRadius)
{
    // Create position
    NavInfo posNavInfo(PosInfo(RegionId, PosX, PosY, PosZ));

    // Check if can be solved
    if (!CRegionManagerBody::ResolveCellAndHeight(&posNavInfo))
    {
        //printf(" * [CreateMob] Couldn't resolve cell & height. | [%d - %f, %f, %f]\n", RegionId, PosX, PosY, PosZ);
        return NULL;
    }

    // Check if the data from object can be found
    void * const objBaseDataPtr = GetRefObjBaseData(RefObjId);
    if (!objBaseDataPtr)
    {
        //printf(" * [CreateMob] Couldn't find the object data. | [%d]", RefObjId);
        return NULL;
    }

    // Retrieve base data
    int objCreepType = GetCreepType(objBaseDataPtr);
    void* objRefTacticPtr = GetRefTactic(objBaseDataPtr);
    float objDir = rand() / 32767.f * 6.283185482025146f;

    CGObjMob* _eax;

    __asm
    {
    mov edx, 0x400000
    lea esi, [edx + 0x1F6EB0]	// base_addr + CreateObj(RVA)
    lea edi, [edx + 0x90B380]	// base_addr + CGameWorldMgr(RVA of Instance)
    lea eax, [posNavInfo]		// nav_info

    push 1										// unk
    push 0										// unk
    push 0										// unk
    push 0										// unk
    push objCreepType							// obj_creep_type
    push GenerateRadius							// generate_radius
    push objDir									// obj_direction
    push dword ptr ss : [objRefTacticPtr]		// obj_ref_tactic_ptr
    push 0										// obj_ref_nest_ptr
    push dword ptr ss : [objBaseDataPtr]		// obj_ref_base_data_ptr
    push 0										// unk
    push GameWorldId								// unk
    push edi									// CGameWorldMgr instance
    call esi
    mov _eax, eax
    }
    if (_eax != NULL)
    {
        ++s_mobTrackerSpawnCount;
        if ((s_mobTrackerSpawnCount % MOB_TRACKER_PRUNE_INTERVAL) == 0)
            PruneTrackedMobs();

        CreatedMobList test = CreatedMobList();
        test.MobID = RefObjId;
        test.WorldID = GameWorldId;
        test.UniqueID = _eax->GetGameID();
        MobList.push_back(test);
        //pPC->DealDamage(999999999, 888888888);
       // CSqlCommands::RecordMobSpawn(pPC->m_dwGameId, pPC->GetCodeName());
    }
    return _eax;
}

const DWORD FN_DESPAWN_OBJ_FN_OFFSET = 0x00485E50;



void CGObjMob::DeleteMonster(int RefObjId)
{
    std::vector<int> gameIds;
    std::vector<CreatedMobList>::iterator it = MobList.begin();
    while (it != MobList.end())
    {
        if (it->MobID == RefObjId)
        {
            gameIds.push_back(it->UniqueID);
            it = MobList.erase(it);
            continue;
        }
        ++it;
    }

    for (size_t index = 0; index < gameIds.size(); ++index)
    {
        IGObj* mob = g_pCGame->GetObjByGameID(gameIds[index]);
        if (mob != NULL)
            DespawnGObj(mob);
    }
}
void CGObjMob::DeleteMonsterByWorldID(int WorldID)
{
    const uint32_t targetWorldId = static_cast<uint32_t>(WorldID + 0x10000);
    std::vector<int> gameIds;
    std::vector<CreatedMobList>::iterator it = MobList.begin();
    while (it != MobList.end())
    {
        if (it->WorldID == targetWorldId)
        {
            gameIds.push_back(it->UniqueID);
            it = MobList.erase(it);
            continue;
        }
        ++it;
    }


    for (size_t index = 0; index < gameIds.size(); ++index)
    {
        IGObj* mob = g_pCGame->GetObjByGameID(gameIds[index]);
        if (mob != NULL)
            DespawnGObj(mob);
    }
}
void CGObjMob::DespawnGObj(IGObj* pObj)
{
    __asm pushad;
    __asm pushfd;
    __asm mov esi, pObj;
    __asm call FN_DESPAWN_OBJ_FN_OFFSET;
    __asm popfd;
    __asm popad;
}
CGObjMob *CGObjMob::SpawnMob(uint32_t RefObjId, uint32_t GameWorldId, uint16_t RegionId, float PosX, float PosY, float PosZ, float GenerateRadius) {
    // Create position
    NavInfo posNavInfo(PosInfo(RegionId, PosX, PosY, PosZ));

    // Check if can be solved
    if (!CRegionManagerBody::ResolveCellAndHeight(&posNavInfo)) {
        //printf(" * [CreateMob] Couldn't resolve cell & height. | [%d - %f, %f, %f]\n", RegionId, PosX, PosY, PosZ);
        return NULL;
    }

    // Check if the data from object can be found
    void *const objBaseDataPtr = GetRefObjBaseData(RefObjId);
    if (!objBaseDataPtr) {
        //printf(" * [CreateMob] Couldn't find the object data. | [%d]", RefObjId);
        return NULL;
    }

    // Retrieve base data
    int objCreepType = GetCreepType(objBaseDataPtr);
    void *objRefTacticPtr = GetRefTactic(objBaseDataPtr);
    float objDir = rand() / 32767.f * 6.283185482025146f;

    CGObjMob *_eax;

    __asm {
    mov edx, 0x400000
    lea esi, [edx + 0x1F6EB0]// base_addr + CreateObj(RVA)
    lea edi, [edx + 0x90B380]// base_addr + CGameWorldMgr(RVA of Instance)
    lea eax, [posNavInfo]// nav_info

    push 1// unk
    push 0// unk
    push 0// unk
    push 0// unk
    push objCreepType// obj_creep_type
    push GenerateRadius// generate_radius
    push objDir// obj_direction
    push dword ptr ss : [objRefTacticPtr]// obj_ref_tactic_ptr
    push 0// obj_ref_nest_ptr
    push dword ptr ss : [objBaseDataPtr]// obj_ref_base_data_ptr
    push 0// unk
    push GameWorldId// unk
    push edi// CGameWorldMgr instance
    call esi
    mov _eax, eax
    }
    return _eax;
}

void* CGObjMob::GetRefObjBaseData(int RefObjId)
{
    void* _eax;

    __asm
    {
    mov edx, 0x400000
    lea edx, [edx + 0x260B0]

    mov edi, dword ptr ds : [0xD6AA14]
    push RefObjId
    call edx
    mov _eax, eax
    }
    return _eax;
}


__int32 CGObjMob::GetCreepType(void* RefObjBaseDataPtr)
{
    return *reinterpret_cast<uint8_t*>(reinterpret_cast<unsigned int>(RefObjBaseDataPtr) + 0x89);
}

void* CGObjMob::GetRefTactic(void* RefObjBaseDataPtr)
{
    void* _eax;

    __asm
    {
    mov edx, 0x400000
    lea edx, [edx + 0x120D10]

    mov eax, dword ptr ss : [RefObjBaseDataPtr]
    call edx
    mov _eax, eax
    }
    return _eax;
}

