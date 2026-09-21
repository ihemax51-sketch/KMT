/*
    Transactional Attendance integration test.

    This test temporarily creates one reward and test state, verifies daily
    idempotency, the 35-day rollover, and exactly-once reward claiming, then
    rolls every test change back.
*/

SET NOCOUNT ON;
SET XACT_ABORT ON;

BEGIN TRY
    BEGIN TRANSACTION;

    DECLARE @SyntheticCharID INT = 2147483000;
    DECLARE @Result INT;
    DECLARE @DayCount INT;
    DECLARE @AttendanceDate DATE;

    DELETE FROM dbo.Attendance_Players
    WHERE CharID = @SyntheticCharID;

    EXEC dbo.Attendance_Record
        @CharID = @SyntheticCharID,
        @Result = @Result OUTPUT,
        @DayCount = @DayCount OUTPUT,
        @AttendanceDate = @AttendanceDate OUTPUT;

    IF @Result <> 1 OR @DayCount <> 1
        THROW 51520, 'First Attendance record did not create day one.', 1;

    EXEC dbo.Attendance_Record
        @CharID = @SyntheticCharID,
        @Result = @Result OUTPUT,
        @DayCount = @DayCount OUTPUT,
        @AttendanceDate = @AttendanceDate OUTPUT;

    IF @Result <> 0 OR @DayCount <> 1
        THROW 51521, 'Attendance was not idempotent on the same database date.', 1;

    UPDATE dbo.Attendance_Players
    SET
        DayCount = 35,
        LastAttendedDate = DATEADD(DAY, -1, CONVERT(DATE, GETDATE()))
    WHERE CharID = @SyntheticCharID;

    EXEC dbo.Attendance_Record
        @CharID = @SyntheticCharID,
        @Result = @Result OUTPUT,
        @DayCount = @DayCount OUTPUT,
        @AttendanceDate = @AttendanceDate OUTPUT;

    IF @Result <> 1 OR @DayCount <> 1
        THROW 51522, 'Attendance did not roll from day 35 to a new cycle.', 1;

    DECLARE @RealCharID INT =
    (
        SELECT TOP (1) CharID
        FROM SRO_VT_SHARD.dbo._Char
        WHERE Deleted = 0
          AND CharID > 0
        ORDER BY CharID
    );
    DECLARE @ItemID INT;
    DECLARE @ItemCodeName128 VARCHAR(128);
    DECLARE @RewardID INT;

    SELECT TOP (1)
        @ItemID = ID,
        @ItemCodeName128 = CodeName128
    FROM SRO_VT_SHARD.dbo._RefObjCommon
    WHERE Service = 1
      AND TypeID1 = 3
    ORDER BY ID;

    IF @RealCharID IS NULL OR @ItemID IS NULL
        THROW 51523, 'A test character or active item was not available.', 1;

    IF NOT EXISTS
    (
        SELECT 1
        FROM dbo.Attendance_Players
        WHERE CharID = @RealCharID
    )
    BEGIN
        INSERT dbo.Attendance_Players(CharID, DayCount, LastAttendedDate)
        VALUES (@RealCharID, 1, CONVERT(DATE, GETDATE()));
    END;

    DECLARE @InsertedReward TABLE (ID INT NOT NULL);

    INSERT dbo.Attendance_Rewards
    (
        ItemID,
        ItemCodeName128,
        ItemCount,
        DayCount
    )
    OUTPUT inserted.ID INTO @InsertedReward(ID)
    VALUES
    (
        @ItemID,
        @ItemCodeName128,
        1,
        1
    );

    SELECT @RewardID = ID
    FROM @InsertedReward;

    INSERT dbo.Attendance_RewardLog
    (
        RefRewardID,
        CharID,
        DayCount,
        CanTake,
        AlreadyTaken
    )
    VALUES
    (
        @RewardID,
        @RealCharID,
        1,
        1,
        0
    );

    DECLARE @ChestRowsBefore BIGINT =
    (
        SELECT COUNT_BIG(*)
        FROM dbo.Item_Chest
        WHERE CharID = @RealCharID
          AND [Type] = 'Daily Login Event'
    );
    DECLARE @ClaimItemID INT;
    DECLARE @ClaimItemCount INT;

    EXEC dbo.Attendance_ClaimReward
        @CharID = @RealCharID,
        @RefRewardID = @RewardID,
        @Result = @Result OUTPUT,
        @ItemID = @ClaimItemID OUTPUT,
        @ItemCount = @ClaimItemCount OUTPUT;

    IF @Result <> 1 OR @ClaimItemID <> @ItemID OR @ClaimItemCount <> 1
        THROW 51524, 'The first Attendance reward claim failed.', 1;

    IF
    (
        SELECT COUNT_BIG(*)
        FROM dbo.Item_Chest
        WHERE CharID = @RealCharID
          AND [Type] = 'Daily Login Event'
    ) <> @ChestRowsBefore + 1
        THROW 51525, 'The Attendance reward was not added exactly once.', 1;

    EXEC dbo.Attendance_ClaimReward
        @CharID = @RealCharID,
        @RefRewardID = @RewardID,
        @Result = @Result OUTPUT,
        @ItemID = @ClaimItemID OUTPUT,
        @ItemCount = @ClaimItemCount OUTPUT;

    IF @Result <> 0
        THROW 51526, 'A consumed Attendance reward was claimable twice.', 1;

    ROLLBACK TRANSACTION;

    SELECT N'PASS' AS IntegrationTestResult;
END TRY
BEGIN CATCH
    IF XACT_STATE() <> 0
        ROLLBACK TRANSACTION;
    THROW;
END CATCH;
