using System.Buffers.Binary;
using System.Security.Cryptography;
using Microsoft.Data.SqlClient;
using SilkroadSecurityAPI;

namespace KMTGuard.Helpers;

internal static class GameServerPacketAuthenticator
{
    private const byte ProtocolVersion = 2;
    private static byte[]? _sharedSecret;

    public static async Task InitializeAsync(string connectionString)
    {
        await using var connection = new SqlConnection(connectionString);
        await connection.OpenAsync();
        await using var command = new SqlCommand(
            "SELECT TOP (1) Value FROM dbo.System_Settings WHERE SettingName = 'Security_InternalPacketSharedSecret'",
            connection)
        {
            CommandTimeout = 30
        };

        var value = Convert.ToString(await command.ExecuteScalarAsync());
        if (value is null || value.Length != 64)
            throw new InvalidOperationException("Internal packet authentication secret is missing or invalid.");

        byte[] decoded;
        try
        {
            decoded = Convert.FromHexString(value);
        }
        catch (FormatException exception)
        {
            throw new InvalidOperationException("Internal packet authentication secret is not hexadecimal.", exception);
        }

        if (decoded.Length != 32)
            throw new InvalidOperationException("Internal packet authentication secret must contain 32 bytes.");

        var previous = Interlocked.Exchange(ref _sharedSecret, decoded);
        if (previous is not null)
            CryptographicOperations.ZeroMemory(previous);
    }

    internal static void InitializeForTesting(string sharedSecretHex)
    {
        var decoded = Convert.FromHexString(sharedSecretHex);
        if (decoded.Length != 32)
            throw new ArgumentException("The test secret must contain 32 bytes.", nameof(sharedSecretHex));
        var previous = Interlocked.Exchange(ref _sharedSecret, decoded);
        if (previous is not null)
            CryptographicOperations.ZeroMemory(previous);
    }

    public static Packet BuildRegistrationPacket(string sessionKeyHex, uint gameId)
    {
        var secret = Volatile.Read(ref _sharedSecret)
            ?? throw new InvalidOperationException("Internal packet authentication is not initialized.");
        if (gameId == 0)
            throw new InvalidOperationException("The character GameID is unavailable.");

        byte[] sessionKey;
        try
        {
            sessionKey = Convert.FromHexString(sessionKeyHex);
        }
        catch (FormatException exception)
        {
            throw new InvalidOperationException("The GameServer session key is invalid.", exception);
        }
        if (sessionKey.Length != 32)
            throw new InvalidOperationException("The GameServer session key must contain 32 bytes.");

        var nonce = RandomNumberGenerator.GetBytes(16);
        var issuedAt = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
        var signedBytes = new byte[61];
        signedBytes[0] = ProtocolVersion;
        BinaryPrimitives.WriteUInt32LittleEndian(signedBytes.AsSpan(1, 4), gameId);
        BinaryPrimitives.WriteInt64LittleEndian(signedBytes.AsSpan(5, 8), issuedAt);
        nonce.CopyTo(signedBytes, 13);
        sessionKey.CopyTo(signedBytes, 29);

        byte[] mac;
        using (var hmac = new HMACSHA256(secret))
            mac = hmac.ComputeHash(signedBytes);

        var packet = new Packet(0x35FE, false, false);
        packet.WriteUInt8(ProtocolVersion);
        packet.WriteUInt32(gameId);
        packet.WriteInt64(issuedAt);
        packet.WriteUInt8Array(nonce);
        packet.WriteUInt8Array(sessionKey);
        packet.WriteUInt8Array(mac);

        CryptographicOperations.ZeroMemory(signedBytes);
        CryptographicOperations.ZeroMemory(sessionKey);
        CryptographicOperations.ZeroMemory(mac);
        return packet;
    }
}
