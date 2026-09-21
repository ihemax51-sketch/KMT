USE [Events];
GO

IF OBJECT_ID(N'dbo._AutoEventSchedule', N'U') IS NULL
    THROW 51000, 'Validation failed: dbo._AutoEventSchedule is missing.', 1;
GO

IF COL_LENGTH(N'dbo._AutoEventSchedule', N'RepeatMinutes') IS NULL
    THROW 51000, 'Validation failed: RepeatMinutes is missing.', 1;
GO

IF COL_LENGTH(N'dbo._AutoEventSchedule', N'LastRunAtLocal') IS NULL
    THROW 51000, 'Validation failed: LastRunAtLocal is missing.', 1;
GO

IF EXISTS
(
    SELECT 1
    FROM dbo._AutoEventSchedule
    WHERE RepeatMinutes IS NOT NULL
      AND RepeatMinutes NOT BETWEEN 30 AND 1440
)
    THROW 51000, 'Validation failed: an Auto Event repeat interval is outside the supported range.', 1;
GO

SELECT ScheduleID,
       EventCode,
       CONVERT(varchar(5), StartTime, 108) AS StartTime,
       ISNULL(RepeatMinutes, 0) AS RepeatMinutes,
       DaysMask,
       IsActive,
       LastRunAtLocal
FROM dbo._AutoEventSchedule
ORDER BY EventCode, StartTime, ScheduleID;
GO

PRINT 'Auto Event recurring schedule validation passed.';
GO
