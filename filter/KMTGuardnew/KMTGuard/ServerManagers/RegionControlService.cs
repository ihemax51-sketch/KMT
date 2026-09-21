using System.Collections.Concurrent;
using Dapper;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.PacketHandlerManager;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.ServerManagers;

public enum RegionTravelMethod
{
    Arrival,
    Teleport,
    Reverse,
    Trace,
    FilterTeleport
}

public sealed record RegionAdmissionProfile(
    int Strength,
    int Intellect,
    byte Level,
    byte JobType,
    bool IsChinese,
    bool IsEuropean,
    bool IsInParty);

public sealed record RegionAdmissionDecision(bool Allowed, string? LanguageKey = null, object[]? Arguments = null)
{
    public static readonly RegionAdmissionDecision Allow = new(true);
}

public sealed record RegionDestination(int WorldID, int RegionID);

public static class RegionControlService
{
    private const byte EventSuitFreeForAll = 0;
    private const byte EventSuitTeams = 1;
    private static readonly ConcurrentDictionary<RuleKey, CacheEntry> Cache = new();
    private static readonly ConcurrentDictionary<RuleKey, SemaphoreSlim> CacheLocks = new();
    private static readonly SemaphoreSlim SchemaLock = new(1, 1);
    private static readonly TimeSpan CacheTtl = TimeSpan.FromSeconds(2);
    private static readonly TimeSpan CacheRetention = TimeSpan.FromMinutes(10);
    private static readonly TimeSpan CacheCleanupInterval = TimeSpan.FromMinutes(1);
    private static readonly TimeSpan AutoPvpCheckInterval = TimeSpan.FromSeconds(2);
    private static readonly TimeSpan EventTeamCacheTtl = TimeSpan.FromSeconds(30);
    private static readonly TimeSpan EventTeamMissCacheTtl = TimeSpan.FromSeconds(2);
    private static readonly TimeSpan NoticeCooldown = TimeSpan.FromSeconds(2);
    private static readonly TimeSpan NoticeRetention = TimeSpan.FromMinutes(1);
    private static readonly TimeSpan NoticeCleanupInterval = TimeSpan.FromSeconds(30);
    private static readonly TimeSpan ReturnCooldown = TimeSpan.FromSeconds(10);
    private static readonly ConcurrentDictionary<string, long> NoticeTicks = new();
    private static long _lastCacheCleanupTicks;
    private static long _lastNoticeCleanupTicks;
    private static bool _schemaReady;

    private readonly record struct RuleKey(int WorldID, int RegionID);
    private sealed record CacheEntry(_FilterRegionControl? Rule, DateTime FetchedAt, bool LookupSucceeded);

    public static async Task<_FilterRegionControl?> GetRuleAsync(ISession session)
    {
        return await GetRuleAsync(session.SessionData.WorldID, session.SessionData.LatestRegion);
    }

    public static async Task<_FilterRegionControl?> GetRuleAsync(int regionId)
    {
        return await GetRuleAsync(0, regionId);
    }

    public static async Task<_FilterRegionControl?> GetRuleAsync(int worldId, int regionId)
    {
        regionId = NormalizeRegionId(regionId);
        if (!IsValidRegionId(regionId))
            return null;

        RemoveExpiredCacheEntries(DateTime.UtcNow);
        var key = new RuleKey(Math.Max(0, worldId), regionId);
        if (Cache.TryGetValue(key, out var cached) &&
            DateTime.UtcNow - cached.FetchedAt < CacheTtl)
        {
            return cached.Rule;
        }

        var cacheLock = CacheLocks.GetOrAdd(key, static _ => new SemaphoreSlim(1, 1));
        await cacheLock.WaitAsync();
        try
        {
            if (Cache.TryGetValue(key, out cached) &&
                DateTime.UtcNow - cached.FetchedAt < CacheTtl)
            {
                return cached.Rule;
            }

            await EnsureSchemaAsync();
            await using var connection = new SqlConnection(Program.Connectionstring);
            const string sql = @"
SELECT TOP (1) *
FROM [dbo].[Security_RegionFeatures] WITH (NOLOCK)
WHERE RegionID = @RegionID
  AND Enabled = 1
  AND (WorldID = @WorldID OR WorldID = 0)
ORDER BY CASE WHEN WorldID = @WorldID THEN 0 ELSE 1 END, ID DESC;";

            var rule = await connection.QueryFirstOrDefaultAsync<_FilterRegionControl>(
                sql,
                new { WorldID = key.WorldID, RegionID = key.RegionID });
            Cache[key] = new CacheEntry(rule, DateTime.UtcNow, true);
            return rule;
        }
        catch (Exception ex)
        {
            Log.Warning(
                "[dbo].[Security_RegionFeatures] read failed for WorldID={WorldID}, RegionID={RegionID}: {Message}",
                key.WorldID,
                key.RegionID,
                ex.Message);
            Cache[key] = new CacheEntry(null, DateTime.UtcNow, false);
            return null;
        }
        finally
        {
            cacheLock.Release();
        }
    }

    public static async Task EnsureEventProfileAsync(
        string eventCode,
        int worldId,
        int regionId,
        bool teams,
        bool allowParty)
    {
        if (string.IsNullOrWhiteSpace(eventCode))
            throw new ArgumentException("Event code is required.", nameof(eventCode));
        regionId = NormalizeRegionId(regionId);
        if (worldId <= 0 || !IsValidRegionId(regionId))
            throw new ArgumentOutOfRangeException(nameof(regionId), "Event WorldID must be positive and RegionID must be a non-zero signed 16-bit value.");

        await EnsureSchemaAsync();
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.ExecuteAsync(@"
MERGE [dbo].[Security_RegionFeatures] WITH (HOLDLOCK) AS target
USING (SELECT @WorldID AS WorldID, @RegionID AS RegionID) AS source
ON target.WorldID = source.WorldID AND target.RegionID = source.RegionID
WHEN MATCHED THEN
    UPDATE SET
        BuildMode = 0,
        JobMode = 1,
        RaceMode = 0,
        MinLevel = 0,
        MaxLevel = 0,
        PartyMode = 0,
        AllowTeleport = 1,
        AllowReverse = 0,
        AllowTrace = 0,
        AllowMovement = 1,
        AllowChat = 1,
        AllowGlobalChat = 1,
        AllowParty = @AllowParty,
        AllowExchange = 0,
        AllowStall = 0,
        AllowPvP = 1,
        AllowAlchemy = 0,
        AllowSpecialItems = 0,
        AllowBerserk = 1,
        AutoPvpCape = 0,
        InactivityReturnSeconds = 0,
        EventSuitMode = @EventSuitMode,
        Enabled = 1,
        ManagedEventCode = @EventCode,
        ManagedAtUtc = SYSUTCDATETIME(),
        UpdatedAt = SYSUTCDATETIME()
WHEN NOT MATCHED THEN
    INSERT
    (
        WorldID, RegionID, RuleName, Enabled, BuildMode, JobMode, RaceMode, PartyMode,
        MinLevel, MaxLevel, AllowTeleport, AllowReverse, AllowTrace, AllowMovement, AllowChat,
        AllowGlobalChat, AllowParty, AllowExchange, AllowStall, AllowPvP, AllowAlchemy,
        AllowSpecialItems, AllowBerserk, AutoPvpCape, InactivityReturnSeconds, EventSuitMode,
        ManagedEventCode, ManagedAtUtc
    )
    VALUES
    (
        @WorldID, @RegionID, @EventCode, 1, 0, 1, 0, 0,
        0, 0, 1, 0, 0, 1, 1, 1, @AllowParty, 0, 0, 1, 0, 0, 1, 0, 0, @EventSuitMode,
        @EventCode, SYSUTCDATETIME()
    );

DELETE FROM [dbo].[Security_RegionFeatures]
WHERE ManagedEventCode = @EventCode
  AND (WorldID <> @WorldID OR RegionID <> @RegionID);",
            new
            {
                EventCode = eventCode.Trim().ToUpperInvariant(),
                WorldID = worldId,
                RegionID = regionId,
                EventSuitMode = teams ? 2 : 1,
                AllowParty = allowParty
            });

        Invalidate();
        Log.Information(
            "[RegionFeatures] synchronized event profile Event={EventCode} WorldID={WorldID} RegionID={RegionID} Mode={Mode} Party={AllowParty}",
            eventCode,
            worldId,
            regionId,
            teams ? "Teams" : "FreeForAll",
            allowParty);
    }

    private static async Task EnsureSchemaAsync()
    {
        if (_schemaReady)
            return;

        await SchemaLock.WaitAsync();
        try
        {
            if (_schemaReady)
                return;

        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        var ready = await connection.ExecuteScalarAsync<int>(@"
SELECT CASE WHEN OBJECT_ID(N'dbo.Security_RegionFeatures', N'U') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Security_RegionFeatures', N'WorldID') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Security_RegionFeatures', N'RegionID') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Security_RegionFeatures', N'ManagedEventCode') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Security_RegionFeatures', N'ManagedAtUtc') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Security_RegionFeatures', N'BuildMode') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Security_RegionFeatures', N'JobMode') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Security_RegionFeatures', N'RaceMode') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Security_RegionFeatures', N'PartyMode') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Security_RegionFeatures', N'AllowTeleport') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Security_RegionFeatures', N'InactivityReturnSeconds') IS NOT NULL
             THEN 1 ELSE 0 END;");
        if (ready != 1)
            throw new InvalidOperationException(
                "Region-control schema is incomplete. Apply the packaged database updates.");
            _schemaReady = true;
        }
        finally
        {
            SchemaLock.Release();
        }
    }

    private static void Invalidate()
    {
        Cache.Clear();
    }

    private static void RemoveExpiredCacheEntries(DateTime nowUtc)
    {
        var nowTicks = nowUtc.Ticks;
        var lastCleanupTicks = Volatile.Read(ref _lastCacheCleanupTicks);
        if (nowTicks - lastCleanupTicks < CacheCleanupInterval.Ticks ||
            Interlocked.CompareExchange(ref _lastCacheCleanupTicks, nowTicks, lastCleanupTicks) != lastCleanupTicks)
        {
            return;
        }

        foreach (var pair in Cache)
        {
            if (nowUtc - pair.Value.FetchedAt <= CacheRetention ||
                !Cache.TryRemove(pair.Key, out _))
            {
                continue;
            }

        }
    }

    public static async Task<PacketResult?> BlockIfAsync(
        ISession session,
        Func<_FilterRegionControl, bool> blocked,
        string languageKey,
        bool useNativeNotice = false)
    {
        var rule = await GetRuleAsync(session);
        if (rule == null || !blocked(rule))
            return null;

        var message = PlayerLanguage.Get(languageKey);
        if (useNativeNotice)
            await session.SendNotice(message);
        else
            await SendNoticeAsync(session, message);
        return new PacketResult(PacketResultType.Block);
    }

    public static async Task SendNoticeAsync(ISession session, string message)
    {
        if (!ShouldSendNotice(session, message))
            return;

        var packet = new Packet(0x168A);
        packet.WriteUInt8(NoticeType.WARNING);
        packet.WriteUnicode(message);
        await session.SendToClient(packet);
    }

    public static void MarkMovement(ISession session)
    {
        session.SessionData.LastRegionMovementUtc = DateTime.UtcNow;
    }

    public static void ArmPostArrivalAdmission(ISession session, RegionTravelMethod method)
    {
        if (method == RegionTravelMethod.Arrival)
            return;
        session.SessionData.PendingRegionTravelMethod = (byte)method;
        session.SessionData.PendingRegionTravelUtc = DateTime.UtcNow;
    }

    public static async Task<bool> CheckInactivityAsync(ISession session)
    {
        var rule = await GetRuleAsync(session);
        if (rule == null || !rule.Enable_InactivityReturn)
            return false;

        var timeoutSeconds = Math.Max(1, rule.InactivitySeconds);
        if (DateTime.UtcNow - session.SessionData.LastRegionMovementUtc < TimeSpan.FromSeconds(timeoutSeconds))
            return false;

        Log.Information(
            "[RegionFeatures] inactivity return. CharID={CharID} WorldID={WorldID} RegionID={RegionID} Seconds={Seconds}",
            session.SessionData.Charid,
            session.SessionData.WorldID,
            session.SessionData.LatestRegion,
            timeoutSeconds);
        await ReturnToTownAsync(session, "Region.InactivityReturned", timeoutSeconds);
        return true;
    }

    public static async Task ApplyAutoPvpAsync(ISession session, bool force = false)
    {
        if (PvpChallengeService.IsCapeControlled(session.SessionData.Charid))
            return;

        var now = DateTime.UtcNow;
        if (!force && now - session.SessionData.LastAutoPvpCheckUtc < AutoPvpCheckInterval)
            return;

        session.SessionData.LastAutoPvpCheckUtc = now;
        if (force)
            ResetEventTeamCache(session);

        var rule = await GetRuleAsync(session);
        var eventSuitSetting = GetEffectiveEventSuitSetting(session, rule);
        if (session.SessionData.JobType == 4 &&
            eventSuitSetting is { EventSuit: true, AutoCape: true })
        {
            await ApplyEventSuitCapeAsync(session, eventSuitSetting);
            return;
        }

        ResetEventTeamCache(session);
        if (rule == null)
            return;

        var cape = rule.Enable_AutoPvP;
        if (cape < 1 || cape > 5)
            return;

        if ((byte)session.SessionData.State.PvpCape == cape)
            return;

        var packet = new Packet(0x3502);
        packet.WriteAscii(session.GameServerPacketKey);
        packet.WriteUInt8(cape);
        await session.SendToServer(packet);
    }

    public static async Task ApplyEventSuitAutoCapeAsync(ISession session)
    {
        if (session.SessionData.JobType != 4 || PvpChallengeService.IsCapeControlled(session.SessionData.Charid))
            return;

        var rule = await GetRuleAsync(session);
        var setting = GetEffectiveEventSuitSetting(session, rule);
        if (setting == null || !setting.EventSuit || !setting.AutoCape)
            return;

        await ApplyEventSuitCapeAsync(session, setting);
    }

    public static async Task<bool> IsEventSuitCapeControlledAsync(ISession session)
    {
        var rule = await GetRuleAsync(session);
        var setting = GetEffectiveEventSuitSetting(session, rule);
        return setting is { AutoCape: true };
    }

    private static async Task ApplyEventSuitCapeAsync(ISession session, _RefMapSettings setting)
    {
        byte cape = 0;
        if (setting.RegionType == EventSuitFreeForAll)
        {
            cape = 5;
        }
        else if (setting.RegionType == EventSuitTeams)
        {
            cape = await GetEventTeamAsync(session, setting.EventName);
        }

        if (cape == 0 || (byte)session.SessionData.State.PvpCape == cape)
            return;

        var packet = new Packet(0x3502);
        packet.WriteAscii(session.GameServerPacketKey);
        packet.WriteUInt8(cape);
        await session.SendToServer(packet);
    }

    public static async Task SendEventSuitSnapshotAsync(ISession session)
    {
        var rule = await GetRuleAsync(session);
        var setting = GetEffectiveEventSuitSetting(session, rule);
        if (setting == null)
            return;

        var packet = new Packet(0x189E, false, false);
        packet.WriteInt32(1);
        packet.WriteInt32(setting.RegionID);
        packet.WriteBool(setting.EventSuit);
        packet.WriteBool(setting.HideBuffViewer);
        packet.WriteBool(setting.DisablePetSpawn);
        packet.WriteBool(setting.DisableParty);
        packet.WriteBool(setting.AutoCape);
        packet.WriteBool(setting.HideMiniMap);
        packet.WriteByte(setting.RegionType);

        Log.Information(
            "[EventSuit] sent snapshot Char={CharName} Region={RegionID} World={WorldID} Source={Source} Mode={Mode}",
            session.SessionData.Charname,
            session.SessionData.LatestRegion,
            session.SessionData.WorldID,
            setting.EventName,
            setting.RegionType == EventSuitTeams ? "Teams" : "FreeForAll");

        await session.SendToClient(packet);
    }

    private static _RefMapSettings? GetEffectiveEventSuitSetting(
        ISession session,
        _FilterRegionControl? rule)
    {
        _RefMapSettings? mapSetting = null;
        var currentRegionId = NormalizeRegionId(session.SessionData.LatestRegion);
        if (IsValidRegionId(currentRegionId))
            RefManager.m_RefEventMapSettings.TryGetValue(currentRegionId, out mapSetting);

        if (mapSetting == null && session.SessionData.WorldID > 0)
            RefManager.m_RefEventMapSettings.TryGetValue(session.SessionData.WorldID, out mapSetting);

        var clientRegionId = IsValidRegionId(currentRegionId)
            ? currentRegionId
            : mapSetting?.RegionID ?? session.SessionData.WorldID;

        if (rule != null &&
            rule.WorldID > 0 &&
            rule.WorldID == session.SessionData.WorldID &&
            !string.IsNullOrWhiteSpace(rule.ManagedEventCode))
        {
            return rule.Enable_EventSuit
                ? CreateEventSuitSetting(rule, clientRegionId)
                : null;
        }

        // Map_Settings remains the explicit, backwards-compatible override.
        if (mapSetting != null)
            return CloneMapSetting(mapSetting, clientRegionId);

        if (rule == null || !rule.Enable_EventSuit)
            return null;

        return CreateEventSuitSetting(rule, clientRegionId);
    }

    private static async Task<byte> GetEventTeamAsync(ISession session, string eventName)
    {
        var now = DateTime.UtcNow;
        var cachedTeam = session.SessionData.EventSuitTeam;
        var cacheTtl = cachedTeam is > 0 and < 5 ? EventTeamCacheTtl : EventTeamMissCacheTtl;
        if (session.SessionData.EventSuitTeamCharId == session.SessionData.Charid &&
            string.Equals(session.SessionData.EventSuitTeamEventName, eventName, StringComparison.OrdinalIgnoreCase) &&
            now - session.SessionData.EventSuitTeamFetchedUtc < cacheTtl)
        {
            return cachedTeam;
        }

        byte team = 0;
        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            team = await connection.QueryFirstOrDefaultAsync<byte>(
                "SELECT Team FROM [dbo].[Event_CurrentTeams] WITH (NOLOCK) WHERE CharID = @CharID",
                new { CharID = session.SessionData.Charid });
        }
        catch (Exception ex)
        {
            Log.Warning("[EventSuit] team lookup failed for CharID={CharID}: {Message}", session.SessionData.Charid, ex.Message);
        }

        session.SessionData.EventSuitTeamCharId = session.SessionData.Charid;
        session.SessionData.EventSuitTeamEventName = eventName ?? string.Empty;
        session.SessionData.EventSuitTeam = team is > 0 and < 5 ? team : (byte)0;
        session.SessionData.EventSuitTeamFetchedUtc = now;
        return session.SessionData.EventSuitTeam;
    }

    private static void ResetEventTeamCache(ISession session)
    {
        session.SessionData.EventSuitTeamCharId = 0;
        session.SessionData.EventSuitTeamEventName = string.Empty;
        session.SessionData.EventSuitTeam = 0;
        session.SessionData.EventSuitTeamFetchedUtc = DateTime.MinValue;
    }

    private static _RefMapSettings CreateEventSuitSetting(_FilterRegionControl rule, int clientRegionId)
    {
        return new _RefMapSettings
        {
            EventName = string.IsNullOrWhiteSpace(rule.ManagedEventCode)
                ? "Security_RegionFeatures"
                : rule.ManagedEventCode,
            RegionType = rule.EventSuit_Team == EventSuitFreeForAll
                ? EventSuitFreeForAll
                : EventSuitTeams,
            RegionID = IsValidRegionId(clientRegionId) ? clientRegionId : rule.RegionID,
            EventSuit = true,
            DisableParty = !rule.Enable_Party,
            AutoCape = true,
            DisableChat = !rule.Enable_Chat,
            DisableTrace = !rule.Enable_Trace,
            DisableZerk = !rule.Enable_Zerk
        };
    }

    private static _RefMapSettings CloneMapSetting(_RefMapSettings setting, int regionId)
    {
        return new _RefMapSettings
        {
            EventName = string.IsNullOrWhiteSpace(setting.EventName) ? "Map_Settings" : setting.EventName,
            RegionType = setting.RegionType,
            RegionID = IsValidRegionId(NormalizeRegionId(regionId)) ? NormalizeRegionId(regionId) : setting.RegionID,
            EventSuit = setting.EventSuit,
            HideBuffViewer = setting.HideBuffViewer,
            DisablePetSpawn = setting.DisablePetSpawn,
            DisableParty = setting.DisableParty,
            AutoCape = setting.AutoCape,
            DisableChat = setting.DisableChat,
            DisableTrace = setting.DisableTrace,
            DisableZerk = setting.DisableZerk,
            ClosePlayerAttack = setting.ClosePlayerAttack,
            HideMiniMap = setting.HideMiniMap,
            HideName = setting.HideName
        };
    }

    public static async Task<RegionAdmissionDecision> CheckAdmissionAsync(
        ISession session,
        int worldId,
        int regionId,
        RegionTravelMethod method,
        bool sendNotice = true,
        bool failClosed = true)
    {
        regionId = NormalizeRegionId(regionId);
        if (worldId < 0 || !IsValidRegionId(regionId))
            return await DenyAsync(session, "Region.AdmissionDestinationUnavailable", sendNotice);

        var rule = await GetRuleAsync(worldId, regionId);
        var key = new RuleKey(Math.Max(0, worldId), regionId);
        if (!Cache.TryGetValue(key, out var cacheEntry) || !cacheEntry.LookupSucceeded)
        {
            if (!failClosed)
                return RegionAdmissionDecision.Allow;
            return await DenyAsync(session, "Region.AdmissionTemporarilyUnavailable", sendNotice);
        }

        if (rule == null)
            return RegionAdmissionDecision.Allow;

        var requiresBuild = !rule.Allow_StrCharacter || !rule.Allow_IntCharacter || !rule.Allow_HybridCharacter;
        var strength = session.SessionData.BaseStr > 0 ? session.SessionData.BaseStr : session.SessionData.StatSTR;
        var intellect = session.SessionData.BaseInt > 0 ? session.SessionData.BaseInt : session.SessionData.StatINT;
        if (requiresBuild)
        {
            var build = await LoadCharacterBuildAsync(session.SessionData.Charid);
            if (build == null)
                return await DenyAsync(session, "Region.AdmissionCharacterDataUnavailable", sendNotice);

            strength = build.Strength;
            intellect = build.Intellect;
            session.SessionData.BaseStr = strength;
            session.SessionData.BaseInt = intellect;
        }

        var profile = new RegionAdmissionProfile(
            strength,
            intellect,
            session.SessionData.CurLevel,
            session.SessionData.JobType,
            session.SessionData.CHChar,
            session.SessionData.EUChar,
            session.SessionData.IsInParty);
        var decision = EvaluateAdmission(rule, profile, method);
        if (!decision.Allowed && sendNotice && decision.LanguageKey != null)
            await SendNoticeAsync(session, PlayerLanguage.Get(decision.LanguageKey, decision.Arguments ?? Array.Empty<object>()));

        if (!decision.Allowed)
        {
            Log.Information(
                "[RegionAdmission] blocked. CharID={CharID} WorldID={WorldID} RegionID={RegionID} Method={Method} Reason={Reason}",
                session.SessionData.Charid,
                worldId,
                regionId,
                method,
                decision.LanguageKey);
        }
        return decision;
    }

    public static RegionAdmissionDecision EvaluateAdmission(
        _FilterRegionControl rule,
        RegionAdmissionProfile profile,
        RegionTravelMethod method)
    {
        if ((method is RegionTravelMethod.Teleport or RegionTravelMethod.FilterTeleport) && !rule.Enable_TeleportEntry)
            return new(false, "Region.TeleportEntryDisabled");
        if (method == RegionTravelMethod.Reverse && !rule.Enable_Reverse)
            return new(false, "Region.ReverseDisabled");
        if (method == RegionTravelMethod.Trace && !rule.Enable_Trace)
            return new(false, "Region.TraceDisabled");

        var isJobless = profile.JobType == 4;
        if (profile.JobType is < 1 or > 4)
            return new(false, "Region.AdmissionCharacterDataUnavailable");
        if (rule.JobMode == 1 && !isJobless)
            return new(false, "Region.JoblessRequired");
        if (rule.JobMode is >= 2 and <= 5 && isJobless)
            return new(false, "Region.JobRequired");
        if (rule.JobMode == 3 && profile.JobType != 1)
            return new(false, "Region.TraderRequired");
        if (rule.JobMode == 4 && profile.JobType != 2)
            return new(false, "Region.ThiefRequired");
        if (rule.JobMode == 5 && profile.JobType != 3)
            return new(false, "Region.HunterRequired");

        if (profile.Strength > profile.Intellect && !rule.Allow_StrCharacter)
            return new(false, "Region.StrCharactersDisabled");
        if (profile.Intellect > profile.Strength && !rule.Allow_IntCharacter)
            return new(false, "Region.IntCharactersDisabled");
        if (profile.Strength == profile.Intellect && !rule.Allow_HybridCharacter)
            return new(false, "Region.HybridCharactersDisabled");

        if (profile.IsChinese && !rule.Allow_Chinese)
            return new(false, "Region.ChineseCharactersDisabled");
        if (profile.IsEuropean && !rule.Allow_European)
            return new(false, "Region.EuropeanCharactersDisabled");
        if (rule.MinLevel > 0 && profile.Level < rule.MinLevel)
            return new(false, "Region.MinimumLevelRequired", new object[] { rule.MinLevel });
        if (rule.MaxLevel > 0 && profile.Level > rule.MaxLevel)
            return new(false, "Region.MaximumLevelAllowed", new object[] { rule.MaxLevel });
        if (rule.EntryPartyMode == 1 && !profile.IsInParty)
            return new(false, "Region.PartyRequired");
        if (rule.EntryPartyMode == 2 && profile.IsInParty)
            return new(false, "Region.PartyNotAllowed");

        return RegionAdmissionDecision.Allow;
    }

    public static async Task<RegionDestination?> ResolveTeleportDestinationAsync(int refTeleportId)
    {
        if (refTeleportId <= 0 || string.IsNullOrWhiteSpace(_serverSettings.ShardDB))
            return null;

        try
        {
            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
            await using var connection = new SqlConnection(Program.Connectionstring);
            var destination = await connection.QueryFirstOrDefaultAsync<RegionDestination>(
                $"SELECT TOP (1) CONVERT(int, GenWorldID) WorldID, CONVERT(int, GenRegionID) RegionID FROM {shardDb}.dbo._RefTeleport WITH (NOLOCK) WHERE ID = @ID AND Service = 1;",
                new { ID = refTeleportId });
            return destination == null
                ? null
                : destination with { RegionID = NormalizeRegionId(destination.RegionID) };
        }
        catch (Exception ex)
        {
            Log.Warning("[RegionAdmission] teleport destination lookup failed. RefTeleportID={RefTeleportID}: {Message}", refTeleportId, ex.Message);
            return null;
        }
    }

    public static async Task<RegionDestination?> ResolveReverseDestinationAsync(
        ISession session,
        bool deadLocation)
    {
        if (session.SessionData.Charid <= 0 || string.IsNullOrWhiteSpace(_serverSettings.ShardDB))
            return null;

        try
        {
            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
            var column = deadLocation ? "DiedRegion" : "TelRegion";
            await using var connection = new SqlConnection(Program.Connectionstring);
            var regionId = await connection.QueryFirstOrDefaultAsync<int?>(
                $"SELECT TOP (1) CONVERT(int, {column}) FROM {shardDb}.dbo._Char WITH (NOLOCK) WHERE CharID = @CharID;",
                new { CharID = session.SessionData.Charid });
            return regionId.HasValue && IsValidRegionId(NormalizeRegionId(regionId.Value))
                ? new RegionDestination(Math.Max(0, session.SessionData.WorldID), NormalizeRegionId(regionId.Value))
                : null;
        }
        catch (Exception ex)
        {
            Log.Warning(
                "[RegionAdmission] reverse destination lookup failed. CharID={CharID} DeadLocation={DeadLocation}: {Message}",
                session.SessionData.Charid,
                deadLocation,
                ex.Message);
            return null;
        }
    }

    public static async Task<PacketResult?> BlockReverseItemActivationAsync(ISession session)
    {
        var destinations = new List<(string Name, RegionDestination Destination)>();
        var lastRecall = await ResolveReverseDestinationAsync(session, deadLocation: false);
        if (lastRecall != null)
            destinations.Add(("LastRecall", lastRecall));

        var deathLocation = await ResolveReverseDestinationAsync(session, deadLocation: true);
        if (deathLocation != null && !destinations.Any(candidate =>
                candidate.Destination.WorldID == deathLocation.WorldID &&
                candidate.Destination.RegionID == deathLocation.RegionID))
        {
            destinations.Add(("DeathLocation", deathLocation));
        }

        if (destinations.Count == 0)
        {
            // A new character legitimately has neither value until the native
            // game establishes one. The native GameServer owns that destination;
            // arm the existing post-arrival admission check and allow the scroll.
            Log.Debug(
                "[RegionAdmission] Reverse item activation has no saved destination; native travel allowed with post-arrival enforcement. CharID={CharID}",
                session.SessionData.Charid);
            return null;
        }

        foreach (var candidate in destinations)
        {
            var decision = await CheckAdmissionAsync(
                session,
                candidate.Destination.WorldID,
                candidate.Destination.RegionID,
                RegionTravelMethod.Reverse,
                sendNotice: false);
            if (decision.Allowed)
                continue;

            if (!string.IsNullOrWhiteSpace(decision.LanguageKey))
            {
                await SendNoticeAsync(
                    session,
                    PlayerLanguage.Get(decision.LanguageKey, decision.Arguments ?? Array.Empty<object>()));
            }

            Log.Information(
                "[RegionAdmission] Reverse item activation blocked before movement. CharID={CharID}, Candidate={Candidate}, WorldID={WorldID}, RegionID={RegionID}, Reason={Reason}",
                session.SessionData.Charid,
                candidate.Name,
                candidate.Destination.WorldID,
                candidate.Destination.RegionID,
                decision.LanguageKey ?? "Denied");
            return new PacketResult(PacketResultType.Block);
        }

        Log.Debug(
            "[RegionAdmission] Reverse item activation admitted before movement. CharID={CharID}, CandidateCount={CandidateCount}",
            session.SessionData.Charid,
            destinations.Count);
        return null;
    }

    public static async Task EnforceRegionEntryAsync(ISession session)
    {
        var method = RegionTravelMethod.Arrival;
        if (DateTime.UtcNow - session.SessionData.PendingRegionTravelUtc <= TimeSpan.FromSeconds(45) &&
            Enum.IsDefined(typeof(RegionTravelMethod), (int)session.SessionData.PendingRegionTravelMethod))
        {
            method = (RegionTravelMethod)session.SessionData.PendingRegionTravelMethod;
        }
        session.SessionData.PendingRegionTravelMethod = 0;
        session.SessionData.PendingRegionTravelUtc = DateTime.MinValue;

        var decision = await CheckAdmissionAsync(
            session,
            session.SessionData.WorldID,
            session.SessionData.LatestRegion,
            method,
            sendNotice: true,
            failClosed: false);
        if (!decision.Allowed)
            await ReturnToTownAsync(session, null);
    }

    private static async Task<RegionAdmissionDecision> DenyAsync(
        ISession session,
        string languageKey,
        bool sendNotice,
        params object[] arguments)
    {
        if (sendNotice)
            await SendNoticeAsync(session, PlayerLanguage.Get(languageKey, arguments));
        return new RegionAdmissionDecision(false, languageKey, arguments);
    }

    /// <summary>
    /// vSRO sends RegionID as an unsigned 16-bit packet value while SQL stores it
    /// as SMALLINT. Values such as 32788 therefore represent -32748, not an
    /// invalid destination. All admission keys use the signed SQL form.
    /// </summary>
    public static int NormalizeRegionId(int regionId)
    {
        if (regionId is >= short.MinValue and <= short.MaxValue)
            return regionId;
        if (regionId is > short.MaxValue and <= ushort.MaxValue)
            return unchecked((short)(ushort)regionId);
        return regionId;
    }

    public static bool IsValidRegionId(int regionId)
        => regionId != 0 && regionId is >= short.MinValue and <= short.MaxValue;

    private static async Task<CharacterBuildRow?> LoadCharacterBuildAsync(int charId)
    {
        if (charId <= 0 || string.IsNullOrWhiteSpace(_serverSettings.ShardDB))
            return null;

        try
        {
            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
            await using var connection = new SqlConnection(Program.Connectionstring);
            return await connection.QueryFirstOrDefaultAsync<CharacterBuildRow>(
                $"SELECT TOP (1) CONVERT(int, Strength) Strength, CONVERT(int, Intellect) Intellect FROM {shardDb}.dbo._Char WITH (NOLOCK) WHERE CharID = @CharID;",
                new { CharID = charId });
        }
        catch (Exception ex)
        {
            Log.Warning("[RegionAdmission] character build lookup failed. CharID={CharID}: {Message}", charId, ex.Message);
            return null;
        }
    }

    private static async Task ReturnToTownAsync(ISession session, string? languageKey, params object[] arguments)
    {
        if (DateTime.UtcNow - session.SessionData.LastRegionReturnUtc < ReturnCooldown)
            return;

        session.SessionData.LastRegionReturnUtc = DateTime.UtcNow;
        MarkMovement(session);
        if (!string.IsNullOrWhiteSpace(languageKey))
            await SendNoticeAsync(session, PlayerLanguage.Get(languageKey, arguments));

        var returnToTown = new Packet(0x3543, false, false);
        returnToTown.WriteAscii(session.GameServerPacketKey);
        await session.SendToServer(returnToTown);
    }

    private sealed class CharacterBuildRow
    {
        public int Strength { get; set; }
        public int Intellect { get; set; }
    }

    private static bool ShouldSendNotice(ISession session, string message)
    {
        var key = $"{session.ClientGuid}:{message}";
        var now = DateTime.UtcNow.Ticks;
        RemoveExpiredNoticeTicks(now);
        var cooldownTicks = NoticeCooldown.Ticks;

        if (!NoticeTicks.TryGetValue(key, out var previous))
            return NoticeTicks.TryAdd(key, now);

        if (now - previous < cooldownTicks)
            return false;

        return NoticeTicks.TryUpdate(key, now, previous);
    }

    private static void RemoveExpiredNoticeTicks(long nowTicks)
    {
        var previousCleanup = Volatile.Read(ref _lastNoticeCleanupTicks);
        if (nowTicks - previousCleanup < NoticeCleanupInterval.Ticks ||
            Interlocked.CompareExchange(ref _lastNoticeCleanupTicks, nowTicks, previousCleanup) != previousCleanup)
            return;

        foreach (var entry in NoticeTicks)
        {
            if (nowTicks - entry.Value >= NoticeRetention.Ticks)
                NoticeTicks.TryRemove(entry);
        }
    }
}
