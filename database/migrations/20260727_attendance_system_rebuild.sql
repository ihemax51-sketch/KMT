/*
    KMTGuard Attendance system rebuild.

    The Attendance calendar is a repeating 35-day cycle. The client supports
    at most 50 configured rewards. No reward definitions are inserted here;
    an empty Attendance_Rewards table is a valid configuration.

    Run this migration while every KMTGuard Agent instance is stopped.
*/

SET NOCOUNT ON;
SET XACT_ABORT ON;

IF OBJECT_ID(N'dbo.Attendance_Players', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Attendance_Players
    (
        ID               INT IDENTITY(1, 1) NOT NULL
            CONSTRAINT PK_Attendance_Players PRIMARY KEY CLUSTERED,
        CharID           INT  NOT NULL,
        DayCount         INT  NOT NULL,
        LastAttendedDate DATE NOT NULL
    );
END;

IF OBJECT_ID(N'dbo.Attendance_Rewards', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Attendance_Rewards
    (
        ID                  INT IDENTITY(1, 1) NOT NULL
            CONSTRAINT PK_Attendance_Rewards PRIMARY KEY CLUSTERED,
        ItemID              INT          NOT NULL,
        ItemCodeName128     VARCHAR(128) NOT NULL,
        ItemCount           INT          NOT NULL,
        DayCount            INT          NOT NULL
    );
END;

IF OBJECT_ID(N'dbo.Attendance_RewardLog', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Attendance_RewardLog
    (
        ID           INT IDENTITY(1, 1) NOT NULL
            CONSTRAINT PK_Attendance_RewardLog PRIMARY KEY CLUSTERED,
        RefRewardID  INT NOT NULL,
        CharID       INT NOT NULL,
        DayCount     INT NOT NULL,
        CanTake      INT NOT NULL,
        AlreadyTaken INT NOT NULL
    );
END;

BEGIN TRANSACTION;

IF (SELECT COUNT_BIG(*) FROM dbo.Attendance_Rewards) > 50
    THROW 51400, 'Attendance supports at most 50 reward definitions.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.Attendance_Rewards
    WHERE ID <= 0
       OR ItemID <= 0
       OR ItemCount <= 0
       OR ItemCount > 1000000
       OR DayCount NOT BETWEEN 1 AND 35
       OR NULLIF(LTRIM(RTRIM(ItemCodeName128)), '') IS NULL
)
    THROW 51401, 'Attendance_Rewards contains an invalid reward definition.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.Attendance_Rewards AS reward
    LEFT JOIN SRO_VT_SHARD.dbo._RefObjCommon AS item
      ON item.ID = reward.ItemID
     AND item.CodeName128 COLLATE DATABASE_DEFAULT = reward.ItemCodeName128
     AND item.Service = 1
     AND item.TypeID1 = 3
    WHERE item.ID IS NULL
)
    THROW 51402, 'Attendance_Rewards contains an inactive or mismatched item.', 1;

/* Preserve the furthest valid progress when repairing legacy duplicates. */
;WITH PlayerSummary AS
(
    SELECT
        CharID,
        MIN(ID) AS KeepID,
        MAX(CASE WHEN DayCount < 1 THEN 1
                 WHEN DayCount > 35 THEN 35
                 ELSE DayCount END) AS RepairedDayCount,
        MAX(LastAttendedDate) AS RepairedLastDate
    FROM dbo.Attendance_Players
    GROUP BY CharID
)
UPDATE player
SET
    DayCount = summary.RepairedDayCount,
    LastAttendedDate = summary.RepairedLastDate
FROM dbo.Attendance_Players AS player
INNER JOIN PlayerSummary AS summary
    ON summary.KeepID = player.ID;

;WITH RankedPlayers AS
(
    SELECT ID, ROW_NUMBER() OVER (PARTITION BY CharID ORDER BY ID) AS RowNumber
    FROM dbo.Attendance_Players
)
DELETE FROM RankedPlayers
WHERE RowNumber > 1;

/* Removed reward definitions must not leave claimable orphan state behind. */
DELETE rewardLog
FROM dbo.Attendance_RewardLog AS rewardLog
LEFT JOIN dbo.Attendance_Rewards AS reward
    ON reward.ID = rewardLog.RefRewardID
WHERE reward.ID IS NULL
   OR rewardLog.RefRewardID IS NULL
   OR rewardLog.DayCount IS NULL;

UPDATE rewardLog
SET
    DayCount = reward.DayCount,
    AlreadyTaken = CASE WHEN rewardLog.AlreadyTaken = 1 THEN 1 ELSE 0 END,
    CanTake = CASE
                  WHEN rewardLog.AlreadyTaken = 1 THEN 0
                  WHEN rewardLog.CanTake = 1 THEN 1
                  ELSE 0
              END
FROM dbo.Attendance_RewardLog AS rewardLog
INNER JOIN dbo.Attendance_Rewards AS reward
    ON reward.ID = rewardLog.RefRewardID;

;WITH RewardLogSummary AS
(
    SELECT
        CharID,
        RefRewardID,
        MIN(ID) AS KeepID,
        MAX(AlreadyTaken) AS WasTaken,
        MAX(CanTake) AS WasAvailable
    FROM dbo.Attendance_RewardLog
    GROUP BY CharID, RefRewardID
)
UPDATE rewardLog
SET
    AlreadyTaken = summary.WasTaken,
    CanTake = CASE WHEN summary.WasTaken = 1 THEN 0 ELSE summary.WasAvailable END
FROM dbo.Attendance_RewardLog AS rewardLog
INNER JOIN RewardLogSummary AS summary
    ON summary.KeepID = rewardLog.ID;

;WITH RankedRewardLogs AS
(
    SELECT ID, ROW_NUMBER() OVER
    (
        PARTITION BY CharID, RefRewardID
        ORDER BY ID
    ) AS RowNumber
    FROM dbo.Attendance_RewardLog
)
DELETE FROM RankedRewardLogs
WHERE RowNumber > 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.Attendance_RewardLog AS rewardLog
    LEFT JOIN dbo.Attendance_Players AS player
        ON player.CharID = rewardLog.CharID
    WHERE player.CharID IS NULL
)
BEGIN
    DELETE rewardLog
    FROM dbo.Attendance_RewardLog AS rewardLog
    LEFT JOIN dbo.Attendance_Players AS player
        ON player.CharID = rewardLog.CharID
    WHERE player.CharID IS NULL;
END;

IF EXISTS
(
    SELECT 1
    FROM sys.columns
    WHERE object_id = OBJECT_ID(N'dbo.Attendance_Rewards')
      AND name = N'ItemCodeName128'
      AND is_nullable = 1
)
    ALTER TABLE dbo.Attendance_Rewards ALTER COLUMN ItemCodeName128 VARCHAR(128) NOT NULL;

IF EXISTS
(
    SELECT 1
    FROM sys.columns
    WHERE object_id = OBJECT_ID(N'dbo.Attendance_RewardLog')
      AND name = N'RefRewardID'
      AND is_nullable = 1
)
    ALTER TABLE dbo.Attendance_RewardLog ALTER COLUMN RefRewardID INT NOT NULL;

IF EXISTS
(
    SELECT 1
    FROM sys.columns
    WHERE object_id = OBJECT_ID(N'dbo.Attendance_RewardLog')
      AND name = N'DayCount'
      AND is_nullable = 1
)
    ALTER TABLE dbo.Attendance_RewardLog ALTER COLUMN DayCount INT NOT NULL;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Attendance_Players')
      AND name = N'UX_Attendance_Players_CharID'
)
    CREATE UNIQUE INDEX UX_Attendance_Players_CharID
        ON dbo.Attendance_Players(CharID);

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Attendance_RewardLog')
      AND name = N'UX_Attendance_RewardLog_Char_Reward'
)
    CREATE UNIQUE INDEX UX_Attendance_RewardLog_Char_Reward
        ON dbo.Attendance_RewardLog(CharID, RefRewardID);

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Attendance_RewardLog')
      AND name = N'IX_Attendance_RewardLog_Eligible'
)
    CREATE INDEX IX_Attendance_RewardLog_Eligible
        ON dbo.Attendance_RewardLog(CharID, CanTake, AlreadyTaken)
        INCLUDE (RefRewardID, DayCount);

IF OBJECT_ID(N'dbo.CK_Attendance_Players_DayCount', N'C') IS NULL
    ALTER TABLE dbo.Attendance_Players WITH CHECK
        ADD CONSTRAINT CK_Attendance_Players_DayCount
            CHECK (DayCount BETWEEN 1 AND 35);

IF OBJECT_ID(N'dbo.CK_Attendance_Rewards_Values', N'C') IS NULL
    ALTER TABLE dbo.Attendance_Rewards WITH CHECK
        ADD CONSTRAINT CK_Attendance_Rewards_Values
            CHECK
            (
                ID > 0
                AND ItemID > 0
                AND ItemCount BETWEEN 1 AND 1000000
                AND DayCount BETWEEN 1 AND 35
                AND NULLIF(LTRIM(RTRIM(ItemCodeName128)), '') IS NOT NULL
            );

IF OBJECT_ID(N'dbo.CK_Attendance_RewardLog_State', N'C') IS NULL
    ALTER TABLE dbo.Attendance_RewardLog WITH CHECK
        ADD CONSTRAINT CK_Attendance_RewardLog_State
            CHECK
            (
                DayCount BETWEEN 1 AND 35
                AND CanTake IN (0, 1)
                AND AlreadyTaken IN (0, 1)
                AND NOT (CanTake = 1 AND AlreadyTaken = 1)
            );

IF OBJECT_ID(N'dbo.FK_Attendance_RewardLog_Reward', N'F') IS NULL
    ALTER TABLE dbo.Attendance_RewardLog WITH CHECK
        ADD CONSTRAINT FK_Attendance_RewardLog_Reward
            FOREIGN KEY (RefRewardID)
            REFERENCES dbo.Attendance_Rewards(ID)
            ON DELETE CASCADE;

IF OBJECT_ID(N'dbo.FK_Attendance_RewardLog_Player', N'F') IS NULL
    ALTER TABLE dbo.Attendance_RewardLog WITH CHECK
        ADD CONSTRAINT FK_Attendance_RewardLog_Player
            FOREIGN KEY (CharID)
            REFERENCES dbo.Attendance_Players(CharID)
            ON DELETE CASCADE;

COMMIT TRANSACTION;
GO

CREATE OR ALTER TRIGGER dbo.Attendance_Rewards_Validate
ON dbo.Attendance_Rewards
AFTER INSERT, UPDATE
AS
BEGIN
    SET NOCOUNT ON;

    IF (SELECT COUNT_BIG(*) FROM dbo.Attendance_Rewards) > 50
        THROW 51403, 'Attendance supports at most 50 reward definitions.', 1;

    IF EXISTS
    (
        SELECT 1
        FROM inserted AS reward
        LEFT JOIN SRO_VT_SHARD.dbo._RefObjCommon AS item
          ON item.ID = reward.ItemID
         AND item.CodeName128 COLLATE DATABASE_DEFAULT = reward.ItemCodeName128
         AND item.Service = 1
         AND item.TypeID1 = 3
        WHERE item.ID IS NULL
    )
        THROW 51404, 'The Attendance reward item is inactive or does not match ItemCodeName128.', 1;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Attendance_GetState
    @CharID INT
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @CharID <= 0
        THROW 51410, 'A valid CharID is required.', 1;

    BEGIN TRANSACTION;

    DECLARE @DayCount INT = 0;
    DECLARE @LastAttendedDate DATE = NULL;

    SELECT
        @DayCount = player.DayCount,
        @LastAttendedDate = player.LastAttendedDate
    FROM dbo.Attendance_Players AS player WITH (UPDLOCK, HOLDLOCK)
    WHERE player.CharID = @CharID;

    IF @DayCount > 0
    BEGIN
        UPDATE rewardLog
        SET
            rewardLog.DayCount = reward.DayCount,
            rewardLog.CanTake = CASE
                                    WHEN rewardLog.AlreadyTaken = 0
                                     AND reward.DayCount <= @DayCount THEN 1
                                    ELSE 0
                                END
        FROM dbo.Attendance_RewardLog AS rewardLog
        INNER JOIN dbo.Attendance_Rewards AS reward
            ON reward.ID = rewardLog.RefRewardID
        WHERE rewardLog.CharID = @CharID;

        INSERT dbo.Attendance_RewardLog
        (
            RefRewardID,
            CharID,
            DayCount,
            CanTake,
            AlreadyTaken
        )
        SELECT
            reward.ID,
            @CharID,
            reward.DayCount,
            CASE WHEN reward.DayCount <= @DayCount THEN 1 ELSE 0 END,
            0
        FROM dbo.Attendance_Rewards AS reward
        WHERE NOT EXISTS
        (
            SELECT 1
            FROM dbo.Attendance_RewardLog AS rewardLog WITH (UPDLOCK, HOLDLOCK)
            WHERE rewardLog.CharID = @CharID
              AND rewardLog.RefRewardID = reward.ID
        );
    END;

    COMMIT TRANSACTION;

    SELECT
        @DayCount AS DayCount,
        @LastAttendedDate AS LastAttendedDate;

    SELECT rewardLog.RefRewardID
    FROM dbo.Attendance_RewardLog AS rewardLog
    INNER JOIN dbo.Attendance_Rewards AS reward
        ON reward.ID = rewardLog.RefRewardID
    WHERE rewardLog.CharID = @CharID
      AND rewardLog.CanTake = 1
      AND rewardLog.AlreadyTaken = 0
    ORDER BY reward.DayCount, reward.ID;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Attendance_Record
    @CharID              INT,
    @Result              INT OUTPUT,
    @DayCount            INT OUTPUT,
    @AttendanceDate      DATE OUTPUT
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @CharID <= 0
        THROW 51420, 'A valid CharID is required.', 1;

    SET @AttendanceDate = CONVERT(DATE, GETDATE());
    SET @Result = 0;
    SET @DayCount = 0;

    BEGIN TRANSACTION;

    DECLARE @LastAttendedDate DATE;

    SELECT
        @DayCount = player.DayCount,
        @LastAttendedDate = player.LastAttendedDate
    FROM dbo.Attendance_Players AS player WITH (UPDLOCK, HOLDLOCK)
    WHERE player.CharID = @CharID;

    IF @LastAttendedDate IS NULL
    BEGIN
        SET @DayCount = 1;

        INSERT dbo.Attendance_Players(CharID, DayCount, LastAttendedDate)
        VALUES (@CharID, @DayCount, @AttendanceDate);

        SET @Result = 1;
    END
    ELSE IF @LastAttendedDate > @AttendanceDate
    BEGIN
        SET @Result = -1;
    END
    ELSE IF @LastAttendedDate < @AttendanceDate
    BEGIN
        SET @DayCount = CASE WHEN @DayCount >= 35 THEN 1 ELSE @DayCount + 1 END;

        UPDATE dbo.Attendance_Players
        SET
            DayCount = @DayCount,
            LastAttendedDate = @AttendanceDate
        WHERE CharID = @CharID;

        IF @DayCount = 1
        BEGIN
            UPDATE rewardLog
            SET
                rewardLog.CanTake = CASE WHEN reward.DayCount = 1 THEN 1 ELSE 0 END,
                rewardLog.AlreadyTaken = 0,
                rewardLog.DayCount = reward.DayCount
            FROM dbo.Attendance_RewardLog AS rewardLog
            INNER JOIN dbo.Attendance_Rewards AS reward
                ON reward.ID = rewardLog.RefRewardID
            WHERE rewardLog.CharID = @CharID;
        END
        ELSE
        BEGIN
            UPDATE rewardLog
            SET
                rewardLog.CanTake = CASE
                                        WHEN rewardLog.AlreadyTaken = 0
                                         AND reward.DayCount <= @DayCount THEN 1
                                        ELSE 0
                                    END,
                rewardLog.DayCount = reward.DayCount
            FROM dbo.Attendance_RewardLog AS rewardLog
            INNER JOIN dbo.Attendance_Rewards AS reward
                ON reward.ID = rewardLog.RefRewardID
            WHERE rewardLog.CharID = @CharID;
        END;

        SET @Result = 1;
    END;

    INSERT dbo.Attendance_RewardLog
    (
        RefRewardID,
        CharID,
        DayCount,
        CanTake,
        AlreadyTaken
    )
    SELECT
        reward.ID,
        @CharID,
        reward.DayCount,
        CASE WHEN reward.DayCount <= @DayCount THEN 1 ELSE 0 END,
        0
    FROM dbo.Attendance_Rewards AS reward
    WHERE @DayCount > 0
      AND NOT EXISTS
      (
          SELECT 1
          FROM dbo.Attendance_RewardLog AS rewardLog WITH (UPDLOCK, HOLDLOCK)
          WHERE rewardLog.CharID = @CharID
            AND rewardLog.RefRewardID = reward.ID
      );

    COMMIT TRANSACTION;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Attendance_ClaimReward
    @CharID       INT,
    @RefRewardID  INT,
    @Result       INT OUTPUT,
    @ItemID       INT OUTPUT,
    @ItemCount    INT OUTPUT
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @CharID <= 0 OR @RefRewardID <= 0
        THROW 51430, 'A valid CharID and RefRewardID are required.', 1;

    SET @Result = 0;
    SET @ItemID = 0;
    SET @ItemCount = 0;

    BEGIN TRANSACTION;

    DECLARE @RewardLogID INT;
    DECLARE @ItemCodeName128 VARCHAR(128);

    SELECT
        @RewardLogID = rewardLog.ID,
        @ItemID = reward.ItemID,
        @ItemCodeName128 = reward.ItemCodeName128,
        @ItemCount = reward.ItemCount
    FROM dbo.Attendance_RewardLog AS rewardLog WITH (UPDLOCK, HOLDLOCK)
    INNER JOIN dbo.Attendance_Rewards AS reward
        ON reward.ID = rewardLog.RefRewardID
    WHERE rewardLog.CharID = @CharID
      AND rewardLog.RefRewardID = @RefRewardID
      AND rewardLog.CanTake = 1
      AND rewardLog.AlreadyTaken = 0;

    IF @RewardLogID IS NULL
    BEGIN
        IF NOT EXISTS
        (
            SELECT 1
            FROM dbo.Attendance_Rewards
            WHERE ID = @RefRewardID
        )
            SET @Result = -1;

        COMMIT TRANSACTION;
        RETURN;
    END;

    EXEC dbo.Item_AddChestByCodeName
        @CharID = @CharID,
        @ItemCodeName = @ItemCodeName128,
        @Quantity = @ItemCount,
        @From = 'Daily Login Event',
        @Plus = 0;

    UPDATE dbo.Attendance_RewardLog
    SET
        CanTake = 0,
        AlreadyTaken = 1
    WHERE ID = @RewardLogID
      AND CanTake = 1
      AND AlreadyTaken = 0;

    IF @@ROWCOUNT <> 1
        THROW 51431, 'Attendance reward state changed before the claim completed.', 1;

    SET @Result = 1;
    COMMIT TRANSACTION;
END;
GO

/* Retire the unsafe API that accepted a caller-supplied date. */
IF OBJECT_ID(N'dbo.Event_Attendance', N'P') IS NOT NULL
    DROP PROCEDURE dbo.Event_Attendance;
GO
