USE [KMTGuard];
GO

SET NOCOUNT ON;
GO

IF OBJECT_ID(N'dbo.System_GameServerSettings', N'U') IS NULL
    THROW 51660, 'dbo.System_GameServerSettings was not found.', 1;

IF
(
    SELECT COUNT_BIG(*)
    FROM dbo.System_GameServerSettings
    WHERE SettingName = 'DisableOriginalTradeGold'
) <> 1
    THROW 51661, 'DisableOriginalTradeGold must exist exactly once.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.System_GameServerSettings
    WHERE SettingName = 'DisableOriginalTradeGold'
      AND LOWER(Value) NOT IN ('true', 'false', '1', '0')
)
    THROW 51662, 'DisableOriginalTradeGold must contain a valid boolean value.', 1;

SELECT SettingName, Value
FROM dbo.System_GameServerSettings
WHERE SettingName = 'DisableOriginalTradeGold';
GO
