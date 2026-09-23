using System.Collections.Concurrent;
using System.Net;
using System.Text;
using System.Text.Json;
using Dapper;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.RuntimeContract;
using Microsoft.Data.SqlClient;
using Serilog;

namespace KMTGuard.Features.Telegram;

public static class TelegramNotificationService
{
    private const int MaxAttempts = 8;
    private static readonly HttpClient HttpClient = new()
    {
        Timeout = TimeSpan.FromSeconds(12)
    };
    private static readonly SemaphoreSlim LifecycleLock = new(1, 1);
    private static readonly SemaphoreSlim SettingsLock = new(1, 1);
    private static readonly ConcurrentDictionary<string, DateTime> RecentEvents = new(StringComparer.Ordinal);
    // Retained for binary compatibility with older integrations. Unique
    // notifications are now queued by dbo.Telegram_Notification instead of
    // packet observers.
    private static readonly ConcurrentDictionary<int, byte> UnresolvedNamesLogged = new();
    private static readonly ConcurrentDictionary<int, EntitySpawnIdentity> EntitySpawns = new();
    private static IReadOnlySet<int> _uniqueRefObjIds = new HashSet<int>();
    private static readonly IReadOnlyDictionary<string, string> CanonicalUniqueNames =
        new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

    private static CancellationTokenSource? _stopSource;
    private static Task? _worker;
    private static TelegramRuntimeSettings _settings = TelegramRuntimeSettings.Disabled;
    private static DateTime _settingsLoadedAtUtc = DateTime.MinValue;

    public static async Task InitializeAsync()
    {
        await LifecycleLock.WaitAsync();
        try
        {
            if (_worker is { IsCompleted: false })
                return;

            await EnsureSchemaAsync();
            await LoadSettingsAsync(force: true);
            await ResetAbandonedMessagesAsync();
            _stopSource = new CancellationTokenSource();
            _worker = Task.Run(() => RunWorkerAsync(_stopSource.Token));
            Log.Information("Telegram notification service initialized");
        }
        finally
        {
            LifecycleLock.Release();
        }
    }

    public static void Stop() => StopAsync().GetAwaiter().GetResult();

    public static async Task StopAsync()
    {
        await LifecycleLock.WaitAsync();
        try
        {
            if (_stopSource == null)
                return;

            _stopSource.Cancel();
            try
            {
                if (_worker != null)
                    await _worker;
            }
            catch (OperationCanceledException)
            {
            }

            _stopSource.Dispose();
            _stopSource = null;
            _worker = null;
            _settings = TelegramRuntimeSettings.Disabled;
        }
        finally
        {
            LifecycleLock.Release();
        }
    }

    public static async Task QueueServerOnlineAsync()
    {
        var settings = await GetSettingsAsync();
        if (!settings.Enabled || !settings.ServerOnlineEnabled)
            return;

        var serverName = DisplayServerName();
        await EnqueueAsync(
            $"server-online:{Environment.ProcessId}:{DateTime.UtcNow:yyyyMMddHHmm}",
            "ServerStatus",
            $"<b>🟢 {Escape(serverName)} IS ONLINE</b>\n\n" +
            "The server is now available and ready to welcome all players.\n" +
            "Log in, reunite with your allies, and continue your adventure.");
    }

    public static void ObserveEntitySpawn(int worldObjectId, int refObjId)
    {
        if (worldObjectId <= 0 || refObjId <= 0 || !_uniqueRefObjIds.Contains(refObjId))
            return;

        var now = DateTime.UtcNow;
        EntitySpawns[worldObjectId] = new EntitySpawnIdentity(refObjId, now);
        if (EntitySpawns.Count <= 50_000)
            return;

        foreach (var stale in EntitySpawns
                     .Where(item => now - item.Value.ObservedAtUtc > TimeSpan.FromMinutes(10))
                     .Take(10_000))
        {
            EntitySpawns.TryRemove(stale.Key, out _);
        }
    }

    public static void ForgetEntitySpawn(int worldObjectId)
    {
        if (worldObjectId > 0)
            EntitySpawns.TryRemove(worldObjectId, out _);
    }

    public static void ObserveEntitySpawnCandidates(byte[] payload)
    {
        if (payload.Length < 8 || _uniqueRefObjIds.Count == 0)
            return;

        // Group-spawn data is a packed sequence and can contain several
        // entity records. We only inspect offsets that match a known Rarity=3
        // RefObjID, then use the following DWORD as its transient ObjectID.
        for (var offset = 0; offset <= payload.Length - 8; offset++)
        {
            var refObjId = BitConverter.ToInt32(payload, offset);
            if (!_uniqueRefObjIds.Contains(refObjId))
                continue;

            var worldObjectId = BitConverter.ToInt32(payload, offset + 4);
            if (worldObjectId > 0)
                ObserveEntitySpawn(worldObjectId, refObjId);
        }
    }

    public static async Task QueueUniqueSpawnAsync(int worldObjectId, int? modelRefObjId = null)
    {
        var settings = await GetSettingsAsync();
        if (!settings.Enabled || !settings.UniqueSpawnEnabled || worldObjectId <= 0)
            return;

        if (!TryAcquireRecent($"spawn-object:{worldObjectId}", TimeSpan.FromMinutes(5)))
            return;

        var refObjId = modelRefObjId is > 0 && _uniqueRefObjIds.Contains(modelRefObjId.Value)
            ? modelRefObjId
            : await ResolveSpawnRefObjIdAsync(worldObjectId);
        if (refObjId == null)
        {
            Log.Warning(
                "Telegram skipped Unique spawn ObjectID {ObjectID} because its RefObjID was not observed in entity spawn data",
                worldObjectId);
            return;
        }

        var displayName = await ResolveUniqueDisplayNameAsync(refObjId.Value);
        if (displayName == null)
            return;

        await EnqueueAsync(
            $"unique-spawn:{worldObjectId}",
            "UniqueSpawn",
            "<b>🔥 UNIQUE SPAWN ALERT</b>\n\n" +
            $"<b>{Escape(displayName)}</b> has entered the battlefield.\n" +
            "Gather your party, prepare your strongest build, and join the hunt before it is too late.");
    }

    public static async Task QueueUniqueKillAsync(int mobId, string? killerName)
    {
        var settings = await GetSettingsAsync();
        if (!settings.Enabled || !settings.UniqueKillEnabled || mobId <= 0)
            return;

        if (!TryAcquireRecent($"kill:{mobId}", TimeSpan.FromSeconds(20)))
            return;

        var displayName = await ResolveUniqueDisplayNameAsync(mobId);
        if (displayName == null)
            return;

        var killerLine = settings.ShowKillerName && !string.IsNullOrWhiteSpace(killerName)
            ? $"\nThe final blow was delivered by <b>{Escape(killerName.Trim())}</b>."
            : string.Empty;

        await EnqueueAsync(
            $"unique-kill:{mobId}:{DateTimeOffset.UtcNow.ToUnixTimeSeconds() / 20}",
            "UniqueKill",
            "<b>🏆 UNIQUE DEFEATED</b>\n\n" +
            $"<b>{Escape(displayName)}</b> has been defeated.{killerLine}\n" +
            "Congratulations to every warrior who took part in the battle.");
    }

    public static async Task QueueEventRemindersIfDueAsync(
        string eventCode,
        string displayName,
        string scheduleIdentity,
        DateTime scheduledAtLocal,
        DateTime nowLocal,
        CancellationToken cancellationToken = default)
    {
        var settings = await GetSettingsAsync();
        if (!settings.Enabled || !settings.EventReminderEnabled)
            return;

        foreach (var minutes in settings.ReminderMinutes)
        {
            var reminderAt = scheduledAtLocal.AddMinutes(-minutes);
            if (nowLocal < reminderAt || nowLocal >= reminderAt.AddMinutes(1))
                continue;

            var eventKey =
                $"event-reminder:{NormalizeKey(eventCode)}:{NormalizeKey(scheduleIdentity)}:{scheduledAtLocal:yyyyMMddHHmm}:{minutes}";
            await EnqueueAsync(
                eventKey,
                "EventReminder",
                "<b>⏰ EVENT REMINDER</b>\n\n" +
                $"<b>{Escape(CleanDisplayName(displayName, eventCode))}</b> begins in <b>{minutes} minute{(minutes == 1 ? string.Empty : "s")}</b>.\n" +
                "Prepare your equipment, organize your strategy, and be ready when registration opens.",
                cancellationToken);
        }
    }

    public static async Task QueueEventStartedAsync(long runId, string eventCode, string displayName)
    {
        var settings = await GetSettingsAsync();
        if (!settings.Enabled || !settings.EventStartedEnabled)
            return;

        await EnqueueAsync(
            $"event-started:{runId}",
            "EventStarted",
            "<b>⚔️ EVENT NOW LIVE</b>\n\n" +
            $"<b>{Escape(CleanDisplayName(displayName, eventCode))}</b> has officially started.\n" +
            "Enter the game, follow the event instructions, and compete for victory.");
    }

    public static async Task QueueEventFinishedAsync(
        long runId,
        string eventCode,
        string displayName,
        string status)
    {
        var settings = await GetSettingsAsync();
        if (!settings.Enabled || !settings.EventFinishedEnabled)
            return;

        var normalizedStatus = status.Equals("Completed", StringComparison.OrdinalIgnoreCase)
            ? "has concluded successfully"
            : status.Equals("Stopped", StringComparison.OrdinalIgnoreCase)
                ? "has been stopped"
                : "has concluded";

        await EnqueueAsync(
            $"event-finished:{runId}:{NormalizeKey(status)}",
            "EventFinished",
            "<b>🏁 EVENT CONCLUDED</b>\n\n" +
            $"<b>{Escape(CleanDisplayName(displayName, eventCode))}</b> {normalizedStatus}.\n" +
            "Thank you to every participant who joined the competition.");
    }

    public static async Task QueueFortressUpdateAsync(byte updateType, int fortressId = 0, string? guildName = null)
    {
        var settings = await GetSettingsAsync();
        if (!settings.Enabled || !settings.FortressWarEnabled)
            return;

        string? eventKeySuffix = null;
        string? message = null;
        switch (updateType)
        {
            case 1:
                eventKeySuffix = "begin-30";
                message = "<b>🏰 FORTRESS WAR APPROACHES</b>\n\n" +
                          "Fortress War begins in <b>30 minutes</b>.\n" +
                          "Guild leaders, assemble your forces and finalize your battle plans.";
                break;
            case 2:
                eventKeySuffix = "begin";
                message = "<b>⚔️ FORTRESS WAR HAS BEGUN</b>\n\n" +
                          "The gates are open and the battle for supremacy is underway.\n" +
                          "Fight with honor and lead your guild to victory.";
                break;
            case 6:
                eventKeySuffix = "ended";
                message = "<b>🏁 FORTRESS WAR HAS ENDED</b>\n\n" +
                          "The battlefield has fallen silent and this Fortress War is now over.\n" +
                          "Congratulations to the victorious guilds and every warrior who participated.";
                break;
            case 8 when !string.IsNullOrWhiteSpace(guildName):
                eventKeySuffix = $"occupied-{fortressId}-{NormalizeKey(guildName)}";
                message = "<b>👑 FORTRESS CONQUERED</b>\n\n" +
                          $"<b>{Escape(guildName.Trim())}</b> has conquered <b>{Escape(FortressName(fortressId))}</b>.\n" +
                          "A new chapter of guild supremacy has begun.";
                break;
        }

        if (message == null || eventKeySuffix == null ||
            !TryAcquireRecent($"fortress:{eventKeySuffix}", TimeSpan.FromMinutes(2)))
        {
            return;
        }

        await EnqueueAsync(
            $"fortress:{eventKeySuffix}:{DateTime.UtcNow:yyyyMMdd}",
            "FortressWar",
            message);
    }

    private static async Task RunWorkerAsync(CancellationToken cancellationToken)
    {
        while (!cancellationToken.IsCancellationRequested)
        {
            try
            {
                var settings = await GetSettingsAsync();
                if (!settings.Enabled || string.IsNullOrWhiteSpace(settings.BotToken) ||
                    string.IsNullOrWhiteSpace(settings.ChannelId))
                {
                    await Task.Delay(TimeSpan.FromSeconds(3), cancellationToken);
                    continue;
                }

                var notification = await ClaimNextAsync(cancellationToken);
                if (notification == null)
                {
                    await Task.Delay(TimeSpan.FromSeconds(2), cancellationToken);
                    continue;
                }

                await DeliverAsync(notification, settings, cancellationToken);
            }
            catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
            {
                return;
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Telegram notification worker iteration failed");
                try
                {
                    await Task.Delay(TimeSpan.FromSeconds(3), cancellationToken);
                }
                catch (OperationCanceledException)
                {
                    return;
                }
            }
        }
    }

    private static async Task DeliverAsync(
        TelegramQueueItem notification,
        TelegramRuntimeSettings settings,
        CancellationToken cancellationToken)
    {
        try
        {
            var endpoint = $"https://api.telegram.org/bot{settings.BotToken}/sendMessage";
            var payload = JsonSerializer.Serialize(new
            {
                chat_id = settings.ChannelId,
                text = notification.MessageHtml,
                parse_mode = "HTML",
                disable_web_page_preview = true
            });
            using var request = new HttpRequestMessage(HttpMethod.Post, endpoint)
            {
                Content = new StringContent(payload, Encoding.UTF8, "application/json")
            };
            using var response = await HttpClient.SendAsync(request, cancellationToken);
            var responseBody = await response.Content.ReadAsStringAsync(cancellationToken);
            if (!response.IsSuccessStatusCode)
            {
                throw new InvalidOperationException(
                    $"Telegram returned HTTP {(int)response.StatusCode}: {Trim(responseBody, 900)}");
            }

            await MarkSentAsync(notification.NotificationID, cancellationToken);
            Log.Information(
                "Telegram notification delivered :: id={NotificationId} category={Category}",
                notification.NotificationID,
                notification.Category);
        }
        catch (Exception ex) when (ex is not OperationCanceledException)
        {
            await MarkFailedAttemptAsync(notification, ex.Message, cancellationToken);
            Log.Warning(
                "Telegram notification delivery failed :: id={NotificationId} attempt={Attempt}: {Message}",
                notification.NotificationID,
                notification.Attempts + 1,
                ex.Message);
        }
    }

    private static async Task<TelegramQueueItem?> ClaimNextAsync(CancellationToken cancellationToken)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(cancellationToken);
        return await connection.QuerySingleOrDefaultAsync<TelegramQueueItem>(
            new CommandDefinition(@"
SET TRANSACTION ISOLATION LEVEL READ COMMITTED;
DECLARE @NotificationID bigint;
SELECT TOP (1) @NotificationID = NotificationID
FROM dbo.TelegramNotificationQueue WITH (UPDLOCK, READPAST, READCOMMITTEDLOCK, ROWLOCK)
WHERE Status = 0 AND NextAttemptUtc <= SYSUTCDATETIME()
ORDER BY NotificationID;

IF @NotificationID IS NOT NULL
BEGIN
    UPDATE dbo.TelegramNotificationQueue
    SET Status = 1, ProcessingAtUtc = SYSUTCDATETIME()
    OUTPUT inserted.NotificationID, inserted.Category, inserted.MessageHtml, inserted.Attempts
    WHERE NotificationID = @NotificationID AND Status = 0;
END;",
                cancellationToken: cancellationToken));
    }

    private static async Task MarkSentAsync(long notificationId, CancellationToken cancellationToken)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(cancellationToken);
        await connection.ExecuteAsync(new CommandDefinition(@"
UPDATE dbo.TelegramNotificationQueue
SET Status = 2, SentAtUtc = SYSUTCDATETIME(), ProcessingAtUtc = NULL, LastError = NULL
WHERE NotificationID = @NotificationID;",
            new { NotificationID = notificationId },
            cancellationToken: cancellationToken));
    }

    private static async Task MarkFailedAttemptAsync(
        TelegramQueueItem notification,
        string error,
        CancellationToken cancellationToken)
    {
        var attempts = notification.Attempts + 1;
        var delaySeconds = Math.Min(900, 5 * (int)Math.Pow(2, Math.Min(attempts - 1, 7)));
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(cancellationToken);
        await connection.ExecuteAsync(new CommandDefinition(@"
UPDATE dbo.TelegramNotificationQueue
SET Status = @Status,
    Attempts = @Attempts,
    NextAttemptUtc = DATEADD(SECOND, @DelaySeconds, SYSUTCDATETIME()),
    ProcessingAtUtc = NULL,
    LastError = @LastError
WHERE NotificationID = @NotificationID;",
            new
            {
                notification.NotificationID,
                Status = attempts >= MaxAttempts ? 3 : 0,
                Attempts = attempts,
                DelaySeconds = delaySeconds,
                LastError = Trim(error, 1000)
            },
            cancellationToken: cancellationToken));
    }

    private static async Task EnqueueAsync(
        string eventKey,
        string category,
        string messageHtml,
        CancellationToken cancellationToken = default)
    {
        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync(cancellationToken);
            await connection.ExecuteAsync(new CommandDefinition(@"
BEGIN TRY
    INSERT dbo.TelegramNotificationQueue
        (EventKey, Category, MessageHtml, Status, Attempts, NextAttemptUtc, CreatedAtUtc)
    VALUES
        (@EventKey, @Category, @MessageHtml, 0, 0, SYSUTCDATETIME(), SYSUTCDATETIME());
END TRY
BEGIN CATCH
    IF ERROR_NUMBER() NOT IN (2601, 2627)
        THROW;
END CATCH;",
                new
                {
                    EventKey = Trim(eventKey, 180),
                    Category = Trim(category, 32),
                    MessageHtml = Trim(messageHtml, 4000)
                },
                cancellationToken: cancellationToken));
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Could not queue Telegram notification {EventKey}", eventKey);
        }
    }

    private static async Task<string?> ResolveUniqueDisplayNameAsync(int mobId)
    {
        try
        {
            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            var row = await connection.QuerySingleOrDefaultAsync<UniqueNameRow>($@"
SELECT
    C.ID AS MobID,
    CONVERT(nvarchar(128), C.CodeName128) AS CodeName128,
    CONVERT(nvarchar(128), C.NameStrID128) AS NameStrID128,
    NULLIF(LTRIM(RTRIM(N.DisplayName)), N'') AS DisplayName
FROM {shardDb}.dbo._RefObjCommon C WITH (NOLOCK)
LEFT JOIN dbo.TelegramUniqueDisplayNames N WITH (NOLOCK) ON N.MobID = C.ID
WHERE C.ID = @MobID AND C.Rarity = 3;",
                new { MobID = mobId });

            if (row == null)
                return await RecordUnresolvedNameAsync(mobId, string.Empty, string.Empty);

            if (!string.IsNullOrWhiteSpace(row.DisplayName))
                return CleanDisplayName(row.DisplayName, string.Empty);

            var canonicalName = ResolveCanonicalName(row.CodeName128);
            if (canonicalName != null)
            {
                await UpsertUniqueNameAsync(row.MobID, row.CodeName128, row.NameStrID128, canonicalName, false);
                return canonicalName;
            }

            if (LooksLikeDisplayName(row.NameStrID128))
            {
                var displayName = CleanDisplayName(row.NameStrID128, string.Empty);
                await UpsertUniqueNameAsync(row.MobID, row.CodeName128, row.NameStrID128, displayName, false);
                return displayName;
            }

            return await RecordUnresolvedNameAsync(row.MobID, row.CodeName128, row.NameStrID128);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Unique display-name resolution failed for MobID {MobID}", mobId);
            return null;
        }
    }

    private static async Task<int?> ResolveSpawnRefObjIdAsync(int worldObjectId)
    {
        // 0x300C carries the transient world object ID. The entity spawn
        // packet supplies its stable _RefObjCommon ID and normally arrives
        // first; allow a short ordering window for concurrent packet handlers.
        for (var attempt = 0; attempt < 20; attempt++)
        {
            if (EntitySpawns.TryGetValue(worldObjectId, out var identity))
                return identity.RefObjId;

            await Task.Delay(100);
        }

        return null;
    }

    private static string? ResolveCanonicalName(string codeName)
    {
        if (CanonicalUniqueNames.TryGetValue(codeName, out var exact))
            return exact;

        foreach (var entry in CanonicalUniqueNames)
        {
            if (codeName.StartsWith(entry.Key + "_", StringComparison.OrdinalIgnoreCase))
                return entry.Value;
        }

        return null;
    }

    private static async Task<string?> RecordUnresolvedNameAsync(int mobId, string codeName, string nameStrId)
    {
        await UpsertUniqueNameAsync(mobId, codeName, nameStrId, null, false);
        if (UnresolvedNamesLogged.TryAdd(mobId, 0))
        {
            Log.Warning(
                "Telegram skipped Unique MobID {MobID} because no safe English display name exists. Add one in Telegram Notifications > Unique Names.",
                mobId);
        }

        return null;
    }

    private static async Task UpsertUniqueNameAsync(
        int mobId,
        string codeName,
        string nameStrId,
        string? displayName,
        bool isCustom)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await connection.ExecuteAsync(@"
MERGE dbo.TelegramUniqueDisplayNames AS target
USING (SELECT @MobID AS MobID) AS source ON source.MobID = target.MobID
WHEN MATCHED THEN UPDATE SET
    CodeName128 = CASE WHEN @CodeName128 <> N'' THEN @CodeName128 ELSE target.CodeName128 END,
    NameStrID128 = CASE WHEN @NameStrID128 <> N'' THEN @NameStrID128 ELSE target.NameStrID128 END,
    DisplayName = COALESCE(target.DisplayName, @DisplayName),
    IsCustom = CASE WHEN target.DisplayName IS NOT NULL THEN target.IsCustom ELSE @IsCustom END,
    UpdatedAtUtc = SYSUTCDATETIME()
WHEN NOT MATCHED THEN INSERT
    (MobID, CodeName128, NameStrID128, DisplayName, IsCustom, UpdatedAtUtc)
VALUES
    (@MobID, NULLIF(@CodeName128, N''), NULLIF(@NameStrID128, N''), @DisplayName, @IsCustom, SYSUTCDATETIME());",
            new
            {
                MobID = mobId,
                CodeName128 = Trim(codeName, 128),
                NameStrID128 = Trim(nameStrId, 128),
                DisplayName = displayName,
                IsCustom = isCustom
            });
    }

    private static async Task<TelegramRuntimeSettings> GetSettingsAsync()
    {
        if (DateTime.UtcNow - _settingsLoadedAtUtc < TimeSpan.FromSeconds(15))
            return _settings;

        await LoadSettingsAsync(force: false);
        return _settings;
    }

    private static async Task LoadSettingsAsync(bool force)
    {
        if (!force && DateTime.UtcNow - _settingsLoadedAtUtc < TimeSpan.FromSeconds(15))
            return;

        await SettingsLock.WaitAsync();
        try
        {
            if (!force && DateTime.UtcNow - _settingsLoadedAtUtc < TimeSpan.FromSeconds(15))
                return;

            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            var row = await connection.QuerySingleAsync<TelegramSettingsRow>(
                "SELECT TOP (1) * FROM dbo.TelegramNotificationSettings WITH (NOLOCK) WHERE SettingID = 1;");
            string botToken;
            try
            {
                botToken = LocalMachineSecretProtector.Unprotect(row.BotTokenProtected ?? string.Empty);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Telegram token could not be decrypted; save it again from the desktop application");
                botToken = string.Empty;
            }

            _settings = new TelegramRuntimeSettings(
                row.Enabled,
                botToken,
                row.ChannelId?.Trim() ?? string.Empty,
                row.UniqueSpawnEnabled,
                row.UniqueKillEnabled,
                row.ShowKillerName,
                row.EventReminderEnabled,
                ParseReminderMinutes(row.ReminderMinutes),
                row.EventStartedEnabled,
                row.EventFinishedEnabled,
                row.ServerOnlineEnabled,
                row.FortressWarEnabled);
            _settingsLoadedAtUtc = DateTime.UtcNow;
        }
        finally
        {
            SettingsLock.Release();
        }
    }

    private static int[] ParseReminderMinutes(string? value)
    {
        return (value ?? string.Empty)
            .Split(',', StringSplitOptions.TrimEntries | StringSplitOptions.RemoveEmptyEntries)
            .Select(item => int.TryParse(item, out var minutes) ? minutes : 0)
            .Where(minutes => minutes is >= 1 and <= 1440)
            .Distinct()
            .OrderByDescending(minutes => minutes)
            .Take(12)
            .ToArray();
    }

    private static async Task ResetAbandonedMessagesAsync()
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await connection.ExecuteAsync(@"
UPDATE dbo.TelegramNotificationQueue
SET Status = 0, ProcessingAtUtc = NULL, NextAttemptUtc = SYSUTCDATETIME()
WHERE Status = 1 AND ProcessingAtUtc < DATEADD(MINUTE, -2, SYSUTCDATETIME());");
    }

    private static async Task EnsureSchemaAsync()
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        var ready = await connection.ExecuteScalarAsync<int>(@"
SELECT CASE WHEN OBJECT_ID(N'dbo.TelegramNotificationSettings',N'U') IS NOT NULL
                  AND OBJECT_ID(N'dbo.TelegramNotificationQueue',N'U') IS NOT NULL
                  AND OBJECT_ID(N'dbo.TelegramUniqueDisplayNames',N'U') IS NOT NULL
             THEN 1 ELSE 0 END;");
        if (ready != 1)
            throw new InvalidOperationException(
                "Telegram notification schema is incomplete. Apply the packaged database updates.");
    }

    private static async Task LoadUniqueRefObjIdsAsync()
    {
        var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        var ids = await connection.QueryAsync<int>($@"
SELECT ID
FROM {shardDb}.dbo._RefObjCommon WITH (NOLOCK)
WHERE Rarity = 3;");
        _uniqueRefObjIds = ids.ToHashSet();
        Log.Information(
            "Telegram loaded {UniqueCount} Unique definitions from _RefObjCommon Rarity=3",
            _uniqueRefObjIds.Count);
    }

    private static bool TryAcquireRecent(string key, TimeSpan lifetime)
    {
        var now = DateTime.UtcNow;
        if (RecentEvents.TryGetValue(key, out var seenAt) && now - seenAt < lifetime)
            return false;

        RecentEvents[key] = now;
        if (RecentEvents.Count > 2048)
        {
            foreach (var stale in RecentEvents.Where(item => now - item.Value > TimeSpan.FromHours(1)).Take(512))
                RecentEvents.TryRemove(stale.Key, out _);
        }

        return true;
    }

    private static bool LooksLikeDisplayName(string value)
    {
        return !string.IsNullOrWhiteSpace(value) &&
               value.Contains(' ') &&
               !value.StartsWith("SN_", StringComparison.OrdinalIgnoreCase) &&
               !value.StartsWith("MOB_", StringComparison.OrdinalIgnoreCase);
    }

    private static string DisplayServerName()
    {
        return CleanDisplayName(_serverSettings.ServerName, "KMTGuard Server");
    }

    private static string CleanDisplayName(string? value, string fallback)
    {
        var result = (value ?? string.Empty).Trim();
        return result.Length == 0 ? fallback : Trim(result, 128);
    }

    private static string Escape(string? value)
    {
        return WebUtility.HtmlEncode(value ?? string.Empty);
    }

    private static string NormalizeKey(string? value)
    {
        var normalized = new string((value ?? string.Empty)
            .ToLowerInvariant()
            .Where(character => char.IsLetterOrDigit(character) || character is '-' or '_')
            .ToArray());
        return normalized.Length == 0 ? "unknown" : Trim(normalized, 64);
    }

    private static string FortressName(int fortressId)
    {
        return fortressId switch
        {
            1 => "Jangan Fortress",
            3 => "Hotan Fortress",
            6 => "Bandit Fortress",
            _ => $"Fortress #{fortressId}"
        };
    }

    private static string Trim(string? value, int length)
    {
        value ??= string.Empty;
        return value.Length <= length ? value : value[..length];
    }

    private sealed class TelegramQueueItem
    {
        public long NotificationID { get; set; }
        public string Category { get; set; } = string.Empty;
        public string MessageHtml { get; set; } = string.Empty;
        public int Attempts { get; set; }
    }

    private sealed class UniqueNameRow
    {
        public int MobID { get; set; }
        public string CodeName128 { get; set; } = string.Empty;
        public string NameStrID128 { get; set; } = string.Empty;
        public string? DisplayName { get; set; }
    }

    private sealed record EntitySpawnIdentity(int RefObjId, DateTime ObservedAtUtc);

    private sealed class TelegramSettingsRow
    {
        public bool Enabled { get; set; }
        public string? BotTokenProtected { get; set; }
        public string? ChannelId { get; set; }
        public bool UniqueSpawnEnabled { get; set; }
        public bool UniqueKillEnabled { get; set; }
        public bool ShowKillerName { get; set; }
        public bool EventReminderEnabled { get; set; }
        public string? ReminderMinutes { get; set; }
        public bool EventStartedEnabled { get; set; }
        public bool EventFinishedEnabled { get; set; }
        public bool ServerOnlineEnabled { get; set; }
        public bool FortressWarEnabled { get; set; }
    }

    private sealed record TelegramRuntimeSettings(
        bool Enabled,
        string BotToken,
        string ChannelId,
        bool UniqueSpawnEnabled,
        bool UniqueKillEnabled,
        bool ShowKillerName,
        bool EventReminderEnabled,
        int[] ReminderMinutes,
        bool EventStartedEnabled,
        bool EventFinishedEnabled,
        bool ServerOnlineEnabled,
        bool FortressWarEnabled)
    {
        public static readonly TelegramRuntimeSettings Disabled =
            new(false, string.Empty, string.Empty, false, false, false, false, Array.Empty<int>(), false, false, false, false);
    }
}
