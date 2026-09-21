//
// Created by YUMBUL on 19.03.2023.
//

#include "GObjSiegeStruct.h"
#include "Game.h"
#include <SqlConnection/sqlCon.h>
#include <KMTGuardCustom/GameServerRuntimeSafety.h>
#include <KMTGuardCustom/GameServerTelemetry.h>
#include <BSObj/BSObj.h>
#include <algorithm>
#include <map>
#include <string>
#include <vector>

namespace
{
    const DWORD SIEGE_STRUCT_DAMAGE_ADDRESS = 0x004cf860;
    const size_t FORTRESS_DPS_MAX_GUILDS = 8;
    const size_t FORTRESS_DPS_MAX_GUILD_NAME = 64;
    const DWORD FORTRESS_DPS_INTERVAL_MS = 1000;

    CRITICAL_SECTION s_fortressDpsLock;
    std::map<DWORD, DWORD> s_lastFortressSnapshotTicks;

    struct FortressStateInitializer
    {
        FortressStateInitializer() { InitializeCriticalSection(&s_fortressDpsLock); }
        ~FortressStateInitializer() { DeleteCriticalSection(&s_fortressDpsLock); }
    } s_fortressStateInitializer;

    class FortressLock
    {
    public:
        FortressLock() { EnterCriticalSection(&s_fortressDpsLock); }
        ~FortressLock() { LeaveCriticalSection(&s_fortressDpsLock); }
    private:
        FortressLock(const FortressLock&);
        FortressLock& operator=(const FortressLock&);
    };

    struct GuildDamage
    {
        int guildId;
        std::string name;
        unsigned __int64 damage;
    };

    struct GuildDamageDescending
    {
        bool operator()(const GuildDamage& left, const GuildDamage& right) const
        {
            if (left.damage != right.damage) return left.damage > right.damage;
            if (left.name != right.name) return left.name < right.name;
            return left.guildId < right.guildId;
        }
    };

    bool TryBeginFortressSnapshot(DWORD objectGameId)
    {
        const DWORD now = GetTickCount();
        FortressLock guard;
        const std::map<DWORD, DWORD>::iterator existing = s_lastFortressSnapshotTicks.find(objectGameId);
        if (existing != s_lastFortressSnapshotTicks.end() &&
            now - existing->second < FORTRESS_DPS_INTERVAL_MS)
            return false;
        s_lastFortressSnapshotTicks[objectGameId] = now;
        return true;
    }

    void SendFortressDpsSnapshot(CGObjSiegeStruct* object)
    {
        if (object == NULL || g_pCGame == NULL || object->MyMap.empty() ||
            !CSqlCon::IsFortressDpsEnabled(object->GetRefObjID()) ||
            !TryBeginFortressSnapshot(object->GetGameID()))
            return;

        std::map<int, GuildDamage> grouped;
        CGObjPC* sender = NULL;
        std::map<DWORD, SAggroMapSecondPairItem>::const_iterator it = object->MyMap.begin();
        for (; it != object->MyMap.end(); ++it)
        {
            const SAggroMapSecondPairItem& aggro = it->second;
            IGObj* candidate = g_pCGame->GetObjByGameID(aggro.dwPlayerGID);
            if (candidate == NULL || !candidate->IsPC())
                continue;

            CGObjPC* player = reinterpret_cast<CGObjPC*>(candidate);
            if (sender == NULL)
                sender = player;
            if (player->MyGuild == NULL || player->MyGuild->IntanceGuild == NULL)
                continue;

            CInstanceGuild* guild = player->MyGuild->IntanceGuild;
            const size_t nameLength = strnlen(guild->MyGuildName, FORTRESS_DPS_MAX_GUILD_NAME + 1);
            if (guild->GuildID <= 0 || nameLength == 0 || nameLength > FORTRESS_DPS_MAX_GUILD_NAME)
                continue;

            GuildDamage& entry = grouped[guild->GuildID];
            entry.guildId = guild->GuildID;
            if (entry.name.empty())
                entry.name.assign(guild->MyGuildName, nameLength);
            entry.damage += static_cast<unsigned __int64>(aggro.dwDamage);
        }

        if (sender == NULL || grouped.empty())
            return;

        std::vector<GuildDamage> rankings;
        rankings.reserve(grouped.size());
        std::map<int, GuildDamage>::const_iterator group = grouped.begin();
        for (; group != grouped.end(); ++group)
            rankings.push_back(group->second);
        std::sort(rankings.begin(), rankings.end(), GuildDamageDescending());
        if (rankings.size() > FORTRESS_DPS_MAX_GUILDS)
            rankings.resize(FORTRESS_DPS_MAX_GUILDS);

        SPosInfo structurePosition;
        object->GetPosInfo(structurePosition);
        CMsg* message = sender->AllocMsg(0x5016);
        if (message == NULL)
            return;

        *message << static_cast<BYTE>(1);
        *message << object->GetRefObjID();
        *message << static_cast<WORD>(structurePosition.wRegionID);
        *message << static_cast<BYTE>(rankings.size());
        for (size_t i = 0; i < rankings.size(); ++i)
        {
            *message << rankings[i].name;
            *message << rankings[i].damage;
        }
        sender->SendMsg(message);
        GameServerTelemetry::RecordFortressDpsSnapshot();
    }
}

CGObjSiegeStruct::FN_SIEGE_STRUCT_DAMAGE CGObjSiegeStruct::s_originalDamage = NULL;
bool CGObjSiegeStruct::s_detourInstalled = false;

bool CGObjSiegeStruct::Initialize()
{
    if (s_detourInstalled)
        return true;
    s_originalDamage = reinterpret_cast<FN_SIEGE_STRUCT_DAMAGE>(SIEGE_STRUCT_DAMAGE_ADDRESS);
    s_detourInstalled = GameServerRuntimeSafety::AttachDetour(
        reinterpret_cast<PVOID*>(&s_originalDamage),
        reinterpret_cast<PVOID>(CGObjSiegeStruct::SiegeStructDamageMeter),
        "fortress DPS detour");
    return s_detourInstalled;
}

void CGObjSiegeStruct::Shutdown()
{
    if (s_detourInstalled)
    {
        GameServerRuntimeSafety::DetachDetour(
            reinterpret_cast<PVOID*>(&s_originalDamage),
            reinterpret_cast<PVOID>(CGObjSiegeStruct::SiegeStructDamageMeter),
            "fortress DPS detour");
        s_detourInstalled = false;
    }
    FortressLock guard;
    s_lastFortressSnapshotTicks.clear();
}

unsigned int __fastcall CGObjSiegeStruct::SiegeStructDamageMeter(
    CGObjSiegeStruct* object, void*, unsigned short a1, int a2)
{
    if (s_originalDamage == NULL)
        return 0;

    const unsigned int result = s_originalDamage(object, a1, a2);
    try
    {
        SendFortressDpsSnapshot(object);
    }
    catch (...)
    {
        GameServerTelemetry::RecordRuntimeError();
        BS_ERROR("Fortress DPS snapshot failed");
    }
    return result;
}
