using System;
using System.Collections.Generic;
using System.Data;
using System.Linq;
using System.Threading.Tasks;
using System.Security.Cryptography;
using System.Threading;
using Dapper;
using KMTGuard.Database.Models;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using static System.Collections.Specialized.BitVector32;

namespace KMTGuard.Database
{
    public enum CredentialValidationStatus
    {
        Invalid = 0,
        Valid = 1,
        Unavailable = 2
    }

    public class SItemInfoDbRecord
    {
        public int nRefItemID;
        public byte btOptLevel;
        public string szCodeName = string.Empty;
        public int nItemDBID;
        public byte btAdvOptLevel;
    }
    public class SilkBuyItem
    {
        public int OrderNumber { get; set; }
        public string AccountName { get; set; } = string.Empty;
        public int SilkAmount { get; set; }
        // İstersen diğer sütunları da ekleyebilirsin
    }

    public class sqlQueryHelper
    {
        private static readonly SemaphoreSlim SecondaryPasswordSchemaLock = new(1, 1);
        private static bool _secondaryPasswordSchemaReady;
        private const int SecondaryPasswordIterations = 100_000;

        private static async Task EnsureSecondaryPasswordSchemaAsync(SqlConnection connection)
        {
            if (_secondaryPasswordSchemaReady)
                return;

            await SecondaryPasswordSchemaLock.WaitAsync();
            try
            {
                if (_secondaryPasswordSchemaReady)
                    return;

                var schemaReady = await connection.ExecuteScalarAsync<int>(@"
SELECT CASE WHEN OBJECT_ID(N'dbo.Auth_SecondaryPasswords', N'U') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Auth_SecondaryPasswords', N'PasswordHash') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Auth_SecondaryPasswords', N'PasswordSalt') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Auth_SecondaryPasswords', N'DeviceKeyThumbprint') IS NOT NULL
             THEN 1 ELSE 0 END;");
                if (schemaReady != 1)
                    throw new InvalidOperationException(
                        "Secondary-password schema v3 is missing. Apply the packaged database migration before starting the Filter.");
                _secondaryPasswordSchemaReady = true;
            }
            finally
            {
                SecondaryPasswordSchemaLock.Release();
            }
        }

        private static (byte[] Hash, byte[] Salt) HashSecondaryPassword(string password)
        {
            var salt = RandomNumberGenerator.GetBytes(16);
            var hash = Rfc2898DeriveBytes.Pbkdf2(
                password,
                salt,
                SecondaryPasswordIterations,
                HashAlgorithmName.SHA256,
                32);
            return (hash, salt);
        }

        public static async Task<List<SilkBuyItem>> GetSilkBuyListAsync(string connectionString)
        {
            var list = new List<SilkBuyItem>();

            try
            {
                using (SqlConnection con = new SqlConnection(connectionString))
                {
                    using (SqlCommand cmd = new SqlCommand("SELECT OrderNumber, AccountName, SilkAmount FROM SRO_VT_ACCOUNT..SK_SilkBuyList", con))
                    {
                        await con.OpenAsync();
                        using (SqlDataReader reader = await cmd.ExecuteReaderAsync())
                        {
                            while (await reader.ReadAsync())
                            {
                                var item = new SilkBuyItem
                                {
                                    OrderNumber = reader.GetInt32(0),
                                    AccountName = reader.GetString(1),
                                    SilkAmount = reader.GetInt32(2)
                                };
                                list.Add(item);
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"SQL Error -> Function GetSilkBuyListAsync(), exception: {ex.Message}");
            }

            return list;
        }
        public static async Task<List<int>> GetWhitelistAsync(string connectionString, int ServerType)
        {
            using (SqlConnection con = new SqlConnection(connectionString))
            {
                var whitelist = await con.QueryAsync<int>(
                    "SELECT MsgId FROM [dbo].[Security_Whitelist] WITH (NOLOCK) WHERE ServerType = @ServerType",
                    new { ServerType });
                return whitelist.ToList();
            }
        }

        public static async Task<List<int>> GetBlacklistAsync(string connectionString, int ServerType)
        {
            using (SqlConnection con = new SqlConnection(connectionString))
            {
                var blacklist = await con.QueryAsync<int>(
                    "SELECT MsgId FROM [dbo].[Security_Blacklist] WITH (NOLOCK) WHERE ServerType = @ServerType",
                    new { ServerType });
                return blacklist.ToList();
            }
        }
        public static async Task<CredentialValidationStatus> ValidateUserCredentialsStatusAsync(
            string username, string password)
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();

                    int result = await connection.QuerySingleOrDefaultAsync<int>(
                        "EXEC [dbo].[Auth_Login] @UserName, @Password",
                        new { UserName = username, Password = password });
                    return result == 1
                        ? CredentialValidationStatus.Valid
                        : CredentialValidationStatus.Invalid;
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error validating user credentials: {ex.Message}");
                return CredentialValidationStatus.Unavailable;
            }
        }
        public static async Task<string> prod_string(string query, string connectionstring)
        {
            string value = "";
            try
            {
                using (SqlConnection con = new SqlConnection(connectionstring))
                {
                    using (SqlCommand cmd = new SqlCommand(query, con))
                    {
                        await con.OpenAsync();
                        return Convert.ToString(await cmd.ExecuteScalarAsync());
                    }
                }
            }
            catch (NullReferenceException)
            {
                Log.Warning($"SQL Error -> Function prod_string({query}), exception catched : Null exception");
                return "";
            }
            catch (SqlException Ex)
            {
                Log.Warning($"SQL Error -> Function GetInt({query}), exception catched : {Ex.ToString()}");
            }
            return value;
        }
        public static async Task<int> prod_int(string query, string connectionstring)
        {
            int value = 0;
            try
            {
                using (SqlConnection con = new SqlConnection(connectionstring))
                {
                    using (SqlCommand cmd = new SqlCommand(query, con))
                    {
                        await con.OpenAsync();
                        object? result = await cmd.ExecuteScalarAsync();
                        value = result != null ? Convert.ToInt32(result) : 0;
                        return value;
                    }
                }
            }
            catch (NullReferenceException)
            {
                Log.Warning($"SQL Error -> Function prod_int({query}), exception catched: Null exception");
                return 0;
            }
            catch (SqlException Ex)
            {
                Log.Warning($"SQL Error -> Function prod_int({query}), exception catched: {Ex.ToString()}");
                return 0;
            }
        }
        public static async Task CallLoggerMobKill(string Charname, int CharID, int MobID, int BRegionID, int WorldID, int MonsterClass)
        {
            using (var connection = new SqlConnection(Program.Connectionstring))
            {
                await connection.OpenAsync();
                await connection.ExecuteAsync(
                    "EXEC _LoggerMobKill @Charname, @CharID, @MobID, @BRegionID, @WorldID, @MonsterClass",
                    new { Charname, CharID, MobID, BRegionID, WorldID, MonsterClass });
            }
        }
        public static async Task<bool> AddItemToChest(int CharID, string ItemCodeName, int Quantity, string Type, int Plus)
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();
                    await connection.ExecuteAsync(
                        @"EXEC [dbo].[Item_AddChestByCodeName]
                            @CharID = @CharID,
                            @ItemCodeName = @ItemCodeName,
                            @Quantity = @Quantity,
                            @From = @Type,
                            @Plus = @Plus",
                        new { CharID, ItemCodeName, Quantity, Type, Plus });
                }
                return true;
            }
            catch (Exception ex)
            {
                Log.Error(
                    ex,
                    "Failed to add Item Chest reward by CodeName. CharID={CharID}, ItemCodeName={ItemCodeName}, Quantity={Quantity}, Type={Type}, Plus={Plus}",
                    CharID,
                    ItemCodeName,
                    Quantity,
                    Type,
                    Plus);
                return false;
            }
        }

        public static async Task<bool> ValidateUserCredentials(string username, string password)
        {
            return await ValidateUserCredentialsStatusAsync(username, password) ==
                   CredentialValidationStatus.Valid;
        }
        public static async Task<bool> AddItemToChest2(int CharID, int ItemID, int Quantity, string Type, int Plus)
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();
                    await connection.ExecuteAsync(
                        @"EXEC [dbo].[Item_AddChest]
                            @CharID = @CharID,
                            @ItemRefObjID = @ItemID,
                            @Quantity = @Quantity,
                            @From = @Type,
                            @Plus = @Plus",
                        new { CharID, ItemID, Quantity, Type, Plus });
                }
                return true;
            }
            catch (Exception ex)
            {
                Log.Error(
                    ex,
                    "Failed to add Item Chest reward by ItemID. CharID={CharID}, ItemID={ItemID}, Quantity={Quantity}, Type={Type}, Plus={Plus}",
                    CharID,
                    ItemID,
                    Quantity,
                    Type,
                    Plus);
                return false;
            }
        }
        public static async Task<bool> InsertSecondaryPasswordData(
            string userId, string password, string hwid, string deviceKeyThumbprint, bool rememberPc)
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await EnsureSecondaryPasswordSchemaAsync(connection);
            var passwordData = HashSecondaryPassword(password);
            var affected = await connection.ExecuteAsync(
                        @"INSERT INTO [dbo].[Auth_SecondaryPasswords]
                          (StrUserID, Password, PasswordHash, PasswordSalt, Hwid, DeviceKeyThumbprint, RememberPC)
                          VALUES (@UserId, 0, @PasswordHash, @PasswordSalt, @Hwid, @DeviceKeyThumbprint, @RememberPC)",
                        new
                        {
                            UserId = userId,
                            PasswordHash = passwordData.Hash,
                            PasswordSalt = passwordData.Salt,
                            Hwid = hwid,
                            DeviceKeyThumbprint = deviceKeyThumbprint,
                            RememberPC = rememberPc
                        });
            return affected == 1;
        }

        public static async Task<bool> UpdateSecondaryPasswordData(
            string userId, string password, bool rememberPc, string hwid, string deviceKeyThumbprint)
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await EnsureSecondaryPasswordSchemaAsync(connection);
            var passwordData = HashSecondaryPassword(password);
            var affected = await connection.ExecuteAsync(
                        @"UPDATE [dbo].[Auth_SecondaryPasswords]
                          SET Password = 0, PasswordHash = @PasswordHash,
                              PasswordSalt = @PasswordSalt, RememberPC = @RememberPC, Hwid = @Hwid,
                              DeviceKeyThumbprint = @DeviceKeyThumbprint
                          WHERE StrUserID = @UserId",
                        new
                        {
                            UserId = userId,
                            PasswordHash = passwordData.Hash,
                            PasswordSalt = passwordData.Salt,
                            RememberPC = rememberPc,
                            Hwid = hwid,
                            DeviceKeyThumbprint = deviceKeyThumbprint
                        });
            return affected == 1;
        }
        public static async Task<SecondaryPasswordData?> GetSecondaryPasswordData(string userId)
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await EnsureSecondaryPasswordSchemaAsync(connection);
            return await connection.QuerySingleOrDefaultAsync<SecondaryPasswordData>(
                "SELECT * FROM [dbo].[Auth_SecondaryPasswords] WHERE StrUserID = @UserId",
                new { UserId = userId });
        }

        public static async Task<bool> VerifySecondaryPasswordAsync(string userId, string password)
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await EnsureSecondaryPasswordSchemaAsync(connection);

            var data = await connection.QuerySingleOrDefaultAsync<SecondaryPasswordData>(
                "SELECT * FROM [dbo].[Auth_SecondaryPasswords] WHERE StrUserID = @UserId",
                new { UserId = userId });
            if (data == null)
                return false;

            if (data.PasswordHash is { Length: 32 } && data.PasswordSalt is { Length: 16 })
            {
                var candidate = Rfc2898DeriveBytes.Pbkdf2(
                    password,
                    data.PasswordSalt,
                    SecondaryPasswordIterations,
                    HashAlgorithmName.SHA256,
                    32);
                return CryptographicOperations.FixedTimeEquals(candidate, data.PasswordHash);
            }

            // One-time compatibility migration for existing plaintext rows.
            if (!int.TryParse(password, out var legacyPassword) || data.Password != legacyPassword)
                return false;

            await UpdateSecondaryPasswordData(
                userId, password, data.RememberPC, data.Hwid, data.DeviceKeyThumbprint);
            return true;
        }

        public static async Task<bool> UpdateSecondaryPasswordRememberPcAsync(
            string userId, bool rememberPc, string hwid, string deviceKeyThumbprint)
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            var affected = await connection.ExecuteAsync(
                @"UPDATE [dbo].[Auth_SecondaryPasswords]
                  SET RememberPC = @RememberPC, Hwid = @Hwid,
                      DeviceKeyThumbprint = @DeviceKeyThumbprint
                  WHERE StrUserID = @UserId",
                new
                {
                    UserId = userId,
                    RememberPC = rememberPc,
                    Hwid = hwid,
                    DeviceKeyThumbprint = deviceKeyThumbprint
                });
            return affected == 1;
        }

        public static async Task<bool> BindAccountDeviceAsync(
            string userId, string hwid, string deviceKeyThumbprint, string publicKeyBase64)
        {
            if (string.IsNullOrWhiteSpace(userId) || hwid.Length != 64 ||
                deviceKeyThumbprint.Length != 64)
                return false;

            byte[] publicKey;
            try
            {
                publicKey = Convert.FromBase64String(publicKeyBase64);
            }
            catch (FormatException)
            {
                return false;
            }

            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await using var transaction =
                (SqlTransaction)await connection.BeginTransactionAsync(IsolationLevel.Serializable);

            var existingKey = await connection.QuerySingleOrDefaultAsync<byte[]>(
                @"SELECT PublicKeyBlob
                  FROM dbo.Auth_DeviceKeys WITH (UPDLOCK, HOLDLOCK)
                  WHERE DeviceKeyThumbprint = @DeviceKeyThumbprint;",
                new { DeviceKeyThumbprint = deviceKeyThumbprint }, transaction);
            if (existingKey is not null &&
                !CryptographicOperations.FixedTimeEquals(existingKey, publicKey))
            {
                await transaction.RollbackAsync();
                return false;
            }

            await connection.ExecuteAsync(
                @"IF NOT EXISTS (SELECT 1 FROM dbo.Auth_DeviceKeys WHERE DeviceKeyThumbprint = @DeviceKeyThumbprint)
                      INSERT dbo.Auth_DeviceKeys
                          (DeviceKeyThumbprint, PublicKeyBlob, HardwareHash, FirstSeenUtc, LastSeenUtc)
                      VALUES
                          (@DeviceKeyThumbprint, @PublicKeyBlob, @HardwareHash, SYSUTCDATETIME(), SYSUTCDATETIME());
                  ELSE
                      UPDATE dbo.Auth_DeviceKeys
                         SET HardwareHash = @HardwareHash, LastSeenUtc = SYSUTCDATETIME()
                       WHERE DeviceKeyThumbprint = @DeviceKeyThumbprint AND RevokedUtc IS NULL;

                  IF NOT EXISTS
                     (SELECT 1 FROM dbo.Auth_AccountDeviceKeys
                       WHERE StrUserID = @UserId AND DeviceKeyThumbprint = @DeviceKeyThumbprint)
                      INSERT dbo.Auth_AccountDeviceKeys
                          (StrUserID, DeviceKeyThumbprint, EnrolledUtc, LastValidatedUtc, Status)
                      VALUES
                          (@UserId, @DeviceKeyThumbprint, SYSUTCDATETIME(), SYSUTCDATETIME(), 1);
                  ELSE
                      UPDATE dbo.Auth_AccountDeviceKeys
                         SET LastValidatedUtc = SYSUTCDATETIME()
                       WHERE StrUserID = @UserId AND DeviceKeyThumbprint = @DeviceKeyThumbprint AND Status = 1;",
                new
                {
                    UserId = userId,
                    DeviceKeyThumbprint = deviceKeyThumbprint,
                    PublicKeyBlob = publicKey,
                    HardwareHash = hwid
                }, transaction);

            var active = await connection.ExecuteScalarAsync<int>(
                @"SELECT COUNT(*) FROM dbo.Auth_AccountDeviceKeys
                   WHERE StrUserID = @UserId AND DeviceKeyThumbprint = @DeviceKeyThumbprint AND Status = 1;",
                new { UserId = userId, DeviceKeyThumbprint = deviceKeyThumbprint }, transaction);
            if (active != 1)
            {
                await transaction.RollbackAsync();
                return false;
            }

            await transaction.CommitAsync();
            return true;
        }

        public static async Task<DateTime?> GetSecondaryPasswordBlockAsync(
            string userId, string clientIp, string deviceKeyThumbprint)
        {
            var ipHash = SHA256.HashData(System.Text.Encoding.UTF8.GetBytes(clientIp));
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            return await connection.ExecuteScalarAsync<DateTime?>(
                @"SELECT MAX(BlockedUntilUtc)
                  FROM dbo.Auth_SecondaryPasswordAttempts
                  WHERE StrUserID = @UserId
                    AND (IpHash = @IpHash OR DeviceKeyThumbprint = @DeviceKeyThumbprint)
                    AND BlockedUntilUtc > SYSUTCDATETIME();",
                new { UserId = userId, IpHash = ipHash, DeviceKeyThumbprint = deviceKeyThumbprint });
        }

        public static async Task<DateTime?> RegisterSecondaryPasswordFailureAsync(
            string userId, string clientIp, string deviceKeyThumbprint)
        {
            var ipHash = SHA256.HashData(System.Text.Encoding.UTF8.GetBytes(clientIp));
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            return await connection.QuerySingleAsync<DateTime?>(
                @"DECLARE @Now datetime2(0) = SYSUTCDATETIME();
                  MERGE dbo.Auth_SecondaryPasswordAttempts WITH (HOLDLOCK) AS Target
                  USING (SELECT @UserId AS StrUserID, @IpHash AS IpHash,
                                @DeviceKeyThumbprint AS DeviceKeyThumbprint) AS Source
                     ON Target.StrUserID = Source.StrUserID
                    AND Target.IpHash = Source.IpHash
                    AND Target.DeviceKeyThumbprint = Source.DeviceKeyThumbprint
                  WHEN MATCHED THEN UPDATE SET
                    FailureCount = CASE
                        WHEN Target.BlockedUntilUtc > @Now THEN Target.FailureCount
                        WHEN Target.WindowStartedUtc < DATEADD(MINUTE, -15, @Now) THEN 1
                        WHEN Target.FailureCount + 1 >= 5 THEN 0
                        ELSE Target.FailureCount + 1 END,
                    WindowStartedUtc = CASE
                        WHEN Target.WindowStartedUtc < DATEADD(MINUTE, -15, @Now) THEN @Now
                        ELSE Target.WindowStartedUtc END,
                    BlockedUntilUtc = CASE
                        WHEN Target.BlockedUntilUtc > @Now THEN Target.BlockedUntilUtc
                        WHEN Target.WindowStartedUtc >= DATEADD(MINUTE, -15, @Now)
                             AND Target.FailureCount + 1 >= 5 THEN DATEADD(MINUTE, 15, @Now)
                        ELSE NULL END,
                    LastFailureUtc = @Now
                  WHEN NOT MATCHED THEN
                    INSERT (StrUserID, IpHash, DeviceKeyThumbprint, FailureCount,
                            WindowStartedUtc, LastFailureUtc)
                    VALUES (@UserId, @IpHash, @DeviceKeyThumbprint, 1, @Now, @Now)
                  OUTPUT inserted.BlockedUntilUtc;",
                new { UserId = userId, IpHash = ipHash, DeviceKeyThumbprint = deviceKeyThumbprint });
        }

        public static async Task ClearSecondaryPasswordFailuresAsync(
            string userId, string clientIp, string deviceKeyThumbprint)
        {
            var ipHash = SHA256.HashData(System.Text.Encoding.UTF8.GetBytes(clientIp));
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await connection.ExecuteAsync(
                @"DELETE dbo.Auth_SecondaryPasswordAttempts
                  WHERE StrUserID = @UserId
                    AND IpHash = @IpHash
                    AND DeviceKeyThumbprint = @DeviceKeyThumbprint;",
                new { UserId = userId, IpHash = ipHash, DeviceKeyThumbprint = deviceKeyThumbprint });
        }
    }
}
