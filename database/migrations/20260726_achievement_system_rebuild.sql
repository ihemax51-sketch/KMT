USE [KMTGuard];
GO

SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.Achievement_List', N'U') IS NULL
    THROW 51000, 'dbo.Achievement_List is required.', 1;
IF OBJECT_ID(N'dbo.Achievement_Conditions', N'U') IS NULL
    THROW 51000, 'dbo.Achievement_Conditions is required.', 1;
IF OBJECT_ID(N'dbo.Achievement_Players', N'U') IS NULL
    THROW 51000, 'dbo.Achievement_Players is required.', 1;
IF OBJECT_ID(N'dbo.Achievement_PlayerConditions', N'U') IS NULL
    THROW 51000, 'dbo.Achievement_PlayerConditions is required.', 1;
IF DB_ID(N'SRO_VT_SHARD') IS NULL
    THROW 51000, 'SRO_VT_SHARD is required by the achievement system.', 1;
GO

BEGIN TRANSACTION;

/* Normalize nullable reference data before enforcing the runtime contract. */
UPDATE dbo.Achievement_List
SET RewardType = ISNULL(RewardType, 0),
    RewardTagID = ISNULL(RewardTagID, 0),
    RewardSkillPoint = ISNULL(RewardSkillPoint, 0),
    RewardGold = ISNULL(RewardGold, 0);

UPDATE dbo.Achievement_Conditions
SET Name = ISNULL(NULLIF(LTRIM(RTRIM(Name)), ''), 'Achievement requirement'),
    Type = ISNULL(Type, 0),
    CompleteCount = CASE WHEN CompleteCount <= 0 THEN 1 ELSE CompleteCount END;

ALTER TABLE dbo.Achievement_List ALTER COLUMN RewardType tinyint NOT NULL;
ALTER TABLE dbo.Achievement_List ALTER COLUMN RewardTagID tinyint NOT NULL;
ALTER TABLE dbo.Achievement_List ALTER COLUMN RewardSkillPoint int NOT NULL;
ALTER TABLE dbo.Achievement_List ALTER COLUMN RewardGold bigint NOT NULL;
ALTER TABLE dbo.Achievement_Conditions ALTER COLUMN Name varchar(200) NOT NULL;
ALTER TABLE dbo.Achievement_Conditions ALTER COLUMN Type tinyint NOT NULL;

/*
Archive invalid rows inside the same database before removing them. The tables
are intentionally persistent so a customer can inspect or restore a row after
the migration without restoring the complete database backup.
*/
IF OBJECT_ID(N'dbo.Achievement_RepairArchive_Players', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Achievement_RepairArchive_Players
    (
        ArchivedAtUtc datetime2(3) NOT NULL,
        Reason varchar(64) NOT NULL,
        ID int NOT NULL,
        CharID int NOT NULL,
        RefAchievementID int NOT NULL,
        State tinyint NOT NULL
    );
END;

IF OBJECT_ID(N'dbo.Achievement_RepairArchive_PlayerConditions', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Achievement_RepairArchive_PlayerConditions
    (
        ArchivedAtUtc datetime2(3) NOT NULL,
        Reason varchar(64) NOT NULL,
        ID int NOT NULL,
        CharID int NOT NULL,
        AchievementID int NOT NULL,
        RefAchievementConditionID int NOT NULL,
        ProgressCount bigint NOT NULL
    );
END;

INSERT dbo.Achievement_RepairArchive_PlayerConditions
       (ArchivedAtUtc, Reason, ID, CharID, AchievementID,
        RefAchievementConditionID, ProgressCount)
SELECT SYSUTCDATETIME(), 'Missing character or reference', pc.ID, pc.CharID,
       pc.AchievementID, pc.RefAchievementConditionID, pc.ProgressCount
FROM dbo.Achievement_PlayerConditions AS pc
LEFT JOIN SRO_VT_SHARD.dbo._Char AS character ON character.CharID = pc.CharID
LEFT JOIN dbo.Achievement_Conditions AS referenceCondition
    ON referenceCondition.ID = pc.RefAchievementConditionID
WHERE pc.CharID <= 0
   OR character.CharID IS NULL
   OR referenceCondition.ID IS NULL;

DELETE pc
FROM dbo.Achievement_PlayerConditions AS pc
LEFT JOIN SRO_VT_SHARD.dbo._Char AS character ON character.CharID = pc.CharID
LEFT JOIN dbo.Achievement_Conditions AS referenceCondition
    ON referenceCondition.ID = pc.RefAchievementConditionID
WHERE pc.CharID <= 0
   OR character.CharID IS NULL
   OR referenceCondition.ID IS NULL;

INSERT dbo.Achievement_RepairArchive_Players
       (ArchivedAtUtc, Reason, ID, CharID, RefAchievementID, State)
SELECT SYSUTCDATETIME(), 'Missing character or reference', player.ID,
       player.CharID, player.RefAchievementID, player.State
FROM dbo.Achievement_Players AS player
LEFT JOIN SRO_VT_SHARD.dbo._Char AS character ON character.CharID = player.CharID
LEFT JOIN dbo.Achievement_List AS reference
    ON reference.ID = player.RefAchievementID
WHERE player.CharID <= 0
   OR character.CharID IS NULL
   OR reference.ID IS NULL;

DELETE player
FROM dbo.Achievement_Players AS player
LEFT JOIN SRO_VT_SHARD.dbo._Char AS character ON character.CharID = player.CharID
LEFT JOIN dbo.Achievement_List AS reference
    ON reference.ID = player.RefAchievementID
WHERE player.CharID <= 0
   OR character.CharID IS NULL
   OR reference.ID IS NULL;

/*
    Some legacy rows stored the reference-achievement ID directly in
    PlayerConditions.AchievementID without ever creating the corresponding
    player row. Preserve their valid progress by restoring that parent first.
    This also covers disabled historical achievements without exposing them
    through the active runtime.
*/
INSERT dbo.Achievement_Players (CharID, RefAchievementID, State)
SELECT playerCondition.CharID, referenceCondition.RefAchievementID, 0
FROM dbo.Achievement_PlayerConditions AS playerCondition
INNER JOIN dbo.Achievement_Conditions AS referenceCondition
    ON referenceCondition.ID = playerCondition.RefAchievementConditionID
WHERE playerCondition.CharID > 0
  AND NOT EXISTS
(
    SELECT 1
    FROM dbo.Achievement_Players AS player WITH (UPDLOCK, HOLDLOCK)
    WHERE player.CharID = playerCondition.CharID
      AND player.RefAchievementID = referenceCondition.RefAchievementID
)
GROUP BY playerCondition.CharID, referenceCondition.RefAchievementID;

/* Backfill every existing character with every active achievement. */
INSERT dbo.Achievement_Players (CharID, RefAchievementID, State)
SELECT character.CharID, reference.ID, 0
FROM SRO_VT_SHARD.dbo._Char AS character
CROSS JOIN dbo.Achievement_List AS reference
WHERE character.CharID > 0
  AND reference.Service = 1
  AND NOT EXISTS
  (
      SELECT 1
      FROM dbo.Achievement_Players AS player WITH (UPDLOCK, HOLDLOCK)
      WHERE player.CharID = character.CharID
        AND player.RefAchievementID = reference.ID
  );

/* Insert every missing condition and use the player-row identity consistently. */
INSERT dbo.Achievement_PlayerConditions
       (CharID, AchievementID, RefAchievementConditionID, ProgressCount)
SELECT player.CharID, player.ID, referenceCondition.ID, 0
FROM dbo.Achievement_Players AS player
INNER JOIN dbo.Achievement_Conditions AS referenceCondition
    ON referenceCondition.RefAchievementID = player.RefAchievementID
WHERE NOT EXISTS
(
    SELECT 1
    FROM dbo.Achievement_PlayerConditions AS playerCondition WITH (UPDLOCK, HOLDLOCK)
    WHERE playerCondition.CharID = player.CharID
      AND playerCondition.RefAchievementConditionID = referenceCondition.ID
);

/* Repair the mixed legacy/new meaning of AchievementID. */
UPDATE playerCondition
SET AchievementID = player.ID
FROM dbo.Achievement_PlayerConditions AS playerCondition
INNER JOIN dbo.Achievement_Conditions AS referenceCondition
    ON referenceCondition.ID = playerCondition.RefAchievementConditionID
INNER JOIN dbo.Achievement_Players AS player
    ON player.CharID = playerCondition.CharID
   AND player.RefAchievementID = referenceCondition.RefAchievementID
WHERE playerCondition.AchievementID <> player.ID;

/* Clamp corrupted/test values and recompute one authoritative state. */
UPDATE playerCondition
SET ProgressCount =
    CASE
        WHEN playerCondition.ProgressCount < 0 THEN 0
        WHEN playerCondition.ProgressCount > referenceCondition.CompleteCount
            THEN referenceCondition.CompleteCount
        ELSE playerCondition.ProgressCount
    END
FROM dbo.Achievement_PlayerConditions AS playerCondition
INNER JOIN dbo.Achievement_Conditions AS referenceCondition
    ON referenceCondition.ID = playerCondition.RefAchievementConditionID;

UPDATE player
SET State =
    CASE
        WHEN EXISTS
        (
            SELECT 1
            FROM dbo.Achievement_Conditions AS referenceCondition
            WHERE referenceCondition.RefAchievementID = player.RefAchievementID
        )
        AND NOT EXISTS
        (
            SELECT 1
            FROM dbo.Achievement_Conditions AS referenceCondition
            LEFT JOIN dbo.Achievement_PlayerConditions AS playerCondition
                ON playerCondition.CharID = player.CharID
               AND playerCondition.RefAchievementConditionID = referenceCondition.ID
            WHERE referenceCondition.RefAchievementID = player.RefAchievementID
              AND
              (
                  playerCondition.ID IS NULL
                  OR playerCondition.ProgressCount < referenceCondition.CompleteCount
              )
        )
            THEN 1
        ELSE 0
    END
FROM dbo.Achievement_Players AS player;

/* This production implementation awards titles. Reject misleading reward rows. */
IF EXISTS (SELECT 1 FROM dbo.Achievement_List WHERE RewardType <> 0)
    THROW 51001, 'Only title rewards (RewardType 0) are supported by this achievement runtime.', 1;

IF EXISTS (SELECT 1 FROM dbo.Achievement_Conditions WHERE Type <> 0)
    THROW 51002, 'Only count-based conditions (Type 0) are supported by this achievement runtime.', 1;

IF NOT EXISTS (SELECT 1 FROM sys.check_constraints WHERE name = N'CK_Achievement_List_RewardType')
    ALTER TABLE dbo.Achievement_List WITH CHECK
        ADD CONSTRAINT CK_Achievement_List_RewardType CHECK (RewardType = 0);

IF NOT EXISTS (SELECT 1 FROM sys.check_constraints WHERE name = N'CK_Achievement_Conditions_Type')
    ALTER TABLE dbo.Achievement_Conditions WITH CHECK
        ADD CONSTRAINT CK_Achievement_Conditions_Type CHECK (Type = 0);

IF NOT EXISTS (SELECT 1 FROM sys.check_constraints WHERE name = N'CK_Achievement_Conditions_CompleteCount')
    ALTER TABLE dbo.Achievement_Conditions WITH CHECK
        ADD CONSTRAINT CK_Achievement_Conditions_CompleteCount CHECK (CompleteCount > 0);

IF NOT EXISTS (SELECT 1 FROM sys.check_constraints WHERE name = N'CK_Achievement_Players_State')
    ALTER TABLE dbo.Achievement_Players WITH CHECK
        ADD CONSTRAINT CK_Achievement_Players_State CHECK (State IN (0, 1));

IF NOT EXISTS (SELECT 1 FROM sys.check_constraints WHERE name = N'CK_Achievement_PlayerConditions_Progress')
    ALTER TABLE dbo.Achievement_PlayerConditions WITH CHECK
        ADD CONSTRAINT CK_Achievement_PlayerConditions_Progress CHECK (ProgressCount >= 0);

IF NOT EXISTS (SELECT 1 FROM sys.foreign_keys WHERE name = N'FK_Achievement_Conditions_List')
    ALTER TABLE dbo.Achievement_Conditions WITH CHECK
        ADD CONSTRAINT FK_Achievement_Conditions_List
        FOREIGN KEY (RefAchievementID) REFERENCES dbo.Achievement_List(ID);

IF NOT EXISTS (SELECT 1 FROM sys.foreign_keys WHERE name = N'FK_Achievement_Players_List')
    ALTER TABLE dbo.Achievement_Players WITH CHECK
        ADD CONSTRAINT FK_Achievement_Players_List
        FOREIGN KEY (RefAchievementID) REFERENCES dbo.Achievement_List(ID);

IF NOT EXISTS (SELECT 1 FROM sys.foreign_keys WHERE name = N'FK_Achievement_PlayerConditions_Player')
    ALTER TABLE dbo.Achievement_PlayerConditions WITH CHECK
        ADD CONSTRAINT FK_Achievement_PlayerConditions_Player
        FOREIGN KEY (AchievementID) REFERENCES dbo.Achievement_Players(ID)
        ON DELETE CASCADE;

IF NOT EXISTS (SELECT 1 FROM sys.foreign_keys WHERE name = N'FK_Achievement_PlayerConditions_Condition')
    ALTER TABLE dbo.Achievement_PlayerConditions WITH CHECK
        ADD CONSTRAINT FK_Achievement_PlayerConditions_Condition
        FOREIGN KEY (RefAchievementConditionID) REFERENCES dbo.Achievement_Conditions(ID);

COMMIT TRANSACTION;
GO

CREATE OR ALTER PROCEDURE dbo.Achievement_AddPlayer
    @CharID int
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @CharID IS NULL OR @CharID <= 0
        RETURN -100;

    IF NOT EXISTS (SELECT 1 FROM SRO_VT_SHARD.dbo._Char WHERE CharID = @CharID)
        RETURN -101;

    BEGIN TRY
        BEGIN TRANSACTION;

        INSERT dbo.Achievement_Players (CharID, RefAchievementID, State)
        SELECT @CharID, reference.ID, 0
        FROM dbo.Achievement_List AS reference
        WHERE reference.Service = 1
          AND NOT EXISTS
          (
              SELECT 1
              FROM dbo.Achievement_Players AS player WITH (UPDLOCK, HOLDLOCK)
              WHERE player.CharID = @CharID
                AND player.RefAchievementID = reference.ID
          );

        INSERT dbo.Achievement_PlayerConditions
               (CharID, AchievementID, RefAchievementConditionID, ProgressCount)
        SELECT @CharID, player.ID, referenceCondition.ID, 0
        FROM dbo.Achievement_Players AS player
        INNER JOIN dbo.Achievement_List AS reference
            ON reference.ID = player.RefAchievementID
           AND reference.Service = 1
        INNER JOIN dbo.Achievement_Conditions AS referenceCondition
            ON referenceCondition.RefAchievementID = reference.ID
        WHERE player.CharID = @CharID
          AND NOT EXISTS
          (
              SELECT 1
              FROM dbo.Achievement_PlayerConditions AS playerCondition WITH (UPDLOCK, HOLDLOCK)
              WHERE playerCondition.CharID = @CharID
                AND playerCondition.RefAchievementConditionID = referenceCondition.ID
          );

        UPDATE playerCondition
        SET AchievementID = player.ID
        FROM dbo.Achievement_PlayerConditions AS playerCondition
        INNER JOIN dbo.Achievement_Conditions AS referenceCondition
            ON referenceCondition.ID = playerCondition.RefAchievementConditionID
        INNER JOIN dbo.Achievement_Players AS player
            ON player.CharID = playerCondition.CharID
           AND player.RefAchievementID = referenceCondition.RefAchievementID
        WHERE playerCondition.CharID = @CharID
          AND playerCondition.AchievementID <> player.ID;

        COMMIT TRANSACTION;
        RETURN 1;
    END TRY
    BEGIN CATCH
        IF XACT_STATE() <> 0
            ROLLBACK TRANSACTION;
        THROW;
    END CATCH
END;
GO

CREATE OR ALTER PROCEDURE dbo.Achievement_Update
    @CharID int,
    @RefAchievementID int,
    @RefAchievementConditionID int,
    @Progress bigint
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @CharID IS NULL OR @CharID <= 0 OR @Progress IS NULL OR @Progress <= 0
        RETURN -100;

    DECLARE @CompleteCount bigint;
    DECLARE @PlayerAchievementID int;
    DECLARE @CurrentState tinyint;
    DECLARE @NewProgress bigint;
    DECLARE @NewState tinyint = 0;
    DECLARE @Updated table (ProgressCount bigint NOT NULL);

    BEGIN TRY
        BEGIN TRANSACTION;

        SELECT @CompleteCount = referenceCondition.CompleteCount
        FROM dbo.Achievement_Conditions AS referenceCondition
        INNER JOIN dbo.Achievement_List AS reference
            ON reference.ID = referenceCondition.RefAchievementID
        WHERE reference.ID = @RefAchievementID
          AND reference.Service = 1
          AND reference.RewardType = 0
          AND referenceCondition.ID = @RefAchievementConditionID
          AND referenceCondition.Type = 0;

        IF @CompleteCount IS NULL
        BEGIN
            ROLLBACK TRANSACTION;
            RETURN -101;
        END;

        SELECT @PlayerAchievementID = player.ID,
               @CurrentState = player.State
        FROM dbo.Achievement_Players AS player WITH (UPDLOCK, HOLDLOCK)
        WHERE player.CharID = @CharID
          AND player.RefAchievementID = @RefAchievementID;

        IF @PlayerAchievementID IS NULL
        BEGIN
            ROLLBACK TRANSACTION;
            RETURN -102;
        END;

        IF @CurrentState = 1
        BEGIN
            COMMIT TRANSACTION;
            RETURN 2;
        END;

        UPDATE playerCondition WITH (UPDLOCK, ROWLOCK)
        SET ProgressCount =
            CASE
                WHEN playerCondition.ProgressCount >= @CompleteCount
                    THEN @CompleteCount
                WHEN @Progress >= @CompleteCount - playerCondition.ProgressCount
                    THEN @CompleteCount
                ELSE playerCondition.ProgressCount + @Progress
            END
        OUTPUT inserted.ProgressCount INTO @Updated (ProgressCount)
        FROM dbo.Achievement_PlayerConditions AS playerCondition
        WHERE playerCondition.CharID = @CharID
          AND playerCondition.AchievementID = @PlayerAchievementID
          AND playerCondition.RefAchievementConditionID = @RefAchievementConditionID;

        SELECT @NewProgress = ProgressCount FROM @Updated;
        IF @NewProgress IS NULL
        BEGIN
            ROLLBACK TRANSACTION;
            RETURN -103;
        END;

        IF NOT EXISTS
        (
            SELECT 1
            FROM dbo.Achievement_Conditions AS referenceCondition
            LEFT JOIN dbo.Achievement_PlayerConditions AS playerCondition
                ON playerCondition.AchievementID = @PlayerAchievementID
               AND playerCondition.RefAchievementConditionID = referenceCondition.ID
            WHERE referenceCondition.RefAchievementID = @RefAchievementID
              AND
              (
                  playerCondition.ID IS NULL
                  OR playerCondition.ProgressCount < referenceCondition.CompleteCount
              )
        )
        BEGIN
            SET @NewState = 1;
            UPDATE dbo.Achievement_Players
            SET State = 1
            WHERE ID = @PlayerAchievementID
              AND State = 0;

            IF @@ROWCOUNT = 1
                EXEC dbo.Command_NoticeByID
                    @CharID = @CharID,
                    @NoticeType = 8,
                    @Notice = 'Achievement completed. A new title has been unlocked.';
        END;

        INSERT dbo.Command_FilterQueue
               (CommandID, Data1, Data2, Data3, Data4, Data5, Status)
        VALUES (29, @CharID, @RefAchievementID, @RefAchievementConditionID,
                @NewProgress, @NewState, 1);

        COMMIT TRANSACTION;
        RETURN 1;
    END TRY
    BEGIN CATCH
        IF XACT_STATE() <> 0
            ROLLBACK TRANSACTION;
        THROW;
    END CATCH
END;
GO

IF OBJECT_ID(N'dbo.Achievement_UniqueTargets', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Achievement_UniqueTargets
    (
        MobCodeName128 varchar(128) NOT NULL
            CONSTRAINT PK_Achievement_UniqueTargets PRIMARY KEY,
        RefAchievementID int NOT NULL,
        RefAchievementConditionID int NOT NULL,
        ProgressPerKill bigint NOT NULL
            CONSTRAINT DF_Achievement_UniqueTargets_Progress DEFAULT (1),
        Service bit NOT NULL
            CONSTRAINT DF_Achievement_UniqueTargets_Service DEFAULT (1),
        CONSTRAINT CK_Achievement_UniqueTargets_Progress CHECK (ProgressPerKill > 0),
        CONSTRAINT FK_Achievement_UniqueTargets_Achievement
            FOREIGN KEY (RefAchievementID) REFERENCES dbo.Achievement_List(ID),
        CONSTRAINT FK_Achievement_UniqueTargets_Condition
            FOREIGN KEY (RefAchievementConditionID) REFERENCES dbo.Achievement_Conditions(ID)
    );
END;
GO

MERGE dbo.Achievement_UniqueTargets AS target
USING
(
    VALUES
        ('MOB_CH_TIGERWOMAN', 1, 1, CONVERT(bigint, 1)),
        ('MOB_EU_CERBERUS', 2, 2, CONVERT(bigint, 1)),
        ('MOB_AM_IVY', 3, 3, CONVERT(bigint, 1)),
        ('MOB_CH_IVY', 3, 3, CONVERT(bigint, 1)),
        ('MOB_OA_URUCHI', 4, 4, CONVERT(bigint, 1)),
        ('MOB_OA_ISYUTARU', 5, 5, CONVERT(bigint, 1)),
        ('MOB_KK_ISYUTARU', 5, 5, CONVERT(bigint, 1)),
        ('MOB_KK_LORD_YARKAN', 6, 6, CONVERT(bigint, 1)),
        ('MOB_TK_BONELORD', 6, 6, CONVERT(bigint, 1)),
        ('MOB_TQ_SHITAN', 7, 7, CONVERT(bigint, 1)),
        ('MOB_RM_TAHOMET', 7, 7, CONVERT(bigint, 1))
) AS source (MobCodeName128, RefAchievementID, RefAchievementConditionID, ProgressPerKill)
ON target.MobCodeName128 = source.MobCodeName128
WHEN MATCHED THEN
    UPDATE SET RefAchievementID = source.RefAchievementID,
               RefAchievementConditionID = source.RefAchievementConditionID,
               ProgressPerKill = source.ProgressPerKill
WHEN NOT MATCHED THEN
    INSERT (MobCodeName128, RefAchievementID, RefAchievementConditionID, ProgressPerKill, Service)
    VALUES (source.MobCodeName128, source.RefAchievementID,
            source.RefAchievementConditionID, source.ProgressPerKill, 1);
GO

CREATE OR ALTER PROCEDURE dbo.Achievement_UniqueKill
    @RefObjID int,
    @KillerrName varchar(16)
AS
BEGIN
    SET NOCOUNT ON;

    SET @KillerrName = LTRIM(RTRIM(@KillerrName));
    IF @RefObjID IS NULL OR NULLIF(@KillerrName, '') IS NULL
        RETURN -100;

    DECLARE @CharID int;
    DECLARE @RefAchievementID int;
    DECLARE @RefAchievementConditionID int;
    DECLARE @Progress bigint;

    SELECT TOP (1) @CharID = CharID
    FROM SRO_VT_SHARD.dbo._Char
    WHERE CharName16 COLLATE DATABASE_DEFAULT = @KillerrName
    ORDER BY CharID DESC;

    SELECT @RefAchievementID = target.RefAchievementID,
           @RefAchievementConditionID = target.RefAchievementConditionID,
           @Progress = target.ProgressPerKill
    FROM SRO_VT_SHARD.dbo._RefObjCommon AS monster
    INNER JOIN dbo.Achievement_UniqueTargets AS target
        ON target.MobCodeName128 = monster.CodeName128 COLLATE DATABASE_DEFAULT
       AND target.Service = 1
    WHERE monster.ID = @RefObjID;

    IF @CharID IS NULL OR @RefAchievementID IS NULL
        RETURN 2;

    EXEC dbo.Achievement_Update
        @CharID = @CharID,
        @RefAchievementID = @RefAchievementID,
        @RefAchievementConditionID = @RefAchievementConditionID,
        @Progress = @Progress;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Achievement_UniqueKillByID
    @RefObjID int,
    @KillerCharName varchar(16)
AS
BEGIN
    SET NOCOUNT ON;
    EXEC dbo.Achievement_UniqueKill
        @RefObjID = @RefObjID,
        @KillerrName = @KillerCharName;
END;
GO

/*
Keep the historical c2 hook name working, but route it to the single
authoritative implementation instead of referencing removed synonyms.
*/
USE [c2];
GO
CREATE OR ALTER PROCEDURE dbo._Achievement_AddNewChar
    @CharID int
AS
BEGIN
    SET NOCOUNT ON;
    EXEC KMTGuard.dbo.Achievement_AddPlayer @CharID = @CharID;
END;
GO

USE [KMTGuard];
GO
