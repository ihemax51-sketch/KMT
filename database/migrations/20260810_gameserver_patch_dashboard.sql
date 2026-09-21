/*
    Consolidates every customer-editable GameServer setting in
    dbo.System_GameServerSettings.

    The GameServer reads this catalog once during startup. GUILD_POINTS is
    consumed by the ShardManager guild-point protection and therefore also
    requires a ShardManager restart.
*/

USE [KMTGuard];
GO
SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.System_Settings', N'U') IS NULL
    THROW 51200, 'dbo.System_Settings is required.', 1;
IF OBJECT_ID(N'dbo.System_ShardSettings', N'U') IS NULL
    THROW 51201, 'dbo.System_ShardSettings is required.', 1;

IF OBJECT_ID(N'dbo.System_GameServerSettings', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.System_GameServerSettings
    (
        ID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_System_GameServerSettings PRIMARY KEY,
        SettingName varchar(128) NOT NULL,
        Value varchar(128) NOT NULL
    );
END;
GO

IF COL_LENGTH(N'dbo.System_GameServerSettings', N'ID') IS NULL OR
   COL_LENGTH(N'dbo.System_GameServerSettings', N'SettingName') IS NULL OR
   COL_LENGTH(N'dbo.System_GameServerSettings', N'Value') IS NULL
    THROW 51202, 'dbo.System_GameServerSettings has an incompatible schema.', 1;
GO

IF EXISTS
(
    SELECT SettingName
    FROM dbo.System_GameServerSettings
    GROUP BY SettingName
    HAVING COUNT_BIG(*) > 1 AND COUNT_BIG(DISTINCT Value) > 1
)
    THROW 51203, 'Conflicting duplicate GameServer setting values must be resolved before this update.', 1;
GO

;WITH duplicates AS
(
    SELECT ID, ROW_NUMBER() OVER (PARTITION BY SettingName ORDER BY ID) AS DuplicateOrder
    FROM dbo.System_GameServerSettings
)
DELETE FROM duplicates WHERE DuplicateOrder > 1;
GO

BEGIN TRANSACTION;

CREATE TABLE #DesiredGameServerSettings
(
    SettingName varchar(128) NOT NULL PRIMARY KEY,
    Value varchar(128) NOT NULL
);

INSERT #DesiredGameServerSettings(SettingName, Value)
VALUES
(
    'DisableDurability',
    COALESCE(
        (SELECT TOP (1) Value FROM dbo.System_GameServerSettings WHERE SettingName='DisableDurability'),
        (SELECT TOP (1) CONVERT(varchar(128),Value) FROM dbo.System_Settings WHERE SettingName=N'DisableDurability'),
        'False')
),
(
    'DisableGreenBook',
    COALESCE(
        (SELECT TOP (1) Value FROM dbo.System_GameServerSettings WHERE SettingName='DisableGreenBook'),
        (SELECT TOP (1) CONVERT(varchar(128),Value) FROM dbo.System_Settings WHERE SettingName=N'DisableGreenBook'),
        'False')
),
(
    'EnablePartyMonsterSpawn',
    COALESCE(
        (SELECT TOP (1) Value FROM dbo.System_GameServerSettings WHERE SettingName='EnablePartyMonsterSpawn'),
        (SELECT TOP (1) CONVERT(varchar(128),Value) FROM dbo.System_Settings WHERE SettingName=N'EnablePartyMonsterSpawn'),
        'True')
),
(
    'PartyMonsterMinimumMembers',
    COALESCE(
        (SELECT TOP (1) Value FROM dbo.System_GameServerSettings WHERE SettingName='PartyMonsterMinimumMembers'),
        (SELECT TOP (1) CONVERT(varchar(128),Value) FROM dbo.System_Settings WHERE SettingName=N'PartyMonsterMinimumMembers'),
        (SELECT TOP (1) Value FROM dbo.System_GameServerSettings WHERE SettingName='PARTY_MOB_MEMBERS_REQUIRED'),
        '2')
),
(
    'PartyMonsterSpawnRate',
    COALESCE(
        (SELECT TOP (1) Value FROM dbo.System_GameServerSettings WHERE SettingName='PartyMonsterSpawnRate'),
        (SELECT TOP (1) CONVERT(varchar(128),Value) FROM dbo.System_Settings WHERE SettingName=N'PartyMonsterSpawnRate'),
        (SELECT TOP (1) Value FROM dbo.System_GameServerSettings WHERE SettingName='PARTY_MOB_SPAWN_PROBABILITY'),
        '50')
),
(
    'PENALTY_DROP_LEVEL_MIN',
    COALESCE(
        (SELECT TOP (1) Value FROM dbo.System_GameServerSettings WHERE SettingName='PENALTY_DROP_LEVEL_MIN'),
        (SELECT TOP (1) Value FROM dbo.System_GameServerSettings WHERE SettingName='MIN_PK_LEVEL_FOR_DROP_ITEM'),
        '10')
),
(
    'MinItemLevelForAstralToTakeEffect',
    COALESCE(
        (SELECT TOP (1) Value FROM dbo.System_GameServerSettings WHERE SettingName='MinItemLevelForAstralToTakeEffect'),
        (SELECT TOP (1) Value FROM dbo.System_GameServerSettings WHERE SettingName='STONE_ASTRAL_VALUE'),
        '4')
),
(
    'ItemLevelForAstralRecovery',
    COALESCE(
        (SELECT TOP (1) Value FROM dbo.System_GameServerSettings WHERE SettingName='ItemLevelForAstralRecovery'),
        (SELECT TOP (1) Value FROM dbo.System_GameServerSettings WHERE SettingName='STONE_ASTRAL_VALUE'),
        '4')
),
(
    'GUILD_POINTS',
    COALESCE(
        (SELECT TOP (1) Value FROM dbo.System_GameServerSettings WHERE SettingName='GUILD_POINTS'),
        (SELECT TOP (1) CONVERT(varchar(128),Value) FROM dbo.System_ShardSettings WHERE SettingName=N'FixNegativeGuildPoint'),
        'True')
);

UPDATE target
SET target.Value=source.Value
FROM dbo.System_GameServerSettings AS target
INNER JOIN #DesiredGameServerSettings AS source ON source.SettingName=target.SettingName;

IF COLUMNPROPERTY(OBJECT_ID(N'dbo.System_GameServerSettings'), N'ID', 'IsIdentity') = 1
BEGIN
    INSERT dbo.System_GameServerSettings(SettingName, Value)
    SELECT source.SettingName, source.Value
    FROM #DesiredGameServerSettings AS source
    WHERE NOT EXISTS
    (
        SELECT 1 FROM dbo.System_GameServerSettings AS target
        WHERE target.SettingName=source.SettingName
    );
END
ELSE
BEGIN
    DECLARE @NextGameServerSettingID int=
        ISNULL((SELECT MAX(ID) FROM dbo.System_GameServerSettings WITH (UPDLOCK,HOLDLOCK)),0);
    INSERT dbo.System_GameServerSettings(ID,SettingName,Value)
    SELECT @NextGameServerSettingID+ROW_NUMBER() OVER(ORDER BY source.SettingName),
           source.SettingName,source.Value
    FROM #DesiredGameServerSettings AS source
    WHERE NOT EXISTS
    (
        SELECT 1 FROM dbo.System_GameServerSettings AS target
        WHERE target.SettingName=source.SettingName
    );
END;

DELETE dbo.System_Settings
WHERE SettingName IN
(
    N'DisableDurability',N'DisableGreenBook',N'EnablePartyMonsterSpawn',
    N'PartyMonsterMinimumMembers',N'PartyMonsterSpawnRate'
);

DELETE dbo.System_ShardSettings WHERE SettingName=N'FixNegativeGuildPoint';

DELETE dbo.System_GameServerSettings
WHERE SettingName IN
(
    'PARTY_MOB_MEMBERS_REQUIRED','PARTY_MOB_SPAWN_PROBABILITY',
    'MIN_PK_LEVEL_FOR_DROP_ITEM','STONE_ASTRAL_VALUE',
    'ACADEMY_DISBAND_PENALTY_TIME'
);

COMMIT TRANSACTION;
GO

IF NOT EXISTS
(
    SELECT 1 FROM sys.indexes
    WHERE object_id=OBJECT_ID(N'dbo.System_GameServerSettings')
      AND name=N'UX_System_GameServerSettings_SettingName'
)
    CREATE UNIQUE INDEX UX_System_GameServerSettings_SettingName
        ON dbo.System_GameServerSettings(SettingName);
GO

PRINT 'GameServer settings were consolidated for the Gameserver Patch dashboard.';
GO
