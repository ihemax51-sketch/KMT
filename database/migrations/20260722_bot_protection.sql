USE [KMTGuard];
GO

SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

BEGIN TRANSACTION;

IF OBJECT_ID(N'dbo.System_Settings', N'U') IS NULL
    THROW 51000, 'KMTGuard.dbo.System_Settings was not found.', 1;

MERGE dbo.System_Settings AS target
USING
(
    VALUES
        (N'AllowBotLogin', N'True'),
        (N'AllowBotTrade', N'True'),
        (N'BotProtectionLogEnabled', N'True')
) AS source (SettingName, Value)
ON target.SettingName = source.SettingName
WHEN NOT MATCHED THEN
    INSERT (SettingName, Value) VALUES (source.SettingName, source.Value);

IF COL_LENGTH(N'dbo.System_Settings', N'Category') IS NOT NULL
   AND COL_LENGTH(N'dbo.System_Settings', N'DisplayOrder') IS NOT NULL
   AND COL_LENGTH(N'dbo.System_Settings', N'Description') IS NOT NULL
BEGIN
    EXEC sys.sp_executesql N'
        UPDATE dbo.System_Settings
        SET Category = N''Security.BotProtection'',
            DisplayOrder = 92,
            Description = CASE SettingName
                WHEN N''AllowBotLogin'' THEN N''Allows verified managed clientless/bot accounts to enter the game. False also requires official DLL proof at Gateway login.''
                WHEN N''AllowBotTrade'' THEN N''Allows bot sessions to use exchange, trade goods and trade transports.''
                WHEN N''BotProtectionLogEnabled'' THEN N''Writes bot protection decisions to Security_BotProtectionLog.''
            END
        WHERE SettingName IN (N''AllowBotLogin'', N''AllowBotTrade'', N''BotProtectionLogEnabled'');';
END;

IF OBJECT_ID(N'dbo.Security_RegionFeatures', N'U') IS NULL
    THROW 51001, 'KMTGuard.dbo.Security_RegionFeatures was not found.', 1;

IF COL_LENGTH(N'dbo.Security_RegionFeatures', N'NoBot_Action') IS NULL
BEGIN
    ALTER TABLE dbo.Security_RegionFeatures
        ADD NoBot_Action TINYINT NOT NULL
            CONSTRAINT DF_Security_RegionFeatures_NoBotAction DEFAULT (1);
END;

IF COL_LENGTH(N'dbo.Security_RegionFeatures', N'NoBot_WarningSeconds') IS NULL
BEGIN
    ALTER TABLE dbo.Security_RegionFeatures
        ADD NoBot_WarningSeconds INT NOT NULL
            CONSTRAINT DF_Security_RegionFeatures_NoBotWarning DEFAULT (30);
END;

IF COL_LENGTH(N'dbo.Security_RegionFeatures', N'NoBot_LogOnly') IS NULL
BEGIN
    ALTER TABLE dbo.Security_RegionFeatures
        ADD NoBot_LogOnly BIT NOT NULL
            CONSTRAINT DF_Security_RegionFeatures_NoBotLogOnly DEFAULT (0);
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.check_constraints
    WHERE parent_object_id = OBJECT_ID(N'dbo.Security_RegionFeatures')
      AND name = N'CK_Security_RegionFeatures_NoBotAction'
)
BEGIN
    EXEC sys.sp_executesql N'
        ALTER TABLE dbo.Security_RegionFeatures WITH CHECK
            ADD CONSTRAINT CK_Security_RegionFeatures_NoBotAction CHECK (NoBot_Action IN (0, 1));';
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.check_constraints
    WHERE parent_object_id = OBJECT_ID(N'dbo.Security_RegionFeatures')
      AND name = N'CK_Security_RegionFeatures_NoBotTimes'
)
BEGIN
    EXEC sys.sp_executesql N'
        ALTER TABLE dbo.Security_RegionFeatures WITH CHECK
            ADD CONSTRAINT CK_Security_RegionFeatures_NoBotTimes
                CHECK (NoBot_TimeSeconds >= 0 AND NoBot_WarningSeconds >= 0);';
END;

IF OBJECT_ID(N'dbo.Security_BotProtectionLog', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Security_BotProtectionLog
    (
        ID BIGINT IDENTITY(1,1) NOT NULL CONSTRAINT PK_Security_BotProtectionLog PRIMARY KEY,
        CreatedAtUtc DATETIME2(0) NOT NULL
            CONSTRAINT DF_Security_BotProtectionLog_CreatedAt DEFAULT (SYSUTCDATETIME()),
        EventType VARCHAR(40) NOT NULL,
        ActionTaken VARCHAR(24) NOT NULL,
        AccountName VARCHAR(128) NULL,
        CharID INT NULL,
        CharName VARCHAR(64) NULL,
        Hwid VARCHAR(128) NULL,
        ClientIP VARCHAR(64) NULL,
        RegionID INT NULL,
        Details NVARCHAR(512) NULL
    );
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Security_BotProtectionLog')
      AND name = N'IX_Security_BotProtectionLog_CreatedAtUtc'
)
BEGIN
    CREATE INDEX IX_Security_BotProtectionLog_CreatedAtUtc
        ON dbo.Security_BotProtectionLog (CreatedAtUtc DESC)
        INCLUDE (EventType, ActionTaken, AccountName, CharName, RegionID);
END;

COMMIT TRANSACTION;
GO

/*
    Global examples:
      UPDATE dbo.System_Settings SET Value = N'False' WHERE SettingName = N'AllowBotLogin';
      UPDATE dbo.System_Settings SET Value = N'False' WHERE SettingName = N'AllowBotTrade';

    Region examples:
      -- Immediate disconnect for bots entering Region 25000.
      UPDATE dbo.Security_RegionFeatures
      SET Enable_NoBot = 1, NoBot_TimeSeconds = 0, NoBot_Action = 1,
          NoBot_WarningSeconds = 0, NoBot_LogOnly = 0
      WHERE RegionID = 25000;

      -- Allow bots for 10 minutes, warn during the last 30 seconds, then return to town.
      UPDATE dbo.Security_RegionFeatures
      SET Enable_NoBot = 1, NoBot_TimeSeconds = 600, NoBot_Action = 0,
          NoBot_WarningSeconds = 30, NoBot_LogOnly = 0
      WHERE RegionID = 25000;
*/
