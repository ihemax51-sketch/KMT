using System.Security.Cryptography;
using System.Collections.Concurrent;
using System.Text.RegularExpressions;
using Dapper;
using KMTGuard.Clientless;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Features.AutoEvents;

public sealed class HideAndSeekRuntimeConfig
{
    public string EventCode { get; set; } = "HNS";
    public string DisplayName { get; set; } = "Hide and Seek";
    public bool Enabled { get; set; } = true;
    public int StartDelaySeconds { get; set; } = 60;
    public int SearchSeconds { get; set; } = 600;
    public int ReminderIntervalSeconds { get; set; } = 120;
    public int MinLevel { get; set; } = 1;
    public int HwidLimit { get; set; } = 1;
    public bool RequireHwid { get; set; }
    public string BotAccountName { get; set; } = "kmthnsmaster";
    public string BotPassword { get; set; } = string.Empty;
    public string BotCharacterName { get; set; } = "KMT_HideMaster";
    public short BotShardID { get; set; } = 64;
    public byte BotLocale { get; set; } = 22;
    public int ReturnWorldID { get; set; } = 1;
    public int ReturnRegionID { get; set; } = 25000;
    public int ReturnX { get; set; } = 982;
    public int ReturnY { get; set; }
    public int ReturnZ { get; set; } = 140;
}

public readonly record struct HideAndSeekExchangeResult(bool IsEventTarget, bool ForwardForRangeValidation);

public static class HideAndSeekEventService
{
    private const string SystemRole = "HNS";
    internal const ushort OperatorCommandOpcode = 0x7010;
    internal const ushort InvisibleOperatorCommand = 14;
    private static readonly TimeSpan BotPositioningTimeout = TimeSpan.FromSeconds(30);
    private static readonly TimeSpan BotVisibilityChangeTimeout = TimeSpan.FromSeconds(5);
    private static readonly SemaphoreSlim ProvisionLock = new(1, 1);
    private static readonly SemaphoreSlim MaintenanceLock = new(1, 1);
    private static readonly ConcurrentDictionary<Guid, PendingExchangeAttempt> PendingExchangeAttempts = new();
    private static ActiveHideAndSeekRun? _activeRun;
    private static DateTime _lastMaintenanceUtc = DateTime.MinValue;
    private static long _botUniqueId;
    private static string _botCharacterName = string.Empty;

    public static async Task EnsureSchemaAsync(SqlConnection connection)
    {
        var proxyDb = SqlIdentifier.Quote(Program.ProxyDb);
        var ready = await connection.ExecuteScalarAsync<int>($@"
SELECT CASE WHEN OBJECT_ID(N'Events.dbo._HideAndSeekConfig', N'U') IS NOT NULL
                  AND OBJECT_ID(N'Events.dbo._HideAndSeekLocation', N'U') IS NOT NULL
                  AND OBJECT_ID(N'Events.dbo._HideAndSeekReward', N'U') IS NOT NULL
                  AND OBJECT_ID(N'Events.dbo._HideAndSeekSchedule', N'U') IS NOT NULL
                  AND OBJECT_ID(N'Events.dbo._HideAndSeekRun', N'U') IS NOT NULL
                  AND COL_LENGTH(N'{proxyDb}.dbo.Clientless_Accounts', N'SystemRole') IS NOT NULL
             THEN 1 ELSE 0 END;");
        if (ready != 1)
            throw new InvalidOperationException(
                "Hide and Seek schema is incomplete. Apply the packaged database updates.");
    }
    public static async Task<HideAndSeekRuntimeConfig> LoadRuntimeConfigAsync()
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await EnsureSchemaAsync(connection);
        return await connection.QuerySingleAsync<HideAndSeekRuntimeConfig>(@"
SELECT EventCode, DisplayName, Enabled, StartDelaySeconds, SearchSeconds,
       ReminderIntervalSeconds, MinLevel, HwidLimit, RequireHwid,
       BotAccountName, ISNULL(BotPassword, '') AS BotPassword, BotCharacterName,
       BotShardID, BotLocale, ReturnWorldID, ReturnRegionID, ReturnX, ReturnY, ReturnZ
FROM Events.dbo._HideAndSeekConfig WITH (NOLOCK)
WHERE EventCode = N'HNS';");
    }

    public static string? ValidateForStart(HideAndSeekRuntimeConfig config)
    {
        if (!config.Enabled)
            return "Event HNS is disabled.";
        if (config.SearchSeconds < 30)
            return "Hide and Seek search duration must be at least 30 seconds.";
        if (config.ReturnWorldID <= 0 || config.ReturnRegionID <= 0)
            return "Hide and Seek return WorldID and RegionID are required.";
        if (!Regex.IsMatch(config.BotAccountName ?? string.Empty, @"^[a-z0-9_]{4,16}$",
                RegexOptions.IgnoreCase))
            return "Hide and Seek bot account username must be 4-16 letters, numbers, or underscore.";
        if (!Regex.IsMatch(config.BotPassword ?? string.Empty, @"^[\x21-\x7E]{6,32}$"))
            return "Hide and Seek bot password must be 6-32 visible characters without spaces.";
        if (string.IsNullOrWhiteSpace(config.BotCharacterName) ||
            config.BotCharacterName.Length is < 3 or > 16 ||
            config.BotCharacterName.Any(char.IsWhiteSpace))
            return "Hide and Seek bot character name must be 3-16 characters without spaces.";
        return null;
    }

    public static async Task<HideAndSeekRuntimeConfig> PrepareForStartAsync(
        HideAndSeekRuntimeConfig config,
        CancellationToken cancellationToken = default)
    {
        return await EnsureBotProvisionedAsync(config, cancellationToken);
    }

    public static async Task<string> LoginBotAsync(CancellationToken cancellationToken = default)
    {
        var config = await LoadRuntimeConfigAsync();
        config = await EnsureBotProvisionedAsync(config, cancellationToken);
        _botCharacterName = config.BotCharacterName;
        var result = await ClientlessManager.EnsureSystemAccountRunningAsync(SystemRole);
        Log.Information(
            "Hide and Seek manual login requested. Account={Account} Character={Character}",
            config.BotAccountName,
            config.BotCharacterName);
        return result;
    }

    public static Task<string> LogoutBotAsync()
    {
        return ClientlessManager.StopSystemAccountAsync(SystemRole);
    }

    public static async Task MaintainBotAsync(CancellationToken cancellationToken)
    {
        if (DateTime.UtcNow - _lastMaintenanceUtc < TimeSpan.FromSeconds(15))
            return;
        if (!await MaintenanceLock.WaitAsync(0, cancellationToken))
            return;

        try
        {
            _lastMaintenanceUtc = DateTime.UtcNow;
            var config = await LoadRuntimeConfigAsync();
            if (!config.Enabled)
                return;

            config = await EnsureBotProvisionedAsync(config, cancellationToken);
            _botCharacterName = config.BotCharacterName;
            RememberOnlineBot(config.BotCharacterName);
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Hide and Seek system character maintenance failed");
        }
        finally
        {
            MaintenanceLock.Release();
        }
    }

    public static async Task RunAsync(
        HideAndSeekRuntimeConfig config,
        long autoRunId,
        string startedBy,
        CancellationToken cancellationToken)
    {
        _ = startedBy;
        config = await EnsureBotProvisionedAsync(config, cancellationToken);

        var botSession = await WaitForOnlineBotAsync(config.BotCharacterName, TimeSpan.FromSeconds(60), cancellationToken);
        if (botSession == null)
            throw new InvalidOperationException(
                $"Hide and Seek character '{config.BotCharacterName}' is offline. Use Login Account in the desktop dashboard before starting the event.");

        var location = await PickLocationAsync(cancellationToken);
        var runIds = await CreateRunAsync(autoRunId, config.DisplayName, location, botSession, cancellationToken);
        var active = new ActiveHideAndSeekRun(runIds.HnsRunID, runIds.RoundID, autoRunId, config, location, botSession.SessionData.Charid);

        _activeRun = active;
        _botCharacterName = config.BotCharacterName;
        Interlocked.Exchange(ref _botUniqueId, botSession.SessionData.UniqueCharId);

        try
        {
            await TeleportFreezeService.TeleportToPositionAsync(
                botSession,
                location.WorldID,
                location.RegionID,
                location.PosX,
                location.PosY,
                location.PosZ);

            await WaitForBotLocationAsync(botSession, location, cancellationToken);
            await EnsureBotVisibleAsync(botSession, cancellationToken);
            var endsAtUtc = DateTime.UtcNow.AddSeconds(config.SearchSeconds);
            await MarkSearchingAsync(active.HnsRunID, endsAtUtc, cancellationToken);
            await BroadcastNoticeAsync(
                PlayerLanguage.Get(
                    "HideAndSeek.HidingAnnouncement",
                    config.BotCharacterName,
                    location.LocationName,
                    PlayerLanguage.FormatDurationSeconds(config.SearchSeconds)),
                NoticeType.NOTICE);

            var winner = await WaitForWinnerAsync(active, endsAtUtc, cancellationToken);
            if (winner == null)
            {
                await CompleteRunAsync(active, "TimedOut", null, "No player found the event character.", cancellationToken);
                await TryBroadcastNoticeAsync(
                    PlayerLanguage.Get("HideAndSeek.TimeOver", config.BotCharacterName, location.LocationName),
                    NoticeType.WARNING);
                return;
            }

            var rewardSummary = await ApplyRewardsAsync(winner);
            await CompleteRunAsync(
                active,
                "Won",
                rewardSummary,
                $"{winner.SessionData.Charname} won the event.",
                cancellationToken);
            await TryBroadcastNoticeAsync(
                PlayerLanguage.Get(
                    "HideAndSeek.WinnerAnnouncement",
                    winner.SessionData.Charname,
                    config.BotCharacterName,
                    config.DisplayName,
                    rewardSummary),
                NoticeType.NOTICE);
        }
        catch (OperationCanceledException)
        {
            await TryCompleteRunAsync(active, "Stopped", null, "Event stopped.");
            throw;
        }
        catch (Exception ex)
        {
            await TryCompleteRunAsync(active, "Failed", null, ex.Message);
            throw;
        }
        finally
        {
            if (ReferenceEquals(_activeRun, active))
                _activeRun = null;
            foreach (var pending in PendingExchangeAttempts)
            {
                if (pending.Value.HnsRunID == active.HnsRunID)
                    PendingExchangeAttempts.TryRemove(pending.Key, out _);
            }

            var currentBot = FindOnlineBot(config.BotCharacterName);
            if (currentBot != null)
            {
                try
                {
                    await TeleportFreezeService.TeleportToPositionAsync(
                        currentBot,
                        config.ReturnWorldID,
                        config.ReturnRegionID,
                        config.ReturnX,
                        config.ReturnY,
                        config.ReturnZ);
                }
                catch (Exception ex)
                {
                    Log.Warning(ex, "Hide and Seek character return teleport failed");
                }
            }
        }
    }

    public static async Task<HideAndSeekExchangeResult> PrepareExchangeAttemptAsync(
        ISession player,
        uint targetUniqueId)
    {
        var knownBotUniqueId = (uint)Math.Max(0, Interlocked.Read(ref _botUniqueId));
        if (knownBotUniqueId == 0 || targetUniqueId != knownBotUniqueId)
            return new HideAndSeekExchangeResult(false, false);

        var active = _activeRun;
        if (active == null)
        {
            await player.SendNotice(PlayerLanguage.Get("HideAndSeek.NotActive"));
            return new HideAndSeekExchangeResult(true, false);
        }

        if (player.SessionData.SELECTEDUNIQUEID != targetUniqueId)
        {
            await player.SendNotice(PlayerLanguage.Get("HideAndSeek.SelectCharacter"));
            return new HideAndSeekExchangeResult(true, false);
        }

        var bot = FindOnlineBot(active.Config.BotCharacterName);
        if (bot == null || bot.SessionData.UniqueCharId != targetUniqueId)
        {
            await player.SendNotice(PlayerLanguage.Get("HideAndSeek.Reconnecting"));
            return new HideAndSeekExchangeResult(true, false);
        }

        if (player.SessionData.WorldID != bot.SessionData.WorldID ||
            player.SessionData.LatestRegion != bot.SessionData.LatestRegion ||
            player.SessionData.WorldID != active.Location.WorldID ||
            player.SessionData.LatestRegion != active.Location.RegionID)
        {
            await player.SendNotice(PlayerLanguage.Get("HideAndSeek.MustBeNearby"));
            return new HideAndSeekExchangeResult(true, false);
        }

        if (!ServerManager.IsOnlinePlayer(player) || player.IsManagedClientless)
            return new HideAndSeekExchangeResult(true, false);

        if (active.Config.MinLevel > 0 && player.SessionData.CurLevel < active.Config.MinLevel)
        {
            await player.SendNotice(PlayerLanguage.Get("HideAndSeek.MinimumLevel", active.Config.MinLevel));
            return new HideAndSeekExchangeResult(true, false);
        }

        if (active.Config.RequireHwid && string.IsNullOrWhiteSpace(player.SessionData.Hwid))
        {
            await player.SendNotice(PlayerLanguage.Get("HideAndSeek.HwidRequired"));
            return new HideAndSeekExchangeResult(true, false);
        }

        if (!IsWithinHwidLimit(player, active.Config.HwidLimit))
        {
            await player.SendNotice(PlayerLanguage.Get("HideAndSeek.HwidLimit"));
            return new HideAndSeekExchangeResult(true, false);
        }

        PendingExchangeAttempts[player.ClientGuid] = new PendingExchangeAttempt(
            active.HnsRunID,
            targetUniqueId,
            DateTime.UtcNow.AddSeconds(5));
        return new HideAndSeekExchangeResult(true, true);
    }

    public static async Task<bool> ConfirmExchangeRangeAsync(ISession player, bool gameServerAccepted)
    {
        if (!PendingExchangeAttempts.TryRemove(player.ClientGuid, out var pending))
            return false;
        if (!gameServerAccepted || pending.ExpiresAtUtc < DateTime.UtcNow)
            return false;

        var active = _activeRun;
        if (active == null ||
            active.HnsRunID != pending.HnsRunID ||
            active.Winner.Task.IsCompleted)
        {
            return true;
        }

        var bot = FindOnlineBot(active.Config.BotCharacterName);
        if (bot == null ||
            bot.SessionData.UniqueCharId != pending.TargetUniqueID ||
            player.SessionData.SELECTEDUNIQUEID != pending.TargetUniqueID ||
            player.SessionData.WorldID != bot.SessionData.WorldID ||
            player.SessionData.LatestRegion != bot.SessionData.LatestRegion)
        {
            return true;
        }

        if (!await active.WinnerLock.WaitAsync(0))
            return true;

        try
        {
            if (active.Winner.Task.IsCompleted)
                return true;

            var claimed = await ClaimWinnerAsync(active.HnsRunID, player);
            if (!claimed)
                return true;

            active.Winner.TrySetResult(player);
            await player.SendNotice(PlayerLanguage.Get("HideAndSeek.YouWon"));
            return true;
        }
        finally
        {
            active.WinnerLock.Release();
        }
    }

    private static async Task<HideAndSeekRuntimeConfig> EnsureBotProvisionedAsync(
        HideAndSeekRuntimeConfig config,
        CancellationToken cancellationToken)
    {
        await ProvisionLock.WaitAsync(cancellationToken);
        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync(cancellationToken);
            await EnsureSchemaAsync(connection);

            var systemAccount = await connection.QueryFirstOrDefaultAsync<SystemClientlessRow>(new CommandDefinition(@"
SELECT TOP (1) AccountName, AccountPassword, CharacterName
FROM dbo.Clientless_Accounts WITH (NOLOCK)
WHERE SystemRole = @SystemRole
ORDER BY ID;", new { SystemRole }, cancellationToken: cancellationToken));

            if (systemAccount != null)
            {
                if (string.IsNullOrWhiteSpace(config.BotAccountName))
                    config.BotAccountName = systemAccount.AccountName;
                if (string.IsNullOrWhiteSpace(config.BotPassword))
                    config.BotPassword = systemAccount.AccountPassword;
                if (string.IsNullOrWhiteSpace(config.BotCharacterName))
                    config.BotCharacterName = systemAccount.CharacterName;
            }
            else
            {
                var linked = await IsConfiguredIdentityLinkedAsync(connection, config, cancellationToken);
                if (!linked)
                {
                    config.BotAccountName = await GenerateUniqueAccountNameAsync(connection, cancellationToken);
                    config.BotCharacterName = await GenerateUniqueCharacterNameAsync(connection, cancellationToken);
                }

                if (string.IsNullOrWhiteSpace(config.BotPassword))
                    config.BotPassword = GeneratePassword();
            }

            await SaveBotIdentityAsync(connection, config, cancellationToken);
            await ValidateConfiguredIdentityAsync(connection, config, cancellationToken);
            await ProvisionCharacterAsync(connection, config, cancellationToken);
            await ValidateConfiguredIdentityAsync(connection, config, cancellationToken);
            await UpsertSystemClientlessAsync(connection, config, cancellationToken);
            return config;
        }
        finally
        {
            ProvisionLock.Release();
        }
    }

    private static async Task ValidateConfiguredIdentityAsync(
        SqlConnection connection,
        HideAndSeekRuntimeConfig config,
        CancellationToken cancellationToken)
    {
        var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
        var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
        var error = await connection.ExecuteScalarAsync<string?>(new CommandDefinition($@"
DECLARE @AccountJID int =
(
    SELECT TOP (1) JID
    FROM {accountDb}.dbo.TB_User WITH (NOLOCK)
    WHERE StrUserID = @AccountName
);

IF @AccountJID IS NOT NULL
   AND NOT EXISTS
   (
       SELECT 1
       FROM {accountDb}.dbo.TB_User WITH (NOLOCK)
       WHERE JID = @AccountJID
         AND LOWER([password]) = LOWER(CONVERT(varchar(32), HASHBYTES('MD5', CONVERT(varchar(64), @Password)), 2))
   )
BEGIN
    SELECT CONVERT(nvarchar(512),
        N'Hide and Seek account password is incorrect for [' + @AccountName + N']. Update it in Events > Hide and Seek > Setup.');
    RETURN;
END;

DECLARE @CharacterOwnerJID int =
(
    SELECT TOP (1) U.UserJID
    FROM {shardDb}.dbo._Char C WITH (NOLOCK)
    INNER JOIN {shardDb}.dbo._User U WITH (NOLOCK) ON U.CharID = C.CharID
    WHERE C.CharName16 = @CharacterName
);

IF @CharacterOwnerJID IS NOT NULL
   AND (@AccountJID IS NULL OR @CharacterOwnerJID <> @AccountJID)
BEGIN
    SELECT CONVERT(nvarchar(512),
        N'Hide and Seek character [' + @CharacterName + N'] does not belong to account [' + @AccountName + N'].');
    RETURN;
END;

SELECT CONVERT(nvarchar(512), NULL);",
            new
            {
                AccountName = config.BotAccountName,
                Password = config.BotPassword,
                CharacterName = config.BotCharacterName
            },
            cancellationToken: cancellationToken));

        if (!string.IsNullOrWhiteSpace(error))
            throw new InvalidOperationException(error);
    }

    private static async Task<bool> IsConfiguredIdentityLinkedAsync(
        SqlConnection connection,
        HideAndSeekRuntimeConfig config,
        CancellationToken cancellationToken)
    {
        if (string.IsNullOrWhiteSpace(config.BotAccountName) ||
            string.IsNullOrWhiteSpace(config.BotCharacterName))
        {
            return false;
        }

        var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
        var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
        return await connection.ExecuteScalarAsync<bool>(new CommandDefinition($@"
IF NOT EXISTS
(
    SELECT 1 FROM {accountDb}.dbo.TB_User WITH (NOLOCK)
    WHERE StrUserID = @AccountName
)
AND NOT EXISTS
(
    SELECT 1 FROM {shardDb}.dbo._Char WITH (NOLOCK)
    WHERE CharName16 = @CharacterName
)
    SELECT CONVERT(bit, 1);
ELSE
    SELECT CONVERT(bit, CASE WHEN EXISTS
    (
        SELECT 1
        FROM {accountDb}.dbo.TB_User A WITH (NOLOCK)
        INNER JOIN {shardDb}.dbo._User U WITH (NOLOCK) ON U.UserJID = A.JID
        INNER JOIN {shardDb}.dbo._Char C WITH (NOLOCK) ON C.CharID = U.CharID
        WHERE A.StrUserID = @AccountName
          AND C.CharName16 = @CharacterName
    ) THEN 1 ELSE 0 END);",
            new
            {
                AccountName = config.BotAccountName,
                CharacterName = config.BotCharacterName
            },
            cancellationToken: cancellationToken));
    }

    private static async Task<int> ProvisionCharacterAsync(
        SqlConnection connection,
        HideAndSeekRuntimeConfig config,
        CancellationToken cancellationToken)
    {
        var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
        var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
        var sql = $@"
DECLARE @UserJID int = 0;
DECLARE @CharID int = 0;
DECLARE @RefCharID int = 1907;
DECLARE @InventorySize int = 109;

BEGIN TRY
    BEGIN TRANSACTION;

    IF NOT EXISTS
    (
        SELECT 1 FROM {accountDb}.dbo.TB_User WITH (UPDLOCK, HOLDLOCK)
        WHERE StrUserID = @AccountName
    )
        INSERT INTO {accountDb}.dbo.TB_User (StrUserID, [password], sec_content, sec_primary)
        VALUES (@AccountName, LOWER(CONVERT(varchar(32), HASHBYTES('MD5', CONVERT(varchar(64), @Password)), 2)), 3, 3);

    SELECT @UserJID = JID
    FROM {accountDb}.dbo.TB_User WITH (UPDLOCK, HOLDLOCK)
    WHERE StrUserID = @AccountName;

    IF ISNULL(@UserJID, 0) <= 0
        THROW 51000, 'Hide and Seek account JID could not be resolved.', 1;

    IF NOT EXISTS
    (
        SELECT 1 FROM {shardDb}.dbo._AccountJID WITH (UPDLOCK, HOLDLOCK)
        WHERE AccountID = @AccountName
    )
        INSERT INTO {shardDb}.dbo._AccountJID (AccountID, JID, Gold)
        VALUES (@AccountName, @UserJID, 0);

    SELECT @CharID = C.CharID
    FROM {shardDb}.dbo._Char C WITH (UPDLOCK, HOLDLOCK)
    WHERE C.CharName16 = @CharacterName;

    IF ISNULL(@CharID, 0) > 0
       AND NOT EXISTS
       (
           SELECT 1 FROM {shardDb}.dbo._User WITH (NOLOCK)
           WHERE UserJID = @UserJID AND CharID = @CharID
       )
        THROW 51001, 'Hide and Seek character belongs to a different account.', 1;

    IF ISNULL(@CharID, 0) <= 0
    BEGIN
        IF (SELECT COUNT(1) FROM {shardDb}.dbo._User WITH (NOLOCK) WHERE UserJID = @UserJID) >= 4
            THROW 51002, 'Hide and Seek account already has four characters.', 1;

        INSERT INTO {shardDb}.dbo._Char
        (
            RefObjID, CharName16, Scale, Strength, Intellect,
            LatestRegion, PosX, PosY, PosZ, AppointedTeleport,
            InventorySize, LastLogout, CurLevel, MaxLevel,
            RemainGold, RemainSkillPoint, HP, MP,
            JobLvl_Trader, JobLvl_Hunter, JobLvl_Robber, WorldID
        )
        VALUES
        (
            @RefCharID, @CharacterName, 68, 53, 53,
            @ReturnRegionID, @ReturnX, @ReturnY, @ReturnZ, 0,
            @InventorySize, GETDATE(), 1, 1,
            0, 0, 200, 200,
            1, 1, 1, @ReturnWorldID
        );

        SET @CharID = CONVERT(int, SCOPE_IDENTITY());

        INSERT INTO {shardDb}.dbo._User (UserJID, CharID)
        VALUES (@UserJID, @CharID);

        INSERT INTO {shardDb}.dbo._Inventory (CharID, Slot, ItemID)
        SELECT @CharID, cnt, 0
        FROM {shardDb}.dbo._RefDummySlot WITH (NOLOCK)
        WHERE cnt < @InventorySize;

        INSERT INTO {shardDb}.dbo._InventoryForAvatar (CharID, Slot, ItemID)
        SELECT @CharID, cnt, 0
        FROM {shardDb}.dbo._RefDummySlot WITH (NOLOCK)
        WHERE cnt < 5;

        IF OBJECT_ID(N'{_serverSettings.ShardDB}.dbo._CharSkillMastery', N'U') IS NOT NULL
            INSERT INTO {shardDb}.dbo._CharSkillMastery (CharID, MasteryID, Level)
            SELECT @CharID, MasteryID, 0
            FROM {shardDb}.dbo._RefCharDefault_SkillMastery WITH (NOLOCK)
            WHERE Race IN (0, 3);

        IF OBJECT_ID(N'{_serverSettings.ShardDB}.dbo._CharSkill', N'U') IS NOT NULL
            INSERT INTO {shardDb}.dbo._CharSkill (CharID, SkillID, Enable)
            SELECT @CharID, SkillID, 1
            FROM {shardDb}.dbo._RefCharDefault_Skill WITH (NOLOCK)
            WHERE Race IN (0, 3);

        IF OBJECT_ID(N'{_serverSettings.ShardDB}.dbo._StaticAvatar', N'U') IS NOT NULL
            INSERT INTO {shardDb}.dbo._StaticAvatar (CharID) VALUES (@CharID);

        IF OBJECT_ID(N'{_serverSettings.ShardDB}.dbo._CharTrijob', N'U') IS NOT NULL
            INSERT INTO {shardDb}.dbo._CharTrijob
                (CharID, JobType, Level, Exp, Contribution, Reward)
            VALUES
                (@CharID, 0, 1, 0, 0, 0);

        IF OBJECT_ID(N'{_serverSettings.ShardDB}.dbo._CharNameList', N'U') IS NOT NULL
            INSERT INTO {shardDb}.dbo._CharNameList (CharName16, CharID)
            VALUES (@CharacterName, @CharID);

        IF OBJECT_ID(N'{_serverSettings.ShardDB}.dbo._AddNewClientConfig', N'P') IS NOT NULL
            EXEC {shardDb}.dbo._AddNewClientConfig @CharID;
    END;

    INSERT INTO {shardDb}.dbo._Inventory (CharID, Slot, ItemID)
    SELECT @CharID, D.cnt, 0
    FROM {shardDb}.dbo._RefDummySlot D WITH (NOLOCK)
    WHERE D.cnt < @InventorySize
      AND NOT EXISTS
      (
          SELECT 1 FROM {shardDb}.dbo._Inventory I WITH (NOLOCK)
          WHERE I.CharID = @CharID AND I.Slot = D.cnt
      );

    INSERT INTO {shardDb}.dbo._InventoryForAvatar (CharID, Slot, ItemID)
    SELECT @CharID, D.cnt, 0
    FROM {shardDb}.dbo._RefDummySlot D WITH (NOLOCK)
    WHERE D.cnt < 5
      AND NOT EXISTS
      (
          SELECT 1 FROM {shardDb}.dbo._InventoryForAvatar I WITH (NOLOCK)
          WHERE I.CharID = @CharID AND I.Slot = D.cnt
      );

    IF OBJECT_ID(N'{_serverSettings.ShardDB}.dbo._CharSkillMastery', N'U') IS NOT NULL
        INSERT INTO {shardDb}.dbo._CharSkillMastery (CharID, MasteryID, Level)
        SELECT @CharID, D.MasteryID, 0
        FROM {shardDb}.dbo._RefCharDefault_SkillMastery D WITH (NOLOCK)
        WHERE D.Race IN (0, 3)
          AND NOT EXISTS
          (
              SELECT 1 FROM {shardDb}.dbo._CharSkillMastery M WITH (NOLOCK)
              WHERE M.CharID = @CharID AND M.MasteryID = D.MasteryID
          );

    IF OBJECT_ID(N'{_serverSettings.ShardDB}.dbo._CharSkill', N'U') IS NOT NULL
        INSERT INTO {shardDb}.dbo._CharSkill (CharID, SkillID, Enable)
        SELECT @CharID, D.SkillID, 1
        FROM {shardDb}.dbo._RefCharDefault_Skill D WITH (NOLOCK)
        WHERE D.Race IN (0, 3)
          AND NOT EXISTS
          (
              SELECT 1 FROM {shardDb}.dbo._CharSkill S WITH (NOLOCK)
              WHERE S.CharID = @CharID AND S.SkillID = D.SkillID
          );

    IF OBJECT_ID(N'{_serverSettings.ShardDB}.dbo._CharQuest', N'U') IS NOT NULL
        INSERT INTO {shardDb}.dbo._CharQuest
            (CharID, QuestID, Status, AchievementCount, StartTime, EndTime, QuestData1, QuestData2)
        SELECT @CharID, Q.ID, 1, 0, GETDATE(), GETDATE(), 0, 0
        FROM {shardDb}.dbo._RefQuest Q WITH (NOLOCK)
        WHERE Q.CodeName IN
        (
            SELECT CodeName
            FROM {shardDb}.dbo._RefCharDefault_Quest WITH (NOLOCK)
            WHERE Race IN (0, 3) AND RequiredLevel = 1 AND Service = 1
        )
          AND NOT EXISTS
          (
              SELECT 1 FROM {shardDb}.dbo._CharQuest CQ WITH (NOLOCK)
              WHERE CQ.CharID = @CharID AND CQ.QuestID = Q.ID
          );

    IF OBJECT_ID(N'{_serverSettings.ShardDB}.dbo._StaticAvatar', N'U') IS NOT NULL
       AND NOT EXISTS (SELECT 1 FROM {shardDb}.dbo._StaticAvatar WITH (NOLOCK) WHERE CharID = @CharID)
        INSERT INTO {shardDb}.dbo._StaticAvatar (CharID) VALUES (@CharID);

    IF OBJECT_ID(N'{_serverSettings.ShardDB}.dbo._CharTrijob', N'U') IS NOT NULL
       AND NOT EXISTS (SELECT 1 FROM {shardDb}.dbo._CharTrijob WITH (NOLOCK) WHERE CharID = @CharID)
        INSERT INTO {shardDb}.dbo._CharTrijob
            (CharID, JobType, Level, Exp, Contribution, Reward)
        VALUES
            (@CharID, 0, 1, 0, 0, 0);

    IF OBJECT_ID(N'{_serverSettings.ShardDB}.dbo._CharNameList', N'U') IS NOT NULL
       AND NOT EXISTS (SELECT 1 FROM {shardDb}.dbo._CharNameList WITH (NOLOCK) WHERE CharID = @CharID)
        INSERT INTO {shardDb}.dbo._CharNameList (CharName16, CharID)
        VALUES (@CharacterName, @CharID);

    IF NOT EXISTS (SELECT 1 FROM {accountDb}.dbo.SK_Silk WITH (UPDLOCK, HOLDLOCK) WHERE JID = @UserJID)
        INSERT INTO {accountDb}.dbo.SK_Silk (JID, silk_own, silk_gift, silk_point)
        VALUES (@UserJID, 0, 0, 0);

    COMMIT TRANSACTION;
END TRY
BEGIN CATCH
    IF @@TRANCOUNT > 0
        ROLLBACK TRANSACTION;
    THROW;
END CATCH;

SELECT @CharID;";

        return await connection.ExecuteScalarAsync<int>(new CommandDefinition(
            sql,
            new
            {
                AccountName = config.BotAccountName,
                Password = config.BotPassword,
                CharacterName = config.BotCharacterName,
                config.ReturnWorldID,
                config.ReturnRegionID,
                config.ReturnX,
                config.ReturnY,
                config.ReturnZ
            },
            commandTimeout: 120,
            cancellationToken: cancellationToken));
    }

    private static async Task SaveBotIdentityAsync(
        SqlConnection connection,
        HideAndSeekRuntimeConfig config,
        CancellationToken cancellationToken)
    {
        await connection.ExecuteAsync(new CommandDefinition(@"
UPDATE Events.dbo._HideAndSeekConfig
SET BotAccountName = @BotAccountName,
    BotPassword = @BotPassword,
    BotCharacterName = @BotCharacterName,
    UpdatedAtUtc = SYSUTCDATETIME()
WHERE EventCode = N'HNS';",
            new
            {
                config.BotAccountName,
                config.BotPassword,
                config.BotCharacterName
            },
            cancellationToken: cancellationToken));
    }

    private static async Task UpsertSystemClientlessAsync(
        SqlConnection connection,
        HideAndSeekRuntimeConfig config,
        CancellationToken cancellationToken)
    {
        await connection.ExecuteAsync(new CommandDefinition(@"
IF EXISTS (SELECT 1 FROM dbo.Clientless_Accounts WITH (UPDLOCK, HOLDLOCK) WHERE SystemRole = @SystemRole)
BEGIN
    UPDATE dbo.Clientless_Accounts
    SET Enabled = 1,
        Locale = @Locale,
        ShardID = @ShardID,
        AccountName = @AccountName,
        AccountPassword = @AccountPassword,
        CharacterName = @CharacterName,
        AgentAuthMode = 'Auto',
        AgentAuthDelayMs = 1500,
        AgentAuthPaddingHex = '',
        LaunchDelayMs = 0,
        ReconnectDelaySeconds = 10,
        LastStatus = 'Pending',
        LastMessage = NULL,
        UpdatedAt = SYSDATETIME()
    WHERE SystemRole = @SystemRole;
END
ELSE
BEGIN
    INSERT INTO dbo.Clientless_Accounts
        (Enabled, Locale, ShardID, AccountName, AccountPassword, CharacterName,
         AgentAuthMode, AgentAuthDelayMs, AgentAuthPaddingHex, LaunchDelayMs,
         ReconnectDelaySeconds, LastStatus, SystemRole)
    VALUES
        (1, @Locale, @ShardID, @AccountName, @AccountPassword, @CharacterName,
         'Auto', 1500, '', 0, 10, 'Pending', @SystemRole);
END;",
            new
            {
                SystemRole,
                Locale = config.BotLocale,
                ShardID = config.BotShardID,
                AccountName = config.BotAccountName,
                AccountPassword = config.BotPassword,
                CharacterName = config.BotCharacterName
            },
            cancellationToken: cancellationToken));
    }

    private static async Task<string> GenerateUniqueAccountNameAsync(
        SqlConnection connection,
        CancellationToken cancellationToken)
    {
        var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
        for (var attempt = 0; attempt < 100; attempt++)
        {
            var candidate = "kmthns" + Convert.ToHexString(RandomNumberGenerator.GetBytes(4)).ToLowerInvariant();
            var exists = await connection.ExecuteScalarAsync<int>(new CommandDefinition(
                $"SELECT COUNT(1) FROM {accountDb}.dbo.TB_User WITH (NOLOCK) WHERE StrUserID = @Candidate;",
                new { Candidate = candidate },
                cancellationToken: cancellationToken));
            if (exists == 0)
                return candidate;
        }

        throw new InvalidOperationException("Could not reserve a unique Hide and Seek account name.");
    }

    private static async Task<string> GenerateUniqueCharacterNameAsync(
        SqlConnection connection,
        CancellationToken cancellationToken)
    {
        var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
        for (var attempt = 0; attempt < 100; attempt++)
        {
            var candidate = "KMT_Hide" + Convert.ToHexString(RandomNumberGenerator.GetBytes(3));
            var exists = await connection.ExecuteScalarAsync<int>(new CommandDefinition(
                $"SELECT COUNT(1) FROM {shardDb}.dbo._Char WITH (NOLOCK) WHERE CharName16 = @Candidate;",
                new { Candidate = candidate },
                cancellationToken: cancellationToken));
            if (exists == 0)
                return candidate;
        }

        throw new InvalidOperationException("Could not reserve a unique Hide and Seek character name.");
    }

    private static string GeneratePassword()
    {
        return Convert.ToHexString(RandomNumberGenerator.GetBytes(16)).ToLowerInvariant();
    }

    private static async Task<HideAndSeekLocation> PickLocationAsync(CancellationToken cancellationToken)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(cancellationToken);
        var locations = (await connection.QueryAsync<HideAndSeekLocation>(new CommandDefinition(@"
SELECT LocationID, LocationName, WorldID, RegionID, PosX, PosY, PosZ, Weight, LastUsedAtUtc
FROM Events.dbo._HideAndSeekLocation WITH (NOLOCK)
WHERE IsActive = 1
  AND WorldID > 0
  AND RegionID > 0
ORDER BY LocationID;",
            cancellationToken: cancellationToken))).ToList();

        if (locations.Count == 0)
            throw new InvalidOperationException("Hide and Seek has no active locations.");

        var leastRecent = locations
            .OrderBy(location => location.LastUsedAtUtc ?? DateTime.MinValue)
            .Take(Math.Max(1, (locations.Count + 1) / 2))
            .ToList();
        var totalWeight = leastRecent.Sum(location => Math.Clamp(location.Weight, 1, 1000));
        var roll = Random.Shared.Next(1, totalWeight + 1);
        var selected = leastRecent[0];
        foreach (var location in leastRecent)
        {
            roll -= Math.Clamp(location.Weight, 1, 1000);
            if (roll <= 0)
            {
                selected = location;
                break;
            }
        }

        await connection.ExecuteAsync(new CommandDefinition(@"
UPDATE Events.dbo._HideAndSeekLocation
SET LastUsedAtUtc = SYSUTCDATETIME(),
    UpdatedAtUtc = SYSUTCDATETIME()
WHERE LocationID = @LocationID;",
            new { selected.LocationID },
            cancellationToken: cancellationToken));
        return selected;
    }

    private static async Task<HideAndSeekRunIds> CreateRunAsync(
        long autoRunId,
        string displayName,
        HideAndSeekLocation location,
        ISession bot,
        CancellationToken cancellationToken)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(cancellationToken);
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync(cancellationToken);
        var roundId = await connection.ExecuteScalarAsync<long>(new CommandDefinition(@"
INSERT Events.dbo._AutoEventRound(RunID,RoundNo,Prompt,AnswerMasked,Status,StartedAtUtc)
VALUES(@RunID,1,@Prompt,N'exchange',N'Running',SYSUTCDATETIME());
SELECT CONVERT(bigint,SCOPE_IDENTITY());",
            new { RunID = autoRunId, Prompt = Trim($"{displayName}: {location.LocationName}", 512) },
            transaction,
            cancellationToken: cancellationToken));
        var hnsRunId = await connection.ExecuteScalarAsync<long>(new CommandDefinition(@"
INSERT INTO Events.dbo._HideAndSeekRun
    (AutoRunID, LocationID, LocationName, WorldID, RegionID, PosX, PosY, PosZ,
     BotCharID, BotCharacterName, Status, EndsAtUtc)
VALUES
    (@AutoRunID, @LocationID, @LocationName, @WorldID, @RegionID, @PosX, @PosY, @PosZ,
      @BotCharID, @BotCharacterName, N'Positioning', SYSUTCDATETIME());
SELECT CONVERT(bigint, SCOPE_IDENTITY());",
            new
            {
                AutoRunID = autoRunId,
                location.LocationID,
                location.LocationName,
                location.WorldID,
                location.RegionID,
                location.PosX,
                location.PosY,
                location.PosZ,
                BotCharID = bot.SessionData.Charid,
                BotCharacterName = bot.SessionData.Charname
            },
            transaction,
            cancellationToken: cancellationToken));
        await transaction.CommitAsync(cancellationToken);
        return new HideAndSeekRunIds(hnsRunId, roundId);
    }

    private static async Task MarkSearchingAsync(long hnsRunId, DateTime endsAtUtc, CancellationToken cancellationToken)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(cancellationToken);
        await connection.ExecuteAsync(new CommandDefinition(@"
UPDATE Events.dbo._HideAndSeekRun
SET Status = N'Searching', EndsAtUtc = @EndsAtUtc
WHERE HnsRunID = @HnsRunID
  AND Status = N'Positioning';",
            new { HnsRunID = hnsRunId, EndsAtUtc = endsAtUtc },
            cancellationToken: cancellationToken));
    }

    private static async Task<bool> ClaimWinnerAsync(long hnsRunId, ISession player)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        var affected = await connection.ExecuteAsync(@"
UPDATE Events.dbo._HideAndSeekRun WITH (UPDLOCK, SERIALIZABLE)
SET WinnerCharID = @WinnerCharID,
    WinnerCharName = @WinnerCharName,
    WinnerJID = @WinnerJID,
    WinnerHwid = NULLIF(@WinnerHwid, ''),
    WinnerIP = NULLIF(@WinnerIP, ''),
    Status = N'Claimed'
WHERE HnsRunID = @HnsRunID
  AND Status = N'Searching'
  AND WinnerCharID IS NULL;",
            new
            {
                HnsRunID = hnsRunId,
                WinnerCharID = player.SessionData.Charid,
                WinnerCharName = Trim(player.SessionData.Charname, 64),
                WinnerJID = player.SessionData.JID,
                WinnerHwid = Trim(player.SessionData.Hwid, 128),
                WinnerIP = Trim(player.ClientIp, 64)
            });
        return affected == 1;
    }

    private static async Task CompleteRunAsync(
        ActiveHideAndSeekRun active,
        string status,
        string? rewardSummary,
        string message,
        CancellationToken cancellationToken)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(cancellationToken);
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync(cancellationToken);
        await connection.ExecuteAsync(new CommandDefinition(@"
UPDATE Events.dbo._HideAndSeekRun
SET Status = @Status,
    RewardSummary = @RewardSummary,
    Message = @Message,
    EndedAtUtc = SYSUTCDATETIME()
WHERE HnsRunID = @HnsRunID AND EndedAtUtc IS NULL;

UPDATE r
SET r.Status = @Status,
    r.WinnerCharID = hns.WinnerCharID,
    r.WinnerCharName = hns.WinnerCharName,
    r.EndedAtUtc = SYSUTCDATETIME()
FROM Events.dbo._AutoEventRound r
JOIN Events.dbo._HideAndSeekRun hns ON hns.HnsRunID = @HnsRunID
WHERE r.RoundID = @RoundID AND r.EndedAtUtc IS NULL;

IF @Status = N'Won' AND NOT EXISTS
   (SELECT 1 FROM Events.dbo._AutoEventWinnerLog WITH (UPDLOCK, HOLDLOCK) WHERE RoundID=@RoundID)
BEGIN
    INSERT Events.dbo._AutoEventWinnerLog
        (RunID,RoundID,EventCode,RoundNo,CharID,CharName,JID,Hwid,ClientIP,Answer,WonAtUtc,RewardSummary)
    SELECT @AutoRunID,@RoundID,N'HNS',1,WinnerCharID,WinnerCharName,WinnerJID,
           ISNULL(WinnerHwid,''),ISNULL(WinnerIP,''),@Answer,SYSUTCDATETIME(),@RewardSummary
    FROM Events.dbo._HideAndSeekRun
    WHERE HnsRunID=@HnsRunID AND WinnerCharID IS NOT NULL;
END;",
            new
            {
                active.HnsRunID,
                active.RoundID,
                active.AutoRunID,
                Status = Trim(status, 24),
                RewardSummary = Trim(rewardSummary, 512),
                Message = Trim(message, 512),
                Answer = Trim($"Location={active.Location.LocationName};Exchange", 256)
            },
            transaction,
            cancellationToken: cancellationToken));
        await transaction.CommitAsync(cancellationToken);
    }

    private static async Task TryCompleteRunAsync(
        ActiveHideAndSeekRun active,
        string status,
        string? rewardSummary,
        string message)
    {
        try
        {
            await CompleteRunAsync(active, status, rewardSummary, message, CancellationToken.None);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Hide and Seek run finalization failed for HnsRunID={HnsRunID}", active.HnsRunID);
        }
    }

    private static async Task<ISession?> WaitForWinnerAsync(
        ActiveHideAndSeekRun active,
        DateTime endsAtUtc,
        CancellationToken cancellationToken)
    {
        var nextReminderUtc = DateTime.UtcNow.AddSeconds(active.Config.ReminderIntervalSeconds);
        while (DateTime.UtcNow < endsAtUtc)
        {
            cancellationToken.ThrowIfCancellationRequested();
            if (active.Winner.Task.IsCompleted)
                return await active.Winner.Task;

            var bot = FindOnlineBot(active.Config.BotCharacterName);
            if (bot == null)
                throw new InvalidOperationException("Hide and Seek character disconnected during the event.");

            Interlocked.Exchange(ref _botUniqueId, bot.SessionData.UniqueCharId);
            if (DateTime.UtcNow >= nextReminderUtc &&
                active.Config.ReminderIntervalSeconds > 0)
            {
                var remaining = Math.Max(1, (int)Math.Ceiling((endsAtUtc - DateTime.UtcNow).TotalSeconds));
                await BroadcastNoticeAsync(
                    PlayerLanguage.Get(
                        "HideAndSeek.Reminder",
                        active.Config.BotCharacterName,
                        active.Location.LocationName,
                        PlayerLanguage.FormatDurationSeconds(remaining)),
                    NoticeType.NOTICE);
                nextReminderUtc = DateTime.UtcNow.AddSeconds(active.Config.ReminderIntervalSeconds);
            }

            await Task.Delay(250, cancellationToken);
        }

        return active.Winner.Task.IsCompleted ? await active.Winner.Task : null;
    }

    private static async Task<string> ApplyRewardsAsync(ISession winner)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        var rewards = (await connection.QueryAsync<HideAndSeekReward>(@"
SELECT RewardType, Amount, ItemCodeName128, ItemID, ItemCount, Plus
FROM Events.dbo._HideAndSeekReward WITH (NOLOCK)
WHERE IsActive = 1
ORDER BY RewardID;")).ToList();

        if (rewards.Count == 0)
            return PlayerLanguage.Get("Common.NoRewardConfigured");

        var summaries = new List<string>();
        foreach (var reward in rewards)
        {
            try
            {
                summaries.Add(await AutoEventService.ApplyRewardCoreAsync(
                    winner,
                    reward.RewardType,
                    reward.Amount,
                    reward.ItemCodeName128,
                    reward.ItemID,
                    reward.ItemCount,
                    reward.Plus,
                    "HideAndSeek"));
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Hide and Seek reward failed for CharID={CharID}, RewardType={RewardType}",
                    winner.SessionData.Charid,
                    reward.RewardType);
                summaries.Add(PlayerLanguage.Get("Reward.DeliveryFailed"));
            }
        }

        return string.Join(", ", summaries.Where(summary => !string.IsNullOrWhiteSpace(summary)));
    }

    private static async Task WaitForBotLocationAsync(
        ISession bot,
        HideAndSeekLocation location,
        CancellationToken cancellationToken)
    {
        var deadline = DateTime.UtcNow.Add(BotPositioningTimeout);
        while (DateTime.UtcNow < deadline)
        {
            cancellationToken.ThrowIfCancellationRequested();
            if (!ServerManager.IsOnlinePlayer(bot) || !bot.IsManagedClientless)
            {
                throw new InvalidOperationException(
                    "Hide and Seek character disconnected while moving to the selected location.");
            }

            if (bot.SessionData.WorldID == location.WorldID &&
                bot.SessionData.LatestRegion == location.RegionID)
            {
                return;
            }
            await Task.Delay(250, cancellationToken);
        }

        throw new TimeoutException(
            $"Hide and Seek character did not reach WorldID {location.WorldID}, RegionID {location.RegionID}. " +
            $"Current WorldID={bot.SessionData.WorldID}, RegionID={bot.SessionData.LatestRegion}, " +
            $"GameReady={bot.CharacterGameReady}.");
    }

    private static async Task EnsureBotVisibleAsync(
        ISession bot,
        CancellationToken cancellationToken)
    {
        // GM characters can enter the world in GM Invisible state. The command
        // is a toggle, so converge carefully instead of sending it blindly:
        // a known visible state needs no command, while an unknown initial state
        // may need one extra toggle if the first update proves it was visible.
        for (var attempt = 1; attempt <= 2; attempt++)
        {
            cancellationToken.ThrowIfCancellationRequested();
            if (!ServerManager.IsOnlinePlayer(bot) || !bot.IsManagedClientless)
            {
                throw new InvalidOperationException(
                    "Hide and Seek character disconnected while clearing GM invisibility.");
            }

            var previousState = bot.SessionData.State.BodyState;
            if (previousState.HasValue && previousState.Value != BodyState.GameMasterInvisible)
                return;

            await bot.SendToServer(CreateInvisibleTogglePacket());

            if (!await WaitForBotBodyStateChangeAsync(bot, previousState, cancellationToken))
            {
                throw new TimeoutException(
                    $"Hide and Seek character did not confirm GM invisibility removal. " +
                    $"Current BodyState={FormatBodyState(bot.SessionData.State.BodyState)}.");
            }

            var currentState = bot.SessionData.State.BodyState;
            if (currentState.HasValue && currentState.Value != BodyState.GameMasterInvisible)
            {
                Log.Information(
                    "Hide and Seek character {Character} is visible after positioning. BodyState={BodyState}",
                    bot.SessionData.Charname,
                    currentState.Value);
                return;
            }

            if (attempt < 2)
            {
                Log.Debug(
                    "Hide and Seek character {Character} reported GM Invisible after the first visibility toggle; retrying once to converge to visible state.",
                    bot.SessionData.Charname);
            }
        }

        throw new InvalidOperationException(
            $"Hide and Seek character remained GM Invisible after positioning. " +
            $"Current BodyState={FormatBodyState(bot.SessionData.State.BodyState)}.");
    }

    private static async Task<bool> WaitForBotBodyStateChangeAsync(
        ISession bot,
        BodyState? previousState,
        CancellationToken cancellationToken)
    {
        var deadline = DateTime.UtcNow.Add(BotVisibilityChangeTimeout);
        while (DateTime.UtcNow < deadline)
        {
            cancellationToken.ThrowIfCancellationRequested();
            if (!ServerManager.IsOnlinePlayer(bot) || !bot.IsManagedClientless)
            {
                throw new InvalidOperationException(
                    "Hide and Seek character disconnected while waiting for its visibility state.");
            }

            if (bot.SessionData.State.BodyState != previousState)
                return true;

            await Task.Delay(100, cancellationToken);
        }

        return false;
    }

    internal static Packet CreateInvisibleTogglePacket()
    {
        var packet = new Packet(OperatorCommandOpcode, true, false);
        packet.WriteUInt16(InvisibleOperatorCommand);
        return packet;
    }

    private static string FormatBodyState(BodyState? state)
    {
        return state.HasValue ? state.Value.ToString() : "Unknown";
    }

    private static async Task<ISession?> WaitForOnlineBotAsync(
        string characterName,
        TimeSpan timeout,
        CancellationToken cancellationToken)
    {
        var deadline = DateTime.UtcNow.Add(timeout);
        while (DateTime.UtcNow < deadline)
        {
            cancellationToken.ThrowIfCancellationRequested();
            var session = FindOnlineBot(characterName);
            if (session != null)
            {
                RememberOnlineBot(characterName);
                return session;
            }

            await Task.Delay(500, cancellationToken);
        }

        return null;
    }

    private static ISession? FindOnlineBot(string characterName)
    {
        return ServerManager.AgentSessions.FindByCharName(
            characterName,
            session =>
                ServerManager.IsOnlinePlayer(session) &&
                session.IsManagedClientless);
    }

    private static void RememberOnlineBot(string characterName)
    {
        var bot = FindOnlineBot(characterName);
        if (bot == null)
            return;

        _botCharacterName = characterName;
        Interlocked.Exchange(ref _botUniqueId, bot.SessionData.UniqueCharId);
    }

    private static bool IsWithinHwidLimit(ISession player, int limit)
    {
        if (limit <= 0)
            return true;
        if (string.IsNullOrWhiteSpace(player.SessionData.Hwid))
            return true;

        var onlineForHwid = ServerManager.AgentSessions.CountWhere(session =>
                ServerManager.IsOnlinePlayer(session) &&
                !session.IsManagedClientless &&
                string.Equals(
                    session.SessionData.Hwid,
                    player.SessionData.Hwid,
                    StringComparison.OrdinalIgnoreCase));
        return onlineForHwid <= limit;
    }

    private static async Task BroadcastNoticeAsync(string message, NoticeType noticeType)
    {
        var packet = new Packet(0x168A);
        packet.WriteUInt8(noticeType);
        packet.WriteUnicode(PlayerLanguage.Get("HideAndSeek.Prefix") + message);
        await ServerManager.BroadcastPacket(packet);
    }

    private static async Task TryBroadcastNoticeAsync(string message, NoticeType noticeType)
    {
        try
        {
            await BroadcastNoticeAsync(message, noticeType);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Hide and Seek terminal notice broadcast failed");
        }
    }

    private static string Trim(string? value, int maxLength)
    {
        value = (value ?? string.Empty).Trim();
        return value.Length <= maxLength ? value : value[..maxLength];
    }

    private sealed class ActiveHideAndSeekRun
    {
        public ActiveHideAndSeekRun(
            long hnsRunId,
            long roundId,
            long autoRunId,
            HideAndSeekRuntimeConfig config,
            HideAndSeekLocation location,
            int botCharId)
        {
            HnsRunID = hnsRunId;
            RoundID = roundId;
            AutoRunID = autoRunId;
            Config = config;
            Location = location;
            BotCharID = botCharId;
        }

        public long HnsRunID { get; }
        public long RoundID { get; }
        public long AutoRunID { get; }
        public HideAndSeekRuntimeConfig Config { get; }
        public HideAndSeekLocation Location { get; }
        public int BotCharID { get; }
        public SemaphoreSlim WinnerLock { get; } = new(1, 1);
        public TaskCompletionSource<ISession> Winner { get; } =
            new(TaskCreationOptions.RunContinuationsAsynchronously);
    }

    private sealed record HideAndSeekRunIds(long HnsRunID, long RoundID);

    private sealed class HideAndSeekLocation
    {
        public int LocationID { get; set; }
        public string LocationName { get; set; } = string.Empty;
        public int WorldID { get; set; }
        public int RegionID { get; set; }
        public int PosX { get; set; }
        public int PosY { get; set; }
        public int PosZ { get; set; }
        public int Weight { get; set; } = 1;
        public DateTime? LastUsedAtUtc { get; set; }
    }

    private sealed class HideAndSeekReward
    {
        public string RewardType { get; set; } = string.Empty;
        public long Amount { get; set; }
        public string? ItemCodeName128 { get; set; }
        public int? ItemID { get; set; }
        public int ItemCount { get; set; } = 1;
        public int Plus { get; set; }
    }

    private sealed class SystemClientlessRow
    {
        public string AccountName { get; set; } = string.Empty;
        public string AccountPassword { get; set; } = string.Empty;
        public string CharacterName { get; set; } = string.Empty;
    }

    private sealed record PendingExchangeAttempt(
        long HnsRunID,
        uint TargetUniqueID,
        DateTime ExpiresAtUtc);
}
