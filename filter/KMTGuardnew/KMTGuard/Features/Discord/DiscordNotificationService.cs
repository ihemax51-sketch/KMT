using System.Net;
using System.Text;
using System.Text.Json;
using Dapper;
using KMTGuard.Database;
using KMTGuard.RuntimeContract;
using Microsoft.Data.SqlClient;
using Serilog;

namespace KMTGuard.Features.Discord;

public static class DiscordNotificationService
{
    private const int MaxAttempts = 8;
    private static readonly HttpClient HttpClient = new()
    {
        Timeout = TimeSpan.FromSeconds(12)
    };
    private static readonly SemaphoreSlim LifecycleLock = new(1, 1);
    private static readonly SemaphoreSlim SettingsLock = new(1, 1);

    private static CancellationTokenSource? _stopSource;
    private static Task? _worker;
    private static DiscordRuntimeSettings _settings = DiscordRuntimeSettings.Disabled;
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
            Log.Information("Discord notification service initialized");
        }
        finally
        {
            LifecycleLock.Release();
        }
    }

    public static void Stop()
    {
        LifecycleLock.Wait();
        try
        {
            if (_stopSource == null)
                return;

            _stopSource.Cancel();
            try
            {
                _worker?.Wait(TimeSpan.FromSeconds(5));
            }
            catch (AggregateException ex) when (ex.InnerExceptions.All(item => item is OperationCanceledException))
            {
            }

            _stopSource.Dispose();
            _stopSource = null;
            _worker = null;
            _settings = DiscordRuntimeSettings.Disabled;
        }
        finally
        {
            LifecycleLock.Release();
        }
    }

    private static async Task RunWorkerAsync(CancellationToken cancellationToken)
    {
        while (!cancellationToken.IsCancellationRequested)
        {
            try
            {
                var settings = await GetSettingsAsync();
                if (!settings.Enabled || string.IsNullOrWhiteSpace(settings.BotToken))
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
                Log.Warning(ex, "Discord notification worker iteration failed");
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
        DiscordQueueItem notification,
        DiscordRuntimeSettings settings,
        CancellationToken cancellationToken)
    {
        if (string.IsNullOrWhiteSpace(notification.DiscordChannelID))
        {
            await MarkPermanentFailureAsync(
                notification.NotificationID,
                $"Channel '{notification.ChannelName}' does not exist or is disabled.",
                cancellationToken);
            return;
        }

        try
        {
            var endpoint =
                $"https://discord.com/api/v10/channels/{Uri.EscapeDataString(notification.DiscordChannelID)}/messages";
            var payload = JsonSerializer.Serialize(new
            {
                content = notification.MessageText,
                allowed_mentions = new { parse = Array.Empty<string>() }
            });
            using var request = new HttpRequestMessage(HttpMethod.Post, endpoint)
            {
                Content = new StringContent(payload, Encoding.UTF8, "application/json")
            };
            request.Headers.TryAddWithoutValidation("Authorization", $"Bot {settings.BotToken}");
            request.Headers.TryAddWithoutValidation("User-Agent", "KMTGuard-DiscordNotifications/1.0");

            using var response = await HttpClient.SendAsync(request, cancellationToken);
            var responseBody = await response.Content.ReadAsStringAsync(cancellationToken);
            if (response.IsSuccessStatusCode)
            {
                await MarkSentAsync(notification.NotificationID, cancellationToken);
                Log.Information(
                    "Discord notification delivered :: id={NotificationId} channel={ChannelName}",
                    notification.NotificationID,
                    notification.ChannelName);
                return;
            }

            var error = $"Discord returned HTTP {(int)response.StatusCode}: {Trim(responseBody, 900)}";
            if (response.StatusCode == HttpStatusCode.TooManyRequests)
            {
                await RescheduleRateLimitedAsync(
                    notification.NotificationID,
                    error,
                    ParseRetryAfter(responseBody, response),
                    cancellationToken);
                return;
            }

            if ((int)response.StatusCode is >= 400 and < 500)
            {
                await MarkPermanentFailureAsync(notification.NotificationID, error, cancellationToken);
                Log.Warning(
                    "Discord notification permanently failed :: id={NotificationId} channel={ChannelName}: {Message}",
                    notification.NotificationID,
                    notification.ChannelName,
                    error);
                return;
            }

            await MarkFailedAttemptAsync(notification, error, cancellationToken);
        }
        catch (Exception ex) when (ex is not OperationCanceledException)
        {
            await MarkFailedAttemptAsync(notification, ex.Message, cancellationToken);
            Log.Warning(
                ex,
                "Discord notification delivery failed :: id={NotificationId} channel={ChannelName} attempt={Attempt}",
                notification.NotificationID,
                notification.ChannelName,
                notification.Attempts + 1);
        }
    }

    private static TimeSpan ParseRetryAfter(string responseBody, HttpResponseMessage response)
    {
        try
        {
            using var document = JsonDocument.Parse(responseBody);
            if (document.RootElement.TryGetProperty("retry_after", out var value) &&
                value.TryGetDouble(out var seconds))
            {
                return TimeSpan.FromSeconds(Math.Clamp(seconds, 1, 900));
            }
        }
        catch (JsonException)
        {
        }

        if (response.Headers.RetryAfter?.Delta is { } delta)
            return TimeSpan.FromSeconds(Math.Clamp(delta.TotalSeconds, 1, 900));

        return TimeSpan.FromSeconds(5);
    }

    private static async Task<DiscordQueueItem?> ClaimNextAsync(CancellationToken cancellationToken)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(cancellationToken);
        return await connection.QuerySingleOrDefaultAsync<DiscordQueueItem>(
            new CommandDefinition(@"
SET TRANSACTION ISOLATION LEVEL READ COMMITTED;
DECLARE @NotificationID bigint;
SELECT TOP (1) @NotificationID = NotificationID
FROM dbo.DiscordNotificationQueue WITH (UPDLOCK, READPAST, READCOMMITTEDLOCK, ROWLOCK)
WHERE Status = 0 AND NextAttemptUtc <= SYSUTCDATETIME()
ORDER BY NotificationID;

IF @NotificationID IS NOT NULL
BEGIN
    UPDATE Q
    SET Status = 1, ProcessingAtUtc = SYSUTCDATETIME()
    OUTPUT inserted.NotificationID,
           inserted.ChannelName,
           inserted.MessageText,
           inserted.Attempts,
           C.DiscordChannelID
    FROM dbo.DiscordNotificationQueue Q
    LEFT JOIN dbo.DiscordNotificationChannels C WITH (NOLOCK)
      ON C.ChannelName = Q.ChannelName AND C.Enabled = 1
    WHERE Q.NotificationID = @NotificationID AND Q.Status = 0;
END;",
                cancellationToken: cancellationToken));
    }

    private static async Task MarkSentAsync(long notificationId, CancellationToken cancellationToken)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(cancellationToken);
        await connection.ExecuteAsync(new CommandDefinition(@"
UPDATE dbo.DiscordNotificationQueue
SET Status = 2, SentAtUtc = SYSUTCDATETIME(), ProcessingAtUtc = NULL, LastError = NULL
WHERE NotificationID = @NotificationID;",
            new { NotificationID = notificationId },
            cancellationToken: cancellationToken));
    }

    private static async Task MarkPermanentFailureAsync(
        long notificationId,
        string error,
        CancellationToken cancellationToken)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(cancellationToken);
        await connection.ExecuteAsync(new CommandDefinition(@"
UPDATE dbo.DiscordNotificationQueue
SET Status = 3,
    ProcessingAtUtc = NULL,
    LastError = @LastError
WHERE NotificationID = @NotificationID;",
            new
            {
                NotificationID = notificationId,
                LastError = Trim(error, 1000)
            },
            cancellationToken: cancellationToken));
    }

    private static async Task RescheduleRateLimitedAsync(
        long notificationId,
        string error,
        TimeSpan requestedDelay,
        CancellationToken cancellationToken)
    {
        var delaySeconds = (int)Math.Ceiling(Math.Clamp(requestedDelay.TotalSeconds, 1, 900));
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(cancellationToken);
        await connection.ExecuteAsync(new CommandDefinition(@"
UPDATE dbo.DiscordNotificationQueue
SET Status = 0,
    NextAttemptUtc = DATEADD(SECOND, @DelaySeconds, SYSUTCDATETIME()),
    ProcessingAtUtc = NULL,
    LastError = @LastError
WHERE NotificationID = @NotificationID;",
            new
            {
                NotificationID = notificationId,
                DelaySeconds = delaySeconds,
                LastError = Trim(error, 1000)
            },
            cancellationToken: cancellationToken));
    }

    private static async Task MarkFailedAttemptAsync(
        DiscordQueueItem notification,
        string error,
        CancellationToken cancellationToken)
    {
        var attempts = notification.Attempts + 1;
        var delaySeconds = Math.Min(900, 5 * (int)Math.Pow(2, Math.Min(attempts - 1, 7)));
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(cancellationToken);
        await connection.ExecuteAsync(new CommandDefinition(@"
UPDATE dbo.DiscordNotificationQueue
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

    private static async Task<DiscordRuntimeSettings> GetSettingsAsync()
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
            var row = await connection.QuerySingleAsync<DiscordSettingsRow>(
                "SELECT Enabled, BotTokenProtected FROM dbo.DiscordNotificationSettings WITH (NOLOCK) WHERE SettingID = 1;");
            string botToken;
            try
            {
                botToken = LocalMachineSecretProtector.Unprotect(row.BotTokenProtected ?? string.Empty);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Discord Bot Token could not be decrypted; save it again from the desktop application");
                botToken = string.Empty;
            }

            _settings = new DiscordRuntimeSettings(row.Enabled, botToken);
            _settingsLoadedAtUtc = DateTime.UtcNow;
        }
        finally
        {
            SettingsLock.Release();
        }
    }

    private static async Task ResetAbandonedMessagesAsync()
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await connection.ExecuteAsync(@"
UPDATE dbo.DiscordNotificationQueue
SET Status = 0, ProcessingAtUtc = NULL, NextAttemptUtc = SYSUTCDATETIME()
WHERE Status = 1 AND ProcessingAtUtc < DATEADD(MINUTE, -2, SYSUTCDATETIME());");
    }

    private static async Task EnsureSchemaAsync()
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        var ready = await connection.ExecuteScalarAsync<int>(@"
SELECT CASE WHEN OBJECT_ID(N'dbo.DiscordNotificationSettings',N'U') IS NOT NULL
                  AND OBJECT_ID(N'dbo.DiscordNotificationChannels',N'U') IS NOT NULL
                  AND OBJECT_ID(N'dbo.DiscordNotificationQueue',N'U') IS NOT NULL
                  AND OBJECT_ID(N'dbo.Discord_AddChannel',N'P') IS NOT NULL
                  AND OBJECT_ID(N'dbo.Discord_Notification',N'P') IS NOT NULL
             THEN 1 ELSE 0 END;");
        if (ready != 1)
            throw new InvalidOperationException(
                "Discord notification schema is incomplete. Apply the packaged database updates.");
    }

    private static string Trim(string? value, int maxLength)
    {
        var normalized = (value ?? string.Empty).Trim();
        return normalized.Length <= maxLength ? normalized : normalized[..maxLength];
    }

    private sealed class DiscordQueueItem
    {
        public long NotificationID { get; set; }
        public string ChannelName { get; set; } = string.Empty;
        public string MessageText { get; set; } = string.Empty;
        public string? DiscordChannelID { get; set; }
        public int Attempts { get; set; }
    }

    private sealed class DiscordSettingsRow
    {
        public bool Enabled { get; set; }
        public string? BotTokenProtected { get; set; }
    }

    private sealed record DiscordRuntimeSettings(bool Enabled, string BotToken)
    {
        public static readonly DiscordRuntimeSettings Disabled = new(false, string.Empty);
    }
}
