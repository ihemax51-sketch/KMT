using System.Collections.Concurrent;
using KMTGuard.RuntimeContract;
using KMTGuard.SessionManager;
using Serilog;
using System.Security.Cryptography;
using System.Text;

namespace KMTGuard.Server;

public sealed class QuickLoginAgentAuth
{
    public required string Username { get; init; }
    public required byte[] PasswordCipher { get; init; }
    public required byte[] PasswordNonce { get; init; }
    public required byte[] PasswordTag { get; init; }
    public required string ClientIp { get; init; }
    public required string DeviceKeyThumbprint { get; init; }
    public required byte Locale { get; init; }
    public required DateTime ExpiresAtUtc { get; init; }
}

public sealed record QuickLoginAgentCredentials(string Username, string Password, byte Locale);

public readonly record struct QuickLoginBindResult(bool Success, string Username);

public static class QuickLoginAgentAuthBridge
{
    private static readonly TimeSpan AuthTtl = TimeSpan.FromSeconds(45);
    private static readonly ConcurrentDictionary<Guid, QuickLoginAgentAuth> PendingGatewaySessions = new();
    private static readonly ConcurrentDictionary<uint, QuickLoginAgentAuth> PendingAgentTokens = new();
    private static Timer? _cleanupTimer;

    public static void Start()
    {
        var timer = new Timer(
            _ => CleanupExpired(),
            null,
            TimeSpan.FromSeconds(15),
            TimeSpan.FromSeconds(15));
        if (Interlocked.CompareExchange(ref _cleanupTimer, timer, null) != null)
            timer.Dispose();
    }

    public static void Stop()
    {
        Interlocked.Exchange(ref _cleanupTimer, null)?.Dispose();
        foreach (var key in PendingGatewaySessions.Keys)
            CancelGatewayLogin(key);
        foreach (var key in PendingAgentTokens.Keys)
        {
            if (PendingAgentTokens.TryRemove(key, out var auth))
                ZeroEncryptedPassword(auth);
        }
    }

    public static void BeginGatewayLogin(ISession session, string username, string password, byte locale)
    {
        CleanupExpired();

        var encrypted = EncryptPassword(password);
        var auth = new QuickLoginAgentAuth
        {
            Username = username,
            PasswordCipher = encrypted.Cipher,
            PasswordNonce = encrypted.Nonce,
            PasswordTag = encrypted.Tag,
            ClientIp = session.ClientIp,
            DeviceKeyThumbprint = session.DeviceKeyThumbprint,
            Locale = locale,
            ExpiresAtUtc = DateTime.UtcNow.Add(AuthTtl)
        };
        PendingGatewaySessions.AddOrUpdate(
            session.ClientGuid,
            auth,
            (_, previous) =>
            {
                ZeroEncryptedPassword(previous);
                return auth;
            });
    }

    public static void CancelGatewayLogin(Guid gatewaySessionId)
    {
        if (PendingGatewaySessions.TryRemove(gatewaySessionId, out var auth))
            ZeroEncryptedPassword(auth);
    }

    public static async Task<QuickLoginBindResult> BindGatewayTokenAsync(
        Guid gatewaySessionId,
        uint token,
        string clientIp)
    {
        CleanupExpired();

        if (!PendingGatewaySessions.TryRemove(gatewaySessionId, out var auth))
            return new QuickLoginBindResult(false, string.Empty);

        if (IsExpired(auth) || !string.Equals(auth.ClientIp, clientIp, StringComparison.Ordinal))
        {
            ZeroEncryptedPassword(auth);
            return new QuickLoginBindResult(false, string.Empty);
        }

        if (global::Program.CurrentRole == FilterRole.All)
        {
            if (PendingAgentTokens.TryAdd(token, auth))
                return new QuickLoginBindResult(true, auth.Username);

            ZeroEncryptedPassword(auth);
            return new QuickLoginBindResult(false, string.Empty);
        }

        try
        {
            var response = await RuntimePipeClient.SendAsync(
                FilterRole.Agent,
                "agent.quickLogin.publish",
                new QuickLoginAuthPayload
                {
                    Token = token,
                    Username = auth.Username,
                    PasswordCipher = auth.PasswordCipher,
                    PasswordNonce = auth.PasswordNonce,
                    PasswordTag = auth.PasswordTag,
                    ClientIp = auth.ClientIp,
                    DeviceKeyThumbprint = auth.DeviceKeyThumbprint,
                    Locale = auth.Locale,
                    ExpiresAtUtc = auth.ExpiresAtUtc
                },
                TimeSpan.FromSeconds(3));

            if (!response.Success)
            {
                Log.Warning("Agent service rejected quick-login auth for {Username}: {Message}", auth.Username, response.Message);
                return new QuickLoginBindResult(false, string.Empty);
            }

            return new QuickLoginBindResult(true, auth.Username);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Could not publish quick-login auth for {Username} to the Agent service", auth.Username);
            return new QuickLoginBindResult(false, string.Empty);
        }
        finally
        {
            ZeroEncryptedPassword(auth);
        }
    }

    public static bool AcceptPublishedToken(QuickLoginAuthPayload payload)
    {
        CleanupExpired();
        if (payload.Token == 0 || payload.ExpiresAtUtc <= DateTime.UtcNow ||
            string.IsNullOrWhiteSpace(payload.Username) || string.IsNullOrWhiteSpace(payload.ClientIp) ||
            payload.PasswordCipher is not { Length: > 0 } || payload.PasswordNonce is not { Length: 12 } ||
            payload.PasswordTag is not { Length: 16 })
            return false;

        var auth = new QuickLoginAgentAuth
        {
            Username = payload.Username,
            PasswordCipher = payload.PasswordCipher,
            PasswordNonce = payload.PasswordNonce,
            PasswordTag = payload.PasswordTag,
            ClientIp = payload.ClientIp,
            DeviceKeyThumbprint = payload.DeviceKeyThumbprint,
            Locale = payload.Locale,
            ExpiresAtUtc = payload.ExpiresAtUtc
        };
        if (PendingAgentTokens.TryAdd(payload.Token, auth))
            return true;

        ZeroEncryptedPassword(auth);
        return false;
    }

    public static bool TryConsumeAgentAuth(
        uint token,
        string clientIp,
        string deviceKeyThumbprint,
        out QuickLoginAgentCredentials auth)
    {
        auth = null!;
        CleanupExpired();

        if (!PendingAgentTokens.TryRemove(token, out var candidate))
            return false;

        if (IsExpired(candidate) ||
            !string.Equals(candidate.ClientIp, clientIp, StringComparison.Ordinal) ||
            (!string.IsNullOrWhiteSpace(candidate.DeviceKeyThumbprint) &&
             !string.Equals(candidate.DeviceKeyThumbprint, deviceKeyThumbprint, StringComparison.Ordinal)))
        {
            ZeroEncryptedPassword(candidate);
            return false;
        }

        try
        {
            auth = new QuickLoginAgentCredentials(
                candidate.Username,
                DecryptPassword(candidate),
                candidate.Locale);
            return true;
        }
        finally
        {
            CryptographicOperations.ZeroMemory(candidate.PasswordCipher);
            CryptographicOperations.ZeroMemory(candidate.PasswordNonce);
            CryptographicOperations.ZeroMemory(candidate.PasswordTag);
        }
    }

    private static bool IsExpired(QuickLoginAgentAuth auth)
    {
        return auth.ExpiresAtUtc <= DateTime.UtcNow;
    }

    private static void CleanupExpired()
    {
        foreach (var pair in PendingGatewaySessions)
        {
            if (IsExpired(pair.Value))
            {
                if (PendingGatewaySessions.TryRemove(pair.Key, out var expired))
                    ZeroEncryptedPassword(expired);
            }
        }

        foreach (var pair in PendingAgentTokens)
        {
            if (IsExpired(pair.Value))
            {
                if (PendingAgentTokens.TryRemove(pair.Key, out var expired))
                    ZeroEncryptedPassword(expired);
            }
        }
    }

    private static (byte[] Cipher, byte[] Nonce, byte[] Tag) EncryptPassword(string password)
    {
        byte[] plain = Encoding.UTF8.GetBytes(password);
        byte[] cipher = new byte[plain.Length];
        byte[] nonce = RandomNumberGenerator.GetBytes(12);
        byte[] tag = new byte[16];
        try
        {
            using var aes = new AesGcm(GetMasterKey(), tag.Length);
            aes.Encrypt(nonce, plain, cipher, tag);
            return (cipher, nonce, tag);
        }
        finally
        {
            CryptographicOperations.ZeroMemory(plain);
        }
    }

    private static string DecryptPassword(QuickLoginAgentAuth auth)
    {
        byte[] plain = new byte[auth.PasswordCipher.Length];
        try
        {
            using var aes = new AesGcm(GetMasterKey(), auth.PasswordTag.Length);
            aes.Decrypt(auth.PasswordNonce, auth.PasswordCipher, auth.PasswordTag, plain);
            return Encoding.UTF8.GetString(plain);
        }
        finally
        {
            CryptographicOperations.ZeroMemory(plain);
        }
    }

    private static byte[] GetMasterKey()
    {
        if (global::Program.QuickLoginMasterKey.Length != 32)
            throw new InvalidOperationException("Quick-login master key is unavailable.");
        return global::Program.QuickLoginMasterKey;
    }

    private static void ZeroEncryptedPassword(QuickLoginAgentAuth auth)
    {
        CryptographicOperations.ZeroMemory(auth.PasswordCipher);
        CryptographicOperations.ZeroMemory(auth.PasswordNonce);
        CryptographicOperations.ZeroMemory(auth.PasswordTag);
    }
}
