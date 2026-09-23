using System.Collections.Concurrent;
using System.Security.Cryptography;
using System.Text;
using Dapper;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.PacketHandlerManager;
using KMTGuard.Server;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Server.GatewayPacketHandler;

public partial class SERVER_DLL_SETTINGS_RESPONSE
{
    private const ushort CLIENT_CREATE_QUICK_LOGIN_TOKEN_OPCODE = 0x1670;
    private const ushort SERVER_CREATE_QUICK_LOGIN_TOKEN_RESULT_OPCODE = 0x1671;
    private const ushort CLIENT_QUICK_LOGIN_OPCODE = 0x1672;
    private const ushort SERVER_QUICK_LOGIN_RESULT_OPCODE = 0x1673;
    private const ushort CLIENT_REVOKE_QUICK_LOGIN_TOKEN_OPCODE = 0x1674;
    private const ushort SERVER_REVOKE_QUICK_LOGIN_TOKEN_RESULT_OPCODE = 0x1675;

    private const int QuickLoginTokenBytes = 64;
    private const int QuickLoginExpiryDays = 90;
    private const int MaxQuickLoginDevicesPerAccount = 5;
    private const int MaxQuickLoginFailedTries = 5;
    private static readonly TimeSpan QuickLoginTimestampSkew = TimeSpan.FromMinutes(2);
    private static readonly ConcurrentDictionary<string, RateWindow> QuickLoginRateLimits = new();
    private static bool _quickLoginSchemaReady;
    private static long _lastQuickLoginRateCleanupTicks;

    private sealed class RateWindow
    {
        public int Count;
        public DateTime StartedUtc = DateTime.UtcNow;
    }

    private sealed class QuickLoginRecord
    {
        public int ID { get; set; }
        public int AccountID { get; set; }
        public DateTime? ExpireDate { get; set; }
        public int FailedTryCount { get; set; }
        public byte[] DeviceIDHash { get; set; } = Array.Empty<byte>();
        public string LastIP { get; set; } = string.Empty;
        public byte[]? PasswordCipher { get; set; }
        public byte[]? PasswordIV { get; set; }
        public byte[]? PasswordNonce { get; set; }
        public byte[]? PasswordTag { get; set; }
        public byte EncryptionVersion { get; set; }
        public DateTime? LockedUntilUtc { get; set; }
        public byte Locale { get; set; }
        public int ServerID { get; set; }
    }

    private async Task<PacketResult> CLIENT_CREATE_QUICK_LOGIN_TOKEN(Packet packet, ISession session, object obj)
    {
        if (!CanAcceptQuickLoginAction(session))
            return new PacketResult(PacketResultType.Block);

        try
        {
            var username = packet.ReadAscii().Trim().ToLowerInvariant();
            var password = packet.ReadAscii();
            var deviceId = packet.ReadAscii();
            var clientVersion = packet.ReadAscii();
            var timestamp = packet.ReadInt32();
            var requestNonce = packet.ReadAscii();
            var requestedCharacterName = packet.RemainingRead() > 0 ? packet.ReadAscii().Trim() : string.Empty;

            if (!ValidateQuickLoginTimestamp(timestamp) ||
                !TryConsumeQuickLoginNonce(session, requestNonce))
            {
                await SendCreateQuickLoginTokenResult(session, false, username, string.Empty, PlayerLanguage.Get("QuickLogin.RequestExpired"));
                return new PacketResult(PacketResultType.Block);
            }

            if (!CheckQuickLoginRateLimit($"create:{session.ClientIp}", 5, TimeSpan.FromMinutes(1)))
            {
                await LogQuickLoginAsync(null, username, null, session.ClientIp, "CREATE_TOKEN", "TOO_MANY_ATTEMPTS");
                await SendCreateQuickLoginTokenResult(session, false, username, string.Empty, PlayerLanguage.Get("QuickLogin.TooManyAttempts"));
                return new PacketResult(PacketResultType.Block);
            }

            if (string.IsNullOrWhiteSpace(username) || string.IsNullOrWhiteSpace(password) ||
                !HasVerifiedQuickLoginDevice(session, deviceId))
            {
                await SendCreateQuickLoginTokenResult(session, false, username, string.Empty, PlayerLanguage.Get("QuickLogin.InvalidRequest"));
                return new PacketResult(PacketResultType.Block);
            }

            var credentialStatus = await sqlQueryHelper.ValidateUserCredentialsStatusAsync(username, password);
            if (credentialStatus != CredentialValidationStatus.Valid)
            {
                var result = credentialStatus == CredentialValidationStatus.Invalid
                    ? "INVALID_CREDENTIALS"
                    : "AUTH_UNAVAILABLE";
                await LogQuickLoginAsync(null, username, null, session.ClientIp, "CREATE_TOKEN", result);
                await SendCreateQuickLoginTokenResult(session, false, username, string.Empty,
                    PlayerLanguage.Get(credentialStatus == CredentialValidationStatus.Invalid
                        ? "QuickLogin.InvalidCredentials"
                        : "Security.AuthenticationUnavailable"));
                return new PacketResult(PacketResultType.Block);
            }

            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await EnsureQuickLoginSchemaAsync(connection);

            var accountId = await GetAccountIdAsync(connection, username);
            if (accountId <= 0)
            {
                await LogQuickLoginAsync(null, username, null, session.ClientIp, "CREATE_TOKEN", "ACCOUNT_NOT_FOUND");
                await SendCreateQuickLoginTokenResult(session, false, username, string.Empty, PlayerLanguage.Get("QuickLogin.AccountNotFound"));
                return new PacketResult(PacketResultType.Block);
            }

            if (!string.IsNullOrWhiteSpace(requestedCharacterName) &&
                !await QuickLoginCharacterBelongsToAccountAsync(connection, accountId, requestedCharacterName))
            {
                await LogQuickLoginAsync(accountId, username, null, session.ClientIp, "CREATE_TOKEN", "INVALID_CHARACTER");
                await SendCreateQuickLoginTokenResult(session, false, username, string.Empty, PlayerLanguage.Get("QuickLogin.CharacterNotFound"));
                return new PacketResult(PacketResultType.Block);
            }

            var serverSecret = GetQuickLoginServerSecret();
            var tokenBytes = RandomNumberGenerator.GetBytes(QuickLoginTokenBytes);
            var token = Base64UrlEncode(tokenBytes);
            var tokenHash = HmacSha256(serverSecret, Encoding.UTF8.GetBytes(token));
            var deviceHash = HmacSha256(serverSecret, Encoding.UTF8.GetBytes(deviceId));
            var encryptedPassword = EncryptQuickLoginPassword(serverSecret, password);
            var expireDate = DateTime.UtcNow.AddDays(QuickLoginExpiryDays);
            var serverId = await ResolveQuickLoginServerIdAsync(connection, session, session.SessionData.ServerID);

            await connection.ExecuteAsync(
                @"INSERT INTO [dbo].[QuickLogin_Tokens]
                    (AccountID, Username, TokenHash, DeviceIDHash, DeviceName, ExpireDate, LastIP,
                     PasswordCipher, PasswordIV, PasswordNonce, PasswordTag, EncryptionVersion, Locale, ServerID)
                  VALUES
                    (@AccountID, @Username, @TokenHash, @DeviceIDHash, @DeviceName, @ExpireDate, @LastIP,
                     @PasswordCipher, NULL, @PasswordNonce, @PasswordTag, 2, @Locale, @ServerID)",
                new
                {
                    AccountID = accountId,
                    Username = username,
                    TokenHash = tokenHash,
                    DeviceIDHash = deviceHash,
                    DeviceName = TrimForDb(clientVersion, 128),
                    ExpireDate = expireDate,
                    LastIP = TrimForDb(session.ClientIp, 64),
                    PasswordCipher = encryptedPassword.Cipher,
                    PasswordNonce = encryptedPassword.Nonce,
                    PasswordTag = encryptedPassword.Tag,
                    Locale = session.SessionData.locale == 0 ? (byte)22 : session.SessionData.locale,
                    ServerID = serverId
                });

            await connection.ExecuteAsync(
                @"WITH OldTokens AS
                  (
                      SELECT ID,
                             ROW_NUMBER() OVER (PARTITION BY AccountID ORDER BY CreatedDate DESC, ID DESC) AS rn
                      FROM [dbo].[QuickLogin_Tokens]
                      WHERE AccountID = @AccountID AND IsActive = 1
                  )
                  UPDATE [dbo].[QuickLogin_Tokens]
                     SET IsActive = 0, RevokedDate = GETDATE()
                   WHERE ID IN (SELECT ID FROM OldTokens WHERE rn > @MaxDevices);",
                new { AccountID = accountId, MaxDevices = MaxQuickLoginDevicesPerAccount });

            await LogQuickLoginAsync(accountId, username, deviceHash, session.ClientIp, "CREATE_TOKEN", "SUCCESS");
            await SendCreateQuickLoginTokenResult(session, true, username, token, PlayerLanguage.Get("QuickLogin.AccountSaved"));
            return new PacketResult(PacketResultType.Block);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "CLIENT_CREATE_QUICK_LOGIN_TOKEN failed for {ClientIp}", session.ClientIp);
            await SendCreateQuickLoginTokenResult(session, false, string.Empty, string.Empty, PlayerLanguage.Get("QuickLogin.SaveFailed"));
            return new PacketResult(PacketResultType.Block);
        }
    }

    private async Task<PacketResult> CLIENT_QUICK_LOGIN(Packet packet, ISession session, object obj)
    {
        if (!CanAcceptQuickLoginAction(session))
            return new PacketResult(PacketResultType.Block);

        try
        {
            var username = packet.ReadAscii().Trim().ToLowerInvariant();
            var token = packet.ReadAscii();
            var deviceId = packet.ReadAscii();
            var timestamp = packet.ReadInt32();
            var requestNonce = packet.ReadAscii();

            if (!ValidateQuickLoginTimestamp(timestamp) ||
                !TryConsumeQuickLoginNonce(session, requestNonce) ||
                !HasVerifiedQuickLoginDevice(session, deviceId))
            {
                await SendQuickLoginResult(session, false, PlayerLanguage.Get("QuickLogin.RequestExpired"));
                return new PacketResult(PacketResultType.Block);
            }

            if (!CheckQuickLoginRateLimit($"play:{session.ClientIp}", 10, TimeSpan.FromMinutes(1)))
            {
                await LogQuickLoginAsync(null, username, null, session.ClientIp, "QUICK_LOGIN", "TOO_MANY_ATTEMPTS");
                await SendQuickLoginResult(session, false, PlayerLanguage.Get("QuickLogin.TooManyAttempts"));
                return new PacketResult(PacketResultType.Block);
            }

            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await EnsureQuickLoginSchemaAsync(connection);

            var serverSecret = GetQuickLoginServerSecret();
            var tokenHash = HmacSha256(serverSecret, Encoding.UTF8.GetBytes(token));
            var deviceHash = HmacSha256(serverSecret, Encoding.UTF8.GetBytes(deviceId));

            var record = await connection.QuerySingleOrDefaultAsync<QuickLoginRecord>(
                @"SELECT TOP 1 ID, AccountID, ExpireDate, FailedTryCount, DeviceIDHash, LastIP,
                                    PasswordCipher, PasswordIV, PasswordNonce, PasswordTag,
                                    EncryptionVersion, LockedUntilUtc, Locale, ServerID
                    FROM [dbo].[QuickLogin_Tokens] WITH (UPDLOCK)
                   WHERE Username = @Username
                     AND TokenHash = @TokenHash
                     AND IsActive = 1",
                new { Username = username, TokenHash = tokenHash });

            if (record == null)
            {
                await LogQuickLoginAsync(null, username, deviceHash, session.ClientIp, "QUICK_LOGIN", "INVALID_TOKEN");
                await SendQuickLoginResult(session, false, PlayerLanguage.Get("QuickLogin.InvalidToken"));
                return new PacketResult(PacketResultType.Block);
            }

            if (record.LockedUntilUtc > DateTime.UtcNow)
            {
                await LogQuickLoginAsync(record.AccountID, username, deviceHash, session.ClientIp, "QUICK_LOGIN", "TOO_MANY_ATTEMPTS");
                await SendQuickLoginResult(session, false, PlayerLanguage.Get("QuickLogin.TemporarilyLocked"));
                return new PacketResult(PacketResultType.Block);
            }

            if (record.ExpireDate.HasValue && record.ExpireDate.Value < DateTime.UtcNow)
            {
                await connection.ExecuteAsync(
                    "UPDATE [dbo].[QuickLogin_Tokens] SET IsActive = 0, RevokedDate = GETDATE() WHERE ID = @ID",
                    new { record.ID });
                await LogQuickLoginAsync(record.AccountID, username, deviceHash, session.ClientIp, "QUICK_LOGIN", "EXPIRED_TOKEN");
                await SendQuickLoginResult(session, false, PlayerLanguage.Get("QuickLogin.TokenExpired"));
                return new PacketResult(PacketResultType.Block);
            }

            if (!CryptographicOperations.FixedTimeEquals(record.DeviceIDHash, deviceHash))
            {
                await connection.ExecuteAsync(
                    @"UPDATE [dbo].[QuickLogin_Tokens]
                         SET FailedTryCount = FailedTryCount + 1,
                             LockedUntilUtc = CASE WHEN FailedTryCount + 1 >= @MaxFailed
                                                   THEN DATEADD(MINUTE, 15, SYSUTCDATETIME())
                                                   ELSE LockedUntilUtc END
                       WHERE ID = @ID",
                    new { record.ID, MaxFailed = MaxQuickLoginFailedTries });
                await LogQuickLoginAsync(record.AccountID, username, deviceHash, session.ClientIp,
                    "QUICK_LOGIN", "DEVICE_MISMATCH");
                await SendQuickLoginResult(session, false, PlayerLanguage.Get("QuickLogin.DifferentDevice"));
                return new PacketResult(PacketResultType.Block);
            }

            await connection.ExecuteAsync(
                @"UPDATE [dbo].[QuickLogin_Tokens]
                     SET LastUsedDate = GETDATE(), FailedTryCount = 0, LockedUntilUtc = NULL, LastIP = @LastIP
                   WHERE ID = @ID",
                new { record.ID, LastIP = TrimForDb(session.ClientIp, 64) });

            await LogQuickLoginAsync(record.AccountID, username, deviceHash, session.ClientIp, "QUICK_LOGIN", "SUCCESS");

            if (record.EncryptionVersion != 2 || record.PasswordCipher is not { Length: > 0 } ||
                record.PasswordNonce is not { Length: 12 } || record.PasswordTag is not { Length: 16 })
            {
                await SendQuickLoginResult(session, false, PlayerLanguage.Get("QuickLogin.ResaveRequired"));
                return new PacketResult(PacketResultType.Block);
            }

            var password = DecryptQuickLoginPassword(
                serverSecret, record.PasswordCipher, record.PasswordNonce, record.PasswordTag);
            var serverId = await ResolveQuickLoginServerIdAsync(connection, session, record.ServerID);
            var locale = record.Locale == 0 ? (byte)22 : record.Locale;

            session.SessionData.locale = locale;
            session.SessionData.user_id = username;
            session.SessionData.GatewayCredential.Replace(password);
            session.SessionData.ServerID = (ushort)serverId;
            session.PlayerUserID = username;

            if (!await sqlQueryHelper.BindAccountDeviceAsync(
                    username, session.SessionData.Hwid, session.DeviceKeyThumbprint,
                    session.DevicePublicKey))
            {
                session.SessionData.GatewayCredential.Clear();
                await SendQuickLoginResult(session, false, PlayerLanguage.Get("QuickLogin.DifferentDevice"));
                return new PacketResult(PacketResultType.Block);
            }

            async Task ReplayQuickLoginAsync()
            {
                try
                {
                    QuickLoginAgentAuthBridge.BeginGatewayLogin(session, username, password, locale);
                    var loginPacket = new Packet(0x6102, true, false);
                    loginPacket.WriteUInt8(locale);
                    loginPacket.WriteAscii(username);
                    loginPacket.WriteAscii(password);
                    loginPacket.WriteUInt16((ushort)serverId);
                    await session.SendToServer(loginPacket);
                    await SendQuickLoginResult(session, true, PlayerLanguage.Get("QuickLogin.Accepted"));
                }
                finally
                {
                    session.SessionData.GatewayCredential.Clear();
                }
            }

            if (_serverSettings.SecondaryPassword)
            {
                var secondaryPassword = await sqlQueryHelper.GetSecondaryPasswordData(username);
                if (secondaryPassword == null)
                {
                    session.PendingQuickLogin = true;
                    session.GatewayAuthenticationState = GatewayAuthenticationState.AwaitingSecondaryCreate;
                    var create = new Packet(0x1212);
                    create.WriteUInt8(SecondaryPasswordResponse.CREATE_THE_PASSWORD);
                    await session.SendToClient(create);
                    return new PacketResult(PacketResultType.Block);
                }

                if (!secondaryPassword.RememberPC ||
                    !string.Equals(secondaryPassword.DeviceKeyThumbprint,
                        session.DeviceKeyThumbprint, StringComparison.OrdinalIgnoreCase))
                {
                    session.PendingQuickLogin = true;
                    session.GatewayAuthenticationState = GatewayAuthenticationState.AwaitingSecondaryEntry;
                    var enter = new Packet(0x1212);
                    enter.WriteUInt8(SecondaryPasswordResponse.ENTER_THE_PASSWORD);
                    await session.SendToClient(enter);
                    return new PacketResult(PacketResultType.Block);
                }
            }

            session.GatewayAuthenticationState = GatewayAuthenticationState.Released;

            if (await OfflineStallGatewayBridge.TryReplaceOfflineStallLoginAsync(
                    session, username, password, ReplayQuickLoginAsync, credentialsAlreadyValidated: true))
                return new PacketResult(PacketResultType.Block);

            await ReplayQuickLoginAsync();
            return new PacketResult(PacketResultType.Block);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "CLIENT_QUICK_LOGIN failed for {ClientIp}", session.ClientIp);
            await SendQuickLoginResult(session, false, PlayerLanguage.Get("QuickLogin.Failed"));
            return new PacketResult(PacketResultType.Block);
        }
    }

    private async Task<PacketResult> CLIENT_REVOKE_QUICK_LOGIN_TOKEN(Packet packet, ISession session, object obj)
    {
        if (!CanAcceptQuickLoginAction(session))
            return new PacketResult(PacketResultType.Block);

        try
        {
            var username = packet.ReadAscii().Trim().ToLowerInvariant();
            var token = packet.ReadAscii();
            var deviceId = packet.ReadAscii();
            var timestamp = packet.ReadInt32();
            var requestNonce = packet.ReadAscii();

            if (!ValidateQuickLoginTimestamp(timestamp) ||
                !TryConsumeQuickLoginNonce(session, requestNonce) ||
                !HasVerifiedQuickLoginDevice(session, deviceId))
            {
                await SendRevokeQuickLoginTokenResult(
                    session, false, PlayerLanguage.Get("QuickLogin.InvalidRequest"));
                return new PacketResult(PacketResultType.Block);
            }

            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await EnsureQuickLoginSchemaAsync(connection);

            var serverSecret = GetQuickLoginServerSecret();
            var tokenHash = HmacSha256(serverSecret, Encoding.UTF8.GetBytes(token));
            var deviceHash = HmacSha256(serverSecret, Encoding.UTF8.GetBytes(deviceId));

            var accountId = await connection.ExecuteScalarAsync<int?>(
                @"UPDATE [dbo].[QuickLogin_Tokens]
                     SET IsActive = 0, RevokedDate = GETDATE()
                   OUTPUT INSERTED.AccountID
                   WHERE Username = @Username
                     AND TokenHash = @TokenHash
                     AND DeviceIDHash = @DeviceIDHash
                     AND IsActive = 1",
                new { Username = username, TokenHash = tokenHash, DeviceIDHash = deviceHash });

            await LogQuickLoginAsync(accountId, username, deviceHash, session.ClientIp, "REVOKE_TOKEN", "SUCCESS");
            await SendRevokeQuickLoginTokenResult(session, true, PlayerLanguage.Get("QuickLogin.AccountRemoved"));
            return new PacketResult(PacketResultType.Block);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "CLIENT_REVOKE_QUICK_LOGIN_TOKEN failed for {ClientIp}", session.ClientIp);
            await SendRevokeQuickLoginTokenResult(session, false, PlayerLanguage.Get("QuickLogin.RemoveFailed"));
            return new PacketResult(PacketResultType.Block);
        }
    }

    private static async Task EnsureQuickLoginSchemaAsync(SqlConnection connection)
    {
        if (_quickLoginSchemaReady)
            return;

        var schemaReady = await connection.ExecuteScalarAsync<int>(@"
SELECT CASE WHEN OBJECT_ID(N'dbo.QuickLogin_Tokens', N'U') IS NOT NULL
                  AND OBJECT_ID(N'dbo.QuickLogin_Log', N'U') IS NOT NULL
                  AND COL_LENGTH(N'dbo.QuickLogin_Tokens', N'PasswordNonce') IS NOT NULL
                  AND COL_LENGTH(N'dbo.QuickLogin_Tokens', N'PasswordTag') IS NOT NULL
                  AND COL_LENGTH(N'dbo.QuickLogin_Tokens', N'EncryptionVersion') IS NOT NULL
                  AND COL_LENGTH(N'dbo.QuickLogin_Tokens', N'LockedUntilUtc') IS NOT NULL
             THEN 1 ELSE 0 END;");
        if (schemaReady != 1)
            throw new InvalidOperationException(
                "Quick Login schema v3 is missing. Apply the packaged database migration before starting the Filter.");
        _quickLoginSchemaReady = true;
    }

    private static byte[] GetQuickLoginServerSecret()
    {
        if (Program.QuickLoginMasterKey.Length != 32)
            throw new InvalidOperationException("Quick Login master key is unavailable.");
        return Program.QuickLoginMasterKey;
    }

    private async Task<int> GetAccountIdAsync(SqlConnection connection, string username)
    {
        var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
        return await connection.ExecuteScalarAsync<int>(
            $@"SELECT TOP 1 JID
                 FROM {accountDb}..TB_User WITH (NOLOCK)
                WHERE StrUserID = @Username",
            new { Username = username });
    }

    private async Task<bool> QuickLoginCharacterBelongsToAccountAsync(SqlConnection connection, int accountId, string characterName)
    {
        var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
        var count = await connection.ExecuteScalarAsync<int>(
            $@"SELECT COUNT(1)
                 FROM {shardDb}.._User u WITH (NOLOCK)
                 JOIN {shardDb}.._Char c WITH (NOLOCK) ON c.CharID = u.CharID
                WHERE u.UserJID = @AccountID
                  AND c.CharName16 = @CharacterName",
            new
            {
                AccountID = accountId,
                CharacterName = TrimForDb(characterName, 64)
            });

        return count > 0;
    }

    private async Task<int> ResolveQuickLoginServerIdAsync(SqlConnection connection, ISession session, int storedServerId)
    {
        if (session.SessionData.ServerID > 0)
            return session.SessionData.ServerID;

        if (storedServerId > 1)
            return storedServerId;

        try
        {
            var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
            var shardIdColumn = await connection.ExecuteScalarAsync<string?>(
                $@"SELECT TOP 1 c.name
                     FROM {accountDb}.sys.columns c
                     JOIN {accountDb}.sys.tables t ON c.object_id = t.object_id
                    WHERE t.name = '_Shard'
                      AND c.name IN ('ID', 'nID', 'ShardID')
                    ORDER BY CASE c.name
                        WHEN 'ID' THEN 1
                        WHEN 'nID' THEN 2
                        WHEN 'ShardID' THEN 3
                        ELSE 4
                    END");

            if (!string.IsNullOrWhiteSpace(shardIdColumn))
            {
                var quotedShardIdColumn = SqlIdentifier.Quote(shardIdColumn);
                var configuredShardId = await connection.ExecuteScalarAsync<int?>(
                    $@"SELECT TOP 1 {quotedShardIdColumn}
                         FROM {accountDb}.._Shard WITH (NOLOCK)
                        ORDER BY {quotedShardIdColumn}");
                if (configuredShardId.HasValue && configuredShardId.Value > 0)
                    return configuredShardId.Value;
            }

            var shardId = await connection.ExecuteScalarAsync<int?>(
                $@"SELECT TOP 1 nShardID
                     FROM {accountDb}.._ShardCurrentUser WITH (NOLOCK)
                    WHERE nShardID > 0
                    GROUP BY nShardID
                    ORDER BY MAX(dLogDate) DESC, nShardID");
            if (shardId.HasValue && shardId.Value > 0)
                return shardId.Value;
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Could not resolve quick login shard id; falling back to stored/default ServerID={ServerID}", storedServerId);
        }

        return storedServerId > 0 ? storedServerId : 1;
    }

    private static async Task LogQuickLoginAsync(int? accountId, string? username, byte[]? deviceHash, string ip, string actionType, string result)
    {
        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await EnsureQuickLoginSchemaAsync(connection);
            await connection.ExecuteAsync(
                @"INSERT INTO [dbo].[QuickLogin_Log]
                    (AccountID, Username, DeviceIDHash, IP, ActionType, Result)
                  VALUES
                    (@AccountID, @Username, @DeviceIDHash, @IP, @ActionType, @Result)",
                new
                {
                    AccountID = accountId,
                    Username = TrimForDb(username ?? string.Empty, 64),
                    DeviceIDHash = deviceHash,
                    IP = TrimForDb(ip, 64),
                    ActionType = TrimForDb(actionType, 32),
                    Result = TrimForDb(result, 32)
                });
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Secure quick login audit log failed");
        }
    }

    private static bool CheckQuickLoginRateLimit(string key, int maxCount, TimeSpan window)
    {
        var now = DateTime.UtcNow;
        var nowTicks = Environment.TickCount64;
        var previousCleanup = Interlocked.Read(ref _lastQuickLoginRateCleanupTicks);
        if (nowTicks - previousCleanup >= 60_000 &&
            Interlocked.CompareExchange(
                ref _lastQuickLoginRateCleanupTicks, nowTicks, previousCleanup) == previousCleanup)
        {
            foreach (var entry in QuickLoginRateLimits)
            {
                if (now - entry.Value.StartedUtc > TimeSpan.FromMinutes(10))
                    QuickLoginRateLimits.TryRemove(entry);
            }
        }

        if (QuickLoginRateLimits.Count >= 10_000 && !QuickLoginRateLimits.ContainsKey(key))
            return false;

        var rateWindow = QuickLoginRateLimits.GetOrAdd(key, _ => new RateWindow());
        lock (rateWindow)
        {
            if (now - rateWindow.StartedUtc > window)
            {
                rateWindow.StartedUtc = now;
                rateWindow.Count = 0;
            }

            rateWindow.Count++;
            return rateWindow.Count <= maxCount;
        }
    }

    internal static bool ValidateQuickLoginTimestamp(int unixSeconds)
    {
        var requestTime = DateTimeOffset.FromUnixTimeSeconds(unixSeconds);
        var delta = DateTimeOffset.UtcNow - requestTime;
        return delta.Duration() <= QuickLoginTimestampSkew;
    }

    private static byte[] HmacSha256(byte[] secret, byte[] data)
    {
        using var hmac = new HMACSHA256(secret);
        return hmac.ComputeHash(data);
    }

    internal static (byte[] Cipher, byte[] Nonce, byte[] Tag) EncryptQuickLoginPassword(
        byte[] serverSecret, string password)
    {
        var plain = Encoding.UTF8.GetBytes(password);
        var cipher = new byte[plain.Length];
        var nonce = RandomNumberGenerator.GetBytes(12);
        var tag = new byte[16];
        try
        {
            using var aes = new AesGcm(serverSecret, tag.Length);
            aes.Encrypt(nonce, plain, cipher, tag);
            return (cipher, nonce, tag);
        }
        finally
        {
            CryptographicOperations.ZeroMemory(plain);
        }
    }

    internal static string DecryptQuickLoginPassword(
        byte[] serverSecret, byte[] cipher, byte[] nonce, byte[] tag)
    {
        var plain = new byte[cipher.Length];
        try
        {
            using var aes = new AesGcm(serverSecret, tag.Length);
            aes.Decrypt(nonce, cipher, tag, plain);
            return Encoding.UTF8.GetString(plain);
        }
        finally
        {
            CryptographicOperations.ZeroMemory(plain);
        }
    }

    private static bool HasVerifiedQuickLoginDevice(ISession session, string deviceId)
    {
        return session.GatewayAuthenticationState == GatewayAuthenticationState.AwaitingPrimaryCredentials &&
               !string.IsNullOrWhiteSpace(session.DeviceKeyThumbprint) &&
               !string.IsNullOrWhiteSpace(session.DevicePublicKey) &&
               string.Equals(session.SessionData.Hwid, deviceId, StringComparison.OrdinalIgnoreCase);
    }

    internal static bool CanAcceptQuickLoginAction(ISession session)
    {
        return !session.QuickLoginNonceRefreshPending &&
               session.GatewayAuthenticationState == GatewayAuthenticationState.AwaitingPrimaryCredentials;
    }

    internal static bool TakeQuickLoginNonceRefreshMarker(ISession session)
    {
        var wasPending = session.QuickLoginNonceRefreshPending;
        session.QuickLoginNonceRefreshPending = false;
        return wasPending;
    }

    internal static bool TryPrepareNextQuickLoginNonce(ISession session, out Packet? challengePacket)
    {
        challengePacket = null;
        session.VerifiedHwidNonce = string.Empty;

        if (!HwidSecurity.TryCreateChallenge(
                session, HwidSecurity.GatewayRole, out var challenge))
            return false;

        session.QuickLoginNonceRefreshPending = true;
        challengePacket = new Packet(0x165A);
        challengePacket.WriteAscii(challenge);
        return true;
    }

    private static async Task PrepareNextQuickLoginNonceAsync(ISession session)
    {
        if (TryPrepareNextQuickLoginNonce(session, out var challengePacket) &&
            challengePacket != null)
        {
            // Keep the existing client protocol: the signed HWID challenge refreshes
            // HWIDGenerator's session nonce. Send it before the action result so the
            // proof is queued back to the Gateway before the UI can submit again.
            await session.SendToClient(challengePacket);
        }
    }

    private static bool TryConsumeQuickLoginNonce(ISession session, string suppliedNonce)
    {
        var expectedNonce = session.VerifiedHwidNonce;
        session.VerifiedHwidNonce = string.Empty;
        if (expectedNonce.Length != 48 || suppliedNonce.Length != expectedNonce.Length)
            return false;
        return CryptographicOperations.FixedTimeEquals(
            Encoding.ASCII.GetBytes(expectedNonce), Encoding.ASCII.GetBytes(suppliedNonce));
    }

    private static string Base64UrlEncode(byte[] data)
    {
        return Convert.ToBase64String(data)
            .TrimEnd('=')
            .Replace('+', '-')
            .Replace('/', '_');
    }

    private static string TrimForDb(string value, int maxLength)
    {
        if (string.IsNullOrEmpty(value))
            return string.Empty;
        return value.Length <= maxLength ? value : value[..maxLength];
    }

    private static async Task SendCreateQuickLoginTokenResult(ISession session, bool success, string username, string token, string message)
    {
        await PrepareNextQuickLoginNonceAsync(session);
        var response = new Packet(SERVER_CREATE_QUICK_LOGIN_TOKEN_RESULT_OPCODE);
        response.WriteUInt8((byte)(success ? 1 : 0));
        response.WriteAscii(username ?? string.Empty);
        response.WriteAscii(success ? token : string.Empty);
        response.WriteUnicode(message);
        await session.SendToClient(response);
    }

    private static async Task SendQuickLoginResult(ISession session, bool success, string message)
    {
        if (!success)
            await PrepareNextQuickLoginNonceAsync(session);
        var response = new Packet(SERVER_QUICK_LOGIN_RESULT_OPCODE);
        response.WriteUInt8((byte)(success ? 1 : 0));
        response.WriteUnicode(message);
        await session.SendToClient(response);
    }

    private static async Task SendRevokeQuickLoginTokenResult(ISession session, bool success, string message)
    {
        await PrepareNextQuickLoginNonceAsync(session);
        var response = new Packet(SERVER_REVOKE_QUICK_LOGIN_TOKEN_RESULT_OPCODE);
        response.WriteUInt8((byte)(success ? 1 : 0));
        response.WriteUnicode(message);
        await session.SendToClient(response);
    }
}
