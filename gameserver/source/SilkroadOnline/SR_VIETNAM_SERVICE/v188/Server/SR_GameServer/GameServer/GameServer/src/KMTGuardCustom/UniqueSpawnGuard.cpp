#include "UniqueSpawnGuard.h"

#include <Objects/GObjPC.h>
#include <SettingMgr/IniReader.h>
#include "GameServerTelemetry.h"
#include <map>
#include <set>
#include <sstream>
#include <cstdlib>

namespace
{
    CRITICAL_SECTION s_lock;
    bool s_initialized = false;
    std::set<int> s_authorizedCharacterIds;
    std::set<DWORD> s_allowedWorlds;
    std::set<WORD> s_allowedRegions;
    std::map<DWORD, DWORD> s_characterCooldowns;
    std::map<DWORD, DWORD> s_recentTokens;
    DWORD s_cooldownMs = 10000;
    DWORD s_replayWindowMs = 30000;
    unsigned int s_capacity = 5;
    unsigned int s_refillPerMinute = 5;
    double s_tokens = 0.0;
    DWORD s_lastRefillTick = 0;

    struct LockLifetime { LockLifetime() { InitializeCriticalSection(&s_lock); }
        ~LockLifetime() { DeleteCriticalSection(&s_lock); } } s_lockLifetime;
    struct Guard { Guard() { EnterCriticalSection(&s_lock); } ~Guard() { LeaveCriticalSection(&s_lock); } };

    template <typename T> void ParseList(const std::string& text, std::set<T>& output)
    {
        std::istringstream stream(text);
        std::string value;
        while (std::getline(stream, value, ','))
        {
            char* end = NULL;
            const unsigned long parsed = strtoul(value.c_str(), &end, 10);
            if (end != value.c_str() && *end == '\0') output.insert(static_cast<T>(parsed));
        }
    }

    DWORD HashToken(CGObjPC* player, BYTE type, const BYTE* token, size_t length)
    {
        DWORD hash = 2166136261u ^ player->GetGameID() ^ type;
        for (size_t i = 0; i < length; ++i) hash = (hash ^ token[i]) * 16777619u;
        return hash;
    }
}

bool UniqueSpawnGuard::Initialize()
{
    CIniReader reader(".\\KMTGuard-Addon.ini");
    Guard guard;
    s_authorizedCharacterIds.clear(); s_allowedWorlds.clear(); s_allowedRegions.clear();
    ParseList<int>(reader.ReadStringA("UniqueSpawn", "AuthorizedCharacterIds", "", false), s_authorizedCharacterIds);
    ParseList<DWORD>(reader.ReadStringA("UniqueSpawn", "AllowedWorlds", "", false), s_allowedWorlds);
    ParseList<WORD>(reader.ReadStringA("UniqueSpawn", "AllowedRegions", "", false), s_allowedRegions);
    s_cooldownMs = static_cast<DWORD>(reader.ReadInt("UniqueSpawn", "CharacterCooldownMs", 10000));
    s_replayWindowMs = static_cast<DWORD>(reader.ReadInt("UniqueSpawn", "ReplayWindowMs", 30000));
    s_capacity = static_cast<unsigned int>(reader.ReadInt("UniqueSpawn", "GlobalBurst", 5));
    s_refillPerMinute = static_cast<unsigned int>(reader.ReadInt("UniqueSpawn", "GlobalPerMinute", 5));
    if (s_cooldownMs < 1000 || s_replayWindowMs < 1000 || s_capacity == 0 || s_capacity > 100 || s_refillPerMinute == 0 || s_refillPerMinute > 1000)
        return false;
    s_tokens = static_cast<double>(s_capacity); s_lastRefillTick = GetTickCount(); s_initialized = true;
    return true;
}

void UniqueSpawnGuard::Shutdown()
{
    Guard guard; s_initialized = false; s_characterCooldowns.clear(); s_recentTokens.clear();
    s_authorizedCharacterIds.clear(); s_allowedWorlds.clear(); s_allowedRegions.clear();
}

bool UniqueSpawnGuard::AuthorizeAndConsume(CGObjPC* player, BYTE uniqueType,
    const SWorldID& world, const SPosInfo& position, const BYTE* replayToken, size_t replayTokenLength)
{
    if (player == NULL || uniqueType > 4) { GameServerTelemetry::RecordUniqueSpawnRejected(1); return false; }
    const DWORD now = GetTickCount();
    Guard guard;
    if (!s_initialized || s_authorizedCharacterIds.find(player->GetDBID()) == s_authorizedCharacterIds.end())
        { GameServerTelemetry::RecordUniqueSpawnRejected(2); return false; }
    if (s_allowedWorlds.find(world.dwWorldID) == s_allowedWorlds.end() ||
        s_allowedRegions.find(static_cast<WORD>(position.wRegionID)) == s_allowedRegions.end())
        { GameServerTelemetry::RecordUniqueSpawnRejected(3); return false; }
    const DWORD token = HashToken(player, uniqueType, replayToken, replayTokenLength);
    const DWORD effectiveReplayWindow = s_replayWindowMs < s_cooldownMs
        ? s_replayWindowMs : s_cooldownMs;
    std::map<DWORD,DWORD>::iterator replay = s_recentTokens.begin();
    while (replay != s_recentTokens.end()) { if (now - replay->second >= effectiveReplayWindow) s_recentTokens.erase(replay++); else ++replay; }
    if (s_recentTokens.find(token) != s_recentTokens.end()) { GameServerTelemetry::RecordUniqueSpawnRejected(4); return false; }
    const DWORD id = player->GetGameID();
    if (s_characterCooldowns.find(id) != s_characterCooldowns.end() && now - s_characterCooldowns[id] < s_cooldownMs)
        { GameServerTelemetry::RecordUniqueSpawnRejected(5); return false; }
    const DWORD elapsed = now - s_lastRefillTick;
    s_tokens += static_cast<double>(elapsed) * s_refillPerMinute / 60000.0;
    if (s_tokens > s_capacity) s_tokens = static_cast<double>(s_capacity);
    s_lastRefillTick = now;
    if (s_tokens < 1.0) { GameServerTelemetry::RecordUniqueSpawnRejected(6); return false; }
    s_tokens -= 1.0; s_characterCooldowns[id] = now; s_recentTokens[token] = now;
    return true;
}

void UniqueSpawnGuard::ForgetPlayer(CGObjPC* player)
{
    if (player == NULL) return; Guard guard; s_characterCooldowns.erase(player->GetGameID());
}
