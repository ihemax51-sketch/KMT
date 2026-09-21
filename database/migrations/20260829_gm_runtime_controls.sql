/*
    Adds customer-controlled GameServer behavior for GM Unique kill notices
    and GM visibility when a character is spawned into the world.

    Both settings default to False to preserve the existing server behavior.
    The GameServer reads these values during startup.
*/

USE [KMTGuard];
GO
SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.System_GameServerSettings', N'U') IS NULL
    THROW 51620, 'dbo.System_GameServerSettings is required.', 1;
GO

IF EXISTS
(
    SELECT SettingName
    FROM dbo.System_GameServerSettings
    WHERE SettingName IN ('ShowGmUniqueKillNotice', 'ForceGmVisibleOnSpawn')
    GROUP BY SettingName
    HAVING COUNT_BIG(*) > 1
)
    THROW 51621, 'Duplicate GM GameServer settings must be resolved before this update.', 1;
GO

IF EXISTS
(
    SELECT 1
    FROM dbo.System_GameServerSettings
    WHERE SettingName IN ('ShowGmUniqueKillNotice', 'ForceGmVisibleOnSpawn')
      AND LOWER(LTRIM(RTRIM(Value))) NOT IN ('true', 'false', '1', '0')
)
    THROW 51622, 'Existing GM GameServer settings must contain True, False, 1, or 0.', 1;
GO

BEGIN TRANSACTION;

CREATE TABLE #DesiredGmGameServerSettings
(
    SettingName varchar(128) NOT NULL PRIMARY KEY,
    Value varchar(128) NOT NULL
);

INSERT #DesiredGmGameServerSettings(SettingName, Value)
VALUES
    ('ShowGmUniqueKillNotice', 'False'),
    ('ForceGmVisibleOnSpawn', 'False');

IF COLUMNPROPERTY(OBJECT_ID(N'dbo.System_GameServerSettings'), N'ID', 'IsIdentity') = 1
BEGIN
    INSERT dbo.System_GameServerSettings(SettingName, Value)
    SELECT desired.SettingName, desired.Value
    FROM #DesiredGmGameServerSettings AS desired
    WHERE NOT EXISTS
    (
        SELECT 1
        FROM dbo.System_GameServerSettings AS actual
        WHERE actual.SettingName = desired.SettingName
    );
END
ELSE
BEGIN
    DECLARE @NextGameServerSettingID int =
        ISNULL((SELECT MAX(ID) FROM dbo.System_GameServerSettings WITH (UPDLOCK, HOLDLOCK)), 0);

    INSERT dbo.System_GameServerSettings(ID, SettingName, Value)
    SELECT @NextGameServerSettingID + ROW_NUMBER() OVER (ORDER BY desired.SettingName),
           desired.SettingName,
           desired.Value
    FROM #DesiredGmGameServerSettings AS desired
    WHERE NOT EXISTS
    (
        SELECT 1
        FROM dbo.System_GameServerSettings AS actual
        WHERE actual.SettingName = desired.SettingName
    );
END;

COMMIT TRANSACTION;
GO

PRINT 'GM GameServer controls are available and default to False.';
GO
