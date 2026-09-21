SET NOCOUNT ON;

IF NOT EXISTS (SELECT 1 FROM dbo.System_SchemaVersion WHERE Component=N'KMTGuard' AND Version='3.0.0')
    THROW 51001, 'KMTGuard schema version 3.0.0 is not active.', 1;

IF OBJECT_ID(N'dbo.Auth_DeviceKeys', N'U') IS NULL OR
   OBJECT_ID(N'dbo.Auth_AccountDeviceKeys', N'U') IS NULL OR
   OBJECT_ID(N'dbo.Auth_SecondaryPasswordAttempts', N'U') IS NULL
    THROW 51002, 'KMTGuard v3 device or secondary-password tables are missing.', 1;

IF OBJECT_ID(N'dbo.System_ExternalSettings', N'U') IS NULL OR
   EXISTS
   (
       SELECT 1 FROM dbo.System_Settings
        WHERE SettingName IN
              (N'EnablePartyMonsterSpawn',N'PartyMonsterMinimumMembers',N'PartyMonsterSpawnRate')
   )
    THROW 51006, 'GameServer-owned settings were not separated from Filter settings.', 1;

IF EXISTS
(
    SELECT 1 FROM dbo.System_Settings
    WHERE SettingName = N'OfflineStallConfirmSeconds'
)
    THROW 51009, 'The retired OfflineStallConfirmSeconds setting still exists.', 1;

IF OBJECT_ID(N'dbo.Security_Whitelist', N'U') IS NOT NULL
   AND EXISTS
   (
       SELECT 1 FROM dbo.Security_Whitelist
       WHERE MsgId = 6355 AND ServerType IN (2, 3)
   )
    THROW 51010, 'The retired Offline Stall decision opcode is still whitelisted.', 1;

IF OBJECT_ID(N'dbo.QuickLogin_Tokens', N'U') IS NULL OR
   OBJECT_ID(N'dbo.QuickLogin_Log', N'U') IS NULL OR
   COL_LENGTH(N'dbo.QuickLogin_Tokens',N'PasswordNonce') IS NULL OR
   COL_LENGTH(N'dbo.QuickLogin_Tokens',N'PasswordTag') IS NULL OR
   COL_LENGTH(N'dbo.QuickLogin_Tokens',N'EncryptionVersion') IS NULL OR
   COL_LENGTH(N'dbo.QuickLogin_Tokens',N'LockedUntilUtc') IS NULL
    THROW 51007, 'Quick Login v3 schema is incomplete.', 1;

IF EXISTS (SELECT 1 FROM dbo.QuickLogin_Tokens WHERE EncryptionVersion < 2 AND IsActive=1)
    THROW 51008, 'A legacy unauthenticated Quick Login token is still active.', 1;

IF EXISTS (SELECT 1 FROM dbo.Command_PlannedQueue WHERE CommandID=100 AND Status IN (1,2))
    THROW 51003, 'An executable free-form SQL command remains in the planned queue.', 1;

IF EXISTS
(
    SELECT RequiredColumn
    FROM (VALUES
        (N'ClaimToken'), (N'ClaimedUtc'), (N'ClaimedBy'), (N'Attempts'),
        (N'LastError'), (N'CreatedUtc'), (N'CompletedUtc'), (N'IdempotencyKey')) R(RequiredColumn)
    WHERE COL_LENGTH(N'dbo.Command_FilterQueue', RequiredColumn) IS NULL
       OR COL_LENGTH(N'dbo.Command_PlannedQueue', RequiredColumn) IS NULL
)
    THROW 51004, 'KMTGuard v3 queue claim columns are incomplete.', 1;

IF NOT EXISTS (SELECT 1 FROM sys.database_principals WHERE name=N'KMTGuardRuntimeRole' AND type='R')
    THROW 51005, 'KMTGuardRuntimeRole is missing.', 1;

SELECT N'KMTGuard v3 security runtime validation passed.' AS Result;
