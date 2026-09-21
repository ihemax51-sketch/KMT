USE KMTGuard;
GO

SET NOCOUNT ON;
SET XACT_ABORT ON;

IF (SELECT COUNT_BIG(*) FROM dbo.Achievement_List WHERE Service = 1) = 0
    THROW 51100, 'No active achievement references were found.', 1;

IF EXISTS
(
    SELECT 1
    FROM SRO_VT_SHARD.dbo._Char AS character
    CROSS JOIN dbo.Achievement_List AS reference
    WHERE character.CharID > 0
      AND reference.Service = 1
      AND NOT EXISTS
      (
          SELECT 1
          FROM dbo.Achievement_Players AS player
          WHERE player.CharID = character.CharID
            AND player.RefAchievementID = reference.ID
      )
)
    THROW 51101, 'At least one character is missing an active achievement row.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.Achievement_Players AS player
    INNER JOIN dbo.Achievement_List AS reference
        ON reference.ID = player.RefAchievementID
       AND reference.Service = 1
    INNER JOIN dbo.Achievement_Conditions AS referenceCondition
        ON referenceCondition.RefAchievementID = reference.ID
    WHERE NOT EXISTS
    (
        SELECT 1
        FROM dbo.Achievement_PlayerConditions AS playerCondition
        WHERE playerCondition.CharID = player.CharID
          AND playerCondition.AchievementID = player.ID
          AND playerCondition.RefAchievementConditionID = referenceCondition.ID
    )
)
    THROW 51102, 'At least one active player condition is missing or linked to the wrong parent.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.Achievement_PlayerConditions AS playerCondition
    LEFT JOIN dbo.Achievement_Players AS player
        ON player.ID = playerCondition.AchievementID
       AND player.CharID = playerCondition.CharID
    LEFT JOIN dbo.Achievement_Conditions AS referenceCondition
        ON referenceCondition.ID = playerCondition.RefAchievementConditionID
       AND referenceCondition.RefAchievementID = player.RefAchievementID
    WHERE player.ID IS NULL OR referenceCondition.ID IS NULL
)
    THROW 51103, 'Achievement parent/reference integrity validation failed.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.Achievement_PlayerConditions AS playerCondition
    INNER JOIN dbo.Achievement_Conditions AS referenceCondition
        ON referenceCondition.ID = playerCondition.RefAchievementConditionID
    WHERE playerCondition.ProgressCount < 0
       OR playerCondition.ProgressCount > referenceCondition.CompleteCount
)
    THROW 51104, 'An achievement progress value is outside its valid range.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.Achievement_Players AS player
    CROSS APPLY
    (
        SELECT CONVERT(tinyint,
            CASE
                WHEN COUNT_BIG(*) > 0
                 AND SUM(CASE WHEN playerCondition.ProgressCount >= referenceCondition.CompleteCount
                              THEN 0 ELSE 1 END) = 0
                    THEN 1
                ELSE 0
            END) AS ExpectedState
        FROM dbo.Achievement_Conditions AS referenceCondition
        LEFT JOIN dbo.Achievement_PlayerConditions AS playerCondition
            ON playerCondition.AchievementID = player.ID
           AND playerCondition.RefAchievementConditionID = referenceCondition.ID
        WHERE referenceCondition.RefAchievementID = player.RefAchievementID
    ) AS calculated
    WHERE player.State <> calculated.ExpectedState
)
    THROW 51105, 'An achievement completion state does not match its conditions.', 1;

IF EXISTS
(
    SELECT 1
    FROM sys.foreign_keys
    WHERE name IN
    (
        N'FK_Achievement_Conditions_List',
        N'FK_Achievement_Players_List',
        N'FK_Achievement_PlayerConditions_Player',
        N'FK_Achievement_PlayerConditions_Condition'
    )
      AND (is_disabled = 1 OR is_not_trusted = 1)
)
    THROW 51106, 'At least one achievement foreign key is disabled or untrusted.', 1;

IF
(
    SELECT COUNT_BIG(*)
    FROM sys.foreign_keys
    WHERE name IN
    (
        N'FK_Achievement_Conditions_List',
        N'FK_Achievement_Players_List',
        N'FK_Achievement_PlayerConditions_Player',
        N'FK_Achievement_PlayerConditions_Condition'
    )
) <> 4
    THROW 51107, 'One or more achievement foreign keys are missing.', 1;

DECLARE @CharID int;
DECLARE @RefAchievementID int;
DECLARE @RefAchievementConditionID int;
DECLARE @PlayerAchievementID int;
DECLARE @CompleteCount bigint;
DECLARE @ReturnCode int;
DECLARE @PlayerCountBefore bigint;
DECLARE @ConditionCountBefore bigint;

SELECT TOP (1)
       @CharID = player.CharID,
       @RefAchievementID = player.RefAchievementID,
       @PlayerAchievementID = player.ID,
       @RefAchievementConditionID = referenceCondition.ID,
       @CompleteCount = referenceCondition.CompleteCount
FROM dbo.Achievement_Players AS player
INNER JOIN dbo.Achievement_List AS reference
    ON reference.ID = player.RefAchievementID
   AND reference.Service = 1
INNER JOIN dbo.Achievement_Conditions AS referenceCondition
    ON referenceCondition.RefAchievementID = reference.ID
WHERE player.CharID > 0
ORDER BY player.CharID, player.RefAchievementID, referenceCondition.ID;

IF @PlayerAchievementID IS NULL
    THROW 51108, 'No row is available for the transactional achievement smoke test.', 1;

BEGIN TRANSACTION;

SELECT @PlayerCountBefore = COUNT_BIG(*)
FROM dbo.Achievement_Players
WHERE CharID = @CharID;

SELECT @ConditionCountBefore = COUNT_BIG(*)
FROM dbo.Achievement_PlayerConditions
WHERE CharID = @CharID;

EXEC @ReturnCode = dbo.Achievement_AddPlayer @CharID = @CharID;
IF @ReturnCode <> 1
    THROW 51109, 'Achievement_AddPlayer returned an unexpected status.', 1;

EXEC @ReturnCode = dbo.Achievement_AddPlayer @CharID = @CharID;
IF @ReturnCode <> 1
    THROW 51110, 'Achievement_AddPlayer is not idempotent.', 1;

IF @PlayerCountBefore <>
   (SELECT COUNT_BIG(*) FROM dbo.Achievement_Players WHERE CharID = @CharID)
    THROW 51111, 'Achievement_AddPlayer duplicated player rows.', 1;

IF @ConditionCountBefore <>
   (SELECT COUNT_BIG(*) FROM dbo.Achievement_PlayerConditions WHERE CharID = @CharID)
    THROW 51112, 'Achievement_AddPlayer duplicated condition rows.', 1;

UPDATE dbo.Achievement_Players
SET State = 0
WHERE ID = @PlayerAchievementID;

UPDATE dbo.Achievement_PlayerConditions
SET ProgressCount = 0
WHERE AchievementID = @PlayerAchievementID;

EXEC @ReturnCode = dbo.Achievement_Update
    @CharID = @CharID,
    @RefAchievementID = @RefAchievementID,
    @RefAchievementConditionID = @RefAchievementConditionID,
    @Progress = 9223372036854775807;

IF @ReturnCode <> 1
    THROW 51113, 'Achievement_Update returned an unexpected status.', 1;

IF
(
    SELECT ProgressCount
    FROM dbo.Achievement_PlayerConditions
    WHERE AchievementID = @PlayerAchievementID
      AND RefAchievementConditionID = @RefAchievementConditionID
) <> @CompleteCount
    THROW 51114, 'Achievement_Update did not cap progress at CompleteCount.', 1;

IF NOT EXISTS
(
    SELECT 1
    FROM dbo.Command_FilterQueue
    WHERE CommandID = 29
      AND TRY_CONVERT(int, Data1) = @CharID
      AND TRY_CONVERT(int, Data2) = @RefAchievementID
      AND TRY_CONVERT(int, Data3) = @RefAchievementConditionID
)
    THROW 51115, 'Achievement_Update did not queue its runtime update.', 1;

ROLLBACK TRANSACTION;

SELECT
    (SELECT COUNT_BIG(*) FROM dbo.Achievement_List WHERE Service = 1) AS ActiveAchievements,
    (SELECT COUNT_BIG(*) FROM dbo.Achievement_Conditions AS condition
     INNER JOIN dbo.Achievement_List AS reference
        ON reference.ID = condition.RefAchievementID
       AND reference.Service = 1) AS ActiveConditions,
    (SELECT COUNT_BIG(*) FROM dbo.Achievement_Players) AS PlayerAchievements,
    (SELECT COUNT_BIG(*) FROM dbo.Achievement_PlayerConditions) AS PlayerConditions,
    N'PASS' AS ValidationResult;
GO
