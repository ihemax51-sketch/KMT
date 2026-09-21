USE [KMTGuard];
GO
SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.System_GameServerSettings',N'U') IS NULL
    THROW 51300,'System_GameServerSettings is missing.',1;

IF EXISTS
(
    SELECT SettingName FROM dbo.System_GameServerSettings
    GROUP BY SettingName HAVING COUNT_BIG(*)>1
)
    THROW 51301,'Duplicate GameServer settings remain.',1;

IF NOT EXISTS
(
    SELECT 1 FROM sys.indexes
    WHERE object_id=OBJECT_ID(N'dbo.System_GameServerSettings')
      AND name=N'UX_System_GameServerSettings_SettingName' AND is_unique=1
)
    THROW 51302,'The GameServer setting-name unique index is missing.',1;

IF EXISTS
(
    SELECT 1 FROM dbo.System_Settings
    WHERE SettingName IN
    (
        N'DisableDurability',N'DisableGreenBook',N'EnablePartyMonsterSpawn',
        N'PartyMonsterMinimumMembers',N'PartyMonsterSpawnRate'
    )
)
    THROW 51303,'GameServer-owned settings remain in System_Settings.',1;

IF EXISTS(SELECT 1 FROM dbo.System_ShardSettings WHERE SettingName=N'FixNegativeGuildPoint')
    THROW 51304,'The duplicate ShardManager guild-point setting remains.',1;

IF EXISTS
(
    SELECT 1 FROM dbo.System_GameServerSettings
    WHERE SettingName IN
    (
        'PARTY_MOB_MEMBERS_REQUIRED','PARTY_MOB_SPAWN_PROBABILITY',
        'MIN_PK_LEVEL_FOR_DROP_ITEM','STONE_ASTRAL_VALUE',
        'ACADEMY_DISBAND_PENALTY_TIME'
    )
)
    THROW 51305,'Retired or duplicate GameServer settings remain.',1;

IF EXISTS
(
    SELECT required.SettingName
    FROM
    (
        VALUES
        ('DisableDurability'),('DisableGreenBook'),('EnablePartyMonsterSpawn'),
        ('PartyMonsterMinimumMembers'),('PartyMonsterSpawnRate'),
        ('PENALTY_DROP_LEVEL_MIN'),('MinItemLevelForAstralToTakeEffect'),
        ('ItemLevelForAstralRecovery'),('GUILD_POINTS')
    ) AS required(SettingName)
    WHERE NOT EXISTS
    (
        SELECT 1 FROM dbo.System_GameServerSettings AS actual
        WHERE actual.SettingName=required.SettingName
    )
)
    THROW 51306,'One or more required GameServer settings are missing.',1;

IF EXISTS
(
    SELECT 1 FROM dbo.System_GameServerSettings
    WHERE SettingName IN
    (
        'DisableDurability','DisableGreenBook','EnablePartyMonsterSpawn','GUILD_POINTS'
    ) AND LOWER(Value) NOT IN ('true','false','1','0')
)
    THROW 51307,'A GameServer boolean setting is invalid.',1;

IF EXISTS
(
    SELECT 1 FROM dbo.System_GameServerSettings
    WHERE (SettingName='PartyMonsterMinimumMembers' AND TRY_CONVERT(int,Value) NOT BETWEEN 1 AND 9)
       OR (SettingName='PartyMonsterSpawnRate' AND TRY_CONVERT(int,Value) NOT BETWEEN 0 AND 100)
       OR (SettingName='PENALTY_DROP_LEVEL_MIN' AND TRY_CONVERT(int,Value) NOT BETWEEN 1 AND 255)
       OR (SettingName IN ('MinItemLevelForAstralToTakeEffect','ItemLevelForAstralRecovery')
           AND TRY_CONVERT(int,Value) NOT BETWEEN 0 AND 255)
)
    THROW 51308,'A consolidated GameServer numeric setting is outside its supported range.',1;

SELECT N'Gameserver Patch dashboard database validation passed.' AS Result;
GO
