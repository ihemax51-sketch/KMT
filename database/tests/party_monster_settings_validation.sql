USE [KMTGuard];
GO

SET NOCOUNT ON;
GO

IF OBJECT_ID(N'dbo.System_Settings', N'U') IS NULL
    THROW 51000, 'KMTGuard.dbo.System_Settings was not found.', 1;

IF
(
    SELECT COUNT_BIG(*)
    FROM dbo.System_Settings
    WHERE SettingName IN
    (
        N'EnablePartyMonsterSpawn',
        N'PartyMonsterMinimumMembers',
        N'PartyMonsterSpawnRate'
    )
) <> 3
    THROW 51001, 'Expected exactly three Party Monster setting rows.', 1;

IF EXISTS
(
    SELECT SettingName
    FROM dbo.System_Settings
    WHERE SettingName IN
    (
        N'EnablePartyMonsterSpawn',
        N'PartyMonsterMinimumMembers',
        N'PartyMonsterSpawnRate'
    )
    GROUP BY SettingName
    HAVING COUNT_BIG(*) <> 1
)
    THROW 51002, 'Each Party Monster setting must exist exactly once.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.System_Settings
    WHERE SettingName = N'EnablePartyMonsterSpawn'
      AND Value NOT IN (N'True', N'False')
)
    THROW 51003, 'EnablePartyMonsterSpawn must be True or False.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.System_Settings
    WHERE SettingName = N'PartyMonsterMinimumMembers'
      AND
      (
          TRY_CONVERT(INT, Value) IS NULL
          OR TRY_CONVERT(INT, Value) NOT BETWEEN 1 AND 9
      )
)
    THROW 51004, 'PartyMonsterMinimumMembers must be an integer from 1 to 9.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.System_Settings
    WHERE SettingName = N'PartyMonsterSpawnRate'
      AND
      (
          TRY_CONVERT(INT, Value) IS NULL
          OR TRY_CONVERT(INT, Value) NOT BETWEEN 0 AND 100
      )
)
    THROW 51005, 'PartyMonsterSpawnRate must be an integer from 0 to 100.', 1;

SELECT SettingName, Value
FROM dbo.System_Settings
WHERE SettingName IN
(
    N'EnablePartyMonsterSpawn',
    N'PartyMonsterMinimumMembers',
    N'PartyMonsterSpawnRate'
)
ORDER BY SettingName;

PRINT 'Party Monster settings validation passed.';
GO
