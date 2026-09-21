USE [KMTGuard];
GO
SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.System_GameServerSettings', N'U') IS NULL
    THROW 51630, 'System_GameServerSettings is missing.', 1;

IF EXISTS
(
    SELECT required.SettingName
    FROM
    (
        VALUES
            ('ShowGmUniqueKillNotice'),
            ('ForceGmVisibleOnSpawn')
    ) AS required(SettingName)
    WHERE
    (
        SELECT COUNT_BIG(*)
        FROM dbo.System_GameServerSettings AS actual
        WHERE actual.SettingName = required.SettingName
    ) <> 1
)
    THROW 51631, 'One or more GM GameServer controls are missing or duplicated.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.System_GameServerSettings
    WHERE SettingName IN ('ShowGmUniqueKillNotice', 'ForceGmVisibleOnSpawn')
      AND LOWER(LTRIM(RTRIM(Value))) NOT IN ('true', 'false', '1', '0')
)
    THROW 51632, 'A GM GameServer control contains an invalid boolean value.', 1;

SELECT N'GM GameServer controls validation passed.' AS Result;
GO
