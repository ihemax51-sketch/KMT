USE [master];
GO

IF DB_ID(N'Events') IS NULL
    THROW 51000, 'Events database is missing. Apply the earlier Auto Events database updates first.', 1;
GO

IF OBJECT_ID(N'Events.dbo._AutoEventSchedule', N'U') IS NULL
    THROW 51000, 'Events.dbo._AutoEventSchedule is missing. Apply the earlier Auto Events schedule update first.', 1;
GO

IF COL_LENGTH(N'Events.dbo._AutoEventSchedule', N'RepeatMinutes') IS NULL
    ALTER TABLE Events.dbo._AutoEventSchedule ADD RepeatMinutes smallint NULL;
GO

IF COL_LENGTH(N'Events.dbo._AutoEventSchedule', N'LastRunAtLocal') IS NULL
    ALTER TABLE Events.dbo._AutoEventSchedule ADD LastRunAtLocal datetime2(0) NULL;
GO

IF NOT EXISTS
(
    SELECT 1
    FROM Events.sys.check_constraints
    WHERE name = N'CK_AutoEventSchedule_RepeatMinutes'
      AND parent_object_id = OBJECT_ID(N'Events.dbo._AutoEventSchedule')
)
BEGIN
    ALTER TABLE Events.dbo._AutoEventSchedule WITH CHECK
        ADD CONSTRAINT CK_AutoEventSchedule_RepeatMinutes
        CHECK (RepeatMinutes IS NULL OR RepeatMinutes BETWEEN 30 AND 1440);
END;
GO

IF NOT EXISTS
(
    SELECT 1
    FROM Events.sys.indexes
    WHERE name = N'IX_AutoEventSchedule_RecurringDue'
      AND object_id = OBJECT_ID(N'Events.dbo._AutoEventSchedule')
)
BEGIN
    CREATE INDEX IX_AutoEventSchedule_RecurringDue
        ON Events.dbo._AutoEventSchedule(IsActive, EventCode, StartTime, RepeatMinutes)
        INCLUDE (DaysMask, LastRunLocalDate, LastRunAtLocal);
END;
GO

PRINT 'Auto Event recurring schedules and last-run tracking installed.';
GO
