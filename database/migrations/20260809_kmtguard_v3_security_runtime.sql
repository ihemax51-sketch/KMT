SET NOCOUNT ON;
SET XACT_ABORT ON;

BEGIN TRANSACTION;

IF OBJECT_ID(N'dbo.Party_Members', N'U') IS NOT NULL
    DELETE FROM dbo.Party_Members;

IF OBJECT_ID(N'dbo.Auth_SecondaryPasswords', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Auth_SecondaryPasswords
    (
        ID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_Auth_SecondaryPasswords PRIMARY KEY,
        StrUserID nvarchar(128) NOT NULL,
        Password int NOT NULL CONSTRAINT DF_Auth_SecondaryPasswords_Password DEFAULT (0),
        PasswordHash varbinary(32) NULL,
        PasswordSalt varbinary(16) NULL,
        Hwid varchar(64) NOT NULL CONSTRAINT DF_Auth_SecondaryPasswords_Hwid DEFAULT (''),
        DeviceKeyThumbprint char(64) NOT NULL CONSTRAINT DF_Auth_SecondaryPasswords_DeviceKey DEFAULT (''),
        RememberPC bit NOT NULL CONSTRAINT DF_Auth_SecondaryPasswords_Remember DEFAULT (0)
    );
    CREATE UNIQUE INDEX UX_Auth_SecondaryPasswords_User
        ON dbo.Auth_SecondaryPasswords(StrUserID);
END;

IF COL_LENGTH(N'dbo.Auth_SecondaryPasswords', N'PasswordHash') IS NULL
    ALTER TABLE dbo.Auth_SecondaryPasswords ADD PasswordHash varbinary(32) NULL;
IF COL_LENGTH(N'dbo.Auth_SecondaryPasswords', N'PasswordSalt') IS NULL
    ALTER TABLE dbo.Auth_SecondaryPasswords ADD PasswordSalt varbinary(16) NULL;
IF COL_LENGTH(N'dbo.Auth_SecondaryPasswords', N'DeviceKeyThumbprint') IS NULL
    ALTER TABLE dbo.Auth_SecondaryPasswords
        ADD DeviceKeyThumbprint char(64) NOT NULL
            CONSTRAINT DF_Auth_SecondaryPasswords_DeviceKey_v3 DEFAULT ('');

IF OBJECT_ID(N'dbo.Auth_DeviceKeys', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Auth_DeviceKeys
    (
        DeviceKeyThumbprint char(64) NOT NULL CONSTRAINT PK_Auth_DeviceKeys PRIMARY KEY,
        PublicKeyBlob varbinary(1024) NOT NULL,
        HardwareHash char(64) NOT NULL,
        FirstSeenUtc datetime2(0) NOT NULL,
        LastSeenUtc datetime2(0) NOT NULL,
        RevokedUtc datetime2(0) NULL
    );
END;

IF OBJECT_ID(N'dbo.Auth_AccountDeviceKeys', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Auth_AccountDeviceKeys
    (
        StrUserID nvarchar(128) NOT NULL,
        DeviceKeyThumbprint char(64) NOT NULL,
        EnrolledUtc datetime2(0) NOT NULL,
        LastValidatedUtc datetime2(0) NOT NULL,
        Status tinyint NOT NULL,
        CONSTRAINT PK_Auth_AccountDeviceKeys PRIMARY KEY(StrUserID, DeviceKeyThumbprint),
        CONSTRAINT FK_Auth_AccountDeviceKeys_Device FOREIGN KEY(DeviceKeyThumbprint)
            REFERENCES dbo.Auth_DeviceKeys(DeviceKeyThumbprint),
        CONSTRAINT CK_Auth_AccountDeviceKeys_Status CHECK(Status IN (0,1,2))
    );
    CREATE INDEX IX_Auth_AccountDeviceKeys_Device
        ON dbo.Auth_AccountDeviceKeys(DeviceKeyThumbprint, Status);
END;

IF OBJECT_ID(N'dbo.Auth_SecondaryPasswordAttempts', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Auth_SecondaryPasswordAttempts
    (
        StrUserID nvarchar(128) NOT NULL,
        IpHash binary(32) NOT NULL,
        DeviceKeyThumbprint char(64) NOT NULL,
        FailureCount int NOT NULL,
        WindowStartedUtc datetime2(0) NOT NULL,
        LastFailureUtc datetime2(0) NOT NULL,
        BlockedUntilUtc datetime2(0) NULL,
        CONSTRAINT PK_Auth_SecondaryPasswordAttempts
            PRIMARY KEY(StrUserID, IpHash, DeviceKeyThumbprint),
        CONSTRAINT CK_Auth_SecondaryPasswordAttempts_Count CHECK(FailureCount BETWEEN 0 AND 5)
    );
    CREATE INDEX IX_Auth_SecondaryPasswordAttempts_Expiry
        ON dbo.Auth_SecondaryPasswordAttempts(BlockedUntilUtc, LastFailureUtc);
END;

IF OBJECT_ID(N'dbo.Command_Quarantine', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Command_Quarantine
    (
        QuarantineID bigint IDENTITY(1,1) NOT NULL CONSTRAINT PK_Command_Quarantine PRIMARY KEY,
        SourceQueue sysname NOT NULL,
        SourceID int NOT NULL,
        CommandID int NOT NULL,
        RawCommand nvarchar(max) NULL,
        Reason nvarchar(500) NOT NULL,
        QuarantinedUtc datetime2(0) NOT NULL CONSTRAINT DF_Command_Quarantine_Utc DEFAULT SYSUTCDATETIME()
    );
END;

IF OBJECT_ID(N'dbo.Command_ExecutionLedger', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Command_ExecutionLedger
    (
        IdempotencyKey uniqueidentifier NOT NULL CONSTRAINT PK_Command_ExecutionLedger PRIMARY KEY,
        SourceQueue sysname NOT NULL,
        SourceID int NOT NULL,
        CommandID int NOT NULL,
        StartedUtc datetime2(0) NOT NULL,
        CompletedUtc datetime2(0) NULL,
        Result tinyint NOT NULL,
        CONSTRAINT CK_Command_ExecutionLedger_Result CHECK(Result IN (0,1,2))
    );
END;

DECLARE @Queue sysname;
DECLARE QueueCursor CURSOR LOCAL FAST_FORWARD FOR
    SELECT Name FROM (VALUES (N'Command_FilterQueue'), (N'Command_PlannedQueue')) Q(Name);
OPEN QueueCursor;
FETCH NEXT FROM QueueCursor INTO @Queue;
WHILE @@FETCH_STATUS = 0
BEGIN
    IF OBJECT_ID(N'dbo.' + @Queue, N'U') IS NULL
        THROW 51000, 'A required command queue is missing.', 1;

    DECLARE @Sql nvarchar(max) = N'';
    IF COL_LENGTH(N'dbo.' + @Queue, N'ClaimToken') IS NULL
        SET @Sql += N'ALTER TABLE dbo.' + QUOTENAME(@Queue) + N' ADD ClaimToken uniqueidentifier NULL;';
    IF COL_LENGTH(N'dbo.' + @Queue, N'ClaimedUtc') IS NULL
        SET @Sql += N'ALTER TABLE dbo.' + QUOTENAME(@Queue) + N' ADD ClaimedUtc datetime2(0) NULL;';
    IF COL_LENGTH(N'dbo.' + @Queue, N'ClaimedBy') IS NULL
        SET @Sql += N'ALTER TABLE dbo.' + QUOTENAME(@Queue) + N' ADD ClaimedBy nvarchar(128) NULL;';
    IF COL_LENGTH(N'dbo.' + @Queue, N'Attempts') IS NULL
        SET @Sql += N'ALTER TABLE dbo.' + QUOTENAME(@Queue) + N' ADD Attempts int NOT NULL CONSTRAINT DF_' + @Queue + N'_Attempts DEFAULT (0);';
    IF COL_LENGTH(N'dbo.' + @Queue, N'LastError') IS NULL
        SET @Sql += N'ALTER TABLE dbo.' + QUOTENAME(@Queue) + N' ADD LastError nvarchar(1000) NULL;';
    IF COL_LENGTH(N'dbo.' + @Queue, N'CreatedUtc') IS NULL
        SET @Sql += N'ALTER TABLE dbo.' + QUOTENAME(@Queue) + N' ADD CreatedUtc datetime2(0) NOT NULL CONSTRAINT DF_' + @Queue + N'_CreatedUtc DEFAULT SYSUTCDATETIME();';
    IF COL_LENGTH(N'dbo.' + @Queue, N'CompletedUtc') IS NULL
        SET @Sql += N'ALTER TABLE dbo.' + QUOTENAME(@Queue) + N' ADD CompletedUtc datetime2(0) NULL;';
    IF COL_LENGTH(N'dbo.' + @Queue, N'IdempotencyKey') IS NULL
        SET @Sql += N'ALTER TABLE dbo.' + QUOTENAME(@Queue) + N' ADD IdempotencyKey uniqueidentifier NULL;';
    IF @Sql <> N'' EXEC sys.sp_executesql @Sql;

    SET @Sql = N'IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id=OBJECT_ID(N''dbo.' + @Queue + N''') AND name=N''IX_' + @Queue + N'_Claim'')
        CREATE INDEX IX_' + @Queue + N'_Claim ON dbo.' + QUOTENAME(@Queue) + N'(Status, ClaimedUtc, ID);';
    EXEC sys.sp_executesql @Sql;

    SET @Sql = N'IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id=OBJECT_ID(N''dbo.' + @Queue + N''') AND name=N''UX_' + @Queue + N'_Idempotency'')
        CREATE UNIQUE INDEX UX_' + @Queue + N'_Idempotency ON dbo.' + QUOTENAME(@Queue) + N'(IdempotencyKey) WHERE IdempotencyKey IS NOT NULL;';
    EXEC sys.sp_executesql @Sql;

    FETCH NEXT FROM QueueCursor INTO @Queue;
END;
CLOSE QueueCursor;
DEALLOCATE QueueCursor;

INSERT dbo.Command_Quarantine(SourceQueue, SourceID, CommandID, RawCommand, Reason)
SELECT N'Command_PlannedQueue', ID, CommandID, Data1,
       N'Free-form planned SQL was disabled by the KMTGuard v3 security cutover.'
FROM dbo.Command_PlannedQueue Q
WHERE CommandID = 100 AND Status = 1
  AND NOT EXISTS
      (SELECT 1 FROM dbo.Command_Quarantine X
       WHERE X.SourceQueue=N'Command_PlannedQueue' AND X.SourceID=Q.ID);

-- Compile this statement only after the queue-column ALTER statements above
-- have completed. A static UPDATE in the same batch is bound before execution
-- and fails with error 207 on databases that do not have the v3 columns yet.
EXEC sys.sp_executesql N'
UPDATE dbo.Command_PlannedQueue
   SET Status=4, CompletedUtc=SYSUTCDATETIME(),
       LastError=N''Quarantined: free-form SQL is not supported in v3.''
 WHERE CommandID=100 AND Status=1;';

IF OBJECT_ID(N'dbo.QuickLogin_Tokens', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.QuickLogin_Tokens
    (
        ID bigint IDENTITY(1,1) NOT NULL CONSTRAINT PK_QuickLogin_Tokens PRIMARY KEY,
        AccountID int NOT NULL,
        Username nvarchar(128) NOT NULL,
        TokenHash binary(32) NOT NULL,
        DeviceIDHash binary(32) NOT NULL,
        DeviceName nvarchar(128) NULL,
        CreatedDate datetime2(0) NOT NULL CONSTRAINT DF_QuickLogin_Tokens_Created DEFAULT SYSUTCDATETIME(),
        ExpireDate datetime2(0) NULL,
        LastUsedDate datetime2(0) NULL,
        RevokedDate datetime2(0) NULL,
        IsActive bit NOT NULL CONSTRAINT DF_QuickLogin_Tokens_Active DEFAULT (1),
        FailedTryCount int NOT NULL CONSTRAINT DF_QuickLogin_Tokens_Failed DEFAULT (0),
        LockedUntilUtc datetime2(0) NULL,
        LastIP varchar(64) NULL,
        PasswordCipher varbinary(512) NULL,
        PasswordIV varbinary(16) NULL,
        PasswordNonce varbinary(12) NULL,
        PasswordTag varbinary(16) NULL,
        EncryptionVersion tinyint NOT NULL CONSTRAINT DF_QuickLogin_Tokens_EncryptionVersion DEFAULT (2),
        Locale tinyint NOT NULL CONSTRAINT DF_QuickLogin_Tokens_Locale DEFAULT (22),
        ServerID int NOT NULL CONSTRAINT DF_QuickLogin_Tokens_Server DEFAULT (0)
    );
    CREATE UNIQUE INDEX UX_QuickLogin_Tokens_TokenHash
        ON dbo.QuickLogin_Tokens(TokenHash);
    CREATE INDEX IX_QuickLogin_Tokens_AccountActive
        ON dbo.QuickLogin_Tokens(AccountID, IsActive, CreatedDate DESC);
END;

IF OBJECT_ID(N'dbo.QuickLogin_Log', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.QuickLogin_Log
    (
        ID bigint IDENTITY(1,1) NOT NULL CONSTRAINT PK_QuickLogin_Log PRIMARY KEY,
        AccountID int NULL,
        Username nvarchar(128) NOT NULL,
        DeviceIDHash binary(32) NULL,
        IP varchar(64) NOT NULL,
        ActionType varchar(32) NOT NULL,
        Result varchar(32) NOT NULL,
        CreatedDate datetime2(0) NOT NULL CONSTRAINT DF_QuickLogin_Log_Created DEFAULT SYSUTCDATETIME()
    );
    CREATE INDEX IX_QuickLogin_Log_AccountDate
        ON dbo.QuickLogin_Log(AccountID, CreatedDate DESC);
END;

IF COL_LENGTH(N'dbo.QuickLogin_Tokens', N'PasswordNonce') IS NULL
    ALTER TABLE dbo.QuickLogin_Tokens ADD PasswordNonce varbinary(12) NULL;
IF COL_LENGTH(N'dbo.QuickLogin_Tokens', N'PasswordTag') IS NULL
    ALTER TABLE dbo.QuickLogin_Tokens ADD PasswordTag varbinary(16) NULL;
IF COL_LENGTH(N'dbo.QuickLogin_Tokens', N'EncryptionVersion') IS NULL
    ALTER TABLE dbo.QuickLogin_Tokens
        ADD EncryptionVersion tinyint NOT NULL
            CONSTRAINT DF_QuickLogin_Tokens_EncryptionVersion DEFAULT (1);
IF COL_LENGTH(N'dbo.QuickLogin_Tokens', N'LockedUntilUtc') IS NULL
    ALTER TABLE dbo.QuickLogin_Tokens ADD LockedUntilUtc datetime2(0) NULL;

-- Version 1 kept both its CBC key and ciphertext in SQL. It cannot be migrated
-- safely without retaining that exposed secret, so the v3 cutover revokes it.
EXEC sys.sp_executesql N'
UPDATE dbo.QuickLogin_Tokens
   SET IsActive=0, RevokedDate=COALESCE(RevokedDate,SYSUTCDATETIME())
 WHERE EncryptionVersion < 2 AND IsActive=1;';

IF OBJECT_ID(N'dbo.QuickLogin_ServerSecret', N'U') IS NOT NULL
    DELETE FROM dbo.QuickLogin_ServerSecret;

IF OBJECT_ID(N'dbo.System_Settings', N'U') IS NULL
    THROW 51001, 'The required Filter settings table is missing.', 1;

IF OBJECT_ID(N'dbo.System_ExternalSettings', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.System_ExternalSettings
    (
        SettingName nvarchar(128) NOT NULL CONSTRAINT PK_System_ExternalSettings PRIMARY KEY,
        Value nvarchar(1000) NOT NULL,
        Owner nvarchar(64) NOT NULL,
        MigratedUtc datetime2(0) NOT NULL CONSTRAINT DF_System_ExternalSettings_Utc DEFAULT SYSUTCDATETIME()
    );
END;

MERGE dbo.System_ExternalSettings AS Target
USING
(
    SELECT SettingName, Value, N'GameServer' AS Owner
      FROM dbo.System_Settings
     WHERE SettingName IN
           (N'EnablePartyMonsterSpawn',N'PartyMonsterMinimumMembers',N'PartyMonsterSpawnRate')
) AS Source
ON Target.SettingName=Source.SettingName
WHEN MATCHED THEN UPDATE SET Value=Source.Value, Owner=Source.Owner
WHEN NOT MATCHED THEN INSERT(SettingName,Value,Owner)
     VALUES(Source.SettingName,Source.Value,Source.Owner);

DELETE dbo.System_Settings
 WHERE SettingName IN
       (N'EnablePartyMonsterSpawn',N'PartyMonsterMinimumMembers',N'PartyMonsterSpawnRate');

-- Retired by the offline-stall integrity update. Older customer databases can
-- still contain this row when they cut over directly to v3.
DELETE dbo.System_Settings
 WHERE SettingName = N'OfflineStallConfirmSeconds';

IF OBJECT_ID(N'dbo.Security_Whitelist', N'U') IS NOT NULL
BEGIN
    IF NOT EXISTS
    (
        SELECT 1 FROM dbo.Security_Whitelist
        WHERE ServerType = 3 AND MsgId = 6352
    )
        INSERT dbo.Security_Whitelist(ServerType, MsgId) VALUES (3, 6352);

    DELETE dbo.Security_Whitelist
    WHERE MsgId = 6355 AND ServerType IN (2, 3);
END;

IF OBJECT_ID(N'dbo.__Whitelist', N'U') IS NOT NULL
BEGIN
    DELETE dbo.__Whitelist
    WHERE MsgId = 6355 AND ServerType IN (2, 3);
END;

MERGE dbo.System_Settings AS Target
USING (VALUES
    (N'EnableQuickLogin',N'True'),
    (N'LogDB',N'SRO_VT_SHARDLOG'),
    (N'DynamicRankingRefreshMinutes',N'10'),
    (N'VipSystemEnabled',N'True'),
    (N'EnablePvpChallenge',N'True'),
    (N'ShowGuidePvpChallenge',N'True'),
    (N'ShowGuideKillerAnimation',N'True'),
    (N'AutoEquipMaxLevel',N'90'),
    (N'NewInventoryDesign',N'True'),
    (N'EnableOfflineStall',N'True'),
    (N'Menu-like-maxi',N'False'),
    (N'OfflineStallMaxHours',N'24'),
    (N'AllowBotLogin',N'True'),
    (N'AllowBotTrade',N'True'),
    (N'BotProtectionLogEnabled',N'True'),
    (N'DisableDurability',N'False')
) AS Source(SettingName,Value)
ON Target.SettingName=Source.SettingName
WHEN NOT MATCHED THEN INSERT(SettingName,Value)
     VALUES(Source.SettingName,Source.Value);

IF NOT EXISTS (SELECT 1 FROM sys.database_principals WHERE name=N'KMTGuardRuntimeRole')
    CREATE ROLE KMTGuardRuntimeRole AUTHORIZATION dbo;
GRANT CONNECT TO KMTGuardRuntimeRole;
GRANT SELECT ON SCHEMA::dbo TO KMTGuardRuntimeRole;
GRANT EXECUTE ON SCHEMA::dbo TO KMTGuardRuntimeRole;
GRANT INSERT, UPDATE, DELETE ON SCHEMA::dbo TO KMTGuardRuntimeRole;
GRANT INSERT, UPDATE, DELETE ON dbo.Auth_SecondaryPasswords TO KMTGuardRuntimeRole;
GRANT INSERT, UPDATE, DELETE ON dbo.Auth_DeviceKeys TO KMTGuardRuntimeRole;
GRANT INSERT, UPDATE, DELETE ON dbo.Auth_AccountDeviceKeys TO KMTGuardRuntimeRole;
GRANT INSERT, UPDATE, DELETE ON dbo.Auth_SecondaryPasswordAttempts TO KMTGuardRuntimeRole;
GRANT INSERT, UPDATE ON dbo.Command_FilterQueue TO KMTGuardRuntimeRole;
GRANT INSERT, UPDATE ON dbo.Command_PlannedQueue TO KMTGuardRuntimeRole;
GRANT INSERT, UPDATE ON dbo.Command_Quarantine TO KMTGuardRuntimeRole;
GRANT INSERT, UPDATE ON dbo.Command_ExecutionLedger TO KMTGuardRuntimeRole;
GRANT INSERT, UPDATE ON dbo.QuickLogin_Tokens TO KMTGuardRuntimeRole;
GRANT INSERT ON dbo.QuickLogin_Log TO KMTGuardRuntimeRole;
GRANT SELECT ON dbo.System_ExternalSettings TO KMTGuardRuntimeRole;
DENY ALTER, CONTROL, TAKE OWNERSHIP TO KMTGuardRuntimeRole;

IF OBJECT_ID(N'dbo.System_SchemaVersion', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.System_SchemaVersion
    (
        Component sysname NOT NULL CONSTRAINT PK_System_SchemaVersion PRIMARY KEY,
        Version varchar(32) NOT NULL,
        AppliedUtc datetime2(0) NOT NULL
    );
END;
MERGE dbo.System_SchemaVersion AS Target
USING (SELECT N'KMTGuard' AS Component, '3.0.0' AS Version) AS Source
   ON Target.Component=Source.Component
WHEN MATCHED THEN UPDATE SET Version=Source.Version, AppliedUtc=SYSUTCDATETIME()
WHEN NOT MATCHED THEN INSERT(Component,Version,AppliedUtc)
     VALUES(Source.Component,Source.Version,SYSUTCDATETIME());

COMMIT TRANSACTION;
