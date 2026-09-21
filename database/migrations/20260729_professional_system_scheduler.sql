/*
    Professional recurring scheduler for KMTGuard.
    Backward compatible with existing None, Daily, and Weekly rows.
*/
SET NOCOUNT ON;
SET XACT_ABORT ON;

IF OBJECT_ID(N'dbo.System_Schedule', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.System_Schedule
    (
        Idx INT IDENTITY(1,1) NOT NULL CONSTRAINT PK__Scheduler PRIMARY KEY,
        Name NVARCHAR(128) NOT NULL,
        Query NVARCHAR(512) NOT NULL,
        ScheduledDate DATE NULL,
        StartDateTime DATETIME2(0) NULL,
        Time TIME NOT NULL,
        RepeatType NVARCHAR(16) NOT NULL CONSTRAINT DF__Scheduler_RepeatType DEFAULT(N'None'),
        RepeatDayOfWeek TINYINT NULL,
        DaysOfWeekMask TINYINT NULL,
        IntervalSeconds INT NULL,
        IsEnabled BIT NOT NULL CONSTRAINT DF__Scheduler_IsEnabled DEFAULT(1),
        LastRunDateTime DATETIME2(0) NULL,
        ExecutionTimeoutSeconds INT NOT NULL CONSTRAINT DF_SystemSchedule_ExecutionTimeout DEFAULT(7200),
        CatchUpWindowSeconds INT NOT NULL CONSTRAINT DF_SystemSchedule_CatchUpWindow DEFAULT(300),
        LastScheduledDateTime DATETIME2(0) NULL,
        RunningToken UNIQUEIDENTIFIER NULL,
        RunningBy NVARCHAR(160) NULL,
        RunningSinceUtc DATETIME2(0) NULL,
        LeaseUntilUtc DATETIME2(0) NULL,
        LastStatus NVARCHAR(16) NULL,
        LastError NVARCHAR(2048) NULL,
        LastDurationMs BIGINT NULL,
        LastCompletedDateTimeUtc DATETIME2(0) NULL,
        CreatedAtUtc DATETIME2(0) NOT NULL CONSTRAINT DF_SystemSchedule_CreatedAtUtc DEFAULT(SYSUTCDATETIME()),
        UpdatedAtUtc DATETIME2(0) NOT NULL CONSTRAINT DF_SystemSchedule_UpdatedAtUtc DEFAULT(SYSUTCDATETIME())
    );
END;

IF COL_LENGTH(N'dbo.System_Schedule', N'StartDateTime') IS NULL
    ALTER TABLE dbo.System_Schedule ADD StartDateTime DATETIME2(0) NULL;

IF COL_LENGTH(N'dbo.System_Schedule', N'DaysOfWeekMask') IS NULL
BEGIN
    ALTER TABLE dbo.System_Schedule ADD DaysOfWeekMask TINYINT NULL;
    UPDATE dbo.System_Schedule
    SET DaysOfWeekMask = CONVERT(TINYINT, POWER(CONVERT(FLOAT, 2), RepeatDayOfWeek - 1))
    WHERE RepeatType = N'Weekly'
      AND RepeatDayOfWeek BETWEEN 1 AND 7;
END;

IF COL_LENGTH(N'dbo.System_Schedule', N'IntervalSeconds') IS NULL
    ALTER TABLE dbo.System_Schedule ADD IntervalSeconds INT NULL;

IF COL_LENGTH(N'dbo.System_Schedule', N'CreatedAtUtc') IS NULL
    ALTER TABLE dbo.System_Schedule
        ADD CreatedAtUtc DATETIME2(0) NOT NULL
            CONSTRAINT DF_SystemSchedule_CreatedAtUtc DEFAULT(SYSUTCDATETIME());

IF COL_LENGTH(N'dbo.System_Schedule', N'UpdatedAtUtc') IS NULL
    ALTER TABLE dbo.System_Schedule
        ADD UpdatedAtUtc DATETIME2(0) NOT NULL
            CONSTRAINT DF_SystemSchedule_UpdatedAtUtc DEFAULT(SYSUTCDATETIME());

IF COL_LENGTH(N'dbo.System_Schedule', N'ExecutionTimeoutSeconds') IS NULL
    ALTER TABLE dbo.System_Schedule
        ADD ExecutionTimeoutSeconds INT NOT NULL
            CONSTRAINT DF_SystemSchedule_ExecutionTimeout DEFAULT(7200);

IF COL_LENGTH(N'dbo.System_Schedule', N'CatchUpWindowSeconds') IS NULL
    ALTER TABLE dbo.System_Schedule
        ADD CatchUpWindowSeconds INT NOT NULL
            CONSTRAINT DF_SystemSchedule_CatchUpWindow DEFAULT(300);

IF COL_LENGTH(N'dbo.System_Schedule', N'LastScheduledDateTime') IS NULL
BEGIN
    ALTER TABLE dbo.System_Schedule ADD LastScheduledDateTime DATETIME2(0) NULL;
    UPDATE dbo.System_Schedule
    SET LastScheduledDateTime = LastRunDateTime
    WHERE LastRunDateTime IS NOT NULL;
END;

IF COL_LENGTH(N'dbo.System_Schedule', N'RunningToken') IS NULL
    ALTER TABLE dbo.System_Schedule ADD RunningToken UNIQUEIDENTIFIER NULL;
IF COL_LENGTH(N'dbo.System_Schedule', N'RunningBy') IS NULL
    ALTER TABLE dbo.System_Schedule ADD RunningBy NVARCHAR(160) NULL;
IF COL_LENGTH(N'dbo.System_Schedule', N'RunningSinceUtc') IS NULL
    ALTER TABLE dbo.System_Schedule ADD RunningSinceUtc DATETIME2(0) NULL;
IF COL_LENGTH(N'dbo.System_Schedule', N'LeaseUntilUtc') IS NULL
    ALTER TABLE dbo.System_Schedule ADD LeaseUntilUtc DATETIME2(0) NULL;
IF COL_LENGTH(N'dbo.System_Schedule', N'LastStatus') IS NULL
    ALTER TABLE dbo.System_Schedule ADD LastStatus NVARCHAR(16) NULL;
IF COL_LENGTH(N'dbo.System_Schedule', N'LastError') IS NULL
    ALTER TABLE dbo.System_Schedule ADD LastError NVARCHAR(2048) NULL;
IF COL_LENGTH(N'dbo.System_Schedule', N'LastDurationMs') IS NULL
    ALTER TABLE dbo.System_Schedule ADD LastDurationMs BIGINT NULL;
IF COL_LENGTH(N'dbo.System_Schedule', N'LastCompletedDateTimeUtc') IS NULL
    ALTER TABLE dbo.System_Schedule ADD LastCompletedDateTimeUtc DATETIME2(0) NULL;

IF OBJECT_ID(N'dbo.System_ScheduleHistory', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.System_ScheduleHistory
    (
        RunID BIGINT IDENTITY(1,1) NOT NULL CONSTRAINT PK_SystemScheduleHistory PRIMARY KEY,
        JobId INT NOT NULL,
        JobName NVARCHAR(128) NOT NULL,
        TriggerType NVARCHAR(16) NOT NULL,
        ScheduledFor DATETIME2(0) NULL,
        StartedAtUtc DATETIME2(0) NOT NULL,
        CompletedAtUtc DATETIME2(0) NOT NULL,
        Status NVARCHAR(16) NOT NULL,
        DurationMs BIGINT NOT NULL,
        ExecutedBy NVARCHAR(160) NULL,
        Error NVARCHAR(2048) NULL
    );
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.System_ScheduleHistory')
      AND name = N'IX_SystemScheduleHistory_JobRun'
)
BEGIN
    CREATE INDEX IX_SystemScheduleHistory_JobRun
        ON dbo.System_ScheduleHistory(JobId, RunID DESC);
END;

PRINT 'Professional System_Schedule migration completed.';
