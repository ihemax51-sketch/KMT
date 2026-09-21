using System.Collections.Concurrent;
using System.Security.Cryptography;
using System.Text;
using Dapper;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.PacketHandlerManager;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.ServerManagers;

public sealed record BotProtectionConfig(
    bool AllowBotLogin,
    bool AllowBotTrade,
    bool LogEnabled);

public static class BotProtectionService
{
    private static readonly TimeSpan ConfigCacheTtl = TimeSpan.FromSeconds(2);
    private static readonly TimeSpan AccountCacheTtl = TimeSpan.FromSeconds(30);
    private static readonly TimeSpan AccountCacheRetention = TimeSpan.FromMinutes(10);
    private static readonly TimeSpan AccountCacheCleanupInterval = TimeSpan.FromMinutes(1);
    private static readonly SemaphoreSlim ConfigLock = new(1, 1);
    private static readonly ConcurrentDictionary<string, ClientlessAccountCacheEntry> AccountCache =
        new(StringComparer.OrdinalIgnoreCase);

    private static ConfigCacheEntry? _configCache;
    private static long _lastAccountCacheCleanupTicks;

    private sealed record ConfigCacheEntry(BotProtectionConfig Value, DateTime FetchedAtUtc);
    private sealed record ClientlessAccountCacheEntry(ClientlessAccountProfile Value, DateTime FetchedAtUtc);
    private sealed class ClientlessAccountProfile
    {
        public bool IsManaged { get; set; }
        public bool IsSystem { get; set; }
    }

    public static bool IsBotIdentity(bool isManagedClientless, string? hwid)
    {
        return isManagedClientless ||
               (!string.IsNullOrWhiteSpace(hwid) &&
                hwid.StartsWith("CLIENTLESS:", StringComparison.OrdinalIgnoreCase));
    }

    public static bool IsBotIdentity(bool isManagedClientless, bool isExternalBot, string? hwid)
    {
        return isExternalBot || IsBotIdentity(isManagedClientless, hwid);
    }

    public static bool IsBotSession(ISession session)
    {
        return IsBotIdentity(
            session.IsManagedClientless,
            session.IsExternalBot,
            session.SessionData.Hwid);
    }

    public static string CreateExternalBotHwid(string? accountName)
    {
        var normalizedAccount = accountName?.Trim().ToLowerInvariant();
        if (string.IsNullOrWhiteSpace(normalizedAccount))
            return string.Empty;

        return Convert.ToHexString(SHA256.HashData(
            Encoding.UTF8.GetBytes($"KMTGuard.ExternalBot:{normalizedAccount}")));
    }

    public static async Task<bool> TryAdmitExternalBotSessionAsync(ISession session)
    {
        var config = await GetConfigAsync();
        if (!config.AllowBotLogin)
            return false;

        var externalBotHwid = CreateExternalBotHwid(session.PlayerUserID);
        if (string.IsNullOrWhiteSpace(externalBotHwid))
        {
            Log.Warning(
                "External bot admission rejected because the Agent account is missing. IP={ClientIp}",
                session.ClientIp);
            return false;
        }

        session.IsExternalBot = true;
        session.SessionData.Hwid = externalBotHwid;

        Log.Information(
            "External bot admitted by AllowBotLogin. Account={AccountName} IP={ClientIp}",
            session.PlayerUserID,
            session.ClientIp);
        QueueAudit(session, "LoginPolicy", "Allow", "external-bot;missing-agent-client-proof");
        return true;
    }

    public static async Task<BotProtectionConfig> GetConfigAsync()
    {
        var cached = _configCache;
        if (cached != null && DateTime.UtcNow - cached.FetchedAtUtc < ConfigCacheTtl)
            return cached.Value;

        await ConfigLock.WaitAsync();
        try
        {
            cached = _configCache;
            if (cached != null && DateTime.UtcNow - cached.FetchedAtUtc < ConfigCacheTtl)
                return cached.Value;

            var fallback = new BotProtectionConfig(
                _serverSettings.AllowBotLogin,
                _serverSettings.AllowBotTrade,
                _serverSettings.BotProtectionLogEnabled);

            try
            {
                await using var connection = new SqlConnection(Program.Connectionstring);
                var rows = await connection.QueryAsync<SettingRow>(@"
SELECT SettingName, Value
FROM [dbo].[System_Settings] WITH (NOLOCK)
WHERE SettingName IN ('AllowBotLogin', 'AllowBotTrade', 'BotProtectionLogEnabled');");

                var values = rows.ToDictionary(row => row.SettingName, row => row.Value, StringComparer.OrdinalIgnoreCase);
                var config = new BotProtectionConfig(
                    ReadBool(values, "AllowBotLogin", fallback.AllowBotLogin),
                    ReadBool(values, "AllowBotTrade", fallback.AllowBotTrade),
                    ReadBool(values, "BotProtectionLogEnabled", fallback.LogEnabled));

                _configCache = new ConfigCacheEntry(config, DateTime.UtcNow);
                return config;
            }
            catch (Exception ex)
            {
                Log.Warning("Bot protection settings read failed; using startup defaults: {Message}", ex.Message);
                _configCache = new ConfigCacheEntry(fallback, DateTime.UtcNow);
                return fallback;
            }
        }
        finally
        {
            ConfigLock.Release();
        }
    }

    public static async Task<bool> ShouldBlockGatewayLoginAsync(ISession session, string accountName)
    {
        var profile = await GetClientlessAccountProfileAsync(accountName);
        session.IsManagedClientless = profile.IsManaged;
        session.IsSystemClientless = profile.IsSystem;

        if (profile.IsSystem)
            return false;

        var config = await GetConfigAsync();
        if (config.AllowBotLogin)
            return false;

        var isUnverified = string.IsNullOrWhiteSpace(session.SessionData.Hwid);
        if (!profile.IsManaged && !isUnverified)
            return false;

        var reason = profile.IsManaged ? "managed-clientless" : "missing-client-proof";
        Log.Warning(
            "Bot login blocked at Gateway. Account={AccountName} IP={ClientIp} Reason={Reason}",
            accountName,
            session.ClientIp,
            reason);
        QueueAudit(session, "LoginBlocked", "Disconnect", reason);
        return true;
    }

    public static async Task<bool> ShouldDisconnectBotSessionAsync(ISession session)
    {
        if (!IsBotSession(session))
            return false;

        if (session.IsSystemClientless)
            return false;

        var config = await GetConfigAsync();
        if (config.AllowBotLogin)
            return false;

        Log.Warning(
            "Bot session disconnected by AllowBotLogin. Account={AccountName} Char={CharName} IP={ClientIp}",
            session.PlayerUserID,
            session.SessionData.Charname,
            session.ClientIp);
        QueueAudit(session, "LoginPolicy", "Disconnect", "AllowBotLogin=False");
        return true;
    }

    public static async Task<PacketResult?> BlockBotTradeIfDisabledAsync(ISession session, string activity)
    {
        if (!IsBotSession(session))
            return null;

        var config = await GetConfigAsync();
        if (config.AllowBotTrade)
            return null;

        var notice = PlayerLanguage.Get("BotProtection.ActivityDisabled", activity);
        var packet = new Packet(0x168A);
        packet.WriteUInt8(NoticeType.WARNING);
        packet.WriteUnicode(notice);
        await session.SendToClient(packet);

        Log.Warning(
            "Bot trade blocked. Activity={Activity} Account={AccountName} Char={CharName} Region={RegionID}",
            activity,
            session.PlayerUserID,
            session.SessionData.Charname,
            session.SessionData.LatestRegion);
        QueueAudit(session, "TradeBlocked", "Block", activity);
        return new PacketResult(PacketResultType.Block);
    }

    public static void QueueAudit(ISession session, string eventType, string action, string details)
    {
        _ = WriteAuditAsync(
            eventType,
            action,
            session.PlayerUserID,
            session.SessionData.Charid,
            session.SessionData.Charname,
            session.SessionData.Hwid,
            session.ClientIp,
            session.SessionData.LatestRegion,
            details);
    }

    public static async Task PopulateClientlessIdentityAsync(ISession session, string accountName)
    {
        var profile = await GetClientlessAccountProfileAsync(accountName);
        session.IsManagedClientless = profile.IsManaged;
        session.IsSystemClientless = profile.IsSystem;
    }

    private static async Task<ClientlessAccountProfile> GetClientlessAccountProfileAsync(string accountName)
    {
        var normalizedAccount = accountName?.Trim() ?? string.Empty;
        if (normalizedAccount.Length == 0)
            return new ClientlessAccountProfile();

        var now = DateTime.UtcNow;
        RemoveExpiredAccountCacheEntries(now);
        if (AccountCache.TryGetValue(normalizedAccount, out var cached) &&
            now - cached.FetchedAtUtc < AccountCacheTtl)
        {
            return cached.Value;
        }

        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            var profile = await connection.QuerySingleAsync<ClientlessAccountProfile>(@"
IF OBJECT_ID(N'[dbo].[Clientless_Accounts]', N'U') IS NULL
BEGIN
    SELECT CONVERT(bit, 0) AS IsManaged, CONVERT(bit, 0) AS IsSystem;
END
ELSE IF COL_LENGTH(N'[dbo].[Clientless_Accounts]', N'SystemRole') IS NULL
BEGIN
    SELECT
        CONVERT(bit, CASE WHEN EXISTS
        (
            SELECT 1
            FROM [dbo].[Clientless_Accounts] WITH (NOLOCK)
            WHERE Enabled = 1 AND AccountName = @AccountName
        ) THEN 1 ELSE 0 END) AS IsManaged,
        CONVERT(bit, 0) AS IsSystem;
END
ELSE
BEGIN
    EXEC sys.sp_executesql N'
        SELECT
            CONVERT(bit, CASE WHEN AccountName IS NULL THEN 0 ELSE 1 END) AS IsManaged,
            CONVERT(bit, CASE WHEN SystemRole IS NULL THEN 0 ELSE 1 END) AS IsSystem
        FROM (VALUES (1)) AS seed(Value)
        OUTER APPLY
        (
            SELECT TOP (1) AccountName, SystemRole
            FROM [dbo].[Clientless_Accounts] WITH (NOLOCK)
            WHERE Enabled = 1 AND AccountName = @AccountName
        ) AS account;',
        N'@AccountName varchar(128)',
        @AccountName;
END",
                new { AccountName = normalizedAccount });

            AccountCache[normalizedAccount] = new ClientlessAccountCacheEntry(profile, now);
            return profile;
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Clientless account profile lookup failed for {AccountName}", normalizedAccount);
            return new ClientlessAccountProfile();
        }
    }

    private static void RemoveExpiredAccountCacheEntries(DateTime now)
    {
        var nowTicks = now.Ticks;
        var previousCleanup = Volatile.Read(ref _lastAccountCacheCleanupTicks);
        if (nowTicks - previousCleanup < AccountCacheCleanupInterval.Ticks ||
            Interlocked.CompareExchange(ref _lastAccountCacheCleanupTicks, nowTicks, previousCleanup) != previousCleanup)
        {
            return;
        }

        foreach (var entry in AccountCache)
        {
            if (now - entry.Value.FetchedAtUtc >= AccountCacheRetention)
                AccountCache.TryRemove(entry.Key, out _);
        }
    }

    private static async Task WriteAuditAsync(
        string eventType,
        string action,
        string accountName,
        int charId,
        string charName,
        string hwid,
        string clientIp,
        int regionId,
        string details)
    {
        try
        {
            var config = await GetConfigAsync();
            if (!config.LogEnabled)
                return;

            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.ExecuteAsync(@"
IF OBJECT_ID(N'[dbo].[Security_BotProtectionLog]', N'U') IS NOT NULL
BEGIN
    INSERT INTO [dbo].[Security_BotProtectionLog]
        (EventType, ActionTaken, AccountName, CharID, CharName, Hwid, ClientIP, RegionID, Details)
    VALUES
        (@EventType, @ActionTaken, NULLIF(@AccountName, ''), NULLIF(@CharID, 0),
         NULLIF(@CharName, ''), NULLIF(@Hwid, ''), NULLIF(@ClientIP, ''), NULLIF(@RegionID, 0), @Details);
END",
                new
                {
                    EventType = eventType,
                    ActionTaken = action,
                    AccountName = accountName ?? string.Empty,
                    CharID = charId,
                    CharName = charName ?? string.Empty,
                    Hwid = hwid ?? string.Empty,
                    ClientIP = clientIp ?? string.Empty,
                    RegionID = regionId,
                    Details = details ?? string.Empty
                });
        }
        catch (Exception ex)
        {
            Log.Debug(ex, "Bot protection audit write failed");
        }
    }

    private static bool ReadBool(IReadOnlyDictionary<string, string> values, string name, bool fallback)
    {
        return values.TryGetValue(name, out var value) && bool.TryParse(value, out var parsed)
            ? parsed
            : fallback;
    }

    private sealed class SettingRow
    {
        public string SettingName { get; set; } = string.Empty;
        public string Value { get; set; } = string.Empty;
    }
}
