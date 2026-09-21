USE [KMTGuard];
GO

SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.System_Settings', N'U') IS NULL
    THROW 51000, 'KMTGuard.dbo.System_Settings was not found.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.System_Settings
    WHERE SettingName = N'LogDB'
)
BEGIN
    UPDATE dbo.System_Settings
    SET Value = CASE
                    WHEN NULLIF(LTRIM(RTRIM(CONVERT(NVARCHAR(512), Value))), N'') IS NULL
                        THEN N'SRO_VT_SHARDLOG'
                    ELSE Value
                END,
        Category = N'Core.Database',
        DisplayOrder = 10,
        Description = N'Shard log database that contains _LogEventItem drop records.'
    WHERE SettingName = N'LogDB';
END
ELSE
BEGIN
    INSERT INTO dbo.System_Settings
    (
        SettingName,
        Value,
        Category,
        DisplayOrder,
        Description
    )
    VALUES
    (
        N'LogDB',
        N'SRO_VT_SHARDLOG',
        N'Core.Database',
        10,
        N'Shard log database that contains _LogEventItem drop records.'
    );
END;
GO

CREATE OR ALTER VIEW dbo.vw_Settings_InvalidValues
AS
WITH typed AS
(
    SELECT
        SettingName,
        Value,
        CASE
            WHEN SettingName IN
            (
                N'AccountDB', N'ShardDB', N'LogDB', N'FacebookURL', N'DiscordURL',
                N'WebsiteURL', N'ServerName', N'CaptchaValue'
            ) THEN N'string'
            WHEN SettingName IN
            (
                N'MasteryLimit', N'ServerMaxLevel', N'FakePlayerCount', N'HWID_LIMIT',
                N'HWID_JOB_LIMIT', N'AlchemyItemLinkMinLevel', N'ReverseDelay', N'MaxPlus',
                N'AutoAttackMaxLevel', N'StallDelay', N'StallLevel', N'ExchangeDelay',
                N'ExchangeLevel', N'GuildInviteDelay', N'UnionInviteDelay', N'GlobalDelay',
                N'GlobalLevel', N'LiveItemDelay', N'TradePetSpawnDelay', N'RestartDelay',
                N'ExitDelay', N'ItemTranslationPayment', N'ItemTranslationPrice',
                N'SHOW_CHAR_INFO_DELAY', N'IPLimit', N'MaxPlusDevil', N'LuckySpinPrice',
                N'TradeSellCaptchaTimeoutSeconds', N'TradeSellCaptchaMaxAttempts',
                N'OfflineStallMaxHours', N'OfflineStallConfirmSeconds'
            ) THEN N'int'
            ELSE N'bool'
        END AS ExpectedType
    FROM dbo.System_Settings
)
SELECT SettingName, Value, ExpectedType
FROM typed
WHERE
    (ExpectedType = N'bool' AND Value NOT IN (N'True', N'False'))
    OR (ExpectedType = N'int' AND TRY_CONVERT(INT, Value) IS NULL);
GO

PRINT 'LogDB setting added. Drop Logs can now use a configurable shard log database.';
GO
