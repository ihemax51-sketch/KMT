/*
    Reliable System_Schedule runtime for KMTGuard.

    Run on the KMTGuard/proxy database. The filter and Admin Desktop also apply
    this migration idempotently, so existing installations remain compatible.
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
        Time TIME NOT NULL,
        RepeatType NVARCHAR(16) NOT NULL CONSTRAINT DF__Scheduler_RepeatType DEFAULT(N'None'),
        RepeatDayOfWeek TINYINT NULL,
        IsEnabled BIT NOT NULL CONSTRAINT DF__Scheduler_IsEnabled DEFAULT(1),
        LastRunDateTime DATETIME2(0) NULL
    );
END;

IF COL_LENGTH(N'dbo.System_Schedule', N'ExecutionTimeoutSeconds') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD ExecutionTimeoutSeconds INT NOT NULL CONSTRAINT DF_SystemSchedule_ExecutionTimeout DEFAULT(7200);');

IF COL_LENGTH(N'dbo.System_Schedule', N'CatchUpWindowSeconds') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD CatchUpWindowSeconds INT NOT NULL CONSTRAINT DF_SystemSchedule_CatchUpWindow DEFAULT(300);');

IF COL_LENGTH(N'dbo.System_Schedule', N'LastScheduledDateTime') IS NULL
BEGIN
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD LastScheduledDateTime DATETIME2(0) NULL;');
    EXEC(N'UPDATE dbo.System_Schedule SET LastScheduledDateTime = LastRunDateTime WHERE LastRunDateTime IS NOT NULL;');
END;

IF COL_LENGTH(N'dbo.System_Schedule', N'RunningToken') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD RunningToken UNIQUEIDENTIFIER NULL;');

IF COL_LENGTH(N'dbo.System_Schedule', N'RunningBy') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD RunningBy NVARCHAR(160) NULL;');

IF COL_LENGTH(N'dbo.System_Schedule', N'RunningSinceUtc') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD RunningSinceUtc DATETIME2(0) NULL;');

IF COL_LENGTH(N'dbo.System_Schedule', N'LeaseUntilUtc') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD LeaseUntilUtc DATETIME2(0) NULL;');

IF COL_LENGTH(N'dbo.System_Schedule', N'LastStatus') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD LastStatus NVARCHAR(16) NULL;');

IF COL_LENGTH(N'dbo.System_Schedule', N'LastError') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD LastError NVARCHAR(2048) NULL;');

IF COL_LENGTH(N'dbo.System_Schedule', N'LastDurationMs') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD LastDurationMs BIGINT NULL;');

IF COL_LENGTH(N'dbo.System_Schedule', N'LastCompletedDateTimeUtc') IS NULL
    EXEC(N'ALTER TABLE dbo.System_Schedule ADD LastCompletedDateTimeUtc DATETIME2(0) NULL;');

PRINT 'Reliable System_Schedule migration completed.';
