using System.Data;
using System.IO;
using System.Globalization;
using System.Net;
using System.Net.Http;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;
using KMTGuard.AdminDesktop.Models;
using KMTGuard.RuntimeContract;
using Microsoft.Data.SqlClient;

namespace KMTGuard.AdminDesktop.Services;

public sealed record CompetitiveEventAdminConfig(
    string EventCode, string DisplayName, bool Enabled, int EventID,
    int StartDelaySeconds, int RegistrationSeconds, int PrepareSeconds, int FightSeconds,
    int MinPlayers, int MaxPlayers, int MinLevel, int HwidLimit, bool RequireHwid, bool RequireNoParty,
    int ArenaWorldID, int ArenaRegionID, int ArenaX, int ArenaY, int ArenaZ,
    int Team1X, int Team1Y, int Team1Z, int Team2X, int Team2Y, int Team2Z,
    int MadnessMobID, int MadnessMobCount, int MadnessMobX, int MadnessMobY, int MadnessMobZ,
    int MobSpawnDelaySeconds, int PairKillLimit, int TotalKillLimit,
    string KillRewardItemCode, int KillRewardItemCount, int KillRewardLimit,
    int Team1TowerMobID, int Team1TowerX, int Team1TowerY, int Team1TowerZ,
    int Team2TowerMobID, int Team2TowerX, int Team2TowerY, int Team2TowerZ);

internal sealed record AutoEventSchedulePattern(
    string ScheduleKey,
    string EventCode,
    TimeSpan StartTime,
    int DaysMask,
    int RepeatMinutes);

internal sealed record AutoEventScheduleConflict(
    string EventCode,
    DayOfWeek Day,
    TimeSpan Time);

internal sealed record ClientlessWeaponProfile(
    string Race,
    string Weapon,
    byte ArmorTypeId3,
    byte WeaponTypeId4,
    bool UseShield,
    int MasteryId,
    string MasteryName);

internal readonly record struct ClientlessHunterCandidate(
    int Id,
    string City,
    bool Enabled,
    bool GameReady,
    string? SystemRole);

internal sealed record ClientlessHunterSelection(
    string City,
    IReadOnlyList<int> SelectedAccountIds,
    int TotalAccounts,
    int ReadyEnabledAccounts);

internal sealed record ClientlessRuntimePlanSelection(
    string City,
    IReadOnlyList<int> OnlineAccountIds,
    IReadOnlyList<int> HunterAccountIds,
    int TotalAccounts,
    int ReadyAccounts);

public sealed class SqlAdminService
{
    private const string EventsDatabaseName = "Events";
    internal const string AutoEventSchedulePatternKeyColumnName = "ScheduleKey";
    private const string PacketWhitelistTable = "[dbo].[Security_Whitelist]";
    private const string PacketBlacklistTable = "[dbo].[Security_Blacklist]";
    private static readonly Regex SchedulerProcedureIdentifierPattern = new(
        @"^(?:\[[^\]\r\n]+\]|[A-Za-z_][A-Za-z0-9_]*)(?:\.(?:\[[^\]\r\n]+\]|[A-Za-z_][A-Za-z0-9_]*)){0,2}$",
        RegexOptions.Compiled | RegexOptions.CultureInvariant);
    private static readonly ClientlessWeaponProfile[] ClientlessWeaponProfiles =
    [
        new("Chinese", "Sword + Shield", 1, 2, true, 257, "Bicheon"),
        new("Chinese", "Sword + Shield", 2, 2, true, 257, "Bicheon"),
        new("Chinese", "Blade + Shield", 2, 3, true, 257, "Bicheon"),
        new("Chinese", "Blade + Shield", 3, 3, true, 257, "Bicheon"),
        new("Chinese", "Spear", 2, 4, false, 258, "Heuksal"),
        new("Chinese", "Glaive", 3, 5, false, 258, "Heuksal"),
        new("Chinese", "Bow", 1, 6, false, 259, "Pacheon"),
        new("Europe", "One-Hand Sword + Shield", 11, 7, true, 513, "Warrior"),
        new("Europe", "Two-Hand Sword", 11, 8, false, 513, "Warrior"),
        new("Europe", "Dual Axe", 11, 9, false, 513, "Warrior"),
        new("Europe", "Warlock Rod", 9, 10, false, 516, "Warlock"),
        new("Europe", "Wizard Staff", 9, 11, false, 514, "Wizard"),
        new("Europe", "Crossbow", 10, 12, false, 515, "Rogue"),
        new("Europe", "Dagger", 10, 13, false, 515, "Rogue"),
        new("Europe", "Harp", 9, 14, false, 517, "Bard"),
        new("Europe", "Cleric Rod + Shield", 9, 15, true, 518, "Cleric")
    ];
    private readonly string _connectionString;

    public SqlAdminService(string connectionString)
    {
        _connectionString = connectionString;
    }

    public async Task<DiscordNotificationSettings> LoadDiscordSettingsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureDiscordSchemaAsync(connection);

        await using var command = new SqlCommand(@"
SELECT Enabled, BotTokenProtected, UpdatedAtUtc
FROM dbo.DiscordNotificationSettings WITH (NOLOCK)
WHERE SettingID = 1;", connection);
        await using var reader = await command.ExecuteReaderAsync();
        if (!await reader.ReadAsync())
            throw new InvalidOperationException("Discord notification settings could not be initialized.");

        var hasStoredToken = !reader.IsDBNull(1) && reader.GetString(1).Length > 0;
        return new DiscordNotificationSettings
        {
            Enabled = reader.GetBoolean(0),
            HasStoredBotToken = hasStoredToken,
            UpdatedAtUtc = reader.GetDateTime(2)
        };
    }

    public async Task SaveDiscordSettingsAsync(DiscordNotificationSettings settings)
    {
        ArgumentNullException.ThrowIfNull(settings);
        if (settings.Enabled && string.IsNullOrWhiteSpace(settings.BotToken) &&
            !settings.HasStoredBotToken)
        {
            throw new InvalidOperationException(
                "Enter and save the Discord Bot Token before enabling notifications.");
        }

        if (!string.IsNullOrWhiteSpace(settings.BotToken))
            ValidateDiscordBotToken(settings.BotToken);

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureDiscordSchemaAsync(connection);
        var protectedToken = string.IsNullOrWhiteSpace(settings.BotToken)
            ? null
            : LocalMachineSecretProtector.Protect(settings.BotToken.Trim());

        await using var command = new SqlCommand(@"
UPDATE dbo.DiscordNotificationSettings
SET Enabled = @Enabled,
    BotTokenProtected = CASE WHEN @BotTokenProtected IS NULL
                             THEN BotTokenProtected ELSE @BotTokenProtected END,
    UpdatedAtUtc = SYSUTCDATETIME()
WHERE SettingID = 1;", connection);
        command.Parameters.Add("@Enabled", SqlDbType.Bit).Value = settings.Enabled;
        command.Parameters.Add("@BotTokenProtected", SqlDbType.NVarChar, 2048).Value =
            protectedToken == null ? DBNull.Value : protectedToken;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "SaveDiscordSettings", "Discord", "Bot notification settings updated");
    }

    public async Task<string> TestDiscordBotAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureDiscordSchemaAsync(connection);
        var token = await ReadDiscordBotTokenAsync(connection);

        using var client = CreateDiscordClient(token);
        using var response = await client.GetAsync("https://discord.com/api/v10/users/@me");
        var body = await response.Content.ReadAsStringAsync();
        if (!response.IsSuccessStatusCode)
        {
            throw new InvalidOperationException(
                $"Discord rejected the Bot Token (HTTP {(int)response.StatusCode}): {TrimForAudit(body, 700)}");
        }

        using var document = JsonDocument.Parse(body);
        var username = document.RootElement.TryGetProperty("username", out var value)
            ? value.GetString()
            : null;
        return string.IsNullOrWhiteSpace(username)
            ? "Discord Bot connection verified."
            : $"Connected to Discord as {username}.";
    }

    public async Task<IReadOnlyList<DiscordNotificationChannel>> LoadDiscordChannelsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureDiscordSchemaAsync(connection);
        await using var command = new SqlCommand(@"
SELECT ChannelRecordID, ChannelName, DiscordChannelID, Enabled, SortOrder
FROM dbo.DiscordNotificationChannels WITH (NOLOCK)
ORDER BY SortOrder, ChannelRecordID;", connection);
        await using var reader = await command.ExecuteReaderAsync();
        var channels = new List<DiscordNotificationChannel>();
        while (await reader.ReadAsync())
        {
            var name = reader.GetString(1);
            channels.Add(new DiscordNotificationChannel
            {
                ChannelRecordID = reader.GetInt32(0),
                ChannelName = name,
                OriginalChannelName = name,
                DiscordChannelID = reader.GetString(2),
                Enabled = reader.GetBoolean(3),
                SortOrder = reader.GetInt32(4),
                StatusMessage = "Saved and ready."
            });
        }

        return channels;
    }

    public async Task<DiscordNotificationChannel> SaveDiscordChannelAsync(
        DiscordNotificationChannel channel)
    {
        ArgumentNullException.ThrowIfNull(channel);
        ValidateDiscordChannel(channel.ChannelName, channel.DiscordChannelID);
        var isNew = channel.ChannelRecordID <= 0;

        var channelName = channel.ChannelName.Trim();
        var originalName = channel.OriginalChannelName.Trim();
        var discordChannelId = channel.DiscordChannelID.Trim();

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureDiscordSchemaAsync(connection);
        await using var transaction = await connection.BeginTransactionAsync();
        try
        {
            if (isNew)
            {
                await using var insert = new SqlCommand(@"
IF EXISTS (SELECT 1 FROM dbo.DiscordNotificationChannels WHERE ChannelName = @ChannelName)
    THROW 51122, 'A Discord channel with this name already exists.', 1;
IF EXISTS (SELECT 1 FROM dbo.DiscordNotificationChannels WHERE DiscordChannelID = @DiscordChannelID)
    THROW 51123, 'This Discord Channel ID is already assigned to another channel.', 1;

DECLARE @SortOrder int =
    ISNULL((SELECT MAX(SortOrder) + 10 FROM dbo.DiscordNotificationChannels WITH (UPDLOCK, HOLDLOCK)), 10);
INSERT dbo.DiscordNotificationChannels
    (ChannelName, DiscordChannelID, Enabled, SortOrder, CreatedAtUtc, UpdatedAtUtc)
VALUES
    (@ChannelName, @DiscordChannelID, @Enabled, @SortOrder, SYSUTCDATETIME(), SYSUTCDATETIME());
SELECT CONVERT(int, SCOPE_IDENTITY()), @SortOrder;", connection, (SqlTransaction)transaction);
                insert.Parameters.Add("@ChannelName", SqlDbType.NVarChar, 80).Value = channelName;
                insert.Parameters.Add("@DiscordChannelID", SqlDbType.VarChar, 32).Value = discordChannelId;
                insert.Parameters.Add("@Enabled", SqlDbType.Bit).Value = channel.Enabled;
                await using var reader = await insert.ExecuteReaderAsync();
                await reader.ReadAsync();
                channel.ChannelRecordID = reader.GetInt32(0);
                channel.SortOrder = reader.GetInt32(1);
            }
            else
            {
                await using var update = new SqlCommand(@"
IF EXISTS
(
    SELECT 1 FROM dbo.DiscordNotificationChannels
    WHERE ChannelName = @ChannelName AND ChannelRecordID <> @ChannelRecordID
)
    THROW 51122, 'A Discord channel with this name already exists.', 1;
IF EXISTS
(
    SELECT 1 FROM dbo.DiscordNotificationChannels
    WHERE DiscordChannelID = @DiscordChannelID AND ChannelRecordID <> @ChannelRecordID
)
    THROW 51123, 'This Discord Channel ID is already assigned to another channel.', 1;

UPDATE dbo.DiscordNotificationChannels
SET ChannelName = @ChannelName,
    DiscordChannelID = @DiscordChannelID,
    Enabled = @Enabled,
    UpdatedAtUtc = SYSUTCDATETIME()
WHERE ChannelRecordID = @ChannelRecordID;
IF @@ROWCOUNT = 0
    THROW 51121, 'The Discord channel no longer exists.', 1;

IF @OriginalChannelName <> @ChannelName
BEGIN
    UPDATE dbo.DiscordNotificationQueue
    SET ChannelName = @ChannelName
    WHERE ChannelName = @OriginalChannelName AND Status = 0;
END;", connection, (SqlTransaction)transaction);
                update.Parameters.Add("@ChannelRecordID", SqlDbType.Int).Value = channel.ChannelRecordID;
                update.Parameters.Add("@ChannelName", SqlDbType.NVarChar, 80).Value = channelName;
                update.Parameters.Add("@OriginalChannelName", SqlDbType.NVarChar, 80).Value =
                    originalName.Length == 0 ? channelName : originalName;
                update.Parameters.Add("@DiscordChannelID", SqlDbType.VarChar, 32).Value = discordChannelId;
                update.Parameters.Add("@Enabled", SqlDbType.Bit).Value = channel.Enabled;
                await update.ExecuteNonQueryAsync();
            }

            await AuditAsync(
                connection,
                isNew ? "AddDiscordChannel" : "SaveDiscordChannel",
                channelName,
                discordChannelId,
                (SqlTransaction)transaction);
            await transaction.CommitAsync();
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }

        channel.ChannelName = channelName;
        channel.OriginalChannelName = channelName;
        channel.DiscordChannelID = discordChannelId;
        channel.StatusMessage = "Saved and ready.";
        return channel;
    }

    public async Task DeleteDiscordChannelAsync(int channelRecordId)
    {
        if (channelRecordId <= 0)
            return;

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureDiscordSchemaAsync(connection);
        await using var transaction = await connection.BeginTransactionAsync();
        try
        {
            await using var command = new SqlCommand(@"
DECLARE @ChannelName nvarchar(80);
SELECT @ChannelName = ChannelName
FROM dbo.DiscordNotificationChannels WITH (UPDLOCK, HOLDLOCK)
WHERE ChannelRecordID = @ChannelRecordID;
IF @ChannelName IS NULL
    RETURN;

UPDATE dbo.DiscordNotificationQueue
SET Status = 3,
    ProcessingAtUtc = NULL,
    LastError = N'The destination channel was removed before delivery.'
WHERE ChannelName = @ChannelName AND Status IN (0, 1);

DELETE dbo.DiscordNotificationChannels
WHERE ChannelRecordID = @ChannelRecordID;

SELECT @ChannelName;", connection, (SqlTransaction)transaction);
            command.Parameters.Add("@ChannelRecordID", SqlDbType.Int).Value = channelRecordId;
            var channelName = Convert.ToString(await command.ExecuteScalarAsync()) ?? channelRecordId.ToString();
            await AuditAsync(
                connection,
                "DeleteDiscordChannel",
                channelName,
                "Notification route removed",
                (SqlTransaction)transaction);
            await transaction.CommitAsync();
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }
    }

    public async Task<string> SendDiscordChannelTestAsync(DiscordNotificationChannel channel)
    {
        ArgumentNullException.ThrowIfNull(channel);
        ValidateDiscordChannel(channel.ChannelName, channel.DiscordChannelID);

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureDiscordSchemaAsync(connection);
        var token = await ReadDiscordBotTokenAsync(connection);
        using var client = CreateDiscordClient(token);
        var payload = JsonSerializer.Serialize(new
        {
            content = $"✅ **KMTGuard Discord connected**\n\n" +
                      $"The **{channel.ChannelName.Trim()}** notification route is configured correctly.",
            allowed_mentions = new { parse = Array.Empty<string>() }
        });
        using var response = await client.PostAsync(
            $"https://discord.com/api/v10/channels/{Uri.EscapeDataString(channel.DiscordChannelID.Trim())}/messages",
            new StringContent(payload, Encoding.UTF8, "application/json"));
        var body = await response.Content.ReadAsStringAsync();
        if (!response.IsSuccessStatusCode)
        {
            throw new InvalidOperationException(
                $"Discord rejected the channel test (HTTP {(int)response.StatusCode}): {TrimForAudit(body, 700)}");
        }

        return "Test delivered successfully.";
    }

    public async Task QueueDiscordNotificationAsync(string channelName, string message)
    {
        channelName = channelName.Trim();
        message = message.Trim();
        if (channelName.Length == 0)
            throw new InvalidOperationException("Choose a destination channel.");
        if (message.Length == 0)
            throw new InvalidOperationException("Enter a notification message.");
        if (message.Length > 2000)
            throw new InvalidOperationException("Discord messages cannot exceed 2,000 characters.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureDiscordSchemaAsync(connection);
        await using var command = new SqlCommand(
            "EXEC dbo.Discord_Notification @ChannelName, @Message, @EventKey;", connection);
        command.Parameters.Add("@ChannelName", SqlDbType.NVarChar, 80).Value = channelName;
        command.Parameters.Add("@Message", SqlDbType.NVarChar, 2000).Value = message;
        command.Parameters.Add("@EventKey", SqlDbType.NVarChar, 180).Value = $"manual:{Guid.NewGuid():N}";
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "QueueDiscordNotification", channelName, TrimForAudit(message, 300));
    }

    public async Task<IReadOnlyList<DiscordDeliveryActivity>> LoadDiscordDeliveryActivityAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureDiscordSchemaAsync(connection);
        await using var command = new SqlCommand(@"
SELECT TOP (40)
    NotificationID,
    ChannelName,
    LEFT(REPLACE(REPLACE(MessageText, CHAR(13), N' '), CHAR(10), N' '), 180) AS MessagePreview,
    CASE Status WHEN 0 THEN N'Pending' WHEN 1 THEN N'Processing'
                WHEN 2 THEN N'Sent' ELSE N'Failed' END AS DeliveryStatus,
    Attempts,
    CreatedAtUtc,
    SentAtUtc,
    ISNULL(LastError, N'') AS LastError
FROM dbo.DiscordNotificationQueue WITH (NOLOCK)
ORDER BY NotificationID DESC;", connection);
        await using var reader = await command.ExecuteReaderAsync();
        var activity = new List<DiscordDeliveryActivity>();
        while (await reader.ReadAsync())
        {
            activity.Add(new DiscordDeliveryActivity
            {
                NotificationID = reader.GetInt64(0),
                ChannelName = reader.GetString(1),
                MessagePreview = reader.GetString(2),
                DeliveryStatus = reader.GetString(3),
                Attempts = reader.GetInt32(4),
                CreatedAtUtc = reader.GetDateTime(5),
                SentAtUtc = reader.IsDBNull(6) ? null : reader.GetDateTime(6),
                LastError = reader.GetString(7)
            });
        }

        return activity;
    }

    public async Task RetryFailedDiscordNotificationsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureDiscordSchemaAsync(connection);
        await using var command = new SqlCommand(@"
UPDATE dbo.DiscordNotificationQueue
SET Status = 0, Attempts = 0, NextAttemptUtc = SYSUTCDATETIME(),
    ProcessingAtUtc = NULL, LastError = NULL
WHERE Status = 3;", connection);
        var affected = await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "RetryDiscordNotifications", "Failed", affected.ToString());
    }

    internal static void ValidateDiscordBotToken(string token)
    {
        token = token.Trim();
        if (token.Length is < 30 or > 200 || token.Any(char.IsWhiteSpace) ||
            token.StartsWith("http", StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidOperationException("The Discord Bot Token format is not valid.");
        }
    }

    internal static void ValidateDiscordChannel(string channelName, string channelId)
    {
        channelName = channelName.Trim();
        channelId = channelId.Trim();
        if (channelName.Length is < 2 or > 80)
            throw new InvalidOperationException("Channel name must contain 2 to 80 characters.");
        if (channelId.Length is < 17 or > 20 || channelId.Any(character => !char.IsDigit(character)))
            throw new InvalidOperationException("Discord Channel ID must contain 17 to 20 digits.");
    }

    private static HttpClient CreateDiscordClient(string token)
    {
        var client = new HttpClient { Timeout = TimeSpan.FromSeconds(12) };
        client.DefaultRequestHeaders.TryAddWithoutValidation("Authorization", $"Bot {token}");
        client.DefaultRequestHeaders.TryAddWithoutValidation(
            "User-Agent",
            "KMTGuard-AdminDesktop/1.0");
        return client;
    }

    private static async Task<string> ReadDiscordBotTokenAsync(SqlConnection connection)
    {
        await using var command = new SqlCommand(@"
SELECT BotTokenProtected
FROM dbo.DiscordNotificationSettings WITH (NOLOCK)
WHERE SettingID = 1;", connection);
        var protectedToken = Convert.ToString(await command.ExecuteScalarAsync()) ?? string.Empty;
        if (protectedToken.Length == 0)
            throw new InvalidOperationException("Enter and save the Discord Bot Token first.");

        try
        {
            return LocalMachineSecretProtector.Unprotect(protectedToken);
        }
        catch (Exception ex)
        {
            throw new InvalidOperationException(
                "The Discord Bot Token cannot be decrypted on this server. Enter and save it again.", ex);
        }
    }

    public async Task<TelegramNotificationSettings> LoadTelegramSettingsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTelegramSchemaAsync(connection);

        await using var command = new SqlCommand(@"
SELECT Enabled, BotTokenProtected, ChannelId, UniqueSpawnEnabled, UniqueKillEnabled,
       ShowKillerName, EventReminderEnabled, ReminderMinutes, EventStartedEnabled,
       EventFinishedEnabled, ServerOnlineEnabled, FortressWarEnabled, UpdatedAtUtc
FROM dbo.TelegramNotificationSettings WITH (NOLOCK)
WHERE SettingID = 1;", connection);
        await using var reader = await command.ExecuteReaderAsync();
        if (!await reader.ReadAsync())
            throw new InvalidOperationException("Telegram notification settings could not be initialized.");

        var protectedToken = reader.IsDBNull(1) ? string.Empty : reader.GetString(1);
        string token;
        try
        {
            token = LocalMachineSecretProtector.Unprotect(protectedToken);
        }
        catch (Exception ex)
        {
            throw new InvalidOperationException(
                "The saved Bot Token cannot be decrypted on this server. Enter and save it again.", ex);
        }

        return new TelegramNotificationSettings
        {
            Enabled = reader.GetBoolean(0),
            BotToken = token,
            HasStoredBotToken = protectedToken.Length > 0,
            ChannelId = reader.IsDBNull(2) ? string.Empty : reader.GetString(2),
            UniqueSpawnEnabled = reader.GetBoolean(3),
            UniqueKillEnabled = reader.GetBoolean(4),
            ShowKillerName = reader.GetBoolean(5),
            EventReminderEnabled = reader.GetBoolean(6),
            ReminderMinutes = reader.GetString(7),
            EventStartedEnabled = reader.GetBoolean(8),
            EventFinishedEnabled = reader.GetBoolean(9),
            ServerOnlineEnabled = reader.GetBoolean(10),
            FortressWarEnabled = reader.GetBoolean(11),
            UpdatedAtUtc = reader.GetDateTime(12)
        };
    }

    public async Task SaveTelegramSettingsAsync(TelegramNotificationSettings settings)
    {
        ValidateTelegramSettings(settings);
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTelegramSchemaAsync(connection);

        var protectedToken = string.IsNullOrWhiteSpace(settings.BotToken)
            ? null
            : LocalMachineSecretProtector.Protect(settings.BotToken.Trim());

        await using var command = new SqlCommand(@"
UPDATE dbo.TelegramNotificationSettings
SET Enabled = @Enabled,
    BotTokenProtected = CASE WHEN @BotTokenProtected IS NULL THEN BotTokenProtected ELSE @BotTokenProtected END,
    ChannelId = @ChannelId,
    UniqueSpawnEnabled = @UniqueSpawnEnabled,
    UniqueKillEnabled = @UniqueKillEnabled,
    ShowKillerName = @ShowKillerName,
    EventReminderEnabled = @EventReminderEnabled,
    ReminderMinutes = @ReminderMinutes,
    EventStartedEnabled = @EventStartedEnabled,
    EventFinishedEnabled = @EventFinishedEnabled,
    ServerOnlineEnabled = @ServerOnlineEnabled,
    FortressWarEnabled = @FortressWarEnabled,
    UpdatedAtUtc = SYSUTCDATETIME()
WHERE SettingID = 1;", connection);
        command.Parameters.Add("@Enabled", SqlDbType.Bit).Value = settings.Enabled;
        command.Parameters.Add("@BotTokenProtected", SqlDbType.NVarChar, 2048).Value =
            protectedToken == null ? DBNull.Value : protectedToken;
        command.Parameters.Add("@ChannelId", SqlDbType.NVarChar, 128).Value = settings.ChannelId.Trim();
        command.Parameters.Add("@UniqueSpawnEnabled", SqlDbType.Bit).Value = settings.UniqueSpawnEnabled;
        command.Parameters.Add("@UniqueKillEnabled", SqlDbType.Bit).Value = settings.UniqueKillEnabled;
        command.Parameters.Add("@ShowKillerName", SqlDbType.Bit).Value = settings.ShowKillerName;
        command.Parameters.Add("@EventReminderEnabled", SqlDbType.Bit).Value = settings.EventReminderEnabled;
        command.Parameters.Add("@ReminderMinutes", SqlDbType.NVarChar, 128).Value =
            NormalizeReminderMinutes(settings.ReminderMinutes);
        command.Parameters.Add("@EventStartedEnabled", SqlDbType.Bit).Value = settings.EventStartedEnabled;
        command.Parameters.Add("@EventFinishedEnabled", SqlDbType.Bit).Value = settings.EventFinishedEnabled;
        command.Parameters.Add("@ServerOnlineEnabled", SqlDbType.Bit).Value = settings.ServerOnlineEnabled;
        command.Parameters.Add("@FortressWarEnabled", SqlDbType.Bit).Value = settings.FortressWarEnabled;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "SaveTelegramSettings", "Telegram", "Notification preferences updated");
    }

    public async Task<string> SendTelegramTestAsync()
    {
        var settings = await LoadTelegramSettingsAsync();
        if (string.IsNullOrWhiteSpace(settings.BotToken))
            throw new InvalidOperationException("Enter and save the Bot Token first.");
        if (string.IsNullOrWhiteSpace(settings.ChannelId))
            throw new InvalidOperationException("Enter and save the Channel ID first.");

        using var client = new HttpClient { Timeout = TimeSpan.FromSeconds(12) };
        var endpoint = $"https://api.telegram.org/bot{settings.BotToken}/sendMessage";
        var payload = JsonSerializer.Serialize(new
        {
            chat_id = settings.ChannelId,
            text = "<b>✅ KMTGUARD TELEGRAM CONNECTED</b>\n\n" +
                   "Your notification channel is configured correctly and ready to receive live server alerts.",
            parse_mode = "HTML",
            disable_web_page_preview = true
        });
        using var response = await client.PostAsync(
            endpoint,
            new StringContent(payload, Encoding.UTF8, "application/json"));
        var body = await response.Content.ReadAsStringAsync();
        if (!response.IsSuccessStatusCode)
            throw new InvalidOperationException(
                $"Telegram rejected the test message (HTTP {(int)response.StatusCode}): {TrimForAudit(body, 700)}");

        return "Connection verified. The test message was delivered to the Telegram channel.";
    }

    public async Task QueueManualTelegramAnnouncementAsync(string title, string message)
    {
        title = title.Trim();
        message = message.Trim();
        if (title.Length == 0)
            throw new InvalidOperationException("Announcement title is required.");
        if (message.Length == 0)
            throw new InvalidOperationException("Announcement message is required.");
        if (title.Length > 120)
            throw new InvalidOperationException("Announcement title cannot exceed 120 characters.");
        if (message.Length > 3000)
            throw new InvalidOperationException("Announcement message cannot exceed 3,000 characters.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTelegramSchemaAsync(connection);
        var html = $"<b>📢 {WebUtility.HtmlEncode(title)}</b>\n\n{WebUtility.HtmlEncode(message)}";
        await using var command = new SqlCommand(
            "EXEC dbo.Telegram_Notification @Message = @Message, @Category = N'Manual', @EventKey = @EventKey;",
            connection);
        command.Parameters.Add("@EventKey", SqlDbType.NVarChar, 180).Value = $"manual:{Guid.NewGuid():N}";
        command.Parameters.Add("@Message", SqlDbType.NVarChar, 4000).Value = html;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "QueueTelegramAnnouncement", title, message);
    }

    public async Task<DataTable> LoadTelegramDeliveryLogAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTelegramSchemaAsync(connection);
        await using var command = new SqlCommand(@"
SELECT TOP (250)
    NotificationID,
    Category,
    CASE Status WHEN 0 THEN N'Pending' WHEN 1 THEN N'Processing'
                WHEN 2 THEN N'Sent' ELSE N'Failed' END AS DeliveryStatus,
    Attempts,
    CreatedAtUtc,
    SentAtUtc,
    LastError
FROM dbo.TelegramNotificationQueue WITH (NOLOCK)
ORDER BY NotificationID DESC;", connection);
        await using var reader = await command.ExecuteReaderAsync();
        var table = new DataTable();
        table.Load(reader);
        return table;
    }

    public async Task RetryFailedTelegramNotificationsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTelegramSchemaAsync(connection);
        await using var command = new SqlCommand(@"
UPDATE dbo.TelegramNotificationQueue
SET Status = 0, Attempts = 0, NextAttemptUtc = SYSUTCDATETIME(),
    ProcessingAtUtc = NULL, LastError = NULL
WHERE Status = 3;", connection);
        var affected = await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "RetryTelegramNotifications", "Failed", affected.ToString());
    }

    public async Task<DataTable> LoadTelegramUniqueNamesAsync(string term)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTelegramSchemaAsync(connection);
        await using var command = new SqlCommand(@"
SELECT TOP (250)
    MobID,
    CodeName128,
    NameStrID128,
    DisplayName,
    CASE WHEN NULLIF(LTRIM(RTRIM(DisplayName)), N'') IS NULL
         THEN N'Needs display name' ELSE N'Ready' END AS NameStatus,
    IsCustom,
    UpdatedAtUtc
FROM dbo.TelegramUniqueDisplayNames WITH (NOLOCK)
WHERE @Term = N''
   OR CONVERT(nvarchar(20), MobID) = @Term
   OR CodeName128 LIKE N'%' + @Term + N'%'
   OR NameStrID128 LIKE N'%' + @Term + N'%'
   OR DisplayName LIKE N'%' + @Term + N'%'
ORDER BY CASE WHEN DisplayName IS NULL THEN 0 ELSE 1 END, UpdatedAtUtc DESC;", connection);
        command.Parameters.Add("@Term", SqlDbType.NVarChar, 128).Value = term.Trim();
        await using var reader = await command.ExecuteReaderAsync();
        var table = new DataTable();
        table.Load(reader);
        return table;
    }

    public async Task SaveTelegramUniqueDisplayNameAsync(int mobId, string displayName)
    {
        displayName = displayName.Trim();
        if (mobId <= 0)
            throw new InvalidOperationException("Enter a valid Mob ID.");
        if (displayName.Length is < 2 or > 128)
            throw new InvalidOperationException("Display name must contain 2 to 128 characters.");
        if (displayName.StartsWith("MOB_", StringComparison.OrdinalIgnoreCase) ||
            displayName.StartsWith("SN_", StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidOperationException("Enter the real English display name, not a CodeName or text key.");
        }

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTelegramSchemaAsync(connection);
        var shardDb = QuoteDb(await ReadSettingAsync(connection, "ShardDB", "SRO_VT_SHARD"));
        await using var command = new SqlCommand($@"
DECLARE @CodeName nvarchar(128), @NameStrID nvarchar(128);
SELECT @CodeName = CONVERT(nvarchar(128), CodeName128),
       @NameStrID = CONVERT(nvarchar(128), NameStrID128)
FROM {shardDb}.dbo._RefObjCommon WITH (NOLOCK)
WHERE ID = @MobID;

MERGE dbo.TelegramUniqueDisplayNames AS target
USING (SELECT @MobID AS MobID) AS source ON source.MobID = target.MobID
WHEN MATCHED THEN UPDATE SET
    CodeName128 = COALESCE(@CodeName, target.CodeName128),
    NameStrID128 = COALESCE(@NameStrID, target.NameStrID128),
    DisplayName = @DisplayName,
    IsCustom = 1,
    UpdatedAtUtc = SYSUTCDATETIME()
WHEN NOT MATCHED THEN INSERT
    (MobID, CodeName128, NameStrID128, DisplayName, IsCustom, UpdatedAtUtc)
VALUES
    (@MobID, @CodeName, @NameStrID, @DisplayName, 1, SYSUTCDATETIME());", connection);
        command.Parameters.Add("@MobID", SqlDbType.Int).Value = mobId;
        command.Parameters.Add("@DisplayName", SqlDbType.NVarChar, 128).Value = displayName;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "SaveTelegramUniqueName", mobId.ToString(), displayName);
    }

    private static void ValidateTelegramSettings(TelegramNotificationSettings settings)
    {
        ArgumentNullException.ThrowIfNull(settings);

        var channelId = settings.ChannelId.Trim();
        if (settings.Enabled && string.IsNullOrWhiteSpace(settings.BotToken) &&
            !settings.HasStoredBotToken)
        {
            throw new InvalidOperationException("A Bot Token is required before Telegram notifications can be enabled.");
        }

        if (settings.Enabled && channelId.Length == 0)
            throw new InvalidOperationException("A Channel ID is required before Telegram notifications can be enabled.");

        if (channelId.Length > 0 &&
            !Regex.IsMatch(channelId, @"^(?:-\d{5,20}|@[A-Za-z][A-Za-z0-9_]{3,31})$",
                RegexOptions.CultureInvariant))
        {
            throw new InvalidOperationException(
                "Channel ID must be a numeric Telegram chat ID such as -1001234567890 or a public @channel username.");
        }

        if (!string.IsNullOrWhiteSpace(settings.BotToken) &&
            !Regex.IsMatch(settings.BotToken.Trim(), @"^\d{5,15}:[A-Za-z0-9_-]{20,}$",
                RegexOptions.CultureInvariant))
        {
            throw new InvalidOperationException("The Bot Token format is not valid.");
        }

        _ = NormalizeReminderMinutes(settings.ReminderMinutes);
    }

    private static string NormalizeReminderMinutes(string? value)
    {
        var entries = (value ?? string.Empty)
            .Split(',', StringSplitOptions.TrimEntries | StringSplitOptions.RemoveEmptyEntries);
        if (entries.Length == 0)
            throw new InvalidOperationException("Enter at least one reminder time, for example: 15, 5.");

        var minutes = new List<int>();
        foreach (var entry in entries)
        {
            if (!int.TryParse(entry, out var minute) || minute is < 1 or > 1440)
                throw new InvalidOperationException("Reminder times must be whole minutes between 1 and 1440.");
            if (!minutes.Contains(minute))
                minutes.Add(minute);
        }

        if (minutes.Count > 12)
            throw new InvalidOperationException("A maximum of 12 reminder times is supported.");

        return string.Join(",", minutes.OrderByDescending(item => item));
    }

    private static string TrimForAudit(string? value, int maximumLength)
    {
        value ??= string.Empty;
        return value.Length <= maximumLength ? value : value[..maximumLength];
    }

    private static async Task EnsureTelegramSchemaAsync(SqlConnection connection)
    {
        await using var command = new SqlCommand(TelegramSchemaSql, connection)
        {
            CommandTimeout = 60
        };
        await command.ExecuteNonQueryAsync();
    }

    private static async Task EnsureDiscordSchemaAsync(SqlConnection connection)
    {
        foreach (var sql in new[]
                 {
                     DiscordSchemaSql,
                     DiscordAddChannelProcedureSql,
                     DiscordNotificationProcedureSql
                 })
        {
            await using var command = new SqlCommand(sql, connection)
            {
                CommandTimeout = 60
            };
            await command.ExecuteNonQueryAsync();
        }
    }

    private const string TelegramSchemaSql = @"
IF OBJECT_ID(N'dbo.TelegramNotificationSettings', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.TelegramNotificationSettings
    (
        SettingID tinyint NOT NULL CONSTRAINT PK_TelegramNotificationSettings PRIMARY KEY,
        Enabled bit NOT NULL CONSTRAINT DF_TelegramSettings_Enabled DEFAULT (0),
        BotTokenProtected nvarchar(2048) NULL,
        ChannelId nvarchar(128) NULL,
        UniqueSpawnEnabled bit NOT NULL CONSTRAINT DF_TelegramSettings_UniqueSpawn DEFAULT (1),
        UniqueKillEnabled bit NOT NULL CONSTRAINT DF_TelegramSettings_UniqueKill DEFAULT (1),
        ShowKillerName bit NOT NULL CONSTRAINT DF_TelegramSettings_ShowKiller DEFAULT (1),
        EventReminderEnabled bit NOT NULL CONSTRAINT DF_TelegramSettings_EventReminder DEFAULT (1),
        ReminderMinutes nvarchar(128) NOT NULL CONSTRAINT DF_TelegramSettings_Reminders DEFAULT (N'15,5'),
        EventStartedEnabled bit NOT NULL CONSTRAINT DF_TelegramSettings_EventStarted DEFAULT (1),
        EventFinishedEnabled bit NOT NULL CONSTRAINT DF_TelegramSettings_EventFinished DEFAULT (1),
        ServerOnlineEnabled bit NOT NULL CONSTRAINT DF_TelegramSettings_ServerOnline DEFAULT (0),
        FortressWarEnabled bit NOT NULL CONSTRAINT DF_TelegramSettings_Fortress DEFAULT (1),
        UpdatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_TelegramSettings_Updated DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT CK_TelegramSettings_SingleRow CHECK (SettingID = 1)
    );
END;

IF NOT EXISTS (SELECT 1 FROM dbo.TelegramNotificationSettings WHERE SettingID = 1)
    INSERT dbo.TelegramNotificationSettings (SettingID) VALUES (1);

IF OBJECT_ID(N'dbo.TelegramNotificationQueue', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.TelegramNotificationQueue
    (
        NotificationID bigint IDENTITY(1,1) NOT NULL CONSTRAINT PK_TelegramNotificationQueue PRIMARY KEY,
        EventKey nvarchar(180) NOT NULL,
        Category nvarchar(32) NOT NULL,
        MessageHtml nvarchar(4000) NOT NULL,
        Status tinyint NOT NULL CONSTRAINT DF_TelegramQueue_Status DEFAULT (0),
        Attempts int NOT NULL CONSTRAINT DF_TelegramQueue_Attempts DEFAULT (0),
        NextAttemptUtc datetime2(0) NOT NULL CONSTRAINT DF_TelegramQueue_NextAttempt DEFAULT (SYSUTCDATETIME()),
        ProcessingAtUtc datetime2(0) NULL,
        CreatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_TelegramQueue_Created DEFAULT (SYSUTCDATETIME()),
        SentAtUtc datetime2(0) NULL,
        LastError nvarchar(1000) NULL,
        CONSTRAINT CK_TelegramQueue_Status CHECK (Status BETWEEN 0 AND 3),
        CONSTRAINT CK_TelegramQueue_Attempts CHECK (Attempts >= 0)
    );
    CREATE UNIQUE INDEX UX_TelegramNotificationQueue_EventKey
        ON dbo.TelegramNotificationQueue(EventKey);
    CREATE INDEX IX_TelegramNotificationQueue_Delivery
        ON dbo.TelegramNotificationQueue(Status, NextAttemptUtc, NotificationID)
        INCLUDE (Attempts, Category);
END;

IF OBJECT_ID(N'dbo.TelegramUniqueDisplayNames', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.TelegramUniqueDisplayNames
    (
        MobID int NOT NULL CONSTRAINT PK_TelegramUniqueDisplayNames PRIMARY KEY,
        CodeName128 nvarchar(128) NULL,
        NameStrID128 nvarchar(128) NULL,
        DisplayName nvarchar(128) NULL,
        IsCustom bit NOT NULL CONSTRAINT DF_TelegramUniqueNames_Custom DEFAULT (0),
        UpdatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_TelegramUniqueNames_Updated DEFAULT (SYSUTCDATETIME())
    );
END;";

    private const string DiscordSchemaSql = @"
IF OBJECT_ID(N'dbo.DiscordNotificationSettings', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.DiscordNotificationSettings
    (
        SettingID tinyint NOT NULL CONSTRAINT PK_DiscordNotificationSettings PRIMARY KEY,
        Enabled bit NOT NULL CONSTRAINT DF_DiscordSettings_Enabled DEFAULT (0),
        BotTokenProtected nvarchar(2048) NULL,
        UpdatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_DiscordSettings_Updated DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT CK_DiscordSettings_SingleRow CHECK (SettingID = 1)
    );
END;
IF NOT EXISTS (SELECT 1 FROM dbo.DiscordNotificationSettings WHERE SettingID = 1)
    INSERT dbo.DiscordNotificationSettings (SettingID) VALUES (1);

IF OBJECT_ID(N'dbo.DiscordNotificationChannels', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.DiscordNotificationChannels
    (
        ChannelRecordID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_DiscordNotificationChannels PRIMARY KEY,
        ChannelName nvarchar(80) NOT NULL,
        DiscordChannelID varchar(32) NOT NULL,
        Enabled bit NOT NULL CONSTRAINT DF_DiscordChannels_Enabled DEFAULT (1),
        SortOrder int NOT NULL CONSTRAINT DF_DiscordChannels_SortOrder DEFAULT (0),
        CreatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_DiscordChannels_Created DEFAULT (SYSUTCDATETIME()),
        UpdatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_DiscordChannels_Updated DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT UQ_DiscordChannels_Name UNIQUE (ChannelName),
        CONSTRAINT UQ_DiscordChannels_DiscordID UNIQUE (DiscordChannelID),
        CONSTRAINT CK_DiscordChannels_Name CHECK (LEN(LTRIM(RTRIM(ChannelName))) BETWEEN 2 AND 80),
        CONSTRAINT CK_DiscordChannels_ID CHECK
        (
            LEN(DiscordChannelID) BETWEEN 17 AND 20
            AND DiscordChannelID NOT LIKE '%[^0-9]%'
        )
    );
END;

IF OBJECT_ID(N'dbo.DiscordNotificationQueue', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.DiscordNotificationQueue
    (
        NotificationID bigint IDENTITY(1,1) NOT NULL CONSTRAINT PK_DiscordNotificationQueue PRIMARY KEY,
        EventKey nvarchar(180) NOT NULL
            CONSTRAINT DF_DiscordQueue_EventKey DEFAULT (CONVERT(nvarchar(36), NEWID())),
        ChannelName nvarchar(80) NOT NULL,
        MessageText nvarchar(2000) NOT NULL,
        Status tinyint NOT NULL CONSTRAINT DF_DiscordQueue_Status DEFAULT (0),
        Attempts int NOT NULL CONSTRAINT DF_DiscordQueue_Attempts DEFAULT (0),
        NextAttemptUtc datetime2(0) NOT NULL CONSTRAINT DF_DiscordQueue_NextAttempt DEFAULT (SYSUTCDATETIME()),
        ProcessingAtUtc datetime2(0) NULL,
        CreatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_DiscordQueue_Created DEFAULT (SYSUTCDATETIME()),
        SentAtUtc datetime2(0) NULL,
        LastError nvarchar(1000) NULL,
        CONSTRAINT CK_DiscordQueue_Status CHECK (Status BETWEEN 0 AND 3),
        CONSTRAINT CK_DiscordQueue_Attempts CHECK (Attempts >= 0),
        CONSTRAINT CK_DiscordQueue_Message CHECK (LEN(LTRIM(RTRIM(MessageText))) BETWEEN 1 AND 2000)
    );
    CREATE UNIQUE INDEX UX_DiscordNotificationQueue_EventKey
        ON dbo.DiscordNotificationQueue(EventKey);
    CREATE INDEX IX_DiscordNotificationQueue_Delivery
        ON dbo.DiscordNotificationQueue(Status, NextAttemptUtc, NotificationID)
        INCLUDE (Attempts, ChannelName);
END;";

    private const string DiscordAddChannelProcedureSql = @"
CREATE OR ALTER PROCEDURE dbo.Discord_AddChannel
    @ChannelName nvarchar(80),
    @DiscordChannelID varchar(32),
    @Enabled bit = 1,
    @SortOrder int = 0
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;
    SET @ChannelName = NULLIF(LTRIM(RTRIM(@ChannelName)), N'');
    SET @DiscordChannelID = NULLIF(LTRIM(RTRIM(@DiscordChannelID)), '');
    IF @ChannelName IS NULL OR LEN(@ChannelName) < 2
        THROW 51101, 'Discord channel name must contain at least 2 characters.', 1;
    IF @DiscordChannelID IS NULL OR LEN(@DiscordChannelID) NOT BETWEEN 17 AND 20
       OR @DiscordChannelID LIKE '%[^0-9]%'
        THROW 51102, 'Discord Channel ID must contain 17 to 20 digits.', 1;
    IF EXISTS
    (
        SELECT 1 FROM dbo.DiscordNotificationChannels
        WHERE DiscordChannelID = @DiscordChannelID AND ChannelName <> @ChannelName
    )
        THROW 51103, 'This Discord Channel ID is already assigned to another channel name.', 1;
    MERGE dbo.DiscordNotificationChannels WITH (HOLDLOCK) AS target
    USING (SELECT @ChannelName AS ChannelName) AS source
       ON target.ChannelName = source.ChannelName
    WHEN MATCHED THEN UPDATE SET
        DiscordChannelID = @DiscordChannelID,
        Enabled = @Enabled,
        SortOrder = @SortOrder,
        UpdatedAtUtc = SYSUTCDATETIME()
    WHEN NOT MATCHED THEN INSERT
        (ChannelName, DiscordChannelID, Enabled, SortOrder, CreatedAtUtc, UpdatedAtUtc)
    VALUES
        (@ChannelName, @DiscordChannelID, @Enabled, @SortOrder, SYSUTCDATETIME(), SYSUTCDATETIME());
    SELECT ChannelRecordID, ChannelName, DiscordChannelID, Enabled, SortOrder, UpdatedAtUtc
    FROM dbo.DiscordNotificationChannels WHERE ChannelName = @ChannelName;
END;";

    private const string DiscordNotificationProcedureSql = @"
CREATE OR ALTER PROCEDURE dbo.Discord_Notification
    @ChannelName nvarchar(80),
    @Message nvarchar(2000),
    @EventKey nvarchar(180) = NULL
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;
    SET @ChannelName = NULLIF(LTRIM(RTRIM(@ChannelName)), N'');
    SET @Message = NULLIF(LTRIM(RTRIM(@Message)), N'');
    SET @EventKey = NULLIF(LTRIM(RTRIM(@EventKey)), N'');
    IF @ChannelName IS NULL
        THROW 51111, 'Discord channel name is required.', 1;
    IF NOT EXISTS
    (
        SELECT 1 FROM dbo.DiscordNotificationChannels WITH (UPDLOCK, HOLDLOCK)
        WHERE ChannelName = @ChannelName AND Enabled = 1
    )
        THROW 51112, 'The requested Discord notification channel does not exist or is disabled.', 1;
    IF @Message IS NULL
        THROW 51113, 'Discord notification message is required.', 1;
    IF LEN(@Message) > 2000
        THROW 51114, 'Discord notification message cannot exceed 2000 characters.', 1;
    IF @EventKey IS NULL
        SET @EventKey = CONCAT(N'discord-procedure:', CONVERT(nvarchar(36), NEWID()));
    IF EXISTS
    (
        SELECT 1 FROM dbo.DiscordNotificationQueue WITH (UPDLOCK, HOLDLOCK)
        WHERE EventKey = @EventKey
    )
    BEGIN
        SELECT NotificationID FROM dbo.DiscordNotificationQueue WHERE EventKey = @EventKey;
        RETURN;
    END;
    INSERT dbo.DiscordNotificationQueue
        (EventKey, ChannelName, MessageText, Status, Attempts, NextAttemptUtc, CreatedAtUtc)
    VALUES
        (@EventKey, @ChannelName, @Message, 0, 0, SYSUTCDATETIME(), SYSUTCDATETIME());
    SELECT CONVERT(bigint, SCOPE_IDENTITY()) AS NotificationID;
END;";

    internal static string EventTable(string tableName)
    {
        if (!Regex.IsMatch(tableName, @"^_(AutoEvent|SurvivalParty|SurvivalSolo|CompetitiveEvent|HideAndSeek)[A-Za-z0-9_]*$"))
            throw new InvalidOperationException($"Unsafe Auto Event table name: {tableName}");

        return $"{QuoteDb(EventsDatabaseName)}.dbo.[{tableName}]";
    }

    private static string EventObjectName(string tableName) => $"{EventsDatabaseName}.dbo.{tableName}";

    public async Task<DbHealth> TestAsync()
    {
        var health = new DbHealth();

        try
        {
            await using var connection = new SqlConnection(_connectionString);
            await connection.OpenAsync();

            health.IsConnected = true;
            health.ServerVersion = await ScalarStringAsync(connection, "SELECT CONVERT(NVARCHAR(256), SERVERPROPERTY('ProductVersion'))");
            health.TableCount = await ScalarIntAsync(connection, "SELECT COUNT(*) FROM sys.tables");
            health.SettingCount = await CountRowsIfExistsAsync(connection, "[dbo].[System_Settings]");
            health.PendingCommands = await ScalarIntAsync(connection,
                "IF OBJECT_ID(N'[dbo].[Command_FilterQueue]', N'U') IS NULL SELECT 0 ELSE SELECT COUNT(*) FROM [dbo].[Command_FilterQueue] WHERE Status = 1");
            health.InvalidSettings = await ScalarIntAsync(connection,
                @"IF OBJECT_ID(N'dbo.vw_Settings_InvalidValues', N'V') IS NULL
                      SELECT 0
                  ELSE
                      SELECT COUNT(*)
                      FROM dbo.vw_Settings_InvalidValues
                      WHERE ExpectedType <> N'unknown'");
            health.Message = "Connected";
        }
        catch (Exception ex)
        {
            health.Message = ex.Message;
        }

        return health;
    }

    public async Task<DashboardOperationsOverview> LoadDashboardOperationsAsync()
    {
        var overview = new DashboardOperationsOverview();

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        overview.ActiveEvents =
            await CountRowsWhereIfExistsAsync(connection, EventObjectName("_AutoEventConfig"), "Enabled = 1") +
            await CountRowsWhereIfExistsAsync(connection, EventObjectName("_SurvivalPartyConfig"), "Enabled = 1") +
            await CountRowsWhereIfExistsAsync(connection, EventObjectName("_SurvivalSoloConfig"), "Enabled = 1") +
            await CountRowsWhereIfExistsAsync(connection, EventObjectName("_HideAndSeekConfig"), "Enabled = 1") +
            await CountRowsWhereIfExistsAsync(connection, EventObjectName("_CompetitiveEventConfig"), "Enabled = 1");

        overview.ActiveSchedules =
            await CountRowsWhereIfExistsAsync(connection, "[dbo].[System_Schedule]", "IsEnabled = 1") +
            await CountRowsWhereIfExistsAsync(connection, EventObjectName("_AutoEventSchedule"), "IsActive = 1") +
            await CountRowsWhereIfExistsAsync(connection, EventObjectName("_SurvivalPartySchedule"), "IsActive = 1") +
            await CountRowsWhereIfExistsAsync(connection, EventObjectName("_SurvivalSoloSchedule"), "IsActive = 1") +
            await CountRowsWhereIfExistsAsync(connection, EventObjectName("_HideAndSeekSchedule"), "IsActive = 1") +
            await CountRowsWhereIfExistsAsync(connection, EventObjectName("_CompetitiveEventSchedule"), "IsActive = 1");

        if (await ColumnExistsAsync(connection, "[dbo].[System_Schedule]", "LastStatus"))
        {
            overview.FailedSchedules = await CountRowsWhereIfExistsAsync(
                connection,
                "[dbo].[System_Schedule]",
                "LastStatus IN (N'Failed', N'Error', N'TimedOut')");
        }
        overview.PendingTelegramNotifications = await CountRowsWhereIfExistsAsync(
            connection,
            "[dbo].[TelegramNotificationQueue]",
            "Status IN (0, 1)");
        overview.FailedTelegramNotifications = await CountRowsWhereIfExistsAsync(
            connection,
            "[dbo].[TelegramNotificationQueue]",
            "Status = 3");
        overview.SecurityActionsLast24Hours = await CountRowsWhereIfExistsAsync(
            connection,
            "[dbo].[Security_BotProtectionLog]",
            "CreatedAtUtc >= DATEADD(HOUR, -24, SYSUTCDATETIME())");

        overview.UpcomingEvents = await LoadUpcomingDashboardEventsAsync(connection);
        overview.RecentAdminActivity = await LoadRecentDashboardActivityAsync(connection);
        return overview;
    }

    public async Task<IReadOnlyList<SettingEntry>> LoadSettingsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        var settings = new List<SettingEntry>();
        if (await ObjectExistsAsync(connection, "[dbo].[System_Settings]", "U"))
        {
            var useView = await ObjectExistsAsync(connection, "dbo.vw_Settings_Organized", "V");
            var sql = useView
                ? @"SELECT ISNULL(Category, 'Unsorted') AS Category,
                       ISNULL(DisplayOrder, 999) AS DisplayOrder,
                       SettingName,
                       CONVERT(NVARCHAR(512), Value) AS Value,
                       ISNULL(CONVERT(NVARCHAR(256), Description), '') AS Description
                FROM dbo.vw_Settings_Organized
                ORDER BY DisplayOrder, Category, SettingName"
                : @"SELECT 'Unsorted' AS Category,
                       999 AS DisplayOrder,
                       SettingName,
                       CONVERT(NVARCHAR(512), Value) AS Value,
                       '' AS Description
                FROM [dbo].[System_Settings]
                ORDER BY SettingName";

            await using var command = new SqlCommand(sql, connection);
            await using var reader = await command.ExecuteReaderAsync();
            while (await reader.ReadAsync())
            {
                settings.Add(new SettingEntry
                {
                    Store = SettingStore.Filter,
                    Category = reader.GetString(0),
                    DisplayOrder = reader.GetInt32(1),
                    SettingName = reader.GetString(2),
                    Value = reader.GetString(3),
                    Description = reader.GetString(4)
                });
            }
        }

        if (await ObjectExistsAsync(connection, "[dbo].[System_GameServerSettings]", "U"))
        {
            const string gameServerSql = @"
SELECT N'GameServer.Patch', ID, SettingName,
       CONVERT(NVARCHAR(512), Value), N''
FROM dbo.System_GameServerSettings WITH (NOLOCK)
ORDER BY ID, SettingName;";
            await using var command = new SqlCommand(gameServerSql, connection);
            await using var reader = await command.ExecuteReaderAsync();
            while (await reader.ReadAsync())
            {
                settings.Add(new SettingEntry
                {
                    Store = SettingStore.GameServer,
                    Category = reader.GetString(0),
                    DisplayOrder = reader.GetInt32(1),
                    SettingName = reader.GetString(2),
                    Value = reader.GetString(3),
                    Description = reader.GetString(4)
                });
            }
        }

        return settings;
    }

    public async Task UpdateSettingAsync(SettingStore store, string settingName, string value)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        var tableName = store switch
        {
            SettingStore.Filter => "[dbo].[System_Settings]",
            SettingStore.GameServer => "[dbo].[System_GameServerSettings]",
            _ => throw new InvalidOperationException("Unsupported setting store.")
        };
        var sql = $"UPDATE {tableName} SET Value = @Value WHERE SettingName = @SettingName";
        await using var command = new SqlCommand(sql, connection);
        command.Parameters.Add("@Value", SqlDbType.NVarChar, 512).Value = value;
        command.Parameters.Add("@SettingName", SqlDbType.NVarChar, 128).Value = settingName;

        var affected = await command.ExecuteNonQueryAsync();
        if (affected == 0)
            throw new InvalidOperationException($"Setting '{settingName}' was not found.");

        await AuditAsync(connection, "UpdateSetting", $"{store}:{settingName}", value);
    }

    public async Task<IReadOnlyList<ProxyServiceEntry>> LoadProxyServicesAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        var tableName = await ResolveProxyServicesTableAsync(connection);
        if (tableName is null)
            return Array.Empty<ProxyServiceEntry>();

        await using var command = new SqlCommand($@"
SELECT
    ServiceId,
    Name,
    CONVERT(INT, ServerType) AS ServerType,
    RemoteIP,
    RemotePort,
    BindIP,
    BindPort,
    ByteLimitation,
    AutoStart
FROM {tableName} WITH (NOLOCK)
ORDER BY
    CASE CONVERT(INT, ServerType)
        WHEN 3 THEN 1
        WHEN 1 THEN 2
        WHEN 2 THEN 3
        ELSE 4
    END,
    ServiceId;", connection);

        await using var reader = await command.ExecuteReaderAsync();
        var services = new List<ProxyServiceEntry>();
        while (await reader.ReadAsync())
        {
            services.Add(new ProxyServiceEntry
            {
                ServiceId = reader.GetInt32(0),
                Name = reader.GetString(1),
                ServerType = reader.GetInt32(2),
                RemoteIP = reader.GetString(3),
                RemotePort = reader.GetInt32(4),
                BindIP = reader.GetString(5),
                BindPort = reader.GetInt32(6),
                ByteLimitation = reader.GetInt32(7),
                AutoStart = reader.GetBoolean(8)
            });
        }

        return services;
    }

    public async Task UpdateProxyServiceAsync(ProxyServiceEntry service)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        var tableName = await ResolveProxyServicesTableAsync(connection)
            ?? throw new InvalidOperationException("System_ProxyServices was not found.");

        await using var command = new SqlCommand($@"
UPDATE {tableName}
SET
    RemoteIP = @RemoteIP,
    RemotePort = @RemotePort,
    BindIP = @BindIP,
    BindPort = @BindPort,
    ByteLimitation = @ByteLimitation,
    AutoStart = @AutoStart
WHERE ServiceId = @ServiceId;", connection);

        command.Parameters.Add("@RemoteIP", SqlDbType.NVarChar, 64).Value = service.RemoteIP;
        command.Parameters.Add("@RemotePort", SqlDbType.Int).Value = service.RemotePort;
        command.Parameters.Add("@BindIP", SqlDbType.NVarChar, 64).Value = service.BindIP;
        command.Parameters.Add("@BindPort", SqlDbType.Int).Value = service.BindPort;
        command.Parameters.Add("@ByteLimitation", SqlDbType.Int).Value = service.ByteLimitation;
        command.Parameters.Add("@AutoStart", SqlDbType.Bit).Value = service.AutoStart;
        command.Parameters.Add("@ServiceId", SqlDbType.Int).Value = service.ServiceId;

        var affected = await command.ExecuteNonQueryAsync();
        if (affected == 0)
            throw new InvalidOperationException($"Proxy service #{service.ServiceId} was not found.");

        await AuditAsync(connection,
            "UpdateProxyService",
            $"{service.Name} ({GetProxyServiceRole(service.ServerType)})",
            $"{service.RemoteIP}:{service.RemotePort} -> {service.BindIP}:{service.BindPort};AutoStart={service.AutoStart}");
    }

    public async Task QueueRuntimeCommandAsync(int commandId, params string?[] data)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        if (!await ObjectExistsAsync(connection, "[dbo].[Command_FilterQueue]", "U"))
            throw new InvalidOperationException("[dbo].[Command_FilterQueue] was not found.");

        const string sql = @"
INSERT INTO [dbo].[Command_FilterQueue]
    (CommandID, Data1, Data2, Data3, Data4, Data5, Data6, Data7, Data8, Data9, Data10, Data11, Status)
VALUES
    (@CommandID, @Data1, @Data2, @Data3, @Data4, @Data5, @Data6, @Data7, @Data8, @Data9, @Data10, @Data11, 1);";

        await using var command = new SqlCommand(sql, connection);
        command.Parameters.Add("@CommandID", SqlDbType.Int).Value = commandId;

        for (var index = 0; index < 11; index++)
        {
            command.Parameters.Add($"@Data{index + 1}", SqlDbType.NVarChar, 512).Value =
                index < data.Length && !string.IsNullOrWhiteSpace(data[index])
                    ? data[index]!
                    : DBNull.Value;
        }

        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "QueueRuntimeCommand", $"CommandID={commandId}", string.Join("|", data.Where(x => !string.IsNullOrWhiteSpace(x))));
    }

    public async Task<PlayerCommandTarget> GetPlayerCommandTargetAsync(string characterName)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        return await ResolvePlayerCommandTargetAsync(connection, characterName);
    }

    public async Task AdjustPlayerGoldAsync(string characterName, long amount, bool add)
    {
        if (amount <= 0 || amount > 2_000_000_000L)
            throw new InvalidOperationException("Gold amount must be between 1 and 2,000,000,000.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        var target = await ResolvePlayerCommandTargetAsync(connection, characterName);
        if (!add && target.Gold < amount)
            throw new InvalidOperationException($"{target.CharacterName} has only {target.Gold:N0} gold.");
        if (!await ObjectExistsAsync(connection, "[dbo].[Live_Gold]", "P"))
            throw new InvalidOperationException("[dbo].[Live_Gold] was not found.");

        await using var command = new SqlCommand(
            "EXEC [dbo].[Live_Gold] @CharID, @Gold, @AddOrRemove;", connection);
        command.Parameters.Add("@CharID", SqlDbType.Int).Value = target.CharId;
        command.Parameters.Add("@Gold", SqlDbType.BigInt).Value = amount;
        command.Parameters.Add("@AddOrRemove", SqlDbType.Int).Value = add ? 1 : 0;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection,
            add ? "PlayerGoldAdd" : "PlayerGoldRemove",
            $"{target.CharacterName} (CharID={target.CharId})",
            $"Amount={amount}");
    }

    public async Task AdjustPlayerSilkAsync(
        string characterName,
        int silkOwn,
        int silkGift,
        int silkPoint,
        bool add)
    {
        if (silkOwn < 0 || silkGift < 0 || silkPoint < 0)
            throw new InvalidOperationException("Silk values cannot be negative.");
        if (silkOwn == 0 && silkGift == 0 && silkPoint == 0)
            throw new InvalidOperationException("Enter at least one silk amount.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        var target = await ResolvePlayerCommandTargetAsync(connection, characterName);
        if (target.UserJid <= 0)
            throw new InvalidOperationException($"{target.CharacterName} is not linked to a valid account JID.");

        var multiplier = add ? 1L : -1L;
        var nextOwn = target.SilkOwn + (multiplier * silkOwn);
        var nextGift = target.SilkGift + (multiplier * silkGift);
        var nextPoint = target.SilkPoint + (multiplier * silkPoint);
        if (nextOwn < 0 || nextGift < 0 || nextPoint < 0)
            throw new InvalidOperationException(
                $"Insufficient silk. Current balance: own {target.SilkOwn:N0}, gift {target.SilkGift:N0}, points {target.SilkPoint:N0}.");
        if (nextOwn > int.MaxValue || nextGift > int.MaxValue || nextPoint > int.MaxValue)
            throw new InvalidOperationException("The requested silk update would exceed the database INT limit.");

        if (!await ObjectExistsAsync(connection, "[dbo].[Live_Silk]", "P"))
            throw new InvalidOperationException("[dbo].[Live_Silk] was not found.");

        await using var command = new SqlCommand(
            "EXEC [dbo].[Live_Silk] @CharID, @SilkOwn, @SilkGift, @SilkPoint;", connection);
        command.Parameters.Add("@CharID", SqlDbType.Int).Value = target.CharId;
        command.Parameters.Add("@SilkOwn", SqlDbType.Int).Value = checked((int)(silkOwn * multiplier));
        command.Parameters.Add("@SilkGift", SqlDbType.Int).Value = checked((int)(silkGift * multiplier));
        command.Parameters.Add("@SilkPoint", SqlDbType.Int).Value = checked((int)(silkPoint * multiplier));
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection,
            add ? "PlayerSilkAdd" : "PlayerSilkRemove",
            $"{target.CharacterName} (CharID={target.CharId};JID={target.UserJid})",
            $"Own={silkOwn};Gift={silkGift};Point={silkPoint}");
    }

    public async Task TeleportPlayerToTownAsync(string characterName)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        var target = await ResolvePlayerCommandTargetAsync(connection, characterName);
        if (!await ObjectExistsAsync(connection, "[dbo].[Teleport_PlayerToTown]", "P"))
            throw new InvalidOperationException("[dbo].[Teleport_PlayerToTown] was not found.");

        await using var command = new SqlCommand(
            "EXEC [dbo].[Teleport_PlayerToTown] @CharID;", connection);
        command.Parameters.Add("@CharID", SqlDbType.Int).Value = target.CharId;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "PlayerTeleportTown",
            $"{target.CharacterName} (CharID={target.CharId})", "Town");
    }

    public async Task TeleportPlayerToPositionAsync(
        string characterName,
        int worldId,
        int regionId,
        int x,
        int y,
        int z)
    {
        if (worldId <= 0)
            throw new InvalidOperationException("World ID must be greater than zero.");
        regionId = NormalizeRegionId(regionId);
        if (!IsValidRegionId(regionId))
            throw new InvalidOperationException("Region ID must be a non-zero signed 16-bit value.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        var target = await ResolvePlayerCommandTargetAsync(connection, characterName);
        if (!await ObjectExistsAsync(connection, "[dbo].[Teleport_Position]", "P"))
            throw new InvalidOperationException("[dbo].[Teleport_Position] was not found.");

        await using var command = new SqlCommand(@"
EXEC [dbo].[Teleport_Position]
    @CharID, @GameWorldID, @RegionID, @PosX, @PosY, @PosZ;", connection);
        command.Parameters.Add("@CharID", SqlDbType.Int).Value = target.CharId;
        command.Parameters.Add("@GameWorldID", SqlDbType.Int).Value = worldId;
        command.Parameters.Add("@RegionID", SqlDbType.Int).Value = regionId;
        command.Parameters.Add("@PosX", SqlDbType.Int).Value = x;
        command.Parameters.Add("@PosY", SqlDbType.Int).Value = y;
        command.Parameters.Add("@PosZ", SqlDbType.Int).Value = z;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "PlayerTeleportPosition",
            $"{target.CharacterName} (CharID={target.CharId})",
            $"World={worldId};Region={regionId};X={x};Y={y};Z={z}");
    }

    public async Task QueuePlayerSelfTeleportAsync(string characterName)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        var target = await ResolvePlayerCommandTargetAsync(connection, characterName);
        if (!await ObjectExistsAsync(connection, "[dbo].[Teleport_Self]", "P"))
            throw new InvalidOperationException("[dbo].[Teleport_Self] was not found.");

        await using var command = new SqlCommand(
            "EXEC [dbo].[Teleport_Self] @CharID;", connection);
        command.Parameters.Add("@CharID", SqlDbType.Int).Value = target.CharId;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "PlayerSelfTeleport",
            $"{target.CharacterName} (CharID={target.CharId})", "CommandID=41");
    }

    public async Task QueuePlayerNoticeAsync(string characterName, byte noticeType, string message)
    {
        if (string.IsNullOrWhiteSpace(message))
            throw new InvalidOperationException("Notice message is required.");

        var target = await GetPlayerCommandTargetAsync(characterName);
        await QueueRuntimeCommandAsync(
            28,
            target.CharId.ToString(),
            message.Trim(),
            noticeType.ToString());
    }

    public async Task QueuePlayerDisconnectAsync(string characterName)
    {
        var target = await GetPlayerCommandTargetAsync(characterName);
        await QueueRuntimeCommandAsync(34, target.CharId.ToString());
    }

    public async Task QueuePlayerNameColorAsync(string characterName, string? color)
    {
        var target = await GetPlayerCommandTargetAsync(characterName);
        if (string.IsNullOrWhiteSpace(color))
        {
            await ExecutePlayerStyleCommandAsync(
                "EXEC dbo.NameColor_Deactivate @CharName16;",
                target,
                command => command.Parameters.Add("@CharName16", SqlDbType.VarChar, 16).Value =
                    target.CharacterName,
                "NameColor_Deactivate");
            return;
        }

        var normalized = color.Trim();
        if (!Regex.IsMatch(normalized, "^#?[0-9A-Fa-f]{6}([0-9A-Fa-f]{2})?$"))
            throw new InvalidOperationException("Color must be a 6 or 8 digit hexadecimal value.");
        if (!normalized.StartsWith('#'))
            normalized = $"#{normalized}";

        await ExecutePlayerStyleCommandAsync(
            @"EXEC dbo.NameColor_Add @CharID, @ColorCode, @ColorName;
              DECLARE @ColorID INT =
              (
                  SELECT TOP (1) ID
                  FROM dbo.PlayerNameColors
                  WHERE CharID = @CharID AND ColorCode = @ColorCode
                  ORDER BY ID
              );
              EXEC dbo.NameColor_Activate @CharID, @CharName16, @ColorID;",
            target,
            command =>
            {
                command.Parameters.Add("@CharID", SqlDbType.Int).Value = target.CharId;
                command.Parameters.Add("@CharName16", SqlDbType.VarChar, 16).Value =
                    target.CharacterName;
                command.Parameters.Add("@ColorCode", SqlDbType.VarChar, 100).Value = normalized;
                command.Parameters.Add("@ColorName", SqlDbType.VarChar, 100).Value = normalized;
            },
            "NameColor_Activate");
    }

    public async Task QueuePlayerTagAsync(string characterName, byte? tagId)
    {
        var target = await GetPlayerCommandTargetAsync(characterName);
        if (tagId is null)
        {
            await ExecutePlayerStyleCommandAsync(
                "EXEC dbo.Tag_Deactivate @CharName16;",
                target,
                command => command.Parameters.Add("@CharName16", SqlDbType.VarChar, 16).Value =
                    target.CharacterName,
                "Tag_Deactivate");
        }
        else
        {
            await ExecutePlayerStyleCommandAsync(
                @"EXEC dbo.Tag_Add @CharID, @TagID;
                  EXEC dbo.Tag_Activate @CharID, @CharName16, @TagID;",
                target,
                command =>
                {
                    command.Parameters.Add("@CharID", SqlDbType.Int).Value = target.CharId;
                    command.Parameters.Add("@CharName16", SqlDbType.VarChar, 16).Value =
                        target.CharacterName;
                    command.Parameters.Add("@TagID", SqlDbType.TinyInt).Value = tagId.Value;
                },
                "Tag_Activate");
        }
    }

    public async Task QueuePlayerIconAsync(
        string characterName,
        bool leftSide,
        int? iconId)
    {
        var target = await GetPlayerCommandTargetAsync(characterName);
        if (iconId is null)
        {
            await ExecutePlayerStyleCommandAsync(
                leftSide
                    ? "EXEC dbo.LeftIcon_Deactivate @CharName16;"
                    : "EXEC dbo.RightIcon_Deactivate @CharName16;",
                target,
                command => command.Parameters.Add("@CharName16", SqlDbType.VarChar, 16).Value =
                    target.CharacterName,
                leftSide ? "LeftIcon_Deactivate" : "RightIcon_Deactivate");
            return;
        }
        if (iconId <= 0)
            throw new InvalidOperationException("Icon ID must be greater than zero.");

        await ExecutePlayerStyleCommandAsync(
            leftSide
                ? @"EXEC dbo.LeftIcon_Add @CharID, @IconID;
                    EXEC dbo.LeftIcon_Activate @CharID, @CharName16, @IconID;"
                : @"EXEC dbo.RightIcon_Add @CharID, @IconID;
                    EXEC dbo.RightIcon_Activate @CharID, @CharName16, @IconID;",
            target,
            command =>
            {
                command.Parameters.Add("@CharID", SqlDbType.Int).Value = target.CharId;
                command.Parameters.Add("@CharName16", SqlDbType.VarChar, 16).Value =
                    target.CharacterName;
                command.Parameters.Add("@IconID", SqlDbType.Int).Value = iconId.Value;
            },
            leftSide ? "LeftIcon_Activate" : "RightIcon_Activate");
    }

    private async Task ExecutePlayerStyleCommandAsync(
        string sql,
        PlayerCommandTarget target,
        Action<SqlCommand> addParameters,
        string action)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await using var command = new SqlCommand(sql, connection);
        addParameters(command);
        await command.ExecuteNonQueryAsync();
        await AuditAsync(
            connection,
            action,
            $"{target.CharacterName} (CharID={target.CharId})",
            "Player style API");
    }

    public async Task<DataTable> LoadClientlessAccountsAsync(
        string term,
        string? city = null,
        string? state = null)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureClientlessSchemaAsync(connection);

        var accountDb = await ReadSettingAsync(connection, "AccountDB", "SRO_VT_ACCOUNT");
        var shardDb = await ReadSettingAsync(connection, "ShardDB", "SRO_VT_SHARD");
        var accountDbName = QuoteDb(accountDb);
        var shardDbName = QuoteDb(shardDb);
        city = string.IsNullOrWhiteSpace(city) || city.Equals("All", StringComparison.OrdinalIgnoreCase)
            ? null
            : NormalizeClientlessTownOrUnassigned(city);
        state = string.IsNullOrWhiteSpace(state) ? "All" : state.Trim();

        return await QueryTableAsync(connection,
               $@"SELECT TOP (1000)
                      Accounts.ID,
                      Accounts.Enabled,
                      CASE WHEN Accounts.HuntEnabled = 1 THEN 'Hunter' ELSE 'City standby' END AS [Hunting role],
                      Accounts.AccountName,
                      Accounts.CharacterName,
                      Accounts.City,
                      CASE WHEN GameCharacter.CharID IS NULL THEN 'Missing game account / character'
                           WHEN Accounts.Enabled = 0 THEN 'Disabled'
                           WHEN Accounts.LastStatus = 'Online' THEN 'Online'
                           ELSE 'Ready' END AS Readiness,
                      GameCharacter.CurLevel AS CharacterLevel,
                      Accounts.Locale,
                      Accounts.ShardID,
                      Accounts.LaunchDelayMs,
                      Accounts.ReconnectDelaySeconds,
                      Accounts.LastStatus,
                      Accounts.LastMessage,
                      Accounts.LastLoginAt,
                      Accounts.LastDisconnectAt,
                      Accounts.UpdatedAt
               FROM [dbo].[Clientless_Accounts] AS Accounts WITH (NOLOCK)
               OUTER APPLY
               (
                   SELECT TOP (1) Characters.CharID, Characters.CurLevel
                   FROM {accountDbName}.dbo.TB_User AS Users WITH (NOLOCK)
                   INNER JOIN {shardDbName}.dbo._User AS UserCharacters WITH (NOLOCK)
                       ON UserCharacters.UserJID = Users.JID
                   INNER JOIN {shardDbName}.dbo._Char AS Characters WITH (NOLOCK)
                       ON Characters.CharID = UserCharacters.CharID
                   WHERE Users.StrUserID COLLATE DATABASE_DEFAULT = Accounts.AccountName COLLATE DATABASE_DEFAULT
                     AND Characters.CharName16 COLLATE DATABASE_DEFAULT = Accounts.CharacterName COLLATE DATABASE_DEFAULT
               ) AS GameCharacter
               WHERE NULLIF(LTRIM(RTRIM(ISNULL(Accounts.SystemRole, ''))), '') IS NULL
                 AND (@Term = N''
                      OR Accounts.AccountName LIKE '%' + @Term + '%'
                      OR Accounts.CharacterName LIKE '%' + @Term + '%'
                      OR Accounts.City LIKE '%' + @Term + '%'
                      OR Accounts.LastStatus LIKE '%' + @Term + '%')
                 AND (@City IS NULL OR Accounts.City = @City)
                 AND
                 (
                     @State = 'All'
                     OR (@State = 'Ready' AND Accounts.Enabled = 1 AND GameCharacter.CharID IS NOT NULL)
                     OR (@State = 'Online' AND Accounts.Enabled = 1 AND Accounts.LastStatus = 'Online')
                     OR (@State = 'Attention' AND Accounts.Enabled = 1 AND
                         (GameCharacter.CharID IS NULL OR Accounts.LastStatus IN ('Disconnected', 'PolicyBlocked')))
                     OR (@State = 'Enabled' AND Accounts.Enabled = 1)
                     OR (@State = 'Disabled' AND Accounts.Enabled = 0)
                 )
               ORDER BY Accounts.City, Accounts.ID;",
            new SqlParameter("@Term", term.Trim()),
            new SqlParameter("@City", SqlDbType.VarChar, 32) { Value = (object?)city ?? DBNull.Value },
            new SqlParameter("@State", SqlDbType.VarChar, 16) { Value = state });
    }

    public async Task<ClientlessOperationsOverview> LoadClientlessOperationsOverviewAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureClientlessSchemaAsync(connection);

        var accountDb = await ReadSettingAsync(connection, "AccountDB", "SRO_VT_ACCOUNT");
        var shardDb = await ReadSettingAsync(connection, "ShardDB", "SRO_VT_SHARD");
        var accountDbName = QuoteDb(accountDb);
        var shardDbName = QuoteDb(shardDb);
        var cities = await QueryTableAsync(connection, $@"
WITH CityCatalog AS
(
    SELECT City, SortOrder
    FROM (VALUES
        ('Jangan', 1),
        ('Donwhang', 2),
        ('Hotan', 3),
        ('SamarKand', 4),
        ('Constantinople', 5),
        ('Alexandria North (SD)', 6),
        ('Unassigned', 7)
    ) AS Cities(City, SortOrder)
),
AccountState AS
(
    SELECT
        ISNULL(NULLIF(LTRIM(RTRIM(Accounts.City)), ''), 'Unassigned') AS City,
        Accounts.Enabled,
        Accounts.HuntEnabled,
        Accounts.LastStatus,
        CASE WHEN GameCharacter.CharID IS NULL THEN 0 ELSE 1 END AS GameReady
    FROM dbo.Clientless_Accounts AS Accounts WITH (NOLOCK)
    OUTER APPLY
    (
        SELECT TOP (1) Characters.CharID
        FROM {accountDbName}.dbo.TB_User AS Users WITH (NOLOCK)
        INNER JOIN {shardDbName}.dbo._User AS UserCharacters WITH (NOLOCK)
            ON UserCharacters.UserJID = Users.JID
        INNER JOIN {shardDbName}.dbo._Char AS Characters WITH (NOLOCK)
            ON Characters.CharID = UserCharacters.CharID
        WHERE Users.StrUserID COLLATE DATABASE_DEFAULT = Accounts.AccountName COLLATE DATABASE_DEFAULT
          AND Characters.CharName16 COLLATE DATABASE_DEFAULT = Accounts.CharacterName COLLATE DATABASE_DEFAULT
    ) AS GameCharacter
    WHERE NULLIF(LTRIM(RTRIM(ISNULL(Accounts.SystemRole, ''))), '') IS NULL
)
SELECT
    Catalog.City,
    COUNT(State.City) AS Total,
    SUM(CASE WHEN State.Enabled = 1 THEN 1 ELSE 0 END) AS Enabled,
    SUM(CASE WHEN State.Enabled = 0 THEN 1 ELSE 0 END) AS Disabled,
    SUM(CASE WHEN State.GameReady = 1 THEN 1 ELSE 0 END) AS Ready,
    SUM(CASE WHEN State.Enabled = 1 AND State.GameReady = 1 AND State.HuntEnabled = 1 THEN 1 ELSE 0 END) AS [Assigned hunters],
    SUM(CASE WHEN State.Enabled = 1 AND State.GameReady = 1 AND State.HuntEnabled = 0 THEN 1 ELSE 0 END) AS [Ready parked],
    SUM(CASE WHEN State.Enabled = 1 AND State.LastStatus = 'Online' THEN 1 ELSE 0 END) AS Online,
    SUM(CASE WHEN State.Enabled = 1 AND
        (State.GameReady = 0 OR State.LastStatus IN ('Disconnected', 'PolicyBlocked')) THEN 1 ELSE 0 END) AS NeedsAttention
FROM CityCatalog AS Catalog
LEFT JOIN AccountState AS State ON State.City = Catalog.City
GROUP BY Catalog.City, Catalog.SortOrder
ORDER BY Catalog.SortOrder;");

        return new ClientlessOperationsOverview
        {
            TotalAccounts = SumClientlessColumn(cities, "Total"),
            EnabledAccounts = SumClientlessColumn(cities, "Enabled"),
            ReadyAccounts = SumClientlessColumn(cities, "Ready"),
            OnlineAccounts = SumClientlessColumn(cities, "Online"),
            NeedsAttentionAccounts = SumClientlessColumn(cities, "NeedsAttention"),
            Cities = cities
        };
    }

    private static int SumClientlessColumn(DataTable table, string columnName) =>
        table.Rows.Cast<DataRow>().Sum(row => Convert.ToInt32(row[columnName]));

    public async Task<string> ImportClientlessAccountsAsync(string filePath, int shardId, byte locale, int launchDelayMs, int reconnectDelaySeconds)
    {
        ValidateClientlessRuntimeFields(shardId, launchDelayMs, reconnectDelaySeconds);
        if (!File.Exists(filePath))
            throw new FileNotFoundException("Clientless account file was not found.", filePath);

        var rows = File.ReadLines(filePath)
            .Select((line, index) => ParseClientlessAccountLine(line, index + 1))
            .Where(row => row is not null)
            .Select(row => row!)
            .ToList();

        if (rows.Count == 0)
            throw new InvalidOperationException("No valid account rows were found in the selected file.");
        if (rows.Any(row => row.AccountPassword.Length is < 1 or > 50))
            throw new InvalidOperationException("Every imported account password must contain 1 to 50 characters.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureClientlessSchemaAsync(connection);
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync();

        var inserted = 0;
        var updated = 0;
        try
        {
            foreach (var row in rows)
            {
                if (await ClientlessAccountExistsAsync(connection, transaction, row.AccountName, row.CharacterName))
                    updated++;
                else
                    inserted++;

                await UpsertClientlessAccountAsync(
                    connection,
                    transaction,
                    row.AccountName,
                    row.AccountPassword,
                    row.CharacterName,
                    row.City,
                    shardId,
                    locale,
                    launchDelayMs,
                    reconnectDelaySeconds);
            }

            await transaction.CommitAsync();
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }

        await AuditAsync(connection, "ImportClientlessAccounts", Path.GetFileName(filePath), $"Imported={rows.Count};Inserted={inserted};Updated={updated}");
        return $"Imported {rows.Count:N0} account(s). Inserted {inserted:N0}, updated {updated:N0}.";
    }

    public async Task SaveClientlessAccountAsync(
        string accountName,
        string password,
        string characterName,
        string city,
        int shardId,
        byte locale,
        int launchDelayMs,
        int reconnectDelaySeconds)
    {
        ValidateClientlessRuntimeFields(shardId, launchDelayMs, reconnectDelaySeconds);
        if (string.IsNullOrWhiteSpace(accountName))
            throw new InvalidOperationException("Account name is required.");
        if (string.IsNullOrWhiteSpace(characterName))
            throw new InvalidOperationException("Character name is required.");
        if (accountName.Trim().Length > 64)
            throw new InvalidOperationException("Account name cannot exceed 64 characters.");
        if (characterName.Trim().Length > 64)
            throw new InvalidOperationException("Character name cannot exceed 64 characters.");
        if (!string.IsNullOrEmpty(password) && password.Length > 50)
            throw new InvalidOperationException("Password cannot exceed 50 characters.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureClientlessSchemaAsync(connection);
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync();
        try
        {
            await UpsertClientlessAccountAsync(
                connection,
                transaction,
                accountName.Trim(),
                password,
                characterName.Trim(),
                NormalizeClientlessTownOrUnassigned(city),
                shardId,
                locale,
                launchDelayMs,
                reconnectDelaySeconds);
            await transaction.CommitAsync();
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }

        await AuditAsync(connection, "SaveClientlessAccount", accountName.Trim(), characterName.Trim());
    }

    public async Task SetClientlessAccountEnabledAsync(int id, bool enabled)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureClientlessSchemaAsync(connection);

        await using var command = new SqlCommand(
            @"UPDATE [dbo].[Clientless_Accounts]
              SET Enabled = @Enabled,
                  LastStatus = CASE WHEN @Enabled = 1 THEN 'Pending' ELSE 'Disabled' END,
                  LastMessage = NULL,
                  UpdatedAt = SYSDATETIME()
              WHERE ID = @ID;", connection);
        command.Parameters.Add("@ID", SqlDbType.Int).Value = id;
        command.Parameters.Add("@Enabled", SqlDbType.Bit).Value = enabled;

        if (await command.ExecuteNonQueryAsync() == 0)
            throw new InvalidOperationException($"Clientless account row #{id} was not found.");

        await AuditAsync(connection, enabled ? "EnableClientlessAccount" : "DisableClientlessAccount", id.ToString(), string.Empty);
    }

    public async Task DeleteClientlessAccountAsync(int id)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureClientlessSchemaAsync(connection);

        await using var command = new SqlCommand("DELETE FROM [dbo].[Clientless_Accounts] WHERE ID = @ID;", connection);
        command.Parameters.Add("@ID", SqlDbType.Int).Value = id;
        if (await command.ExecuteNonQueryAsync() == 0)
            throw new InvalidOperationException($"Clientless account row #{id} was not found.");

        await AuditAsync(connection, "DeleteClientlessAccount", id.ToString(), string.Empty);
    }

    public async Task<int> DeleteAllClientlessAccountsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureClientlessSchemaAsync(connection);

        await using var command = new SqlCommand("DELETE FROM [dbo].[Clientless_Accounts];", connection);
        var deleted = await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "DeleteAllClientlessAccounts", "[dbo].[Clientless_Accounts]", $"Deleted={deleted}");
        return deleted;
    }

    public async Task<ClientlessHuntingWorkspace> LoadClientlessHuntingWorkspaceAsync(string city)
    {
        city = NormalizeClientlessTown(city);
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureClientlessSchemaAsync(connection);
        await EnsureClientlessHuntCityRowsAsync(connection, city);

        var areas = await QueryTableAsync(connection,
            @"SELECT ID, SlotNumber AS [Area], DisplayName AS [Name], Enabled, RegionID,
                     CONVERT(DECIMAL(10,1), PosX) AS X,
                     CONVERT(DECIMAL(10,1), PosY) AS Y,
                     CONVERT(DECIMAL(10,1), PosZ) AS Z,
                     CONVERT(DECIMAL(10,1), Radius) AS Radius
              FROM dbo.Clientless_HuntAreas WITH (NOLOCK)
              WHERE City = @City
              ORDER BY SlotNumber;",
            new SqlParameter("@City", SqlDbType.VarChar, 32) { Value = city });

        var accounts = await QueryTableAsync(connection,
            @"SELECT A.ID, A.CharacterName AS [Character], A.City,
                     CASE WHEN A.Enabled = 1 THEN 'Enabled' ELSE 'Disabled' END AS [Account],
                     CASE WHEN A.HuntEnabled = 1 THEN 'Enabled' ELSE 'Paused' END AS [Hunting],
                     A.HuntStatus AS [Status], ISNULL(A.HuntMessage, '') AS [Details],
                     ISNULL(H.DisplayName, '') AS [Assigned area],
                     ISNULL(A.CurrentTargetName, '') AS [Target], A.LastHuntAt AS [Last activity]
              FROM dbo.Clientless_Accounts A WITH (NOLOCK)
              LEFT JOIN dbo.Clientless_HuntAreas H WITH (NOLOCK) ON H.ID = A.HuntAreaID
              WHERE A.City = @City
                AND NULLIF(LTRIM(RTRIM(ISNULL(A.SystemRole, ''))), '') IS NULL
              ORDER BY A.ID;",
            new SqlParameter("@City", SqlDbType.VarChar, 32) { Value = city });

        ClientlessHuntPolicy policy;
        await using (var command = new SqlCommand(@"
SELECT Enabled, AttackNormal, AttackUnique, UniquePriority, UseSkills, UseBasicAttack,
       HpPotionPercent, MpPotionPercent, StuckSeconds, TargetTimeoutSeconds
FROM dbo.Clientless_HuntPolicy WITH (NOLOCK)
WHERE SettingID = 1;", connection))
        await using (var reader = await command.ExecuteReaderAsync())
        {
            if (!await reader.ReadAsync())
                throw new InvalidOperationException("Clientless hunting policy is missing.");
            policy = new ClientlessHuntPolicy
            {
                Enabled = reader.GetBoolean(0),
                AttackNormal = reader.GetBoolean(1),
                AttackUnique = reader.GetBoolean(2),
                UniquePriority = reader.GetBoolean(3),
                UseSkills = reader.GetBoolean(4),
                UseBasicAttack = reader.GetBoolean(5),
                HpPotionPercent = reader.GetByte(6),
                MpPotionPercent = reader.GetByte(7),
                StuckSeconds = reader.GetInt16(8),
                TargetTimeoutSeconds = reader.GetInt16(9)
            };
        }

        return new ClientlessHuntingWorkspace
        {
            Policy = policy,
            Areas = CreateClientlessHuntAreaEditorTable(areas),
            Accounts = accounts
        };
    }

    public async Task<string> SaveClientlessHuntingAsync(
        string city,
        DataTable areas,
        ClientlessHuntPolicy policy)
    {
        city = NormalizeClientlessTown(city);
        if (areas.Rows.Count != 5)
            throw new InvalidOperationException("Each city must contain exactly five hunting areas.");
        if (!policy.AttackNormal && !policy.AttackUnique)
            throw new InvalidOperationException("Enable normal monsters, uniques, or both.");
        if (!policy.UseSkills && !policy.UseBasicAttack)
            throw new InvalidOperationException("Enable skills, basic attack, or both.");
        if (policy.HpPotionPercent is < 1 or > 99 || policy.MpPotionPercent is < 1 or > 99)
            throw new InvalidOperationException("HP and MP potion values must be between 1 and 99 percent.");
        if (policy.StuckSeconds is < 5 or > 120 || policy.TargetTimeoutSeconds is < 5 or > 300)
            throw new InvalidOperationException("Stuck time must be 5-120 seconds and target timeout 5-300 seconds.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureClientlessSchemaAsync(connection);
        await EnsureClientlessHuntCityRowsAsync(connection, city);
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync();

        try
        {
            var seenSlots = new HashSet<int>();
            foreach (DataRow row in areas.Rows)
            {
                var slot = Convert.ToInt32(row["Area"]);
                if (slot is < 1 or > 5 || !seenSlots.Add(slot))
                    throw new InvalidOperationException("Hunting area numbers must be unique values from 1 to 5.");

                var enabled = Convert.ToBoolean(row["Enabled"]);
                var name = (Convert.ToString(row["Name"]) ?? $"Area {slot}").Trim();
                var regionId = ParseClientlessHuntInteger(row["RegionID"], $"Area {slot} Region ID");
                var x = ParseClientlessHuntNumber(row["X"], $"Area {slot} X");
                var y = ParseClientlessHuntNumber(row["Y"], $"Area {slot} Y");
                var z = ParseClientlessHuntNumber(row["Z"], $"Area {slot} Z");
                var radius = ParseClientlessHuntNumber(row["Radius"], $"Area {slot} radius");
                if (name.Length is < 1 or > 64)
                    throw new InvalidOperationException($"Area {slot} name must contain 1 to 64 characters.");
                regionId = NormalizeRegionId(regionId);
                if (enabled && !IsValidRegionId(regionId))
                    throw new InvalidOperationException($"Area {slot} needs a valid Region ID before it can be enabled.");
                if (!float.IsFinite(x) || !float.IsFinite(y) || !float.IsFinite(z) || !float.IsFinite(radius) || radius is < 5 or > 500)
                    throw new InvalidOperationException($"Area {slot} coordinates must be valid and its radius must be 5-500.");

                await using var areaCommand = new SqlCommand(@"
UPDATE dbo.Clientless_HuntAreas
SET DisplayName = @DisplayName, Enabled = @Enabled, RegionID = @RegionID,
    PosX = @PosX, PosY = @PosY, PosZ = @PosZ, Radius = @Radius, UpdatedAt = SYSDATETIME()
WHERE City = @City AND SlotNumber = @SlotNumber;", connection, transaction);
                areaCommand.Parameters.AddWithValue("@City", city);
                areaCommand.Parameters.AddWithValue("@SlotNumber", slot);
                areaCommand.Parameters.AddWithValue("@DisplayName", name);
                areaCommand.Parameters.AddWithValue("@Enabled", enabled);
                areaCommand.Parameters.AddWithValue("@RegionID", regionId);
                areaCommand.Parameters.AddWithValue("@PosX", x);
                areaCommand.Parameters.AddWithValue("@PosY", y);
                areaCommand.Parameters.AddWithValue("@PosZ", z);
                areaCommand.Parameters.AddWithValue("@Radius", radius);
                await areaCommand.ExecuteNonQueryAsync();
            }

            await using var policyCommand = new SqlCommand(@"
UPDATE dbo.Clientless_HuntPolicy
SET Enabled = @Enabled, AttackNormal = @AttackNormal, AttackUnique = @AttackUnique,
    UniquePriority = @UniquePriority, UseSkills = @UseSkills, UseBasicAttack = @UseBasicAttack,
    HpPotionPercent = @HpPotionPercent, MpPotionPercent = @MpPotionPercent,
    StuckSeconds = @StuckSeconds, TargetTimeoutSeconds = @TargetTimeoutSeconds,
    UpdatedAt = SYSDATETIME()
WHERE SettingID = 1;", connection, transaction);
            policyCommand.Parameters.AddWithValue("@Enabled", policy.Enabled);
            policyCommand.Parameters.AddWithValue("@AttackNormal", policy.AttackNormal);
            policyCommand.Parameters.AddWithValue("@AttackUnique", policy.AttackUnique);
            policyCommand.Parameters.AddWithValue("@UniquePriority", policy.UniquePriority);
            policyCommand.Parameters.AddWithValue("@UseSkills", policy.UseSkills);
            policyCommand.Parameters.AddWithValue("@UseBasicAttack", policy.UseBasicAttack);
            policyCommand.Parameters.AddWithValue("@HpPotionPercent", policy.HpPotionPercent);
            policyCommand.Parameters.AddWithValue("@MpPotionPercent", policy.MpPotionPercent);
            policyCommand.Parameters.AddWithValue("@StuckSeconds", policy.StuckSeconds);
            policyCommand.Parameters.AddWithValue("@TargetTimeoutSeconds", policy.TargetTimeoutSeconds);
            await policyCommand.ExecuteNonQueryAsync();

            await transaction.CommitAsync();
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }

        await AuditAsync(connection, "SaveClientlessHunting", city,
            $"Enabled={policy.Enabled};Areas={areas.Rows.Cast<DataRow>().Count(row => Convert.ToBoolean(row["Enabled"]))}");
        return $"Saved five hunting areas for {city}. Hunting is {(policy.Enabled ? "enabled" : "paused")} globally.";
    }

    public async Task<string> SetClientlessAccountHuntingAsync(int id, bool enabled)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureClientlessSchemaAsync(connection);
        await using var command = new SqlCommand(@"
UPDATE dbo.Clientless_Accounts
SET HuntEnabled = @Enabled,
    HuntStatus = CASE WHEN @Enabled = 1 THEN 'Waiting' ELSE 'Paused' END,
    HuntMessage = CASE WHEN @Enabled = 1 THEN 'Waiting for a hunting area' ELSE 'Paused from Dashboard' END,
    CurrentTargetName = NULL, UpdatedAt = SYSDATETIME()
WHERE ID = @ID;", connection);
        command.Parameters.AddWithValue("@ID", id);
        command.Parameters.AddWithValue("@Enabled", enabled);
        if (await command.ExecuteNonQueryAsync() == 0)
            throw new InvalidOperationException($"Clientless account #{id} was not found.");
        await AuditAsync(connection, "SetClientlessAccountHunting", id.ToString(), enabled ? "Enabled" : "Paused");
        return $"Hunting {(enabled ? "enabled" : "paused")} for clientless account #{id}.";
    }

    public async Task<ClientlessHunterAllocationResult> SetClientlessCityRuntimePlanAsync(
        string? city,
        int onlinePerCity,
        int huntersPerCity)
    {
        if (onlinePerCity is < 0 or > 5000)
            throw new InvalidOperationException("Online accounts per city must be between 0 and 5,000.");
        if (huntersPerCity is < 0 or > 5000)
            throw new InvalidOperationException("Hunters per city must be between 0 and 5,000.");
        if (huntersPerCity > onlinePerCity)
            throw new InvalidOperationException("Hunters cannot exceed the requested online accounts for a city.");

        var scopedCity = string.IsNullOrWhiteSpace(city) ||
                         city.Equals("All", StringComparison.OrdinalIgnoreCase)
            ? null
            : NormalizeClientlessTownOrUnassigned(city);

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureClientlessSchemaAsync(connection);

        var accountDb = QuoteDb(await ReadSettingAsync(connection, "AccountDB", "SRO_VT_ACCOUNT"));
        var shardDb = QuoteDb(await ReadSettingAsync(connection, "ShardDB", "SRO_VT_SHARD"));
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync();

        IReadOnlyList<ClientlessRuntimePlanSelection> selections;
        try
        {
            var candidates = new List<ClientlessHunterCandidate>();
            await using (var candidateCommand = new SqlCommand($@"
SELECT Accounts.ID,
       ISNULL(NULLIF(LTRIM(RTRIM(Accounts.City)), ''), 'Unassigned') AS City,
       Accounts.Enabled,
       CASE WHEN GameCharacter.CharID IS NULL THEN CONVERT(bit, 0) ELSE CONVERT(bit, 1) END AS GameReady,
       Accounts.SystemRole
FROM dbo.Clientless_Accounts AS Accounts
OUTER APPLY
(
    SELECT TOP (1) Characters.CharID
    FROM {accountDb}.dbo.TB_User AS Users WITH (NOLOCK)
    INNER JOIN {shardDb}.dbo._User AS UserCharacters WITH (NOLOCK)
        ON UserCharacters.UserJID = Users.JID
    INNER JOIN {shardDb}.dbo._Char AS Characters WITH (NOLOCK)
        ON Characters.CharID = UserCharacters.CharID
    WHERE Users.StrUserID COLLATE DATABASE_DEFAULT = Accounts.AccountName COLLATE DATABASE_DEFAULT
      AND Characters.CharName16 COLLATE DATABASE_DEFAULT = Accounts.CharacterName COLLATE DATABASE_DEFAULT
) AS GameCharacter
WHERE @City IS NULL
   OR ISNULL(NULLIF(LTRIM(RTRIM(Accounts.City)), ''), 'Unassigned') = @City
ORDER BY City, Accounts.ID;", connection, transaction))
            {
                candidateCommand.Parameters.Add("@City", SqlDbType.VarChar, 32).Value =
                    (object?)scopedCity ?? DBNull.Value;
                await using var reader = await candidateCommand.ExecuteReaderAsync();
                while (await reader.ReadAsync())
                {
                    candidates.Add(new ClientlessHunterCandidate(
                        reader.GetInt32(0),
                        reader.GetString(1),
                        reader.GetBoolean(2),
                        reader.GetBoolean(3),
                        reader.IsDBNull(4) ? null : reader.GetString(4)));
                }
            }

            selections = BuildClientlessRuntimePlanSelections(candidates, onlinePerCity, huntersPerCity);
            if (selections.Count == 0 && scopedCity is not null)
            {
                selections =
                [
                    new ClientlessRuntimePlanSelection(
                        scopedCity,
                        Array.Empty<int>(),
                        Array.Empty<int>(),
                        0,
                        0)
                ];
            }

            await using (var pauseCommand = new SqlCommand(@"
UPDATE Accounts
SET Enabled = 0,
    HuntEnabled = 0,
    HuntStatus = 'Stopped',
    HuntMessage = N'Offline by city runtime plan',
    HuntAreaID = NULL,
    CurrentTargetName = NULL,
    UpdatedAt = SYSDATETIME()
FROM dbo.Clientless_Accounts AS Accounts
WHERE NULLIF(LTRIM(RTRIM(ISNULL(Accounts.SystemRole, ''))), '') IS NULL
  AND (@City IS NULL
       OR ISNULL(NULLIF(LTRIM(RTRIM(Accounts.City)), ''), 'Unassigned') = @City);", connection, transaction))
            {
                pauseCommand.Parameters.Add("@City", SqlDbType.VarChar, 32).Value =
                    (object?)scopedCity ?? DBNull.Value;
                await pauseCommand.ExecuteNonQueryAsync();
            }

            var onlineIds = selections
                .SelectMany(selection => selection.OnlineAccountIds)
                .Distinct()
                .OrderBy(id => id)
                .ToArray();
            foreach (var batch in onlineIds.Chunk(1000))
            {
                var parameterNames = batch
                    .Select((_, index) => $"@OnlineID{index}")
                    .ToArray();
                await using var enableCommand = new SqlCommand($@"
UPDATE dbo.Clientless_Accounts
SET Enabled = 1,
    HuntEnabled = 0,
    HuntStatus = 'Paused',
    HuntMessage = N'Online and staying in city by runtime plan',
    HuntAreaID = NULL,
    CurrentTargetName = NULL,
    UpdatedAt = SYSDATETIME()
WHERE NULLIF(LTRIM(RTRIM(ISNULL(SystemRole, ''))), '') IS NULL
  AND ID IN ({string.Join(",", parameterNames)});", connection, transaction);
                for (var index = 0; index < batch.Length; index++)
                    enableCommand.Parameters.Add(parameterNames[index], SqlDbType.Int).Value = batch[index];
                await enableCommand.ExecuteNonQueryAsync();
            }

            var hunterIds = selections
                .SelectMany(selection => selection.HunterAccountIds)
                .Distinct()
                .OrderBy(id => id)
                .ToArray();
            foreach (var batch in hunterIds.Chunk(1000))
            {
                var parameterNames = batch
                    .Select((_, index) => $"@HunterID{index}")
                    .ToArray();
                await using var huntCommand = new SqlCommand($@"
UPDATE dbo.Clientless_Accounts
SET HuntEnabled = 1,
    HuntStatus = 'Waiting',
    HuntMessage = N'Online and hunting by runtime plan',
    HuntAreaID = NULL,
    CurrentTargetName = NULL,
    UpdatedAt = SYSDATETIME()
WHERE Enabled = 1
  AND NULLIF(LTRIM(RTRIM(ISNULL(SystemRole, ''))), '') IS NULL
  AND ID IN ({string.Join(",", parameterNames)});", connection, transaction);
                for (var index = 0; index < batch.Length; index++)
                    huntCommand.Parameters.Add(parameterNames[index], SqlDbType.Int).Value = batch[index];
                await huntCommand.ExecuteNonQueryAsync();
            }

            await transaction.CommitAsync();
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }

        var result = new ClientlessHunterAllocationResult
        {
            RequestedOnlinePerCity = onlinePerCity,
            RequestedHuntersPerCity = huntersPerCity,
            Cities = selections.Select(selection => new ClientlessHunterCityAllocation
            {
                City = selection.City,
                TotalAccounts = selection.TotalAccounts,
                ReadyAvailableAccounts = selection.ReadyAccounts,
                ReadyEnabledAccounts = selection.OnlineAccountIds.Count,
                ActiveHunters = selection.HunterAccountIds.Count,
                ParkedReadyAccounts = selection.OnlineAccountIds.Count - selection.HunterAccountIds.Count,
                OfflineReadyAccounts = selection.ReadyAccounts - selection.OnlineAccountIds.Count,
                UnavailableAccounts = selection.TotalAccounts - selection.ReadyAccounts
            }).ToArray()
        };

        await AuditAsync(
            connection,
            "SetClientlessCityRuntimePlan",
            scopedCity ?? "All cities",
            $"RequestedOnlinePerCity={onlinePerCity};RequestedHuntersPerCity={huntersPerCity};Cities={result.Cities.Count};" +
            $"Online={result.Cities.Sum(item => item.ReadyEnabledAccounts)};" +
            $"Hunters={result.Cities.Sum(item => item.ActiveHunters)};" +
            $"Ready={result.Cities.Sum(item => item.ReadyAvailableAccounts)};" +
            $"ParkedReady={result.Cities.Sum(item => item.ParkedReadyAccounts)}");
        return result;
    }

    internal static IReadOnlyList<ClientlessRuntimePlanSelection> BuildClientlessRuntimePlanSelections(
        IEnumerable<ClientlessHunterCandidate> candidates,
        int onlinePerCity,
        int huntersPerCity)
    {
        ArgumentNullException.ThrowIfNull(candidates);
        if (onlinePerCity is < 0 or > 5000 || huntersPerCity < 0 || huntersPerCity > onlinePerCity)
            throw new InvalidOperationException("The Clientless city runtime plan is invalid.");

        return candidates
            .Where(candidate => string.IsNullOrWhiteSpace(candidate.SystemRole))
            .GroupBy(
                candidate => NormalizeClientlessTownOrUnassigned(candidate.City),
                StringComparer.OrdinalIgnoreCase)
            .Select(group =>
            {
                var accounts = group
                    .GroupBy(candidate => candidate.Id)
                    .Select(duplicates => duplicates.First())
                    .OrderBy(candidate => candidate.Id)
                    .ToArray();
                var ready = accounts.Where(candidate => candidate.GameReady).ToArray();
                var online = ready.Take(onlinePerCity).Select(candidate => candidate.Id).ToArray();
                return new ClientlessRuntimePlanSelection(
                    group.Key,
                    online,
                    online.Take(huntersPerCity).ToArray(),
                    accounts.Length,
                    ready.Length);
            })
            .OrderBy(selection => GetClientlessTownSortOrder(selection.City))
            .ThenBy(selection => selection.City, StringComparer.OrdinalIgnoreCase)
            .ToArray();
    }

    internal static IReadOnlyList<ClientlessHunterSelection> BuildClientlessHunterSelections(
        IEnumerable<ClientlessHunterCandidate> candidates,
        int huntersPerCity)
    {
        ArgumentNullException.ThrowIfNull(candidates);
        if (huntersPerCity is < 0 or > 5000)
            throw new InvalidOperationException("Hunters per city must be between 0 and 5,000.");

        return candidates
            .Where(candidate => string.IsNullOrWhiteSpace(candidate.SystemRole))
            .GroupBy(
                candidate => NormalizeClientlessTownOrUnassigned(candidate.City),
                StringComparer.OrdinalIgnoreCase)
            .Select(group =>
            {
                var accounts = group
                    .GroupBy(candidate => candidate.Id)
                    .Select(duplicates => duplicates.First())
                    .OrderBy(candidate => candidate.Id)
                    .ToArray();
                var ready = accounts
                    .Where(candidate => candidate.Enabled && candidate.GameReady)
                    .ToArray();
                return new ClientlessHunterSelection(
                    group.Key,
                    ready.Take(huntersPerCity).Select(candidate => candidate.Id).ToArray(),
                    accounts.Length,
                    ready.Length);
            })
            .OrderBy(selection => GetClientlessTownSortOrder(selection.City))
            .ThenBy(selection => selection.City, StringComparer.OrdinalIgnoreCase)
            .ToArray();
    }

    private static int GetClientlessTownSortOrder(string city) => city switch
    {
        "Jangan" => 1,
        "Donwhang" => 2,
        "Hotan" => 3,
        "SamarKand" => 4,
        "Constantinople" => 5,
        "Alexandria North (SD)" => 6,
        "Unassigned" => 7,
        _ => 8
    };

    internal static DataTable CreateClientlessHuntAreaEditorTable(DataTable source)
    {
        ArgumentNullException.ThrowIfNull(source);
        var editor = new DataTable("ClientlessHuntAreaEditor")
        {
            Locale = CultureInfo.InvariantCulture
        };
        editor.Columns.Add("ID", typeof(int));
        editor.Columns.Add("Area", typeof(int));
        editor.Columns.Add("Name", typeof(string));
        editor.Columns.Add("Enabled", typeof(bool));
        // Numeric inputs intentionally stay strings while the operator moves between
        // cells. WPF must not lock focus while attempting culture-sensitive conversion.
        editor.Columns.Add("RegionID", typeof(string));
        editor.Columns.Add("X", typeof(string));
        editor.Columns.Add("Y", typeof(string));
        editor.Columns.Add("Z", typeof(string));
        editor.Columns.Add("Radius", typeof(string));

        foreach (DataRow sourceRow in source.Rows)
        {
            var row = editor.NewRow();
            row["ID"] = Convert.ToInt32(sourceRow["ID"], CultureInfo.InvariantCulture);
            row["Area"] = Convert.ToInt32(sourceRow["Area"], CultureInfo.InvariantCulture);
            row["Name"] = Convert.ToString(sourceRow["Name"], CultureInfo.InvariantCulture) ?? string.Empty;
            row["Enabled"] = Convert.ToBoolean(sourceRow["Enabled"], CultureInfo.InvariantCulture);
            foreach (var columnName in new[] { "RegionID", "X", "Y", "Z", "Radius" })
                row[columnName] = FormatClientlessHuntEditorNumber(sourceRow[columnName]);
            editor.Rows.Add(row);
        }

        return editor;
    }

    internal static int ParseClientlessHuntInteger(object? value, string fieldName)
    {
        var normalized = NormalizeClientlessHuntNumericText(value);
        if (int.TryParse(normalized, NumberStyles.AllowLeadingSign, CultureInfo.InvariantCulture, out var result))
            return result;
        throw new InvalidOperationException($"{fieldName} must be a whole number.");
    }

    internal static float ParseClientlessHuntNumber(object? value, string fieldName)
    {
        var normalized = NormalizeClientlessHuntNumericText(value);
        if (float.TryParse(normalized, NumberStyles.Float, CultureInfo.InvariantCulture, out var result) &&
            float.IsFinite(result))
        {
            return result;
        }
        throw new InvalidOperationException($"{fieldName} must be a valid number.");
    }

    private static string FormatClientlessHuntEditorNumber(object value) => value switch
    {
        IFormattable formattable => formattable.ToString(null, CultureInfo.InvariantCulture),
        _ => Convert.ToString(value, CultureInfo.InvariantCulture) ?? string.Empty
    };

    private static string NormalizeClientlessHuntNumericText(object? value)
    {
        var text = Convert.ToString(value, CultureInfo.CurrentCulture)?.Trim() ?? string.Empty;
        var normalized = new StringBuilder(text.Length);
        foreach (var character in text)
        {
            if (char.GetUnicodeCategory(character) == UnicodeCategory.DecimalDigitNumber)
            {
                var digit = (int)char.GetNumericValue(character);
                normalized.Append((char)('0' + digit));
                continue;
            }

            normalized.Append(character switch
            {
                '\u066B' or ',' => '.', // Arabic or regional decimal separator.
                '\u2212' => '-',
                _ => character
            });
        }
        return normalized.ToString();
    }

    private static async Task EnsureClientlessHuntCityRowsAsync(SqlConnection connection, string city)
    {
        await using var command = new SqlCommand(@"
;WITH Slots AS
(
    SELECT CONVERT(TINYINT, 1) AS SlotNumber UNION ALL SELECT 2 UNION ALL SELECT 3 UNION ALL SELECT 4 UNION ALL SELECT 5
)
INSERT dbo.Clientless_HuntAreas (City, SlotNumber, DisplayName, Enabled, RegionID, PosX, PosY, PosZ, Radius)
SELECT @City, SlotNumber, CONCAT(N'Area ', SlotNumber), 0, 0, 0, 0, 0, 50
FROM Slots
WHERE NOT EXISTS
(
    SELECT 1 FROM dbo.Clientless_HuntAreas A
    WHERE A.City = @City AND A.SlotNumber = Slots.SlotNumber
);", connection);
        command.Parameters.AddWithValue("@City", city);
        await command.ExecuteNonQueryAsync();
    }

    public async Task<string> BulkCreateEquippedClientlessAccountsAsync(
        IReadOnlyDictionary<string, int> cityCounts,
        string password,
        int minLevel,
        int maxLevel,
        int shardId,
        byte locale,
        int launchDelayMs,
        int reconnectDelaySeconds,
        string raceSelection,
        string weaponSelection,
        string genderSelection,
        string equipmentSelection,
        string skillSelection,
        string buildSelection,
        bool includeAvatars,
        bool includeAttackPet,
        bool includeGrabPet)
    {
        ValidateClientlessRuntimeFields(shardId, launchDelayMs, reconnectDelaySeconds);
        raceSelection = NormalizeClientlessRaceSelection(raceSelection);
        weaponSelection = NormalizeClientlessWeaponSelection(weaponSelection);
        genderSelection = NormalizeClientlessGenderSelection(genderSelection);
        equipmentSelection = NormalizeClientlessEquipmentSelection(equipmentSelection);
        skillSelection = NormalizeClientlessSkillSelection(skillSelection);
        buildSelection = NormalizeClientlessBuildSelection(buildSelection);
        if (minLevel is < 1 or > byte.MaxValue || maxLevel is < 1 or > byte.MaxValue)
            throw new InvalidOperationException($"Clientless levels must be between 1 and {byte.MaxValue}.");
        if (minLevel > maxLevel)
            throw new InvalidOperationException("Minimum level cannot exceed maximum level.");

        var allocations = cityCounts
            .Select(item => new ClientlessCityAllocation(
                NormalizeClientlessTown(item.Key),
                Math.Clamp(item.Value, 0, 5000)))
            .GroupBy(item => item.Town, StringComparer.OrdinalIgnoreCase)
            .Select(group => new ClientlessCityAllocation(group.Key, group.Sum(item => item.Count)))
            .Where(item => item.Count > 0)
            .ToArray();
        var count = allocations.Sum(item => item.Count);
        if (count is < 1 or > 5000)
            throw new InvalidOperationException("The combined city account count must be between 1 and 5,000.");

        var accountPassword = string.IsNullOrWhiteSpace(password) ? "bot123" : password.Trim();
        if (accountPassword.Length is < 1 or > 50)
            throw new InvalidOperationException("Clientless password must contain 1 to 50 characters.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureClientlessSchemaAsync(connection);

        var accountDb = await ReadSettingAsync(connection, "AccountDB", "SRO_VT_ACCOUNT");
        var shardDb = await ReadSettingAsync(connection, "ShardDB", "SRO_VT_SHARD");
        var serverMaxLevel = await ReadGameServerSettingIntAsync(connection, "SERVER_MAX_LEVEL", 125, 1, byte.MaxValue);
        var chineseMasteryCap = await ReadGameServerSettingIntAsync(connection, "CH_MAX_MASTERY_LEVEL", 300, 1, 10000);
        var europeanMasteryCap = await ReadGameServerSettingIntAsync(connection, "EU_MAX_MASTERY_LEVEL", 240, 1, 10000);
        if (maxLevel > serverMaxLevel)
            throw new InvalidOperationException(
                $"Maximum level cannot exceed the configured GameServer level cap ({serverMaxLevel}).");
        var repairedOwners = new List<string>();
        if (await EnsureDatabaseOwnerCanExecuteAsDboAsync(connection, accountDb))
            repairedOwners.Add(accountDb);
        if (await EnsureDatabaseOwnerCanExecuteAsDboAsync(connection, shardDb))
            repairedOwners.Add(shardDb);
        if (repairedOwners.Count > 0)
            await AuditAsync(connection, "RepairDatabaseOwner", string.Join(",", repairedOwners), "Restored dbo execution for Clientless provisioning");

        var spawnPlan = allocations
            .SelectMany(allocation => CreateClientlessSpawnPositions(allocation.Town, allocation.Count)
                .Select(position => new ClientlessSpawnAssignment(allocation.Town, position)))
            .OrderBy(_ => Random.Shared.Next())
            .ToArray();
        var created = 0;
        var failed = 0;
        var lastError = string.Empty;

        foreach (var spawn in spawnPlan)
        {
            try
            {
                var characterName = await GenerateUniqueClientlessCharacterNameAsync(connection, shardDb);
                var accountName = await GenerateUniqueClientlessAccountNameAsync(connection, accountDb, characterName);
                var level = Random.Shared.Next(minLevel, maxLevel + 1);
                var equipmentProfile = ResolveClientlessWeaponProfile(raceSelection, weaponSelection);
                var race = equipmentProfile.Race;
                var gender = genderSelection == "Random"
                    ? (Random.Shared.Next(0, 2) == 0 ? "Male" : "Female")
                    : genderSelection;
                var masteryCap = race == "Chinese" ? chineseMasteryCap : europeanMasteryCap;
                var characterBuild = buildSelection == "Random"
                    ? (Random.Shared.Next(0, 2) == 0 ? "Strength" : "Intelligence")
                    : buildSelection;

                await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync();
                try
                {
                    await CreateEquippedClientlessCharacterAsync(
                        connection,
                        transaction,
                        accountDb,
                        shardDb,
                        accountName,
                        accountPassword,
                        characterName,
                        race,
                        gender,
                        level,
                        spawn.Position,
                        equipmentProfile,
                        equipmentSelection,
                        skillSelection,
                        masteryCap,
                        characterBuild,
                        includeAvatars,
                        includeAttackPet,
                        includeGrabPet);

                    await UpsertClientlessAccountAsync(
                        connection,
                        transaction,
                        accountName,
                        accountPassword,
                        characterName,
                        spawn.Town,
                        shardId,
                        locale,
                        launchDelayMs,
                        reconnectDelaySeconds,
                        equipmentProfile,
                        equipmentSelection,
                        skillSelection,
                        characterBuild,
                        spawn.Position,
                        includeAvatars,
                        includeAttackPet,
                        includeGrabPet);
                    await transaction.CommitAsync();
                }
                catch
                {
                    if (transaction.Connection != null)
                        await transaction.RollbackAsync();
                    throw;
                }

                created++;
            }
            catch (Exception ex)
            {
                failed++;
                lastError = ex.Message;
                break;
            }
        }

        var allocationSummary = string.Join(",", allocations.Select(item => $"{item.Town}:{item.Count}"));
        await AuditAsync(
            connection,
            "BulkCreateEquippedClientlessAccounts",
            "GamingNames",
            $"Requested={count};Created={created};Failed={failed};Level={minLevel}-{maxLevel};Cities={allocationSummary};" +
            $"Race={raceSelection};Weapon={weaponSelection};Gender={genderSelection};Equipment={equipmentSelection};" +
            $"Skills={skillSelection};Build={buildSelection};Avatars={includeAvatars};" +
            $"AttackPet={includeAttackPet};GrabPet={includeGrabPet};Spawn=Distributed");

        if (failed > 0)
            throw new InvalidOperationException(
                $"Created {created:N0}/{count:N0} equipped clientless account(s). Creation stopped safely after: {lastError}");

        return failed == 0
            ? $"Created {created:N0} clientless account(s). Race={raceSelection}, weapon={weaponSelection}, " +
              $"equipment={equipmentSelection}, skills={skillSelection}, build={buildSelection}, " +
              $"avatars={(includeAvatars ? "enabled" : "disabled")}, " +
              $"attack pet={(includeAttackPet ? "enabled" : "disabled")}, " +
              $"grab pet={(includeGrabPet ? "enabled" : "disabled")}. " +
              $"Cities: {allocationSummary}."
            : $"Created {created:N0}/{count:N0} equipped clientless account(s). Failed {failed:N0}. Last error: {lastError}";
    }

    public async Task<string> RepairClientlessCharacterEssentialsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureClientlessSchemaAsync(connection);

        var accountDb = await ReadSettingAsync(connection, "AccountDB", "SRO_VT_ACCOUNT");
        var shardDb = await ReadSettingAsync(connection, "ShardDB", "SRO_VT_SHARD");
        var rows = await QueryTableAsync(connection,
            @"SELECT AccountName, CharacterName
              FROM [dbo].[Clientless_Accounts] WITH (NOLOCK)
              WHERE Enabled = 1
              ORDER BY ID;");

        var repaired = 0;
        var skipped = 0;
        var lastError = string.Empty;
        foreach (DataRow row in rows.Rows)
        {
            try
            {
                await EnsureClientlessCharacterEssentialsAsync(
                    connection,
                    accountDb,
                    shardDb,
                    Convert.ToString(row["AccountName"]) ?? string.Empty,
                    Convert.ToString(row["CharacterName"]) ?? string.Empty);
                repaired++;
            }
            catch (Exception ex)
            {
                skipped++;
                lastError = ex.Message;
            }
        }

        await AuditAsync(connection, "RepairClientlessCharacterEssentials", "[dbo].[Clientless_Accounts]", $"Repaired={repaired};Skipped={skipped}");
        return skipped == 0
            ? $"Repaired {repaired:N0} clientless character(s)."
            : $"Repaired {repaired:N0} clientless character(s), skipped {skipped:N0}. Last error: {lastError}";
    }

    internal static string BuildClientlessOwnedCharacterJoinSql(string accountDbName, string shardDbName) => $@"
INNER JOIN {accountDbName}.dbo.TB_User AS AccountOwner
    ON AccountOwner.StrUserID COLLATE Latin1_General_CI_AS =
       AccountRow.AccountName COLLATE Latin1_General_CI_AS
INNER JOIN {shardDbName}.dbo._User AS CharacterOwner
    ON CharacterOwner.UserJID = AccountOwner.JID
INNER JOIN {shardDbName}.dbo._Char AS C
    ON C.CharID = CharacterOwner.CharID
   AND C.CharName16 COLLATE Latin1_General_CI_AS =
       AccountRow.CharacterName COLLATE Latin1_General_CI_AS";

    internal static string BuildClientlessUnusedMasteryResetSql(string shardDbName) => $@"
UPDATE ExistingMastery
SET Level = 0
FROM {shardDbName}.dbo._CharSkillMastery AS ExistingMastery
INNER JOIN #ClientlessCharacters AS ManagedCharacter
    ON ManagedCharacter.CharID = ExistingMastery.CharID
WHERE ExistingMastery.Level <> 0
  AND NOT EXISTS
  (
      SELECT 1
      FROM #ClientlessSkillTargets AS RequiredMastery
      WHERE RequiredMastery.CharID = ExistingMastery.CharID
        AND RequiredMastery.MasteryID = ExistingMastery.MasteryID
  );";

    internal static string BuildClientlessMasteryCapGuardSql() => @"
IF EXISTS
(
    SELECT 1
    FROM #ClientlessSkillTargets AS Target
    INNER JOIN #ClientlessCharacters AS ManagedCharacter
        ON ManagedCharacter.CharID = Target.CharID
    GROUP BY Target.CharID, ManagedCharacter.TotalMasteryCap
    HAVING SUM(CONVERT(BIGINT, Target.MasteryLevel)) > ManagedCharacter.TotalMasteryCap
)
    THROW 51035, 'Prepared mastery allocation exceeds the configured mastery cap.', 1;";

    internal static string BuildClientlessObsoleteSkillDeleteSql(string shardDbName) => $@"
DELETE Learned
FROM {shardDbName}.dbo._CharSkill AS Learned
INNER JOIN #ClientlessCharacters AS ManagedCharacter
    ON ManagedCharacter.CharID = Learned.CharID
INNER JOIN {shardDbName}.dbo._RefSkill AS CurrentSkill WITH (NOLOCK)
    ON CurrentSkill.ID = Learned.SkillID
WHERE ISNULL(CurrentSkill.ReqCommon_Mastery1, 0) > 0
  AND NOT EXISTS
  (
      SELECT 1
      FROM #ClientlessDesiredSkills AS Desired
      WHERE Desired.CharID = Learned.CharID
        AND Desired.SkillID = Learned.SkillID
  );";

    internal static string BuildClientlessAvatarEquipmentSql(
        string shardDbName,
        string characterIdExpression,
        string genderExpression,
        string variationSeedExpression) => $@"
IF OBJECT_ID(N'{shardDbName}.dbo._InventoryForAvatar', N'U') IS NULL
   OR OBJECT_ID(N'{shardDbName}.dbo._Items', N'U') IS NULL
   OR OBJECT_ID(N'{shardDbName}.dbo._ItemPool', N'U') IS NULL
   OR OBJECT_ID(N'{shardDbName}.dbo._LatestItemSerial', N'U') IS NULL
   OR OBJECT_ID(N'{shardDbName}.dbo._STRG_ALLOC_ITEM_NoTX', N'P') IS NULL
    THROW 51045, 'The vSRO avatar inventory or item allocator is unavailable.', 1;

DECLARE @AvatarSetEquipped BIT = 0;
DECLARE @AvatarTheme VARCHAR(16);
DECLARE @AvatarGenderCode CHAR(1);
DECLARE @AvatarCodePrefix VARCHAR(128);
DECLARE @AvatarSlot TINYINT;
DECLARE @AvatarRefItemID INT;
DECLARE @AvatarNewItemID INT;
DECLARE @AvatarSerial BIGINT;
DECLARE @AvatarExpected TABLE
(
    Slot TINYINT NOT NULL PRIMARY KEY,
    RefItemID INT NOT NULL
);

SET @AvatarSetEquipped = 0;
DELETE FROM @AvatarExpected;
SET @AvatarGenderCode = CASE WHEN {genderExpression} = 1 THEN 'M' ELSE 'W' END;
SET @AvatarTheme = CASE (CONVERT(BIGINT, {variationSeedExpression}) & 2147483647) % 6
    WHEN 0 THEN 'HALLOWEEN'
    WHEN 1 THEN 'PIRATE'
    WHEN 2 THEN 'ARABIA'
    WHEN 3 THEN 'CLOWN'
    WHEN 4 THEN 'CARNIVAL'
    ELSE 'SPARTA'
END;
SET @AvatarCodePrefix = 'ITEM_MALL_AVATAR_' + @AvatarGenderCode + '_' + @AvatarTheme;

INSERT INTO @AvatarExpected (Slot, RefItemID)
SELECT AvatarSlots.Slot, Common.ID
FROM
(
    VALUES
        (CONVERT(TINYINT, 0), CONVERT(TINYINT, 2), CONVERT(VARCHAR(16), '')),
        (CONVERT(TINYINT, 1), CONVERT(TINYINT, 1), CONVERT(VARCHAR(16),
            CASE WHEN @AvatarTheme = 'CLOWN' THEN '_HAT_2' ELSE '_HAT' END)),
        (CONVERT(TINYINT, 2), CONVERT(TINYINT, 3), CONVERT(VARCHAR(16), '_ATTACH'))
) AS AvatarSlots(Slot, ItemTypeID4, CodeSuffix)
INNER JOIN {shardDbName}.dbo._RefObjCommon AS Common WITH (NOLOCK)
    ON Common.CodeName128 = @AvatarCodePrefix + AvatarSlots.CodeSuffix
   AND Common.Service = 1
   AND Common.Country = 3
   AND Common.TypeID1 = 3
   AND Common.TypeID2 = 1
   AND Common.TypeID3 = 13
   AND Common.TypeID4 = AvatarSlots.ItemTypeID4
INNER JOIN {shardDbName}.dbo._RefObjItem AS AvatarItem WITH (NOLOCK)
    ON AvatarItem.ID = Common.Link
   AND AvatarItem.ReqGender = {genderExpression};

IF (SELECT COUNT(1) FROM @AvatarExpected) <> 3
    THROW 51046, 'A complete gender-compatible Clientless avatar set could not be resolved.', 1;

INSERT INTO {shardDbName}.dbo._InventoryForAvatar (CharID, Slot, ItemID)
SELECT {characterIdExpression}, Dummy.cnt, 0
FROM {shardDbName}.dbo._RefDummySlot AS Dummy WITH (NOLOCK)
WHERE Dummy.cnt < 5
  AND NOT EXISTS
  (
      SELECT 1
      FROM {shardDbName}.dbo._InventoryForAvatar AS Existing WITH (UPDLOCK, HOLDLOCK)
      WHERE Existing.CharID = {characterIdExpression} AND Existing.Slot = Dummy.cnt
  );

-- Preserve a deliberately assigned avatar. Empty managed characters receive
-- one complete, coherent set instead of mixing new pieces with an old set.
IF NOT EXISTS
(
    SELECT 1
    FROM {shardDbName}.dbo._InventoryForAvatar WITH (UPDLOCK, HOLDLOCK)
    WHERE CharID = {characterIdExpression} AND Slot BETWEEN 0 AND 2 AND ItemID > 0
)
BEGIN
    SET @AvatarSlot = 0;
    WHILE @AvatarSlot <= 2
    BEGIN
        SET @AvatarRefItemID = NULL;
        SELECT @AvatarRefItemID = RefItemID
        FROM @AvatarExpected
        WHERE Slot = @AvatarSlot;

        SET @AvatarSerial = 0;
        SET @AvatarNewItemID = 0;
        EXEC @AvatarNewItemID = {shardDbName}.dbo._STRG_ALLOC_ITEM_NoTX
            @LatestItemSerial = @AvatarSerial OUTPUT;
        IF ISNULL(@AvatarNewItemID, 0) <= 0
            THROW 51047, 'vSRO could not allocate a Clientless avatar item.', 1;

        UPDATE {shardDbName}.dbo._Items
        SET RefItemID = @AvatarRefItemID,
            OptLevel = 0,
            Variance = 0,
            Data = 0,
            MagParamNum = 0,
            CreaterName = NULL
        WHERE ID64 = @AvatarNewItemID;
        IF @@ROWCOUNT <> 1
            THROW 51048, 'vSRO could not initialize a Clientless avatar item.', 1;

        UPDATE {shardDbName}.dbo._InventoryForAvatar
        SET ItemID = @AvatarNewItemID
        WHERE CharID = {characterIdExpression} AND Slot = @AvatarSlot AND ItemID = 0;
        IF @@ROWCOUNT <> 1
            THROW 51049, 'vSRO could not equip a Clientless avatar item.', 1;

        SET @AvatarSlot += 1;
    END

    IF (SELECT COUNT(1)
        FROM {shardDbName}.dbo._InventoryForAvatar AS AvatarInventory
        INNER JOIN {shardDbName}.dbo._Items AS ItemRow
            ON ItemRow.ID64 = AvatarInventory.ItemID
        INNER JOIN @AvatarExpected AS Expected
            ON Expected.Slot = AvatarInventory.Slot
           AND Expected.RefItemID = ItemRow.RefItemID
        WHERE AvatarInventory.CharID = {characterIdExpression}) <> 3
        THROW 51050, 'The Clientless avatar set failed final inventory validation.', 1;

    SET @AvatarSetEquipped = 1;
END;";

    internal static string BuildClientlessPetEquipmentSql(
        string shardDbName,
        string characterIdExpression,
        string characterLevelExpression,
        string variationSeedExpression,
        string attackPetEnabledExpression = "1",
        string grabPetEnabledExpression = "1") => $@"
IF OBJECT_ID(N'{shardDbName}.dbo._Inventory', N'U') IS NULL
   OR OBJECT_ID(N'{shardDbName}.dbo._Items', N'U') IS NULL
   OR OBJECT_ID(N'{shardDbName}.dbo._ItemPool', N'U') IS NULL
   OR OBJECT_ID(N'{shardDbName}.dbo._LatestItemSerial', N'U') IS NULL
   OR OBJECT_ID(N'{shardDbName}.dbo._BindingOptionWithItem', N'U') IS NULL
   OR OBJECT_ID(N'{shardDbName}.dbo._CharCOS', N'U') IS NULL
   OR OBJECT_ID(N'{shardDbName}.dbo._InvCOS', N'U') IS NULL
    THROW 51051, 'The vSRO inventory or item allocator required for Clientless pets is unavailable.', 1;

DECLARE @ClientlessPetsAdded INT = 0;
DECLARE @ClientlessEnableAttackPet BIT = CASE WHEN ISNULL(CONVERT(INT, {attackPetEnabledExpression}), 0) = 1 THEN 1 ELSE 0 END;
DECLARE @ClientlessEnableGrabPet BIT = CASE WHEN ISNULL(CONVERT(INT, {grabPetEnabledExpression}), 0) = 1 THEN 1 ELSE 0 END;
DECLARE @ClientlessExpectedPetCount INT = CONVERT(INT, @ClientlessEnableAttackPet) + CONVERT(INT, @ClientlessEnableGrabPet);
DECLARE @ClientlessPetKind TINYINT;
DECLARE @ClientlessPetRefItemID INT;
DECLARE @ClientlessPetInventorySlot TINYINT;
DECLARE @ClientlessPetExpected TABLE
(
    PetKind TINYINT NOT NULL PRIMARY KEY,
    RefItemID INT NOT NULL
);
DECLARE @ClientlessSafePetCodes TABLE
(
    PetKind TINYINT NOT NULL,
    CodeName128 VARCHAR(128) NOT NULL,
    PRIMARY KEY (PetKind, CodeName128)
);
DECLARE @ClientlessUnsafePets TABLE
(
    ItemID BIGINT NOT NULL PRIMARY KEY,
    CosID INT NULL,
    SafeRefItemID INT NOT NULL
);
DECLARE @ClientlessAllocationRequests TABLE
(
    RequestID INT IDENTITY(1,1) NOT NULL PRIMARY KEY,
    InventorySlot TINYINT NOT NULL UNIQUE,
    RefItemID INT NOT NULL,
    DataValue INT NOT NULL
);

INSERT INTO @ClientlessSafePetCodes (PetKind, CodeName128)
VALUES
    (1, 'ITEM_COS_P_FLUTE'),
    (1, 'ITEM_COS_P_FLUTE_SILK'),
    (1, 'ITEM_COS_P_FLUTE_WHITE'),
    (1, 'ITEM_COS_P_FLUTE_WHITE_SMALL'),
    (1, 'ITEM_COS_P_JINN_SCROLL'),
    (1, 'ITEM_COS_P_RAVEN_SCROLL'),
    (1, 'ITEM_COS_P_PENGUIN_SCROLL'),
    (1, 'ITEM_COS_P_KANGAROO_SCROLL'),
    (1, 'ITEM_COS_P_BEAR_SCROLL'),
    (1, 'ITEM_COS_P_FOX_SCROLL'),
    (2, 'ITEM_COS_P_MYOWON_SCROLL'),
    (2, 'ITEM_COS_P_SEOWON_SCROLL'),
    (2, 'ITEM_COS_P_SPOT_RABBIT_SCROLL'),
    (2, 'ITEM_COS_P_RABBIT_SCROLL_SILK'),
    (2, 'ITEM_COS_P_GOLDPIG_SCROLL_SILK'),
    (2, 'ITEM_COS_P_GGLIDER_SCROLL'),
    (2, 'ITEM_COS_P_PINKPIG_SCROLL'),
    (2, 'ITEM_COS_P_CAT_SCROLL'),
    (2, 'ITEM_COS_P_RACCOONDOG_SCROLL'),
    (2, 'ITEM_COS_P_BROWNIE_SCROLL');

;WITH SafePetCandidates AS
(
    SELECT Codes.PetKind,
           Common.ID AS RefItemID,
           ROW_NUMBER() OVER(PARTITION BY Codes.PetKind ORDER BY Common.ID) AS CandidateRank,
           COUNT(*) OVER(PARTITION BY Codes.PetKind) AS CandidateCount
    FROM @ClientlessSafePetCodes AS Codes
    INNER JOIN {shardDbName}.dbo._RefObjCommon AS Common WITH (NOLOCK)
        ON Common.CodeName128 COLLATE Latin1_General_CI_AS =
           Codes.CodeName128 COLLATE Latin1_General_CI_AS
       AND Common.TypeID4 = Codes.PetKind
    WHERE Common.Service = 1
      AND Common.TypeID1 = 3 AND Common.TypeID2 = 2 AND Common.TypeID3 = 1
      AND ((Codes.PetKind = 1 AND @ClientlessEnableAttackPet = 1)
           OR (Codes.PetKind = 2 AND @ClientlessEnableGrabPet = 1))
      AND Common.ReqLevel1 <= {characterLevelExpression}
)
INSERT INTO @ClientlessPetExpected (PetKind, RefItemID)
SELECT PetKind, RefItemID
FROM SafePetCandidates
WHERE CandidateRank =
      (ABS(CONVERT(BIGINT, {variationSeedExpression}) + (CONVERT(BIGINT, PetKind) * 7919)) % CandidateCount) + 1;

IF {characterLevelExpression} >= 5 AND
   (SELECT COUNT(1) FROM @ClientlessPetExpected) <> @ClientlessExpectedPetCount
    THROW 51052, 'A selected standard vSRO Clientless pet is unavailable in the active catalog.', 1;

-- Earlier builds accepted every active pet record, including custom records whose
-- client resources may be incomplete. Convert all managed pet scrolls to the two
-- original vSRO allowlist before the runtime is allowed to summon them.
INSERT INTO @ClientlessUnsafePets (ItemID, CosID, SafeRefItemID)
SELECT ExistingItem.ID64,
       NULLIF(CONVERT(INT, ExistingItem.Data), 0),
       Expected.RefItemID
FROM {shardDbName}.dbo._Inventory AS ExistingInventory WITH (UPDLOCK, HOLDLOCK)
INNER JOIN {shardDbName}.dbo._Items AS ExistingItem WITH (UPDLOCK, HOLDLOCK)
    ON ExistingItem.ID64 = ExistingInventory.ItemID
INNER JOIN {shardDbName}.dbo._RefObjCommon AS ExistingCommon WITH (NOLOCK)
    ON ExistingCommon.ID = ExistingItem.RefItemID
INNER JOIN @ClientlessPetExpected AS Expected
    ON Expected.PetKind = ExistingCommon.TypeID4
WHERE ExistingInventory.CharID = {characterIdExpression}
  AND ExistingCommon.TypeID1 = 3 AND ExistingCommon.TypeID2 = 2
  AND ExistingCommon.TypeID3 = 1 AND ExistingCommon.TypeID4 IN (1, 2)
  AND ExistingItem.RefItemID <> Expected.RefItemID;

IF EXISTS
(
    SELECT 1
    FROM {shardDbName}.dbo._InvCOS AS PetInventory WITH (NOLOCK)
    INNER JOIN @ClientlessUnsafePets AS UnsafePet
        ON UnsafePet.CosID = PetInventory.COSID
    WHERE PetInventory.ItemID > 0
)
    THROW 51062, 'A non-standard Clientless grab pet still contains items. Empty it before repairing combat.', 1;

DELETE PetInventory
FROM {shardDbName}.dbo._InvCOS AS PetInventory
INNER JOIN @ClientlessUnsafePets AS UnsafePet
    ON UnsafePet.CosID = PetInventory.COSID;

DELETE PetCharacter
FROM {shardDbName}.dbo._CharCOS AS PetCharacter
INNER JOIN @ClientlessUnsafePets AS UnsafePet
    ON UnsafePet.CosID = PetCharacter.ID
WHERE PetCharacter.OwnerCharID = {characterIdExpression};

DELETE BindingRow
FROM {shardDbName}.dbo._BindingOptionWithItem AS BindingRow
INNER JOIN @ClientlessUnsafePets AS UnsafePet
    ON UnsafePet.ItemID = BindingRow.nItemDBID;

UPDATE ItemRow
SET RefItemID = UnsafePet.SafeRefItemID,
    OptLevel = 0,
    Variance = 0,
    Data = 0,
    MagParamNum = 0,
    CreaterName = NULL
FROM {shardDbName}.dbo._Items AS ItemRow
INNER JOIN @ClientlessUnsafePets AS UnsafePet
    ON UnsafePet.ItemID = ItemRow.ID64;

SET @ClientlessPetKind = 1;
WHILE @ClientlessPetKind <= 2
BEGIN
    SET @ClientlessPetRefItemID = NULL;
    SELECT @ClientlessPetRefItemID = RefItemID
    FROM @ClientlessPetExpected
    WHERE PetKind = @ClientlessPetKind;

    IF ISNULL(@ClientlessPetRefItemID, 0) > 0
       AND NOT EXISTS
       (
           SELECT 1
           FROM {shardDbName}.dbo._Inventory AS ExistingInventory WITH (UPDLOCK, HOLDLOCK)
           INNER JOIN {shardDbName}.dbo._Items AS ExistingItem WITH (NOLOCK)
               ON ExistingItem.ID64 = ExistingInventory.ItemID
           INNER JOIN {shardDbName}.dbo._RefObjCommon AS ExistingCommon WITH (NOLOCK)
               ON ExistingCommon.ID = ExistingItem.RefItemID
           WHERE ExistingInventory.CharID = {characterIdExpression}
             AND ExistingCommon.TypeID1 = 3 AND ExistingCommon.TypeID2 = 2
             AND ExistingCommon.TypeID3 = 1 AND ExistingCommon.TypeID4 = @ClientlessPetKind
       )
    BEGIN
        SET @ClientlessPetInventorySlot = NULL;
        SELECT TOP (1) @ClientlessPetInventorySlot = InventoryRow.Slot
        FROM {shardDbName}.dbo._Inventory AS InventoryRow WITH (UPDLOCK, HOLDLOCK)
        WHERE InventoryRow.CharID = {characterIdExpression}
          AND InventoryRow.ItemID = 0 AND InventoryRow.Slot >= 13
          AND NOT EXISTS
          (
              SELECT 1 FROM @ClientlessAllocationRequests AS Reserved
              WHERE Reserved.InventorySlot = InventoryRow.Slot
          )
        ORDER BY CASE WHEN InventoryRow.Slot = CASE WHEN @ClientlessPetKind = 1 THEN 16 ELSE 17 END
                      THEN 0 ELSE 1 END,
                 InventoryRow.Slot;

        IF @ClientlessPetInventorySlot IS NULL
            THROW 51053, 'No free Clientless inventory slot is available for a pet.', 1;

        INSERT INTO @ClientlessAllocationRequests (InventorySlot, RefItemID, DataValue)
        VALUES (@ClientlessPetInventorySlot, @ClientlessPetRefItemID, 0);
    END

    SET @ClientlessPetKind += 1;
END

IF @ClientlessEnableAttackPet = 1
BEGIN
DECLARE @ClientlessPetSupplyKind TINYINT;
DECLARE @ClientlessPetSupplyRefItemID INT;
DECLARE @ClientlessPetSupplyCount INT;
DECLARE @ClientlessPetSupplySlot TINYINT;
DECLARE @ClientlessPetSupplies TABLE
(
    SupplyKind TINYINT NOT NULL PRIMARY KEY,
    RefItemID INT NOT NULL,
    StackCount INT NOT NULL
);

INSERT INTO @ClientlessPetSupplies (SupplyKind, RefItemID, StackCount)
SELECT SupplyKinds.SupplyKind,
       Picked.RefItemID,
       Picked.StackCount
FROM (VALUES (CONVERT(TINYINT, 1)), (2), (3)) AS SupplyKinds(SupplyKind)
CROSS APPLY
(
    SELECT TOP (1)
           Common.ID AS RefItemID,
           CASE WHEN ISNULL(RefItem.MaxStack, 0) > 0 THEN RefItem.MaxStack ELSE 50 END AS StackCount
    FROM {shardDbName}.dbo._RefObjCommon AS Common WITH (NOLOCK)
    INNER JOIN {shardDbName}.dbo._RefObjItem AS RefItem WITH (NOLOCK)
        ON RefItem.ID = Common.Link
    WHERE Common.Service = 1 AND Common.CashItem = 0
      AND Common.TypeID1 = 3 AND Common.TypeID2 = 3 AND Common.TypeID3 = 1
      AND
      (
          (SupplyKinds.SupplyKind = 1 AND Common.TypeID4 = 4 AND ISNULL(RefItem.Param2, 0) = 0)
          OR (SupplyKinds.SupplyKind = 2 AND Common.TypeID4 = 6)
          OR (SupplyKinds.SupplyKind = 3 AND Common.TypeID4 = 9 AND RefItem.Param1 = 10)
      )
    ORDER BY Common.ReqLevel1 DESC, Common.ID DESC
) AS Picked;

IF {characterLevelExpression} >= 5 AND (SELECT COUNT(1) FROM @ClientlessPetSupplies) <> 3
    THROW 51055, 'Attack-pet HP, revival, and hunger supplies are incomplete in the active vSRO catalog.', 1;

SET @ClientlessPetSupplyKind = 1;
WHILE @ClientlessPetSupplyKind <= 3
BEGIN
    SET @ClientlessPetSupplyRefItemID = NULL;
    SET @ClientlessPetSupplyCount = NULL;
    SELECT @ClientlessPetSupplyRefItemID = RefItemID,
           @ClientlessPetSupplyCount = StackCount
    FROM @ClientlessPetSupplies
    WHERE SupplyKind = @ClientlessPetSupplyKind;

    IF ISNULL(@ClientlessPetSupplyRefItemID, 0) > 0
       AND NOT EXISTS
       (
           SELECT 1
           FROM {shardDbName}.dbo._Inventory AS ExistingInventory WITH (UPDLOCK, HOLDLOCK)
           INNER JOIN {shardDbName}.dbo._Items AS ExistingItem WITH (NOLOCK)
               ON ExistingItem.ID64 = ExistingInventory.ItemID AND ExistingItem.Data > 0
           WHERE ExistingInventory.CharID = {characterIdExpression}
             AND ExistingItem.RefItemID = @ClientlessPetSupplyRefItemID
       )
    BEGIN
        SET @ClientlessPetSupplySlot = NULL;
        SELECT TOP (1) @ClientlessPetSupplySlot = InventoryRow.Slot
        FROM {shardDbName}.dbo._Inventory AS InventoryRow WITH (UPDLOCK, HOLDLOCK)
        WHERE InventoryRow.CharID = {characterIdExpression}
          AND InventoryRow.ItemID = 0 AND InventoryRow.Slot >= 13
          AND NOT EXISTS
          (
              SELECT 1 FROM @ClientlessAllocationRequests AS Reserved
              WHERE Reserved.InventorySlot = InventoryRow.Slot
          )
        ORDER BY CASE WHEN InventoryRow.Slot = 17 + @ClientlessPetSupplyKind THEN 0 ELSE 1 END,
                 InventoryRow.Slot;

        IF @ClientlessPetSupplySlot IS NULL
            THROW 51056, 'No free Clientless inventory slot is available for pet supplies.', 1;

        INSERT INTO @ClientlessAllocationRequests (InventorySlot, RefItemID, DataValue)
        VALUES (@ClientlessPetSupplySlot, @ClientlessPetSupplyRefItemID, @ClientlessPetSupplyCount);
    END

    SET @ClientlessPetSupplyKind += 1;
END
END

DECLARE @ClientlessRequestCount INT = (SELECT COUNT(1) FROM @ClientlessAllocationRequests);
IF @ClientlessRequestCount > 0
BEGIN
    DECLARE @ClientlessAllocatedItems TABLE
    (
        AllocationOrder INT IDENTITY(1,1) NOT NULL PRIMARY KEY,
        ItemID BIGINT NOT NULL UNIQUE
    );

    INSERT INTO @ClientlessAllocatedItems (ItemID)
    SELECT TOP (@ClientlessRequestCount) Pool.ItemID
    FROM {shardDbName}.dbo._ItemPool AS Pool WITH (UPDLOCK, READPAST)
    WHERE Pool.InUse = 0
    ORDER BY Pool.ItemID;

    DECLARE @ClientlessNewItemID BIGINT;
    WHILE (SELECT COUNT(1) FROM @ClientlessAllocatedItems) < @ClientlessRequestCount
    BEGIN
        INSERT INTO {shardDbName}.dbo._Items
            (RefItemID, OptLevel, Data, MagParamNum, Serial64)
        VALUES (0, 0, 0, 0, 0);
        SET @ClientlessNewItemID = CONVERT(BIGINT, SCOPE_IDENTITY());
        IF ISNULL(@ClientlessNewItemID, 0) <= 0
            THROW 51057, 'vSRO could not expand its item pool for Clientless pets.', 1;
        INSERT INTO {shardDbName}.dbo._ItemPool (ItemID, InUse)
        VALUES (@ClientlessNewItemID, 0);
        INSERT INTO @ClientlessAllocatedItems (ItemID)
        VALUES (@ClientlessNewItemID);
    END

    DECLARE @ClientlessLastSerial BIGINT;
    UPDATE {shardDbName}.dbo._LatestItemSerial WITH (UPDLOCK, HOLDLOCK)
    SET LatestItemSerial = LatestItemSerial + @ClientlessRequestCount;
    SELECT @ClientlessLastSerial = LatestItemSerial
    FROM {shardDbName}.dbo._LatestItemSerial WITH (UPDLOCK, HOLDLOCK);

    DELETE BindingRow
    FROM {shardDbName}.dbo._BindingOptionWithItem AS BindingRow
    INNER JOIN @ClientlessAllocatedItems AS Allocated
        ON Allocated.ItemID = BindingRow.nItemDBID;

    UPDATE ItemRow
    SET RefItemID = Request.RefItemID,
        OptLevel = 0,
        Variance = 0,
        Data = Request.DataValue,
        MagParamNum = 0,
        CreaterName = NULL,
        Serial64 = @ClientlessLastSerial - @ClientlessRequestCount + Allocated.AllocationOrder
    FROM {shardDbName}.dbo._Items AS ItemRow
    INNER JOIN @ClientlessAllocatedItems AS Allocated
        ON Allocated.ItemID = ItemRow.ID64
    INNER JOIN @ClientlessAllocationRequests AS Request
        ON Request.RequestID = Allocated.AllocationOrder;
    IF @@ROWCOUNT <> @ClientlessRequestCount
        THROW 51058, 'vSRO could not initialize all Clientless pet items.', 1;

    UPDATE Pool
    SET InUse = 1
    FROM {shardDbName}.dbo._ItemPool AS Pool
    INNER JOIN @ClientlessAllocatedItems AS Allocated
        ON Allocated.ItemID = Pool.ItemID;
    IF @@ROWCOUNT <> @ClientlessRequestCount
        THROW 51060, 'vSRO could not reserve all Clientless pet item-pool rows.', 1;

    UPDATE InventoryRow
    SET ItemID = Allocated.ItemID
    FROM {shardDbName}.dbo._Inventory AS InventoryRow
    INNER JOIN @ClientlessAllocationRequests AS Request
        ON Request.InventorySlot = InventoryRow.Slot
    INNER JOIN @ClientlessAllocatedItems AS Allocated
        ON Allocated.AllocationOrder = Request.RequestID
    WHERE InventoryRow.CharID = {characterIdExpression}
      AND InventoryRow.ItemID = 0;
    IF @@ROWCOUNT <> @ClientlessRequestCount
        THROW 51061, 'vSRO could not place all Clientless pet items in inventory.', 1;

    SET @ClientlessPetsAdded = @ClientlessRequestCount;
END

IF {characterLevelExpression} >= 5 AND
   (SELECT COUNT(DISTINCT Expected.PetKind)
    FROM {shardDbName}.dbo._Inventory AS InventoryRow WITH (NOLOCK)
    INNER JOIN {shardDbName}.dbo._Items AS ItemRow WITH (NOLOCK)
        ON ItemRow.ID64 = InventoryRow.ItemID
    INNER JOIN @ClientlessPetExpected AS Expected
        ON Expected.RefItemID = ItemRow.RefItemID
    WHERE InventoryRow.CharID = {characterIdExpression}) <> @ClientlessExpectedPetCount
    THROW 51063, 'The selected standard Clientless vSRO pets failed final inventory validation.', 1;";

    internal static string BuildOptionalClientlessAvatarEquipmentSql(
        string shardDbName,
        string characterIdExpression,
        string genderExpression,
        string variationSeedExpression,
        bool enabled) => enabled
            ? BuildClientlessAvatarEquipmentSql(
                shardDbName, characterIdExpression, genderExpression, variationSeedExpression)
            : string.Empty;

    internal static string BuildOptionalClientlessPetEquipmentSql(
        string shardDbName,
        string characterIdExpression,
        string characterLevelExpression,
        string variationSeedExpression,
        bool includeAttackPet,
        bool includeGrabPet) => includeAttackPet || includeGrabPet
            ? BuildClientlessPetEquipmentSql(
                shardDbName,
                characterIdExpression,
                characterLevelExpression,
                variationSeedExpression,
                includeAttackPet ? "1" : "0",
                includeGrabPet ? "1" : "0")
            : string.Empty;

    internal static string BuildClientlessMonsterAttackSkillPredicate(string alias)
    {
        var attackParameter = string.Join(
            " OR ",
            Enumerable.Range(1, 50).Select(index => $"ISNULL({alias}.Param{index}, 0) = @AttackParam"));
        return $@"
({attackParameter})
AND ISNULL({alias}.TargetEtc_SelectDeadBody, 0) = 0
AND {alias}.Basic_Code NOT LIKE '%DOWNATTACK%'
AND {alias}.Basic_Code NOT LIKE '%RESURRECT%'
AND {alias}.Basic_Code NOT LIKE '%[_]BASE[_]%'
AND {alias}.Basic_Code NOT LIKE '%SACRIFICE%'
AND NOT
(
    ISNULL({alias}.Basic_Activity, 0) = 1
    AND ISNULL({alias}.Target_Required, 0) = 0
    AND ISNULL({alias}.TargetGroup_Enemy_M, 0) = 0
)";
    }

    internal static string BuildClientlessWeaponSkillCompatibilityPredicate(
        string alias,
        string weaponTypeExpression)
    {
        var marker = string.Join(
            " OR ",
            Enumerable.Range(1, 50).Select(index => $"ISNULL({alias}.Param{index}, 0) = @RequiredItemParam"));
        var matchingMarker = string.Join(
            " OR ",
            Enumerable.Range(1, 48).Select(index => $@"(
                {alias}.Param{index} = @RequiredItemParam
                AND {alias}.Param{index + 1} = 6
                AND {alias}.Param{index + 2} = {weaponTypeExpression})"));
        return $@"(
    (ISNULL({alias}.ReqCast_Weapon1, 255) = 255 AND (NOT ({marker}) OR ({matchingMarker})))
    OR {alias}.ReqCast_Weapon1 = {weaponTypeExpression}
    OR (ISNULL({alias}.ReqCast_Weapon2, 255) <> 255 AND {alias}.ReqCast_Weapon2 = {weaponTypeExpression})
)";
    }

    internal static string BuildClientlessUnlockedAttackSkillExistsSql(string shardDbName)
    {
        var attackSkillPredicate = BuildClientlessMonsterAttackSkillPredicate("AvailableAttackSkill");
        var weaponSkillPredicate = BuildClientlessWeaponSkillCompatibilityPredicate(
            "AvailableAttackSkill", "@WeaponTypeID4");
        return $@"EXISTS
(
    SELECT 1
    FROM {shardDbName}.dbo._RefSkill AS AvailableAttackSkill WITH (NOLOCK)
    WHERE AvailableAttackSkill.Service = 1
      AND AvailableAttackSkill.GroupID > 0
      AND AvailableAttackSkill.ReqCommon_Mastery1 = @MasteryID
      AND AvailableAttackSkill.Basic_Level <= @Level
      AND AvailableAttackSkill.ReqCommon_MasteryLevel1 <= @Level
      AND ISNULL(AvailableAttackSkill.ReqCommon_Str, 0) <= @Strength
      AND ISNULL(AvailableAttackSkill.ReqCommon_Int, 0) <= @Intellect
      AND
      (
          ISNULL(AvailableAttackSkill.ReqCommon_Mastery2, 0) = 0
          OR EXISTS
          (
              SELECT 1
              FROM @ClientlessDesiredMasteries AS SecondaryMastery
              WHERE SecondaryMastery.MasteryID = AvailableAttackSkill.ReqCommon_Mastery2
                AND SecondaryMastery.MasteryLevel >= AvailableAttackSkill.ReqCommon_MasteryLevel2
          )
      )
      AND ({attackSkillPredicate})
      AND ({weaponSkillPredicate})
)";
    }

    private static async Task<int> RebuildExistingClientlessEquipmentAsync(
        SqlConnection connection,
        string accountDb,
        string shardDb)
    {
        var accountDbName = QuoteDb(accountDb);
        var shardDbName = QuoteDb(shardDb);
        var ownershipJoin = BuildClientlessOwnedCharacterJoinSql(accountDbName, shardDbName);
        var sql = $@"
SET NOCOUNT ON;
SET XACT_ABORT ON;
BEGIN TRANSACTION;

DECLARE @CharID INT;
DECLARE @Country TINYINT;
DECLARE @Gender TINYINT;
DECLARE @CharacterLevel INT;
DECLARE @WeaponTypeID4 TINYINT;
DECLARE @ArmorTypeID3 TINYINT;
DECLARE @UseShield BIT;
DECLARE @EquipmentMode VARCHAR(16);
DECLARE @Rarity TINYINT;
DECLARE @Rebuilt INT = 0;
DECLARE @Resolved TABLE (Slot TINYINT PRIMARY KEY, RefItemID INT NOT NULL, DataValue INT NOT NULL);

-- Repair the persisted profile first. Older releases stored the European
-- robe/heavy TypeID3 values in reverse, so trusting ProfileArmorTypeID3 would
-- simply recreate the same invisible or class-incompatible equipment.
UPDATE AccountRow
SET ProfileArmorTypeID3 = CASE
        WHEN AccountRow.ProfileWeaponTypeID4 IN (7, 8, 9) THEN 11
        WHEN AccountRow.ProfileWeaponTypeID4 IN (12, 13) THEN 10
        WHEN AccountRow.ProfileWeaponTypeID4 BETWEEN 10 AND 15 THEN 9
        ELSE AccountRow.ProfileArmorTypeID3
    END,
    UpdatedAt = SYSDATETIME()
FROM dbo.Clientless_Accounts AS AccountRow
WHERE AccountRow.ProfileWeaponTypeID4 BETWEEN 7 AND 15
  AND NULLIF(LTRIM(RTRIM(ISNULL(AccountRow.SystemRole, ''))), '') IS NULL
  AND ISNULL(AccountRow.ProfileArmorTypeID3, 0) <> CASE
        WHEN AccountRow.ProfileWeaponTypeID4 IN (7, 8, 9) THEN 11
        WHEN AccountRow.ProfileWeaponTypeID4 IN (12, 13) THEN 10
        ELSE 9
    END;

DECLARE EquipmentCharacters CURSOR LOCAL FAST_FORWARD FOR
SELECT DISTINCT
    C.CharID,
    CONVERT(TINYINT, CharacterCommon.Country),
    CONVERT(TINYINT, CharacterRef.CharGender),
    CONVERT(INT, C.CurLevel),
    CONVERT(TINYINT, COALESCE(AccountRow.ProfileWeaponTypeID4, CurrentWeapon.TypeID4,
        CASE WHEN CharacterCommon.Country = 0 THEN 2 ELSE 11 END)),
    CONVERT(TINYINT, CASE
        WHEN CharacterCommon.Country = 1 THEN CASE
            WHEN COALESCE(AccountRow.ProfileWeaponTypeID4, CurrentWeapon.TypeID4, 11) IN (7, 8, 9) THEN 11
            WHEN COALESCE(AccountRow.ProfileWeaponTypeID4, CurrentWeapon.TypeID4, 11) IN (12, 13) THEN 10
            ELSE 9
        END
        ELSE COALESCE(AccountRow.ProfileArmorTypeID3, CurrentArmor.TypeID3, 1)
    END),
    CONVERT(BIT, COALESCE(AccountRow.ProfileUseShield,
        CASE WHEN COALESCE(AccountRow.ProfileWeaponTypeID4, CurrentWeapon.TypeID4, 11) IN (2, 3, 7, 15) THEN 1 ELSE 0 END)),
    CONVERT(VARCHAR(16), COALESCE(NULLIF(AccountRow.ProfileEquipmentMode, ''),
        CASE WHEN CurrentRarity.HasRare = 1 THEN 'Rare' ELSE 'Normal' END))
FROM dbo.Clientless_Accounts AS AccountRow
{ownershipJoin}
INNER JOIN {shardDbName}.dbo._RefObjCommon AS CharacterCommon WITH (NOLOCK)
    ON CharacterCommon.ID = C.RefObjID
INNER JOIN {shardDbName}.dbo._RefObjChar AS CharacterRef WITH (NOLOCK)
    ON CharacterRef.ID = CharacterCommon.Link
OUTER APPLY
(
    SELECT TOP (1) Common.TypeID4
    FROM {shardDbName}.dbo._Inventory AS InventoryRow WITH (NOLOCK)
    INNER JOIN {shardDbName}.dbo._Items AS ItemRow WITH (NOLOCK)
        ON ItemRow.ID64 = InventoryRow.ItemID
    INNER JOIN {shardDbName}.dbo._RefObjCommon AS Common WITH (NOLOCK)
        ON Common.ID = ItemRow.RefItemID
    WHERE InventoryRow.CharID = C.CharID AND InventoryRow.Slot = 6
      AND Common.TypeID1 = 3 AND Common.TypeID2 = 1 AND Common.TypeID3 = 6
) AS CurrentWeapon
OUTER APPLY
(
    SELECT TOP (1) Common.TypeID3
    FROM {shardDbName}.dbo._Inventory AS InventoryRow WITH (NOLOCK)
    INNER JOIN {shardDbName}.dbo._Items AS ItemRow WITH (NOLOCK)
        ON ItemRow.ID64 = InventoryRow.ItemID
    INNER JOIN {shardDbName}.dbo._RefObjCommon AS Common WITH (NOLOCK)
        ON Common.ID = ItemRow.RefItemID
    WHERE InventoryRow.CharID = C.CharID AND InventoryRow.Slot BETWEEN 0 AND 5
      AND Common.TypeID1 = 3 AND Common.TypeID2 = 1
      AND Common.TypeID3 IN (1, 2, 3, 9, 10, 11)
    ORDER BY InventoryRow.Slot
) AS CurrentArmor
OUTER APPLY
(
    SELECT CONVERT(BIT, CASE WHEN EXISTS
    (
        SELECT 1
        FROM {shardDbName}.dbo._Inventory AS InventoryRow WITH (NOLOCK)
        INNER JOIN {shardDbName}.dbo._Items AS ItemRow WITH (NOLOCK) ON ItemRow.ID64 = InventoryRow.ItemID
        INNER JOIN {shardDbName}.dbo._RefObjCommon AS Common WITH (NOLOCK) ON Common.ID = ItemRow.RefItemID
        WHERE InventoryRow.CharID = C.CharID AND InventoryRow.Slot BETWEEN 0 AND 6 AND Common.Rarity = 2
    ) THEN 1 ELSE 0 END) AS HasRare
) AS CurrentRarity
WHERE NULLIF(LTRIM(RTRIM(ISNULL(AccountRow.SystemRole, ''))), '') IS NULL
  AND COALESCE(NULLIF(AccountRow.ProfileEquipmentMode, ''), 'Normal') <> 'None';

OPEN EquipmentCharacters;
FETCH NEXT FROM EquipmentCharacters INTO
    @CharID, @Country, @Gender, @CharacterLevel, @WeaponTypeID4,
    @ArmorTypeID3, @UseShield, @EquipmentMode;
WHILE @@FETCH_STATUS = 0
BEGIN
    SET @Rarity = CASE WHEN @EquipmentMode = 'Rare' THEN 2 ELSE 0 END;
    -- Table variables live for the whole SQL batch, even when DECLARE appears
    -- inside a cursor loop. Clear the previous character before resolving the
    -- next equipment set or slot 0 immediately violates the primary key.
    DELETE FROM @Resolved;

    INSERT INTO @Resolved (Slot, RefItemID, DataValue)
    SELECT Slots.Slot, Picked.RefItemID, Picked.DataValue
    FROM
    (
        VALUES (CONVERT(TINYINT,0),CONVERT(TINYINT,1)), (1,3), (2,2), (3,5), (4,4), (5,6)
    ) AS Slots(Slot, ItemTypeID4)
    CROSS APPLY
    (
        SELECT TOP (1) Common.ID AS RefItemID,
               CONVERT(INT, CEILING(ISNULL(Item.Dur_L, 0))) AS DataValue
        FROM {shardDbName}.dbo._RefObjCommon AS Common WITH (NOLOCK)
        INNER JOIN {shardDbName}.dbo._RefObjItem AS Item WITH (NOLOCK) ON Item.ID = Common.Link
        WHERE Common.Service = 1 AND Common.CashItem = 0
          AND Common.Country = @Country
          AND Common.TypeID1 = 3 AND Common.TypeID2 = 1
          AND Common.TypeID3 = @ArmorTypeID3 AND Common.TypeID4 = Slots.ItemTypeID4
          AND Item.ReqGender = @Gender AND Common.ReqLevel1 <= @CharacterLevel
          AND Common.Rarity = @Rarity
          AND (@Rarity <> 2 OR Common.CodeName128 LIKE '%[_]C[_]RARE')
          AND Common.CodeName128 NOT LIKE '%SET%' AND Common.CodeName128 NOT LIKE '%MALL%'
          AND Common.CodeName128 NOT LIKE '%EVENT%' AND Common.CodeName128 NOT LIKE '%TEST%'
        ORDER BY Common.ReqLevel1 DESC, Common.ID ASC
    ) AS Picked;

    INSERT INTO @Resolved (Slot, RefItemID, DataValue)
    SELECT TOP (1) 6, Common.ID, CONVERT(INT, CEILING(ISNULL(Item.Dur_L, 0)))
    FROM {shardDbName}.dbo._RefObjCommon AS Common WITH (NOLOCK)
    INNER JOIN {shardDbName}.dbo._RefObjItem AS Item WITH (NOLOCK) ON Item.ID = Common.Link
    WHERE Common.Service = 1 AND Common.CashItem = 0 AND Common.Country = @Country
      AND Common.TypeID1 = 3 AND Common.TypeID2 = 1 AND Common.TypeID3 = 6 AND Common.TypeID4 = @WeaponTypeID4
      AND Common.ReqLevel1 <= @CharacterLevel AND Common.Rarity = @Rarity
      AND (@Rarity <> 2 OR Common.CodeName128 LIKE '%[_]C[_]RARE')
      AND Common.CodeName128 NOT LIKE '%SET%' AND Common.CodeName128 NOT LIKE '%MALL%'
      AND Common.CodeName128 NOT LIKE '%EVENT%' AND Common.CodeName128 NOT LIKE '%TEST%'
    ORDER BY Common.ReqLevel1 DESC, Common.ID ASC;

    IF @UseShield = 1
    BEGIN
        INSERT INTO @Resolved (Slot, RefItemID, DataValue)
        SELECT TOP (1) 7, Common.ID, CONVERT(INT, CEILING(ISNULL(Item.Dur_L, 0)))
        FROM {shardDbName}.dbo._RefObjCommon AS Common WITH (NOLOCK)
        INNER JOIN {shardDbName}.dbo._RefObjItem AS Item WITH (NOLOCK) ON Item.ID = Common.Link
        WHERE Common.Service = 1 AND Common.CashItem = 0 AND Common.Country = @Country
          AND Common.TypeID1 = 3 AND Common.TypeID2 = 1 AND Common.TypeID3 = 4
          AND Common.TypeID4 = CASE WHEN @Country = 0 THEN 1 ELSE 2 END
          AND Common.ReqLevel1 <= @CharacterLevel AND Common.Rarity = @Rarity
          AND (@Rarity <> 2 OR Common.CodeName128 LIKE '%[_]C[_]RARE')
          AND Common.CodeName128 NOT LIKE '%SET%' AND Common.CodeName128 NOT LIKE '%MALL%'
          AND Common.CodeName128 NOT LIKE '%EVENT%' AND Common.CodeName128 NOT LIKE '%TEST%'
        ORDER BY Common.ReqLevel1 DESC, Common.ID ASC;
    END
    ELSE IF @WeaponTypeID4 IN (6, 12)
    BEGIN
        INSERT INTO @Resolved (Slot, RefItemID, DataValue)
        SELECT TOP (1) 7, Common.ID, CASE WHEN ISNULL(Item.MaxStack, 0) > 0 THEN Item.MaxStack ELSE 1000 END
        FROM {shardDbName}.dbo._RefObjCommon AS Common WITH (NOLOCK)
        INNER JOIN {shardDbName}.dbo._RefObjItem AS Item WITH (NOLOCK) ON Item.ID = Common.Link
        WHERE Common.Service = 1 AND Common.CashItem = 0
          AND Common.TypeID1 = 3 AND Common.TypeID2 = 3 AND Common.TypeID3 = 4
          AND Common.TypeID4 = CASE WHEN @WeaponTypeID4 = 6 THEN 1 ELSE 2 END
          AND Common.ReqLevel1 <= @CharacterLevel
          AND Common.CodeName128 NOT LIKE '%EVENT%' AND Common.CodeName128 NOT LIKE '%MALL%'
        ORDER BY Common.ReqLevel1 DESC, Common.ID ASC;
    END

    IF (SELECT COUNT(1) FROM @Resolved WHERE Slot BETWEEN 0 AND 6) <> 7
        THROW 51040, 'Prepare Existing could not resolve a complete visible equipment set.', 1;
    IF (@UseShield = 1 OR @WeaponTypeID4 IN (6,12)) AND NOT EXISTS (SELECT 1 FROM @Resolved WHERE Slot = 7)
        THROW 51041, 'Prepare Existing could not resolve the required offhand item.', 1;

    UPDATE InventoryRow
    SET ItemID = 0
    FROM {shardDbName}.dbo._Inventory AS InventoryRow
    WHERE InventoryRow.CharID = @CharID AND InventoryRow.Slot BETWEEN 0 AND 7;

    DECLARE @Slot TINYINT = 0;
    DECLARE @RefItemID INT;
    DECLARE @DataValue INT;
    DECLARE @Result INT;
    WHILE @Slot <= 7
    BEGIN
        SET @RefItemID = NULL;
        SELECT @RefItemID = RefItemID, @DataValue = DataValue FROM @Resolved WHERE Slot = @Slot;
        IF ISNULL(@RefItemID, 0) > 0
        BEGIN
            SET @Result = NULL;
            EXEC @Result = {shardDbName}.dbo._FN_ADD_INITIAL_EQUIP
                @CharID = @CharID, @Slot = @Slot, @RefItemID = @RefItemID, @Data = @DataValue;
            IF ISNULL(@Result, -1) < 0
                THROW 51042, 'vSRO rejected a repaired Clientless equipment item.', 1;
        END
        SET @Slot += 1;
    END

    IF (SELECT COUNT(1)
        FROM {shardDbName}.dbo._Inventory AS InventoryRow
        INNER JOIN {shardDbName}.dbo._Items AS ItemRow ON ItemRow.ID64 = InventoryRow.ItemID AND ItemRow.Data > 0
        INNER JOIN @Resolved AS Expected ON Expected.Slot = InventoryRow.Slot AND Expected.RefItemID = ItemRow.RefItemID
        INNER JOIN {shardDbName}.dbo._RefObjCommon AS Common ON Common.ID = ItemRow.RefItemID AND Common.Service = 1
        INNER JOIN {shardDbName}.dbo._RefObjItem AS RefItem ON RefItem.ID = Common.Link
        WHERE InventoryRow.CharID = @CharID AND InventoryRow.Slot BETWEEN 0 AND 6) <> 7
        THROW 51043, 'Prepared Clientless equipment failed the final inventory/reference validation.', 1;

    SET @Rebuilt += 1;
    FETCH NEXT FROM EquipmentCharacters INTO
        @CharID, @Country, @Gender, @CharacterLevel, @WeaponTypeID4,
        @ArmorTypeID3, @UseShield, @EquipmentMode;
END
CLOSE EquipmentCharacters;
DEALLOCATE EquipmentCharacters;

COMMIT TRANSACTION;
SELECT @Rebuilt;";

        await using var command = new SqlCommand(sql, connection) { CommandTimeout = 300 };
        return Convert.ToInt32(await command.ExecuteScalarAsync());
    }

    private static async Task<int> EquipExistingClientlessAvatarsAsync(
        SqlConnection connection,
        string accountDb,
        string shardDb)
    {
        var accountDbName = QuoteDb(accountDb);
        var shardDbName = QuoteDb(shardDb);
        var ownershipJoin = BuildClientlessOwnedCharacterJoinSql(accountDbName, shardDbName);
        var avatarEquipmentSql = BuildClientlessAvatarEquipmentSql(
            shardDbName,
            "@AvatarCharID",
            "@AvatarGender",
            "@AvatarCharID");
        var sql = $@"
SET NOCOUNT ON;
SET XACT_ABORT ON;
BEGIN TRANSACTION;

DECLARE @AvatarCharID INT;
DECLARE @AvatarGender TINYINT;
DECLARE @AvatarRepaired INT = 0;

DECLARE AvatarCharacters CURSOR LOCAL FAST_FORWARD FOR
SELECT DISTINCT C.CharID, CONVERT(TINYINT, CharacterRef.CharGender)
FROM dbo.Clientless_Accounts AS AccountRow
{ownershipJoin}
INNER JOIN {shardDbName}.dbo._RefObjCommon AS CharacterCommon WITH (NOLOCK)
    ON CharacterCommon.ID = C.RefObjID AND CharacterCommon.Service = 1
INNER JOIN {shardDbName}.dbo._RefObjChar AS CharacterRef WITH (NOLOCK)
    ON CharacterRef.ID = CharacterCommon.Link
WHERE NULLIF(LTRIM(RTRIM(ISNULL(AccountRow.SystemRole, ''))), '') IS NULL
  AND ISNULL(AccountRow.ProfileAvatarsEnabled, 1) = 1;

OPEN AvatarCharacters;
FETCH NEXT FROM AvatarCharacters INTO @AvatarCharID, @AvatarGender;
WHILE @@FETCH_STATUS = 0
BEGIN
    {avatarEquipmentSql}
    IF @AvatarSetEquipped = 1
        SET @AvatarRepaired += 1;

    FETCH NEXT FROM AvatarCharacters INTO @AvatarCharID, @AvatarGender;
END
CLOSE AvatarCharacters;
DEALLOCATE AvatarCharacters;

COMMIT TRANSACTION;
SELECT @AvatarRepaired;";

        await using var command = new SqlCommand(sql, connection) { CommandTimeout = 300 };
        return Convert.ToInt32(await command.ExecuteScalarAsync());
    }

    private static async Task<int> EquipExistingClientlessPetsAsync(
        SqlConnection connection,
        string accountDb,
        string shardDb)
    {
        var accountDbName = QuoteDb(accountDb);
        var shardDbName = QuoteDb(shardDb);
        var ownershipJoin = BuildClientlessOwnedCharacterJoinSql(accountDbName, shardDbName);
        var petEquipmentSql = BuildClientlessPetEquipmentSql(
            shardDbName,
            "@PetCharID",
            "@PetCharacterLevel",
            "@PetCharID",
            "@PetAttackEnabled",
            "@PetGrabEnabled");
        var sql = $@"
SET NOCOUNT ON;
SET XACT_ABORT ON;
BEGIN TRANSACTION;

DECLARE @PetCharID INT;
DECLARE @PetCharacterLevel INT;
DECLARE @PetAttackEnabled BIT;
DECLARE @PetGrabEnabled BIT;
DECLARE @PetItemsAdded INT = 0;

DECLARE PetCharacters CURSOR LOCAL FAST_FORWARD FOR
SELECT DISTINCT C.CharID,
       CONVERT(INT, C.CurLevel),
       ISNULL(AccountRow.ProfileAttackPetEnabled, ISNULL(AccountRow.ProfilePetsEnabled, 1)),
       ISNULL(AccountRow.ProfileGrabPetEnabled, ISNULL(AccountRow.ProfilePetsEnabled, 1))
FROM dbo.Clientless_Accounts AS AccountRow
{ownershipJoin}
WHERE NULLIF(LTRIM(RTRIM(ISNULL(AccountRow.SystemRole, ''))), '') IS NULL
  AND (ISNULL(AccountRow.ProfileAttackPetEnabled, ISNULL(AccountRow.ProfilePetsEnabled, 1)) = 1
       OR ISNULL(AccountRow.ProfileGrabPetEnabled, ISNULL(AccountRow.ProfilePetsEnabled, 1)) = 1);

OPEN PetCharacters;
FETCH NEXT FROM PetCharacters INTO @PetCharID, @PetCharacterLevel, @PetAttackEnabled, @PetGrabEnabled;
WHILE @@FETCH_STATUS = 0
BEGIN
    {petEquipmentSql}
    SET @PetItemsAdded += @ClientlessPetsAdded;

    FETCH NEXT FROM PetCharacters INTO @PetCharID, @PetCharacterLevel, @PetAttackEnabled, @PetGrabEnabled;
END
CLOSE PetCharacters;
DEALLOCATE PetCharacters;

COMMIT TRANSACTION;
SELECT @PetItemsAdded;";

        await using var command = new SqlCommand(sql, connection) { CommandTimeout = 300 };
        return Convert.ToInt32(await command.ExecuteScalarAsync());
    }

    public async Task<string> PrepareExistingClientlessWeaponSkillsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureClientlessSchemaAsync(connection);

        var accountDb = await ReadSettingAsync(connection, "AccountDB", "SRO_VT_ACCOUNT");
        var shardDb = await ReadSettingAsync(connection, "ShardDB", "SRO_VT_SHARD");
        var accountDbName = QuoteDb(accountDb);
        var shardDbName = QuoteDb(shardDb);
        var chineseMasteryCap = await ReadGameServerSettingIntAsync(connection, "CH_MAX_MASTERY_LEVEL", 300, 1, 10000);
        var europeanMasteryCap = await ReadGameServerSettingIntAsync(connection, "EU_MAX_MASTERY_LEVEL", 240, 1, 10000);
        var ownedCharacterJoinSql = BuildClientlessOwnedCharacterJoinSql(accountDbName, shardDbName);
        var masteryCapGuardSql = BuildClientlessMasteryCapGuardSql();
        var unusedMasteryResetSql = BuildClientlessUnusedMasteryResetSql(shardDbName);
        var obsoleteSkillDeleteSql = BuildClientlessObsoleteSkillDeleteSql(shardDbName);
        var equipmentRepaired = await RebuildExistingClientlessEquipmentAsync(connection, accountDb, shardDb);
        var avatarsEquipped = await EquipExistingClientlessAvatarsAsync(connection, accountDb, shardDb);
        var petItemsAdded = await EquipExistingClientlessPetsAsync(connection, accountDb, shardDb);
        var attackSkillPredicate = BuildClientlessMonsterAttackSkillPredicate("AttackSkill");
        var weaponSkillPredicate = BuildClientlessWeaponSkillCompatibilityPredicate("AttackSkill", "ManagedCharacter.WeaponTypeID4");

        var sql = $@"
SET NOCOUNT ON;
SET XACT_ABORT ON;

IF DB_ID(@AccountDbName) IS NULL
    THROW 51029, 'Account database was not found.', 1;

IF DB_ID(@ShardDbName) IS NULL
    THROW 51030, 'Shard database was not found.', 1;

IF OBJECT_ID(N'{accountDb}.dbo.TB_User', N'U') IS NULL
    THROW 51033, 'Account ownership table was not found.', 1;

IF OBJECT_ID(N'{shardDb}.dbo._User', N'U') IS NULL
    THROW 51034, 'Character ownership table was not found.', 1;

IF OBJECT_ID(N'{shardDb}.dbo._CharSkillMastery', N'U') IS NULL
    THROW 51031, 'Character mastery table was not found.', 1;

IF OBJECT_ID(N'{shardDb}.dbo._CharSkill', N'U') IS NULL
    THROW 51032, 'Character skill table was not found.', 1;

BEGIN TRANSACTION;

DECLARE @TotalAccounts INT = (SELECT COUNT(1) FROM dbo.Clientless_Accounts WITH (NOLOCK));

SELECT DISTINCT
    C.CharID,
    CONVERT(INT, Weapon.TypeID4) AS WeaponTypeID4,
    CONVERT(BIT, CASE WHEN Weapon.TypeID4 BETWEEN 2 AND 6 THEN 1 ELSE 0 END) AS IsChinese,
    Mapping.MasteryID AS PrimaryMasteryID,
    CONVERT(INT, C.CurLevel) AS CharacterLevel,
    CONVERT(INT, CASE WHEN Weapon.TypeID4 BETWEEN 2 AND 6 THEN @ChineseMasteryCap ELSE @EuropeanMasteryCap END) AS TotalMasteryCap,
    CONVERT(INT, C.Strength) AS Strength,
    CONVERT(INT, C.Intellect) AS Intellect
INTO #ClientlessCharacters
FROM dbo.Clientless_Accounts AS AccountRow
{ownedCharacterJoinSql}
INNER JOIN {shardDbName}.dbo._Inventory AS InventorySlot WITH (NOLOCK)
    ON InventorySlot.CharID = C.CharID AND InventorySlot.Slot = 6 AND InventorySlot.ItemID > 0
INNER JOIN {shardDbName}.dbo._Items AS EquippedItem WITH (NOLOCK)
    ON EquippedItem.ID64 = InventorySlot.ItemID AND EquippedItem.Data > 0
INNER JOIN {shardDbName}.dbo._RefObjCommon AS Weapon WITH (NOLOCK)
    ON Weapon.ID = EquippedItem.RefItemID
CROSS APPLY
(
    VALUES
    (
        CASE
            WHEN Weapon.TypeID4 IN (2, 3) THEN 257
            WHEN Weapon.TypeID4 IN (4, 5) THEN 258
            WHEN Weapon.TypeID4 = 6 THEN 259
            WHEN Weapon.TypeID4 IN (7, 8, 9) THEN 513
            WHEN Weapon.TypeID4 = 11 THEN 514
            WHEN Weapon.TypeID4 IN (12, 13) THEN 515
            WHEN Weapon.TypeID4 = 10 THEN 516
            WHEN Weapon.TypeID4 = 14 THEN 517
            WHEN Weapon.TypeID4 = 15 THEN 518
            ELSE NULL
        END
    )
) AS Mapping(MasteryID)
WHERE Weapon.Service = 1
  AND Weapon.TypeID1 = 3 AND Weapon.TypeID2 = 1 AND Weapon.TypeID3 = 6
  AND Mapping.MasteryID IS NOT NULL;

CREATE UNIQUE CLUSTERED INDEX IX_ClientlessCharacters
    ON #ClientlessCharacters (CharID);

-- Preserve the selected STR/INT build while applying the same large survival
-- reserve to both stats. The GameServer then derives genuinely high maximum
-- HP and MP from the persisted character stats instead of seeing a temporary
-- oversized current-value only.
UPDATE CharacterRow
SET Strength = Desired.Strength,
    Intellect = Desired.Intellect,
    HP = CASE WHEN CharacterRow.HP < Vital.HP THEN Vital.HP ELSE CharacterRow.HP END,
    MP = CASE WHEN CharacterRow.MP < Vital.MP THEN Vital.MP ELSE CharacterRow.MP END,
    RemainStatPoint = 0
FROM {shardDbName}.dbo._Char AS CharacterRow
INNER JOIN #ClientlessCharacters AS Target ON Target.CharID = CharacterRow.CharID
CROSS APPLY
(
    VALUES
    (
        20 + (Target.CharacterLevel - 1) +
        CASE
            WHEN Target.Strength > Target.Intellect THEN (Target.CharacterLevel - 1) * 3
            WHEN Target.Strength = Target.Intellect AND Target.WeaponTypeID4 NOT IN (10, 11, 14, 15)
                THEN (Target.CharacterLevel - 1) * 3
            ELSE 0
        END + ((Target.CharacterLevel - 1) * @SurvivalBonusPerLevel),
        20 + (Target.CharacterLevel - 1) +
        CASE
            WHEN Target.Intellect > Target.Strength THEN (Target.CharacterLevel - 1) * 3
            WHEN Target.Strength = Target.Intellect AND Target.WeaponTypeID4 IN (10, 11, 14, 15)
                THEN (Target.CharacterLevel - 1) * 3
            ELSE 0
        END + ((Target.CharacterLevel - 1) * @SurvivalBonusPerLevel)
    )
) AS Desired(Strength, Intellect)
CROSS APPLY
(
    VALUES
    (
        200 + ((Desired.Strength - 20) * 20) + ((Target.CharacterLevel - 1) * 20),
        200 + ((Desired.Intellect - 20) * 20) + ((Target.CharacterLevel - 1) * 20)
    )
) AS Vital(HP, MP)
WHERE CharacterRow.Strength <> Desired.Strength
   OR CharacterRow.Intellect <> Desired.Intellect
   OR CharacterRow.HP < Vital.HP
   OR CharacterRow.MP < Vital.MP
   OR CharacterRow.RemainStatPoint <> 0;

DECLARE @StatsRepaired INT = @@ROWCOUNT;

UPDATE Target
SET Strength = CharacterRow.Strength,
    Intellect = CharacterRow.Intellect
FROM #ClientlessCharacters AS Target
INNER JOIN {shardDbName}.dbo._Char AS CharacterRow WITH (NOLOCK)
    ON CharacterRow.CharID = Target.CharID;

-- Older Clientless builds put ammunition in bag slot 15. vSRO requires bow
-- arrows and crossbow bolts in equipment slot 7 before any attack can start.
SELECT Target.CharID, SourceInventory.Slot AS SourceSlot, SourceInventory.ItemID AS SourceItemID
INTO #ClientlessAmmoMoves
FROM #ClientlessCharacters AS Target
INNER JOIN {shardDbName}.dbo._Inventory AS AmmoSlot WITH (UPDLOCK, HOLDLOCK)
    ON AmmoSlot.CharID = Target.CharID AND AmmoSlot.Slot = 7 AND AmmoSlot.ItemID = 0
CROSS APPLY
(
    SELECT TOP (1) InventoryRow.Slot, InventoryRow.ItemID
    FROM {shardDbName}.dbo._Inventory AS InventoryRow WITH (UPDLOCK, HOLDLOCK)
    INNER JOIN {shardDbName}.dbo._Items AS ItemRow WITH (NOLOCK)
        ON ItemRow.ID64 = InventoryRow.ItemID AND ItemRow.Data > 0
    INNER JOIN {shardDbName}.dbo._RefObjCommon AS Ammo WITH (NOLOCK)
        ON Ammo.ID = ItemRow.RefItemID
    WHERE InventoryRow.CharID = Target.CharID
      AND InventoryRow.Slot >= 13
      AND Ammo.Service = 1 AND Ammo.TypeID1 = 3 AND Ammo.TypeID2 = 3 AND Ammo.TypeID3 = 4
      AND Ammo.TypeID4 = CASE WHEN Target.WeaponTypeID4 = 6 THEN 1 ELSE 2 END
    ORDER BY InventoryRow.Slot
) AS SourceInventory
WHERE Target.WeaponTypeID4 IN (6, 12);

UPDATE SourceInventory
SET ItemID = 0
FROM {shardDbName}.dbo._Inventory AS SourceInventory
INNER JOIN #ClientlessAmmoMoves AS MoveRow
    ON MoveRow.CharID = SourceInventory.CharID
   AND MoveRow.SourceSlot = SourceInventory.Slot
   AND MoveRow.SourceItemID = SourceInventory.ItemID;

UPDATE AmmoSlot
SET ItemID = MoveRow.SourceItemID
FROM {shardDbName}.dbo._Inventory AS AmmoSlot
INNER JOIN #ClientlessAmmoMoves AS MoveRow
    ON MoveRow.CharID = AmmoSlot.CharID
WHERE AmmoSlot.Slot = 7 AND AmmoSlot.ItemID = 0;

DECLARE @AmmoRepaired INT = (SELECT COUNT(1) FROM #ClientlessAmmoMoves);
DECLARE @AmmoCharID INT;
DECLARE @AmmoWeaponTypeID4 INT;
DECLARE @AmmoCharacterLevel INT;
DECLARE @AmmoRefItemID INT;
DECLARE @AmmoCount INT;
DECLARE @AmmoResult INT;

DECLARE ClientlessAmmoCursor CURSOR LOCAL FAST_FORWARD FOR
SELECT Target.CharID, Target.WeaponTypeID4, Target.CharacterLevel
FROM #ClientlessCharacters AS Target
INNER JOIN {shardDbName}.dbo._Inventory AS AmmoSlot WITH (UPDLOCK, HOLDLOCK)
    ON AmmoSlot.CharID = Target.CharID AND AmmoSlot.Slot = 7 AND AmmoSlot.ItemID = 0
WHERE Target.WeaponTypeID4 IN (6, 12);

OPEN ClientlessAmmoCursor;
FETCH NEXT FROM ClientlessAmmoCursor INTO @AmmoCharID, @AmmoWeaponTypeID4, @AmmoCharacterLevel;
WHILE @@FETCH_STATUS = 0
BEGIN
    SET @AmmoRefItemID = NULL;
    SET @AmmoCount = NULL;
    SELECT TOP (1)
        @AmmoRefItemID = Common.ID,
        @AmmoCount = CASE WHEN ISNULL(Item.MaxStack, 0) > 0 THEN Item.MaxStack ELSE 1000 END
    FROM {shardDbName}.dbo._RefObjCommon AS Common WITH (NOLOCK)
    INNER JOIN {shardDbName}.dbo._RefObjItem AS Item WITH (NOLOCK) ON Item.ID = Common.Link
    WHERE Common.Service = 1 AND Common.CashItem = 0
      AND Common.TypeID1 = 3 AND Common.TypeID2 = 3 AND Common.TypeID3 = 4
      AND Common.TypeID4 = CASE WHEN @AmmoWeaponTypeID4 = 6 THEN 1 ELSE 2 END
      AND Common.ReqLevel1 <= @AmmoCharacterLevel
      AND Common.CodeName128 NOT LIKE '%EVENT%' AND Common.CodeName128 NOT LIKE '%MALL%'
    ORDER BY Common.ReqLevel1 DESC, Common.ID DESC;

    IF ISNULL(@AmmoRefItemID, 0) > 0
    BEGIN
        EXEC @AmmoResult = {shardDbName}.dbo._FN_ADD_INITIAL_EQUIP
            @CharID = @AmmoCharID, @Slot = 7,
            @RefItemID = @AmmoRefItemID, @Data = @AmmoCount;
        SET @AmmoRepaired += 1;
    END

    FETCH NEXT FROM ClientlessAmmoCursor INTO @AmmoCharID, @AmmoWeaponTypeID4, @AmmoCharacterLevel;
END
CLOSE ClientlessAmmoCursor;
DEALLOCATE ClientlessAmmoCursor;

-- Add a standard speed-drug stack to every managed character that does not
-- already own one. The hunt engine tracks the vSRO B04C response and renews it.
DECLARE @SpeedRepaired INT = 0;
DECLARE @SpeedCharID INT;
DECLARE @SpeedCharacterLevel INT;
DECLARE @SpeedSlot TINYINT;
DECLARE @SpeedRefItemID INT;
DECLARE @SpeedCount INT;
DECLARE @SpeedResult INT;

DECLARE ClientlessSpeedCursor CURSOR LOCAL FAST_FORWARD FOR
SELECT Target.CharID, Target.CharacterLevel
FROM #ClientlessCharacters AS Target
WHERE NOT EXISTS
(
    SELECT 1
    FROM {shardDbName}.dbo._Inventory AS InventoryRow WITH (NOLOCK)
    INNER JOIN {shardDbName}.dbo._Items AS ItemRow WITH (NOLOCK)
        ON ItemRow.ID64 = InventoryRow.ItemID AND ItemRow.Data > 0
    INNER JOIN {shardDbName}.dbo._RefObjCommon AS Common WITH (NOLOCK)
        ON Common.ID = ItemRow.RefItemID
    WHERE InventoryRow.CharID = Target.CharID
      AND Common.Service = 1
      AND Common.TypeID1 = 3 AND Common.TypeID2 = 3 AND Common.TypeID3 = 13 AND Common.TypeID4 = 1
      AND Common.CodeName128 LIKE '%SPEED%'
);

OPEN ClientlessSpeedCursor;
FETCH NEXT FROM ClientlessSpeedCursor INTO @SpeedCharID, @SpeedCharacterLevel;
WHILE @@FETCH_STATUS = 0
BEGIN
    SET @SpeedSlot = NULL;
    SET @SpeedRefItemID = NULL;
    SET @SpeedCount = NULL;

    SELECT TOP (1) @SpeedSlot = InventoryRow.Slot
    FROM {shardDbName}.dbo._Inventory AS InventoryRow WITH (UPDLOCK, HOLDLOCK)
    WHERE InventoryRow.CharID = @SpeedCharID
      AND InventoryRow.Slot >= 15
      AND InventoryRow.ItemID = 0
    ORDER BY InventoryRow.Slot;

    SELECT TOP (1)
        @SpeedRefItemID = Common.ID,
        @SpeedCount = CASE WHEN ISNULL(Item.MaxStack, 0) > 0 THEN Item.MaxStack ELSE 1000 END
    FROM {shardDbName}.dbo._RefObjCommon AS Common WITH (NOLOCK)
    INNER JOIN {shardDbName}.dbo._RefObjItem AS Item WITH (NOLOCK) ON Item.ID = Common.Link
    WHERE Common.Service = 1 AND Common.CashItem = 0
      AND Common.TypeID1 = 3 AND Common.TypeID2 = 3 AND Common.TypeID3 = 13 AND Common.TypeID4 = 1
      AND Common.ReqLevel1 <= @SpeedCharacterLevel
      AND Common.CodeName128 LIKE '%SPEED%'
      AND Common.CodeName128 NOT LIKE '%EVENT%' AND Common.CodeName128 NOT LIKE '%MALL%'
    ORDER BY Common.ReqLevel1 DESC, Common.ID DESC;

    IF @SpeedSlot IS NOT NULL AND ISNULL(@SpeedRefItemID, 0) > 0
    BEGIN
        EXEC @SpeedResult = {shardDbName}.dbo._FN_ADD_INITIAL_EQUIP
            @CharID = @SpeedCharID, @Slot = @SpeedSlot,
            @RefItemID = @SpeedRefItemID, @Data = @SpeedCount;
        IF ISNULL(@SpeedResult, -1) >= 0
            SET @SpeedRepaired += 1;
    END

    FETCH NEXT FROM ClientlessSpeedCursor INTO @SpeedCharID, @SpeedCharacterLevel;
END
CLOSE ClientlessSpeedCursor;
DEALLOCATE ClientlessSpeedCursor;

CREATE TABLE #ClientlessSkillTargets
(
    CharID INT NOT NULL,
    MasteryID INT NOT NULL,
    MasteryLevel INT NOT NULL,
    CharacterLevel INT NOT NULL,
    Strength INT NOT NULL,
    Intellect INT NOT NULL,
    CONSTRAINT PK_ClientlessSkillTargets PRIMARY KEY (CharID, MasteryID)
);

INSERT INTO #ClientlessSkillTargets
    (CharID, MasteryID, MasteryLevel, CharacterLevel, Strength, Intellect)
SELECT Target.CharID,
       Target.PrimaryMasteryID,
       CASE WHEN Target.CharacterLevel < Target.TotalMasteryCap THEN Target.CharacterLevel ELSE Target.TotalMasteryCap END,
       Target.CharacterLevel,
       Target.Strength,
       Target.Intellect
FROM #ClientlessCharacters AS Target;

INSERT INTO #ClientlessSkillTargets
    (CharID, MasteryID, MasteryLevel, CharacterLevel, Strength, Intellect)
SELECT Target.CharID,
       275,
       Allocation.FireLevel,
       Target.CharacterLevel,
       Target.Strength,
       Target.Intellect
FROM #ClientlessCharacters AS Target
CROSS APPLY
(
    SELECT CASE
        WHEN Target.TotalMasteryCap -
             CASE WHEN Target.CharacterLevel < Target.TotalMasteryCap THEN Target.CharacterLevel ELSE Target.TotalMasteryCap END <= 0 THEN 0
        WHEN Target.CharacterLevel < Target.TotalMasteryCap -
             CASE WHEN Target.CharacterLevel < Target.TotalMasteryCap THEN Target.CharacterLevel ELSE Target.TotalMasteryCap END
            THEN Target.CharacterLevel
        ELSE Target.TotalMasteryCap -
             CASE WHEN Target.CharacterLevel < Target.TotalMasteryCap THEN Target.CharacterLevel ELSE Target.TotalMasteryCap END
    END AS FireLevel
) AS Allocation
WHERE Target.IsChinese = 1 AND Allocation.FireLevel > 0;

INSERT INTO #ClientlessSkillTargets
    (CharID, MasteryID, MasteryLevel, CharacterLevel, Strength, Intellect)
SELECT Target.CharID,
       274,
       Allocation.LightningLevel,
       Target.CharacterLevel,
       Target.Strength,
       Target.Intellect
FROM #ClientlessCharacters AS Target
CROSS APPLY
(
    SELECT
        CASE WHEN Target.CharacterLevel < Target.TotalMasteryCap THEN Target.CharacterLevel ELSE Target.TotalMasteryCap END AS PrimaryLevel
) AS PrimaryAllocation
CROSS APPLY
(
    SELECT CASE
        WHEN Target.TotalMasteryCap - PrimaryAllocation.PrimaryLevel <= 0 THEN 0
        WHEN Target.CharacterLevel < Target.TotalMasteryCap - PrimaryAllocation.PrimaryLevel
            THEN Target.CharacterLevel
        ELSE Target.TotalMasteryCap - PrimaryAllocation.PrimaryLevel
    END AS FireLevel
) AS FireAllocation
CROSS APPLY
(
    SELECT CASE
        WHEN Target.TotalMasteryCap - PrimaryAllocation.PrimaryLevel - FireAllocation.FireLevel <= 0 THEN 0
        WHEN Target.CharacterLevel < Target.TotalMasteryCap - PrimaryAllocation.PrimaryLevel - FireAllocation.FireLevel
            THEN Target.CharacterLevel
        ELSE Target.TotalMasteryCap - PrimaryAllocation.PrimaryLevel - FireAllocation.FireLevel
    END AS LightningLevel
) AS Allocation
WHERE Target.IsChinese = 1 AND Allocation.LightningLevel > 0;

{masteryCapGuardSql}

{unusedMasteryResetSql}

UPDATE ExistingMastery
SET Level = Target.MasteryLevel
FROM {shardDbName}.dbo._CharSkillMastery AS ExistingMastery
INNER JOIN #ClientlessSkillTargets AS Target
    ON Target.CharID = ExistingMastery.CharID
   AND Target.MasteryID = ExistingMastery.MasteryID
WHERE ExistingMastery.Level <> Target.MasteryLevel;

INSERT INTO {shardDbName}.dbo._CharSkillMastery (CharID, MasteryID, Level)
SELECT Target.CharID, Target.MasteryID, Target.MasteryLevel
FROM #ClientlessSkillTargets AS Target
WHERE NOT EXISTS
(
    SELECT 1
    FROM {shardDbName}.dbo._CharSkillMastery AS ExistingMastery WITH (UPDLOCK, HOLDLOCK)
    WHERE ExistingMastery.CharID = Target.CharID
      AND ExistingMastery.MasteryID = Target.MasteryID
);

;WITH RankedSkills AS
(
    SELECT
        Target.CharID,
        Skill.ID AS SkillID,
        Skill.GroupID,
        ROW_NUMBER() OVER
        (
            PARTITION BY Target.CharID, Skill.GroupID
            ORDER BY Skill.Basic_Level DESC, Skill.ReqCommon_MasteryLevel1 DESC, Skill.ID DESC
        ) AS RankOrder
    FROM #ClientlessSkillTargets AS Target
    INNER JOIN {shardDbName}.dbo._RefSkill AS Skill WITH (NOLOCK)
        ON Skill.ReqCommon_Mastery1 = Target.MasteryID
    WHERE Skill.Service = 1
      AND Skill.GroupID > 0
      AND Skill.Basic_Level <= Target.CharacterLevel
      AND Skill.ReqCommon_MasteryLevel1 <= Target.MasteryLevel
      AND ISNULL(Skill.ReqCommon_Str, 0) <= Target.Strength
      AND ISNULL(Skill.ReqCommon_Int, 0) <= Target.Intellect
      AND
      (
          ISNULL(Skill.ReqCommon_Mastery2, 0) = 0
          OR EXISTS
          (
              SELECT 1
              FROM #ClientlessSkillTargets AS SecondaryMastery
              WHERE SecondaryMastery.CharID = Target.CharID
                AND SecondaryMastery.MasteryID = Skill.ReqCommon_Mastery2
                AND SecondaryMastery.MasteryLevel >= Skill.ReqCommon_MasteryLevel2
          )
      )
)
SELECT CharID, SkillID, GroupID
INTO #ClientlessDesiredSkills
FROM RankedSkills
WHERE RankOrder = 1;

CREATE UNIQUE CLUSTERED INDEX IX_ClientlessDesiredSkills
    ON #ClientlessDesiredSkills (CharID, GroupID);

{obsoleteSkillDeleteSql}

UPDATE Learned
SET Enable = 1
FROM {shardDbName}.dbo._CharSkill AS Learned
INNER JOIN #ClientlessDesiredSkills AS Desired
    ON Desired.CharID = Learned.CharID
   AND Desired.SkillID = Learned.SkillID
WHERE Learned.Enable <> 1;

INSERT INTO {shardDbName}.dbo._CharSkill (CharID, SkillID, Enable)
SELECT Desired.CharID, Desired.SkillID, 1
FROM #ClientlessDesiredSkills AS Desired
WHERE NOT EXISTS
(
    SELECT 1
    FROM {shardDbName}.dbo._CharSkill AS Learned WITH (UPDLOCK, HOLDLOCK)
    WHERE Learned.CharID = Desired.CharID
      AND Learned.SkillID = Desired.SkillID
);

IF EXISTS
(
    SELECT 1
    FROM #ClientlessCharacters AS ManagedCharacter
    WHERE NOT EXISTS
    (
        SELECT 1
        FROM {shardDbName}.dbo._CharSkill AS Learned WITH (NOLOCK)
        INNER JOIN {shardDbName}.dbo._RefSkill AS AttackSkill WITH (NOLOCK)
            ON AttackSkill.ID = Learned.SkillID AND AttackSkill.Service = 1
        WHERE Learned.CharID = ManagedCharacter.CharID AND Learned.Enable = 1
          AND AttackSkill.ReqCommon_Mastery1 = ManagedCharacter.PrimaryMasteryID
          AND ({attackSkillPredicate})
          AND ({weaponSkillPredicate})
    )
)
    THROW 51044, 'A managed Clientless character has no valid learned attack skill for its equipped weapon.', 1;

DECLARE @PreparedCharacters INT = (SELECT COUNT(1) FROM #ClientlessCharacters);
DECLARE @PreparedSkills INT = (SELECT COUNT(1) FROM #ClientlessDesiredSkills);

COMMIT TRANSACTION;

SELECT @TotalAccounts AS TotalAccounts,
       @PreparedCharacters AS PreparedCharacters,
       @PreparedSkills AS PreparedSkills,
       @StatsRepaired AS StatsRepaired,
       @AmmoRepaired AS AmmoRepaired,
       @SpeedRepaired AS SpeedRepaired;";

        await using var command = new SqlCommand(sql, connection)
        {
            CommandTimeout = 300
        };
        command.Parameters.Add("@AccountDbName", SqlDbType.NVarChar, 128).Value = accountDb;
        command.Parameters.Add("@ShardDbName", SqlDbType.NVarChar, 128).Value = shardDb;
        command.Parameters.Add("@ChineseMasteryCap", SqlDbType.Int).Value = chineseMasteryCap;
        command.Parameters.Add("@EuropeanMasteryCap", SqlDbType.Int).Value = europeanMasteryCap;
        command.Parameters.Add("@SurvivalBonusPerLevel", SqlDbType.Int).Value = ClientlessSurvivalStatBonusPerLevel;
        command.Parameters.Add("@AttackParam", SqlDbType.Int).Value = 6386804;
        command.Parameters.Add("@RequiredItemParam", SqlDbType.Int).Value = 1919250793;

        await using var reader = await command.ExecuteReaderAsync();
        if (!await reader.ReadAsync())
            throw new InvalidOperationException("Clientless skill preparation did not return a result.");

        var totalAccounts = reader.GetInt32(0);
        var preparedCharacters = reader.GetInt32(1);
        var preparedSkills = reader.GetInt32(2);
        var statsRepaired = reader.GetInt32(3);
        var ammoRepaired = reader.GetInt32(4);
        var speedRepaired = reader.GetInt32(5);
        var skipped = Math.Max(0, totalAccounts - preparedCharacters);
        await reader.CloseAsync();

        await AuditAsync(
            connection,
            "PrepareExistingClientlessWeaponSkills",
            "[dbo].[Clientless_Accounts]",
            $"Prepared={preparedCharacters};Skills={preparedSkills};Equipment={equipmentRepaired};Avatars={avatarsEquipped};PetItems={petItemsAdded};Stats={statsRepaired};Ammo={ammoRepaired};Speed={speedRepaired};Skipped={skipped}");

        return $"Prepared {preparedCharacters:N0} existing Clientless character(s) with {preparedSkills:N0} maximum valid weapon/support skill(s), rebuilt and verified equipment for {equipmentRepaired:N0} character(s), equipped varied avatar sets for {avatarsEquipped:N0} character(s), added {petItemsAdded:N0} varied attack/grab pet or pet-supply item(s), repaired {statsRepaired:N0} strong stat allocation(s), equipped ammunition for {ammoRepaired:N0} ranged character(s), and added speed scrolls to {speedRepaired:N0} character(s)." +
               (skipped > 0 ? $" Skipped {skipped:N0} character(s) without verified account ownership or a supported equipped weapon." : string.Empty);
    }

    public async Task<DataTable> SearchPlayersAsync(string term)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        var shardDb = await ReadSettingAsync(connection, "ShardDB", "SRO_VT_SHARD");
        var sql = $@"
IF DB_ID(@ShardDbName) IS NULL
BEGIN
    SELECT TOP 0 CAST(NULL AS INT) AS CharID, CAST(NULL AS NVARCHAR(64)) AS CharName, CAST(NULL AS INT) AS CurLevel, CAST(NULL AS INT) AS MaxLevel, CAST(NULL AS INT) AS LatestRegion;
END
ELSE
BEGIN
    SELECT TOP (100)
        CharID,
        CharName16 AS CharName,
        CurLevel,
        MaxLevel,
        LatestRegion
    FROM {QuoteDb(shardDb)}.dbo._Char WITH (NOLOCK)
    WHERE (@Term = N'' OR CharName16 LIKE N'%' + @Term + N'%' OR CONVERT(NVARCHAR(20), CharID) = @Term)
    ORDER BY CharName16;
END";

        return await QueryTableAsync(connection, sql, new SqlParameter("@Term", term.Trim()), new SqlParameter("@ShardDbName", shardDb));
    }

    public async Task<DataTable> SearchItemsAsync(string term)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        var shardDb = await ReadSettingAsync(connection, "ShardDB", "SRO_VT_SHARD");
        var sql = $@"
IF DB_ID(@ShardDbName) IS NULL
BEGIN
    SELECT TOP 0 CAST(NULL AS INT) AS ID, CAST(NULL AS VARCHAR(128)) AS CodeName128, CAST(NULL AS VARCHAR(128)) AS NameStrID128;
END
ELSE
BEGIN
    SELECT TOP (100)
        ID,
        CodeName128,
        NameStrID128
    FROM {QuoteDb(shardDb)}.dbo._RefObjCommon WITH (NOLOCK)
    WHERE Service = 1
      AND (@Term = N'' OR CodeName128 LIKE N'%' + @Term + N'%' OR NameStrID128 LIKE N'%' + @Term + N'%' OR CONVERT(NVARCHAR(20), ID) = @Term)
    ORDER BY ID DESC;
END";

        return await QueryTableAsync(connection, sql, new SqlParameter("@Term", term.Trim()), new SqlParameter("@ShardDbName", shardDb));
    }

    public async Task<DataTable> LoadItemChestAsync(string charNameOrId)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        var charId = await ResolveCharIdAsync(connection, charNameOrId);

        if (charId <= 0 || !await ObjectExistsAsync(connection, "[dbo].[Item_Chest]", "U"))
            return new DataTable();

        return await QueryTableAsync(connection,
            "SELECT TOP (200) * FROM [dbo].[Item_Chest] WITH (NOLOCK) WHERE CharID = @CharID ORDER BY ID DESC",
            new SqlParameter("@CharID", charId));
    }

    public async Task GrantRewardAsync(string charNameOrId, int itemId, int quantity, int plus, string source)
    {
        if (itemId <= 0)
            throw new InvalidOperationException("Item ID must be greater than zero.");
        if (quantity <= 0)
            throw new InvalidOperationException("Quantity must be greater than zero.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        var charId = await ResolveCharIdAsync(connection, charNameOrId);
        if (charId <= 0)
            throw new InvalidOperationException("Character was not found.");

        await using var command = new SqlCommand("EXEC [dbo].[Item_AddChest] @CharID, @ItemID, @Quantity, @Source, @Plus", connection);
        command.Parameters.Add("@CharID", SqlDbType.Int).Value = charId;
        command.Parameters.Add("@ItemID", SqlDbType.Int).Value = itemId;
        command.Parameters.Add("@Quantity", SqlDbType.Int).Value = quantity;
        command.Parameters.Add("@Source", SqlDbType.VarChar, 100).Value = string.IsNullOrWhiteSpace(source) ? "AdminDesktop" : source.Trim();
        command.Parameters.Add("@Plus", SqlDbType.Int).Value = plus;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "GrantReward", $"CharID={charId};ItemID={itemId}", $"Qty={quantity};Plus={plus};Source={source}");
    }

    public async Task<DataTable> LoadLuckySpinRewardsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        if (!await ObjectExistsAsync(connection, "[dbo].[LuckySpin_Rewards]", "U"))
            return new DataTable();

        return await QueryTableAsync(connection,
            "SELECT TOP (300) ID, ItemID, Amount, Rate, IsActive, CreatedAt, UpdatedAt FROM [dbo].[LuckySpin_Rewards] WITH (NOLOCK) ORDER BY IsActive DESC, ID DESC");
    }

    public async Task AddLuckySpinRewardAsync(int itemId, int amount, int rate, bool isActive)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTableAsync(connection, "[dbo].[LuckySpin_Rewards]");
        await using var command = new SqlCommand(@"
INSERT INTO [dbo].[LuckySpin_Rewards] (ItemID, Amount, Rate, IsActive, UpdatedAt)
VALUES (@ItemID, @Amount, @Rate, @IsActive, SYSUTCDATETIME());", connection);
        command.Parameters.Add("@ItemID", SqlDbType.Int).Value = itemId;
        command.Parameters.Add("@Amount", SqlDbType.Int).Value = amount;
        command.Parameters.Add("@Rate", SqlDbType.Int).Value = rate;
        command.Parameters.Add("@IsActive", SqlDbType.Bit).Value = isActive;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "AddLuckySpinReward", $"ItemID={itemId}", $"Amount={amount};Rate={rate};Active={isActive}");
    }

    public async Task SetLuckySpinRewardActiveAsync(int id, bool isActive)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await using var command = new SqlCommand("UPDATE [dbo].[LuckySpin_Rewards] SET IsActive = @IsActive, UpdatedAt = SYSUTCDATETIME() WHERE ID = @ID", connection);
        command.Parameters.Add("@ID", SqlDbType.Int).Value = id;
        command.Parameters.Add("@IsActive", SqlDbType.Bit).Value = isActive;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "SetLuckySpinRewardActive", $"ID={id}", isActive.ToString());
    }

    public async Task DeleteLuckySpinRewardAsync(int id)
    {
        if (id <= 0)
            throw new InvalidOperationException("A valid Lucky Spin reward ID is required.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTableAsync(connection, "[dbo].[LuckySpin_Rewards]");
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync();

        try
        {
            int itemId;
            int amount;
            int rate;
            bool isActive;
            await using (var command = new SqlCommand(@"
DELETE FROM [dbo].[LuckySpin_Rewards]
OUTPUT DELETED.ItemID, DELETED.Amount, DELETED.Rate, DELETED.IsActive
WHERE ID = @ID;", connection, transaction))
            {
                command.Parameters.Add("@ID", SqlDbType.Int).Value = id;
                await using var reader = await command.ExecuteReaderAsync();
                if (!await reader.ReadAsync())
                    throw new InvalidOperationException($"Lucky Spin reward #{id} was not found.");

                itemId = reader.GetInt32(0);
                amount = reader.GetInt32(1);
                rate = reader.GetInt32(2);
                isActive = reader.GetBoolean(3);
            }

            await AuditAsync(connection, "DeleteLuckySpinReward", $"ID={id};ItemID={itemId}",
                $"Amount={amount};Rate={rate};Active={isActive}", transaction);
            await transaction.CommitAsync();
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }
    }

    public async Task<DataTable> LoadSpecialOffersAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        if (!await ObjectExistsAsync(connection, "[dbo].[Offer_List]", "U"))
            return new DataTable();

        return await QueryTableAsync(connection,
            "SELECT TOP (300) ID, Service, SortOrder, Title, ItemID, ItemCount, MainPrice, SalePrice, PaymentType, PreviewImagePath, StartDate, EndDate FROM [dbo].[Offer_List] WITH (NOLOCK) ORDER BY Service DESC, SortOrder, ID DESC");
    }

    public async Task AddSpecialOfferAsync(string title, int itemId, int itemCount, int mainPrice, int salePrice, int paymentType, int sortOrder, bool service)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTableAsync(connection, "[dbo].[Offer_List]");
        await using var command = new SqlCommand(@"
INSERT INTO [dbo].[Offer_List]
    (Service, SortOrder, Title, ItemID, ItemCount, MainPrice, SalePrice, PaymentType, UpdatedAt)
VALUES
    (@Service, @SortOrder, @Title, @ItemID, @ItemCount, @MainPrice, @SalePrice, @PaymentType, SYSUTCDATETIME());", connection);
        command.Parameters.Add("@Service", SqlDbType.Bit).Value = service;
        command.Parameters.Add("@SortOrder", SqlDbType.Int).Value = sortOrder;
        command.Parameters.Add("@Title", SqlDbType.NVarChar, 128).Value = title.Trim();
        command.Parameters.Add("@ItemID", SqlDbType.Int).Value = itemId;
        command.Parameters.Add("@ItemCount", SqlDbType.Int).Value = itemCount;
        command.Parameters.Add("@MainPrice", SqlDbType.Int).Value = mainPrice;
        command.Parameters.Add("@SalePrice", SqlDbType.Int).Value = salePrice;
        command.Parameters.Add("@PaymentType", SqlDbType.TinyInt).Value = paymentType;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "AddSpecialOffer", title, $"ItemID={itemId};Count={itemCount};Sale={salePrice};Active={service}");
    }

    public async Task SetSpecialOfferActiveAsync(int id, bool service)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await using var command = new SqlCommand("UPDATE [dbo].[Offer_List] SET Service = @Service, UpdatedAt = SYSUTCDATETIME() WHERE ID = @ID", connection);
        command.Parameters.Add("@ID", SqlDbType.Int).Value = id;
        command.Parameters.Add("@Service", SqlDbType.Bit).Value = service;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "SetSpecialOfferActive", $"ID={id}", service.ToString());
    }

    public async Task<DataTable> LoadKillerAnimationsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        if (!await ObjectExistsAsync(connection, "[dbo].[KillerAnimation_List]", "U"))
            return new DataTable();

        return await QueryTableAsync(connection, @"
SELECT TOP (300)
    ID,
    Service,
    SortOrder,
    CodeName,
    DisplayName,
    AnimationID,
    Price,
    PaymentType,
    CASE PaymentType WHEN 1 THEN N'Gold' ELSE N'Silk' END AS PaymentName,
    CreatedAt,
    UpdatedAt
FROM [dbo].[KillerAnimation_List] WITH (NOLOCK)
ORDER BY Service DESC, SortOrder, ID;");
    }

    public async Task SaveKillerAnimationAsync(int? id, bool service, int sortOrder, string codeName, string displayName, int animationId, int price, int paymentType)
    {
        if (string.IsNullOrWhiteSpace(codeName))
            throw new InvalidOperationException("Code name is required.");
        if (string.IsNullOrWhiteSpace(displayName))
            throw new InvalidOperationException("Display name is required.");
        if (animationId <= 0 || animationId > 500)
            throw new InvalidOperationException("Animation ID must be between 1 and 500.");
        if (price < 0)
            throw new InvalidOperationException("Price cannot be negative.");
        if (paymentType is < 0 or > 1)
            throw new InvalidOperationException("Payment type must be Silk or Gold.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTableAsync(connection, "[dbo].[KillerAnimation_List]");

        if (id.HasValue && id.Value > 0)
        {
            await using var command = new SqlCommand(@"
UPDATE [dbo].[KillerAnimation_List]
SET Service = @Service,
    SortOrder = @SortOrder,
    CodeName = @CodeName,
    DisplayName = @DisplayName,
    AnimationID = @AnimationID,
    Price = @Price,
    PaymentType = @PaymentType,
    UpdatedAt = SYSUTCDATETIME()
WHERE ID = @ID;", connection);
            command.Parameters.Add("@ID", SqlDbType.Int).Value = id.Value;
            command.Parameters.Add("@Service", SqlDbType.Bit).Value = service;
            command.Parameters.Add("@SortOrder", SqlDbType.Int).Value = sortOrder;
            command.Parameters.Add("@CodeName", SqlDbType.VarChar, 64).Value = codeName.Trim();
            command.Parameters.Add("@DisplayName", SqlDbType.NVarChar, 96).Value = displayName.Trim();
            command.Parameters.Add("@AnimationID", SqlDbType.Int).Value = animationId;
            command.Parameters.Add("@Price", SqlDbType.Int).Value = price;
            command.Parameters.Add("@PaymentType", SqlDbType.TinyInt).Value = paymentType;

            var affected = await command.ExecuteNonQueryAsync();
            if (affected == 0)
                throw new InvalidOperationException($"Killer animation #{id.Value} was not found.");

            await AuditAsync(connection, "UpdateKillerAnimation", $"ID={id.Value}", $"{codeName};Anim={animationId};Price={price};Payment={paymentType};Active={service}");
            return;
        }

        await using var insert = new SqlCommand(@"
INSERT INTO [dbo].[KillerAnimation_List]
    (Service, SortOrder, CodeName, DisplayName, AnimationID, Price, PaymentType, UpdatedAt)
VALUES
    (@Service, @SortOrder, @CodeName, @DisplayName, @AnimationID, @Price, @PaymentType, SYSUTCDATETIME());", connection);
        insert.Parameters.Add("@Service", SqlDbType.Bit).Value = service;
        insert.Parameters.Add("@SortOrder", SqlDbType.Int).Value = sortOrder;
        insert.Parameters.Add("@CodeName", SqlDbType.VarChar, 64).Value = codeName.Trim();
        insert.Parameters.Add("@DisplayName", SqlDbType.NVarChar, 96).Value = displayName.Trim();
        insert.Parameters.Add("@AnimationID", SqlDbType.Int).Value = animationId;
        insert.Parameters.Add("@Price", SqlDbType.Int).Value = price;
        insert.Parameters.Add("@PaymentType", SqlDbType.TinyInt).Value = paymentType;
        await insert.ExecuteNonQueryAsync();
        await AuditAsync(connection, "AddKillerAnimation", codeName, $"Anim={animationId};Price={price};Payment={paymentType};Active={service}");
    }

    public async Task SetKillerAnimationActiveAsync(int id, bool service)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await using var command = new SqlCommand("UPDATE [dbo].[KillerAnimation_List] SET Service = @Service, UpdatedAt = SYSUTCDATETIME() WHERE ID = @ID", connection);
        command.Parameters.Add("@ID", SqlDbType.Int).Value = id;
        command.Parameters.Add("@Service", SqlDbType.Bit).Value = service;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "SetKillerAnimationActive", $"ID={id}", service.ToString());
    }

    public async Task<IReadOnlyList<VipTierEntry>> LoadVipTiersAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTableAsync(connection, "[dbo].[Vip_Tiers]");

        const string sql = @"
SELECT
    tier.RankCode,
    tier.DisplayName,
    tier.MinSilk,
    tier.IconID,
    ISNULL(iconFile.MediaPath, '') AS IconPath,
    ISNULL(tier.BuffSkillCode, '') AS BuffSkillCode,
    COUNT(rankRow.CharID) AS AssignedPlayers
FROM dbo.Vip_Tiers AS tier WITH (NOLOCK)
LEFT JOIN dbo.Icons AS iconFile WITH (NOLOCK)
    ON iconFile.IconID = tier.IconID
LEFT JOIN dbo.Rank_Silk AS rankRow WITH (NOLOCK)
    ON rankRow.SilkRank = tier.RankCode
GROUP BY
    tier.RankCode,
    tier.DisplayName,
    tier.MinSilk,
    tier.IconID,
    iconFile.MediaPath,
    tier.BuffSkillCode
ORDER BY tier.MinSilk, tier.RankCode DESC;";

        await using var command = new SqlCommand(sql, connection);
        await using var reader = await command.ExecuteReaderAsync();
        var tiers = new List<VipTierEntry>();
        while (await reader.ReadAsync())
        {
            tiers.Add(new VipTierEntry
            {
                RankCode = reader.GetInt32(0),
                DisplayName = reader.GetString(1),
                MinSilk = reader.GetInt32(2),
                IconID = reader.GetInt32(3),
                IconPath = reader.GetString(4),
                BuffSkillCode = reader.GetString(5),
                AssignedPlayers = reader.GetInt32(6)
            });
        }

        return tiers;
    }

    public async Task<bool> LoadVipSystemEnabledAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        var value = await ReadSettingAsync(connection, "VipSystemEnabled", "True");
        return !value.Equals("0", StringComparison.OrdinalIgnoreCase) &&
               !value.Equals("false", StringComparison.OrdinalIgnoreCase) &&
               !value.Equals("off", StringComparison.OrdinalIgnoreCase) &&
               !value.Equals("no", StringComparison.OrdinalIgnoreCase);
    }

    public async Task SetVipSystemEnabledAsync(bool enabled)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        const string sql = @"
IF EXISTS (SELECT 1 FROM [dbo].[System_Settings] WHERE SettingName = 'VipSystemEnabled')
    UPDATE [dbo].[System_Settings]
    SET Value = @Value
    WHERE SettingName = 'VipSystemEnabled';
ELSE
    INSERT INTO [dbo].[System_Settings] (SettingName, Value)
    VALUES ('VipSystemEnabled', @Value);";
        await using var command = new SqlCommand(sql, connection);
        command.Parameters.Add("@Value", SqlDbType.NVarChar, 512).Value = enabled ? "True" : "False";
        await command.ExecuteNonQueryAsync();

        await AuditAsync(connection, "SetVipSystemEnabled", "VipSystemEnabled", enabled ? "True" : "False");
    }

    public async Task<DataTable> LoadVipPlayerRankingsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        if (!await ObjectExistsAsync(connection, "[dbo].[Vip_CurrentRankings]", "V"))
        {
            throw new InvalidOperationException(
                "[dbo].[Vip_CurrentRankings] was not found. Apply the v2.7.5 VIP database update first.");
        }

        return await QueryTableAsync(connection, @"
SELECT TOP (500)
    Position AS [POSITION],
    ISNULL(CharName16, '(missing character)') AS [PLAYER],
    TotalSilkSpent AS [TOTAL SILK SPENT],
    ISNULL(RankName, 'Unranked') AS [VIP RANK],
    IconID AS [ICON ID],
    LastSilkSpent AS [LAST SPEND],
    LastSpendAt AS [LAST SPEND TIME],
    RankUpdatedAt AS [RANK UPDATED]
FROM dbo.Vip_CurrentRankings WITH (NOLOCK)
ORDER BY Position;");
    }

    public async Task SaveVipTiersAsync(IReadOnlyList<VipTierEntry> tiers)
    {
        var rows = tiers
            .Select(tier => new VipTierEntry
            {
                RankCode = tier.RankCode,
                DisplayName = tier.DisplayName.Trim(),
                MinSilk = tier.MinSilk,
                IconID = tier.IconID,
                IconPath = tier.IconPath,
                BuffSkillCode = tier.BuffSkillCode.Trim(),
                AssignedPlayers = tier.AssignedPlayers
            })
            .ToArray();

        if (rows.Length != 6 || rows.Select(row => row.RankCode).Distinct().Count() != 6 ||
            rows.Any(row => row.RankCode is < 1 or > 6))
        {
            throw new InvalidOperationException("VIP configuration must contain exactly the six ranks from 1 through 6.");
        }

        if (rows.Any(row => row.MinSilk is < 0 or > 2_000_000_000))
            throw new InvalidOperationException("Required Silk must be between 0 and 2,000,000,000.");

        if (rows.Select(row => row.MinSilk).Distinct().Count() != rows.Length)
            throw new InvalidOperationException("Every VIP icon must have a unique Required Silk value.");

        if (rows.Select(row => row.IconID).Distinct().Count() != rows.Length ||
            rows.Any(row => row.IconID <= 0))
        {
            throw new InvalidOperationException("Every VIP rank must reference a unique valid icon.");
        }

        if (rows.Any(row => string.IsNullOrWhiteSpace(row.DisplayName) || row.DisplayName.Length > 32))
            throw new InvalidOperationException("Every VIP rank must have a display name of 32 characters or fewer.");

        if (rows.Any(row => row.BuffSkillCode.Length > 128))
            throw new InvalidOperationException("Buff skill code names cannot exceed 128 characters.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTableAsync(connection, "[dbo].[Vip_Tiers]");

        var shardDb = await ReadSettingAsync(connection, "ShardDB", "SRO_VT_SHARD");
        var vipSystemEnabled = await ReadSettingAsync(connection, "VipSystemEnabled", "True");
        var shouldRecalculate = !vipSystemEnabled.Equals("0", StringComparison.OrdinalIgnoreCase) &&
                                !vipSystemEnabled.Equals("false", StringComparison.OrdinalIgnoreCase) &&
                                !vipSystemEnabled.Equals("off", StringComparison.OrdinalIgnoreCase) &&
                                !vipSystemEnabled.Equals("no", StringComparison.OrdinalIgnoreCase);
        var quotedShardDb = QuoteDb(shardDb);
        await using var transaction =
            (SqlTransaction)await connection.BeginTransactionAsync(IsolationLevel.Serializable);

        try
        {
            // Move thresholds to a collision-free range first so administrators
            // can swap two tier thresholds without tripping the unique index
            // halfway through the transaction.
            await using (var stageThresholds = new SqlCommand(@"
UPDATE dbo.Vip_Tiers
SET MinSilk = 2147483000 + RankCode;",
                             connection,
                             transaction))
            {
                await stageThresholds.ExecuteNonQueryAsync();
            }

            foreach (var row in rows)
            {
                await using (var iconCommand = new SqlCommand(
                                 "SELECT COUNT(*) FROM dbo.Icons WHERE IconID = @IconID",
                                 connection,
                                 transaction))
                {
                    iconCommand.Parameters.Add("@IconID", SqlDbType.Int).Value = row.IconID;
                    if (Convert.ToInt32(await iconCommand.ExecuteScalarAsync()) != 1)
                        throw new InvalidOperationException($"Icon ID {row.IconID} does not exist in Icons.");
                }

                if (!string.IsNullOrWhiteSpace(row.BuffSkillCode))
                {
                    await using var skillCommand = new SqlCommand($@"
SELECT COUNT(*)
FROM {quotedShardDb}.dbo._RefSkill WITH (NOLOCK)
WHERE Service = 1 AND Basic_Code = @SkillCode;",
                        connection,
                        transaction);
                    skillCommand.Parameters.Add("@SkillCode", SqlDbType.VarChar, 128).Value = row.BuffSkillCode;
                    if (Convert.ToInt32(await skillCommand.ExecuteScalarAsync()) == 0)
                    {
                        throw new InvalidOperationException(
                            $"Buff skill '{row.BuffSkillCode}' was not found as an active skill in {shardDb}.dbo._RefSkill.");
                    }
                }

                await using var updateCommand = new SqlCommand(@"
UPDATE dbo.Vip_Tiers
SET DisplayName = @DisplayName,
    MinSilk = @MinSilk,
    IconID = @IconID,
    BuffSkillCode = NULLIF(@BuffSkillCode, ''),
    UpdatedAt = SYSUTCDATETIME()
WHERE RankCode = @RankCode;",
                    connection,
                    transaction);
                updateCommand.Parameters.Add("@DisplayName", SqlDbType.NVarChar, 32).Value = row.DisplayName;
                updateCommand.Parameters.Add("@MinSilk", SqlDbType.Int).Value = row.MinSilk;
                updateCommand.Parameters.Add("@IconID", SqlDbType.Int).Value = row.IconID;
                updateCommand.Parameters.Add("@BuffSkillCode", SqlDbType.VarChar, 128).Value = row.BuffSkillCode;
                updateCommand.Parameters.Add("@RankCode", SqlDbType.Int).Value = row.RankCode;

                if (await updateCommand.ExecuteNonQueryAsync() != 1)
                    throw new InvalidOperationException($"VIP rank {row.RankCode} was not found.");
            }

            await using (var recalculateCommand =
                         new SqlCommand("EXEC dbo.Vip_RecalculateAll", connection, transaction))
            {
                if (shouldRecalculate)
                {
                    recalculateCommand.CommandTimeout = 120;
                    await recalculateCommand.ExecuteNonQueryAsync();
                }
            }

            await transaction.CommitAsync();
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }

        await AuditAsync(
            connection,
            "UpdateVipTiers",
            "dbo.Vip_Tiers",
            string.Join("; ", rows.OrderBy(row => row.MinSilk)
                .Select(row =>
                    $"{row.DisplayName}={row.MinSilk},Icon={row.IconID},Buff={(string.IsNullOrWhiteSpace(row.BuffSkillCode) ? "None" : row.BuffSkillCode)}")));
    }

    public async Task<DataTable> LoadBlockedWordsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        if (!await ObjectExistsAsync(connection, "[dbo].[Security_BlockedWords]", "U"))
            return new DataTable();

        return await QueryTableAsync(connection,
            "SELECT TOP (300) ID, Word, MatchMode, IsActive, CreatedAt, UpdatedAt FROM [dbo].[Security_BlockedWords] WITH (NOLOCK) ORDER BY IsActive DESC, Word");
    }

    public async Task<DataTable> LoadUniqueRulesAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        if (!await ObjectExistsAsync(connection, "[dbo].[Security_AttackRules]", "U"))
            return new DataTable();

        return await QueryTableAsync(connection, @"
SELECT TOP (500)
    MobRefObjID, OnlyOffJob, OnlyOnJob, OnlyByThief, OnlyByTrader,
    OnlyStrPlayer, OnlyIntPlayer, ISNULL(AllowedJobMask, 0) AS AllowedJobMask,
    ISNULL(AllowedCapeMask, 0) AS AllowedCapeMask,
    ISNULL(AllowedRaceMask, 0) AS AllowedRaceMask,
    ISNULL(RequireParty, -1) AS RequireParty,
    ISNULL(RequireGuild, -1) AS RequireGuild
FROM [dbo].[Security_AttackRules] WITH (NOLOCK)
ORDER BY MobRefObjID;");
    }

    public async Task SaveUniqueRuleAsync(int mobRefObjId, bool onlyOffJob, bool onlyOnJob,
        bool onlyByThief, bool onlyByTrader, bool onlyStrPlayer, bool onlyIntPlayer,
        int allowedJobMask, int allowedCapeMask, int allowedRaceMask, int requireParty, int requireGuild)
    {
        if (mobRefObjId <= 0)
            throw new InvalidOperationException("Unique ID must be greater than zero.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        if (!await ObjectExistsAsync(connection, "[dbo].[Security_AttackRules]", "U"))
            throw new InvalidOperationException("[dbo].[Security_AttackRules] was not found in the selected database.");

        await using var command = new SqlCommand(@"
MERGE [dbo].[Security_AttackRules] AS target
USING (SELECT @MobRefObjID AS MobRefObjID) AS source
ON target.MobRefObjID = source.MobRefObjID
WHEN MATCHED THEN UPDATE SET
    OnlyOffJob = @OnlyOffJob, OnlyOnJob = @OnlyOnJob,
    OnlyByThief = @OnlyByThief, OnlyByTrader = @OnlyByTrader,
    OnlyStrPlayer = @OnlyStrPlayer, OnlyIntPlayer = @OnlyIntPlayer,
    AllowedJobMask = @AllowedJobMask, AllowedCapeMask = @AllowedCapeMask,
    AllowedRaceMask = @AllowedRaceMask, RequireParty = @RequireParty,
    RequireGuild = @RequireGuild
WHEN NOT MATCHED THEN INSERT
    (MobRefObjID, OnlyOffJob, OnlyOnJob, OnlyByThief, OnlyByTrader,
     OnlyStrPlayer, OnlyIntPlayer, AllowedJobMask, AllowedCapeMask,
     AllowedRaceMask, RequireParty, RequireGuild)
VALUES
    (@MobRefObjID, @OnlyOffJob, @OnlyOnJob, @OnlyByThief, @OnlyByTrader,
     @OnlyStrPlayer, @OnlyIntPlayer, @AllowedJobMask, @AllowedCapeMask,
     @AllowedRaceMask, @RequireParty, @RequireGuild);", connection);
        command.Parameters.Add("@MobRefObjID", SqlDbType.Int).Value = mobRefObjId;
        command.Parameters.Add("@OnlyOffJob", SqlDbType.Bit).Value = onlyOffJob;
        command.Parameters.Add("@OnlyOnJob", SqlDbType.Bit).Value = onlyOnJob;
        command.Parameters.Add("@OnlyByThief", SqlDbType.Bit).Value = onlyByThief;
        command.Parameters.Add("@OnlyByTrader", SqlDbType.Bit).Value = onlyByTrader;
        command.Parameters.Add("@OnlyStrPlayer", SqlDbType.Bit).Value = onlyStrPlayer;
        command.Parameters.Add("@OnlyIntPlayer", SqlDbType.Bit).Value = onlyIntPlayer;
        command.Parameters.Add("@AllowedJobMask", SqlDbType.TinyInt).Value = Math.Clamp(allowedJobMask, 0, 255);
        command.Parameters.Add("@AllowedCapeMask", SqlDbType.TinyInt).Value = Math.Clamp(allowedCapeMask, 0, 255);
        command.Parameters.Add("@AllowedRaceMask", SqlDbType.TinyInt).Value = Math.Clamp(allowedRaceMask, 0, 255);
        command.Parameters.Add("@RequireParty", SqlDbType.Int).Value = requireParty;
        command.Parameters.Add("@RequireGuild", SqlDbType.Int).Value = requireGuild;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "SaveUniqueRule", $"MobRefObjID={mobRefObjId}", "Unique attack rule saved");
    }

    public async Task DeleteUniqueRuleAsync(int mobRefObjId)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await using var command = new SqlCommand("DELETE FROM [dbo].[Security_AttackRules] WHERE MobRefObjID = @MobRefObjID", connection);
        command.Parameters.Add("@MobRefObjID", SqlDbType.Int).Value = mobRefObjId;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "DeleteUniqueRule", $"MobRefObjID={mobRefObjId}", "Unique attack rule deleted");
    }

    public async Task AddBlockedWordAsync(string word, int matchMode, bool isActive)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTableAsync(connection, "[dbo].[Security_BlockedWords]");
        await using var command = new SqlCommand(@"
INSERT INTO [dbo].[Security_BlockedWords] (Word, MatchMode, IsActive, UpdatedAt)
VALUES (@Word, @MatchMode, @IsActive, SYSDATETIME());", connection);
        command.Parameters.Add("@Word", SqlDbType.NVarChar, 128).Value = word.Trim();
        command.Parameters.Add("@MatchMode", SqlDbType.TinyInt).Value = matchMode;
        command.Parameters.Add("@IsActive", SqlDbType.Bit).Value = isActive;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "AddBlockedWord", word, $"MatchMode={matchMode};Active={isActive}");
    }

    public async Task SetBlockedWordActiveAsync(int id, bool isActive)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await using var command = new SqlCommand("UPDATE [dbo].[Security_BlockedWords] SET IsActive = @IsActive, UpdatedAt = SYSDATETIME() WHERE ID = @ID", connection);
        command.Parameters.Add("@ID", SqlDbType.Int).Value = id;
        command.Parameters.Add("@IsActive", SqlDbType.Bit).Value = isActive;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "SetBlockedWordActive", $"ID={id}", isActive.ToString());
    }

    public async Task<DataTable> LoadRegionControlAsync(string term)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureFilterRegionControlTableAsync(connection);
        const string sql = @"
SELECT TOP (500)
    *,
    CASE BuildMode WHEN 0 THEN N'Any' WHEN 1 THEN N'STR only' WHEN 2 THEN N'INT only' ELSE N'Hybrid only' END AS BuildDisplay,
    CASE JobMode WHEN 0 THEN N'Any' WHEN 1 THEN N'Jobless only' WHEN 2 THEN N'Any job suit' WHEN 3 THEN N'Trader only' WHEN 4 THEN N'Thief only' ELSE N'Hunter only' END AS JobDisplay,
    CASE RaceMode WHEN 0 THEN N'Any' WHEN 1 THEN N'Chinese only' ELSE N'European only' END AS RaceDisplay,
    CASE PartyMode WHEN 0 THEN N'Any' WHEN 1 THEN N'Party required' ELSE N'Solo only' END AS PartyDisplay
FROM [dbo].[Security_RegionFeatures] WITH (NOLOCK)
WHERE @Term = N''
   OR CONVERT(NVARCHAR(32), WorldID) = @Term
   OR CONVERT(NVARCHAR(32), RegionID) = @Term
   OR RuleName LIKE N'%' + @Term + N'%'
   OR ManagedEventCode LIKE N'%' + @Term + N'%'
ORDER BY Enabled DESC, WorldID, RegionID;";

        return await QueryTableAsync(connection, sql, new SqlParameter("@Term", term.Trim()));
    }

    public async Task SaveRegionControlAsync(
        int worldId,
        int regionId,
        string ruleName,
        bool enabled,
        int buildMode,
        int jobMode,
        int raceMode,
        int partyMode,
        int minLevel,
        int maxLevel,
        bool allowTeleport,
        bool allowReverse,
        bool allowTrace,
        bool allowMovement,
        bool allowChat,
        bool allowGlobalChat,
        bool allowParty,
        bool allowExchange,
        bool allowStall,
        bool allowPvp,
        bool allowAlchemy,
        bool allowSpecialItems,
        bool allowBerserk,
        int autoPvpCape,
        int inactivityReturnSeconds,
        int eventSuitMode)
    {
        if (worldId < 0)
            throw new InvalidOperationException("World ID cannot be negative. Use 0 for all worlds.");
        regionId = NormalizeRegionId(regionId);
        if (!IsValidRegionId(regionId))
            throw new InvalidOperationException("Region ID must be a non-zero signed 16-bit value (-32768 to 32767).");
        if (maxLevel > 0 && minLevel > 0 && maxLevel < minLevel)
            throw new InvalidOperationException("Maximum level cannot be lower than minimum level.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureFilterRegionControlTableAsync(connection);

        await using var command = new SqlCommand(@"
MERGE [dbo].[Security_RegionFeatures] AS target
USING (SELECT @WorldID AS WorldID, @RegionID AS RegionID) AS source
ON target.WorldID = source.WorldID AND target.RegionID = source.RegionID
WHEN MATCHED THEN
    UPDATE SET
        RuleName=@RuleName, Enabled=@Enabled, BuildMode=@BuildMode, JobMode=@JobMode,
        RaceMode=@RaceMode, PartyMode=@PartyMode, MinLevel=@MinLevel, MaxLevel=@MaxLevel,
        AllowTeleport=@AllowTeleport, AllowReverse=@AllowReverse, AllowTrace=@AllowTrace,
        AllowMovement=@AllowMovement, AllowChat=@AllowChat, AllowGlobalChat=@AllowGlobalChat,
        AllowParty=@AllowParty, AllowExchange=@AllowExchange, AllowStall=@AllowStall,
        AllowPvP=@AllowPvP, AllowAlchemy=@AllowAlchemy, AllowSpecialItems=@AllowSpecialItems,
        AllowBerserk=@AllowBerserk, AutoPvpCape=@AutoPvpCape,
        InactivityReturnSeconds=@InactivityReturnSeconds, EventSuitMode=@EventSuitMode,
        UpdatedAt = SYSUTCDATETIME()
WHEN NOT MATCHED THEN
    INSERT
    (
        WorldID,RegionID,RuleName,Enabled,BuildMode,JobMode,RaceMode,PartyMode,MinLevel,MaxLevel,
        AllowTeleport,AllowReverse,AllowTrace,AllowMovement,AllowChat,AllowGlobalChat,AllowParty,
        AllowExchange,AllowStall,AllowPvP,AllowAlchemy,AllowSpecialItems,AllowBerserk,AutoPvpCape,
        InactivityReturnSeconds,EventSuitMode
    )
    VALUES
    (
        @WorldID,@RegionID,@RuleName,@Enabled,@BuildMode,@JobMode,@RaceMode,@PartyMode,@MinLevel,@MaxLevel,
        @AllowTeleport,@AllowReverse,@AllowTrace,@AllowMovement,@AllowChat,@AllowGlobalChat,@AllowParty,
        @AllowExchange,@AllowStall,@AllowPvP,@AllowAlchemy,@AllowSpecialItems,@AllowBerserk,@AutoPvpCape,
        @InactivityReturnSeconds,@EventSuitMode
    );", connection);

        command.Parameters.Add("@WorldID", SqlDbType.Int).Value = worldId;
        command.Parameters.Add("@RegionID", SqlDbType.Int).Value = regionId;
        command.Parameters.Add("@RuleName", SqlDbType.NVarChar, 64).Value = string.IsNullOrWhiteSpace(ruleName) ? DBNull.Value : ruleName.Trim();
        command.Parameters.Add("@Enabled", SqlDbType.Bit).Value = enabled;
        command.Parameters.Add("@BuildMode", SqlDbType.TinyInt).Value = Math.Clamp(buildMode, 0, 3);
        command.Parameters.Add("@JobMode", SqlDbType.TinyInt).Value = Math.Clamp(jobMode, 0, 5);
        command.Parameters.Add("@RaceMode", SqlDbType.TinyInt).Value = Math.Clamp(raceMode, 0, 2);
        command.Parameters.Add("@PartyMode", SqlDbType.TinyInt).Value = Math.Clamp(partyMode, 0, 2);
        command.Parameters.Add("@MinLevel", SqlDbType.TinyInt).Value = Math.Clamp(minLevel, 0, 255);
        command.Parameters.Add("@MaxLevel", SqlDbType.TinyInt).Value = Math.Clamp(maxLevel, 0, 255);
        command.Parameters.Add("@AllowTeleport", SqlDbType.Bit).Value = allowTeleport;
        command.Parameters.Add("@AllowReverse", SqlDbType.Bit).Value = allowReverse;
        command.Parameters.Add("@AllowTrace", SqlDbType.Bit).Value = allowTrace;
        command.Parameters.Add("@AllowMovement", SqlDbType.Bit).Value = allowMovement;
        command.Parameters.Add("@AllowChat", SqlDbType.Bit).Value = allowChat;
        command.Parameters.Add("@AllowGlobalChat", SqlDbType.Bit).Value = allowGlobalChat;
        command.Parameters.Add("@AllowParty", SqlDbType.Bit).Value = allowParty;
        command.Parameters.Add("@AllowExchange", SqlDbType.Bit).Value = allowExchange;
        command.Parameters.Add("@AllowStall", SqlDbType.Bit).Value = allowStall;
        command.Parameters.Add("@AllowPvP", SqlDbType.Bit).Value = allowPvp;
        command.Parameters.Add("@AllowAlchemy", SqlDbType.Bit).Value = allowAlchemy;
        command.Parameters.Add("@AllowSpecialItems", SqlDbType.Bit).Value = allowSpecialItems;
        command.Parameters.Add("@AllowBerserk", SqlDbType.Bit).Value = allowBerserk;
        command.Parameters.Add("@AutoPvpCape", SqlDbType.TinyInt).Value = Math.Clamp(autoPvpCape, 0, 5);
        command.Parameters.Add("@InactivityReturnSeconds", SqlDbType.Int).Value = Math.Clamp(inactivityReturnSeconds, 0, 86400);
        command.Parameters.Add("@EventSuitMode", SqlDbType.TinyInt).Value = Math.Clamp(eventSuitMode, 0, 2);

        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "SaveRegionControl", $"WorldID={worldId};RegionID={regionId}", $"Enabled={enabled};BuildMode={buildMode};JobMode={jobMode}");
    }

    public async Task DeleteRegionControlAsync(int worldId, int regionId)
    {
        if (worldId < 0)
            throw new InvalidOperationException("World ID cannot be negative.");
        regionId = NormalizeRegionId(regionId);
        if (!IsValidRegionId(regionId))
            throw new InvalidOperationException("Region ID must be a non-zero signed 16-bit value.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        if (await ObjectExistsAsync(connection, "[dbo].[Security_RegionFeatures]", "U"))
        {
            await using var command = new SqlCommand(
                "DELETE FROM [dbo].[Security_RegionFeatures] WHERE WorldID = @WorldID AND RegionID = @RegionID",
                connection);
            command.Parameters.Add("@WorldID", SqlDbType.Int).Value = worldId;
            command.Parameters.Add("@RegionID", SqlDbType.Int).Value = regionId;
            await command.ExecuteNonQueryAsync();
        }

        await AuditAsync(connection, "DeleteRegionControl", $"WorldID={worldId};RegionID={regionId}", "Deleted region control rule");
    }

    public async Task<DataTable> SearchHwidAsync(string term)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        if (!await ObjectExistsAsync(connection, "[dbo].[Auth_HWIDs]", "U"))
            return new DataTable();

        return await QueryTableAsync(connection, @"
SELECT TOP (200) *
FROM [dbo].[Auth_HWIDs] WITH (NOLOCK)
WHERE @Term = N''
   OR CONVERT(NVARCHAR(64), CharID) = @Term
   OR CONVERT(NVARCHAR(128), Hwid) LIKE N'%' + @Term + N'%'
ORDER BY Active DESC, CharID DESC;", new SqlParameter("@Term", term.Trim()));
    }

    public async Task DeactivateHwidByCharIdAsync(int charId)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await using var command = new SqlCommand("UPDATE [dbo].[Auth_HWIDs] SET Active = 0 WHERE CharID = @CharID", connection);
        command.Parameters.Add("@CharID", SqlDbType.Int).Value = charId;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "DeactivateHwid", $"CharID={charId}", "Active=0");
    }

    public async Task<DataTable> LoadAuditLogAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureAuditTableAsync(connection);
        return await QueryTableAsync(connection, @"
SELECT TOP (500) ID, CreatedAt, AdminName, Action, Target, Details, MachineName
FROM [dbo].[Admin_AuditLog] WITH (NOLOCK)
ORDER BY ID DESC;");
    }

    public async Task<DataTable> SearchChatLogAsync(string term)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        if (!await ObjectExistsAsync(connection, "[dbo].[Log_Chat]", "U"))
            return new DataTable();

        return await QueryTableAsync(connection, @"
SELECT TOP (500) ID, [Timestamp], Sender, Receiver, ChatType, [Message]
FROM [dbo].[Log_Chat] WITH (NOLOCK)
WHERE @Term = N''
   OR Sender LIKE N'%' + @Term + N'%'
   OR Receiver LIKE N'%' + @Term + N'%'
   OR [Message] LIKE N'%' + @Term + N'%'
ORDER BY [Timestamp] DESC;", new SqlParameter("@Term", term.Trim()));
    }

    public async Task<DataTable> LoadSchedulerAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSchedulerTableAsync(connection);
        var table = await QueryTableAsync(connection, @"
SELECT TOP (300)
       Idx,
       Name,
       Query,
       ScheduledDate,
       StartDateTime,
       Time,
       RepeatType,
       RepeatDayOfWeek,
       DaysOfWeekMask,
       IntervalSeconds,
       IsEnabled,
       ExecutionTimeoutSeconds,
       CatchUpWindowSeconds,
       LastScheduledDateTime,
       LastStatus,
       LastDurationMs,
       LastCompletedDateTimeUtc,
       LastError,
       RunningToken,
       RunningBy,
       RunningSinceUtc,
       LeaseUntilUtc,
       LastRunDateTime,
       CreatedAtUtc,
       UpdatedAtUtc
FROM [dbo].[System_Schedule] WITH (NOLOCK)
ORDER BY IsEnabled DESC, Idx DESC;");
        AddSchedulerDisplayColumns(table, DateTime.Now);
        return table;
    }

    public async Task<string> LoadCurrentDatabaseNameAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await using var command = new SqlCommand("SELECT DB_NAME();", connection);
        return Convert.ToString(await command.ExecuteScalarAsync()) ?? string.Empty;
    }

    public async Task SaveSchedulerJobAsync(SchedulerJobInput input)
    {
        var execQuery = BuildSchedulerExecQuery(
            input.DatabaseName,
            input.ProcedureName,
            input.Arguments);
        if (string.IsNullOrWhiteSpace(input.Name))
            throw new InvalidOperationException("Scheduler name is required.");
        if (input.Name.Trim().Length > 128)
            throw new InvalidOperationException("Scheduler name cannot exceed 128 characters.");
        if (execQuery.Length > 512)
            throw new InvalidOperationException("Scheduler EXEC command cannot exceed 512 characters.");
        if (!Enum.TryParse<SchedulerRepeatType>(input.RepeatType, true, out var parsedRepeat))
            throw new InvalidOperationException("Repeat must be Once, Interval, Daily, or Weekly.");
        if (parsedRepeat == SchedulerRepeatType.None && !input.ScheduledDate.HasValue)
            throw new InvalidOperationException("Date is required when Repeat is None.");
        if (parsedRepeat == SchedulerRepeatType.Interval &&
            (!input.StartDateTime.HasValue || input.IntervalSeconds is < 10 or > 604800))
        {
            throw new InvalidOperationException("Interval schedules require a start time and a repeat interval from 10 seconds to 7 days.");
        }
        if (parsedRepeat == SchedulerRepeatType.Weekly && input.DaysOfWeekMask is not (>= 1 and <= 127))
            throw new InvalidOperationException("Choose at least one weekday.");
        if (input.Time < TimeSpan.Zero || input.Time >= TimeSpan.FromDays(1))
            throw new InvalidOperationException("Time must be between 00:00:00 and 23:59:59.");
        if (input.ExecutionTimeoutSeconds is < 0 or > 604800)
            throw new InvalidOperationException("Execution timeout must be between 0 and 604800 seconds.");
        if (input.CatchUpWindowSeconds is < 1 or > 604800)
            throw new InvalidOperationException("Missed-run tolerance must be between 1 second and 7 days.");

        var catchUpWindowSeconds = parsedRepeat == SchedulerRepeatType.Interval
            ? Math.Max(input.CatchUpWindowSeconds, input.IntervalSeconds!.Value)
            : input.CatchUpWindowSeconds;

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSchedulerTableAsync(connection);
        await using var command = new SqlCommand(@"
IF @Idx IS NULL
BEGIN
    INSERT INTO [dbo].[System_Schedule]
        (Name, Query, ScheduledDate, StartDateTime, Time, RepeatType, RepeatDayOfWeek,
         DaysOfWeekMask, IntervalSeconds, IsEnabled, ExecutionTimeoutSeconds,
         CatchUpWindowSeconds, CreatedAtUtc, UpdatedAtUtc)
    VALUES
        (@Name, @Query, @ScheduledDate, @StartDateTime, @Time, @RepeatType, @RepeatDayOfWeek,
         @DaysOfWeekMask, @IntervalSeconds, @IsEnabled, @ExecutionTimeoutSeconds,
         @CatchUpWindowSeconds, SYSUTCDATETIME(), SYSUTCDATETIME());
    SELECT CONVERT(INT, SCOPE_IDENTITY());
END
ELSE
BEGIN
    UPDATE [dbo].[System_Schedule]
    SET Name = @Name,
        Query = @Query,
        ScheduledDate = @ScheduledDate,
        StartDateTime = @StartDateTime,
        Time = @Time,
        RepeatType = @RepeatType,
        RepeatDayOfWeek = @RepeatDayOfWeek,
        DaysOfWeekMask = @DaysOfWeekMask,
        IntervalSeconds = @IntervalSeconds,
        IsEnabled = @IsEnabled,
        ExecutionTimeoutSeconds = @ExecutionTimeoutSeconds,
        CatchUpWindowSeconds = @CatchUpWindowSeconds,
        LastScheduledDateTime = NULL,
        UpdatedAtUtc = SYSUTCDATETIME()
    WHERE Idx = @Idx
      AND RunningToken IS NULL;

    IF @@ROWCOUNT = 0
        THROW 51000, 'The selected schedule is running or no longer exists.', 1;
    SELECT @Idx;
END;", connection);
        command.Parameters.Add("@Idx", SqlDbType.Int).Value = input.Idx.HasValue ? input.Idx.Value : DBNull.Value;
        command.Parameters.Add("@Name", SqlDbType.NVarChar, 128).Value = input.Name.Trim();
        command.Parameters.Add("@Query", SqlDbType.NVarChar, 512).Value = execQuery;
        command.Parameters.Add("@ScheduledDate", SqlDbType.Date).Value =
            parsedRepeat == SchedulerRepeatType.None && input.ScheduledDate.HasValue
                ? input.ScheduledDate.Value.Date
                : DBNull.Value;
        command.Parameters.Add("@StartDateTime", SqlDbType.DateTime2).Value =
            parsedRepeat == SchedulerRepeatType.Interval && input.StartDateTime.HasValue
                ? input.StartDateTime.Value
                : DBNull.Value;
        command.Parameters.Add("@Time", SqlDbType.Time).Value = input.Time;
        command.Parameters.Add("@RepeatType", SqlDbType.NVarChar, 16).Value = parsedRepeat.ToString();
        command.Parameters.Add("@RepeatDayOfWeek", SqlDbType.TinyInt).Value =
            parsedRepeat == SchedulerRepeatType.Weekly && input.DaysOfWeekMask.HasValue
                ? FirstDayFromMask(input.DaysOfWeekMask.Value)
                : DBNull.Value;
        command.Parameters.Add("@DaysOfWeekMask", SqlDbType.TinyInt).Value =
            parsedRepeat == SchedulerRepeatType.Weekly && input.DaysOfWeekMask.HasValue
                ? input.DaysOfWeekMask.Value
                : DBNull.Value;
        command.Parameters.Add("@IntervalSeconds", SqlDbType.Int).Value =
            parsedRepeat == SchedulerRepeatType.Interval && input.IntervalSeconds.HasValue
                ? input.IntervalSeconds.Value
                : DBNull.Value;
        command.Parameters.Add("@IsEnabled", SqlDbType.Bit).Value = input.Enabled;
        command.Parameters.Add("@ExecutionTimeoutSeconds", SqlDbType.Int).Value = input.ExecutionTimeoutSeconds;
        command.Parameters.Add("@CatchUpWindowSeconds", SqlDbType.Int).Value = catchUpWindowSeconds;
        var savedId = Convert.ToInt32(await command.ExecuteScalarAsync());
        await AuditAsync(
            connection,
            input.Idx.HasValue ? "UpdateSchedulerJob" : "AddSchedulerJob",
            $"{input.Name.Trim()} (#{savedId})",
            $"{parsedRepeat}: {execQuery}");
    }

    public async Task SetSchedulerEnabledAsync(int idx, bool enabled)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSchedulerTableAsync(connection);
        await using var command = new SqlCommand(@"
UPDATE [dbo].[System_Schedule]
SET IsEnabled = @IsEnabled,
    UpdatedAtUtc = SYSUTCDATETIME()
WHERE Idx = @Idx;", connection);
        command.Parameters.Add("@Idx", SqlDbType.Int).Value = idx;
        command.Parameters.Add("@IsEnabled", SqlDbType.Bit).Value = enabled;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "SetSchedulerEnabled", $"Idx={idx}", enabled.ToString());
    }

    public async Task DeleteSchedulerJobAsync(int idx)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSchedulerTableAsync(connection);
        await using var command = new SqlCommand(@"
DELETE FROM [dbo].[System_Schedule]
WHERE Idx = @Idx
  AND RunningToken IS NULL;", connection);
        command.Parameters.Add("@Idx", SqlDbType.Int).Value = idx;
        if (await command.ExecuteNonQueryAsync() == 0)
            throw new InvalidOperationException("The selected schedule is running or no longer exists.");
        await AuditAsync(connection, "DeleteSchedulerJob", $"Idx={idx}", string.Empty);
    }

    public async Task<DataTable> LoadSchedulerHistoryAsync(int? idx = null)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSchedulerTableAsync(connection);
        return await QueryTableAsync(connection, @"
SELECT TOP (200)
       RunID,
       JobId,
       JobName,
       TriggerType,
       ScheduledFor,
       DATEADD(MINUTE, DATEDIFF(MINUTE, GETUTCDATE(), GETDATE()), StartedAtUtc) AS StartedLocal,
       DATEADD(MINUTE, DATEDIFF(MINUTE, GETUTCDATE(), GETDATE()), CompletedAtUtc) AS CompletedLocal,
       Status,
       DurationMs,
       ExecutedBy,
       Error
FROM [dbo].[System_ScheduleHistory] WITH (NOLOCK)
WHERE @Idx IS NULL OR JobId = @Idx
ORDER BY RunID DESC;", new SqlParameter("@Idx", SqlDbType.Int)
        {
            Value = idx.HasValue ? idx.Value : DBNull.Value
        });
    }

    public async Task<string> RunSchedulerJobNowAsync(int idx)
    {
        await using var claimConnection = new SqlConnection(_connectionString);
        await claimConnection.OpenAsync();
        await EnsureSchedulerTableAsync(claimConnection);
        var runToken = Guid.NewGuid();
        string name;
        string query;
        int timeoutSeconds;
        var startedAtUtc = DateTime.UtcNow;

        await using (var claim = new SqlCommand(@"
UPDATE [dbo].[System_Schedule] WITH (UPDLOCK, ROWLOCK)
SET RunningToken = @RunToken,
    RunningBy = @RunningBy,
    RunningSinceUtc = SYSUTCDATETIME(),
    LeaseUntilUtc = DATEADD
    (
        SECOND,
        CASE
            WHEN ExecutionTimeoutSeconds = 0 THEN 7200
            WHEN ExecutionTimeoutSeconds BETWEEN 1 AND 604800 THEN ExecutionTimeoutSeconds + 60
            ELSE 7260
        END,
        SYSUTCDATETIME()
    ),
    LastStatus = N'Running',
    LastError = NULL
OUTPUT INSERTED.Name, INSERTED.Query, INSERTED.ExecutionTimeoutSeconds
WHERE Idx = @Idx
  AND (RunningToken IS NULL OR LeaseUntilUtc < SYSUTCDATETIME());", claimConnection))
        {
            claim.Parameters.Add("@RunToken", SqlDbType.UniqueIdentifier).Value = runToken;
            claim.Parameters.Add("@RunningBy", SqlDbType.NVarChar, 160).Value =
                $"Admin:{AppSession.CurrentAdmin}@{Environment.MachineName}";
            claim.Parameters.Add("@Idx", SqlDbType.Int).Value = idx;
            await using var reader = await claim.ExecuteReaderAsync();
            if (!await reader.ReadAsync())
                throw new InvalidOperationException("The selected schedule is already running or no longer exists.");
            name = reader.GetString(0);
            query = reader.GetString(1);
            timeoutSeconds = reader.GetInt32(2);
        }

        var stopwatch = System.Diagnostics.Stopwatch.StartNew();
        var status = "Succeeded";
        string? error = null;
        try
        {
            if (!IsSafeExec(query))
                throw new InvalidOperationException("The saved command is no longer considered safe.");

            await using var executionConnection = new SqlConnection(_connectionString);
            await executionConnection.OpenAsync();
            await using var execution = new SqlCommand(query, executionConnection)
            {
                CommandTimeout = timeoutSeconds is >= 0 and <= 604800 ? timeoutSeconds : 7200
            };
            await execution.ExecuteNonQueryAsync();
        }
        catch (Exception ex)
        {
            status = ex is SqlException { Number: -2 } ? "TimedOut" : "Failed";
            error = ex.Message;
        }
        finally
        {
            stopwatch.Stop();
        }

        await using var completeConnection = new SqlConnection(_connectionString);
        await completeConnection.OpenAsync();
        await using (var complete = new SqlCommand(@"
SET XACT_ABORT ON;
BEGIN TRANSACTION;
UPDATE [dbo].[System_Schedule]
SET RunningToken = NULL,
    RunningBy = NULL,
    RunningSinceUtc = NULL,
    LeaseUntilUtc = NULL,
    LastStatus = @Status,
    LastError = @Error,
    LastDurationMs = @DurationMs,
    LastCompletedDateTimeUtc = SYSUTCDATETIME(),
    LastRunDateTime = CASE WHEN @Status = N'Succeeded' THEN SYSDATETIME() ELSE LastRunDateTime END
WHERE RunningToken = @RunToken;

INSERT INTO [dbo].[System_ScheduleHistory]
    (JobId, JobName, TriggerType, ScheduledFor, StartedAtUtc, CompletedAtUtc,
     Status, DurationMs, ExecutedBy, Error)
VALUES
    (@Idx, @Name, N'Manual', NULL, @StartedAtUtc, SYSUTCDATETIME(),
     @Status, @DurationMs, @ExecutedBy, @Error);
COMMIT TRANSACTION;", completeConnection))
        {
            complete.Parameters.Add("@RunToken", SqlDbType.UniqueIdentifier).Value = runToken;
            complete.Parameters.Add("@Idx", SqlDbType.Int).Value = idx;
            complete.Parameters.Add("@Name", SqlDbType.NVarChar, 128).Value = name;
            complete.Parameters.Add("@StartedAtUtc", SqlDbType.DateTime2).Value = startedAtUtc;
            complete.Parameters.Add("@Status", SqlDbType.NVarChar, 16).Value = status;
            complete.Parameters.Add("@DurationMs", SqlDbType.BigInt).Value = stopwatch.ElapsedMilliseconds;
            complete.Parameters.Add("@ExecutedBy", SqlDbType.NVarChar, 160).Value =
                $"Admin:{AppSession.CurrentAdmin}@{Environment.MachineName}";
            complete.Parameters.Add("@Error", SqlDbType.NVarChar, 2048).Value =
                string.IsNullOrWhiteSpace(error) ? DBNull.Value : error[..Math.Min(error.Length, 2048)];
            await complete.ExecuteNonQueryAsync();
        }

        await AuditAsync(completeConnection, "RunSchedulerJobNow", $"{name} (#{idx})", status);
        return status == "Succeeded"
            ? $"“{name}” completed successfully in {stopwatch.Elapsed.TotalSeconds:N1} seconds."
            : $"“{name}” ended with {status}: {error}";
    }

    public async Task<DataTable> LoadWebViewerButtonsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        if (!await ObjectExistsAsync(connection, "[dbo].[Web_Buttons]", "U"))
            return new DataTable();

        return await QueryTableAsync(connection, @"
SELECT TOP (300) ID, DisplayOrder, Name, IconPath, Url, FrameWidth, FrameHeight, IsEnabled, UpdatedAtUtc
FROM [dbo].[Web_Buttons] WITH (NOLOCK)
ORDER BY IsEnabled DESC, DisplayOrder, ID;");
    }

    public async Task AddWebViewerButtonAsync(string name, string iconPath, string url, int width, int height, int order, bool enabled)
    {
        if (!Uri.TryCreate(url, UriKind.Absolute, out _))
            throw new InvalidOperationException("URL must be valid.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTableAsync(connection, "[dbo].[Web_Buttons]");
        await using var command = new SqlCommand(@"
INSERT INTO [dbo].[Web_Buttons] (DisplayOrder, Name, IconPath, Url, FrameWidth, FrameHeight, IsEnabled, UpdatedAtUtc)
VALUES (@DisplayOrder, @Name, @IconPath, @Url, @FrameWidth, @FrameHeight, @IsEnabled, SYSUTCDATETIME());", connection);
        command.Parameters.Add("@DisplayOrder", SqlDbType.Int).Value = order;
        command.Parameters.Add("@Name", SqlDbType.NVarChar, 64).Value = name.Trim();
        command.Parameters.Add("@IconPath", SqlDbType.VarChar, 260).Value = iconPath.Trim();
        command.Parameters.Add("@Url", SqlDbType.VarChar, 512).Value = url.Trim();
        command.Parameters.Add("@FrameWidth", SqlDbType.Int).Value = width;
        command.Parameters.Add("@FrameHeight", SqlDbType.Int).Value = height;
        command.Parameters.Add("@IsEnabled", SqlDbType.Bit).Value = enabled;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "AddWebViewerButton", name, url);
    }

    public async Task SetWebViewerButtonEnabledAsync(int id, bool enabled)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await using var command = new SqlCommand("UPDATE [dbo].[Web_Buttons] SET IsEnabled = @IsEnabled, UpdatedAtUtc = SYSUTCDATETIME() WHERE ID = @ID", connection);
        command.Parameters.Add("@ID", SqlDbType.Int).Value = id;
        command.Parameters.Add("@IsEnabled", SqlDbType.Bit).Value = enabled;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "SetWebViewerButtonEnabled", $"ID={id}", enabled.ToString());
    }

    public async Task<DataTable> LoadPacketRulesAsync(string tableName)
    {
        var safeTableName = NormalizePacketRuleTableName(tableName);

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        if (!await ObjectExistsAsync(connection, safeTableName, "U"))
            return new DataTable();

        return await QueryTableAsync(connection, $"SELECT TOP (500) * FROM {safeTableName} WITH (NOLOCK) ORDER BY ServerType, MsgId");
    }

    public async Task AddPacketRuleAsync(string tableName, int serverType, int msgId)
    {
        var safeTableName = NormalizePacketRuleTableName(tableName);

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTableAsync(connection, safeTableName);
        await using var command = new SqlCommand($"IF NOT EXISTS (SELECT 1 FROM {safeTableName} WHERE ServerType = @ServerType AND MsgId = @MsgId) INSERT INTO {safeTableName} (ServerType, MsgId) VALUES (@ServerType, @MsgId);", connection);
        command.Parameters.Add("@ServerType", SqlDbType.Int).Value = serverType;
        command.Parameters.Add("@MsgId", SqlDbType.Int).Value = msgId;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "AddPacketRule", safeTableName, $"ServerType={serverType};MsgId={msgId:X4}");
    }

    internal static string NormalizePacketRuleTableName(string tableName)
    {
        var normalized = Regex.Replace(tableName?.Trim() ?? string.Empty, @"[\[\]\s]", string.Empty);
        if (normalized.StartsWith("dbo.", StringComparison.OrdinalIgnoreCase))
            normalized = normalized[4..];

        return normalized.ToLowerInvariant() switch
        {
            "security_whitelist" or "__whitelist" or "_whitelist" or "whitelist" => PacketWhitelistTable,
            "security_blacklist" or "__blacklist" or "_blacklist" or "blacklist" => PacketBlacklistTable,
            _ => throw new InvalidOperationException("Invalid packet rule table.")
        };
    }

    public async Task<DataTable> LoadEconomyLogAsync(string logType, string term)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        return logType switch
        {
            "LuckySpin" => await LoadOptionalTableAsync(connection, "[dbo].[LuckySpin_Log]", @"
SELECT TOP (500) ID, CharID, JID, RewardID, ItemID, Amount, Price, PaymentType, CreatedAt
FROM [dbo].[LuckySpin_Log] WITH (NOLOCK)
WHERE @Term = N'' OR CONVERT(NVARCHAR(32), CharID) = @Term OR CONVERT(NVARCHAR(32), JID) = @Term OR CONVERT(NVARCHAR(32), ItemID) = @Term
ORDER BY ID DESC;", term),
            "SpecialOffers" => await LoadOptionalTableAsync(connection, "[dbo].[Offer_PurchaseLog]", @"
SELECT TOP (500) ID, OfferID, CharID, JID, ItemID, ItemCount, Price, PaymentType, CreatedAt
FROM [dbo].[Offer_PurchaseLog] WITH (NOLOCK)
WHERE @Term = N'' OR CONVERT(NVARCHAR(32), CharID) = @Term OR CONVERT(NVARCHAR(32), JID) = @Term OR CONVERT(NVARCHAR(32), ItemID) = @Term
ORDER BY ID DESC;", term),
            "SilkStall" => await LoadOptionalTableAsync(connection, "[dbo].[Stall_SilkTransactions]", @"
SELECT TOP (500) ID, BuyerCharName, SellerCharName, StallSlot, SilkAmount, Status, FailureReason, CreatedAt, CompletedAt, RefundedAt
FROM [dbo].[Stall_SilkTransactions] WITH (NOLOCK)
WHERE @Term = N'' OR BuyerCharName LIKE N'%' + @Term + N'%' OR SellerCharName LIKE N'%' + @Term + N'%' OR CONVERT(NVARCHAR(32), ID) = @Term
ORDER BY ID DESC;", term),
            _ => new DataTable()
        };
    }

    public async Task<DataTable> LoadAutoEventConfigAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        return await LoadOptionalTableAsync(connection, EventObjectName("_AutoEventConfig"), $@"
SELECT EventCode, DisplayName, Enabled, StartDelaySeconds, RoundCount, RoundDurationSeconds, InterRoundDelaySeconds, MinLevel, HwidLimit, UniqueWinnerPerRun, RequireHwid, AnswerCooldownMs, AlchemyTargetPlus, UpdatedAtUtc
FROM {EventTable("_AutoEventConfig")} WITH (NOLOCK)
WHERE EventCode NOT IN (N'SPARTY', N'SSOLO')
ORDER BY EventCode;", string.Empty);
    }

    public async Task<DataTable> LoadAutoEventSchedulesAsync(string eventCode)
    {
        eventCode = eventCode.Trim();
        if (eventCode.Length == 0)
            return new DataTable();

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureAutoEventScheduleSchemaAsync(connection);
        return await QueryTableAsync(connection, $@"
SELECT ScheduleID,
       CONVERT(varchar(5), StartTime, 108) AS StartTime,
       ISNULL(RepeatMinutes, 0) AS RepeatMinutes,
       CASE
           WHEN ISNULL(RepeatMinutes, 0) = 0 THEN N'Once daily'
           WHEN RepeatMinutes = 60 THEN N'Every hour'
           WHEN RepeatMinutes % 60 = 0 THEN N'Every ' + CONVERT(nvarchar(10), RepeatMinutes / 60) + N' hours'
           ELSE N'Every ' + CONVERT(nvarchar(10), RepeatMinutes) + N' min'
       END AS RepeatText,
       DaysMask,
       STUFF(
           CASE WHEN (DaysMask & 1) = 1 THEN N', Sun' ELSE N'' END +
           CASE WHEN (DaysMask & 2) = 2 THEN N', Mon' ELSE N'' END +
           CASE WHEN (DaysMask & 4) = 4 THEN N', Tue' ELSE N'' END +
           CASE WHEN (DaysMask & 8) = 8 THEN N', Wed' ELSE N'' END +
           CASE WHEN (DaysMask & 16) = 16 THEN N', Thu' ELSE N'' END +
           CASE WHEN (DaysMask & 32) = 32 THEN N', Fri' ELSE N'' END +
           CASE WHEN (DaysMask & 64) = 64 THEN N', Sat' ELSE N'' END,
           1, 2, N'') AS Days,
       IsActive,
       COALESCE(CONVERT(nvarchar(16), LastRunAtLocal, 120), CONVERT(nvarchar(10), LastRunLocalDate, 120), N'') AS LastRun
FROM {EventTable("_AutoEventSchedule")} WITH (NOLOCK)
WHERE EventCode = @EventCode
ORDER BY StartTime, ScheduleID;",
            new SqlParameter("@EventCode", SqlDbType.NVarChar, 32) { Value = eventCode });
    }

    public async Task SaveAutoEventScheduleAsync(
        string eventCode,
        int scheduleId,
        TimeSpan startTime,
        int daysMask,
        int repeatMinutes,
        bool isActive)
    {
        eventCode = eventCode.Trim();
        if (eventCode.Length == 0)
            throw new InvalidOperationException("Select an event before saving a schedule.");
        if (startTime < TimeSpan.Zero || startTime >= TimeSpan.FromDays(1))
            throw new InvalidOperationException("Start time must be between 00:00 and 23:59.");
        if (daysMask is < 1 or > 127)
            throw new InvalidOperationException("Select at least one day.");
        if (repeatMinutes != 0 && repeatMinutes is < 30 or > 1440)
            throw new InvalidOperationException("Repeat frequency must be at least 30 minutes.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureAutoEventScheduleSchemaAsync(connection);

        if (isActive)
        {
            var existingSchedules = await LoadAutomaticEventSchedulePatternsAsync(connection, scheduleId);
            var candidate = new AutoEventSchedulePattern(
                $"Auto:{scheduleId}", eventCode, startTime, daysMask, repeatMinutes);
            var conflict = FindAutoEventScheduleConflict(candidate, existingSchedules, TimeSpan.FromMinutes(15));
            if (conflict != null)
            {
                throw new InvalidOperationException(
                    $"This schedule conflicts with {conflict.EventCode} on {conflict.Day} at {conflict.Time:hh\\:mm}. " +
                    "Automatic events must be at least 15 minutes apart.");
            }
        }

        var sql = scheduleId > 0
            ? $@"UPDATE {EventTable("_AutoEventSchedule")}
                 SET StartTime = @StartTime,
                     DaysMask = @DaysMask,
                     RepeatMinutes = NULLIF(@RepeatMinutes, 0),
                     IsActive = @IsActive,
                     LastRunLocalDate = NULL,
                     LastRunAtLocal = NULL,
                     UpdatedAtUtc = SYSUTCDATETIME()
                 WHERE ScheduleID = @ScheduleID AND EventCode = @EventCode;"
            : $@"INSERT INTO {EventTable("_AutoEventSchedule")}
                    (EventCode, StartTime, DaysMask, RepeatMinutes, IsActive)
                 VALUES (@EventCode, @StartTime, @DaysMask, NULLIF(@RepeatMinutes, 0), @IsActive);";

        await using var command = new SqlCommand(sql, connection);
        command.Parameters.Add("@ScheduleID", SqlDbType.Int).Value = scheduleId;
        command.Parameters.Add("@EventCode", SqlDbType.NVarChar, 32).Value = eventCode;
        command.Parameters.Add("@StartTime", SqlDbType.Time).Value = startTime;
        command.Parameters.Add("@DaysMask", SqlDbType.TinyInt).Value = daysMask;
        command.Parameters.Add("@RepeatMinutes", SqlDbType.SmallInt).Value = repeatMinutes;
        command.Parameters.Add("@IsActive", SqlDbType.Bit).Value = isActive;
        var affected = await command.ExecuteNonQueryAsync();
        if (scheduleId > 0 && affected != 1)
            throw new InvalidOperationException("The selected schedule no longer exists.");

        await AuditAsync(
            connection,
            scheduleId > 0 ? "UpdateAutoEventSchedule" : "AddAutoEventSchedule",
            eventCode,
            $"{startTime:hh\\:mm};DaysMask={daysMask};RepeatMinutes={repeatMinutes};Active={isActive}");
    }

    public async Task DeleteAutoEventScheduleAsync(string eventCode, int scheduleId)
    {
        if (scheduleId <= 0)
            throw new InvalidOperationException("Select a schedule to delete.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureAutoEventScheduleSchemaAsync(connection);
        await using var command = new SqlCommand(
            $"DELETE FROM {EventTable("_AutoEventSchedule")} WHERE ScheduleID = @ScheduleID AND EventCode = @EventCode;",
            connection);
        command.Parameters.Add("@ScheduleID", SqlDbType.Int).Value = scheduleId;
        command.Parameters.Add("@EventCode", SqlDbType.NVarChar, 32).Value = eventCode.Trim();
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "DeleteAutoEventSchedule", eventCode, scheduleId.ToString());
    }

    private static async Task<IReadOnlyList<AutoEventSchedulePattern>> LoadAutomaticEventSchedulePatternsAsync(
        SqlConnection connection,
        int excludedAutoScheduleId)
    {
        await using var command = new SqlCommand($@"
CREATE TABLE #Schedules
(
    [{AutoEventSchedulePatternKeyColumnName}] nvarchar(64) NOT NULL,
    EventCode nvarchar(32) NOT NULL,
    StartTime time(0) NOT NULL,
    DaysMask tinyint NOT NULL,
    RepeatMinutes int NOT NULL
);

INSERT #Schedules ([{AutoEventSchedulePatternKeyColumnName}], EventCode, StartTime, DaysMask, RepeatMinutes)
SELECT N'Auto:' + CONVERT(nvarchar(20), ScheduleID), EventCode, StartTime, DaysMask, ISNULL(RepeatMinutes, 0)
FROM Events.dbo._AutoEventSchedule WITH (NOLOCK)
WHERE IsActive = 1 AND ScheduleID <> @ExcludedAutoScheduleID;

IF OBJECT_ID(N'Events.dbo._SurvivalPartySchedule', N'U') IS NOT NULL
    EXEC(N'INSERT #Schedules
           SELECT N''SurvivalParty:'' + CONVERT(nvarchar(20), ScheduleID), N''SPARTY'', StartTime, DaysMask, 0
           FROM Events.dbo._SurvivalPartySchedule WITH (NOLOCK) WHERE IsActive = 1;');

IF OBJECT_ID(N'Events.dbo._SurvivalSoloSchedule', N'U') IS NOT NULL
    EXEC(N'INSERT #Schedules
           SELECT N''SurvivalSolo:'' + CONVERT(nvarchar(20), ScheduleID), N''SSOLO'', StartTime, DaysMask, 0
           FROM Events.dbo._SurvivalSoloSchedule WITH (NOLOCK) WHERE IsActive = 1;');

IF OBJECT_ID(N'Events.dbo._HideAndSeekSchedule', N'U') IS NOT NULL
    EXEC(N'INSERT #Schedules
           SELECT N''HideAndSeek:'' + CONVERT(nvarchar(20), ScheduleID), N''HNS'', StartTime, DaysMask, 0
           FROM Events.dbo._HideAndSeekSchedule WITH (NOLOCK) WHERE IsActive = 1;');

IF OBJECT_ID(N'Events.dbo._CompetitiveEventSchedule', N'U') IS NOT NULL
    EXEC(N'INSERT #Schedules
           SELECT N''Competitive:'' + CONVERT(nvarchar(20), ScheduleID), EventCode, StartTime, DaysMask, 0
           FROM Events.dbo._CompetitiveEventSchedule WITH (NOLOCK) WHERE IsActive = 1;');

SELECT [{AutoEventSchedulePatternKeyColumnName}], EventCode, StartTime, DaysMask, RepeatMinutes FROM #Schedules;", connection);
        command.Parameters.Add("@ExcludedAutoScheduleID", SqlDbType.Int).Value = excludedAutoScheduleId;

        var schedules = new List<AutoEventSchedulePattern>();
        await using var reader = await command.ExecuteReaderAsync();
        while (await reader.ReadAsync())
        {
            schedules.Add(new AutoEventSchedulePattern(
                reader.GetString(0),
                reader.GetString(1),
                reader.GetTimeSpan(2),
                reader.GetByte(3),
                reader.GetInt32(4)));
        }

        return schedules;
    }

    internal static AutoEventScheduleConflict? FindAutoEventScheduleConflict(
        AutoEventSchedulePattern candidate,
        IEnumerable<AutoEventSchedulePattern> existingSchedules,
        TimeSpan minimumGap)
    {
        var minimumGapMinutes = Math.Max(1, (int)Math.Ceiling(minimumGap.TotalMinutes));
        var candidateOccurrences = ExpandAutoEventSchedule(candidate).ToArray();

        foreach (var existing in existingSchedules)
        {
            foreach (var existingOccurrence in ExpandAutoEventSchedule(existing))
            {
                foreach (var candidateOccurrence in candidateOccurrences)
                {
                    var directDistance = Math.Abs(candidateOccurrence.MinuteOfWeek - existingOccurrence.MinuteOfWeek);
                    var distance = Math.Min(directDistance, (7 * 24 * 60) - directDistance);
                    if (distance >= minimumGapMinutes)
                        continue;

                    return new AutoEventScheduleConflict(
                        existing.EventCode,
                        (DayOfWeek)(existingOccurrence.MinuteOfWeek / (24 * 60)),
                        TimeSpan.FromMinutes(existingOccurrence.MinuteOfWeek % (24 * 60)));
                }
            }
        }

        return null;
    }

    private static IEnumerable<(int MinuteOfWeek, string ScheduleKey)> ExpandAutoEventSchedule(
        AutoEventSchedulePattern schedule)
    {
        var firstMinute = (int)schedule.StartTime.TotalMinutes;
        var repeatMinutes = schedule.RepeatMinutes >= 30 ? schedule.RepeatMinutes : 0;
        for (var day = 0; day < 7; day++)
        {
            if ((schedule.DaysMask & (1 << day)) == 0)
                continue;

            for (var minute = firstMinute; minute < 24 * 60; minute += repeatMinutes == 0 ? 24 * 60 : repeatMinutes)
            {
                yield return ((day * 24 * 60) + minute, schedule.ScheduleKey);
                if (repeatMinutes == 0)
                    break;
            }
        }
    }

    private static async Task EnsureAutoEventScheduleSchemaAsync(SqlConnection connection)
    {
        await using var command = new SqlCommand(@"
SELECT CASE
           WHEN OBJECT_ID(N'Events.dbo._AutoEventSchedule', N'U') IS NOT NULL
            AND COL_LENGTH(N'Events.dbo._AutoEventSchedule', N'RepeatMinutes') IS NOT NULL
            AND COL_LENGTH(N'Events.dbo._AutoEventSchedule', N'LastRunAtLocal') IS NOT NULL
           THEN 1 ELSE 0
       END;", connection);
        var ready = Convert.ToInt32(await command.ExecuteScalarAsync());
        if (ready != 1)
            throw new InvalidOperationException(
                "Automatic event scheduling requires the v3.1.0 database update.");
    }

    public async Task<DataTable> LoadAutoEventRunsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        return await LoadOptionalTableAsync(connection, EventObjectName("_AutoEventRun"), $@"
SELECT TOP (300) RunID, EventCode, Status, StartedAtUtc, FinishedAtUtc, StartedBy, Message
FROM {EventTable("_AutoEventRun")} WITH (NOLOCK)
ORDER BY RunID DESC;", string.Empty);
    }

    public async Task<DataTable> LoadAutoEventWinnersAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        return await LoadOptionalTableAsync(connection, EventObjectName("_AutoEventWinnerLog"), $@"
SELECT TOP (300) LogID, EventCode, RoundNo, CharID, CharName, JID, Hwid, ClientIP, Answer, WonAtUtc, RewardSummary
FROM {EventTable("_AutoEventWinnerLog")} WITH (NOLOCK)
ORDER BY LogID DESC;", string.Empty);
    }

    public async Task SetAutoEventEnabledAsync(string eventCode, bool enabled)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTableAsync(connection, EventObjectName("_AutoEventConfig"));
        await using var command = new SqlCommand($"UPDATE {EventTable("_AutoEventConfig")} SET Enabled = @Enabled, UpdatedAtUtc = SYSUTCDATETIME() WHERE EventCode = @EventCode", connection);
        command.Parameters.Add("@EventCode", SqlDbType.NVarChar, 32).Value = eventCode;
        command.Parameters.Add("@Enabled", SqlDbType.Bit).Value = enabled;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "SetAutoEventEnabled", eventCode, enabled.ToString());
    }

    public async Task QueueAutoEventCommandAsync(string commandType, string eventCode)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTableAsync(connection, EventObjectName("_AutoEventCommandQueue"));
        await using var command = new SqlCommand($@"
INSERT INTO {EventTable("_AutoEventCommandQueue")} (CommandType, EventCode, RequestedBy, Status)
VALUES (@CommandType, @EventCode, @RequestedBy, 0);", connection);
        command.Parameters.Add("@CommandType", SqlDbType.NVarChar, 16).Value = commandType.Trim().ToUpperInvariant();
        command.Parameters.Add("@EventCode", SqlDbType.NVarChar, 32).Value = eventCode.Trim();
        command.Parameters.Add("@RequestedBy", SqlDbType.NVarChar, 64).Value = "AdminDesktop";
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "QueueAutoEventCommand", commandType, eventCode);
    }

    public async Task<string> QueueAutoEventCommandAndWaitAsync(string commandType, string eventCode, TimeSpan timeout)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTableAsync(connection, EventObjectName("_AutoEventCommandQueue"));
        await using var insert = new SqlCommand($@"
INSERT INTO {EventTable("_AutoEventCommandQueue")} (CommandType, EventCode, RequestedBy, Status)
OUTPUT INSERTED.CommandID
VALUES (@CommandType, @EventCode, @RequestedBy, 0);", connection);
        insert.Parameters.Add("@CommandType", SqlDbType.NVarChar, 16).Value = commandType.Trim().ToUpperInvariant();
        insert.Parameters.Add("@EventCode", SqlDbType.NVarChar, 32).Value = eventCode.Trim();
        insert.Parameters.Add("@RequestedBy", SqlDbType.NVarChar, 64).Value = "AdminDesktop";
        var commandId = Convert.ToInt64(await insert.ExecuteScalarAsync());
        await AuditAsync(connection, "QueueAutoEventCommand", commandType, eventCode);

        var deadline = DateTime.UtcNow.Add(timeout);
        while (DateTime.UtcNow < deadline)
        {
            await Task.Delay(250);
            await using var statusCommand = new SqlCommand($@"
SELECT Status, Message
FROM {EventTable("_AutoEventCommandQueue")} WITH (NOLOCK)
WHERE CommandID = @CommandID;", connection);
            statusCommand.Parameters.Add("@CommandID", SqlDbType.BigInt).Value = commandId;
            await using var reader = await statusCommand.ExecuteReaderAsync();
            if (!await reader.ReadAsync())
                throw new InvalidOperationException("The runtime command was removed before it was processed.");

            var status = Convert.ToInt32(reader.GetValue(0));
            var message = reader.IsDBNull(1) ? string.Empty : reader.GetString(1);
            if (status == 2)
                return string.IsNullOrWhiteSpace(message) ? "Command completed." : message;
            if (status == 3)
                throw new InvalidOperationException(string.IsNullOrWhiteSpace(message) ? "The runtime rejected the command." : message);
        }

        throw new TimeoutException("The Agent service did not process the command. Check that KMTGuard.Agent is running and connected to the same Events database.");
    }

    public async Task SaveAutoEventConfigAsync(
        string eventCode,
        string displayName,
        bool enabled,
        int startDelaySeconds,
        int roundCount,
        int roundDurationSeconds,
        int interRoundDelaySeconds,
        int minLevel,
        int hwidLimit,
        bool uniqueWinnerPerRun,
        bool requireHwid,
        int answerCooldownMs,
        int alchemyTargetPlus)
    {
        if (eventCode.Trim().Equals("SPARTY", StringComparison.OrdinalIgnoreCase) ||
            eventCode.Trim().Equals("SSOLO", StringComparison.OrdinalIgnoreCase))
            throw new InvalidOperationException("Use the dedicated Survival event page for this event code.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTableAsync(connection, EventObjectName("_AutoEventConfig"));

        await using var command = new SqlCommand($@"
MERGE {EventTable("_AutoEventConfig")} AS target
USING (SELECT @EventCode AS EventCode) AS source
ON target.EventCode = source.EventCode
WHEN MATCHED THEN
    UPDATE SET DisplayName = @DisplayName,
               Enabled = @Enabled,
               StartDelaySeconds = @StartDelaySeconds,
               RoundCount = @RoundCount,
               RoundDurationSeconds = @RoundDurationSeconds,
               InterRoundDelaySeconds = @InterRoundDelaySeconds,
               MinLevel = @MinLevel,
               HwidLimit = @HwidLimit,
               UniqueWinnerPerRun = @UniqueWinnerPerRun,
               RequireHwid = @RequireHwid,
               AnswerCooldownMs = @AnswerCooldownMs,
               AlchemyTargetPlus = @AlchemyTargetPlus,
               UpdatedAtUtc = SYSUTCDATETIME()
WHEN NOT MATCHED THEN
    INSERT (EventCode, DisplayName, Enabled, StartDelaySeconds, RoundCount, RoundDurationSeconds, InterRoundDelaySeconds, MinLevel, HwidLimit, UniqueWinnerPerRun, RequireHwid, AnswerCooldownMs, AlchemyTargetPlus)
    VALUES (@EventCode, @DisplayName, @Enabled, @StartDelaySeconds, @RoundCount, @RoundDurationSeconds, @InterRoundDelaySeconds, @MinLevel, @HwidLimit, @UniqueWinnerPerRun, @RequireHwid, @AnswerCooldownMs, @AlchemyTargetPlus);", connection);

        command.Parameters.Add("@EventCode", SqlDbType.NVarChar, 32).Value = eventCode.Trim();
        command.Parameters.Add("@DisplayName", SqlDbType.NVarChar, 64).Value = displayName.Trim();
        command.Parameters.Add("@Enabled", SqlDbType.Bit).Value = enabled;
        command.Parameters.Add("@StartDelaySeconds", SqlDbType.Int).Value = Math.Clamp(startDelaySeconds, 0, 3600);
        command.Parameters.Add("@RoundCount", SqlDbType.Int).Value = roundCount;
        command.Parameters.Add("@RoundDurationSeconds", SqlDbType.Int).Value = roundDurationSeconds;
        command.Parameters.Add("@InterRoundDelaySeconds", SqlDbType.Int).Value = interRoundDelaySeconds;
        command.Parameters.Add("@MinLevel", SqlDbType.Int).Value = minLevel;
        command.Parameters.Add("@HwidLimit", SqlDbType.Int).Value = Math.Clamp(hwidLimit, 0, 32);
        command.Parameters.Add("@UniqueWinnerPerRun", SqlDbType.Bit).Value = uniqueWinnerPerRun;
        command.Parameters.Add("@RequireHwid", SqlDbType.Bit).Value = requireHwid;
        command.Parameters.Add("@AnswerCooldownMs", SqlDbType.Int).Value = answerCooldownMs;
        command.Parameters.Add("@AlchemyTargetPlus", SqlDbType.Int).Value = alchemyTargetPlus;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "SaveAutoEventConfig", eventCode, displayName);
    }

    public async Task<DataTable> LoadAutoEventContentAsync(string eventCode)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        return await LoadOptionalTableAsync(connection, EventObjectName("_AutoEventRoundContent"), $@"
SELECT TOP (300) ContentID, EventCode, IsActive, Prompt, Answer, Weight, CreatedAtUtc
FROM {EventTable("_AutoEventRoundContent")} WITH (NOLOCK)
WHERE EventCode NOT IN (N'SPARTY', N'SSOLO') AND (@Term = N'' OR EventCode = @Term)
ORDER BY EventCode, IsActive DESC, Weight DESC, ContentID DESC;", eventCode);
    }

    public async Task SaveAutoEventContentAsync(int contentId, string eventCode, string prompt, string answer, int weight, bool isActive)
    {
        if (eventCode.Trim().Equals("SPARTY", StringComparison.OrdinalIgnoreCase) ||
            eventCode.Trim().Equals("SSOLO", StringComparison.OrdinalIgnoreCase))
            throw new InvalidOperationException("Use the dedicated Survival event page for this event code.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTableAsync(connection, EventObjectName("_AutoEventRoundContent"));

        var sql = contentId > 0
            ? $"UPDATE {EventTable("_AutoEventRoundContent")} SET EventCode = @EventCode, IsActive = @IsActive, Prompt = @Prompt, Answer = @Answer, Weight = @Weight WHERE ContentID = @ContentID"
            : $"INSERT INTO {EventTable("_AutoEventRoundContent")} (EventCode, IsActive, Prompt, Answer, Weight) VALUES (@EventCode, @IsActive, @Prompt, @Answer, @Weight)";

        await using var command = new SqlCommand(sql, connection);
        command.Parameters.Add("@ContentID", SqlDbType.Int).Value = contentId;
        command.Parameters.Add("@EventCode", SqlDbType.NVarChar, 32).Value = eventCode.Trim();
        command.Parameters.Add("@IsActive", SqlDbType.Bit).Value = isActive;
        command.Parameters.Add("@Prompt", SqlDbType.NVarChar, 512).Value = prompt.Trim();
        command.Parameters.Add("@Answer", SqlDbType.NVarChar, 256).Value = answer.Trim();
        command.Parameters.Add("@Weight", SqlDbType.Int).Value = weight;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, contentId > 0 ? "UpdateAutoEventContent" : "AddAutoEventContent", eventCode, prompt);
    }

    private static async Task EnsureSurvivalPartySchemaAsync(SqlConnection connection)
    {
        await using var command = new SqlCommand(@"
IF DB_ID(N'Events') IS NULL
    EXEC(N'CREATE DATABASE [Events]');

IF OBJECT_ID(N'Events.dbo._SurvivalPartyConfig', N'U') IS NULL
BEGIN
    CREATE TABLE Events.dbo._SurvivalPartyConfig
    (
        EventCode nvarchar(32) NOT NULL CONSTRAINT PK_SurvivalPartyConfig PRIMARY KEY,
        DisplayName nvarchar(64) NOT NULL,
        Enabled bit NOT NULL CONSTRAINT DF_SurvivalPartyConfig_Enabled DEFAULT (1),
        EventID int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_EventID DEFAULT (12),
        StartDelaySeconds int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_StartDelay DEFAULT (60),
        RegistrationSeconds int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_Registration DEFAULT (60),
        FightSeconds int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_Fight DEFAULT (600),
        MinLevel int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_MinLevel DEFAULT (1),
        HwidLimit int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_HwidLimit DEFAULT (1),
        RequireHwid bit NOT NULL CONSTRAINT DF_SurvivalPartyConfig_RequireHwid DEFAULT (1),
        MaxPlayers int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_MaxPlayers DEFAULT (100),
        ArenaWorldID int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_ArenaWorld DEFAULT (107),
        ArenaRegionID int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_ArenaRegion DEFAULT (25580),
        ArenaX int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_ArenaX DEFAULT (500),
        ArenaY int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_ArenaY DEFAULT (0),
        ArenaZ int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_ArenaZ DEFAULT (500),
        UpdatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_SurvivalPartyConfig_Updated DEFAULT (SYSUTCDATETIME())
    );
END;

IF COL_LENGTH(N'Events.dbo._SurvivalPartyConfig', N'DisplayName') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyConfig ADD DisplayName nvarchar(64) NOT NULL DEFAULT (N'Survival Party') WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyConfig', N'Enabled') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyConfig ADD Enabled bit NOT NULL DEFAULT (1) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyConfig', N'EventID') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyConfig ADD EventID int NOT NULL DEFAULT (12) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyConfig', N'StartDelaySeconds') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyConfig ADD StartDelaySeconds int NOT NULL DEFAULT (60) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyConfig', N'RegistrationSeconds') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyConfig ADD RegistrationSeconds int NOT NULL DEFAULT (60) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyConfig', N'FightSeconds') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyConfig ADD FightSeconds int NOT NULL DEFAULT (600) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyConfig', N'MinLevel') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyConfig ADD MinLevel int NOT NULL DEFAULT (1) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyConfig', N'HwidLimit') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyConfig ADD HwidLimit int NOT NULL DEFAULT (1) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyConfig', N'RequireHwid') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyConfig ADD RequireHwid bit NOT NULL DEFAULT (1) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyConfig', N'MaxPlayers') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyConfig ADD MaxPlayers int NOT NULL DEFAULT (100) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyConfig', N'ArenaWorldID') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyConfig ADD ArenaWorldID int NOT NULL DEFAULT (107) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyConfig', N'ArenaRegionID') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyConfig ADD ArenaRegionID int NOT NULL DEFAULT (25580) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyConfig', N'ArenaX') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyConfig ADD ArenaX int NOT NULL DEFAULT (500) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyConfig', N'ArenaY') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyConfig ADD ArenaY int NOT NULL DEFAULT (0) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyConfig', N'ArenaZ') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyConfig ADD ArenaZ int NOT NULL DEFAULT (500) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyConfig', N'UpdatedAtUtc') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyConfig ADD UpdatedAtUtc datetime2(0) NOT NULL DEFAULT (SYSUTCDATETIME()) WITH VALUES;

IF NOT EXISTS (SELECT 1 FROM Events.dbo._SurvivalPartyConfig WITH (NOLOCK) WHERE EventCode = N'SPARTY')
BEGIN
    INSERT INTO Events.dbo._SurvivalPartyConfig
        (EventCode, DisplayName, Enabled, EventID, StartDelaySeconds, RegistrationSeconds, FightSeconds,
         MinLevel, HwidLimit, RequireHwid, MaxPlayers, ArenaWorldID, ArenaRegionID, ArenaX, ArenaY, ArenaZ)
    VALUES
        (N'SPARTY', N'Survival Party', 1, 12, 60, 60, 600,
         1, 1, 1, 100, 107, 25580, 500, 0, 500);
END;

IF OBJECT_ID(N'Events.dbo._SurvivalPartyReward', N'U') IS NULL
BEGIN
    CREATE TABLE Events.dbo._SurvivalPartyReward
    (
        RewardID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_SurvivalPartyReward PRIMARY KEY,
        Placement int NOT NULL,
        RewardType nvarchar(24) NOT NULL,
        Amount bigint NOT NULL CONSTRAINT DF_SurvivalPartyReward_Amount DEFAULT (0),
        ItemCodeName128 varchar(128) NULL,
        ItemID int NULL,
        ItemCount int NOT NULL CONSTRAINT DF_SurvivalPartyReward_ItemCount DEFAULT (1),
        Plus int NOT NULL CONSTRAINT DF_SurvivalPartyReward_Plus DEFAULT (0),
        IsActive bit NOT NULL CONSTRAINT DF_SurvivalPartyReward_Active DEFAULT (1),
        CreatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_SurvivalPartyReward_Created DEFAULT (SYSUTCDATETIME())
    );
END;

IF COL_LENGTH(N'Events.dbo._SurvivalPartyReward', N'Placement') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyReward ADD Placement int NOT NULL DEFAULT (1) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyReward', N'RewardType') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyReward ADD RewardType nvarchar(24) NOT NULL DEFAULT (N'SilkOwn') WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyReward', N'Amount') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyReward ADD Amount bigint NOT NULL DEFAULT (0) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyReward', N'ItemID') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyReward ADD ItemID int NULL;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyReward', N'ItemCodeName128') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyReward ADD ItemCodeName128 varchar(128) NULL;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyReward', N'ItemCount') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyReward ADD ItemCount int NOT NULL DEFAULT (1) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyReward', N'Plus') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyReward ADD Plus int NOT NULL DEFAULT (0) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyReward', N'IsActive') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyReward ADD IsActive bit NOT NULL DEFAULT (1) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartyReward', N'CreatedAtUtc') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartyReward ADD CreatedAtUtc datetime2(0) NOT NULL DEFAULT (SYSUTCDATETIME()) WITH VALUES;

IF NOT EXISTS (SELECT 1 FROM Events.dbo._SurvivalPartyReward WITH (NOLOCK) WHERE Placement = 1 AND IsActive = 1)
    INSERT INTO Events.dbo._SurvivalPartyReward (Placement, RewardType, Amount, ItemCodeName128, ItemCount, Plus)
    VALUES (1, N'SilkOwn', 500, NULL, 1, 0);

IF NOT EXISTS (SELECT 1 FROM Events.dbo._SurvivalPartyReward WITH (NOLOCK) WHERE Placement = 2 AND IsActive = 1)
    INSERT INTO Events.dbo._SurvivalPartyReward (Placement, RewardType, Amount, ItemCodeName128, ItemCount, Plus)
    VALUES (2, N'SilkOwn', 50, NULL, 1, 0);

IF OBJECT_ID(N'Events.dbo._SurvivalPartySchedule', N'U') IS NULL
BEGIN
    CREATE TABLE Events.dbo._SurvivalPartySchedule
    (
        ScheduleID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_SurvivalPartySchedule PRIMARY KEY,
        StartTime time(0) NOT NULL,
        DaysMask tinyint NOT NULL CONSTRAINT DF_SurvivalPartySchedule_DaysMask DEFAULT (127),
        IsActive bit NOT NULL CONSTRAINT DF_SurvivalPartySchedule_Active DEFAULT (1),
        LastRunLocalDate date NULL,
        UpdatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_SurvivalPartySchedule_Updated DEFAULT (SYSUTCDATETIME())
    );
END;

IF COL_LENGTH(N'Events.dbo._SurvivalPartySchedule', N'StartTime') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartySchedule ADD StartTime time(0) NOT NULL DEFAULT ('20:00') WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartySchedule', N'DaysMask') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartySchedule ADD DaysMask tinyint NOT NULL DEFAULT (127) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartySchedule', N'IsActive') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartySchedule ADD IsActive bit NOT NULL DEFAULT (1) WITH VALUES;
IF COL_LENGTH(N'Events.dbo._SurvivalPartySchedule', N'LastRunLocalDate') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartySchedule ADD LastRunLocalDate date NULL;
IF COL_LENGTH(N'Events.dbo._SurvivalPartySchedule', N'UpdatedAtUtc') IS NULL
    ALTER TABLE Events.dbo._SurvivalPartySchedule ADD UpdatedAtUtc datetime2(0) NOT NULL DEFAULT (SYSUTCDATETIME()) WITH VALUES;
", connection);
        await command.ExecuteNonQueryAsync();
    }

    public async Task<DataTable> LoadSurvivalPartyConfigAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSurvivalPartySchemaAsync(connection);
        await EnsureFilterRegionControlTableAsync(connection);
        return await LoadOptionalTableAsync(connection, EventObjectName("_SurvivalPartyConfig"), $@"
SELECT EventCode, DisplayName, Enabled, EventID, StartDelaySeconds, RegistrationSeconds, FightSeconds,
       MinLevel, HwidLimit, RequireHwid, MaxPlayers, ArenaWorldID, ArenaRegionID, ArenaX, ArenaY, ArenaZ, UpdatedAtUtc
FROM {EventTable("_SurvivalPartyConfig")} WITH (NOLOCK)
WHERE EventCode = N'SPARTY';", string.Empty);
    }

    public async Task SaveSurvivalPartyConfigAsync(
        string displayName,
        bool enabled,
        int eventId,
        int startDelaySeconds,
        int registrationSeconds,
        int fightSeconds,
        int minLevel,
        int hwidLimit,
        bool requireHwid,
        int maxPlayers,
        int arenaWorldId,
        int arenaRegionId,
        int arenaX,
        int arenaY,
        int arenaZ)
    {
        arenaRegionId = NormalizeRegionId(arenaRegionId);
        if (arenaWorldId <= 0 || !IsValidRegionId(arenaRegionId))
            throw new InvalidOperationException("Arena World ID must be positive and Region ID must be a non-zero signed 16-bit value.");
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSurvivalPartySchemaAsync(connection);
        await EnsureFilterRegionControlTableAsync(connection);

        await using (var idCheck = new SqlCommand(@"
IF OBJECT_ID(N'dbo.Event_RegisterSettings', N'U') IS NOT NULL
    SELECT TOP (1) Name FROM dbo.Event_RegisterSettings WITH (NOLOCK) WHERE ID = @EventID;", connection))
        {
            idCheck.Parameters.Add("@EventID", SqlDbType.Int).Value = Math.Clamp(eventId, 1, 255);
            var existingName = Convert.ToString(await idCheck.ExecuteScalarAsync());
            if (!string.IsNullOrWhiteSpace(existingName) &&
                !existingName.Equals("Survival Party", StringComparison.OrdinalIgnoreCase))
                throw new InvalidOperationException($"Event ID {eventId} is already used by '{existingName}'. Choose a different Event ID.");
        }

        await using var command = new SqlCommand($@"
MERGE {EventTable("_SurvivalPartyConfig")} AS target
USING (SELECT N'SPARTY' AS EventCode) AS source
ON target.EventCode = source.EventCode
WHEN MATCHED THEN
    UPDATE SET DisplayName = @DisplayName,
               Enabled = @Enabled,
               EventID = @EventID,
               StartDelaySeconds = @StartDelaySeconds,
               RegistrationSeconds = @RegistrationSeconds,
               FightSeconds = @FightSeconds,
               MinLevel = @MinLevel,
               HwidLimit = @HwidLimit,
               RequireHwid = @RequireHwid,
               MaxPlayers = @MaxPlayers,
               ArenaWorldID = @ArenaWorldID,
               ArenaRegionID = @ArenaRegionID,
               ArenaX = @ArenaX,
               ArenaY = @ArenaY,
               ArenaZ = @ArenaZ,
               UpdatedAtUtc = SYSUTCDATETIME()
WHEN NOT MATCHED THEN
    INSERT (EventCode, DisplayName, Enabled, EventID, StartDelaySeconds, RegistrationSeconds, FightSeconds,
            MinLevel, HwidLimit, RequireHwid, MaxPlayers, ArenaWorldID, ArenaRegionID, ArenaX, ArenaY, ArenaZ)
    VALUES (N'SPARTY', @DisplayName, @Enabled, @EventID, @StartDelaySeconds, @RegistrationSeconds, @FightSeconds,
            @MinLevel, @HwidLimit, @RequireHwid, @MaxPlayers, @ArenaWorldID, @ArenaRegionID, @ArenaX, @ArenaY, @ArenaZ);", connection);

        command.Parameters.Add("@DisplayName", SqlDbType.NVarChar, 64).Value = string.IsNullOrWhiteSpace(displayName) ? "Survival Party" : displayName.Trim();
        command.Parameters.Add("@Enabled", SqlDbType.Bit).Value = enabled;
        command.Parameters.Add("@EventID", SqlDbType.Int).Value = Math.Clamp(eventId, 1, 255);
        command.Parameters.Add("@StartDelaySeconds", SqlDbType.Int).Value = Math.Clamp(startDelaySeconds, 0, 3600);
        command.Parameters.Add("@RegistrationSeconds", SqlDbType.Int).Value = Math.Clamp(registrationSeconds, 0, 3600);
        command.Parameters.Add("@FightSeconds", SqlDbType.Int).Value = Math.Clamp(fightSeconds, 30, 7200);
        command.Parameters.Add("@MinLevel", SqlDbType.Int).Value = Math.Max(0, minLevel);
        command.Parameters.Add("@HwidLimit", SqlDbType.Int).Value = Math.Clamp(hwidLimit, 0, 32);
        command.Parameters.Add("@RequireHwid", SqlDbType.Bit).Value = requireHwid;
        command.Parameters.Add("@MaxPlayers", SqlDbType.Int).Value = Math.Clamp(maxPlayers, 2, 100);
        command.Parameters.Add("@ArenaWorldID", SqlDbType.Int).Value = Math.Max(1, arenaWorldId);
        command.Parameters.Add("@ArenaRegionID", SqlDbType.Int).Value = arenaRegionId;
        command.Parameters.Add("@ArenaX", SqlDbType.Int).Value = arenaX;
        command.Parameters.Add("@ArenaY", SqlDbType.Int).Value = arenaY;
        command.Parameters.Add("@ArenaZ", SqlDbType.Int).Value = arenaZ;
        await command.ExecuteNonQueryAsync();

        await using var registrationCommand = new SqlCommand(@"
IF OBJECT_ID(N'dbo.Event_RegisterSettings', N'U') IS NOT NULL
BEGIN
    DELETE FROM dbo.Event_RegisterSettings WHERE Name = N'Survival Party' AND ID <> @EventID;
    MERGE dbo.Event_RegisterSettings AS target
    USING (SELECT @EventID AS ID) AS source
    ON target.ID = source.ID
    WHEN MATCHED THEN UPDATE SET Name=N'Survival Party', Description=N'Party survival arena event'
    WHEN NOT MATCHED THEN INSERT (ID, Name, Description) VALUES (@EventID, N'Survival Party', N'Party survival arena event');
END;", connection);
        registrationCommand.Parameters.Add("@EventID", SqlDbType.Int).Value = Math.Clamp(eventId, 1, 255);
        await registrationCommand.ExecuteNonQueryAsync();
        await UpsertEventRegionFeaturesAsync(
            connection,
            null,
            "SPARTY",
            Math.Max(1, arenaWorldId),
            arenaRegionId,
            teams: true,
            allowParty: true);
        await AuditAsync(connection, "SaveSurvivalPartyConfig", "SPARTY", displayName);
    }

    public async Task<DataTable> LoadSurvivalPartyRewardsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSurvivalPartySchemaAsync(connection);
        return await LoadOptionalTableAsync(connection, EventObjectName("_SurvivalPartyReward"), $@"
SELECT RewardID, Placement, RewardType, Amount, ItemCodeName128, ItemID, ItemCount, Plus, IsActive, CreatedAtUtc
FROM {EventTable("_SurvivalPartyReward")} WITH (NOLOCK)
ORDER BY Placement, IsActive DESC, RewardID DESC;", string.Empty);
    }

    public async Task SaveSurvivalPartyRewardAsync(int rewardId, int placement, string rewardType, long amount, string itemCodeName, int? itemId, int itemCount, int plus, bool isActive)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSurvivalPartySchemaAsync(connection);

        var sql = rewardId > 0
            ? $@"UPDATE {EventTable("_SurvivalPartyReward")} SET IsActive = 0 WHERE Placement = @Placement;
                 UPDATE {EventTable("_SurvivalPartyReward")} SET Placement = @Placement, RewardType = @RewardType, Amount = @Amount, ItemCodeName128 = @ItemCodeName128, ItemID = @ItemID, ItemCount = @ItemCount, Plus = @Plus, IsActive = @IsActive WHERE RewardID = @RewardID"
            : $@"UPDATE {EventTable("_SurvivalPartyReward")} SET IsActive = 0 WHERE Placement = @Placement;
                 INSERT INTO {EventTable("_SurvivalPartyReward")} (Placement, RewardType, Amount, ItemCodeName128, ItemID, ItemCount, Plus, IsActive) VALUES (@Placement, @RewardType, @Amount, @ItemCodeName128, @ItemID, @ItemCount, @Plus, @IsActive)";

        await using var command = new SqlCommand(sql, connection);
        command.Parameters.Add("@RewardID", SqlDbType.Int).Value = rewardId;
        command.Parameters.Add("@Placement", SqlDbType.Int).Value = placement;
        command.Parameters.Add("@RewardType", SqlDbType.NVarChar, 24).Value = rewardType.Trim();
        command.Parameters.Add("@Amount", SqlDbType.BigInt).Value = amount;
        command.Parameters.Add("@ItemCodeName128", SqlDbType.VarChar, 128).Value = string.IsNullOrWhiteSpace(itemCodeName) ? DBNull.Value : itemCodeName.Trim();
        command.Parameters.Add("@ItemID", SqlDbType.Int).Value = itemId.HasValue && itemId.Value > 0 ? itemId.Value : DBNull.Value;
        command.Parameters.Add("@ItemCount", SqlDbType.Int).Value = itemCount;
        command.Parameters.Add("@Plus", SqlDbType.Int).Value = plus;
        command.Parameters.Add("@IsActive", SqlDbType.Bit).Value = isActive;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, rewardId > 0 ? "UpdateSurvivalPartyReward" : "AddSurvivalPartyReward", "SPARTY", $"{rewardType};Placement={placement};Amount={amount}");
    }

    public async Task<DataTable> LoadSurvivalPartySchedulesAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSurvivalPartySchemaAsync(connection);
        return await LoadOptionalTableAsync(connection, EventObjectName("_SurvivalPartySchedule"), $@"
SELECT ScheduleID,
       CONVERT(varchar(8), StartTime, 108) AS StartTime,
       DaysMask,
       STUFF(
           CASE WHEN (DaysMask & 1) = 1 THEN N', Sun' ELSE N'' END +
           CASE WHEN (DaysMask & 2) = 2 THEN N', Mon' ELSE N'' END +
           CASE WHEN (DaysMask & 4) = 4 THEN N', Tue' ELSE N'' END +
           CASE WHEN (DaysMask & 8) = 8 THEN N', Wed' ELSE N'' END +
           CASE WHEN (DaysMask & 16) = 16 THEN N', Thu' ELSE N'' END +
           CASE WHEN (DaysMask & 32) = 32 THEN N', Fri' ELSE N'' END +
           CASE WHEN (DaysMask & 64) = 64 THEN N', Sat' ELSE N'' END,
           1, 2, N'') AS Days,
       IsActive, LastRunLocalDate, UpdatedAtUtc
FROM {EventTable("_SurvivalPartySchedule")} WITH (NOLOCK)
ORDER BY StartTime, ScheduleID;", string.Empty);
    }

    public async Task SaveSurvivalPartyScheduleAsync(int scheduleId, TimeSpan startTime, int daysMask, bool isActive)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSurvivalPartySchemaAsync(connection);

        var sql = scheduleId > 0
            ? $@"UPDATE {EventTable("_SurvivalPartySchedule")}
                 SET StartTime = @StartTime, DaysMask = @DaysMask, IsActive = @IsActive,
                     LastRunLocalDate = NULL, UpdatedAtUtc = SYSUTCDATETIME()
                 WHERE ScheduleID = @ScheduleID"
            : $@"INSERT INTO {EventTable("_SurvivalPartySchedule")} (StartTime, DaysMask, IsActive)
                 VALUES (@StartTime, @DaysMask, @IsActive)";

        await using var command = new SqlCommand(sql, connection);
        command.Parameters.Add("@ScheduleID", SqlDbType.Int).Value = scheduleId;
        command.Parameters.Add("@StartTime", SqlDbType.Time).Value = startTime;
        command.Parameters.Add("@DaysMask", SqlDbType.TinyInt).Value = Math.Clamp(daysMask, 1, 127);
        command.Parameters.Add("@IsActive", SqlDbType.Bit).Value = isActive;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, scheduleId > 0 ? "UpdateSurvivalPartySchedule" : "AddSurvivalPartySchedule", "SPARTY", $"{startTime:hh\\:mm};DaysMask={daysMask};Active={isActive}");
    }

    public async Task DeleteSurvivalPartyScheduleAsync(int scheduleId)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSurvivalPartySchemaAsync(connection);
        await using var command = new SqlCommand($"DELETE FROM {EventTable("_SurvivalPartySchedule")} WHERE ScheduleID = @ScheduleID", connection);
        command.Parameters.Add("@ScheduleID", SqlDbType.Int).Value = scheduleId;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "DeleteSurvivalPartySchedule", "SPARTY", scheduleId.ToString());
    }

    private static async Task EnsureSurvivalSoloSchemaAsync(SqlConnection connection)
    {
        await using var command = new SqlCommand(@"
IF DB_ID(N'Events') IS NULL
    EXEC(N'CREATE DATABASE [Events]');

IF OBJECT_ID(N'Events.dbo._SurvivalSoloConfig', N'U') IS NULL
BEGIN
    CREATE TABLE Events.dbo._SurvivalSoloConfig
    (
        EventCode nvarchar(32) NOT NULL CONSTRAINT PK_SurvivalSoloConfig PRIMARY KEY,
        DisplayName nvarchar(64) NOT NULL,
        Enabled bit NOT NULL CONSTRAINT DF_SurvivalSoloConfig_Enabled DEFAULT (1),
        EventID int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_EventID DEFAULT (13),
        StartDelaySeconds int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_StartDelay DEFAULT (60),
        RegistrationSeconds int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_Registration DEFAULT (60),
        FightSeconds int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_Fight DEFAULT (600),
        MinLevel int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_MinLevel DEFAULT (1),
        HwidLimit int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_HwidLimit DEFAULT (1),
        RequireHwid bit NOT NULL CONSTRAINT DF_SurvivalSoloConfig_RequireHwid DEFAULT (1),
        MaxPlayers int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_MaxPlayers DEFAULT (100),
        ArenaWorldID int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_ArenaWorld DEFAULT (107),
        ArenaRegionID int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_ArenaRegion DEFAULT (25580),
        ArenaX int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_ArenaX DEFAULT (500),
        ArenaY int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_ArenaY DEFAULT (0),
        ArenaZ int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_ArenaZ DEFAULT (500),
        UpdatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_SurvivalSoloConfig_Updated DEFAULT (SYSUTCDATETIME())
    );
END;

IF NOT EXISTS (SELECT 1 FROM Events.dbo._SurvivalSoloConfig WITH (NOLOCK) WHERE EventCode = N'SSOLO')
BEGIN
    INSERT INTO Events.dbo._SurvivalSoloConfig
        (EventCode, DisplayName, Enabled, EventID, StartDelaySeconds, RegistrationSeconds, FightSeconds,
         MinLevel, HwidLimit, RequireHwid, MaxPlayers, ArenaWorldID, ArenaRegionID, ArenaX, ArenaY, ArenaZ)
    VALUES
        (N'SSOLO', N'Survival Solo', 1, 13, 60, 60, 600, 1, 1, 1, 100, 107, 25580, 500, 0, 500);
END;

IF OBJECT_ID(N'Events.dbo._SurvivalSoloReward', N'U') IS NULL
BEGIN
    CREATE TABLE Events.dbo._SurvivalSoloReward
    (
        RewardID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_SurvivalSoloReward PRIMARY KEY,
        Placement int NOT NULL,
        RewardType nvarchar(24) NOT NULL,
        Amount bigint NOT NULL CONSTRAINT DF_SurvivalSoloReward_Amount DEFAULT (0),
        ItemCodeName128 varchar(128) NULL,
        ItemID int NULL,
        ItemCount int NOT NULL CONSTRAINT DF_SurvivalSoloReward_ItemCount DEFAULT (1),
        Plus int NOT NULL CONSTRAINT DF_SurvivalSoloReward_Plus DEFAULT (0),
        IsActive bit NOT NULL CONSTRAINT DF_SurvivalSoloReward_Active DEFAULT (1),
        CreatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_SurvivalSoloReward_Created DEFAULT (SYSUTCDATETIME())
    );
END;

IF NOT EXISTS (SELECT 1 FROM Events.dbo._SurvivalSoloReward WITH (NOLOCK) WHERE Placement = 1 AND IsActive = 1)
    INSERT INTO Events.dbo._SurvivalSoloReward (Placement, RewardType, Amount, ItemCount, Plus)
    VALUES (1, N'SilkOwn', 500, 1, 0);

IF NOT EXISTS (SELECT 1 FROM Events.dbo._SurvivalSoloReward WITH (NOLOCK) WHERE Placement = 2 AND IsActive = 1)
    INSERT INTO Events.dbo._SurvivalSoloReward (Placement, RewardType, Amount, ItemCount, Plus)
    VALUES (2, N'SilkOwn', 50, 1, 0);

IF OBJECT_ID(N'Events.dbo._SurvivalSoloSchedule', N'U') IS NULL
BEGIN
    CREATE TABLE Events.dbo._SurvivalSoloSchedule
    (
        ScheduleID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_SurvivalSoloSchedule PRIMARY KEY,
        StartTime time(0) NOT NULL,
        DaysMask tinyint NOT NULL CONSTRAINT DF_SurvivalSoloSchedule_DaysMask DEFAULT (127),
        IsActive bit NOT NULL CONSTRAINT DF_SurvivalSoloSchedule_Active DEFAULT (1),
        LastRunLocalDate date NULL,
        UpdatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_SurvivalSoloSchedule_Updated DEFAULT (SYSUTCDATETIME())
    );
END;", connection);
        await command.ExecuteNonQueryAsync();
    }

    public async Task<DataTable> LoadSurvivalSoloConfigAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSurvivalSoloSchemaAsync(connection);
        await EnsureFilterRegionControlTableAsync(connection);
        return await LoadOptionalTableAsync(connection, EventObjectName("_SurvivalSoloConfig"), $@"
SELECT EventCode, DisplayName, Enabled, EventID, StartDelaySeconds, RegistrationSeconds, FightSeconds,
       MinLevel, HwidLimit, RequireHwid, MaxPlayers, ArenaWorldID, ArenaRegionID, ArenaX, ArenaY, ArenaZ, UpdatedAtUtc
FROM {EventTable("_SurvivalSoloConfig")} WITH (NOLOCK)
WHERE EventCode = N'SSOLO';", string.Empty);
    }

    public async Task SaveSurvivalSoloConfigAsync(
        string displayName, bool enabled, int eventId, int startDelaySeconds,
        int registrationSeconds, int fightSeconds, int minLevel, int hwidLimit,
        bool requireHwid, int maxPlayers, int arenaWorldId, int arenaRegionId,
        int arenaX, int arenaY, int arenaZ)
    {
        arenaRegionId = NormalizeRegionId(arenaRegionId);
        if (arenaWorldId <= 0 || !IsValidRegionId(arenaRegionId))
            throw new InvalidOperationException("Arena World ID must be positive and Region ID must be a non-zero signed 16-bit value.");
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSurvivalSoloSchemaAsync(connection);
        await EnsureFilterRegionControlTableAsync(connection);

        await using (var idCheck = new SqlCommand(@"
IF OBJECT_ID(N'dbo.Event_RegisterSettings', N'U') IS NOT NULL
    SELECT TOP (1) Name FROM dbo.Event_RegisterSettings WITH (NOLOCK) WHERE ID = @EventID;", connection))
        {
            idCheck.Parameters.Add("@EventID", SqlDbType.Int).Value = Math.Clamp(eventId, 1, 255);
            var existingName = Convert.ToString(await idCheck.ExecuteScalarAsync());
            if (!string.IsNullOrWhiteSpace(existingName) &&
                !existingName.Equals("Survival Solo", StringComparison.OrdinalIgnoreCase))
                throw new InvalidOperationException($"Event ID {eventId} is already used by '{existingName}'. Choose a different Event ID.");
        }

        await using var command = new SqlCommand($@"
MERGE {EventTable("_SurvivalSoloConfig")} AS target
USING (SELECT N'SSOLO' AS EventCode) AS source
ON target.EventCode = source.EventCode
WHEN MATCHED THEN
    UPDATE SET DisplayName=@DisplayName, Enabled=@Enabled, EventID=@EventID,
               StartDelaySeconds=@StartDelaySeconds, RegistrationSeconds=@RegistrationSeconds,
               FightSeconds=@FightSeconds, MinLevel=@MinLevel, HwidLimit=@HwidLimit,
               RequireHwid=@RequireHwid, MaxPlayers=@MaxPlayers, ArenaWorldID=@ArenaWorldID,
               ArenaRegionID=@ArenaRegionID, ArenaX=@ArenaX, ArenaY=@ArenaY, ArenaZ=@ArenaZ,
               UpdatedAtUtc=SYSUTCDATETIME()
WHEN NOT MATCHED THEN
    INSERT (EventCode, DisplayName, Enabled, EventID, StartDelaySeconds, RegistrationSeconds, FightSeconds,
            MinLevel, HwidLimit, RequireHwid, MaxPlayers, ArenaWorldID, ArenaRegionID, ArenaX, ArenaY, ArenaZ)
    VALUES (N'SSOLO', @DisplayName, @Enabled, @EventID, @StartDelaySeconds, @RegistrationSeconds, @FightSeconds,
            @MinLevel, @HwidLimit, @RequireHwid, @MaxPlayers, @ArenaWorldID, @ArenaRegionID, @ArenaX, @ArenaY, @ArenaZ);", connection);
        command.Parameters.Add("@DisplayName", SqlDbType.NVarChar, 64).Value = string.IsNullOrWhiteSpace(displayName) ? "Survival Solo" : displayName.Trim();
        command.Parameters.Add("@Enabled", SqlDbType.Bit).Value = enabled;
        command.Parameters.Add("@EventID", SqlDbType.Int).Value = Math.Clamp(eventId, 1, 255);
        command.Parameters.Add("@StartDelaySeconds", SqlDbType.Int).Value = Math.Clamp(startDelaySeconds, 0, 3600);
        command.Parameters.Add("@RegistrationSeconds", SqlDbType.Int).Value = Math.Clamp(registrationSeconds, 0, 3600);
        command.Parameters.Add("@FightSeconds", SqlDbType.Int).Value = Math.Clamp(fightSeconds, 30, 7200);
        command.Parameters.Add("@MinLevel", SqlDbType.Int).Value = Math.Max(0, minLevel);
        command.Parameters.Add("@HwidLimit", SqlDbType.Int).Value = Math.Clamp(hwidLimit, 0, 32);
        command.Parameters.Add("@RequireHwid", SqlDbType.Bit).Value = requireHwid;
        command.Parameters.Add("@MaxPlayers", SqlDbType.Int).Value = Math.Clamp(maxPlayers, 2, 100);
        command.Parameters.Add("@ArenaWorldID", SqlDbType.Int).Value = Math.Max(1, arenaWorldId);
        command.Parameters.Add("@ArenaRegionID", SqlDbType.Int).Value = arenaRegionId;
        command.Parameters.Add("@ArenaX", SqlDbType.Int).Value = arenaX;
        command.Parameters.Add("@ArenaY", SqlDbType.Int).Value = arenaY;
        command.Parameters.Add("@ArenaZ", SqlDbType.Int).Value = arenaZ;
        await command.ExecuteNonQueryAsync();

        await using var registrationCommand = new SqlCommand(@"
IF OBJECT_ID(N'dbo.Event_RegisterSettings', N'U') IS NOT NULL
BEGIN
    DELETE FROM dbo.Event_RegisterSettings WHERE Name = N'Survival Solo' AND ID <> @EventID;
    MERGE dbo.Event_RegisterSettings AS target
    USING (SELECT @EventID AS ID) AS source
    ON target.ID = source.ID
    WHEN MATCHED THEN UPDATE SET Name=N'Survival Solo', Description=N'Individual free-for-all survival arena event'
    WHEN NOT MATCHED THEN INSERT (ID, Name, Description) VALUES (@EventID, N'Survival Solo', N'Individual free-for-all survival arena event');
END;", connection);
        registrationCommand.Parameters.Add("@EventID", SqlDbType.Int).Value = Math.Clamp(eventId, 1, 255);
        await registrationCommand.ExecuteNonQueryAsync();
        await UpsertEventRegionFeaturesAsync(
            connection,
            null,
            "SSOLO",
            Math.Max(1, arenaWorldId),
            arenaRegionId,
            teams: false,
            allowParty: false);
        await AuditAsync(connection, "SaveSurvivalSoloConfig", "SSOLO", displayName);
    }

    public async Task<DataTable> LoadSurvivalSoloRewardsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSurvivalSoloSchemaAsync(connection);
        return await LoadOptionalTableAsync(connection, EventObjectName("_SurvivalSoloReward"), $@"
SELECT RewardID, Placement, RewardType, Amount, ItemCodeName128, ItemID, ItemCount, Plus, IsActive, CreatedAtUtc
FROM {EventTable("_SurvivalSoloReward")} WITH (NOLOCK)
ORDER BY Placement, IsActive DESC, RewardID DESC;", string.Empty);
    }

    public async Task SaveSurvivalSoloRewardAsync(int rewardId, int placement, string rewardType, long amount, string itemCodeName, int? itemId, int itemCount, int plus, bool isActive)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSurvivalSoloSchemaAsync(connection);
        var sql = rewardId > 0
            ? $@"UPDATE {EventTable("_SurvivalSoloReward")} SET IsActive=0 WHERE Placement=@Placement;
                 UPDATE {EventTable("_SurvivalSoloReward")} SET Placement=@Placement, RewardType=@RewardType, Amount=@Amount, ItemCodeName128=@ItemCodeName128, ItemID=@ItemID, ItemCount=@ItemCount, Plus=@Plus, IsActive=@IsActive WHERE RewardID=@RewardID"
            : $@"UPDATE {EventTable("_SurvivalSoloReward")} SET IsActive=0 WHERE Placement=@Placement;
                 INSERT INTO {EventTable("_SurvivalSoloReward")} (Placement, RewardType, Amount, ItemCodeName128, ItemID, ItemCount, Plus, IsActive) VALUES (@Placement, @RewardType, @Amount, @ItemCodeName128, @ItemID, @ItemCount, @Plus, @IsActive)";
        await using var command = new SqlCommand(sql, connection);
        command.Parameters.Add("@RewardID", SqlDbType.Int).Value = rewardId;
        command.Parameters.Add("@Placement", SqlDbType.Int).Value = placement;
        command.Parameters.Add("@RewardType", SqlDbType.NVarChar, 24).Value = rewardType.Trim();
        command.Parameters.Add("@Amount", SqlDbType.BigInt).Value = amount;
        command.Parameters.Add("@ItemCodeName128", SqlDbType.VarChar, 128).Value = string.IsNullOrWhiteSpace(itemCodeName) ? DBNull.Value : itemCodeName.Trim();
        command.Parameters.Add("@ItemID", SqlDbType.Int).Value = itemId.HasValue && itemId.Value > 0 ? itemId.Value : DBNull.Value;
        command.Parameters.Add("@ItemCount", SqlDbType.Int).Value = Math.Max(1, itemCount);
        command.Parameters.Add("@Plus", SqlDbType.Int).Value = Math.Max(0, plus);
        command.Parameters.Add("@IsActive", SqlDbType.Bit).Value = isActive;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "SaveSurvivalSoloReward", "SSOLO", $"Placement={placement};Type={rewardType};Amount={amount}");
    }

    public async Task<DataTable> LoadSurvivalSoloSchedulesAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSurvivalSoloSchemaAsync(connection);
        return await LoadOptionalTableAsync(connection, EventObjectName("_SurvivalSoloSchedule"), $@"
SELECT ScheduleID, CONVERT(varchar(8), StartTime, 108) AS StartTime, DaysMask,
       STUFF(
           CASE WHEN (DaysMask & 1) = 1 THEN N', Sun' ELSE N'' END +
           CASE WHEN (DaysMask & 2) = 2 THEN N', Mon' ELSE N'' END +
           CASE WHEN (DaysMask & 4) = 4 THEN N', Tue' ELSE N'' END +
           CASE WHEN (DaysMask & 8) = 8 THEN N', Wed' ELSE N'' END +
           CASE WHEN (DaysMask & 16) = 16 THEN N', Thu' ELSE N'' END +
           CASE WHEN (DaysMask & 32) = 32 THEN N', Fri' ELSE N'' END +
           CASE WHEN (DaysMask & 64) = 64 THEN N', Sat' ELSE N'' END,
           1, 2, N'') AS Days,
       IsActive, LastRunLocalDate
FROM {EventTable("_SurvivalSoloSchedule")} WITH (NOLOCK)
ORDER BY StartTime, ScheduleID;", string.Empty);
    }

    public async Task SaveSurvivalSoloScheduleAsync(int scheduleId, TimeSpan startTime, int daysMask, bool isActive)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSurvivalSoloSchemaAsync(connection);
        var sql = scheduleId > 0
            ? $@"UPDATE {EventTable("_SurvivalSoloSchedule")} SET StartTime=@StartTime, DaysMask=@DaysMask, IsActive=@IsActive, LastRunLocalDate=NULL, UpdatedAtUtc=SYSUTCDATETIME() WHERE ScheduleID=@ScheduleID"
            : $@"INSERT INTO {EventTable("_SurvivalSoloSchedule")} (StartTime, DaysMask, IsActive) VALUES (@StartTime, @DaysMask, @IsActive)";
        await using var command = new SqlCommand(sql, connection);
        command.Parameters.Add("@ScheduleID", SqlDbType.Int).Value = scheduleId;
        command.Parameters.Add("@StartTime", SqlDbType.Time).Value = startTime;
        command.Parameters.Add("@DaysMask", SqlDbType.TinyInt).Value = Math.Clamp(daysMask, 0, 127);
        command.Parameters.Add("@IsActive", SqlDbType.Bit).Value = isActive;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, scheduleId > 0 ? "UpdateSurvivalSoloSchedule" : "AddSurvivalSoloSchedule", "SSOLO", $"{startTime:hh\\:mm};DaysMask={daysMask};Active={isActive}");
    }

    public async Task DeleteSurvivalSoloScheduleAsync(int scheduleId)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureSurvivalSoloSchemaAsync(connection);
        await using var command = new SqlCommand($"DELETE FROM {EventTable("_SurvivalSoloSchedule")} WHERE ScheduleID=@ScheduleID", connection);
        command.Parameters.Add("@ScheduleID", SqlDbType.Int).Value = scheduleId;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "DeleteSurvivalSoloSchedule", "SSOLO", scheduleId.ToString());
    }

    private static async Task EnsureHideAndSeekSchemaAsync(SqlConnection connection)
    {
        await using var command = new SqlCommand(@"
IF DB_ID(N'Events') IS NULL EXEC(N'CREATE DATABASE [Events]');

IF OBJECT_ID(N'dbo.Clientless_Accounts', N'U') IS NOT NULL
   AND COL_LENGTH(N'dbo.Clientless_Accounts', N'SystemRole') IS NULL
    ALTER TABLE dbo.Clientless_Accounts ADD SystemRole varchar(32) NULL;

IF OBJECT_ID(N'Events.dbo._HideAndSeekConfig', N'U') IS NULL
BEGIN
    CREATE TABLE Events.dbo._HideAndSeekConfig
    (
        EventCode nvarchar(32) NOT NULL PRIMARY KEY,
        DisplayName nvarchar(64) NOT NULL,
        Enabled bit NOT NULL DEFAULT (1),
        StartDelaySeconds int NOT NULL DEFAULT (60),
        SearchSeconds int NOT NULL DEFAULT (600),
        ReminderIntervalSeconds int NOT NULL DEFAULT (120),
        MinLevel int NOT NULL DEFAULT (1),
        HwidLimit int NOT NULL DEFAULT (1),
        RequireHwid bit NOT NULL DEFAULT (0),
        BotAccountName varchar(24) NOT NULL,
        BotPassword varchar(64) NULL,
        BotCharacterName varchar(16) NOT NULL,
        BotShardID smallint NOT NULL DEFAULT (64),
        BotLocale tinyint NOT NULL DEFAULT (22),
        ReturnWorldID int NOT NULL DEFAULT (1),
        ReturnRegionID int NOT NULL DEFAULT (25000),
        ReturnX int NOT NULL DEFAULT (982),
        ReturnY int NOT NULL DEFAULT (0),
        ReturnZ int NOT NULL DEFAULT (140),
        UpdatedAtUtc datetime2(0) NOT NULL DEFAULT (SYSUTCDATETIME())
    );
END;

IF NOT EXISTS (SELECT 1 FROM Events.dbo._HideAndSeekConfig WHERE EventCode=N'HNS')
    INSERT INTO Events.dbo._HideAndSeekConfig
        (EventCode,DisplayName,Enabled,StartDelaySeconds,SearchSeconds,ReminderIntervalSeconds,
         MinLevel,HwidLimit,RequireHwid,BotAccountName,BotCharacterName,BotShardID,BotLocale,
         ReturnWorldID,ReturnRegionID,ReturnX,ReturnY,ReturnZ)
    VALUES
        (N'HNS',N'Hide and Seek',1,60,600,120,1,1,0,'kmthnsmaster','KMT_HideMaster',
         64,22,1,25000,982,0,140);

IF OBJECT_ID(N'Events.dbo._HideAndSeekLocation', N'U') IS NULL
    CREATE TABLE Events.dbo._HideAndSeekLocation
    (
        LocationID int IDENTITY(1,1) NOT NULL PRIMARY KEY,
        LocationName nvarchar(128) NOT NULL,
        WorldID int NOT NULL,
        RegionID int NOT NULL,
        PosX int NOT NULL,
        PosY int NOT NULL,
        PosZ int NOT NULL,
        Weight int NOT NULL DEFAULT (1),
        IsActive bit NOT NULL DEFAULT (1),
        LastUsedAtUtc datetime2(0) NULL,
        CreatedAtUtc datetime2(0) NOT NULL DEFAULT (SYSUTCDATETIME()),
        UpdatedAtUtc datetime2(0) NOT NULL DEFAULT (SYSUTCDATETIME())
    );

IF NOT EXISTS (SELECT 1 FROM Events.dbo._HideAndSeekLocation)
    INSERT INTO Events.dbo._HideAndSeekLocation
        (LocationName,WorldID,RegionID,PosX,PosY,PosZ)
    VALUES
        (N'Yeohas Forest at Jangan',1,23971,93,1210,4),
        (N'North-Tiger Mountain at Jangan',1,23710,940,1021,1169),
        (N'Bandit Mountain Stronghold at Jangan',1,23455,566,981,1658),
        (N'Jangan Ferry',1,24734,0,298,422),
        (N'Jangan Ferry',1,24992,955,247,368);

IF OBJECT_ID(N'Events.dbo._HideAndSeekReward', N'U') IS NULL
    CREATE TABLE Events.dbo._HideAndSeekReward
    (
        RewardID int IDENTITY(1,1) NOT NULL PRIMARY KEY,
        RewardType nvarchar(24) NOT NULL,
        Amount bigint NOT NULL DEFAULT (0),
        ItemCodeName128 varchar(128) NULL,
        ItemID int NULL,
        ItemCount int NOT NULL DEFAULT (1),
        Plus int NOT NULL DEFAULT (0),
        IsActive bit NOT NULL DEFAULT (1),
        CreatedAtUtc datetime2(0) NOT NULL DEFAULT (SYSUTCDATETIME())
    );

IF NOT EXISTS (SELECT 1 FROM Events.dbo._HideAndSeekReward)
    INSERT INTO Events.dbo._HideAndSeekReward
        (RewardType,Amount,ItemCount,Plus,IsActive)
    VALUES
        (N'SilkOwn',100,1,0,1);

IF OBJECT_ID(N'Events.dbo._HideAndSeekSchedule', N'U') IS NULL
    CREATE TABLE Events.dbo._HideAndSeekSchedule
    (
        ScheduleID int IDENTITY(1,1) NOT NULL PRIMARY KEY,
        StartTime time(0) NOT NULL,
        DaysMask tinyint NOT NULL DEFAULT (127),
        IsActive bit NOT NULL DEFAULT (1),
        LastRunLocalDate date NULL,
        UpdatedAtUtc datetime2(0) NOT NULL DEFAULT (SYSUTCDATETIME())
    );

IF OBJECT_ID(N'Events.dbo._HideAndSeekRun', N'U') IS NULL
BEGIN
    CREATE TABLE Events.dbo._HideAndSeekRun
    (
        HnsRunID bigint IDENTITY(1,1) NOT NULL PRIMARY KEY,
        AutoRunID bigint NOT NULL,
        LocationID int NOT NULL,
        LocationName nvarchar(128) NOT NULL,
        WorldID int NOT NULL,
        RegionID int NOT NULL,
        PosX int NOT NULL,
        PosY int NOT NULL,
        PosZ int NOT NULL,
        BotCharID int NOT NULL,
        BotCharacterName varchar(16) NOT NULL,
        Status nvarchar(24) NOT NULL,
        StartedAtUtc datetime2(3) NOT NULL DEFAULT (SYSUTCDATETIME()),
        EndsAtUtc datetime2(3) NOT NULL,
        EndedAtUtc datetime2(3) NULL,
        WinnerCharID int NULL,
        WinnerCharName nvarchar(64) NULL,
        WinnerJID int NULL,
        WinnerHwid varchar(128) NULL,
        WinnerIP varchar(64) NULL,
        RewardSummary nvarchar(512) NULL,
        Message nvarchar(512) NULL
    );
    CREATE UNIQUE INDEX UX_HideAndSeekRun_AutoRunID
        ON Events.dbo._HideAndSeekRun(AutoRunID);
END;", connection);
        await command.ExecuteNonQueryAsync();
    }

    public async Task<DataTable> LoadHideAndSeekConfigAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureHideAndSeekSchemaAsync(connection);
        return await QueryTableAsync(connection, @"
SELECT C.EventCode,C.DisplayName,C.Enabled,C.StartDelaySeconds,C.SearchSeconds,
       C.ReminderIntervalSeconds,C.MinLevel,C.HwidLimit,C.RequireHwid,
       C.BotAccountName,ISNULL(C.BotPassword,'') AS BotPassword,C.BotCharacterName,C.BotShardID,C.BotLocale,
       C.ReturnWorldID,C.ReturnRegionID,C.ReturnX,C.ReturnY,C.ReturnZ,
       ISNULL(A.LastStatus,N'Not provisioned') AS BotStatus,
       ISNULL(A.LastMessage,N'') AS BotMessage,
       A.LastLoginAt,C.UpdatedAtUtc
FROM Events.dbo._HideAndSeekConfig C WITH(NOLOCK)
LEFT JOIN dbo.Clientless_Accounts A WITH(NOLOCK)
  ON A.SystemRole='HNS'
WHERE C.EventCode=N'HNS';");
    }

    public async Task SaveHideAndSeekConfigAsync(
        string displayName,
        bool enabled,
        int startDelaySeconds,
        int searchSeconds,
        int reminderIntervalSeconds,
        int minLevel,
        int hwidLimit,
        bool requireHwid,
        string botAccountName,
        string botPassword,
        string botCharacterName,
        int returnWorldId,
        int returnRegionId,
        int returnX,
        int returnY,
        int returnZ)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureHideAndSeekSchemaAsync(connection);
        await using var command = new SqlCommand(@"
UPDATE Events.dbo._HideAndSeekConfig
SET DisplayName=@DisplayName,
    Enabled=@Enabled,
    StartDelaySeconds=@StartDelaySeconds,
    SearchSeconds=@SearchSeconds,
    ReminderIntervalSeconds=@ReminderIntervalSeconds,
    MinLevel=@MinLevel,
    HwidLimit=@HwidLimit,
    RequireHwid=@RequireHwid,
    BotAccountName=@BotAccountName,
    BotPassword=@BotPassword,
    BotCharacterName=@BotCharacterName,
    ReturnWorldID=@ReturnWorldID,
    ReturnRegionID=@ReturnRegionID,
    ReturnX=@ReturnX,
    ReturnY=@ReturnY,
    ReturnZ=@ReturnZ,
    UpdatedAtUtc=SYSUTCDATETIME()
WHERE EventCode=N'HNS';", connection);
        command.Parameters.Add("@DisplayName", SqlDbType.NVarChar, 64).Value =
            string.IsNullOrWhiteSpace(displayName) ? "Hide and Seek" : displayName.Trim();
        command.Parameters.Add("@Enabled", SqlDbType.Bit).Value = enabled;
        command.Parameters.Add("@StartDelaySeconds", SqlDbType.Int).Value = Math.Clamp(startDelaySeconds, 0, 3600);
        command.Parameters.Add("@SearchSeconds", SqlDbType.Int).Value = Math.Clamp(searchSeconds, 30, 7200);
        command.Parameters.Add("@ReminderIntervalSeconds", SqlDbType.Int).Value =
            Math.Clamp(reminderIntervalSeconds, 0, 3600);
        command.Parameters.Add("@MinLevel", SqlDbType.Int).Value = Math.Clamp(minLevel, 0, 255);
        command.Parameters.Add("@HwidLimit", SqlDbType.Int).Value = Math.Clamp(hwidLimit, 0, 32);
        command.Parameters.Add("@RequireHwid", SqlDbType.Bit).Value = requireHwid;
        botAccountName = (botAccountName ?? string.Empty).Trim().ToLowerInvariant();
        botPassword ??= string.Empty;
        botCharacterName = (botCharacterName ?? string.Empty).Trim();
        if (!Regex.IsMatch(botAccountName, @"^[a-z0-9_]{4,16}$"))
            throw new InvalidOperationException("Bot account username must be 4-16 letters, numbers, or underscore.");
        if (!Regex.IsMatch(botPassword, @"^[\x21-\x7E]{6,32}$"))
            throw new InvalidOperationException("Bot account password must be 6-32 visible characters without spaces.");
        if (botCharacterName.Length is < 3 or > 16 || botCharacterName.Any(char.IsWhiteSpace))
            throw new InvalidOperationException("Bot character name must be 3-16 characters without spaces.");
        command.Parameters.Add("@BotAccountName", SqlDbType.VarChar, 24).Value = botAccountName;
        command.Parameters.Add("@BotPassword", SqlDbType.VarChar, 64).Value = botPassword;
        command.Parameters.Add("@BotCharacterName", SqlDbType.VarChar, 16).Value = botCharacterName;
        command.Parameters.Add("@ReturnWorldID", SqlDbType.Int).Value = Math.Max(1, returnWorldId);
        command.Parameters.Add("@ReturnRegionID", SqlDbType.Int).Value = Math.Max(1, returnRegionId);
        command.Parameters.Add("@ReturnX", SqlDbType.Int).Value = returnX;
        command.Parameters.Add("@ReturnY", SqlDbType.Int).Value = returnY;
        command.Parameters.Add("@ReturnZ", SqlDbType.Int).Value = returnZ;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "SaveHideAndSeekConfig", "HNS", displayName);
    }

    public async Task<DataTable> LoadHideAndSeekLocationsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureHideAndSeekSchemaAsync(connection);
        return await QueryTableAsync(connection, @"
SELECT LocationID,LocationName,WorldID,RegionID,PosX,PosY,PosZ,Weight,IsActive,LastUsedAtUtc
FROM Events.dbo._HideAndSeekLocation WITH(NOLOCK)
ORDER BY IsActive DESC,LocationName,LocationID;");
    }

    public async Task SaveHideAndSeekLocationAsync(
        int locationId,
        string locationName,
        int worldId,
        int regionId,
        int posX,
        int posY,
        int posZ,
        int weight,
        bool isActive)
    {
        if (string.IsNullOrWhiteSpace(locationName))
            throw new InvalidOperationException("Location name is required.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureHideAndSeekSchemaAsync(connection);
        var sql = locationId > 0
            ? @"UPDATE Events.dbo._HideAndSeekLocation
                SET LocationName=@LocationName,WorldID=@WorldID,RegionID=@RegionID,
                    PosX=@PosX,PosY=@PosY,PosZ=@PosZ,Weight=@Weight,IsActive=@IsActive,
                    UpdatedAtUtc=SYSUTCDATETIME()
                WHERE LocationID=@LocationID;"
            : @"INSERT INTO Events.dbo._HideAndSeekLocation
                    (LocationName,WorldID,RegionID,PosX,PosY,PosZ,Weight,IsActive)
                VALUES
                    (@LocationName,@WorldID,@RegionID,@PosX,@PosY,@PosZ,@Weight,@IsActive);";
        await using var command = new SqlCommand(sql, connection);
        command.Parameters.Add("@LocationID", SqlDbType.Int).Value = locationId;
        command.Parameters.Add("@LocationName", SqlDbType.NVarChar, 128).Value = locationName.Trim();
        command.Parameters.Add("@WorldID", SqlDbType.Int).Value = Math.Max(1, worldId);
        command.Parameters.Add("@RegionID", SqlDbType.Int).Value = Math.Max(1, regionId);
        command.Parameters.Add("@PosX", SqlDbType.Int).Value = posX;
        command.Parameters.Add("@PosY", SqlDbType.Int).Value = posY;
        command.Parameters.Add("@PosZ", SqlDbType.Int).Value = posZ;
        command.Parameters.Add("@Weight", SqlDbType.Int).Value = Math.Clamp(weight, 1, 1000);
        command.Parameters.Add("@IsActive", SqlDbType.Bit).Value = isActive;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, locationId > 0 ? "UpdateHideAndSeekLocation" : "AddHideAndSeekLocation",
            locationId.ToString(), locationName);
    }

    public async Task DeleteHideAndSeekLocationAsync(int locationId)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureHideAndSeekSchemaAsync(connection);
        await using var command = new SqlCommand(
            "DELETE FROM Events.dbo._HideAndSeekLocation WHERE LocationID=@LocationID;",
            connection);
        command.Parameters.Add("@LocationID", SqlDbType.Int).Value = locationId;
        if (await command.ExecuteNonQueryAsync() == 0)
            throw new InvalidOperationException("Hide and Seek location was not found.");
        await AuditAsync(connection, "DeleteHideAndSeekLocation", locationId.ToString(), string.Empty);
    }

    public async Task<DataTable> LoadHideAndSeekRewardsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureHideAndSeekSchemaAsync(connection);
        return await QueryTableAsync(connection, @"
SELECT RewardID,RewardType,Amount,ItemCodeName128,ItemID,ItemCount,Plus,IsActive,CreatedAtUtc
FROM Events.dbo._HideAndSeekReward WITH(NOLOCK)
ORDER BY IsActive DESC,RewardID;");
    }

    public async Task SaveHideAndSeekRewardAsync(
        int rewardId,
        string rewardType,
        long amount,
        string itemCodeName,
        int? itemId,
        int itemCount,
        int plus,
        bool isActive)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureHideAndSeekSchemaAsync(connection);
        var sql = rewardId > 0
            ? @"UPDATE Events.dbo._HideAndSeekReward
                SET RewardType=@RewardType,Amount=@Amount,ItemCodeName128=@ItemCodeName128,
                    ItemID=@ItemID,ItemCount=@ItemCount,Plus=@Plus,IsActive=@IsActive
                WHERE RewardID=@RewardID;"
            : @"INSERT INTO Events.dbo._HideAndSeekReward
                    (RewardType,Amount,ItemCodeName128,ItemID,ItemCount,Plus,IsActive)
                VALUES
                    (@RewardType,@Amount,@ItemCodeName128,@ItemID,@ItemCount,@Plus,@IsActive);";
        await using var command = new SqlCommand(sql, connection);
        command.Parameters.Add("@RewardID", SqlDbType.Int).Value = rewardId;
        command.Parameters.Add("@RewardType", SqlDbType.NVarChar, 24).Value = rewardType.Trim();
        command.Parameters.Add("@Amount", SqlDbType.BigInt).Value = Math.Max(0, amount);
        command.Parameters.Add("@ItemCodeName128", SqlDbType.VarChar, 128).Value =
            string.IsNullOrWhiteSpace(itemCodeName) ? DBNull.Value : itemCodeName.Trim();
        command.Parameters.Add("@ItemID", SqlDbType.Int).Value =
            itemId.HasValue && itemId.Value > 0 ? itemId.Value : DBNull.Value;
        command.Parameters.Add("@ItemCount", SqlDbType.Int).Value = Math.Max(1, itemCount);
        command.Parameters.Add("@Plus", SqlDbType.Int).Value = Math.Clamp(plus, 0, 255);
        command.Parameters.Add("@IsActive", SqlDbType.Bit).Value = isActive;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, rewardId > 0 ? "UpdateHideAndSeekReward" : "AddHideAndSeekReward",
            rewardId.ToString(), rewardType);
    }

    public async Task DeleteHideAndSeekRewardAsync(int rewardId)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureHideAndSeekSchemaAsync(connection);
        await using var command = new SqlCommand(
            "DELETE FROM Events.dbo._HideAndSeekReward WHERE RewardID=@RewardID;",
            connection);
        command.Parameters.Add("@RewardID", SqlDbType.Int).Value = rewardId;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "DeleteHideAndSeekReward", rewardId.ToString(), string.Empty);
    }

    public async Task<DataTable> LoadHideAndSeekSchedulesAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureHideAndSeekSchemaAsync(connection);
        return await QueryTableAsync(connection, @"
SELECT ScheduleID,CONVERT(varchar(8),StartTime,108) AS StartTime,DaysMask,
       STUFF(
           CASE WHEN (DaysMask&1)=1 THEN N', Sun' ELSE N'' END+
           CASE WHEN (DaysMask&2)=2 THEN N', Mon' ELSE N'' END+
           CASE WHEN (DaysMask&4)=4 THEN N', Tue' ELSE N'' END+
           CASE WHEN (DaysMask&8)=8 THEN N', Wed' ELSE N'' END+
           CASE WHEN (DaysMask&16)=16 THEN N', Thu' ELSE N'' END+
           CASE WHEN (DaysMask&32)=32 THEN N', Fri' ELSE N'' END+
           CASE WHEN (DaysMask&64)=64 THEN N', Sat' ELSE N'' END,
           1,2,N'') AS Days,
       IsActive,LastRunLocalDate
FROM Events.dbo._HideAndSeekSchedule WITH(NOLOCK)
ORDER BY StartTime,ScheduleID;");
    }

    public async Task SaveHideAndSeekScheduleAsync(
        int scheduleId,
        TimeSpan startTime,
        int daysMask,
        bool isActive)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureHideAndSeekSchemaAsync(connection);
        var sql = scheduleId > 0
            ? @"UPDATE Events.dbo._HideAndSeekSchedule
                SET StartTime=@StartTime,DaysMask=@DaysMask,IsActive=@IsActive,
                    LastRunLocalDate=NULL,UpdatedAtUtc=SYSUTCDATETIME()
                WHERE ScheduleID=@ScheduleID;"
            : @"INSERT INTO Events.dbo._HideAndSeekSchedule(StartTime,DaysMask,IsActive)
                VALUES(@StartTime,@DaysMask,@IsActive);";
        await using var command = new SqlCommand(sql, connection);
        command.Parameters.Add("@ScheduleID", SqlDbType.Int).Value = scheduleId;
        command.Parameters.Add("@StartTime", SqlDbType.Time).Value = startTime;
        command.Parameters.Add("@DaysMask", SqlDbType.TinyInt).Value = Math.Clamp(daysMask, 0, 127);
        command.Parameters.Add("@IsActive", SqlDbType.Bit).Value = isActive;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, scheduleId > 0 ? "UpdateHideAndSeekSchedule" : "AddHideAndSeekSchedule",
            scheduleId.ToString(), $"{startTime:hh\\:mm};DaysMask={daysMask}");
    }

    public async Task DeleteHideAndSeekScheduleAsync(int scheduleId)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureHideAndSeekSchemaAsync(connection);
        await using var command = new SqlCommand(
            "DELETE FROM Events.dbo._HideAndSeekSchedule WHERE ScheduleID=@ScheduleID;",
            connection);
        command.Parameters.Add("@ScheduleID", SqlDbType.Int).Value = scheduleId;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "DeleteHideAndSeekSchedule", scheduleId.ToString(), string.Empty);
    }

    public async Task<DataTable> LoadHideAndSeekRunsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureHideAndSeekSchemaAsync(connection);
        return await QueryTableAsync(connection, @"
SELECT TOP(100) HnsRunID,Status,LocationName,WorldID,RegionID,BotCharacterName,
       WinnerCharName,RewardSummary,StartedAtUtc,EndedAtUtc,Message
FROM Events.dbo._HideAndSeekRun WITH(NOLOCK)
ORDER BY HnsRunID DESC;");
    }

    private static string NormalizeCompetitiveCode(string eventCode)
    {
        var code = (eventCode ?? string.Empty).Trim().ToUpperInvariant();
        return code is "LMS" or "MADNESS" or "DTT"
            ? code
            : throw new InvalidOperationException("Unknown competitive event code.");
    }

    private async Task EnsureCompetitiveEventSchemaAsync(SqlConnection connection)
    {
        await using var command = new SqlCommand(@"
IF DB_ID(N'Events') IS NULL EXEC(N'CREATE DATABASE [Events]');
IF OBJECT_ID(N'Events.dbo._CompetitiveEventConfig',N'U') IS NULL
BEGIN
 CREATE TABLE Events.dbo._CompetitiveEventConfig
 (
  EventCode nvarchar(32) NOT NULL CONSTRAINT PK_CompetitiveEventConfig PRIMARY KEY,
  DisplayName nvarchar(64) NOT NULL, EventMode nvarchar(24) NOT NULL, EventID int NOT NULL,
  Enabled bit NOT NULL DEFAULT(1), StartDelaySeconds int NOT NULL DEFAULT(60),
  RegistrationSeconds int NOT NULL DEFAULT(600), PrepareSeconds int NOT NULL DEFAULT(20), FightSeconds int NOT NULL DEFAULT(600),
  MinPlayers int NOT NULL DEFAULT(2), MaxPlayers int NOT NULL DEFAULT(100), MinLevel int NOT NULL DEFAULT(1),
  HwidLimit int NOT NULL DEFAULT(1), RequireHwid bit NOT NULL DEFAULT(1), RequireNoParty bit NOT NULL DEFAULT(1),
  ArenaWorldID int NOT NULL, ArenaRegionID int NOT NULL, ArenaX int NOT NULL, ArenaY int NOT NULL, ArenaZ int NOT NULL,
  Team1X int NOT NULL DEFAULT(0),Team1Y int NOT NULL DEFAULT(0),Team1Z int NOT NULL DEFAULT(0),
  Team2X int NOT NULL DEFAULT(0),Team2Y int NOT NULL DEFAULT(0),Team2Z int NOT NULL DEFAULT(0),
  MadnessMobID int NOT NULL DEFAULT(0),MadnessMobCount int NOT NULL DEFAULT(3),
  MadnessMobX int NOT NULL DEFAULT(0),MadnessMobY int NOT NULL DEFAULT(0),MadnessMobZ int NOT NULL DEFAULT(0),
  MobSpawnDelaySeconds int NOT NULL DEFAULT(60),PairKillLimit int NOT NULL DEFAULT(3),TotalKillLimit int NOT NULL DEFAULT(200),
  KillRewardItemCode varchar(128) NULL,KillRewardItemCount int NOT NULL DEFAULT(1),KillRewardLimit int NOT NULL DEFAULT(10),
  Team1TowerMobID int NOT NULL DEFAULT(0),Team1TowerX int NOT NULL DEFAULT(0),Team1TowerY int NOT NULL DEFAULT(0),Team1TowerZ int NOT NULL DEFAULT(0),
  Team2TowerMobID int NOT NULL DEFAULT(0),Team2TowerX int NOT NULL DEFAULT(0),Team2TowerY int NOT NULL DEFAULT(0),Team2TowerZ int NOT NULL DEFAULT(0),
  UpdatedAtUtc datetime2(0) NOT NULL DEFAULT(SYSUTCDATETIME())
 );
END;
IF NOT EXISTS(SELECT 1 FROM Events.dbo._CompetitiveEventConfig WHERE EventCode=N'LMS')
BEGIN
 DECLARE @LmsEventID int=NULL;
 IF OBJECT_ID(N'dbo.Event_RegisterSettings',N'U') IS NOT NULL SELECT TOP(1) @LmsEventID=r.ID FROM dbo.Event_RegisterSettings r WHERE r.Name=N'Last Man Standing' AND NOT EXISTS(SELECT 1 FROM Events.dbo._CompetitiveEventConfig c WHERE c.EventID=r.ID);
 IF @LmsEventID IS NULL BEGIN SET @LmsEventID=14; WHILE @LmsEventID<=255 AND (EXISTS(SELECT 1 FROM Events.dbo._CompetitiveEventConfig WHERE EventID=@LmsEventID) OR EXISTS(SELECT 1 FROM dbo.Event_RegisterSettings WHERE ID=@LmsEventID)) SET @LmsEventID+=1; END;
 IF @LmsEventID>255 THROW 51000,'No free Event Register ID is available for LMS.',1;
 INSERT Events.dbo._CompetitiveEventConfig(EventCode,DisplayName,EventMode,EventID,ArenaWorldID,ArenaRegionID,ArenaX,ArenaY,ArenaZ)
 VALUES(N'LMS',N'Last Man Standing',N'LastManStanding',@LmsEventID,107,25580,500,0,500);
END;
IF NOT EXISTS(SELECT 1 FROM Events.dbo._CompetitiveEventConfig WHERE EventCode=N'MADNESS')
BEGIN
 DECLARE @MadnessEventID int=NULL;
 IF OBJECT_ID(N'dbo.Event_RegisterSettings',N'U') IS NOT NULL SELECT TOP(1) @MadnessEventID=r.ID FROM dbo.Event_RegisterSettings r WHERE r.Name=N'Madness Solo' AND NOT EXISTS(SELECT 1 FROM Events.dbo._CompetitiveEventConfig c WHERE c.EventID=r.ID);
 IF @MadnessEventID IS NULL BEGIN SET @MadnessEventID=14; WHILE @MadnessEventID<=255 AND (EXISTS(SELECT 1 FROM Events.dbo._CompetitiveEventConfig WHERE EventID=@MadnessEventID) OR EXISTS(SELECT 1 FROM dbo.Event_RegisterSettings WHERE ID=@MadnessEventID)) SET @MadnessEventID+=1; END;
 IF @MadnessEventID>255 THROW 51000,'No free Event Register ID is available for Madness.',1;
 INSERT Events.dbo._CompetitiveEventConfig(EventCode,DisplayName,EventMode,EventID,ArenaWorldID,ArenaRegionID,ArenaX,ArenaY,ArenaZ,
 MadnessMobX,MadnessMobY,MadnessMobZ,MobSpawnDelaySeconds)
 VALUES(N'MADNESS',N'Madness Solo',N'MadnessSolo',@MadnessEventID,108,25584,1165,595,1083,1646,503,636,120);
END;
IF NOT EXISTS(SELECT 1 FROM Events.dbo._CompetitiveEventConfig WHERE EventCode=N'DTT')
BEGIN
 DECLARE @DttEventID int=NULL;
 IF OBJECT_ID(N'dbo.Event_RegisterSettings',N'U') IS NOT NULL SELECT TOP(1) @DttEventID=r.ID FROM dbo.Event_RegisterSettings r WHERE r.Name=N'Defend The Tower' AND NOT EXISTS(SELECT 1 FROM Events.dbo._CompetitiveEventConfig c WHERE c.EventID=r.ID);
 IF @DttEventID IS NULL BEGIN SET @DttEventID=14; WHILE @DttEventID<=255 AND (EXISTS(SELECT 1 FROM Events.dbo._CompetitiveEventConfig WHERE EventID=@DttEventID) OR EXISTS(SELECT 1 FROM dbo.Event_RegisterSettings WHERE ID=@DttEventID)) SET @DttEventID+=1; END;
 IF @DttEventID>255 THROW 51000,'No free Event Register ID is available for DTT.',1;
 INSERT Events.dbo._CompetitiveEventConfig(EventCode,DisplayName,EventMode,EventID,MinPlayers,ArenaWorldID,ArenaRegionID,ArenaX,ArenaY,ArenaZ,
 Team1X,Team1Y,Team1Z,Team2X,Team2Y,Team2Z,MobSpawnDelaySeconds,Team1TowerX,Team1TowerY,Team1TowerZ,Team2TowerX,Team2TowerY,Team2TowerZ)
 VALUES(N'DTT',N'Defend The Tower',N'DefendTower',@DttEventID,4,109,32471,960,215,960,960,215,262,961,215,1646,60,960,218,603,958,218,1289);
END;
IF EXISTS(SELECT EventID FROM Events.dbo._CompetitiveEventConfig GROUP BY EventID HAVING COUNT(*)>1)
 THROW 51000,'Competitive Event IDs must be unique.',1;
IF NOT EXISTS(SELECT 1 FROM Events.sys.indexes WHERE object_id=OBJECT_ID(N'Events.dbo._CompetitiveEventConfig') AND name=N'UX_CompetitiveEventConfig_EventID')
 EXEC Events.sys.sp_executesql N'CREATE UNIQUE INDEX UX_CompetitiveEventConfig_EventID ON dbo._CompetitiveEventConfig(EventID);';

IF OBJECT_ID(N'Events.dbo._CompetitiveEventReward',N'U') IS NULL
 CREATE TABLE Events.dbo._CompetitiveEventReward
 (RewardID int IDENTITY(1,1) NOT NULL PRIMARY KEY,EventCode nvarchar(32) NOT NULL,Placement int NOT NULL,
 RewardType nvarchar(24) NOT NULL,Amount bigint NOT NULL DEFAULT(0),ItemCodeName128 varchar(128) NULL,ItemID int NULL,
 ItemCount int NOT NULL DEFAULT(1),Plus int NOT NULL DEFAULT(0),IsActive bit NOT NULL DEFAULT(1),CreatedAtUtc datetime2(0) NOT NULL DEFAULT(SYSUTCDATETIME()));
IF NOT EXISTS(SELECT 1 FROM Events.dbo._CompetitiveEventReward WHERE EventCode=N'LMS' AND Placement=1)
 INSERT Events.dbo._CompetitiveEventReward(EventCode,Placement,RewardType,Amount) VALUES(N'LMS',1,N'SilkOwn',100);
IF NOT EXISTS(SELECT 1 FROM Events.dbo._CompetitiveEventReward WHERE EventCode=N'MADNESS' AND Placement=1)
 INSERT Events.dbo._CompetitiveEventReward(EventCode,Placement,RewardType,Amount) VALUES(N'MADNESS',1,N'SilkOwn',100);
IF NOT EXISTS(SELECT 1 FROM Events.dbo._CompetitiveEventReward WHERE EventCode=N'MADNESS' AND Placement=2)
 INSERT Events.dbo._CompetitiveEventReward(EventCode,Placement,RewardType,Amount) VALUES(N'MADNESS',2,N'SilkOwn',50);
IF NOT EXISTS(SELECT 1 FROM Events.dbo._CompetitiveEventReward WHERE EventCode=N'MADNESS' AND Placement=3)
 INSERT Events.dbo._CompetitiveEventReward(EventCode,Placement,RewardType,Amount) VALUES(N'MADNESS',3,N'SilkOwn',25);
IF NOT EXISTS(SELECT 1 FROM Events.dbo._CompetitiveEventReward WHERE EventCode=N'DTT' AND Placement=1)
 INSERT Events.dbo._CompetitiveEventReward(EventCode,Placement,RewardType,Amount) VALUES(N'DTT',1,N'SilkOwn',500);
IF NOT EXISTS(SELECT 1 FROM Events.dbo._CompetitiveEventReward WHERE EventCode=N'DTT' AND Placement=2)
 INSERT Events.dbo._CompetitiveEventReward(EventCode,Placement,RewardType,Amount) VALUES(N'DTT',2,N'SilkOwn',50);

IF OBJECT_ID(N'Events.dbo._CompetitiveEventSchedule',N'U') IS NULL
 CREATE TABLE Events.dbo._CompetitiveEventSchedule
 (ScheduleID int IDENTITY(1,1) NOT NULL PRIMARY KEY,EventCode nvarchar(32) NOT NULL,StartTime time(0) NOT NULL,
 DaysMask tinyint NOT NULL DEFAULT(127),IsActive bit NOT NULL DEFAULT(1),LastRunLocalDate date NULL,UpdatedAtUtc datetime2(0) NOT NULL DEFAULT(SYSUTCDATETIME()));", connection);
        await command.ExecuteNonQueryAsync();
    }

    public async Task<DataTable> LoadCompetitiveEventConfigAsync(string eventCode)
    {
        eventCode = NormalizeCompetitiveCode(eventCode);
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureCompetitiveEventSchemaAsync(connection);
        return await QueryTableAsync(connection, @"SELECT * FROM Events.dbo._CompetitiveEventConfig WITH(NOLOCK) WHERE EventCode=@EventCode;",
            new SqlParameter("@EventCode", eventCode));
    }

    public async Task SaveCompetitiveEventConfigAsync(CompetitiveEventAdminConfig config)
    {
        var eventCode = NormalizeCompetitiveCode(config.EventCode);
        config = config with { ArenaRegionID = NormalizeRegionId(config.ArenaRegionID) };
        var canonicalName = eventCode switch { "LMS" => "Last Man Standing", "MADNESS" => "Madness Solo", _ => "Defend The Tower" };
        if (string.IsNullOrWhiteSpace(config.DisplayName) || config.DisplayName.Trim().Length > 64) throw new InvalidOperationException("Display name must be between 1 and 64 characters.");
        if (config.EventID is < 1 or > 255) throw new InvalidOperationException("Event ID must be between 1 and 255.");
        if (config.StartDelaySeconds is < 0 or > 3600) throw new InvalidOperationException("Start delay must be between 0 and 3600 seconds.");
        if (config.RegistrationSeconds is < 10 or > 3600) throw new InvalidOperationException("Registration duration must be between 10 and 3600 seconds.");
        if (config.PrepareSeconds is < 0 or > 300) throw new InvalidOperationException("Prepare duration must be between 0 and 300 seconds.");
        if (config.FightSeconds is < 30 or > 7200) throw new InvalidOperationException("Fight duration must be between 30 and 7200 seconds.");
        if (config.MinPlayers < 2 || config.MaxPlayers > 100 || config.MinPlayers > config.MaxPlayers) throw new InvalidOperationException("Players must be between 2 and 100, and minimum cannot exceed maximum.");
        if (config.MinLevel < 0) throw new InvalidOperationException("Minimum level cannot be negative.");
        if (config.HwidLimit is < 0 or > 32) throw new InvalidOperationException("HWID limit must be between 0 and 32.");
        if (config.ArenaWorldID <= 0 || !IsValidRegionId(config.ArenaRegionID)) throw new InvalidOperationException("WorldID must be positive and RegionID must be a non-zero signed 16-bit value.");
        if (config.MobSpawnDelaySeconds is < 0 or > 3600) throw new InvalidOperationException("Mob spawn delay must be between 0 and 3600 seconds.");
        if (config.MadnessMobCount is < 0 or > 20) throw new InvalidOperationException("Madness mob count must be between 0 and 20.");
        if (config.PairKillLimit < 0 || config.TotalKillLimit < 0 || config.KillRewardLimit < 0) throw new InvalidOperationException("Kill limits cannot be negative.");
        if (config.KillRewardItemCount <= 0) throw new InvalidOperationException("Kill reward item count must be greater than zero.");
        if (eventCode == "DTT" && (config.Team1TowerMobID <= 0 || config.Team2TowerMobID <= 0 || config.Team1TowerMobID == config.Team2TowerMobID))
            throw new InvalidOperationException("Defend The Tower requires two different positive tower MobIDs.");
        if (eventCode == "MADNESS" && config.MadnessMobCount > 0 && config.MadnessMobID <= 0)
            throw new InvalidOperationException("Madness MobID is required when mob count is greater than zero.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureCompetitiveEventSchemaAsync(connection);
        await EnsureFilterRegionControlTableAsync(connection);
        var hasRegistrationTable = await ObjectExistsAsync(connection, "dbo.Event_RegisterSettings", "U");
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync();
        try
        {
            await using var ownerCommand = new SqlCommand(
                $"SELECT TOP(1) EventCode FROM {EventTable("_CompetitiveEventConfig")} WITH(UPDLOCK,HOLDLOCK) WHERE EventID=@EventID AND EventCode<>@EventCode;",
                connection, transaction);
            ownerCommand.Parameters.Add("@EventID", SqlDbType.Int).Value = config.EventID;
            ownerCommand.Parameters.Add("@EventCode", SqlDbType.NVarChar, 32).Value = eventCode;
            var owner = Convert.ToString(await ownerCommand.ExecuteScalarAsync());
            if (!string.IsNullOrWhiteSpace(owner))
                throw new InvalidOperationException($"Event ID {config.EventID} is already used by {owner}.");

            await using var oldIdCommand = new SqlCommand(
                $"SELECT EventID FROM {EventTable("_CompetitiveEventConfig")} WITH(UPDLOCK,HOLDLOCK) WHERE EventCode=@EventCode;",
                connection, transaction);
            oldIdCommand.Parameters.Add("@EventCode", SqlDbType.NVarChar, 32).Value = eventCode;
            var oldId = Convert.ToInt32(await oldIdCommand.ExecuteScalarAsync());
            await using var oldDisplayNameCommand = new SqlCommand(
                $"SELECT DisplayName FROM {EventTable("_CompetitiveEventConfig")} WHERE EventCode=@EventCode;",
                connection, transaction);
            oldDisplayNameCommand.Parameters.Add("@EventCode", SqlDbType.NVarChar, 32).Value = eventCode;
            var oldDisplayName = Convert.ToString(await oldDisplayNameCommand.ExecuteScalarAsync()) ?? canonicalName;

            await using var command = new SqlCommand($@"
UPDATE {EventTable("_CompetitiveEventConfig")} SET
 DisplayName=@DisplayName,Enabled=@Enabled,EventID=@EventID,StartDelaySeconds=@StartDelaySeconds,
 RegistrationSeconds=@RegistrationSeconds,PrepareSeconds=@PrepareSeconds,FightSeconds=@FightSeconds,
 MinPlayers=@MinPlayers,MaxPlayers=@MaxPlayers,MinLevel=@MinLevel,HwidLimit=@HwidLimit,
 RequireHwid=@RequireHwid,RequireNoParty=@RequireNoParty,ArenaWorldID=@ArenaWorldID,ArenaRegionID=@ArenaRegionID,
 ArenaX=@ArenaX,ArenaY=@ArenaY,ArenaZ=@ArenaZ,Team1X=@Team1X,Team1Y=@Team1Y,Team1Z=@Team1Z,
 Team2X=@Team2X,Team2Y=@Team2Y,Team2Z=@Team2Z,MadnessMobID=@MadnessMobID,MadnessMobCount=@MadnessMobCount,
 MadnessMobX=@MadnessMobX,MadnessMobY=@MadnessMobY,MadnessMobZ=@MadnessMobZ,MobSpawnDelaySeconds=@MobSpawnDelaySeconds,
 PairKillLimit=@PairKillLimit,TotalKillLimit=@TotalKillLimit,KillRewardItemCode=@KillRewardItemCode,
 KillRewardItemCount=@KillRewardItemCount,KillRewardLimit=@KillRewardLimit,
 Team1TowerMobID=@Team1TowerMobID,Team1TowerX=@Team1TowerX,Team1TowerY=@Team1TowerY,Team1TowerZ=@Team1TowerZ,
 Team2TowerMobID=@Team2TowerMobID,Team2TowerX=@Team2TowerX,Team2TowerY=@Team2TowerY,Team2TowerZ=@Team2TowerZ,
 UpdatedAtUtc=SYSUTCDATETIME() WHERE EventCode=@EventCode;", connection, transaction);
            command.Parameters.AddWithValue("@EventCode", eventCode);
            foreach (var property in typeof(CompetitiveEventAdminConfig).GetProperties().Where(x => x.Name != nameof(CompetitiveEventAdminConfig.EventCode)))
            {
                var value = property.GetValue(config);
                command.Parameters.AddWithValue("@" + property.Name, value is string text && string.IsNullOrWhiteSpace(text) ? DBNull.Value : value ?? DBNull.Value);
            }
            await command.ExecuteNonQueryAsync();

            if (hasRegistrationTable)
            {
                await using var registration = new SqlCommand(@"
IF @OldID<>@EventID DELETE dbo.Event_RegisterSettings WHERE ID=@OldID AND Name IN(@CanonicalName,@OldDisplayName,@DisplayName);
IF EXISTS(SELECT 1 FROM dbo.Event_RegisterSettings WHERE ID=@EventID AND Name NOT IN(@CanonicalName,@DisplayName))
 THROW 51000,'Event ID is already assigned to another event.',1;
MERGE dbo.Event_RegisterSettings target USING(SELECT @EventID ID) source ON target.ID=source.ID
WHEN MATCHED THEN UPDATE SET Name=@DisplayName,Description=N'KMTGuard competitive arena event'
WHEN NOT MATCHED THEN INSERT(ID,Name,Description) VALUES(@EventID,@DisplayName,N'KMTGuard competitive arena event');", connection, transaction);
                registration.Parameters.AddWithValue("@OldID", oldId);
                registration.Parameters.AddWithValue("@EventID", config.EventID);
                registration.Parameters.AddWithValue("@CanonicalName", canonicalName);
                registration.Parameters.AddWithValue("@OldDisplayName", oldDisplayName);
                registration.Parameters.AddWithValue("@DisplayName", config.DisplayName.Trim());
                await registration.ExecuteNonQueryAsync();
            }
            await UpsertEventRegionFeaturesAsync(
                connection,
                transaction,
                eventCode,
                config.ArenaWorldID,
                config.ArenaRegionID,
                teams: eventCode == "DTT",
                allowParty: !config.RequireNoParty);
            await transaction.CommitAsync();
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }
        await AuditAsync(connection, "SaveCompetitiveEventConfig", eventCode, config.DisplayName);
    }

    public async Task<DataTable> LoadCompetitiveEventRewardsAsync(string eventCode)
    {
        eventCode = NormalizeCompetitiveCode(eventCode);
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureCompetitiveEventSchemaAsync(connection);
        return await QueryTableAsync(connection, @"SELECT RewardID,Placement,RewardType,Amount,ItemCodeName128,ItemID,ItemCount,Plus,IsActive,CreatedAtUtc
FROM Events.dbo._CompetitiveEventReward WITH(NOLOCK) WHERE EventCode=@EventCode ORDER BY Placement,RewardID;", new SqlParameter("@EventCode", eventCode));
    }

    public async Task SaveCompetitiveEventRewardAsync(string eventCode, int rewardId, int placement, string rewardType, long amount,
        string itemCodeName, int? itemId, int itemCount, int plus, bool isActive)
    {
        eventCode = NormalizeCompetitiveCode(eventCode);
        if (placement <= 0) throw new InvalidOperationException("Placement must be greater than zero.");
        rewardType = new[] { "SilkOwn", "SilkGift", "SilkPoint", "Gold", "ItemChest" }
            .FirstOrDefault(x => x.Equals(rewardType.Trim(), StringComparison.OrdinalIgnoreCase))
            ?? throw new InvalidOperationException("Unsupported reward type.");
        if (rewardType.Equals("ItemChest", StringComparison.OrdinalIgnoreCase))
        {
            if (string.IsNullOrWhiteSpace(itemCodeName) && itemId is not > 0)
                throw new InvalidOperationException("ItemChest reward requires an Item CodeName or Item ID.");
            if (itemCount <= 0)
                throw new InvalidOperationException("Item count must be greater than zero.");
        }
        else if (amount <= 0)
        {
            throw new InvalidOperationException("Reward amount must be greater than zero.");
        }
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureCompetitiveEventSchemaAsync(connection);
        var sql = rewardId > 0
            ? @"UPDATE Events.dbo._CompetitiveEventReward SET Placement=@Placement,RewardType=@RewardType,Amount=@Amount,ItemCodeName128=@Code,ItemID=@ItemID,ItemCount=@Count,Plus=@Plus,IsActive=@Active WHERE RewardID=@RewardID AND EventCode=@EventCode;"
            : @"INSERT Events.dbo._CompetitiveEventReward(EventCode,Placement,RewardType,Amount,ItemCodeName128,ItemID,ItemCount,Plus,IsActive) VALUES(@EventCode,@Placement,@RewardType,@Amount,@Code,@ItemID,@Count,@Plus,@Active);";
        await using var command = new SqlCommand(sql, connection);
        command.Parameters.AddWithValue("@EventCode", eventCode); command.Parameters.AddWithValue("@RewardID", rewardId);
        command.Parameters.AddWithValue("@Placement", placement); command.Parameters.AddWithValue("@RewardType", rewardType.Trim());
        command.Parameters.AddWithValue("@Amount", amount); command.Parameters.AddWithValue("@Code", string.IsNullOrWhiteSpace(itemCodeName) ? DBNull.Value : itemCodeName.Trim());
        command.Parameters.AddWithValue("@ItemID", itemId is > 0 ? itemId.Value : DBNull.Value); command.Parameters.AddWithValue("@Count", Math.Max(1, itemCount));
        command.Parameters.AddWithValue("@Plus", Math.Max(0, plus)); command.Parameters.AddWithValue("@Active", isActive);
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, rewardId > 0 ? "UpdateCompetitiveReward" : "AddCompetitiveReward", eventCode, $"Placement={placement};Type={rewardType};Amount={amount}");
    }

    public async Task DeleteCompetitiveEventRewardAsync(string eventCode, int rewardId)
    {
        eventCode = NormalizeCompetitiveCode(eventCode);
        await using var connection = new SqlConnection(_connectionString); await connection.OpenAsync(); await EnsureCompetitiveEventSchemaAsync(connection);
        await using var command = new SqlCommand("DELETE Events.dbo._CompetitiveEventReward WHERE RewardID=@RewardID AND EventCode=@EventCode;", connection);
        command.Parameters.AddWithValue("@RewardID", rewardId); command.Parameters.AddWithValue("@EventCode", eventCode); await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "DeleteCompetitiveReward", eventCode, rewardId.ToString());
    }

    public async Task<DataTable> LoadCompetitiveEventSchedulesAsync(string eventCode)
    {
        eventCode = NormalizeCompetitiveCode(eventCode);
        await using var connection = new SqlConnection(_connectionString); await connection.OpenAsync(); await EnsureCompetitiveEventSchemaAsync(connection);
        return await QueryTableAsync(connection, @"SELECT ScheduleID,CONVERT(varchar(8),StartTime,108) StartTime,DaysMask,
STUFF(CASE WHEN (DaysMask&1)=1 THEN N', Sun' ELSE N'' END+CASE WHEN (DaysMask&2)=2 THEN N', Mon' ELSE N'' END+
CASE WHEN (DaysMask&4)=4 THEN N', Tue' ELSE N'' END+CASE WHEN (DaysMask&8)=8 THEN N', Wed' ELSE N'' END+
CASE WHEN (DaysMask&16)=16 THEN N', Thu' ELSE N'' END+CASE WHEN (DaysMask&32)=32 THEN N', Fri' ELSE N'' END+
CASE WHEN (DaysMask&64)=64 THEN N', Sat' ELSE N'' END,1,2,N'') Days,IsActive,LastRunLocalDate
FROM Events.dbo._CompetitiveEventSchedule WITH(NOLOCK) WHERE EventCode=@EventCode ORDER BY StartTime,ScheduleID;", new SqlParameter("@EventCode", eventCode));
    }

    public async Task SaveCompetitiveEventScheduleAsync(string eventCode, int scheduleId, TimeSpan startTime, int daysMask, bool isActive)
    {
        eventCode = NormalizeCompetitiveCode(eventCode);
        if (daysMask is < 1 or > 127) throw new InvalidOperationException("Select at least one valid schedule day.");
        await using var connection = new SqlConnection(_connectionString); await connection.OpenAsync(); await EnsureCompetitiveEventSchemaAsync(connection);
        var sql = scheduleId > 0
            ? "UPDATE Events.dbo._CompetitiveEventSchedule SET StartTime=@Time,DaysMask=@Days,IsActive=@Active,LastRunLocalDate=NULL,UpdatedAtUtc=SYSUTCDATETIME() WHERE ScheduleID=@ID AND EventCode=@EventCode;"
            : "INSERT Events.dbo._CompetitiveEventSchedule(EventCode,StartTime,DaysMask,IsActive) VALUES(@EventCode,@Time,@Days,@Active);";
        await using var command = new SqlCommand(sql, connection); command.Parameters.AddWithValue("@EventCode", eventCode);
        command.Parameters.AddWithValue("@ID", scheduleId); command.Parameters.AddWithValue("@Time", startTime); command.Parameters.AddWithValue("@Days", daysMask); command.Parameters.AddWithValue("@Active", isActive);
        await command.ExecuteNonQueryAsync(); await AuditAsync(connection, scheduleId > 0 ? "UpdateCompetitiveSchedule" : "AddCompetitiveSchedule", eventCode, startTime.ToString());
    }

    public async Task DeleteCompetitiveEventScheduleAsync(string eventCode, int scheduleId)
    {
        eventCode = NormalizeCompetitiveCode(eventCode);
        await using var connection = new SqlConnection(_connectionString); await connection.OpenAsync(); await EnsureCompetitiveEventSchemaAsync(connection);
        await using var command = new SqlCommand("DELETE Events.dbo._CompetitiveEventSchedule WHERE ScheduleID=@ID AND EventCode=@EventCode;", connection);
        command.Parameters.AddWithValue("@ID", scheduleId); command.Parameters.AddWithValue("@EventCode", eventCode); await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "DeleteCompetitiveSchedule", eventCode, scheduleId.ToString());
    }

    public async Task<DataTable> LoadAutoEventRewardsAsync(string eventCode)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        return await LoadOptionalTableAsync(connection, EventObjectName("_AutoEventReward"), $@"
SELECT TOP (300) RewardID, EventCode, Placement, RewardType, Amount, ItemCodeName128, ItemID, ItemCount, Plus, IsActive, CreatedAtUtc
FROM {EventTable("_AutoEventReward")} WITH (NOLOCK)
WHERE EventCode NOT IN (N'SPARTY', N'SSOLO') AND (@Term = N'' OR EventCode = @Term)
ORDER BY EventCode, Placement, IsActive DESC, RewardID DESC;", eventCode);
    }

    public async Task SaveAutoEventRewardAsync(int rewardId, string eventCode, int placement, string rewardType, long amount, string itemCodeName, int? itemId, int itemCount, int plus, bool isActive)
    {
        if (eventCode.Trim().Equals("SPARTY", StringComparison.OrdinalIgnoreCase) ||
            eventCode.Trim().Equals("SSOLO", StringComparison.OrdinalIgnoreCase))
            throw new InvalidOperationException("Use the dedicated Survival event page for this event code.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTableAsync(connection, EventObjectName("_AutoEventReward"));

        var sql = rewardId > 0
            ? $"UPDATE {EventTable("_AutoEventReward")} SET EventCode = @EventCode, Placement = @Placement, RewardType = @RewardType, Amount = @Amount, ItemCodeName128 = @ItemCodeName128, ItemID = @ItemID, ItemCount = @ItemCount, Plus = @Plus, IsActive = @IsActive WHERE RewardID = @RewardID"
            : $"INSERT INTO {EventTable("_AutoEventReward")} (EventCode, Placement, RewardType, Amount, ItemCodeName128, ItemID, ItemCount, Plus, IsActive) VALUES (@EventCode, @Placement, @RewardType, @Amount, @ItemCodeName128, @ItemID, @ItemCount, @Plus, @IsActive)";

        await using var command = new SqlCommand(sql, connection);
        command.Parameters.Add("@RewardID", SqlDbType.Int).Value = rewardId;
        command.Parameters.Add("@EventCode", SqlDbType.NVarChar, 32).Value = eventCode.Trim();
        command.Parameters.Add("@Placement", SqlDbType.Int).Value = placement;
        command.Parameters.Add("@RewardType", SqlDbType.NVarChar, 24).Value = rewardType.Trim();
        command.Parameters.Add("@Amount", SqlDbType.BigInt).Value = amount;
        command.Parameters.Add("@ItemCodeName128", SqlDbType.VarChar, 128).Value = string.IsNullOrWhiteSpace(itemCodeName) ? DBNull.Value : itemCodeName.Trim();
        command.Parameters.Add("@ItemID", SqlDbType.Int).Value = itemId.HasValue && itemId.Value > 0 ? itemId.Value : DBNull.Value;
        command.Parameters.Add("@ItemCount", SqlDbType.Int).Value = itemCount;
        command.Parameters.Add("@Plus", SqlDbType.Int).Value = plus;
        command.Parameters.Add("@IsActive", SqlDbType.Bit).Value = isActive;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, rewardId > 0 ? "UpdateAutoEventReward" : "AddAutoEventReward", eventCode, $"{rewardType};Amount={amount};ItemID={itemId}");
    }

    public async Task<DataTable> LoadAutoEventUniqueRewardsAsync(string term)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        return await LoadOptionalTableAsync(connection, EventObjectName("_AutoEventUniqueReward"), $@"
SELECT TOP (300) RewardID, MobID, Enabled, RewardType, Amount, ItemCodeName128, ItemID, ItemCount, Plus, CreatedAtUtc
FROM {EventTable("_AutoEventUniqueReward")} WITH (NOLOCK)
WHERE @Term = N'' OR CONVERT(NVARCHAR(32), MobID) = @Term OR ItemCodeName128 LIKE '%' + @Term + '%'
ORDER BY Enabled DESC, MobID, RewardID DESC;", term);
    }

    public async Task SaveAutoEventUniqueRewardAsync(int rewardId, int mobId, bool enabled, string rewardType, long amount, string itemCodeName, int? itemId, int itemCount, int plus)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTableAsync(connection, EventObjectName("_AutoEventUniqueReward"));

        var sql = rewardId > 0
            ? $"UPDATE {EventTable("_AutoEventUniqueReward")} SET MobID = @MobID, Enabled = @Enabled, RewardType = @RewardType, Amount = @Amount, ItemCodeName128 = @ItemCodeName128, ItemID = @ItemID, ItemCount = @ItemCount, Plus = @Plus WHERE RewardID = @RewardID"
            : $"INSERT INTO {EventTable("_AutoEventUniqueReward")} (MobID, Enabled, RewardType, Amount, ItemCodeName128, ItemID, ItemCount, Plus) VALUES (@MobID, @Enabled, @RewardType, @Amount, @ItemCodeName128, @ItemID, @ItemCount, @Plus)";

        await using var command = new SqlCommand(sql, connection);
        command.Parameters.Add("@RewardID", SqlDbType.Int).Value = rewardId;
        command.Parameters.Add("@MobID", SqlDbType.Int).Value = mobId;
        command.Parameters.Add("@Enabled", SqlDbType.Bit).Value = enabled;
        command.Parameters.Add("@RewardType", SqlDbType.NVarChar, 24).Value = rewardType.Trim();
        command.Parameters.Add("@Amount", SqlDbType.BigInt).Value = amount;
        command.Parameters.Add("@ItemCodeName128", SqlDbType.VarChar, 128).Value = string.IsNullOrWhiteSpace(itemCodeName) ? DBNull.Value : itemCodeName.Trim();
        command.Parameters.Add("@ItemID", SqlDbType.Int).Value = itemId.HasValue && itemId.Value > 0 ? itemId.Value : DBNull.Value;
        command.Parameters.Add("@ItemCount", SqlDbType.Int).Value = itemCount;
        command.Parameters.Add("@Plus", SqlDbType.Int).Value = plus;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, rewardId > 0 ? "UpdateAutoEventUniqueReward" : "AddAutoEventUniqueReward", $"MobID={mobId}", $"{rewardType};Amount={amount};ItemID={itemId}");
    }

    public async Task<DataTable> LoadAutoEventCommandQueueAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        return await LoadOptionalTableAsync(connection, EventObjectName("_AutoEventCommandQueue"), $@"
SELECT TOP (300) CommandID, CommandType, EventCode, RequestedBy, Status, CreatedAtUtc, ProcessedAtUtc, Message
FROM {EventTable("_AutoEventCommandQueue")} WITH (NOLOCK)
ORDER BY CommandID DESC;", string.Empty);
    }

    public async Task<DataTable> LoadAutoEventRoundsAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        return await LoadOptionalTableAsync(connection, EventObjectName("_AutoEventRound"), $@"
SELECT TOP (300) RoundID, RunID, RoundNo, Prompt, AnswerMasked, Status, StartedAtUtc, EndedAtUtc, WinnerCharID, WinnerCharName
FROM {EventTable("_AutoEventRound")} WITH (NOLOCK)
ORDER BY RoundID DESC;", string.Empty);
    }

    public async Task<DataTable> LoadPvpConfigAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        return await LoadOptionalTableAsync(connection, "[dbo].[PVP_Settings]", "SELECT TOP (20) * FROM [dbo].[PVP_Settings] WITH (NOLOCK) ORDER BY ID;", string.Empty);
    }

    public async Task<DataTable> LoadPvpArenasAsync()
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        return await LoadOptionalTableAsync(connection, "[dbo].[PVP_Arenas]", "SELECT TOP (300) * FROM [dbo].[PVP_Arenas] WITH (NOLOCK) ORDER BY Enabled DESC, SortOrder, ArenaID;", string.Empty);
    }

    public async Task<DataTable> LoadPvpMatchesAsync(string term)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        return await LoadOptionalTableAsync(connection, "[dbo].[PVP_Matches]", @"
SELECT TOP (500) MatchID, ChallengerCharName, OpponentCharName, WagerGold, ArenaID, Status, RequestedAt, AcceptedAt, StartedAt, FinishedAt, WinnerCharName, EndReason
FROM [dbo].[PVP_Matches] WITH (NOLOCK)
WHERE @Term = N'' OR ChallengerCharName LIKE N'%' + @Term + N'%' OR OpponentCharName LIKE N'%' + @Term + N'%' OR CONVERT(NVARCHAR(32), MatchID) = @Term
ORDER BY MatchID DESC;", term);
    }

    public async Task AddPvpArenaAsync(string name, int worldId, int regionId, int posX, int posY, int posZ, int sortOrder, bool enabled)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await EnsureTableAsync(connection, "[dbo].[PVP_Arenas]");
        await using var command = new SqlCommand(@"
INSERT INTO [dbo].[PVP_Arenas] (Enabled, ArenaName, GameWorldID, RegionID, PosX, PosY, PosZ, SortOrder, UpdatedAt)
VALUES (@Enabled, @ArenaName, @GameWorldID, @RegionID, @PosX, @PosY, @PosZ, @SortOrder, SYSUTCDATETIME());", connection);
        command.Parameters.Add("@Enabled", SqlDbType.Bit).Value = enabled;
        command.Parameters.Add("@ArenaName", SqlDbType.NVarChar, 64).Value = name.Trim();
        command.Parameters.Add("@GameWorldID", SqlDbType.Int).Value = worldId;
        command.Parameters.Add("@RegionID", SqlDbType.Int).Value = regionId;
        command.Parameters.Add("@PosX", SqlDbType.Int).Value = posX;
        command.Parameters.Add("@PosY", SqlDbType.Int).Value = posY;
        command.Parameters.Add("@PosZ", SqlDbType.Int).Value = posZ;
        command.Parameters.Add("@SortOrder", SqlDbType.Int).Value = sortOrder;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "AddPvpArena", name, $"World={worldId};Region={regionId}");
    }

    public async Task SetPvpArenaEnabledAsync(int arenaId, bool enabled)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();
        await using var command = new SqlCommand("UPDATE [dbo].[PVP_Arenas] SET Enabled = @Enabled, UpdatedAt = SYSUTCDATETIME() WHERE ArenaID = @ArenaID", connection);
        command.Parameters.Add("@ArenaID", SqlDbType.Int).Value = arenaId;
        command.Parameters.Add("@Enabled", SqlDbType.Bit).Value = enabled;
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "SetPvpArenaEnabled", $"ArenaID={arenaId}", enabled.ToString());
    }

    public IReadOnlyList<ManagedTableInfo> GetManagedTables()
    {
        return ManagedTables;
    }

    public async Task<IReadOnlyList<ManagedColumnInfo>> LoadManagedTableColumnsAsync(string tableKey)
    {
        var spec = ResolveManagedTable(tableKey);
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        return await GetManagedColumnsAsync(connection, spec.TableName);
    }

    public async Task<DataTable> LoadManagedTableRowsAsync(string tableKey, string search)
    {
        var spec = ResolveManagedTable(tableKey);
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        if (!await AnyObjectExistsAsync(connection, spec.TableName))
            return new DataTable();

        var columns = (await GetManagedColumnsAsync(connection, spec.TableName)).ToList();
        if (columns.Count == 0)
            return new DataTable();

        var tableName = QuoteObjectName(spec.TableName);
        var searchableColumns = columns
            .Where(column => IsTextType(column.DataType))
            .Select(column => column.Name)
            .ToList();

        var where = string.Empty;
        if (!string.IsNullOrWhiteSpace(search) && searchableColumns.Count > 0)
        {
            where = "WHERE " + string.Join(" OR ", searchableColumns.Select(column => $"{QuoteName(column)} LIKE N'%' + @Term + N'%'"));
        }

        var orderColumn = columns.FirstOrDefault(column => column.IsPrimaryKey)?.Name
            ?? columns.FirstOrDefault(column => column.IsIdentity)?.Name
            ?? columns[0].Name;

        var sql = $"SELECT TOP (300) * FROM {tableName} WITH (NOLOCK) {where} ORDER BY {QuoteName(orderColumn)} DESC";
        return await QueryTableAsync(connection, sql, new SqlParameter("@Term", search.Trim()));
    }

    public async Task SaveManagedTableRowAsync(string tableKey, IReadOnlyDictionary<string, string?> values)
    {
        var spec = ResolveManagedTable(tableKey);
        if (!spec.AllowInsert && !spec.AllowUpdate)
            throw new InvalidOperationException("This table is read-only in Data Studio.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        var columns = (await GetManagedColumnsAsync(connection, spec.TableName)).ToList();
        var pk = columns.FirstOrDefault(column => column.IsPrimaryKey)
            ?? columns.FirstOrDefault(column => column.IsIdentity);
        values.TryGetValue(pk?.Name ?? string.Empty, out var pkValue);
        var hasPkValue = pk is not null && !string.IsNullOrWhiteSpace(pkValue);
        var isUpdate = hasPkValue;

        if (isUpdate && !spec.AllowUpdate)
            throw new InvalidOperationException("Updates are disabled for this table.");
        if (!isUpdate && !spec.AllowInsert)
            throw new InvalidOperationException("Inserts are disabled for this table.");

        var editableColumns = columns
            .Where(column => !column.IsComputed)
            .Where(column => !column.DataType.Equals("timestamp", StringComparison.OrdinalIgnoreCase) &&
                             !column.DataType.Equals("rowversion", StringComparison.OrdinalIgnoreCase))
            .Where(column => !(isUpdate && column.IsPrimaryKey))
            .Where(column => !(column.IsIdentity && !isUpdate))
            .Where(column => values.ContainsKey(column.Name))
            .ToList();

        if (editableColumns.Count == 0)
            throw new InvalidOperationException("No editable columns were provided.");

        var tableName = QuoteObjectName(spec.TableName);
        var sql = isUpdate
            ? $"UPDATE {tableName} SET {string.Join(", ", editableColumns.Select(column => $"{QuoteName(column.Name)} = @{column.Name}"))} WHERE {QuoteName(pk!.Name)} = @__Pk"
            : $"INSERT INTO {tableName} ({string.Join(", ", editableColumns.Select(column => QuoteName(column.Name)))}) VALUES ({string.Join(", ", editableColumns.Select(column => $"@{column.Name}"))})";

        await using var command = new SqlCommand(sql, connection);
        foreach (var column in editableColumns)
            command.Parameters.AddWithValue($"@{column.Name}", ConvertManagedValue(column, values[column.Name]));

        if (isUpdate)
            command.Parameters.AddWithValue("@__Pk", ConvertManagedValue(pk!, pkValue));

        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, isUpdate ? "UpdateManagedTableRow" : "InsertManagedTableRow", spec.TableName, isUpdate ? $"{pk!.Name}={pkValue}" : "New row");
    }

    public async Task DeleteManagedTableRowAsync(string tableKey, IReadOnlyDictionary<string, string?> values)
    {
        var spec = ResolveManagedTable(tableKey);
        if (!spec.AllowDelete)
            throw new InvalidOperationException("Deletes are disabled for this table.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        var columns = (await GetManagedColumnsAsync(connection, spec.TableName)).ToList();
        var pk = columns.FirstOrDefault(column => column.IsPrimaryKey)
            ?? columns.FirstOrDefault(column => column.IsIdentity);

        if (pk is null || !values.TryGetValue(pk.Name, out var pkValue) || string.IsNullOrWhiteSpace(pkValue))
            throw new InvalidOperationException("This table needs a primary key value before delete.");

        await using var command = new SqlCommand($"DELETE FROM {QuoteObjectName(spec.TableName)} WHERE {QuoteName(pk.Name)} = @Pk", connection);
        command.Parameters.AddWithValue("@Pk", ConvertManagedValue(pk, pkValue));
        await command.ExecuteNonQueryAsync();
        await AuditAsync(connection, "DeleteManagedTableRow", spec.TableName, $"{pk.Name}={pkValue}");
    }

    public async Task<DataTable> LoadDiagnosticsAsync()
    {
        var table = new DataTable();
        table.Columns.Add("Module", typeof(string));
        table.Columns.Add("ObjectName", typeof(string));
        table.Columns.Add("Type", typeof(string));
        table.Columns.Add("Status", typeof(string));
        table.Columns.Add("Notes", typeof(string));

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        foreach (var spec in DiagnosticSpecs)
        {
            var exists = spec.Type == "*"
                ? await AnyObjectExistsAsync(connection, spec.ObjectName)
                : await ObjectExistsAsync(connection, spec.ObjectName, spec.Type);

            table.Rows.Add(
                spec.Module,
                spec.ObjectName,
                ToFriendlyObjectType(spec.Type),
                exists ? "Ready" : spec.Required ? "Missing" : "Optional",
                exists ? "Available in current database." : spec.Notes);
        }

        return table;
    }

    public async Task<IReadOnlyList<MigrationStatus>> CheckMigrationsAsync(string migrationsPath)
    {
        if (!Directory.Exists(migrationsPath))
            return Array.Empty<MigrationStatus>();

        var files = DiscoverMigrationFiles(migrationsPath);

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        var result = new List<MigrationStatus>();
        foreach (var file in files)
        {
            var text = await File.ReadAllTextAsync(file);
            var objectNames = ExtractObjectNames(text).ToList();

            if (objectNames.Count == 0)
            {
                result.Add(new MigrationStatus
                {
                    Name = Path.GetRelativePath(migrationsPath, file),
                    Status = "Available",
                    Details = "No simple object check detected",
                    FullPath = file
                });
                continue;
            }

            var found = 0;
            foreach (var objectName in objectNames)
            {
                if (await AnyObjectExistsAsync(connection, objectName))
                    found++;
            }

            result.Add(new MigrationStatus
            {
                Name = Path.GetRelativePath(migrationsPath, file),
                Status = found == objectNames.Count ? "Looks applied" : found == 0 ? "Not found" : "Partial",
                Details = $"{found}/{objectNames.Count} checked objects found",
                FullPath = file
            });
        }

        return result;
    }

    public async Task<int> RunMigrationAsync(string filePath, string migrationsRoot)
    {
        var root = Path.GetFullPath(migrationsRoot);
        var fullPath = Path.GetFullPath(filePath);
        var relativePath = Path.GetRelativePath(root, fullPath);

        if (Path.IsPathRooted(relativePath) ||
            relativePath.Equals("..", StringComparison.Ordinal) ||
            relativePath.StartsWith($"..{Path.DirectorySeparatorChar}", StringComparison.Ordinal))
            throw new InvalidOperationException("Migration file must be inside the packaged database folder.");

        if (!File.Exists(fullPath) || !fullPath.EndsWith(".sql", StringComparison.OrdinalIgnoreCase))
            throw new InvalidOperationException("Select a valid .sql migration file.");

        var sql = await File.ReadAllTextAsync(fullPath);
        var preserveDatabaseSwitches = Regex.IsMatch(
            sql,
            @"CREATE\s+DATABASE\s+\[?Events\]?|USE\s+\[?Events\]?",
            RegexOptions.IgnoreCase);
        var batches = SplitMigrationBatches(sql, preserveDatabaseSwitches).ToList();
        if (batches.Count == 0)
            throw new InvalidOperationException("Migration file has no executable SQL batches.");

        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync();

        if (preserveDatabaseSwitches)
        {
            foreach (var batch in batches)
            {
                await using var command = new SqlCommand(batch, connection)
                {
                    CommandTimeout = 180
                };
                await command.ExecuteNonQueryAsync();
            }

            await using var auditConnection = new SqlConnection(_connectionString);
            await auditConnection.OpenAsync();
            await AuditAsync(auditConnection, "RunMigration", Path.GetFileName(fullPath), $"{batches.Count} batch(es)");
            return batches.Count;
        }

        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync();

        try
        {
            foreach (var batch in batches)
            {
                await using var command = new SqlCommand(batch, connection, transaction)
                {
                    CommandTimeout = 180
                };
                await command.ExecuteNonQueryAsync();
            }

            await transaction.CommitAsync();
            await AuditAsync(connection, "RunMigration", Path.GetFileName(fullPath), $"{batches.Count} batch(es)");
            return batches.Count;
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }
    }

    internal static IReadOnlyList<string> DiscoverMigrationFiles(string databasePath)
    {
        var versionDirectories = Directory.EnumerateDirectories(databasePath, "v*", SearchOption.TopDirectoryOnly)
            .Select(path => new
            {
                Path = path,
                ParsedVersion = Version.TryParse(Path.GetFileName(path).TrimStart('v', 'V'), out var version)
                    ? version
                    : null
            })
            .Where(entry => entry.ParsedVersion is not null)
            .OrderBy(entry => entry.ParsedVersion)
            .ThenBy(entry => entry.Path, StringComparer.OrdinalIgnoreCase)
            .ToList();

        if (versionDirectories.Count > 0)
        {
            return versionDirectories
                .SelectMany(entry => Directory.EnumerateFiles(entry.Path, "*.sql", SearchOption.TopDirectoryOnly)
                    .OrderBy(Path.GetFileName, StringComparer.OrdinalIgnoreCase))
                .ToList();
        }

        var legacyMigrationsPath = Path.Combine(databasePath, "migrations");
        var sourcePath = Directory.Exists(legacyMigrationsPath)
            ? legacyMigrationsPath
            : databasePath;
        return Directory.EnumerateFiles(sourcePath, "*.sql", SearchOption.TopDirectoryOnly)
            .OrderBy(Path.GetFileName, StringComparer.OrdinalIgnoreCase)
            .ToList();
    }

    private static IEnumerable<string> ExtractObjectNames(string sql)
    {
        var names = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var patterns = new[]
        {
            @"OBJECT_ID\s*\(\s*N?'(?<name>(?:[A-Za-z0-9_]+\.)?dbo\.[^']+)'",
            @"COL_LENGTH\s*\(\s*N?'(?<name>(?:[A-Za-z0-9_]+\.)?dbo\.[^']+)'",
            @"CREATE\s+TABLE\s+(?<name>(?:[A-Za-z0-9_]+\.)?dbo\.[A-Za-z0-9_]+)",
            @"CREATE\s+VIEW\s+(?<name>(?:[A-Za-z0-9_]+\.)?dbo\.[A-Za-z0-9_]+)"
        };

        foreach (var pattern in patterns)
        {
            foreach (Match match in Regex.Matches(sql, pattern, RegexOptions.IgnoreCase))
                names.Add(match.Groups["name"].Value.Trim('[', ']'));
        }

        return names;
    }

    private static IEnumerable<string> SplitMigrationBatches(string sql, bool preserveUseLines = false)
    {
        var current = new List<string>();
        using var reader = new StringReader(sql);

        while (reader.ReadLine() is { } line)
        {
            if (Regex.IsMatch(line, @"^\s*GO\s*(?:--.*)?$", RegexOptions.IgnoreCase))
            {
                var batch = NormalizeMigrationBatch(current, preserveUseLines);
                if (!string.IsNullOrWhiteSpace(batch))
                    yield return batch;

                current.Clear();
                continue;
            }

            current.Add(line);
        }

        var finalBatch = NormalizeMigrationBatch(current, preserveUseLines);
        if (!string.IsNullOrWhiteSpace(finalBatch))
            yield return finalBatch;
    }

    private static string NormalizeMigrationBatch(IEnumerable<string> lines, bool preserveUseLines = false)
    {
        return string.Join(Environment.NewLine, lines
            .Where(line => preserveUseLines || !Regex.IsMatch(line, @"^\s*USE\s+\[[^\]]+\]\s*$", RegexOptions.IgnoreCase)))
            .Trim();
    }

    private static string ToFriendlyObjectType(string type)
    {
        return type switch
        {
            "U" => "Table",
            "V" => "View",
            "P" => "Procedure",
            "*" => "Object",
            _ => type
        };
    }

    private static readonly IReadOnlyList<DiagnosticSpec> DiagnosticSpecs = new[]
    {
        new DiagnosticSpec("Core", "[dbo].[System_Settings]", "U", true, "Run the settings/catalog migration."),
        new DiagnosticSpec("Core", "dbo.vw_Settings_Organized", "V", false, "Optional organized settings view."),
        new DiagnosticSpec("Core", "dbo.vw_Settings_InvalidValues", "V", false, "Optional invalid-value checker view."),
        new DiagnosticSpec("Runtime", "[dbo].[Command_FilterQueue]", "U", true, "Required for notices, disconnects, and reload commands."),
        new DiagnosticSpec("Scheduler", "[dbo].[System_Schedule]", "U", false, "Required only when scheduler jobs are used."),
        new DiagnosticSpec("Rewards", "[dbo].[Item_Chest]", "U", true, "Required for item chest reward management."),
        new DiagnosticSpec("Rewards", "[dbo].[Item_AddChest]", "P", true, "Required for granting rewards."),
        new DiagnosticSpec("Lucky Spin", "[dbo].[LuckySpin_Rewards]", "U", false, "Run the Lucky Spin migration."),
        new DiagnosticSpec("Lucky Spin", "[dbo].[LuckySpin_Log]", "U", false, "Lucky Spin purchase log table."),
        new DiagnosticSpec("Special Offers", "[dbo].[Offer_List]", "U", false, "Run the Special Offers migration."),
        new DiagnosticSpec("Special Offers", "[dbo].[Offer_PurchaseLog]", "U", false, "Special Offers purchase log table."),
        new DiagnosticSpec("Killer Animations", "[dbo].[KillerAnimation_List]", "U", false, "Run database/v1.0.0/20260713_killer_animations.sql."),
        new DiagnosticSpec("Killer Animations", "[dbo].[KillerAnimation_Owned]", "U", false, "Owned killer-animation rows."),
        new DiagnosticSpec("Killer Animations", "[dbo].[KillerAnimation_Active]", "U", false, "Active killer-animation selection per character."),
        new DiagnosticSpec("Economy", "[dbo].[Stall_SilkTransactions]", "U", false, "Run the silk stall transactions migration."),
        new DiagnosticSpec("Security", "[dbo].[Security_BlockedWords]", "U", false, "Run the chat support migration."),
        new DiagnosticSpec("Security", "[dbo].[Log_Chat]", "U", false, "Run the chat support migration."),
        new DiagnosticSpec("Security", "[dbo].[Auth_HWIDs]", "U", false, "HWID support table from the filter database."),
        new DiagnosticSpec("Client UI", "[dbo].[Web_Buttons]", "U", false, "Run the WebViewer buttons migration."),
        new DiagnosticSpec("Packets", "[dbo].[Security_Whitelist]", "U", false, "Packet whitelist table."),
        new DiagnosticSpec("Packets", "[dbo].[Security_Blacklist]", "U", false, "Packet blacklist table."),
        new DiagnosticSpec("Discord", "[dbo].[DiscordNotificationSettings]", "U", false, "Run the Discord notifications update."),
        new DiagnosticSpec("Discord", "[dbo].[DiscordNotificationChannels]", "U", false, "Customer-defined Discord destination cards."),
        new DiagnosticSpec("Discord", "[dbo].[DiscordNotificationQueue]", "U", false, "Durable Discord delivery queue."),
        new DiagnosticSpec("Discord", "[dbo].[Discord_Notification]", "P", false, "SQL notification entry point."),
        new DiagnosticSpec("Auto Events", "Events.dbo._AutoEventConfig", "U", false, "Run database/v1.0.0/20260711_events_database.sql."),
        new DiagnosticSpec("Auto Events", "Events.dbo._AutoEventCommandQueue", "U", false, "Required for auto-event runtime commands."),
        new DiagnosticSpec("Auto Events", "Events.dbo._AutoEventRun", "U", false, "Auto-event run history."),
        new DiagnosticSpec("Auto Events", "Events.dbo._AutoEventWinnerLog", "U", false, "Auto-event winner history."),
        new DiagnosticSpec("PvP Challenge", "[dbo].[PVP_Settings]", "U", false, "Run the PvP Challenge migration."),
        new DiagnosticSpec("PvP Challenge", "[dbo].[PVP_Arenas]", "U", false, "Arena pool table."),
        new DiagnosticSpec("PvP Challenge", "[dbo].[PVP_Matches]", "U", false, "PvP match history."),
        new DiagnosticSpec("Audit", "[dbo].[Admin_AuditLog]", "U", false, "Created automatically when audit is opened.")
    };

    private sealed record DiagnosticSpec(string Module, string ObjectName, string Type, bool Required, string Notes);

    private static async Task<bool> ObjectExistsAsync(SqlConnection connection, string name, string type)
    {
        await using var command = new SqlCommand("SELECT CASE WHEN OBJECT_ID(@Name, @Type) IS NULL THEN 0 ELSE 1 END", connection);
        command.Parameters.Add("@Name", SqlDbType.NVarChar, 256).Value = name;
        command.Parameters.Add("@Type", SqlDbType.NVarChar, 8).Value = type;
        return Convert.ToInt32(await command.ExecuteScalarAsync()) == 1;
    }

    private static async Task EnsureTableAsync(SqlConnection connection, string tableName)
    {
        if (!await ObjectExistsAsync(connection, tableName, "U"))
            throw new InvalidOperationException($"{tableName} was not found. Run its migration first.");
    }

    private static async Task AuditAsync(
        SqlConnection connection,
        string action,
        string target,
        string details,
        SqlTransaction? transaction = null)
    {
        await EnsureAuditTableAsync(connection, transaction);
        await using var command = new SqlCommand(@"
INSERT INTO [dbo].[Admin_AuditLog] (AdminName, Action, Target, Details, MachineName)
VALUES (@AdminName, @Action, @Target, @Details, @MachineName);", connection, transaction);
        command.Parameters.Add("@AdminName", SqlDbType.NVarChar, 64).Value = AppSession.CurrentAdmin;
        command.Parameters.Add("@Action", SqlDbType.NVarChar, 128).Value = action;
        command.Parameters.Add("@Target", SqlDbType.NVarChar, 256).Value = target;
        command.Parameters.Add("@Details", SqlDbType.NVarChar, 1024).Value = details;
        command.Parameters.Add("@MachineName", SqlDbType.NVarChar, 128).Value = Environment.MachineName;
        await command.ExecuteNonQueryAsync();
    }

    private static async Task EnsureAuditTableAsync(
        SqlConnection connection,
        SqlTransaction? transaction = null)
    {
        await using var command = new SqlCommand(@"
IF OBJECT_ID(N'[dbo].[Admin_AuditLog]', N'U') IS NULL
BEGIN
    CREATE TABLE [dbo].[Admin_AuditLog]
    (
        ID BIGINT IDENTITY(1,1) NOT NULL CONSTRAINT PK_AdminAuditLog PRIMARY KEY,
        CreatedAt DATETIME2(0) NOT NULL CONSTRAINT DF_AdminAuditLog_CreatedAt DEFAULT (SYSUTCDATETIME()),
        AdminName NVARCHAR(64) NOT NULL,
        Action NVARCHAR(128) NOT NULL,
        Target NVARCHAR(256) NULL,
        Details NVARCHAR(1024) NULL,
        MachineName NVARCHAR(128) NULL
    );
END;", connection, transaction);
        await command.ExecuteNonQueryAsync();
    }

    internal static void AddSchedulerDisplayColumns(DataTable table, DateTime now)
    {
        if (!table.Columns.Contains("ScheduleSummary"))
            table.Columns.Add("ScheduleSummary", typeof(string));
        if (!table.Columns.Contains("NextRunLocal"))
            table.Columns.Add("NextRunLocal", typeof(DateTime));
        if (!table.Columns.Contains("JobState"))
            table.Columns.Add("JobState", typeof(string));

        var hasRunningToken = table.Columns.Contains("RunningToken");
        var hasLeaseUntilUtc = table.Columns.Contains("LeaseUntilUtc");
        foreach (DataRow row in table.Rows)
        {
            var repeatType = Convert.ToString(row["RepeatType"]) ?? "None";
            var time = row["Time"] == DBNull.Value ? TimeSpan.Zero : (TimeSpan)row["Time"];
            var intervalSeconds = row["IntervalSeconds"] == DBNull.Value
                ? (int?)null
                : Convert.ToInt32(row["IntervalSeconds"]);
            var startDateTime = row["StartDateTime"] == DBNull.Value
                ? (DateTime?)null
                : Convert.ToDateTime(row["StartDateTime"]);
            var scheduledDate = row["ScheduledDate"] == DBNull.Value
                ? (DateTime?)null
                : Convert.ToDateTime(row["ScheduledDate"]);
            var daysMask = row["DaysOfWeekMask"] == DBNull.Value
                ? (byte?)null
                : Convert.ToByte(row["DaysOfWeekMask"]);
            if (!daysMask.HasValue &&
                row["RepeatDayOfWeek"] != DBNull.Value &&
                Convert.ToInt32(row["RepeatDayOfWeek"]) is >= 1 and <= 7)
            {
                daysMask = (byte)(1 << (Convert.ToInt32(row["RepeatDayOfWeek"]) - 1));
            }

            row["ScheduleSummary"] = BuildScheduleSummary(
                repeatType,
                scheduledDate,
                startDateTime,
                time,
                intervalSeconds,
                daysMask);

            var enabled = row["IsEnabled"] != DBNull.Value && Convert.ToBoolean(row["IsEnabled"]);
            var running = hasRunningToken &&
                          hasLeaseUntilUtc &&
                          row["RunningToken"] != DBNull.Value &&
                          row["LeaseUntilUtc"] != DBNull.Value &&
                          Convert.ToDateTime(row["LeaseUntilUtc"]) >= DateTime.UtcNow;
            var lastStatus = row["LastStatus"] == DBNull.Value
                ? string.Empty
                : Convert.ToString(row["LastStatus"]) ?? string.Empty;
            row["JobState"] = running
                ? "Running now"
                : !enabled
                    ? "Paused"
                    : string.IsNullOrWhiteSpace(lastStatus)
                        ? "Ready"
                        : lastStatus;

            if (!enabled)
                continue;

            var nextRun = CalculateNextSchedulerRun(
                repeatType,
                scheduledDate,
                startDateTime,
                time,
                intervalSeconds,
                daysMask,
                now);
            if (nextRun.HasValue)
                row["NextRunLocal"] = nextRun.Value;
        }
    }

    private static string BuildScheduleSummary(
        string repeatType,
        DateTime? scheduledDate,
        DateTime? startDateTime,
        TimeSpan time,
        int? intervalSeconds,
        byte? daysMask)
    {
        if (repeatType.Equals("Interval", StringComparison.OrdinalIgnoreCase))
        {
            var interval = intervalSeconds.GetValueOrDefault();
            var friendly = interval % 86400 == 0
                ? $"Every {interval / 86400} day(s)"
                : interval % 3600 == 0
                    ? $"Every {interval / 3600} hour(s)"
                    : interval % 60 == 0
                        ? $"Every {interval / 60} minute(s)"
                        : $"Every {interval} second(s)";
            return startDateTime.HasValue
                ? $"{friendly} · from {startDateTime:yyyy-MM-dd HH:mm}"
                : friendly;
        }

        if (repeatType.Equals("Daily", StringComparison.OrdinalIgnoreCase))
            return $"Every day · {time:hh\\:mm}";

        if (repeatType.Equals("Weekly", StringComparison.OrdinalIgnoreCase))
        {
            var names = new[] { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
            var selected = Enumerable.Range(0, 7)
                .Where(index => (daysMask.GetValueOrDefault() & (1 << index)) != 0)
                .Select(index => names[index]);
            return $"{string.Join(", ", selected)} · {time:hh\\:mm}";
        }

        return scheduledDate.HasValue
            ? $"Once · {scheduledDate.Value.Date.Add(time):yyyy-MM-dd HH:mm}"
            : "Once · date required";
    }

    private static DateTime? CalculateNextSchedulerRun(
        string repeatType,
        DateTime? scheduledDate,
        DateTime? startDateTime,
        TimeSpan time,
        int? intervalSeconds,
        byte? daysMask,
        DateTime now)
    {
        if (repeatType.Equals("Interval", StringComparison.OrdinalIgnoreCase))
        {
            if (!startDateTime.HasValue ||
                !intervalSeconds.HasValue ||
                intervalSeconds is < 10 or > 604800)
                return null;
            if (startDateTime.Value >= now)
                return startDateTime.Value;

            var intervalTicks = TimeSpan.FromSeconds(intervalSeconds.Value).Ticks;
            var elapsedTicks = now.Ticks - startDateTime.Value.Ticks;
            var completedIntervals = elapsedTicks / intervalTicks;
            var candidate = startDateTime.Value.AddTicks(completedIntervals * intervalTicks);
            return candidate >= now ? candidate : candidate.AddTicks(intervalTicks);
        }

        if (repeatType.Equals("Daily", StringComparison.OrdinalIgnoreCase))
        {
            var candidate = now.Date.Add(time);
            return candidate >= now ? candidate : candidate.AddDays(1);
        }

        if (repeatType.Equals("Weekly", StringComparison.OrdinalIgnoreCase))
        {
            var mask = daysMask.GetValueOrDefault();
            if (mask is < 1 or > 127)
                return null;
            for (var daysAhead = 0; daysAhead <= 7; daysAhead++)
            {
                var date = now.Date.AddDays(daysAhead);
                var dayNumber = date.DayOfWeek == DayOfWeek.Sunday ? 7 : (int)date.DayOfWeek;
                if ((mask & (1 << (dayNumber - 1))) == 0)
                    continue;
                var candidate = date.Add(time);
                if (candidate >= now)
                    return candidate;
            }
            return null;
        }

        if (!scheduledDate.HasValue)
            return null;
        var oneTime = scheduledDate.Value.Date.Add(time);
        return oneTime >= now ? oneTime : null;
    }

    private static byte FirstDayFromMask(byte mask)
    {
        for (byte day = 1; day <= 7; day++)
        {
            if ((mask & (1 << (day - 1))) != 0)
                return day;
        }
        return 1;
    }

    private static async Task EnsureSchedulerTableAsync(SqlConnection connection)
    {
        await using var command = new SqlCommand(@"
IF OBJECT_ID(N'[dbo].[System_Schedule]', N'U') IS NULL
BEGIN
    CREATE TABLE [dbo].[System_Schedule]
    (
        Idx INT IDENTITY(1,1) NOT NULL CONSTRAINT PK__Scheduler PRIMARY KEY,
        Name NVARCHAR(128) NOT NULL,
        Query NVARCHAR(512) NOT NULL,
        ScheduledDate DATE NULL,
        StartDateTime DATETIME2(0) NULL,
        Time TIME NOT NULL,
        RepeatType NVARCHAR(16) NOT NULL CONSTRAINT DF__Scheduler_RepeatType DEFAULT(N'None'),
        RepeatDayOfWeek TINYINT NULL,
        DaysOfWeekMask TINYINT NULL,
        IntervalSeconds INT NULL,
        IsEnabled BIT NOT NULL CONSTRAINT DF__Scheduler_IsEnabled DEFAULT(1),
        LastRunDateTime DATETIME2(0) NULL,
        CreatedAtUtc DATETIME2(0) NOT NULL CONSTRAINT DF_SystemSchedule_CreatedAtUtc DEFAULT(SYSUTCDATETIME()),
        UpdatedAtUtc DATETIME2(0) NOT NULL CONSTRAINT DF_SystemSchedule_UpdatedAtUtc DEFAULT(SYSUTCDATETIME())
    );
END;

IF COL_LENGTH(N'dbo.System_Schedule', N'StartDateTime') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD StartDateTime DATETIME2(0) NULL;');
IF COL_LENGTH(N'dbo.System_Schedule', N'DaysOfWeekMask') IS NULL
BEGIN
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD DaysOfWeekMask TINYINT NULL;');
    EXEC(N'UPDATE dbo.System_Schedule SET DaysOfWeekMask = CONVERT(TINYINT, POWER(CONVERT(FLOAT, 2), RepeatDayOfWeek - 1)) WHERE RepeatType = N''Weekly'' AND RepeatDayOfWeek BETWEEN 1 AND 7;');
END;
IF COL_LENGTH(N'dbo.System_Schedule', N'IntervalSeconds') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD IntervalSeconds INT NULL;');
IF COL_LENGTH(N'dbo.System_Schedule', N'CreatedAtUtc') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD CreatedAtUtc DATETIME2(0) NOT NULL CONSTRAINT DF_SystemSchedule_CreatedAtUtc DEFAULT(SYSUTCDATETIME());');
IF COL_LENGTH(N'dbo.System_Schedule', N'UpdatedAtUtc') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD UpdatedAtUtc DATETIME2(0) NOT NULL CONSTRAINT DF_SystemSchedule_UpdatedAtUtc DEFAULT(SYSUTCDATETIME());');
IF COL_LENGTH(N'dbo.System_Schedule', N'ExecutionTimeoutSeconds') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD ExecutionTimeoutSeconds INT NOT NULL CONSTRAINT DF_SystemSchedule_ExecutionTimeout DEFAULT(7200);');
IF COL_LENGTH(N'dbo.System_Schedule', N'CatchUpWindowSeconds') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD CatchUpWindowSeconds INT NOT NULL CONSTRAINT DF_SystemSchedule_CatchUpWindow DEFAULT(300);');
IF COL_LENGTH(N'dbo.System_Schedule', N'LastScheduledDateTime') IS NULL
BEGIN
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD LastScheduledDateTime DATETIME2(0) NULL;');
    EXEC(N'UPDATE dbo.System_Schedule SET LastScheduledDateTime = LastRunDateTime WHERE LastRunDateTime IS NOT NULL;');
END;
IF COL_LENGTH(N'dbo.System_Schedule', N'RunningToken') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD RunningToken UNIQUEIDENTIFIER NULL;');
IF COL_LENGTH(N'dbo.System_Schedule', N'RunningBy') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD RunningBy NVARCHAR(160) NULL;');
IF COL_LENGTH(N'dbo.System_Schedule', N'RunningSinceUtc') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD RunningSinceUtc DATETIME2(0) NULL;');
IF COL_LENGTH(N'dbo.System_Schedule', N'LeaseUntilUtc') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD LeaseUntilUtc DATETIME2(0) NULL;');
IF COL_LENGTH(N'dbo.System_Schedule', N'LastStatus') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD LastStatus NVARCHAR(16) NULL;');
IF COL_LENGTH(N'dbo.System_Schedule', N'LastError') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD LastError NVARCHAR(2048) NULL;');
IF COL_LENGTH(N'dbo.System_Schedule', N'LastDurationMs') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD LastDurationMs BIGINT NULL;');
IF COL_LENGTH(N'dbo.System_Schedule', N'LastCompletedDateTimeUtc') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD LastCompletedDateTimeUtc DATETIME2(0) NULL;');

IF OBJECT_ID(N'dbo.System_ScheduleHistory', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.System_ScheduleHistory
    (
        RunID BIGINT IDENTITY(1,1) NOT NULL CONSTRAINT PK_SystemScheduleHistory PRIMARY KEY,
        JobId INT NOT NULL,
        JobName NVARCHAR(128) NOT NULL,
        TriggerType NVARCHAR(16) NOT NULL,
        ScheduledFor DATETIME2(0) NULL,
        StartedAtUtc DATETIME2(0) NOT NULL,
        CompletedAtUtc DATETIME2(0) NOT NULL,
        Status NVARCHAR(16) NOT NULL,
        DurationMs BIGINT NOT NULL,
        ExecutedBy NVARCHAR(160) NULL,
        Error NVARCHAR(2048) NULL
    );
    CREATE INDEX IX_SystemScheduleHistory_JobRun
        ON dbo.System_ScheduleHistory(JobId, RunID DESC);
END;", connection)
        {
            CommandTimeout = 120
        };
        await command.ExecuteNonQueryAsync();
    }

    private enum SchedulerRepeatType
    {
        None,
        Interval,
        Daily,
        Weekly
    }

    private static async Task EnsureFilterRegionControlTableAsync(SqlConnection connection)
    {
        await using var command = new SqlCommand(@"
IF OBJECT_ID(N'[dbo].[Security_RegionFeatures]', N'U') IS NULL
BEGIN
    CREATE TABLE [dbo].[Security_RegionFeatures]
    (
        ID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_Security_RegionFeatures PRIMARY KEY,
        WorldID int NOT NULL CONSTRAINT DF_RegionFeatures_World DEFAULT (0),
        RegionID int NOT NULL,
        RuleName nvarchar(64) NULL,
        Enabled bit NOT NULL CONSTRAINT DF_RegionFeatures_Enabled DEFAULT (1),
        BuildMode tinyint NOT NULL CONSTRAINT DF_RegionFeatures_Build DEFAULT (0),
        JobMode tinyint NOT NULL CONSTRAINT DF_RegionFeatures_Job DEFAULT (0),
        RaceMode tinyint NOT NULL CONSTRAINT DF_RegionFeatures_Race DEFAULT (0),
        PartyMode tinyint NOT NULL CONSTRAINT DF_RegionFeatures_PartyMode DEFAULT (0),
        MinLevel tinyint NOT NULL CONSTRAINT DF_RegionFeatures_MinLevel DEFAULT (0),
        MaxLevel tinyint NOT NULL CONSTRAINT DF_RegionFeatures_MaxLevel DEFAULT (0),
        AllowTeleport bit NOT NULL CONSTRAINT DF_RegionFeatures_Teleport DEFAULT (1),
        AllowReverse bit NOT NULL CONSTRAINT DF_RegionFeatures_Reverse DEFAULT (1),
        AllowTrace bit NOT NULL CONSTRAINT DF_RegionFeatures_Trace DEFAULT (1),
        AllowMovement bit NOT NULL CONSTRAINT DF_RegionFeatures_Movement DEFAULT (1),
        AllowChat bit NOT NULL CONSTRAINT DF_RegionFeatures_Chat DEFAULT (1),
        AllowGlobalChat bit NOT NULL CONSTRAINT DF_RegionFeatures_Global DEFAULT (1),
        AllowParty bit NOT NULL CONSTRAINT DF_RegionFeatures_Party DEFAULT (1),
        AllowExchange bit NOT NULL CONSTRAINT DF_RegionFeatures_Exchange DEFAULT (1),
        AllowStall bit NOT NULL CONSTRAINT DF_RegionFeatures_Stall DEFAULT (1),
        AllowPvP bit NOT NULL CONSTRAINT DF_RegionFeatures_PvP DEFAULT (1),
        AllowAlchemy bit NOT NULL CONSTRAINT DF_RegionFeatures_Alchemy DEFAULT (1),
        AllowSpecialItems bit NOT NULL CONSTRAINT DF_RegionFeatures_Items DEFAULT (1),
        AllowBerserk bit NOT NULL CONSTRAINT DF_RegionFeatures_Berserk DEFAULT (1),
        AutoPvpCape tinyint NOT NULL CONSTRAINT DF_RegionFeatures_Cape DEFAULT (0),
        InactivityReturnSeconds int NOT NULL CONSTRAINT DF_RegionFeatures_Inactivity DEFAULT (0),
        EventSuitMode tinyint NOT NULL CONSTRAINT DF_RegionFeatures_EventSuit DEFAULT (0),
        ManagedEventCode nvarchar(32) NULL,
        ManagedAtUtc datetime2(0) NULL,
        CreatedAt datetime2(0) NOT NULL CONSTRAINT DF_RegionFeatures_Created DEFAULT (SYSUTCDATETIME()),
        UpdatedAt datetime2(0) NULL,
        CONSTRAINT CK_RegionFeatures_Region CHECK (RegionID BETWEEN -32768 AND 32767 AND RegionID<>0),
        CONSTRAINT CK_RegionFeatures_Modes CHECK (BuildMode BETWEEN 0 AND 3 AND JobMode BETWEEN 0 AND 5 AND RaceMode BETWEEN 0 AND 2 AND PartyMode BETWEEN 0 AND 2 AND EventSuitMode BETWEEN 0 AND 2),
        CONSTRAINT CK_RegionFeatures_Values CHECK ((MaxLevel=0 OR MinLevel=0 OR MaxLevel>=MinLevel) AND AutoPvpCape BETWEEN 0 AND 5 AND InactivityReturnSeconds BETWEEN 0 AND 86400)
    );
    CREATE UNIQUE INDEX UX_Security_RegionFeatures_World_Region ON dbo.Security_RegionFeatures(WorldID,RegionID);
END
ELSE IF COL_LENGTH(N'dbo.Security_RegionFeatures',N'BuildMode') IS NULL
    THROW 51020, 'Apply the packaged v6.0.0 Region Control SQL update before using this page.', 1;", connection);
        await command.ExecuteNonQueryAsync();
    }

    private static int NormalizeRegionId(int regionId)
        => regionId is > short.MaxValue and <= ushort.MaxValue ? unchecked((short)(ushort)regionId) : regionId;

    private static bool IsValidRegionId(int regionId)
        => regionId != 0 && regionId is >= short.MinValue and <= short.MaxValue;

    private static async Task UpsertEventRegionFeaturesAsync(
        SqlConnection connection,
        SqlTransaction? transaction,
        string eventCode,
        int worldId,
        int regionId,
        bool teams,
        bool allowParty)
    {
        regionId = NormalizeRegionId(regionId);
        if (worldId <= 0 || !IsValidRegionId(regionId))
            throw new InvalidOperationException("Event WorldID must be positive and RegionID must be a non-zero signed 16-bit value.");

        await using var command = new SqlCommand(@"
MERGE [dbo].[Security_RegionFeatures] WITH (HOLDLOCK) AS target
USING (SELECT @WorldID AS WorldID, @RegionID AS RegionID) AS source
ON target.WorldID = source.WorldID AND target.RegionID = source.RegionID
WHEN MATCHED THEN
    UPDATE SET
        RuleName=@EventCode, Enabled=1, BuildMode=0, JobMode=1, RaceMode=0, PartyMode=0,
        MinLevel=0, MaxLevel=0, AllowTeleport=1, AllowReverse=0, AllowTrace=0,
        AllowMovement=1, AllowChat=1, AllowGlobalChat=1, AllowParty=@AllowParty,
        AllowExchange=0, AllowStall=0, AllowPvP=1, AllowAlchemy=0, AllowSpecialItems=0,
        AllowBerserk=1, AutoPvpCape=0, InactivityReturnSeconds=0, EventSuitMode=@EventSuitMode,
        ManagedEventCode = @EventCode,
        ManagedAtUtc = SYSUTCDATETIME(),
        UpdatedAt = SYSUTCDATETIME()
WHEN NOT MATCHED THEN
    INSERT
    (
        WorldID,RegionID,RuleName,Enabled,BuildMode,JobMode,RaceMode,PartyMode,MinLevel,MaxLevel,
        AllowTeleport,AllowReverse,AllowTrace,AllowMovement,AllowChat,AllowGlobalChat,AllowParty,
        AllowExchange,AllowStall,AllowPvP,AllowAlchemy,AllowSpecialItems,AllowBerserk,AutoPvpCape,
        InactivityReturnSeconds,EventSuitMode,ManagedEventCode,ManagedAtUtc
    )
    VALUES
    (
        @WorldID,@RegionID,@EventCode,1,0,1,0,0,0,0,
        1,0,0,1,1,1,@AllowParty,0,0,1,0,0,1,0,0,@EventSuitMode,@EventCode,SYSUTCDATETIME()
    );

DELETE FROM [dbo].[Security_RegionFeatures]
WHERE ManagedEventCode = @EventCode
  AND (WorldID <> @WorldID OR RegionID <> @RegionID);", connection, transaction);
        command.Parameters.Add("@EventCode", SqlDbType.NVarChar, 32).Value = eventCode.Trim().ToUpperInvariant();
        command.Parameters.Add("@WorldID", SqlDbType.Int).Value = worldId;
        command.Parameters.Add("@RegionID", SqlDbType.Int).Value = regionId;
        command.Parameters.Add("@EventSuitMode", SqlDbType.TinyInt).Value = teams ? 2 : 1;
        command.Parameters.Add("@AllowParty", SqlDbType.Bit).Value = allowParty;
        await command.ExecuteNonQueryAsync();
    }

    internal static string BuildSchedulerExecQuery(
        string databaseName,
        string procedureName,
        string arguments)
    {
        var database = NormalizeSchedulerIdentifierPart(databaseName, "Database name");
        var procedureParts = procedureName
            .Trim()
            .Split('.', StringSplitOptions.TrimEntries);
        if (procedureParts.Length is < 1 or > 2 ||
            procedureParts.Any(string.IsNullOrWhiteSpace))
        {
            throw new InvalidOperationException(
                "Stored procedure must be entered as ProcedureName or SchemaName.ProcedureName.");
        }

        var schema = procedureParts.Length == 2
            ? NormalizeSchedulerIdentifierPart(procedureParts[0], "Procedure schema")
            : "dbo";
        var procedure = NormalizeSchedulerIdentifierPart(
            procedureParts[^1],
            "Stored procedure name");
        var query = $"EXEC [{database}].[{schema}].[{procedure}]";
        var normalizedArguments = arguments.Trim();
        if (normalizedArguments.Length > 0)
            query += $" {normalizedArguments}";

        if (!IsSafeExec(query))
        {
            throw new InvalidOperationException(
                "Parameters must belong to one stored-procedure call and cannot contain semicolons or SQL comments.");
        }

        return query;
    }

    internal static (string DatabaseName, string ProcedureName, string Arguments)
        ParseSchedulerExecQuery(string execQuery, string fallbackDatabaseName)
    {
        var normalized = execQuery.Trim();
        if (normalized.StartsWith("EXEC ", StringComparison.OrdinalIgnoreCase))
            normalized = normalized[5..].TrimStart();

        var separator = normalized.IndexOfAny([' ', '\t', '\r', '\n']);
        var target = separator < 0 ? normalized : normalized[..separator];
        var arguments = separator < 0 ? string.Empty : normalized[(separator + 1)..].Trim();
        var parts = target
            .Split('.', StringSplitOptions.TrimEntries | StringSplitOptions.RemoveEmptyEntries)
            .Select(UnquoteSchedulerIdentifierPart)
            .ToArray();

        return parts.Length switch
        {
            3 => (parts[0], $"{parts[1]}.{parts[2]}", arguments),
            2 => (fallbackDatabaseName, $"{parts[0]}.{parts[1]}", arguments),
            1 => (fallbackDatabaseName, parts[0], arguments),
            _ => (fallbackDatabaseName, target, arguments)
        };
    }

    private static string NormalizeSchedulerIdentifierPart(string value, string fieldName)
    {
        var normalized = UnquoteSchedulerIdentifierPart(value.Trim());
        if (normalized.Length is < 1 or > 128)
            throw new InvalidOperationException($"{fieldName} must contain between 1 and 128 characters.");
        if (normalized.Any(char.IsWhiteSpace) ||
            normalized.Any(char.IsControl) ||
            normalized.IndexOfAny(['.', '[', ']']) >= 0)
        {
            throw new InvalidOperationException(
                $"{fieldName} cannot contain spaces, periods, brackets, or control characters.");
        }

        return normalized;
    }

    private static string UnquoteSchedulerIdentifierPart(string value)
    {
        var normalized = value.Trim();
        return normalized.Length >= 2 &&
               normalized[0] == '[' &&
               normalized[^1] == ']'
            ? normalized[1..^1]
            : normalized;
    }

    private static bool IsSafeExec(string query)
    {
        var normalized = query.Trim();
        if (!normalized.StartsWith("EXEC ", StringComparison.OrdinalIgnoreCase) ||
            normalized.Contains(';') ||
            normalized.Contains("--", StringComparison.Ordinal) ||
            normalized.Contains("/*", StringComparison.Ordinal) ||
            normalized.Contains("*/", StringComparison.Ordinal))
        {
            return false;
        }

        var commandBody = normalized[5..].TrimStart();
        var separator = commandBody.IndexOfAny([' ', '\t', '\r', '\n']);
        var procedureIdentifier = separator < 0 ? commandBody : commandBody[..separator];
        if (!SchedulerProcedureIdentifierPattern.IsMatch(procedureIdentifier))
            return false;

        var procedureName = procedureIdentifier.Split('.').Last().Trim('[', ']');
        return !procedureName.Equals("sp_executesql", StringComparison.OrdinalIgnoreCase) &&
               !procedureName.StartsWith("xp_", StringComparison.OrdinalIgnoreCase);
    }

    private sealed class ClientlessAccountImportRow
    {
        public string AccountName { get; init; } = string.Empty;
        public string AccountPassword { get; init; } = string.Empty;
        public string CharacterName { get; init; } = string.Empty;
        public string City { get; init; } = "Unassigned";
    }

    private static ClientlessAccountImportRow? ParseClientlessAccountLine(string line, int lineNumber)
    {
        var trimmed = line.Trim();
        if (trimmed.Length == 0 || trimmed.StartsWith("#", StringComparison.Ordinal))
            return null;

        var parts = Regex.Split(trimmed, @"[\t,; ]+").Where(part => part.Length > 0).ToArray();
        if (parts.Length is < 3 or > 4)
            throw new InvalidOperationException($"Invalid clientless account line {lineNumber}. Expected: user pass charname [city]");

        return new ClientlessAccountImportRow
        {
            AccountName = parts[0],
            AccountPassword = parts[1],
            CharacterName = parts[2],
            City = parts.Length == 4 ? NormalizeClientlessTownOrUnassigned(parts[3]) : "Unassigned"
        };
    }

    private static async Task EnsureClientlessSchemaAsync(SqlConnection connection)
    {
        await using var command = new SqlCommand(@"
IF OBJECT_ID('[dbo].[Clientless_Accounts]', 'U') IS NULL
BEGIN
    CREATE TABLE [dbo].[Clientless_Accounts]
    (
        ID INT IDENTITY(1,1) NOT NULL CONSTRAINT PK_ClientlessAccounts PRIMARY KEY,
        Enabled BIT NOT NULL CONSTRAINT DF_ClientlessAccounts_Enabled DEFAULT(1),
        Locale TINYINT NOT NULL CONSTRAINT DF_ClientlessAccounts_Locale DEFAULT(22),
        ShardID SMALLINT NOT NULL,
        AccountName VARCHAR(64) NOT NULL,
        AccountPassword VARCHAR(128) NOT NULL,
        CharacterName VARCHAR(64) NOT NULL,
        City VARCHAR(32) NOT NULL CONSTRAINT DF_ClientlessAccounts_City DEFAULT('Unassigned'),
        AgentAuthMode VARCHAR(32) NOT NULL CONSTRAINT DF_ClientlessAccounts_AuthMode DEFAULT('Auto'),
        AgentAuthDelayMs INT NOT NULL CONSTRAINT DF_ClientlessAccounts_AuthDelay DEFAULT(1500),
        AgentAuthPaddingHex VARCHAR(256) NOT NULL CONSTRAINT DF_ClientlessAccounts_AuthPadding DEFAULT(''),
        LaunchDelayMs INT NOT NULL CONSTRAINT DF_ClientlessAccounts_LaunchDelay DEFAULT(1000),
        ReconnectDelaySeconds INT NOT NULL CONSTRAINT DF_ClientlessAccounts_Reconnect DEFAULT(30),
        LastStatus VARCHAR(32) NOT NULL CONSTRAINT DF_ClientlessAccounts_Status DEFAULT('Pending'),
        LastMessage NVARCHAR(512) NULL,
        LastLoginAt DATETIME2 NULL,
        LastDisconnectAt DATETIME2 NULL,
        CreatedAt DATETIME2 NOT NULL CONSTRAINT DF_ClientlessAccounts_CreatedAt DEFAULT(SYSDATETIME()),
        UpdatedAt DATETIME2 NOT NULL CONSTRAINT DF_ClientlessAccounts_UpdatedAt DEFAULT(SYSDATETIME())
    );

    CREATE UNIQUE INDEX UX_ClientlessAccounts_AccountCharacter
        ON [dbo].[Clientless_Accounts](AccountName, CharacterName);
END;

IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'AgentAuthMode') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD AgentAuthMode VARCHAR(32) NOT NULL CONSTRAINT DF_ClientlessAccounts_AuthMode DEFAULT('Auto');

IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'AgentAuthDelayMs') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD AgentAuthDelayMs INT NOT NULL CONSTRAINT DF_ClientlessAccounts_AuthDelay DEFAULT(1500);

IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'AgentAuthPaddingHex') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD AgentAuthPaddingHex VARCHAR(256) NOT NULL CONSTRAINT DF_ClientlessAccounts_AuthPadding DEFAULT('');

IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'LaunchDelayMs') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD LaunchDelayMs INT NOT NULL CONSTRAINT DF_ClientlessAccounts_LaunchDelay DEFAULT(1000);

IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'ReconnectDelaySeconds') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD ReconnectDelaySeconds INT NOT NULL CONSTRAINT DF_ClientlessAccounts_Reconnect DEFAULT(30);

IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'City') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD City VARCHAR(32) NOT NULL CONSTRAINT DF_ClientlessAccounts_City DEFAULT('Unassigned');

IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'SystemRole') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD SystemRole VARCHAR(32) NULL;

IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'ProfileRace') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD ProfileRace VARCHAR(16) NULL;
IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'ProfileWeaponTypeID4') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD ProfileWeaponTypeID4 TINYINT NULL;
IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'ProfileArmorTypeID3') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD ProfileArmorTypeID3 TINYINT NULL;
IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'ProfileUseShield') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD ProfileUseShield BIT NULL;
IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'ProfileEquipmentMode') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD ProfileEquipmentMode VARCHAR(16) NULL;
IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'ProfileSkillMode') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD ProfileSkillMode VARCHAR(32) NULL;
IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'ProfileBuild') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD ProfileBuild VARCHAR(16) NULL;
IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'ProfileAvatarsEnabled') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD ProfileAvatarsEnabled BIT NOT NULL
        CONSTRAINT DF_ClientlessAccounts_ProfileAvatarsEnabled DEFAULT(1) WITH VALUES;
IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'ProfilePetsEnabled') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD ProfilePetsEnabled BIT NOT NULL
        CONSTRAINT DF_ClientlessAccounts_ProfilePetsEnabled DEFAULT(1) WITH VALUES;
IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'ProfileAttackPetEnabled') IS NULL
BEGIN
    EXEC(N'ALTER TABLE [dbo].[Clientless_Accounts] ADD ProfileAttackPetEnabled BIT NULL;');
    EXEC(N'UPDATE [dbo].[Clientless_Accounts]
        SET ProfileAttackPetEnabled = ISNULL(ProfilePetsEnabled, 1);');
    EXEC(N'ALTER TABLE [dbo].[Clientless_Accounts] ALTER COLUMN ProfileAttackPetEnabled BIT NOT NULL;');
    EXEC(N'ALTER TABLE [dbo].[Clientless_Accounts] ADD
        CONSTRAINT DF_ClientlessAccounts_ProfileAttackPetEnabled DEFAULT(1) FOR ProfileAttackPetEnabled;');
END;
IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'ProfileGrabPetEnabled') IS NULL
BEGIN
    EXEC(N'ALTER TABLE [dbo].[Clientless_Accounts] ADD ProfileGrabPetEnabled BIT NULL;');
    EXEC(N'UPDATE [dbo].[Clientless_Accounts]
        SET ProfileGrabPetEnabled = ISNULL(ProfilePetsEnabled, 1);');
    EXEC(N'ALTER TABLE [dbo].[Clientless_Accounts] ALTER COLUMN ProfileGrabPetEnabled BIT NOT NULL;');
    EXEC(N'ALTER TABLE [dbo].[Clientless_Accounts] ADD
        CONSTRAINT DF_ClientlessAccounts_ProfileGrabPetEnabled DEFAULT(1) FOR ProfileGrabPetEnabled;');
END;
IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'HomeRegionID') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD HomeRegionID INT NULL;
IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'HomeX') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD HomeX REAL NULL;
IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'HomeY') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD HomeY REAL NULL;
IF COL_LENGTH('[dbo].[Clientless_Accounts]', 'HomeZ') IS NULL
    ALTER TABLE [dbo].[Clientless_Accounts] ADD HomeZ REAL NULL;

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID(N'dbo.Clientless_Accounts') AND name = N'IX_ClientlessAccounts_CityEnabled')
    CREATE INDEX IX_ClientlessAccounts_CityEnabled ON [dbo].[Clientless_Accounts](City, Enabled, ID);

IF OBJECT_ID(N'dbo.Clientless_HuntAreas', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Clientless_HuntAreas
    (
        ID INT IDENTITY(1,1) NOT NULL CONSTRAINT PK_Clientless_HuntAreas PRIMARY KEY,
        City VARCHAR(32) NOT NULL,
        SlotNumber TINYINT NOT NULL,
        DisplayName NVARCHAR(64) NOT NULL,
        Enabled BIT NOT NULL CONSTRAINT DF_Clientless_HuntAreas_Enabled DEFAULT(0),
        RegionID INT NOT NULL CONSTRAINT DF_Clientless_HuntAreas_RegionID DEFAULT(0),
        PosX REAL NOT NULL CONSTRAINT DF_Clientless_HuntAreas_PosX DEFAULT(0),
        PosY REAL NOT NULL CONSTRAINT DF_Clientless_HuntAreas_PosY DEFAULT(0),
        PosZ REAL NOT NULL CONSTRAINT DF_Clientless_HuntAreas_PosZ DEFAULT(0),
        Radius REAL NOT NULL CONSTRAINT DF_Clientless_HuntAreas_Radius DEFAULT(50),
        CreatedAt DATETIME2 NOT NULL CONSTRAINT DF_Clientless_HuntAreas_CreatedAt DEFAULT(SYSDATETIME()),
        UpdatedAt DATETIME2 NOT NULL CONSTRAINT DF_Clientless_HuntAreas_UpdatedAt DEFAULT(SYSDATETIME()),
        CONSTRAINT UQ_Clientless_HuntAreas_CitySlot UNIQUE(City, SlotNumber),
        CONSTRAINT CK_Clientless_HuntAreas_SlotNumber CHECK(SlotNumber BETWEEN 1 AND 5),
        CONSTRAINT CK_Clientless_HuntAreas_Radius CHECK(Radius BETWEEN 5 AND 500)
    );
END;

IF OBJECT_ID(N'dbo.Clientless_HuntPolicy', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Clientless_HuntPolicy
    (
        SettingID TINYINT NOT NULL CONSTRAINT PK_Clientless_HuntPolicy PRIMARY KEY,
        Enabled BIT NOT NULL DEFAULT(0), AttackNormal BIT NOT NULL DEFAULT(1),
        AttackUnique BIT NOT NULL DEFAULT(1), UniquePriority BIT NOT NULL DEFAULT(1),
        UseSkills BIT NOT NULL DEFAULT(1), UseBasicAttack BIT NOT NULL DEFAULT(1),
        HpPotionPercent TINYINT NOT NULL DEFAULT(60), MpPotionPercent TINYINT NOT NULL DEFAULT(40),
        StuckSeconds SMALLINT NOT NULL DEFAULT(15), TargetTimeoutSeconds SMALLINT NOT NULL DEFAULT(30),
        UpdatedAt DATETIME2 NOT NULL DEFAULT(SYSDATETIME())
    );
END;

IF NOT EXISTS (SELECT 1 FROM dbo.Clientless_HuntPolicy WHERE SettingID = 1)
    INSERT dbo.Clientless_HuntPolicy
        (SettingID, Enabled, AttackNormal, AttackUnique, UniquePriority, UseSkills, UseBasicAttack,
         HpPotionPercent, MpPotionPercent, StuckSeconds, TargetTimeoutSeconds)
    VALUES (1, 0, 1, 1, 1, 1, 1, 60, 40, 15, 30);

IF COL_LENGTH(N'dbo.Clientless_Accounts', N'HuntEnabled') IS NULL
    ALTER TABLE dbo.Clientless_Accounts ADD HuntEnabled BIT NOT NULL DEFAULT(1);
IF COL_LENGTH(N'dbo.Clientless_Accounts', N'HuntStatus') IS NULL
    ALTER TABLE dbo.Clientless_Accounts ADD HuntStatus VARCHAR(32) NOT NULL DEFAULT('Stopped');
IF COL_LENGTH(N'dbo.Clientless_Accounts', N'HuntMessage') IS NULL
    ALTER TABLE dbo.Clientless_Accounts ADD HuntMessage NVARCHAR(256) NULL;
IF COL_LENGTH(N'dbo.Clientless_Accounts', N'HuntAreaID') IS NULL
    ALTER TABLE dbo.Clientless_Accounts ADD HuntAreaID INT NULL;
IF COL_LENGTH(N'dbo.Clientless_Accounts', N'CurrentTargetName') IS NULL
    ALTER TABLE dbo.Clientless_Accounts ADD CurrentTargetName NVARCHAR(128) NULL;
IF COL_LENGTH(N'dbo.Clientless_Accounts', N'LastHuntAt') IS NULL
    ALTER TABLE dbo.Clientless_Accounts ADD LastHuntAt DATETIME2 NULL;", connection);
        await command.ExecuteNonQueryAsync();
    }

    private static void ValidateClientlessRuntimeFields(
        int shardId,
        int launchDelayMs,
        int reconnectDelaySeconds)
    {
        if (shardId is < 1 or > short.MaxValue)
            throw new InvalidOperationException($"Shard must be between 1 and {short.MaxValue:N0}.");
        if (launchDelayMs is < 0 or > 60000)
            throw new InvalidOperationException("Launch delay must be between 0 and 60,000 milliseconds.");
        if (reconnectDelaySeconds is < 5 or > 600)
            throw new InvalidOperationException("Reconnect delay must be between 5 and 600 seconds.");
    }

    private static async Task<bool> ClientlessAccountExistsAsync(SqlConnection connection, SqlTransaction transaction, string accountName, string characterName)
    {
        await using var command = new SqlCommand(
            "SELECT COUNT(1) FROM [dbo].[Clientless_Accounts] WHERE AccountName = @AccountName AND CharacterName = @CharacterName;",
            connection,
            transaction);
        command.Parameters.Add("@AccountName", SqlDbType.VarChar, 64).Value = accountName;
        command.Parameters.Add("@CharacterName", SqlDbType.VarChar, 64).Value = characterName;
        return Convert.ToInt32(await command.ExecuteScalarAsync()) > 0;
    }

    private static string NormalizeClientlessTown(string value)
    {
        var town = (value ?? string.Empty).Trim();
        return town.ToLowerInvariant() switch
        {
            "jangan" => "Jangan",
            "donwhang" => "Donwhang",
            "hotan" => "Hotan",
            "samarkand" => "SamarKand",
            "constantinople" => "Constantinople",
            "alexandria north" or "alexandria north (sd)" or "alexandrianorth" or "sd" or "sd2" => "Alexandria North (SD)",
            _ => "Jangan"
        };
    }

    internal static string NormalizeClientlessTownOrUnassigned(string? value)
    {
        var town = (value ?? string.Empty).Trim();
        return town.ToLowerInvariant() switch
        {
            "jangan" => "Jangan",
            "donwhang" => "Donwhang",
            "hotan" => "Hotan",
            "samarkand" => "SamarKand",
            "constantinople" => "Constantinople",
            "alexandria north" or "alexandria north (sd)" or "alexandrianorth" or "sd" or "sd2" => "Alexandria North (SD)",
            _ => "Unassigned"
        };
    }

    private static readonly string[] ClientlessGamingNamePrefixes =
    [
        "Aero", "Arcane", "Ash", "Astral", "Blaze", "Crimson", "Cyber", "Dark", "Dawn", "Drift",
        "Echo", "Ember", "Epic", "Fallen", "Frost", "Ghost", "Golden", "Hyper", "Iron", "Jade",
        "Lunar", "Mystic", "Neon", "Night", "Nova", "Onyx", "Phantom", "Prime", "Rapid", "Raven",
        "Rogue", "Royal", "Rune", "Savage", "Shadow", "Silent", "Silver", "Solar", "Storm", "Swift",
        "Titan", "Toxic", "Turbo", "Venom", "Void", "Wild", "Winter", "Wolf", "Zen", "Zero"
    ];

    private static readonly string[] ClientlessGamingNameSuffixes =
    [
        "Ace", "Arrow", "Blade", "Bolt", "Claw", "Crow", "Demon", "Dragon", "Edge", "Fang",
        "Falcon", "Fire", "Fox", "Fury", "Hawk", "Hunter", "Knight", "Legend", "Lion", "Lord",
        "Mage", "Ninja", "Nomad", "Phoenix", "Pulse", "Reaper", "Rider", "Ronin", "Saint", "Scout",
        "Slayer", "Soul", "Spark", "Spirit", "Strike", "Tiger", "Vex", "Viper", "Walker", "Warden",
        "Warrior", "Wave", "Wing", "Wraith", "Xeno", "Zenith"
    ];

    private static async Task<string> GenerateUniqueClientlessAccountNameAsync(
        SqlConnection connection,
        string accountDb,
        string characterName)
    {
        var accountDbName = QuoteDb(accountDb);
        for (var attempt = 0; attempt < 120; attempt++)
        {
            var numericSuffix = Random.Shared.Next(10, 10000).ToString(
                Random.Shared.Next(0, 2) == 0 ? "00" : "0000");
            var accountName = $"{characterName.ToLowerInvariant()}{numericSuffix}";
            if (accountName.Length > 24)
                accountName = accountName[..24];

            await using var command = new SqlCommand(
                $"SELECT COUNT(1) FROM {accountDbName}.dbo.TB_User WITH (NOLOCK) WHERE StrUserID = @AccountName;",
                connection);
            command.Parameters.Add("@AccountName", SqlDbType.VarChar, 24).Value = accountName;
            if (Convert.ToInt32(await command.ExecuteScalarAsync()) == 0)
                return accountName;
        }

        throw new InvalidOperationException("Could not generate a unique clientless account name.");
    }

    private static async Task<string> GenerateUniqueClientlessCharacterNameAsync(SqlConnection connection, string shardDb)
    {
        var shardDbName = QuoteDb(shardDb);

        for (var attempt = 0; attempt < 240; attempt++)
        {
            var characterName = CreateClientlessGamingNameCandidate(attempt >= 80);

            await using var command = new SqlCommand(
                $"SELECT COUNT(1) FROM {shardDbName}.dbo._Char WITH (NOLOCK) WHERE CharName16 = @CharacterName;",
                connection);
            command.Parameters.Add("@CharacterName", SqlDbType.VarChar, 64).Value = characterName;
            if (Convert.ToInt32(await command.ExecuteScalarAsync()) == 0)
                return characterName;
        }

        throw new InvalidOperationException("Could not generate a unique clientless character name.");
    }

    internal static string CreateClientlessGamingNameCandidate(bool forceNumericSuffix = false)
    {
        var prefix = ClientlessGamingNamePrefixes[Random.Shared.Next(ClientlessGamingNamePrefixes.Length)];
        var suffix = ClientlessGamingNameSuffixes[Random.Shared.Next(ClientlessGamingNameSuffixes.Length)];
        var numericSuffix = forceNumericSuffix || Random.Shared.Next(0, 5) == 0
            ? Random.Shared.Next(10, 100).ToString()
            : string.Empty;
        var candidate = $"{prefix}{suffix}{numericSuffix}";
        return candidate.Length > 16 ? candidate[..16] : candidate;
    }

    private static async Task<int> CreateEquippedClientlessCharacterAsync(
        SqlConnection connection,
        SqlTransaction transaction,
        string accountDb,
        string shardDb,
        string accountName,
        string password,
        string characterName,
        string race,
        string gender,
        int level,
        ClientlessSpawnPosition spawnPosition,
        ClientlessWeaponProfile equipmentProfile,
        string equipmentSelection,
        string skillSelection,
        int masteryCap,
        string buildSelection,
        bool includeAvatars,
        bool includeAttackPet,
        bool includeGrabPet)
    {
        var accountDbName = QuoteDb(accountDb);
        var shardDbName = QuoteDb(shardDb);
        var refCharId = ResolveClientlessRefCharId(race, gender);
        var (strength, intellect) = CalculateClientlessCombatStats(level, buildSelection);
        var attackSkillPredicate = BuildClientlessMonsterAttackSkillPredicate("AttackSkill");
        var weaponSkillPredicate = BuildClientlessWeaponSkillCompatibilityPredicate("AttackSkill", "@WeaponTypeID4");
        var unlockedAttackSkillExistsSql = BuildClientlessUnlockedAttackSkillExistsSql(shardDbName);
        var avatarEquipmentSql = BuildOptionalClientlessAvatarEquipmentSql(
            shardDbName,
            "@NewCharID",
            "@RequiredGender",
            "@NewCharID",
            includeAvatars);
        var petEquipmentSql = BuildOptionalClientlessPetEquipmentSql(
            shardDbName,
            "@NewCharID",
            "@Level",
            "@NewCharID",
            includeAttackPet,
            includeGrabPet);

        var sql = $@"
IF DB_ID(@AccountDbName) IS NULL
    THROW 51000, 'Account database was not found.', 1;
IF DB_ID(@ShardDbName) IS NULL
    THROW 51001, 'Shard database was not found.', 1;

DECLARE @UserJID INT = 0;
DECLARE @NewCharID INT = 0;

    IF NOT EXISTS (SELECT 1 FROM {accountDbName}.dbo.TB_User WITH (UPDLOCK, HOLDLOCK) WHERE StrUserID = @AccountName)
    BEGIN
        INSERT INTO {accountDbName}.dbo.TB_User (StrUserID, [password], sec_content, sec_primary)
        VALUES (@AccountName, LOWER(CONVERT(VARCHAR(32), HASHBYTES('MD5', @Password), 2)), 3, 3);
    END

    SELECT @UserJID = JID
    FROM {accountDbName}.dbo.TB_User WITH (NOLOCK)
    WHERE StrUserID = @AccountName;

    IF ISNULL(@UserJID, 0) <= 0
        THROW 51002, 'Account JID could not be resolved.', 1;

    IF NOT EXISTS (SELECT 1 FROM {shardDbName}.dbo._AccountJID WITH (UPDLOCK, HOLDLOCK) WHERE AccountID = @AccountName)
    BEGIN
        INSERT INTO {shardDbName}.dbo._AccountJID (AccountID, JID, Gold)
        VALUES (@AccountName, @UserJID, 0);
    END

    IF (SELECT COUNT(1) FROM {shardDbName}.dbo._User WITH (NOLOCK) WHERE UserJID = @UserJID) >= 4
        THROW 51003, 'Account already has 4 characters.', 1;

    IF EXISTS (SELECT 1 FROM {shardDbName}.dbo._Char WITH (UPDLOCK, HOLDLOCK) WHERE CharName16 = @CharacterName)
        THROW 51004, 'Character name already exists.', 1;

    INSERT INTO {shardDbName}.dbo._Char
    (
        RefObjID, CharName16, Scale, Strength, Intellect,
        LatestRegion, PosX, PosY, PosZ, AppointedTeleport,
        InventorySize, LastLogout, CurLevel, MaxLevel,
        RemainGold, RemainSkillPoint, RemainStatPoint, HP, MP,
        JobLvl_Trader, JobLvl_Hunter, JobLvl_Robber, WorldID
    )
    VALUES
    (
        @RefCharID, @CharacterName, @Scale, @Strength, @Intellect,
        @RegionID, @PosX, @PosY, @PosZ, 0,
        @InventorySize, GETDATE(), @Level, @Level,
        0, 0, 0, @InitialHP, @InitialMP,
        1, 1, 1, 1
    );

    SET @NewCharID = CONVERT(INT, SCOPE_IDENTITY());
    IF ISNULL(@NewCharID, 0) <= 0
        THROW 51005, 'Character row was not created.', 1;

    INSERT INTO {shardDbName}.dbo._User (UserJID, CharID)
    VALUES (@UserJID, @NewCharID);

    INSERT INTO {shardDbName}.dbo._Inventory (CharID, Slot, ItemID)
    SELECT @NewCharID, cnt, 0
    FROM {shardDbName}.dbo._RefDummySlot WITH (NOLOCK)
    WHERE cnt < @InventorySize;

    INSERT INTO {shardDbName}.dbo._InventoryForAvatar (CharID, Slot, ItemID)
    SELECT @NewCharID, cnt, 0
    FROM {shardDbName}.dbo._RefDummySlot WITH (NOLOCK)
    WHERE cnt < 5;

    {avatarEquipmentSql}

    DECLARE @Country TINYINT = @CharacterCountry;

    DECLARE @Equipment TABLE
    (
        Slot TINYINT NOT NULL PRIMARY KEY,
        RefItemID INT NOT NULL,
        Durability INT NOT NULL
    );

    INSERT INTO @Equipment (Slot, RefItemID, Durability)
    SELECT slots.Slot, picked.RefItemID, picked.Durability
    FROM
    (
        VALUES
            (CONVERT(TINYINT, 0), CONVERT(TINYINT, 1)),
            (CONVERT(TINYINT, 1), CONVERT(TINYINT, 3)),
            (CONVERT(TINYINT, 2), CONVERT(TINYINT, 2)),
            (CONVERT(TINYINT, 3), CONVERT(TINYINT, 5)),
            (CONVERT(TINYINT, 4), CONVERT(TINYINT, 4)),
            (CONVERT(TINYINT, 5), CONVERT(TINYINT, 6))
    ) slots(Slot, ItemTypeID4)
    CROSS APPLY
    (
        SELECT TOP (1)
            C.ID AS RefItemID,
            CONVERT(INT, CEILING(ISNULL(I.Dur_L, 0))) AS Durability
        FROM {shardDbName}.dbo._RefObjCommon C WITH (NOLOCK)
        INNER JOIN {shardDbName}.dbo._RefObjItem I WITH (NOLOCK) ON I.ID = C.Link
        WHERE C.Service = 1
          AND C.CashItem = 0
          AND @CreateEquipment = 1
          AND ((@EquipmentRarity = 2 AND C.Rarity = 2 AND C.CodeName128 LIKE '%[_]C[_]RARE')
               OR (@EquipmentRarity = 0 AND C.Rarity = 0 AND C.CodeName128 NOT LIKE '%[_]RARE%'))
          AND C.Country = @Country
          AND C.TypeID1 = 3
          AND C.TypeID2 = 1
          AND C.TypeID3 = @ArmorTypeID3
          AND C.TypeID4 = slots.ItemTypeID4
          AND C.ReqLevel1 <= @Level
          AND I.ReqGender = @RequiredGender
          AND C.CodeName128 NOT LIKE '%SET%'
          AND C.CodeName128 NOT LIKE '%MALL%'
          AND C.CodeName128 NOT LIKE '%EVENT%'
          AND C.CodeName128 NOT LIKE '%TEST%'
        ORDER BY C.ReqLevel1 DESC, C.ID
    ) picked;

    INSERT INTO @Equipment (Slot, RefItemID, Durability)
    SELECT TOP (1)
        6,
        C.ID,
        CONVERT(INT, CEILING(ISNULL(I.Dur_L, 0)))
    FROM {shardDbName}.dbo._RefObjCommon C WITH (NOLOCK)
    INNER JOIN {shardDbName}.dbo._RefObjItem I WITH (NOLOCK) ON I.ID = C.Link
    WHERE C.Service = 1
      AND C.CashItem = 0
      AND @CreateEquipment = 1
      AND ((@EquipmentRarity = 2 AND C.Rarity = 2 AND C.CodeName128 LIKE '%[_]C[_]RARE')
           OR (@EquipmentRarity = 0 AND C.Rarity = 0 AND C.CodeName128 NOT LIKE '%[_]RARE%'))
      AND C.Country = @Country
      AND C.TypeID1 = 3
      AND C.TypeID2 = 1
      AND C.TypeID3 = 6
      AND C.TypeID4 = @WeaponTypeID4
      AND C.ReqLevel1 <= @Level
      AND C.CodeName128 NOT LIKE '%SET%'
      AND C.CodeName128 NOT LIKE '%MALL%'
      AND C.CodeName128 NOT LIKE '%EVENT%'
      AND C.CodeName128 NOT LIKE '%TEST%'
    ORDER BY C.ReqLevel1 DESC, C.ID;

    IF @UseShield = 1
    BEGIN
        INSERT INTO @Equipment (Slot, RefItemID, Durability)
        SELECT TOP (1)
            7,
            C.ID,
            CONVERT(INT, CEILING(ISNULL(I.Dur_L, 0)))
        FROM {shardDbName}.dbo._RefObjCommon C WITH (NOLOCK)
        INNER JOIN {shardDbName}.dbo._RefObjItem I WITH (NOLOCK) ON I.ID = C.Link
        WHERE C.Service = 1
          AND C.CashItem = 0
          AND @CreateEquipment = 1
          AND ((@EquipmentRarity = 2 AND C.Rarity = 2 AND C.CodeName128 LIKE '%[_]C[_]RARE')
               OR (@EquipmentRarity = 0 AND C.Rarity = 0 AND C.CodeName128 NOT LIKE '%[_]RARE%'))
          AND C.Country = @Country
          AND C.TypeID1 = 3
          AND C.TypeID2 = 1
          AND C.TypeID3 = 4
          AND C.TypeID4 = CASE WHEN @Country = 0 THEN 1 ELSE 2 END
          AND C.ReqLevel1 <= @Level
          AND C.CodeName128 NOT LIKE '%SET%'
          AND C.CodeName128 NOT LIKE '%MALL%'
          AND C.CodeName128 NOT LIKE '%EVENT%'
          AND C.CodeName128 NOT LIKE '%TEST%'
        ORDER BY C.ReqLevel1 DESC, C.ID;
    END

    IF @CreateEquipment = 1 AND (SELECT COUNT(1) FROM @Equipment WHERE Slot BETWEEN 0 AND 6) <> 7
        THROW 51006, 'A complete level-matched clientless equipment set could not be resolved.', 1;

    IF @CreateEquipment = 1 AND @UseShield = 1 AND NOT EXISTS (SELECT 1 FROM @Equipment WHERE Slot = 7)
        THROW 51007, 'A level-matched clientless shield could not be resolved.', 1;

    DECLARE @EquipSlot TINYINT = 0;
    DECLARE @EquipRefItemID INT;
    DECLARE @EquipDurability INT;
    DECLARE @EquipResult INT;

    WHILE @EquipSlot <= 7
    BEGIN
        SET @EquipRefItemID = NULL;
        SET @EquipDurability = NULL;

        SELECT
            @EquipRefItemID = RefItemID,
            @EquipDurability = Durability
        FROM @Equipment
        WHERE Slot = @EquipSlot;

        IF ISNULL(@EquipRefItemID, 0) > 0
        BEGIN
            SET @EquipResult = NULL;
            EXEC @EquipResult = {shardDbName}.dbo._FN_ADD_INITIAL_EQUIP
                @CharID = @NewCharID,
                @Slot = @EquipSlot,
                @RefItemID = @EquipRefItemID,
                @Data = @EquipDurability;
            IF ISNULL(@EquipResult, -1) < 0
                THROW 51011, 'vSRO rejected a resolved Clientless equipment item.', 1;
        END

        SET @EquipSlot += 1;
    END

    IF @CreateEquipment = 1 AND
       (SELECT COUNT(1)
        FROM {shardDbName}.dbo._Inventory AS InventoryRow
        INNER JOIN {shardDbName}.dbo._Items AS ItemRow ON ItemRow.ID64 = InventoryRow.ItemID AND ItemRow.Data > 0
        INNER JOIN @Equipment AS Expected ON Expected.Slot = InventoryRow.Slot AND Expected.RefItemID = ItemRow.RefItemID
        INNER JOIN {shardDbName}.dbo._RefObjCommon AS Common ON Common.ID = ItemRow.RefItemID AND Common.Service = 1
        INNER JOIN {shardDbName}.dbo._RefObjItem AS RefItem ON RefItem.ID = Common.Link
        WHERE InventoryRow.CharID = @NewCharID AND InventoryRow.Slot BETWEEN 0 AND 6) <> 7
        THROW 51008, 'The new clientless character equipment could not be created.', 1;

    IF @CreateEquipment = 1 AND @UseShield = 1
       AND NOT EXISTS (SELECT 1 FROM {shardDbName}.dbo._Inventory WHERE CharID = @NewCharID AND Slot = 7 AND ItemID > 0)
        THROW 51009, 'The new clientless character shield could not be created.', 1;

    -- Potions remain in fixed bag slots. Bow arrows and crossbow bolts must be
    -- equipped in the vSRO secondary equipment slot (7), exactly like a shield.
    DECLARE @PotionType TINYINT = 1;
    DECLARE @PotionSlot TINYINT;
    DECLARE @PotionRefItemID INT;
    DECLARE @PotionCount INT;
    DECLARE @PotionResult INT;
    WHILE @PotionType <= 2
    BEGIN
        SET @PotionSlot = CASE WHEN @PotionType = 1 THEN 13 ELSE 14 END;
        SET @PotionRefItemID = NULL;
        SET @PotionCount = NULL;

        SELECT TOP (1)
            @PotionRefItemID = C.ID,
            @PotionCount = CASE WHEN ISNULL(I.MaxStack, 0) > 0 THEN I.MaxStack ELSE 1000 END
        FROM {shardDbName}.dbo._RefObjCommon C WITH (NOLOCK)
        INNER JOIN {shardDbName}.dbo._RefObjItem I WITH (NOLOCK) ON I.ID = C.Link
        WHERE C.Service = 1 AND C.CashItem = 0
          AND C.TypeID1 = 3 AND C.TypeID2 = 3 AND C.TypeID3 = 1 AND C.TypeID4 = @PotionType
          AND C.ReqLevel1 <= @Level
          AND C.CodeName128 NOT LIKE '%EVENT%' AND C.CodeName128 NOT LIKE '%MALL%'
        ORDER BY C.ReqLevel1 DESC, C.ID DESC;

        IF ISNULL(@PotionRefItemID, 0) > 0
            EXEC @PotionResult = {shardDbName}.dbo._FN_ADD_INITIAL_EQUIP
                @CharID = @NewCharID, @Slot = @PotionSlot,
                @RefItemID = @PotionRefItemID, @Data = @PotionCount;

        SET @PotionType += 1;
    END

    -- Give every generated character a normal vSRO speed drug. TypeID
    -- 3/3/13/1 is the same family used by the native client/bot item logic.
    DECLARE @SpeedRefItemID INT = NULL;
    DECLARE @SpeedCount INT = NULL;
    DECLARE @SpeedResult INT;
    SELECT TOP (1)
        @SpeedRefItemID = C.ID,
        @SpeedCount = CASE WHEN ISNULL(I.MaxStack, 0) > 0 THEN I.MaxStack ELSE 1000 END
    FROM {shardDbName}.dbo._RefObjCommon C WITH (NOLOCK)
    INNER JOIN {shardDbName}.dbo._RefObjItem I WITH (NOLOCK) ON I.ID = C.Link
    WHERE C.Service = 1 AND C.CashItem = 0
      AND C.TypeID1 = 3 AND C.TypeID2 = 3 AND C.TypeID3 = 13 AND C.TypeID4 = 1
      AND C.ReqLevel1 <= @Level
      AND C.CodeName128 LIKE '%SPEED%'
      AND C.CodeName128 NOT LIKE '%EVENT%' AND C.CodeName128 NOT LIKE '%MALL%'
    ORDER BY C.ReqLevel1 DESC, C.ID DESC;

    IF ISNULL(@SpeedRefItemID, 0) > 0
        EXEC @SpeedResult = {shardDbName}.dbo._FN_ADD_INITIAL_EQUIP
            @CharID = @NewCharID, @Slot = 15,
            @RefItemID = @SpeedRefItemID, @Data = @SpeedCount;

    {petEquipmentSql}

    DECLARE @AmmunitionType TINYINT =
        CASE WHEN @WeaponTypeID4 = 6 THEN 1 WHEN @WeaponTypeID4 = 12 THEN 2 ELSE 0 END;
    DECLARE @AmmunitionRefItemID INT = NULL;
    DECLARE @AmmunitionCount INT = NULL;
    DECLARE @AmmunitionResult INT;
    IF @CreateEquipment = 1 AND @AmmunitionType > 0
    BEGIN
        SELECT TOP (1)
            @AmmunitionRefItemID = C.ID,
            @AmmunitionCount = CASE WHEN ISNULL(I.MaxStack, 0) > 0 THEN I.MaxStack ELSE 1000 END
        FROM {shardDbName}.dbo._RefObjCommon C WITH (NOLOCK)
        INNER JOIN {shardDbName}.dbo._RefObjItem I WITH (NOLOCK) ON I.ID = C.Link
        WHERE C.Service = 1 AND C.CashItem = 0
          AND C.TypeID1 = 3 AND C.TypeID2 = 3 AND C.TypeID3 = 4 AND C.TypeID4 = @AmmunitionType
          AND C.ReqLevel1 <= @Level
          AND C.CodeName128 NOT LIKE '%EVENT%' AND C.CodeName128 NOT LIKE '%MALL%'
        ORDER BY C.ReqLevel1 DESC, C.ID DESC;

        IF ISNULL(@AmmunitionRefItemID, 0) > 0
            EXEC @AmmunitionResult = {shardDbName}.dbo._FN_ADD_INITIAL_EQUIP
                @CharID = @NewCharID, @Slot = 7,
                @RefItemID = @AmmunitionRefItemID, @Data = @AmmunitionCount;

        IF ISNULL(@AmmunitionRefItemID, 0) <= 0
           OR NOT EXISTS
              (SELECT 1 FROM {shardDbName}.dbo._Inventory WHERE CharID = @NewCharID AND Slot = 7 AND ItemID > 0)
            THROW 51010, 'Level-matched ammunition could not be equipped for the ranged Clientless character.', 1;
    END

    IF OBJECT_ID(N'{shardDb}.dbo._CharSkillMastery', N'U') IS NOT NULL
    BEGIN
        INSERT INTO {shardDbName}.dbo._CharSkillMastery (CharID, MasteryID, Level)
        SELECT @NewCharID, MasteryID, 0
        FROM {shardDbName}.dbo._RefCharDefault_SkillMastery DSM WITH (NOLOCK)
        WHERE (DSM.Race = @Country OR DSM.Race = 3)
          AND NOT EXISTS
          (
              SELECT 1 FROM {shardDbName}.dbo._CharSkillMastery M WITH (NOLOCK)
              WHERE M.CharID = @NewCharID AND M.MasteryID = DSM.MasteryID
          );
    END

    IF OBJECT_ID(N'{shardDb}.dbo._CharSkill', N'U') IS NOT NULL
    BEGIN
        INSERT INTO {shardDbName}.dbo._CharSkill (CharID, SkillID, Enable)
        SELECT @NewCharID, SkillID, 1
        FROM {shardDbName}.dbo._RefCharDefault_Skill DS WITH (NOLOCK)
        WHERE (DS.Race = @Country OR DS.Race = 3)
          AND NOT EXISTS
          (
              SELECT 1 FROM {shardDbName}.dbo._CharSkill S WITH (NOLOCK)
              WHERE S.CharID = @NewCharID AND S.SkillID = DS.SkillID
          );
    END

    DECLARE @ClientlessDesiredMasteries TABLE
    (
        MasteryID INT NOT NULL PRIMARY KEY,
        MasteryLevel INT NOT NULL
    );

    IF @OpenMastery = 1 AND OBJECT_ID(N'{shardDb}.dbo._CharSkillMastery', N'U') IS NOT NULL
    BEGIN
        DECLARE @PrimaryMasteryLevel INT =
            CASE WHEN @Level < @MasteryCap THEN @Level ELSE @MasteryCap END;
        DECLARE @RemainingMastery INT = @MasteryCap - @PrimaryMasteryLevel;
        DECLARE @FireMasteryLevel INT = 0;
        DECLARE @LightningMasteryLevel INT = 0;

        INSERT INTO @ClientlessDesiredMasteries (MasteryID, MasteryLevel)
        VALUES (@MasteryID, @PrimaryMasteryLevel);

        -- Chinese maximum-skill profiles also receive real weapon buffs. The
        -- total of weapon + Fire + Lightning never exceeds the configured cap.
        IF @Country = 0 AND @MaxMasterySkills = 1 AND @RemainingMastery > 0
        BEGIN
            SET @FireMasteryLevel = CASE WHEN @Level < @RemainingMastery THEN @Level ELSE @RemainingMastery END;
            SET @RemainingMastery -= @FireMasteryLevel;
            IF @FireMasteryLevel > 0
                INSERT INTO @ClientlessDesiredMasteries (MasteryID, MasteryLevel)
                VALUES (275, @FireMasteryLevel);

            SET @LightningMasteryLevel = CASE WHEN @Level < @RemainingMastery THEN @Level ELSE @RemainingMastery END;
            IF @LightningMasteryLevel > 0
                INSERT INTO @ClientlessDesiredMasteries (MasteryID, MasteryLevel)
                VALUES (274, @LightningMasteryLevel);
        END

        UPDATE Existing
        SET Level = Desired.MasteryLevel
        FROM {shardDbName}.dbo._CharSkillMastery AS Existing
        INNER JOIN @ClientlessDesiredMasteries AS Desired
            ON Desired.MasteryID = Existing.MasteryID
        WHERE Existing.CharID = @NewCharID
          AND Existing.Level <> Desired.MasteryLevel;

        INSERT INTO {shardDbName}.dbo._CharSkillMastery (CharID, MasteryID, Level)
        SELECT @NewCharID, Desired.MasteryID, Desired.MasteryLevel
        FROM @ClientlessDesiredMasteries AS Desired
        WHERE NOT EXISTS
        (
            SELECT 1
            FROM {shardDbName}.dbo._CharSkillMastery AS Existing WITH (UPDLOCK, HOLDLOCK)
            WHERE Existing.CharID = @NewCharID
              AND Existing.MasteryID = Desired.MasteryID
        );
    END

    IF @MaxMasterySkills = 1 AND OBJECT_ID(N'{shardDb}.dbo._CharSkill', N'U') IS NOT NULL
    BEGIN
        ;WITH RankedSkills AS
        (
            SELECT
                Skill.ID AS SkillID,
                Skill.GroupID,
                ROW_NUMBER() OVER
                (
                    PARTITION BY Skill.GroupID
                    ORDER BY Skill.Basic_Level DESC, Skill.ReqCommon_MasteryLevel1 DESC, Skill.ID DESC
                ) AS RankOrder
            FROM {shardDbName}.dbo._RefSkill AS Skill WITH (NOLOCK)
            INNER JOIN @ClientlessDesiredMasteries AS DesiredMastery
                ON DesiredMastery.MasteryID = Skill.ReqCommon_Mastery1
            WHERE Skill.Service = 1
              AND Skill.GroupID > 0
              AND Skill.Basic_Level <= @Level
              AND Skill.ReqCommon_MasteryLevel1 <= DesiredMastery.MasteryLevel
              AND ISNULL(Skill.ReqCommon_Str, 0) <= @Strength
              AND ISNULL(Skill.ReqCommon_Int, 0) <= @Intellect
              AND
              (
                  ISNULL(Skill.ReqCommon_Mastery2, 0) = 0
                  OR EXISTS
                  (
                      SELECT 1
                      FROM @ClientlessDesiredMasteries AS SecondaryMastery
                      WHERE SecondaryMastery.MasteryID = Skill.ReqCommon_Mastery2
                        AND SecondaryMastery.MasteryLevel >= Skill.ReqCommon_MasteryLevel2
                  )
              )
        )
        SELECT SkillID, GroupID
        INTO #ClientlessMaxSkills
        FROM RankedSkills
        WHERE RankOrder = 1;

        DELETE Learned
        FROM {shardDbName}.dbo._CharSkill AS Learned
        INNER JOIN {shardDbName}.dbo._RefSkill AS CurrentSkill WITH (NOLOCK)
            ON CurrentSkill.ID = Learned.SkillID
        INNER JOIN #ClientlessMaxSkills AS Desired
            ON Desired.GroupID = CurrentSkill.GroupID
        WHERE Learned.CharID = @NewCharID
          AND Learned.SkillID <> Desired.SkillID;

        INSERT INTO {shardDbName}.dbo._CharSkill (CharID, SkillID, Enable)
        SELECT @NewCharID, Desired.SkillID, 1
        FROM #ClientlessMaxSkills AS Desired
        WHERE NOT EXISTS
        (
            SELECT 1
            FROM {shardDbName}.dbo._CharSkill AS Learned WITH (NOLOCK)
            WHERE Learned.CharID = @NewCharID AND Learned.SkillID = Desired.SkillID
        );

        IF NOT EXISTS
        (
            SELECT 1
            FROM {shardDbName}.dbo._CharSkill AS Learned WITH (NOLOCK)
            INNER JOIN {shardDbName}.dbo._RefSkill AS AttackSkill WITH (NOLOCK)
                ON AttackSkill.ID = Learned.SkillID AND AttackSkill.Service = 1
            WHERE Learned.CharID = @NewCharID AND Learned.Enable = 1
              AND AttackSkill.ReqCommon_Mastery1 = @MasteryID
              AND ({attackSkillPredicate})
              AND ({weaponSkillPredicate})
        )
           AND {unlockedAttackSkillExistsSql}
            THROW 51012, 'No valid monster attack skill could be prepared for the selected Clientless weapon.', 1;
    END

    IF OBJECT_ID(N'{shardDb}.dbo._CharQuest', N'U') IS NOT NULL
    BEGIN
        INSERT INTO {shardDbName}.dbo._CharQuest (CharID, QuestID, Status, AchievementCount, StartTime, EndTime, QuestData1, QuestData2)
        SELECT @NewCharID, Q.ID, 1, 0, GETDATE(), GETDATE(), 0, 0
        FROM {shardDbName}.dbo._RefQuest Q WITH (NOLOCK)
        WHERE Q.CodeName IN
        (
            SELECT CodeName
            FROM {shardDbName}.dbo._RefCharDefault_Quest WITH (NOLOCK)
            WHERE (Race = @Country OR Race = 3) AND RequiredLevel = 1 AND Service = 1
        )
          AND NOT EXISTS
          (
              SELECT 1 FROM {shardDbName}.dbo._CharQuest CQ WITH (NOLOCK)
              WHERE CQ.CharID = @NewCharID AND CQ.QuestID = Q.ID
          );
    END

    IF OBJECT_ID(N'{shardDb}.dbo._StaticAvatar', N'U') IS NOT NULL
       AND NOT EXISTS (SELECT 1 FROM {shardDbName}.dbo._StaticAvatar WITH (NOLOCK) WHERE CharID = @NewCharID)
        INSERT INTO {shardDbName}.dbo._StaticAvatar (CharID) VALUES (@NewCharID);

    IF OBJECT_ID(N'{shardDb}.dbo._CharTrijob', N'U') IS NOT NULL
       AND NOT EXISTS (SELECT 1 FROM {shardDbName}.dbo._CharTrijob WITH (NOLOCK) WHERE CharID = @NewCharID)
        INSERT INTO {shardDbName}.dbo._CharTrijob (CharID, JobType, Level, Exp, Contribution, Reward)
        VALUES (@NewCharID, 0, 1, 0, 0, 0);

    IF OBJECT_ID(N'{shardDb}.dbo._CharNameList', N'U') IS NOT NULL
       AND NOT EXISTS (SELECT 1 FROM {shardDbName}.dbo._CharNameList WITH (NOLOCK) WHERE CharID = @NewCharID)
        INSERT INTO {shardDbName}.dbo._CharNameList (CharName16, CharID)
        VALUES (@CharacterName, @NewCharID);

    IF OBJECT_ID(N'{shardDb}.dbo._Chest', N'U') IS NOT NULL
    BEGIN
        DECLARE @Slot INT = 1;
        WHILE @Slot <= 239
        BEGIN
            IF NOT EXISTS (SELECT 1 FROM {shardDbName}.dbo._Chest WITH (NOLOCK) WHERE UserJID = @UserJID AND Slot = @Slot)
                INSERT INTO {shardDbName}.dbo._Chest (UserJID, Slot) VALUES (@UserJID, @Slot);
            SET @Slot += 1;
        END
    END

    IF OBJECT_ID(N'{shardDb}.dbo._InsertToChestInfo', N'P') IS NOT NULL
        EXEC {shardDbName}.dbo._InsertToChestInfo;

    IF OBJECT_ID(N'{shardDb}.dbo._AddNewClientConfig', N'P') IS NOT NULL
        EXEC {shardDbName}.dbo._AddNewClientConfig @NewCharID;

SELECT @NewCharID;";

        await using var command = new SqlCommand(sql, connection, transaction)
        {
            CommandTimeout = 120
        };
        command.Parameters.Add("@AccountDbName", SqlDbType.NVarChar, 128).Value = accountDb;
        command.Parameters.Add("@ShardDbName", SqlDbType.NVarChar, 128).Value = shardDb;
        command.Parameters.Add("@AccountName", SqlDbType.VarChar, 24).Value = accountName;
        command.Parameters.Add("@Password", SqlDbType.VarChar, 50).Value = password;
        command.Parameters.Add("@CharacterName", SqlDbType.VarChar, 64).Value = characterName;
        command.Parameters.Add("@RefCharID", SqlDbType.Int).Value = refCharId;
        command.Parameters.Add("@CharacterCountry", SqlDbType.TinyInt).Value = race.Equals("Chinese", StringComparison.OrdinalIgnoreCase) ? 0 : 1;
        command.Parameters.Add("@Scale", SqlDbType.TinyInt).Value = gender.Equals("Male", StringComparison.OrdinalIgnoreCase) ? 68 : 48;
        command.Parameters.Add("@Strength", SqlDbType.SmallInt).Value = strength;
        command.Parameters.Add("@Intellect", SqlDbType.SmallInt).Value = intellect;
        command.Parameters.Add("@InitialHP", SqlDbType.Int).Value = CalculateClientlessInitialHealth(strength, level);
        command.Parameters.Add("@InitialMP", SqlDbType.Int).Value = CalculateClientlessInitialMana(intellect, level);
        command.Parameters.Add("@RegionID", SqlDbType.Int).Value = spawnPosition.RegionId;
        command.Parameters.Add("@PosX", SqlDbType.Real).Value = spawnPosition.PosX;
        command.Parameters.Add("@PosY", SqlDbType.Real).Value = spawnPosition.PosY;
        command.Parameters.Add("@PosZ", SqlDbType.Real).Value = spawnPosition.PosZ;
        command.Parameters.Add("@InventorySize", SqlDbType.Int).Value = 109;
        command.Parameters.Add("@Level", SqlDbType.TinyInt).Value = (byte)Math.Clamp(level, 1, byte.MaxValue);
        command.Parameters.Add("@RequiredGender", SqlDbType.TinyInt).Value = gender.Equals("Male", StringComparison.OrdinalIgnoreCase) ? 1 : 0;
        command.Parameters.Add("@ArmorTypeID3", SqlDbType.TinyInt).Value = equipmentProfile.ArmorTypeId3;
        command.Parameters.Add("@WeaponTypeID4", SqlDbType.TinyInt).Value = equipmentProfile.WeaponTypeId4;
        command.Parameters.Add("@UseShield", SqlDbType.Bit).Value = equipmentProfile.UseShield;
        command.Parameters.Add("@CreateEquipment", SqlDbType.Bit).Value = equipmentSelection != "None";
        command.Parameters.Add("@EquipmentRarity", SqlDbType.TinyInt).Value = equipmentSelection == "Rare" ? 2 : 0;
        command.Parameters.Add("@MasteryID", SqlDbType.Int).Value = equipmentProfile.MasteryId;
        command.Parameters.Add("@MasteryCap", SqlDbType.Int).Value = Math.Clamp(masteryCap, 1, 10000);
        command.Parameters.Add("@OpenMastery", SqlDbType.Bit).Value = skillSelection is "Mastery" or "MasteryAndSkills";
        command.Parameters.Add("@MaxMasterySkills", SqlDbType.Bit).Value = skillSelection == "MasteryAndSkills";
        command.Parameters.Add("@AttackParam", SqlDbType.Int).Value = 6386804;
        command.Parameters.Add("@RequiredItemParam", SqlDbType.Int).Value = 1919250793;

        return Convert.ToInt32(await command.ExecuteScalarAsync());
    }

    private static async Task EnsureClientlessCharacterEssentialsAsync(
        SqlConnection connection,
        string accountDb,
        string shardDb,
        string accountName,
        string characterName)
    {
        if (string.IsNullOrWhiteSpace(accountName) || string.IsNullOrWhiteSpace(characterName))
            return;

        var accountDbName = QuoteDb(accountDb);
        var shardDbName = QuoteDb(shardDb);
        var sql = $@"
DECLARE @UserJID INT = 0;
DECLARE @CharID INT = 0;
DECLARE @RefCharID INT = 0;
DECLARE @Country TINYINT = 0;

SELECT @UserJID = JID
FROM {accountDbName}.dbo.TB_User WITH (NOLOCK)
WHERE StrUserID = @AccountName;

SELECT @CharID = CharID, @RefCharID = RefObjID
FROM {shardDbName}.dbo._Char WITH (NOLOCK)
WHERE CharName16 = @CharacterName;

IF ISNULL(@UserJID, 0) <= 0 OR ISNULL(@CharID, 0) <= 0
    RETURN;

EXEC @Country = {shardDbName}.dbo._GetObjCountry @RefCharID;

IF OBJECT_ID(N'{shardDb}.dbo._CharSkillMastery', N'U') IS NOT NULL
BEGIN
    INSERT INTO {shardDbName}.dbo._CharSkillMastery (CharID, MasteryID, Level)
    SELECT @CharID, DSM.MasteryID, 0
    FROM {shardDbName}.dbo._RefCharDefault_SkillMastery DSM WITH (NOLOCK)
    WHERE (DSM.Race = @Country OR DSM.Race = 3)
      AND NOT EXISTS
      (
          SELECT 1 FROM {shardDbName}.dbo._CharSkillMastery M WITH (NOLOCK)
          WHERE M.CharID = @CharID AND M.MasteryID = DSM.MasteryID
      );
END

IF OBJECT_ID(N'{shardDb}.dbo._CharSkill', N'U') IS NOT NULL
BEGIN
    INSERT INTO {shardDbName}.dbo._CharSkill (CharID, SkillID, Enable)
    SELECT @CharID, DS.SkillID, 1
    FROM {shardDbName}.dbo._RefCharDefault_Skill DS WITH (NOLOCK)
    WHERE (DS.Race = @Country OR DS.Race = 3)
      AND NOT EXISTS
      (
          SELECT 1 FROM {shardDbName}.dbo._CharSkill S WITH (NOLOCK)
          WHERE S.CharID = @CharID AND S.SkillID = DS.SkillID
      );
END

IF OBJECT_ID(N'{shardDb}.dbo._CharQuest', N'U') IS NOT NULL
BEGIN
    INSERT INTO {shardDbName}.dbo._CharQuest (CharID, QuestID, Status, AchievementCount, StartTime, EndTime, QuestData1, QuestData2)
    SELECT @CharID, Q.ID, 1, 0, GETDATE(), GETDATE(), 0, 0
    FROM {shardDbName}.dbo._RefQuest Q WITH (NOLOCK)
    WHERE Q.CodeName IN
    (
        SELECT CodeName
        FROM {shardDbName}.dbo._RefCharDefault_Quest WITH (NOLOCK)
        WHERE (Race = @Country OR Race = 3) AND RequiredLevel = 1 AND Service = 1
    )
      AND NOT EXISTS
      (
          SELECT 1 FROM {shardDbName}.dbo._CharQuest CQ WITH (NOLOCK)
          WHERE CQ.CharID = @CharID AND CQ.QuestID = Q.ID
      );
END

IF OBJECT_ID(N'{shardDb}.dbo._StaticAvatar', N'U') IS NOT NULL
   AND NOT EXISTS (SELECT 1 FROM {shardDbName}.dbo._StaticAvatar WITH (NOLOCK) WHERE CharID = @CharID)
    INSERT INTO {shardDbName}.dbo._StaticAvatar (CharID) VALUES (@CharID);

IF OBJECT_ID(N'{shardDb}.dbo._CharTrijob', N'U') IS NOT NULL
   AND NOT EXISTS (SELECT 1 FROM {shardDbName}.dbo._CharTrijob WITH (NOLOCK) WHERE CharID = @CharID)
    INSERT INTO {shardDbName}.dbo._CharTrijob (CharID, JobType, Level, Exp, Contribution, Reward)
    VALUES (@CharID, 0, 1, 0, 0, 0);

IF OBJECT_ID(N'{shardDb}.dbo._CharNameList', N'U') IS NOT NULL
   AND NOT EXISTS (SELECT 1 FROM {shardDbName}.dbo._CharNameList WITH (NOLOCK) WHERE CharID = @CharID)
    INSERT INTO {shardDbName}.dbo._CharNameList (CharName16, CharID)
    VALUES (@CharacterName, @CharID);

IF OBJECT_ID(N'{shardDb}.dbo._Chest', N'U') IS NOT NULL
BEGIN
    DECLARE @Slot INT = 1;
    WHILE @Slot <= 239
    BEGIN
        IF NOT EXISTS (SELECT 1 FROM {shardDbName}.dbo._Chest WITH (NOLOCK) WHERE UserJID = @UserJID AND Slot = @Slot)
            INSERT INTO {shardDbName}.dbo._Chest (UserJID, Slot) VALUES (@UserJID, @Slot);
        SET @Slot += 1;
    END
END

IF OBJECT_ID(N'{shardDb}.dbo._InsertToChestInfo', N'P') IS NOT NULL
    EXEC {shardDbName}.dbo._InsertToChestInfo;

IF OBJECT_ID(N'{shardDb}.dbo._AddNewClientConfig', N'P') IS NOT NULL
    EXEC {shardDbName}.dbo._AddNewClientConfig @CharID;";

        await using var command = new SqlCommand(sql, connection)
        {
            CommandTimeout = 120
        };
        command.Parameters.Add("@AccountName", SqlDbType.VarChar, 24).Value = accountName.Trim();
        command.Parameters.Add("@CharacterName", SqlDbType.VarChar, 64).Value = characterName.Trim();
        await command.ExecuteNonQueryAsync();
    }

    private static int ResolveClientlessRefCharId(string race, string gender)
    {
        return (race, gender) switch
        {
            ("Chinese", "Male") => Random.Shared.Next(1907, 1920),
            ("Chinese", "Female") => Random.Shared.Next(1920, 1933),
            ("Europe", "Male") => Random.Shared.Next(14875, 14888),
            ("Europe", "Female") => Random.Shared.Next(14888, 14901),
            _ => Random.Shared.Next(1907, 1920)
        };
    }

    internal static IReadOnlyList<string> GetClientlessWeaponChoices(string raceSelection)
    {
        var race = NormalizeClientlessRaceSelection(raceSelection);
        return new[] { "Random" }
            .Concat(ClientlessWeaponProfiles
                .Where(profile => race == "Random" || profile.Race == race)
                .Select(profile => profile.Weapon)
                .Distinct(StringComparer.OrdinalIgnoreCase))
            .ToArray();
    }

    internal static ClientlessWeaponProfile ResolveClientlessWeaponProfile(
        string raceSelection,
        string weaponSelection)
    {
        var race = NormalizeClientlessRaceSelection(raceSelection);
        var weapon = NormalizeClientlessWeaponSelection(weaponSelection);
        var candidates = ClientlessWeaponProfiles
            .Where(profile => race == "Random" || profile.Race == race)
            .Where(profile => weapon == "Random" || profile.Weapon == weapon)
            .ToArray();

        if (candidates.Length == 0)
            throw new InvalidOperationException($"Weapon '{weapon}' is not available for race '{race}'.");

        if (race == "Random" && weapon == "Random")
        {
            var chosenRace = Random.Shared.Next(0, 2) == 0 ? "Chinese" : "Europe";
            candidates = candidates.Where(profile => profile.Race == chosenRace).ToArray();
        }

        // Random means an equal weapon-category chance. Sword/Blade deliberately
        // have multiple armor variants; choosing from raw profiles doubled their
        // probability compared with Spear/Glaive/Bow.
        var weaponCategory = candidates
            .Select(profile => profile.Weapon)
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .OrderBy(value => value, StringComparer.OrdinalIgnoreCase)
            .ElementAt(Random.Shared.Next(candidates
                .Select(profile => profile.Weapon)
                .Distinct(StringComparer.OrdinalIgnoreCase)
                .Count()));
        var categoryProfiles = candidates
            .Where(profile => profile.Weapon.Equals(weaponCategory, StringComparison.OrdinalIgnoreCase))
            .ToArray();
        return categoryProfiles[Random.Shared.Next(categoryProfiles.Length)];
    }

    private static string NormalizeClientlessRaceSelection(string? value) =>
        NormalizeClientlessSelection(
            value,
            "race",
            new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["Random"] = "Random",
                ["Chinese"] = "Chinese",
                ["Europe"] = "Europe",
                ["European"] = "Europe"
            });

    private static string NormalizeClientlessWeaponSelection(string? value)
    {
        var trimmed = string.IsNullOrWhiteSpace(value) ? "Random" : value.Trim();
        if (trimmed.Equals("Random", StringComparison.OrdinalIgnoreCase))
            return "Random";

        var profile = ClientlessWeaponProfiles.FirstOrDefault(item =>
            item.Weapon.Equals(trimmed, StringComparison.OrdinalIgnoreCase));
        if (profile is null)
            throw new InvalidOperationException($"Unknown clientless weapon selection '{trimmed}'.");
        return profile.Weapon;
    }

    private static string NormalizeClientlessGenderSelection(string? value) =>
        NormalizeClientlessSelection(
            value,
            "gender",
            new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["Random"] = "Random",
                ["Male"] = "Male",
                ["Female"] = "Female"
            });

    private static string NormalizeClientlessEquipmentSelection(string? value) =>
        NormalizeClientlessSelection(
            string.IsNullOrWhiteSpace(value) ? "Rare" : value,
            "equipment",
            new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["Rare"] = "Rare",
                ["C_RARE"] = "Rare",
                ["Normal"] = "Normal",
                ["None"] = "None",
                ["No Equipment"] = "None"
            });

    private static string NormalizeClientlessSkillSelection(string? value) =>
        NormalizeClientlessSelection(
            string.IsNullOrWhiteSpace(value) ? "MasteryAndSkills" : value,
            "skill setup",
            new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["Default"] = "Default",
                ["Default skills"] = "Default",
                ["Mastery"] = "Mastery",
                ["Open weapon mastery"] = "Mastery",
                ["MasteryAndSkills"] = "MasteryAndSkills",
                ["Open mastery + max all skills"] = "MasteryAndSkills"
            });

    private static string NormalizeClientlessBuildSelection(string? value) =>
        NormalizeClientlessSelection(
            value,
            "stat build",
            new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
            {
                ["Random"] = "Random",
                ["Strength"] = "Strength",
                ["STR"] = "Strength",
                ["Full STR"] = "Strength",
                ["Intelligence"] = "Intelligence",
                ["INT"] = "Intelligence",
                ["Full INT"] = "Intelligence"
            });

    internal static (short Strength, short Intellect) CalculateClientlessStats(int level, string buildSelection)
    {
        const int baseStat = 20;
        var safeLevel = Math.Clamp(level, 1, byte.MaxValue);
        var levelUps = safeLevel - 1;
        var build = NormalizeClientlessBuildSelection(buildSelection);
        if (build == "Random")
            throw new InvalidOperationException("Resolve the random Clientless stat build before calculating character stats.");

        var naturalStat = baseStat + levelUps;
        var assignablePoints = levelUps * 3;
        var strength = naturalStat + (build == "Strength" ? assignablePoints : 0);
        var intellect = naturalStat + (build == "Intelligence" ? assignablePoints : 0);
        return ((short)Math.Clamp(strength, 1, short.MaxValue),
                (short)Math.Clamp(intellect, 1, short.MaxValue));
    }

    internal const int ClientlessSurvivalStatBonusPerLevel = 30;

    internal static (short Strength, short Intellect) CalculateClientlessCombatStats(int level, string buildSelection)
    {
        var (baseStrength, baseIntellect) = CalculateClientlessStats(level, buildSelection);
        var safeLevel = Math.Clamp(level, 1, byte.MaxValue);
        var survivalBonus = (safeLevel - 1) * ClientlessSurvivalStatBonusPerLevel;
        return (
            (short)Math.Clamp(baseStrength + survivalBonus, 1, short.MaxValue),
            (short)Math.Clamp(baseIntellect + survivalBonus, 1, short.MaxValue));
    }

    internal static int CalculateClientlessInitialHealth(int strength, int level)
    {
        // The GameServer publishes the exact maximum after login/equipment.
        // Seed a safe level-scaled current value instead of the level-1 default
        // 200 so a generated high-level character does not enter nearly dead.
        var safeStrength = Math.Clamp(strength, 1, short.MaxValue);
        var safeLevel = Math.Clamp(level, 1, byte.MaxValue);
        return Math.Clamp(200 + ((safeStrength - 20) * 20) + ((safeLevel - 1) * 20), 200, int.MaxValue);
    }

    internal static int CalculateClientlessInitialMana(int intellect, int level)
    {
        var safeIntellect = Math.Clamp(intellect, 1, short.MaxValue);
        var safeLevel = Math.Clamp(level, 1, byte.MaxValue);
        return Math.Clamp(200 + ((safeIntellect - 20) * 20) + ((safeLevel - 1) * 20), 200, int.MaxValue);
    }

    private static string NormalizeClientlessSelection(
        string? value,
        string selectionName,
        IReadOnlyDictionary<string, string> choices)
    {
        var trimmed = string.IsNullOrWhiteSpace(value) ? "Random" : value.Trim();
        if (choices.TryGetValue(trimmed, out var normalized))
            return normalized;

        throw new InvalidOperationException($"Unknown clientless {selectionName} selection '{trimmed}'.");
    }

    internal static IReadOnlyList<ClientlessSpawnPosition> CreateClientlessSpawnPositions(string town, int count)
    {
        var areas = ResolveClientlessSpawnAreas(town);
        var requestedCount = Math.Max(1, count);
        var minimumDistance = requestedCount switch
        {
            <= 25 => 110f,
            <= 50 => 80f,
            <= 100 => 58f,
            <= 250 => 38f,
            <= 500 => 27f,
            <= 1000 => 19f,
            <= 2500 => 11f,
            _ => 7.5f
        };

        var cellSize = minimumDistance;
        var minimumDistanceSquared = minimumDistance * minimumDistance;
        var positions = new List<ClientlessSpawnPosition>(requestedCount);
        var spatialGrid = new Dictionary<(int X, int Z), List<(float X, float Z)>>();

        (int X, int Z) CellFor(float x, float z) =>
            ((int)Math.Floor(x / cellSize), (int)Math.Floor(z / cellSize));

        static (float X, float Z) ToWorldPosition(ClientlessSpawnPosition position)
        {
            const float regionSize = 1920f;
            var regionX = position.RegionId & 0xFF;
            var regionZ = (position.RegionId >> 8) & 0xFF;
            return ((regionX * regionSize) + position.PosX, (regionZ * regionSize) + position.PosZ);
        }

        float FindNearestDistanceSquared(float x, float z)
        {
            var cell = CellFor(x, z);
            var nearest = float.MaxValue;
            for (var cellX = cell.X - 1; cellX <= cell.X + 1; cellX++)
            {
                for (var cellZ = cell.Z - 1; cellZ <= cell.Z + 1; cellZ++)
                {
                    if (!spatialGrid.TryGetValue((cellX, cellZ), out var nearby))
                        continue;

                    foreach (var point in nearby)
                    {
                        var dx = point.X - x;
                        var dz = point.Z - z;
                        nearest = Math.Min(nearest, (dx * dx) + (dz * dz));
                    }
                }
            }

            return nearest;
        }

        void AddPosition(ClientlessSpawnPosition position)
        {
            positions.Add(position);
            var worldPosition = ToWorldPosition(position);
            var cell = CellFor(worldPosition.X, worldPosition.Z);
            if (!spatialGrid.TryGetValue(cell, out var bucket))
            {
                bucket = new List<(float X, float Z)>();
                spatialGrid[cell] = bucket;
            }

            bucket.Add(worldPosition);
        }

        for (var index = 0; index < requestedCount; index++)
        {
            ClientlessSpawnPosition? accepted = null;
            ClientlessSpawnPosition? bestFallback = null;
            var bestNearestDistanceSquared = -1f;
            for (var attempt = 0; attempt < 960; attempt++)
            {
                var area = areas[(index + attempt) % areas.Count];
                var angle = Random.Shared.NextDouble() * Math.PI * 2d;
                var radialSample = Math.Sqrt(0.01d + (Random.Shared.NextDouble() * 0.99d));
                var x = area.CenterX + (float)(Math.Cos(angle) * area.RadiusX * radialSample);
                var z = area.CenterZ + (float)(Math.Sin(angle) * area.RadiusZ * radialSample);

                x += (float)((Random.Shared.NextDouble() - 0.5d) * 4d);
                z += (float)((Random.Shared.NextDouble() - 0.5d) * 4d);

                if (x is < 16f or > 1904f || z is < 16f or > 1904f)
                    continue;

                var candidate = new ClientlessSpawnPosition(area.RegionId, x, area.PosY, z);
                var worldPosition = ToWorldPosition(candidate);
                var nearestDistanceSquared = FindNearestDistanceSquared(worldPosition.X, worldPosition.Z);
                if (nearestDistanceSquared > bestNearestDistanceSquared)
                {
                    bestNearestDistanceSquared = nearestDistanceSquared;
                    bestFallback = candidate;
                }

                if (nearestDistanceSquared < minimumDistanceSquared)
                    continue;

                accepted = candidate;
                break;
            }

            if (!accepted.HasValue)
            {
                var fallbackArea = areas[index % areas.Count];
                accepted = bestFallback ?? new ClientlessSpawnPosition(
                    fallbackArea.RegionId,
                    fallbackArea.CenterX,
                    fallbackArea.PosY,
                    fallbackArea.CenterZ);
            }

            AddPosition(accepted.Value);
        }

        return positions;
    }

    private static IReadOnlyList<ClientlessSpawnArea> ResolveClientlessSpawnAreas(string town)
    {
        return town switch
        {
            "Donwhang" =>
            [
                new(26265, 957f, -80f, 1508f, 220f, 180f),
                new(26265, 600f, -105f, 700f, 190f, 160f),
                new(26265, 1300f, -105f, 700f, 190f, 160f),
                new(26265, 1000f, -105f, 300f, 190f, 150f),
                new(26521, 900f, -95f, 1700f, 190f, 150f)
            ],
            "Hotan" =>
            [
                new(23687, 1138f, 245f, 600f, 220f, 180f),
                new(23431, 1100f, 245f, 1700f, 200f, 150f),
                new(23686, 1073f, 13f, 475f, 180f, 140f),
                new(23688, 1254f, 14f, 480f, 180f, 140f),
                new(23943, 1145f, 145f, 1608f, 190f, 140f)
            ],
            "SamarKand" =>
            [
                new(27244, 600f, 180f, 1400f, 210f, 170f),
                new(27243, 1550f, 180f, 1550f, 190f, 160f),
                new(27244, 500f, 180f, 500f, 190f, 160f),
                new(27499, 1400f, 180f, 500f, 200f, 160f),
                new(27500, 500f, 180f, 700f, 200f, 170f)
            ],
            "Constantinople" =>
            [
                new(26959, 950f, 84f, 1070f, 220f, 180f),
                new(26958, 1400f, 84f, 900f, 200f, 170f),
                new(26957, 1450f, 80f, 1400f, 190f, 160f),
                new(26702, 900f, 84f, 1000f, 210f, 180f),
                new(27471, 1250f, 80f, 500f, 190f, 150f)
            ],
            "Alexandria North (SD)" =>
            [
                new(23603, 111f, 1537f, 524f, 90f, 160f),
                new(23603, 462f, 1530f, 241f, 180f, 150f),
                new(23603, 1180f, 1560f, 990f, 220f, 180f),
                new(23602, 829f, 1408f, 346f, 210f, 170f),
                new(23602, 1225f, 1448f, 529f, 210f, 170f)
            ],
            _ =>
            [
                new(25000, 995f, -32f, 1132f, 220f, 180f),
                new(24999, 1100f, 0f, 900f, 210f, 180f),
                new(25001, 850f, 0f, 1100f, 220f, 190f),
                new(25000, 1000f, 0f, 250f, 190f, 160f),
                new(25000, 1000f, 0f, 1700f, 190f, 160f)
            ]
        };
    }

    private readonly record struct ClientlessSpawnArea(int RegionId, float CenterX, float PosY, float CenterZ, float RadiusX, float RadiusZ);
    internal readonly record struct ClientlessSpawnPosition(int RegionId, float PosX, float PosY, float PosZ);
    private readonly record struct ClientlessSpawnAssignment(string Town, ClientlessSpawnPosition Position);
    private readonly record struct ClientlessCityAllocation(string Town, int Count);

    private static async Task UpsertClientlessAccountAsync(
        SqlConnection connection,
        SqlTransaction transaction,
        string accountName,
        string password,
        string characterName,
        string city,
        int shardId,
        byte locale,
        int launchDelayMs,
        int reconnectDelaySeconds,
        ClientlessWeaponProfile? equipmentProfile = null,
        string? equipmentMode = null,
        string? skillMode = null,
        string? build = null,
        ClientlessSpawnPosition? homePosition = null,
        bool? avatarsEnabled = null,
        bool? attackPetEnabled = null,
        bool? grabPetEnabled = null)
    {
        await using var command = new SqlCommand(@"
IF EXISTS (SELECT 1 FROM [dbo].[Clientless_Accounts] WHERE AccountName = @AccountName AND CharacterName = @CharacterName)
BEGIN
    UPDATE [dbo].[Clientless_Accounts]
    SET Enabled = 1,
        Locale = @Locale,
        ShardID = @ShardID,
        City = @City,
        AccountPassword = CASE WHEN @AccountPassword = '' THEN AccountPassword ELSE @AccountPassword END,
        AgentAuthMode = 'Auto',
        AgentAuthDelayMs = 1500,
        AgentAuthPaddingHex = '',
        LaunchDelayMs = @LaunchDelayMs,
        ReconnectDelaySeconds = @ReconnectDelaySeconds,
        ProfileRace = COALESCE(@ProfileRace, ProfileRace),
        ProfileWeaponTypeID4 = COALESCE(@ProfileWeaponTypeID4, ProfileWeaponTypeID4),
        ProfileArmorTypeID3 = COALESCE(@ProfileArmorTypeID3, ProfileArmorTypeID3),
        ProfileUseShield = COALESCE(@ProfileUseShield, ProfileUseShield),
        ProfileEquipmentMode = COALESCE(@ProfileEquipmentMode, ProfileEquipmentMode),
        ProfileSkillMode = COALESCE(@ProfileSkillMode, ProfileSkillMode),
        ProfileBuild = COALESCE(@ProfileBuild, ProfileBuild),
        ProfileAvatarsEnabled = COALESCE(@ProfileAvatarsEnabled, ProfileAvatarsEnabled),
        ProfilePetsEnabled = COALESCE(@ProfilePetsEnabled, ProfilePetsEnabled),
        ProfileAttackPetEnabled = COALESCE(@ProfileAttackPetEnabled, ProfileAttackPetEnabled),
        ProfileGrabPetEnabled = COALESCE(@ProfileGrabPetEnabled, ProfileGrabPetEnabled),
        HomeRegionID = COALESCE(@HomeRegionID, HomeRegionID),
        HomeX = COALESCE(@HomeX, HomeX),
        HomeY = COALESCE(@HomeY, HomeY),
        HomeZ = COALESCE(@HomeZ, HomeZ),
        LastStatus = 'Pending',
        LastMessage = NULL,
        UpdatedAt = SYSDATETIME()
    WHERE AccountName = @AccountName AND CharacterName = @CharacterName;
END
ELSE
BEGIN
    IF @AccountPassword = ''
        THROW 51010, 'A password is required for a new clientless account.', 1;

    INSERT INTO [dbo].[Clientless_Accounts]
        (Enabled, Locale, ShardID, AccountName, AccountPassword, CharacterName, City, AgentAuthMode, AgentAuthDelayMs, AgentAuthPaddingHex, LaunchDelayMs, ReconnectDelaySeconds, LastStatus,
         ProfileRace, ProfileWeaponTypeID4, ProfileArmorTypeID3, ProfileUseShield, ProfileEquipmentMode, ProfileSkillMode, ProfileBuild,
         ProfileAvatarsEnabled, ProfilePetsEnabled, ProfileAttackPetEnabled, ProfileGrabPetEnabled,
         HomeRegionID, HomeX, HomeY, HomeZ)
    VALUES
        (1, @Locale, @ShardID, @AccountName, @AccountPassword, @CharacterName, @City, 'Auto', 1500, '', @LaunchDelayMs, @ReconnectDelaySeconds, 'Pending',
         @ProfileRace, @ProfileWeaponTypeID4, @ProfileArmorTypeID3, @ProfileUseShield, @ProfileEquipmentMode, @ProfileSkillMode, @ProfileBuild,
         COALESCE(@ProfileAvatarsEnabled, 1),
         COALESCE(@ProfilePetsEnabled, 1),
         COALESCE(@ProfileAttackPetEnabled, COALESCE(@ProfilePetsEnabled, 1)),
         COALESCE(@ProfileGrabPetEnabled, COALESCE(@ProfilePetsEnabled, 1)),
         @HomeRegionID, @HomeX, @HomeY, @HomeZ);
END", connection, transaction);

        command.Parameters.Add("@AccountName", SqlDbType.VarChar, 64).Value = accountName.Trim();
        command.Parameters.Add("@AccountPassword", SqlDbType.VarChar, 128).Value = password;
        command.Parameters.Add("@CharacterName", SqlDbType.VarChar, 64).Value = characterName.Trim();
        command.Parameters.Add("@City", SqlDbType.VarChar, 32).Value = NormalizeClientlessTownOrUnassigned(city);
        command.Parameters.Add("@ShardID", SqlDbType.SmallInt).Value = Math.Clamp(shardId, 1, short.MaxValue);
        command.Parameters.Add("@Locale", SqlDbType.TinyInt).Value = locale;
        command.Parameters.Add("@LaunchDelayMs", SqlDbType.Int).Value = Math.Clamp(launchDelayMs, 0, 60000);
        command.Parameters.Add("@ReconnectDelaySeconds", SqlDbType.Int).Value = Math.Clamp(reconnectDelaySeconds, 5, 600);
        command.Parameters.Add("@ProfileRace", SqlDbType.VarChar, 16).Value = (object?)equipmentProfile?.Race ?? DBNull.Value;
        command.Parameters.Add("@ProfileWeaponTypeID4", SqlDbType.TinyInt).Value = (object?)equipmentProfile?.WeaponTypeId4 ?? DBNull.Value;
        command.Parameters.Add("@ProfileArmorTypeID3", SqlDbType.TinyInt).Value = (object?)equipmentProfile?.ArmorTypeId3 ?? DBNull.Value;
        command.Parameters.Add("@ProfileUseShield", SqlDbType.Bit).Value = (object?)equipmentProfile?.UseShield ?? DBNull.Value;
        command.Parameters.Add("@ProfileEquipmentMode", SqlDbType.VarChar, 16).Value = (object?)equipmentMode ?? DBNull.Value;
        command.Parameters.Add("@ProfileSkillMode", SqlDbType.VarChar, 32).Value = (object?)skillMode ?? DBNull.Value;
        command.Parameters.Add("@ProfileBuild", SqlDbType.VarChar, 16).Value = (object?)build ?? DBNull.Value;
        command.Parameters.Add("@ProfileAvatarsEnabled", SqlDbType.Bit).Value = (object?)avatarsEnabled ?? DBNull.Value;
        var anyPetEnabled = attackPetEnabled.HasValue || grabPetEnabled.HasValue
            ? attackPetEnabled.GetValueOrDefault() || grabPetEnabled.GetValueOrDefault()
            : (bool?)null;
        command.Parameters.Add("@ProfilePetsEnabled", SqlDbType.Bit).Value = (object?)anyPetEnabled ?? DBNull.Value;
        command.Parameters.Add("@ProfileAttackPetEnabled", SqlDbType.Bit).Value = (object?)attackPetEnabled ?? DBNull.Value;
        command.Parameters.Add("@ProfileGrabPetEnabled", SqlDbType.Bit).Value = (object?)grabPetEnabled ?? DBNull.Value;
        command.Parameters.Add("@HomeRegionID", SqlDbType.Int).Value = (object?)homePosition?.RegionId ?? DBNull.Value;
        command.Parameters.Add("@HomeX", SqlDbType.Real).Value = (object?)homePosition?.PosX ?? DBNull.Value;
        command.Parameters.Add("@HomeY", SqlDbType.Real).Value = (object?)homePosition?.PosY ?? DBNull.Value;
        command.Parameters.Add("@HomeZ", SqlDbType.Real).Value = (object?)homePosition?.PosZ ?? DBNull.Value;
        await command.ExecuteNonQueryAsync();
    }

    private static async Task<string> ReadSettingAsync(SqlConnection connection, string settingName, string fallback)
    {
        if (!await ObjectExistsAsync(connection, "[dbo].[System_Settings]", "U"))
            return fallback;

        await using var command = new SqlCommand("SELECT TOP (1) CONVERT(NVARCHAR(128), Value) FROM [dbo].[System_Settings] WITH (NOLOCK) WHERE SettingName = @SettingName", connection);
        command.Parameters.Add("@SettingName", SqlDbType.NVarChar, 128).Value = settingName;
        var value = Convert.ToString(await command.ExecuteScalarAsync());
        return string.IsNullOrWhiteSpace(value) ? fallback : value;
    }

    private static async Task<int> ReadGameServerSettingIntAsync(
        SqlConnection connection,
        string settingName,
        int fallback,
        int minimum,
        int maximum)
    {
        if (!await ObjectExistsAsync(connection, "[dbo].[System_GameServerSettings]", "U"))
            return Math.Clamp(fallback, minimum, maximum);

        await using var command = new SqlCommand(
            "SELECT TOP (1) CONVERT(NVARCHAR(128), Value) FROM dbo.System_GameServerSettings WITH (NOLOCK) WHERE SettingName = @SettingName",
            connection);
        command.Parameters.Add("@SettingName", SqlDbType.NVarChar, 128).Value = settingName;
        var rawValue = Convert.ToString(await command.ExecuteScalarAsync());
        return int.TryParse(rawValue, out var parsed)
            ? Math.Clamp(parsed, minimum, maximum)
            : Math.Clamp(fallback, minimum, maximum);
    }

    private async Task<PlayerCommandTarget> ResolvePlayerCommandTargetAsync(
        SqlConnection connection,
        string characterName)
    {
        var normalizedName = characterName.Trim();
        if (string.IsNullOrWhiteSpace(normalizedName))
            throw new InvalidOperationException("Character name is required.");
        if (normalizedName.Length > 64)
            throw new InvalidOperationException("Character name is too long.");

        var shardDb = await ReadSettingAsync(connection, "ShardDB", "SRO_VT_SHARD");
        var accountDb = await ReadSettingAsync(connection, "AccountDB", "SRO_VT_ACCOUNT");
        var sql = $@"
SELECT TOP (1)
    C.CharID,
    C.CharName16,
    ISNULL(C.CurLevel, 0) AS CurLevel,
    ISNULL(C.LatestRegion, 0) AS LatestRegion,
    ISNULL(C.RemainGold, 0) AS RemainGold,
    ISNULL(U.UserJID, 0) AS UserJID,
    ISNULL(S.silk_own, 0) AS SilkOwn,
    ISNULL(S.silk_gift, 0) AS SilkGift,
    ISNULL(S.silk_point, 0) AS SilkPoint
FROM {QuoteDb(shardDb)}.dbo._Char AS C WITH (NOLOCK)
LEFT JOIN {QuoteDb(shardDb)}.dbo._User AS U WITH (NOLOCK)
    ON U.CharID = C.CharID
LEFT JOIN {QuoteDb(accountDb)}.dbo.SK_Silk AS S WITH (NOLOCK)
    ON S.JID = U.UserJID
WHERE C.CharName16 = @CharacterName;";

        await using var command = new SqlCommand(sql, connection);
        command.Parameters.Add("@CharacterName", SqlDbType.NVarChar, 64).Value = normalizedName;
        await using var reader = await command.ExecuteReaderAsync();
        if (!await reader.ReadAsync())
            throw new InvalidOperationException($"Character '{normalizedName}' was not found.");

        return new PlayerCommandTarget
        {
            CharId = Convert.ToInt32(reader["CharID"]),
            UserJid = Convert.ToInt32(reader["UserJID"]),
            CharacterName = Convert.ToString(reader["CharName16"]) ?? normalizedName,
            Level = Convert.ToInt32(reader["CurLevel"]),
            RegionId = Convert.ToInt32(reader["LatestRegion"]),
            Gold = Convert.ToInt64(reader["RemainGold"]),
            SilkOwn = Convert.ToInt32(reader["SilkOwn"]),
            SilkGift = Convert.ToInt32(reader["SilkGift"]),
            SilkPoint = Convert.ToInt32(reader["SilkPoint"])
        };
    }

    private async Task<int> ResolveCharIdAsync(SqlConnection connection, string charNameOrId)
    {
        if (int.TryParse(charNameOrId.Trim(), out var directId))
            return directId;

        var shardDb = await ReadSettingAsync(connection, "ShardDB", "SRO_VT_SHARD");
        await using var command = new SqlCommand($"SELECT TOP (1) CharID FROM {QuoteDb(shardDb)}.dbo._Char WITH (NOLOCK) WHERE CharName16 = @CharName", connection);
        command.Parameters.Add("@CharName", SqlDbType.NVarChar, 64).Value = charNameOrId.Trim();
        return Convert.ToInt32(await command.ExecuteScalarAsync() ?? 0);
    }

    private static async Task<DataTable> QueryTableAsync(SqlConnection connection, string sql, params SqlParameter[] parameters)
    {
        await using var command = new SqlCommand(sql, connection);
        foreach (var parameter in parameters)
            command.Parameters.Add(parameter);

        var table = new DataTable();
        await using var reader = await command.ExecuteReaderAsync();
        table.Load(reader);
        return table;
    }

    private static async Task<DataTable> LoadOptionalTableAsync(SqlConnection connection, string tableName, string sql, string term)
    {
        if (!await ObjectExistsAsync(connection, tableName, "U"))
            return new DataTable();

        return await QueryTableAsync(connection, sql, new SqlParameter("@Term", term.Trim()));
    }

    private static async Task<bool> EnsureDatabaseOwnerCanExecuteAsDboAsync(
        SqlConnection connection,
        string databaseName)
    {
        var quotedDatabase = QuoteDb(databaseName);
        await using var check = new SqlCommand(
            @"SELECT CASE WHEN SUSER_SNAME(owner_sid) IS NULL THEN 0 ELSE 1 END
              FROM sys.databases
              WHERE name = @DatabaseName;",
            connection);
        check.Parameters.Add("@DatabaseName", SqlDbType.NVarChar, 128).Value = databaseName;
        var ownerResult = await check.ExecuteScalarAsync();
        if (ownerResult == null || ownerResult == DBNull.Value)
            throw new InvalidOperationException($"Database {databaseName} was not found.");

        var ownerIsValid = Convert.ToInt32(ownerResult) == 1;
        if (ownerIsValid)
            return false;

        await using var ownerCommand = new SqlCommand(
            "SELECT TOP (1) name FROM sys.server_principals WHERE principal_id = 1 AND type = 'S';",
            connection);
        var ownerName = Convert.ToString(await ownerCommand.ExecuteScalarAsync());
        if (string.IsNullOrWhiteSpace(ownerName))
            throw new InvalidOperationException(
                $"Database {databaseName} has an orphaned owner and no valid server owner login could be resolved.");

        var quotedOwner = $"[{ownerName.Replace("]", "]]", StringComparison.Ordinal)}]";
        try
        {
            await using var repair = new SqlCommand(
                $"ALTER AUTHORIZATION ON DATABASE::{quotedDatabase} TO {quotedOwner};",
                connection);
            await repair.ExecuteNonQueryAsync();
        }
        catch (SqlException ex)
        {
            throw new InvalidOperationException(
                $"Database {databaseName} has an orphaned owner, which prevents its dbo procedures from running. " +
                "Connect with a SQL Server administrator and assign a valid database owner, then try again.",
                ex);
        }

        return true;
    }

    private static string QuoteDb(string dbName)
    {
        if (!Regex.IsMatch(dbName, @"^[A-Za-z0-9_]+$"))
            throw new InvalidOperationException($"Unsafe database name in [dbo].[System_Settings]: {dbName}");

        return $"[{dbName}]";
    }

    private static async Task<string?> ResolveProxyServicesTableAsync(SqlConnection connection)
    {
        const string tableName = "[dbo].[System_ProxyServices]";
        return await AnyObjectExistsAsync(connection, tableName) ? tableName : null;
    }

    private static string GetProxyServiceRole(int serverType)
    {
        return serverType switch
        {
            1 => "Download",
            2 => "Gateway",
            3 => "Agent",
            _ => "Unknown"
        };
    }

    private static async Task<IReadOnlyList<ManagedColumnInfo>> GetManagedColumnsAsync(SqlConnection connection, string tableName)
    {
        if (!await AnyObjectExistsAsync(connection, tableName))
            return Array.Empty<ManagedColumnInfo>();

        var parts = SplitObjectName(tableName);

        var sysPrefix = parts.Length == 3 ? $"{QuoteName(parts[0])}." : string.Empty;
        var sql = $@"
SELECT
    c.name AS ColumnName,
    t.name AS DataType,
    c.is_nullable,
    c.is_identity,
    c.is_computed,
    c.max_length,
    CASE WHEN pk.column_id IS NULL THEN 0 ELSE 1 END AS IsPrimaryKey
FROM {sysPrefix}sys.columns c
JOIN {sysPrefix}sys.types t ON c.user_type_id = t.user_type_id
OUTER APPLY
(
    SELECT ic.column_id
    FROM {sysPrefix}sys.indexes i
    JOIN {sysPrefix}sys.index_columns ic ON i.object_id = ic.object_id AND i.index_id = ic.index_id
    WHERE i.object_id = c.object_id
      AND i.is_primary_key = 1
      AND ic.column_id = c.column_id
) pk
WHERE c.object_id = OBJECT_ID(@TableName)
ORDER BY c.column_id;";

        await using var command = new SqlCommand(sql, connection);
        command.Parameters.Add("@TableName", SqlDbType.NVarChar, 256).Value = tableName;

        var columns = new List<ManagedColumnInfo>();
        await using var reader = await command.ExecuteReaderAsync();
        while (await reader.ReadAsync())
        {
            columns.Add(new ManagedColumnInfo
            {
                Name = reader.GetString(0),
                DataType = reader.GetString(1),
                IsNullable = reader.GetBoolean(2),
                IsIdentity = reader.GetBoolean(3),
                IsComputed = reader.GetBoolean(4),
                MaxLength = reader.GetInt16(5),
                IsPrimaryKey = Convert.ToInt32(reader.GetValue(6)) == 1
            });
        }

        return columns;
    }

    private static object ConvertManagedValue(ManagedColumnInfo column, string? value)
    {
        if (string.IsNullOrWhiteSpace(value))
            return column.IsNullable || column.IsIdentity ? DBNull.Value : string.Empty;

        var text = value.Trim();
        return column.DataType.ToLowerInvariant() switch
        {
            "bit" => ParseFlexibleBool(text),
            "tinyint" => byte.Parse(text),
            "smallint" => short.Parse(text),
            "int" => int.Parse(text),
            "bigint" => long.Parse(text),
            "decimal" or "numeric" or "money" or "smallmoney" => decimal.Parse(text),
            "float" => double.Parse(text),
            "real" => float.Parse(text),
            "date" => DateTime.Parse(text).Date,
            "datetime" or "datetime2" or "smalldatetime" => DateTime.Parse(text),
            "time" => TimeSpan.Parse(text),
            "uniqueidentifier" => Guid.Parse(text),
            _ => text
        };
    }

    private static bool ParseFlexibleBool(string value)
    {
        if (bool.TryParse(value, out var boolean))
            return boolean;

        if (value is "1" or "yes" or "YES" or "on" or "ON")
            return true;

        if (value is "0" or "no" or "NO" or "off" or "OFF")
            return false;

        throw new InvalidOperationException($"'{value}' is not a valid bit value.");
    }

    private static bool IsTextType(string dataType)
    {
        return dataType.Equals("nvarchar", StringComparison.OrdinalIgnoreCase) ||
               dataType.Equals("varchar", StringComparison.OrdinalIgnoreCase) ||
               dataType.Equals("nchar", StringComparison.OrdinalIgnoreCase) ||
               dataType.Equals("char", StringComparison.OrdinalIgnoreCase) ||
               dataType.Equals("text", StringComparison.OrdinalIgnoreCase) ||
               dataType.Equals("ntext", StringComparison.OrdinalIgnoreCase);
    }

    private static string QuoteObjectName(string objectName)
    {
        var parts = SplitObjectName(objectName);

        return string.Join(".", parts.Select(QuoteName));
    }

    private static string[] SplitObjectName(string objectName)
    {
        var parts = objectName
            .Split('.', StringSplitOptions.RemoveEmptyEntries | StringSplitOptions.TrimEntries)
            .Select(UnquoteName)
            .ToArray();

        if (parts.Length is < 1 or > 3 || parts.Any(part => !Regex.IsMatch(part, @"^[A-Za-z0-9_]+$")))
            throw new InvalidOperationException($"Unsafe table name: {objectName}");

        return parts;
    }

    private static string UnquoteName(string name)
    {
        return name.Length >= 2 && name[0] == '[' && name[^1] == ']'
            ? name[1..^1].Replace("]]", "]")
            : name;
    }

    private static string QuoteName(string name)
    {
        if (!Regex.IsMatch(name, @"^[A-Za-z0-9_]+$"))
            throw new InvalidOperationException($"Unsafe identifier: {name}");

        return $"[{name}]";
    }

    private static ManagedTableInfo ResolveManagedTable(string key)
    {
        return ManagedTables.FirstOrDefault(table => table.Key.Equals(key, StringComparison.OrdinalIgnoreCase))
            ?? throw new InvalidOperationException("Unknown managed table.");
    }

    private static async Task<bool> AnyObjectExistsAsync(SqlConnection connection, string name)
    {
        await using var command = new SqlCommand("SELECT CASE WHEN OBJECT_ID(@Name) IS NULL THEN 0 ELSE 1 END", connection);
        command.Parameters.Add("@Name", SqlDbType.NVarChar, 256).Value = name;
        return Convert.ToInt32(await command.ExecuteScalarAsync()) == 1;
    }

    private static async Task<int> CountRowsIfExistsAsync(SqlConnection connection, string tableName)
    {
        if (!await ObjectExistsAsync(connection, tableName, "U"))
            return 0;

        await using var command = new SqlCommand($"SELECT COUNT(*) FROM {tableName}", connection);
        return Convert.ToInt32(await command.ExecuteScalarAsync());
    }

    private static async Task<int> CountRowsWhereIfExistsAsync(
        SqlConnection connection,
        string tableName,
        string trustedPredicate)
    {
        if (!await AnyObjectExistsAsync(connection, tableName))
            return 0;

        await using var command = new SqlCommand(
            $"SELECT COUNT(*) FROM {QuoteObjectName(tableName)} WITH (NOLOCK) WHERE {trustedPredicate};",
            connection);
        return Convert.ToInt32(await command.ExecuteScalarAsync());
    }

    private static async Task<bool> ColumnExistsAsync(
        SqlConnection connection,
        string tableName,
        string columnName)
    {
        await using var command = new SqlCommand(
            "SELECT CASE WHEN COL_LENGTH(@TableName, @ColumnName) IS NULL THEN 0 ELSE 1 END;",
            connection);
        command.Parameters.Add("@TableName", SqlDbType.NVarChar, 256).Value = tableName;
        command.Parameters.Add("@ColumnName", SqlDbType.NVarChar, 128).Value = columnName;
        return Convert.ToInt32(await command.ExecuteScalarAsync()) == 1;
    }

    private static async Task<DataTable> LoadUpcomingDashboardEventsAsync(SqlConnection connection)
    {
        var result = DashboardOperationsOverview.CreateUpcomingEventsTable();
        var fragments = new List<string>();
        const string daysExpression = @"
STUFF(
    CASE WHEN (s.DaysMask & 1) = 1 THEN N', Sun' ELSE N'' END +
    CASE WHEN (s.DaysMask & 2) = 2 THEN N', Mon' ELSE N'' END +
    CASE WHEN (s.DaysMask & 4) = 4 THEN N', Tue' ELSE N'' END +
    CASE WHEN (s.DaysMask & 8) = 8 THEN N', Wed' ELSE N'' END +
    CASE WHEN (s.DaysMask & 16) = 16 THEN N', Thu' ELSE N'' END +
    CASE WHEN (s.DaysMask & 32) = 32 THEN N', Fri' ELSE N'' END +
    CASE WHEN (s.DaysMask & 64) = 64 THEN N', Sat' ELSE N'' END,
    1, 2, N'')";

        if (await AnyObjectExistsAsync(connection, EventObjectName("_AutoEventSchedule")) &&
            await AnyObjectExistsAsync(connection, EventObjectName("_AutoEventConfig")))
        {
            fragments.Add($@"
SELECT ISNULL(NULLIF(c.DisplayName, N''), s.EventCode) AS Event,
       CONVERT(varchar(5), s.StartTime, 108) AS [Time],
       {daysExpression} AS Days,
       CASE WHEN s.IsActive = 1 AND c.Enabled = 1 THEN N'Ready' ELSE N'Paused' END AS [State]
FROM {EventTable("_AutoEventSchedule")} s WITH (NOLOCK)
LEFT JOIN {EventTable("_AutoEventConfig")} c WITH (NOLOCK) ON c.EventCode = s.EventCode");
        }

        foreach (var schedule in new[]
                 {
                     (Table: "_SurvivalPartySchedule", Name: "Survival Party"),
                     (Table: "_SurvivalSoloSchedule", Name: "Survival Solo"),
                     (Table: "_HideAndSeekSchedule", Name: "Hide and Seek")
                 })
        {
            if (!await AnyObjectExistsAsync(connection, EventObjectName(schedule.Table)))
                continue;

            fragments.Add($@"
SELECT N'{schedule.Name}' AS Event,
       CONVERT(varchar(5), s.StartTime, 108) AS [Time],
       {daysExpression} AS Days,
       CASE WHEN s.IsActive = 1 THEN N'Ready' ELSE N'Paused' END AS [State]
FROM {EventTable(schedule.Table)} s WITH (NOLOCK)");
        }

        if (await AnyObjectExistsAsync(connection, EventObjectName("_CompetitiveEventSchedule")))
        {
            fragments.Add($@"
SELECT CASE s.EventCode
           WHEN N'LMS' THEN N'Last Man Standing'
           WHEN N'MADNESS' THEN N'Madness Solo'
           WHEN N'DTT' THEN N'Defend The Tower'
           ELSE s.EventCode
       END AS Event,
       CONVERT(varchar(5), s.StartTime, 108) AS [Time],
       {daysExpression} AS Days,
       CASE WHEN s.IsActive = 1 THEN N'Ready' ELSE N'Paused' END AS [State]
FROM {EventTable("_CompetitiveEventSchedule")} s WITH (NOLOCK)");
        }

        if (fragments.Count == 0)
            return result;

        return await QueryTableAsync(connection, $@"
SELECT TOP (8) Event, [Time], Days, [State]
FROM
(
    {string.Join($"{Environment.NewLine}UNION ALL{Environment.NewLine}", fragments)}
) schedules
ORDER BY CASE [State] WHEN N'Ready' THEN 0 ELSE 1 END, [Time], Event;");
    }

    private static async Task<DataTable> LoadRecentDashboardActivityAsync(SqlConnection connection)
    {
        if (!await AnyObjectExistsAsync(connection, "[dbo].[Admin_AuditLog]"))
            return DashboardOperationsOverview.CreateRecentActivityTable();

        return await QueryTableAsync(connection, @"
SELECT TOP (8)
       CONVERT(nvarchar(16), CreatedAt, 120) AS [When],
       AdminName AS [Admin],
       Action,
       LEFT(
           CONCAT(
               ISNULL(Target, N''),
               CASE WHEN NULLIF(Details, N'') IS NULL THEN N'' ELSE N' — ' + Details END
           ),
           220
       ) AS Detail
FROM [dbo].[Admin_AuditLog] WITH (NOLOCK)
ORDER BY ID DESC;");
    }

    private static async Task<int> ScalarIntAsync(SqlConnection connection, string sql)
    {
        await using var command = new SqlCommand(sql, connection);
        return Convert.ToInt32(await command.ExecuteScalarAsync());
    }

    private static async Task<string> ScalarStringAsync(SqlConnection connection, string sql)
    {
        await using var command = new SqlCommand(sql, connection);
        return Convert.ToString(await command.ExecuteScalarAsync()) ?? string.Empty;
    }

    private static readonly IReadOnlyList<ManagedTableInfo> ManagedTables = new[]
    {
        new ManagedTableInfo { Key = "FilterRegionControl", Title = "Region Control", Module = "Security", TableName = "[dbo].[Security_RegionFeatures]", AllowInsert = true, AllowUpdate = true, AllowDelete = true, Notes = "Centralized region admission and action rules." },
        new ManagedTableInfo { Key = "BotProtectionLog", Title = "Bot Protection Log", Module = "Security", TableName = "[dbo].[Security_BotProtectionLog]", AllowInsert = false, AllowUpdate = false, AllowDelete = false, Notes = "Read-only bot login, trade and region enforcement decisions." },
        new ManagedTableInfo { Key = "AsyncCommands", Title = "Async Runtime Commands", Module = "Runtime", TableName = "[dbo].[Command_FilterQueue]", AllowInsert = false, AllowUpdate = true, AllowDelete = false, Notes = "Inspect and update queued runtime commands." },
        new ManagedTableInfo { Key = "PlannedCommands", Title = "Planned Runtime Commands", Module = "Runtime", TableName = "[dbo].[Command_PlannedQueue]", AllowInsert = true, AllowUpdate = true, AllowDelete = true, Notes = "Scheduled command queue rows." },
        new ManagedTableInfo { Key = "ItemChest", Title = "Item Chest", Module = "Rewards", TableName = "[dbo].[Item_Chest]", AllowInsert = false, AllowUpdate = true, AllowDelete = true, Notes = "Advanced item chest maintenance." },
        new ManagedTableInfo { Key = "AutoEventConfig", Title = "Auto Event Config", Module = "Auto Events", TableName = "Events.dbo._AutoEventConfig", AllowInsert = true, AllowUpdate = true, AllowDelete = false, Notes = "Runtime event setup used by the Agent service." },
        new ManagedTableInfo { Key = "AutoEventRoundContent", Title = "Auto Event Questions", Module = "Auto Events", TableName = "Events.dbo._AutoEventRoundContent", AllowInsert = true, AllowUpdate = true, AllowDelete = true, Notes = "Round text, answers, and prompts." },
        new ManagedTableInfo { Key = "AutoEventReward", Title = "Auto Event Rewards", Module = "Auto Events", TableName = "Events.dbo._AutoEventReward", AllowInsert = true, AllowUpdate = true, AllowDelete = true, Notes = "Configured rewards per event." },
        new ManagedTableInfo { Key = "AutoEventRound", Title = "Auto Event Rounds", Module = "Auto Events", TableName = "Events.dbo._AutoEventRound", AllowInsert = false, AllowUpdate = true, AllowDelete = false, Notes = "Runtime round state/history." },
        new ManagedTableInfo { Key = "AutoEventCommandQueue", Title = "Auto Event Command Queue", Module = "Auto Events", TableName = "Events.dbo._AutoEventCommandQueue", AllowInsert = true, AllowUpdate = true, AllowDelete = true, Notes = "Manual queue inspection and correction." },
        new ManagedTableInfo { Key = "AutoEventWinnerLog", Title = "Auto Event Winner Logs", Module = "Auto Events", TableName = "Events.dbo._AutoEventWinnerLog", AllowInsert = false, AllowUpdate = false, AllowDelete = false, Notes = "Read-only winner history." },
        new ManagedTableInfo { Key = "KillerAnimations", Title = "Killer Animations", Module = "Killer Animations", TableName = "[dbo].[KillerAnimation_List]", AllowInsert = true, AllowUpdate = true, AllowDelete = false, Notes = "Client killer-animation shop rows. Use Service to hide instead of deleting rows referenced by logs." },
        new ManagedTableInfo { Key = "KillerAnimationOwned", Title = "Owned Killer Animations", Module = "Killer Animations", TableName = "[dbo].[KillerAnimation_Owned]", AllowInsert = true, AllowUpdate = true, AllowDelete = true, Notes = "Purchased animation ownership per character." },
        new ManagedTableInfo { Key = "KillerAnimationActive", Title = "Active Killer Animations", Module = "Killer Animations", TableName = "[dbo].[KillerAnimation_Active]", AllowInsert = true, AllowUpdate = true, AllowDelete = true, Notes = "Selected active killer animation per character." },
        new ManagedTableInfo { Key = "PvpKillLog", Title = "PvP Kill Log", Module = "PvP Challenge", TableName = "[dbo].[PVP_KillLog]", AllowInsert = false, AllowUpdate = false, AllowDelete = false, Notes = "Read-only PvP kill history." },
        new ManagedTableInfo { Key = "TeleportFreeze", Title = "Teleport Freeze Queue", Module = "PvP Challenge", TableName = "[dbo].[Teleport_FreezeQueue]", AllowInsert = true, AllowUpdate = true, AllowDelete = true, Notes = "Teleport freeze command rows." },
        new ManagedTableInfo { Key = "ClientlessAccounts", Title = "Clientless Accounts", Module = "Gateway", TableName = "[dbo].[Clientless_Accounts]", AllowInsert = true, AllowUpdate = true, AllowDelete = true, Notes = "Clientless login account profiles." },
        new ManagedTableInfo { Key = "QuickLoginTokens", Title = "Quick Login Tokens", Module = "Gateway", TableName = "[dbo].[QuickLogin_Tokens]", AllowInsert = false, AllowUpdate = true, AllowDelete = true, Notes = "Secure quick-login tokens." },
        new ManagedTableInfo { Key = "QuickLoginLogs", Title = "Quick Login Logs", Module = "Gateway", TableName = "[dbo].[QuickLogin_Log]", AllowInsert = false, AllowUpdate = false, AllowDelete = false, Notes = "Read-only quick-login audit trail." },
        new ManagedTableInfo { Key = "QuickLoginSecret", Title = "Quick Login Server Secret", Module = "Gateway", TableName = "[dbo].[QuickLogin_ServerSecret]", AllowInsert = false, AllowUpdate = true, AllowDelete = false, Notes = "Server secret state." },
        new ManagedTableInfo { Key = "SettingsBackup", Title = "Settings Backup", Module = "Settings", TableName = "[dbo].[System_SettingsBackup]", AllowInsert = false, AllowUpdate = false, AllowDelete = false, Notes = "Read-only settings migration backup." },
        new ManagedTableInfo { Key = "SecondaryPassword", Title = "Secondary Passwords", Module = "Security", TableName = "[dbo].[Auth_SecondaryPasswords]", AllowInsert = false, AllowUpdate = true, AllowDelete = false, Notes = "Account secondary password maintenance." }
    };
}
