using System.Collections.Concurrent;
using KMTGuard.RuntimeContract;
using KMTGuard.SessionManager;
using Serilog;

namespace KMTGuard.Server;

public sealed class QuickLoginAgentAuth
{
    public required string Username { get; init; }
    public required string Password { get; init; }
    public required string ClientIp { get; init; }
    public required byte Locale { get; init; }
    public required DateTime ExpiresAtUtc { get; init; }
}

public readonly record struct QuickLoginBindResult(bool Success, string Username);

public static class QuickLoginAgentAuthBridge
{
    private static readonly TimeSpan AuthTtl = TimeSpan.FromSeconds(45);
    private static readonly ConcurrentDictionary<Guid, QuickLoginAgentAuth> PendingGatewaySessions = new();
    private static readonly ConcurrentDictionary<uint, QuickLoginAgentAuth> PendingAgentTokens = new();

    public static void BeginGatewayLogin(ISession session, string username, string password, byte locale)
    {
        CleanupExpired();

        PendingGatewaySessions[session.ClientGuid] = new QuickLoginAgentAuth
        {
            Username = username,
            Password = password,
            ClientIp = session.ClientIp,
            Locale = locale,
            ExpiresAtUtc = DateTime.UtcNow.Add(AuthTtl)
        };
    }

    public static void CancelGatewayLogin(Guid gatewaySessionId)
    {
        PendingGatewaySessions.TryRemove(gatewaySessionId, out _);
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
            return new QuickLoginBindResult(false, string.Empty);

        if (global::Program.CurrentRole == FilterRole.All)
        {
            PendingAgentTokens[token] = auth;
            return new QuickLoginBindResult(true, auth.Username);
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
                    Password = auth.Password,
                    ClientIp = auth.ClientIp,
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
    }

    public static bool AcceptPublishedToken(QuickLoginAuthPayload payload)
    {
        CleanupExpired();
        if (payload.Token == 0 || payload.ExpiresAtUtc <= DateTime.UtcNow ||
            string.IsNullOrWhiteSpace(payload.Username) || string.IsNullOrWhiteSpace(payload.ClientIp))
            return false;

        PendingAgentTokens[payload.Token] = new QuickLoginAgentAuth
        {
            Username = payload.Username,
            Password = payload.Password,
            ClientIp = payload.ClientIp,
            Locale = payload.Locale,
            ExpiresAtUtc = payload.ExpiresAtUtc
        };
        return true;
    }

    public static bool TryConsumeAgentAuth(uint token, string clientIp, out QuickLoginAgentAuth auth)
    {
        auth = null!;
        CleanupExpired();

        if (!PendingAgentTokens.TryRemove(token, out var candidate))
            return false;

        if (IsExpired(candidate) || !string.Equals(candidate.ClientIp, clientIp, StringComparison.Ordinal))
            return false;

        auth = candidate;
        return true;
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
                PendingGatewaySessions.TryRemove(pair.Key, out _);
        }

        foreach (var pair in PendingAgentTokens)
        {
            if (IsExpired(pair.Value))
                PendingAgentTokens.TryRemove(pair.Key, out _);
        }
    }
}
