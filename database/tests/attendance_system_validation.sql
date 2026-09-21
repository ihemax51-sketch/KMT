/*
    Read-only validation for the rebuilt Attendance system.
    Run after 20260727_attendance_system_rebuild.sql.
*/

SET NOCOUNT ON;

DECLARE @ExpectedTables TABLE (Name SYSNAME PRIMARY KEY);
INSERT @ExpectedTables(Name)
VALUES
    (N'Attendance_Players'),
    (N'Attendance_Rewards'),
    (N'Attendance_RewardLog');

IF EXISTS
(
    SELECT 1
    FROM @ExpectedTables AS expected
    WHERE OBJECT_ID(N'dbo.' + QUOTENAME(expected.Name), N'U') IS NULL
)
    THROW 51500, 'One or more Attendance tables are missing.', 1;

DECLARE @ExpectedProcedures TABLE (Name SYSNAME PRIMARY KEY);
INSERT @ExpectedProcedures(Name)
VALUES
    (N'Attendance_GetState'),
    (N'Attendance_Record'),
    (N'Attendance_ClaimReward');

IF EXISTS
(
    SELECT 1
    FROM @ExpectedProcedures AS expected
    WHERE OBJECT_ID(N'dbo.' + QUOTENAME(expected.Name), N'P') IS NULL
)
    THROW 51501, 'One or more Attendance procedures are missing.', 1;

IF OBJECT_ID(N'dbo.Event_Attendance', N'P') IS NOT NULL
    THROW 51502, 'The unsafe legacy Event_Attendance procedure still exists.', 1;

IF OBJECT_ID(N'dbo.Attendance_Rewards_Validate', N'TR') IS NULL
    THROW 51503, 'The Attendance reward validation trigger is missing.', 1;

IF EXISTS
(
    SELECT CharID
    FROM dbo.Attendance_Players
    GROUP BY CharID
    HAVING COUNT(*) > 1
)
    THROW 51504, 'Duplicate Attendance player rows were found.', 1;

IF EXISTS
(
    SELECT CharID, RefRewardID
    FROM dbo.Attendance_RewardLog
    GROUP BY CharID, RefRewardID
    HAVING COUNT(*) > 1
)
    THROW 51505, 'Duplicate Attendance reward-state rows were found.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.Attendance_Players
    WHERE DayCount NOT BETWEEN 1 AND 35
)
    THROW 51506, 'Attendance player progress is outside the 35-day cycle.', 1;

IF (SELECT COUNT_BIG(*) FROM dbo.Attendance_Rewards) > 50
    THROW 51507, 'Attendance has more rewards than the client protocol supports.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.Attendance_RewardLog AS rewardLog
    LEFT JOIN dbo.Attendance_Rewards AS reward
        ON reward.ID = rewardLog.RefRewardID
    LEFT JOIN dbo.Attendance_Players AS player
        ON player.CharID = rewardLog.CharID
    WHERE reward.ID IS NULL
       OR player.CharID IS NULL
       OR rewardLog.DayCount <> reward.DayCount
       OR rewardLog.CanTake NOT IN (0, 1)
       OR rewardLog.AlreadyTaken NOT IN (0, 1)
       OR (rewardLog.CanTake = 1 AND rewardLog.AlreadyTaken = 1)
)
    THROW 51508, 'Invalid or orphaned Attendance reward state was found.', 1;

SELECT
    N'PASS' AS ValidationResult,
    (SELECT COUNT_BIG(*) FROM dbo.Attendance_Players) AS Players,
    (SELECT COUNT_BIG(*) FROM dbo.Attendance_Rewards) AS RewardDefinitions,
    (SELECT COUNT_BIG(*) FROM dbo.Attendance_RewardLog) AS RewardStates;
