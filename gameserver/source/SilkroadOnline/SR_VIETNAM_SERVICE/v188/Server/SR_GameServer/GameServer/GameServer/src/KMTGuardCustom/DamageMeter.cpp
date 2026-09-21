#include "DamageMeter.h"
#include "Game.h"
#include "GObjPC.h"
#include <GSLog/Logger.h>
#include <KMTGuardCustom/GameServerRuntimeSafety.h>
#include "../../../CustomNetwork/MsgStreamBuffer.h"

#define CGOBJNPC_AGGRO_MAP_FN_OFFSET		            0x004C44A0

CDamageMeter::FN_CGOBJNPC_AGGRO_MAP CDamageMeter::s_pfnCGObjNpcAggroMap;

namespace
{
    const DWORD RESTRICTION_AGGRO_PRUNE_INTERVAL_MS = 1000;
    const DWORD RESTRICTION_AGGRO_CACHE_SWEEP_INTERVAL_MS = 60000;
    const DWORD RESTRICTION_AGGRO_CACHE_ENTRY_TTL_MS = 600000;
    std::map<DWORD, DWORD> s_lastRestrictionAggroPruneTick;
    CRITICAL_SECTION s_restrictionAggroPruneLock;
    DWORD s_lastRestrictionAggroCacheSweepTick = 0;
    bool s_aggroDetourInstalled = false;

    struct RestrictionAggroPruneState
    {
        RestrictionAggroPruneState()
        {
            InitializeCriticalSection(&s_restrictionAggroPruneLock);
        }

        ~RestrictionAggroPruneState()
        {
            DeleteCriticalSection(&s_restrictionAggroPruneLock);
        }
    } s_restrictionAggroPruneState;

    class ScopedRestrictionAggroLock
    {
    public:
        ScopedRestrictionAggroLock()
        {
            EnterCriticalSection(&s_restrictionAggroPruneLock);
        }

        ~ScopedRestrictionAggroLock()
        {
            LeaveCriticalSection(&s_restrictionAggroPruneLock);
        }

    private:
        ScopedRestrictionAggroLock(const ScopedRestrictionAggroLock&);
        ScopedRestrictionAggroLock& operator=(const ScopedRestrictionAggroLock&);
    };

    bool ShouldPruneRestrictionAggro(DWORD mobGameId)
    {
        const DWORD nowTick = GetTickCount();
        bool shouldPrune = false;

        ScopedRestrictionAggroLock lock;
        if ((nowTick - s_lastRestrictionAggroCacheSweepTick) >=
            RESTRICTION_AGGRO_CACHE_SWEEP_INTERVAL_MS)
        {
            std::map<DWORD, DWORD>::iterator staleIt =
                s_lastRestrictionAggroPruneTick.begin();
            while (staleIt != s_lastRestrictionAggroPruneTick.end())
            {
                if ((nowTick - staleIt->second) >=
                    RESTRICTION_AGGRO_CACHE_ENTRY_TTL_MS)
                    s_lastRestrictionAggroPruneTick.erase(staleIt++);
                else
                    ++staleIt;
            }
            s_lastRestrictionAggroCacheSweepTick = nowTick;
        }

        std::map<DWORD, DWORD>::iterator it =
            s_lastRestrictionAggroPruneTick.find(mobGameId);
        if (it == s_lastRestrictionAggroPruneTick.end() ||
            (nowTick - it->second) >= RESTRICTION_AGGRO_PRUNE_INTERVAL_MS)
        {
            s_lastRestrictionAggroPruneTick[mobGameId] = nowTick;
            shouldPrune = true;
        }
        return shouldPrune;
    }
}

bool CDamageMeter::Initialize()
{
    s_pfnCGObjNpcAggroMap = reinterpret_cast<FN_CGOBJNPC_AGGRO_MAP>(
        CGOBJNPC_AGGRO_MAP_FN_OFFSET
        );

	s_aggroDetourInstalled = GameServerRuntimeSafety::AttachDetour(
        reinterpret_cast<PVOID*>(&s_pfnCGObjNpcAggroMap),
        reinterpret_cast<PVOID>(CDamageMeter::MyCGObjNPC_HandleAggroMap),
        "aggro restriction detour");
    return s_aggroDetourInstalled;
}

void CDamageMeter::Shutdown()
{
    if (s_aggroDetourInstalled)
    {
        GameServerRuntimeSafety::DetachDetour(
            reinterpret_cast<PVOID*>(&s_pfnCGObjNpcAggroMap),
            reinterpret_cast<PVOID>(CDamageMeter::MyCGObjNPC_HandleAggroMap),
            "aggro restriction detour");
        s_aggroDetourInstalled = false;
    }
}

void CDamageMeter::ForgetMob(DWORD mobGameId)
{
    ScopedRestrictionAggroLock lock;
    s_lastRestrictionAggroPruneTick.erase(mobGameId);
}

void* __fastcall CDamageMeter::MyCGObjNPC_HandleAggroMap(IGObj* pObj, void* /* dummy edx */, int a2)
{
    //printf("%s entered, codename = %s", __FUNCTION__, pObj->GetCodeName());

    void* res = s_pfnCGObjNpcAggroMap(pObj, a2);

    try
    {
        int isUniqueMonster = MEMUTIL_READ_BY_PTR_OFFSET(pObj, 0x1CD8, int);
        if (isUniqueMonster != 3)
            return res;

        // CanAttack rejects a restricted hit immediately. Aggro cleanup is
        // only needed periodically to stop an already tracked player from
        // being selected/chased; scanning every attacker after every hit made
        // this path O(attacker count) per attack.
        if (!ShouldPruneRestrictionAggro(pObj->GetGameID()))
            return res;

        __CGOBJ_AGGRO_LIST_MAP* aggro_map = &MEMUTIL_READ_BY_PTR_OFFSET(pObj, 0x1CA0, __CGOBJ_AGGRO_LIST_MAP);

        CGObjMob* pMob = reinterpret_cast<CGObjMob*>(pObj);

        // Remove players that are not allowed to attack this unique from its
        // aggro list as well. This stops target selection and chasing instead
        // of merely cancelling the final damage.
        __CGOBJ_AGGRO_LIST_MAP_IT pruneIt = aggro_map->begin();
        while (pruneIt != aggro_map->end())
        {
            const DWORD playerGID = pruneIt->second.dwPlayerGID;
            IGObj* pCandidate = g_pCGame->GetObjByGameID(playerGID);
            if (pCandidate != NULL && pCandidate->IsPC())
            {
                CGObjPC* pPC = reinterpret_cast<CGObjPC*>(pCandidate);
                if (!CRegionAttackRestrictionsMgr::CanMobInteractWithPlayer(pMob, pPC))
                {
                    __CGOBJ_AGGRO_LIST_MAP_IT eraseIt = pruneIt++;
                    aggro_map->erase(eraseIt);
                    continue;
                }
            }
            ++pruneIt;
        }

        // Live DPS packets are owned by CGObjMob's throttled recorder. Sending
        // the same snapshot here emitted an additional unthrottled 0x5010 on
        // the hot aggro path. Keep this hook focused on restriction pruning.
    }
    catch (...)
    {
        LOG_WRITE(LOG_ERROR, "%s - exception occoured", __FUNCTION__);
    }
    return res;
}
