USE [master];
GO

IF DB_ID(N'Events') IS NULL
    EXEC(N'CREATE DATABASE [Events]');
GO

IF OBJECT_ID(N'Events.dbo._AutoEventSchedule', N'U') IS NULL
BEGIN
    CREATE TABLE Events.dbo._AutoEventSchedule
    (
        ScheduleID int IDENTITY(1,1) NOT NULL
            CONSTRAINT PK_AutoEventSchedule PRIMARY KEY,
        EventCode nvarchar(32) NOT NULL,
        StartTime time(0) NOT NULL,
        DaysMask tinyint NOT NULL
            CONSTRAINT DF_AutoEventSchedule_DaysMask DEFAULT (127),
        IsActive bit NOT NULL
            CONSTRAINT DF_AutoEventSchedule_IsActive DEFAULT (1),
        LastRunLocalDate date NULL,
        UpdatedAtUtc datetime2(0) NOT NULL
            CONSTRAINT DF_AutoEventSchedule_Updated DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT CK_AutoEventSchedule_DaysMask
            CHECK (DaysMask BETWEEN 1 AND 127)
    );

    CREATE INDEX IX_AutoEventSchedule_Due
        ON Events.dbo._AutoEventSchedule(IsActive, EventCode, StartTime);
END;
GO

PRINT 'Auto Event multi-time schedules installed.';
GO
