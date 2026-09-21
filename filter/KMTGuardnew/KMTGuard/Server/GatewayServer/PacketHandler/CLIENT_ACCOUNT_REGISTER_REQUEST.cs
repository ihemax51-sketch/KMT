using System.Text.RegularExpressions;
using System.Security.Cryptography;
using System.Text;
using Dapper;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Localization;
using KMTGuard.PacketHandlerManager;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Server.GatewayPacketHandler;

public partial class SERVER_DLL_SETTINGS_RESPONSE
{
    private static readonly Regex AccountIdPattern = new("^[a-z0-9_]{4,16}$", RegexOptions.Compiled | RegexOptions.IgnoreCase);
    private static readonly Regex AccountPasswordPattern = new("^[\\x21-\\x7E]{6,32}$", RegexOptions.Compiled);

    private async Task<PacketResult> CLIENT_ACCOUNT_REGISTER_REQUEST(Packet packet, ISession session, object obj)
    {
        try
        {
            var userId = packet.ReadAscii().Trim().ToLowerInvariant();
            var password = packet.ReadAscii();
            var confirmPassword = packet.ReadAscii();

            if (!AccountIdPattern.IsMatch(userId))
            {
                await SendRegisterResponse(session, false, PlayerLanguage.Get("AccountRegister.InvalidId"));
                return new PacketResult(PacketResultType.Block);
            }

            if (!AccountPasswordPattern.IsMatch(password))
            {
                await SendRegisterResponse(session, false, PlayerLanguage.Get("AccountRegister.InvalidPassword"));
                return new PacketResult(PacketResultType.Block);
            }

            if (password != confirmPassword)
            {
                await SendRegisterResponse(session, false, PlayerLanguage.Get("AccountRegister.PasswordMismatch"));
                return new PacketResult(PacketResultType.Block);
            }

            var result = await TryCreateGatewayAccountAsync(userId, password, session.ClientIp);
            await SendRegisterResponse(session, result.Success, result.Message);
            return new PacketResult(PacketResultType.Block);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "CLIENT_ACCOUNT_REGISTER_REQUEST failed for {ClientIp}", session.ClientIp);
            await SendRegisterResponse(session, false, PlayerLanguage.Get("AccountRegister.Failed"));
            return new PacketResult(PacketResultType.Block);
        }
    }

    private static async Task<(bool Success, string Message)> TryCreateGatewayAccountAsync(
        string userId,
        string password,
        string clientIp)
    {
        var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);

        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await using var transaction = await connection.BeginTransactionAsync();

        try
        {
            var exists = await connection.ExecuteScalarAsync<int>(
                $@"SELECT COUNT(1)
                   FROM {accountDb}..TB_User WITH (UPDLOCK, HOLDLOCK)
                   WHERE StrUserID = @UserId",
                new { UserId = userId },
                transaction);

            if (exists > 0)
            {
                await transaction.RollbackAsync();
                return (false, PlayerLanguage.Get("AccountRegister.IdExists"));
            }

            var jid = await connection.ExecuteScalarAsync<int>(
                $@"INSERT INTO {accountDb}..TB_User
                      (StrUserID, [password], Email, regtime, reg_ip, Time_log, freetime)
                   OUTPUT INSERTED.JID
                   VALUES
                      (@UserId, @Password, @Email, GETDATE(), @RegIp, GETDATE(), 0);",
                new
                {
                    UserId = userId,
                    Password = Md5Hex(password),
                    Email = $"{userId}@kmtguard.local",
                    RegIp = clientIp.Length > 25 ? clientIp[..25] : clientIp
                },
                transaction);

            await connection.ExecuteAsync(
                $@"IF OBJECT_ID(N'{_serverSettings.AccountDB}..SK_Silk', N'U') IS NOT NULL
                   AND NOT EXISTS (SELECT 1 FROM {accountDb}..SK_Silk WITH (UPDLOCK, HOLDLOCK) WHERE JID = @JID)
                   BEGIN
                       INSERT INTO {accountDb}..SK_Silk (JID, silk_own, silk_gift, silk_point)
                       VALUES (@JID, 0, 0, 0);
                   END",
                new { JID = jid },
                transaction);

            await transaction.CommitAsync();
            Log.Information("Gateway account registered from {ClientIp}: {UserId} JID={JID}", clientIp, userId, jid);
            return (true, PlayerLanguage.Get("AccountRegister.Success"));
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }
    }

    private static string Md5Hex(string value)
    {
        var bytes = MD5.HashData(Encoding.Default.GetBytes(value));
        var builder = new StringBuilder(bytes.Length * 2);
        foreach (var b in bytes)
            builder.Append(b.ToString("x2"));
        return builder.ToString();
    }

    private static async Task SendRegisterResponse(ISession session, bool success, string message)
    {
        var registerResponse = new Packet(0x166B);
        registerResponse.WriteUInt8((byte)(success ? 1 : 0));
        registerResponse.WriteUnicode(message);
        await session.SendToClient(registerResponse);
    }
}
