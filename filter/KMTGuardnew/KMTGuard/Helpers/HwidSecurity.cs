using System.Globalization;
using System.Security.Cryptography;
using System.Text;
using KMTGuard.SessionManager;

namespace KMTGuard.Helpers;

public static class HwidSecurity
{
    public const string ProtocolMarker = "KMT2";
    public const string GatewayRole = "Gateway";
    public const string AgentRole = "Agent";
    private static readonly TimeSpan ChallengeLifetime = TimeSpan.FromMinutes(2);

    public static bool TryCreateChallenge(ISession session, string serverRole, out string challenge)
    {
        if (!string.IsNullOrWhiteSpace(session.HwidChallenge) &&
            string.Equals(session.HwidChallengeRole, serverRole, StringComparison.Ordinal) &&
            DateTime.UtcNow <= session.HwidChallengeExpiresAt)
        {
            challenge = string.Empty;
            return false;
        }

        challenge = CreateChallenge(session, serverRole);
        return true;
    }

    public static string CreateChallenge(ISession session, string serverRole)
    {
        if (serverRole is not (GatewayRole or AgentRole))
            throw new ArgumentOutOfRangeException(nameof(serverRole));

        var nonce = Convert.ToHexString(RandomNumberGenerator.GetBytes(24));
        var issuedAt = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
        session.HwidChallenge = nonce;
        session.HwidChallengeRole = serverRole;
        session.HwidChallengeIssuedAtUnix = issuedAt;
        session.HwidChallengeExpiresAt = DateTime.UtcNow.Add(ChallengeLifetime);
        return string.Join('|', ProtocolMarker, serverRole,
            issuedAt.ToString(CultureInfo.InvariantCulture), nonce);
    }

    public static bool TryValidateResponse(ISession session, string response, out string hwid)
    {
        hwid = string.Empty;
        if (string.IsNullOrWhiteSpace(response) || response.Length > 4096 ||
            string.IsNullOrWhiteSpace(session.HwidChallenge) ||
            string.IsNullOrWhiteSpace(session.HwidChallengeRole) ||
            DateTime.UtcNow > session.HwidChallengeExpiresAt)
        {
            ResetChallenge(session);
            return false;
        }

        var nonce = session.HwidChallenge;
        var role = session.HwidChallengeRole;
        var issuedAt = session.HwidChallengeIssuedAtUnix;
        ResetChallenge(session); // every submitted proof consumes the challenge

        var parts = response.Split('|');
        if (parts.Length != 4 || !string.Equals(parts[0], ProtocolMarker, StringComparison.Ordinal))
            return false;

        var candidateHwid = parts[1];
        if (candidateHwid.Length != 64 || !candidateHwid.All(Uri.IsHexDigit))
            return false;

        byte[] publicKey;
        byte[] signature;
        try
        {
            publicKey = Convert.FromBase64String(parts[2]);
            signature = Convert.FromBase64String(parts[3]);
        }
        catch (FormatException)
        {
            return false;
        }

        if (publicKey.Length is < 128 or > 1024 || signature.Length != 256)
            return false;

        var normalizedHwid = candidateHwid.ToUpperInvariant();
        var canonical = BuildCanonicalProof(role, issuedAt, nonce, normalizedHwid);
        var hash = SHA256.HashData(Encoding.ASCII.GetBytes(canonical));

        try
        {
            using var rsa = new RSACryptoServiceProvider { PersistKeyInCsp = false };
            rsa.ImportCspBlob(publicKey);
            if (rsa.KeySize != 2048 ||
                !rsa.VerifyHash(hash, "2.16.840.1.101.3.4.2.1", signature))
                return false;
        }
        catch (CryptographicException)
        {
            return false;
        }

        session.DevicePublicKey = Convert.ToBase64String(publicKey);
        session.DeviceKeyThumbprint = Convert.ToHexString(SHA256.HashData(publicKey));
        session.VerifiedHwidNonce = nonce;
        hwid = normalizedHwid;
        return true;
    }

    public static string BuildCanonicalProof(string role, long issuedAt, string nonce, string hwid)
    {
        return string.Join('\n', ProtocolMarker, role,
            issuedAt.ToString(CultureInfo.InvariantCulture), nonce, hwid);
    }

    private static void ResetChallenge(ISession session)
    {
        session.HwidChallenge = string.Empty;
        session.HwidChallengeRole = string.Empty;
        session.HwidChallengeIssuedAtUnix = 0;
        session.HwidChallengeExpiresAt = DateTime.MinValue;
    }
}
