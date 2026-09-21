USE [master]
GO

IF DB_ID(N'Events') IS NULL
    CREATE DATABASE [Events];
GO

USE [Events]
GO

SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

IF OBJECT_ID(N'dbo._AutoEventUniqueRewardLog', N'U') IS NOT NULL
    DROP TABLE dbo._AutoEventUniqueRewardLog;
GO

IF OBJECT_ID(N'dbo._AutoEventUniqueReward', N'U') IS NOT NULL
    DROP TABLE dbo._AutoEventUniqueReward;
GO

IF DB_ID(N'KMTGuard') IS NOT NULL AND OBJECT_ID(N'KMTGuard.dbo._AutoEventUniqueRewardLog', N'U') IS NOT NULL
    DROP TABLE KMTGuard.dbo._AutoEventUniqueRewardLog;
GO

IF DB_ID(N'KMTGuard') IS NOT NULL AND OBJECT_ID(N'KMTGuard.dbo._AutoEventUniqueReward', N'U') IS NOT NULL
    DROP TABLE KMTGuard.dbo._AutoEventUniqueReward;
GO

IF OBJECT_ID(N'dbo._AutoEventConfig', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._AutoEventConfig
    (
        EventCode nvarchar(32) NOT NULL CONSTRAINT PK_AutoEventConfig PRIMARY KEY,
        DisplayName nvarchar(64) NOT NULL,
        Enabled bit NOT NULL CONSTRAINT DF_AutoEventConfig_Enabled DEFAULT (1),
        StartDelaySeconds int NOT NULL CONSTRAINT DF_AutoEventConfig_StartDelay DEFAULT (60),
        RoundCount int NOT NULL CONSTRAINT DF_AutoEventConfig_RoundCount DEFAULT (3),
        RoundDurationSeconds int NOT NULL CONSTRAINT DF_AutoEventConfig_RoundDuration DEFAULT (60),
        InterRoundDelaySeconds int NOT NULL CONSTRAINT DF_AutoEventConfig_InterRound DEFAULT (10),
        MinLevel int NOT NULL CONSTRAINT DF_AutoEventConfig_MinLevel DEFAULT (1),
        HwidLimit int NOT NULL CONSTRAINT DF_AutoEventConfig_HwidLimit DEFAULT (1),
        UniqueWinnerPerRun bit NOT NULL CONSTRAINT DF_AutoEventConfig_UniqueWinner DEFAULT (1),
        RequireHwid bit NOT NULL CONSTRAINT DF_AutoEventConfig_RequireHwid DEFAULT (1),
        AnswerCooldownMs int NOT NULL CONSTRAINT DF_AutoEventConfig_Cooldown DEFAULT (750),
        AlchemyTargetPlus int NOT NULL CONSTRAINT DF_AutoEventConfig_AlchemyPlus DEFAULT (7),
        UpdatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_AutoEventConfig_Updated DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT CK_AutoEventConfig_StartDelay CHECK (StartDelaySeconds BETWEEN 0 AND 3600),
        CONSTRAINT CK_AutoEventConfig_Rounds CHECK (RoundCount BETWEEN 1 AND 20),
        CONSTRAINT CK_AutoEventConfig_Duration CHECK (RoundDurationSeconds BETWEEN 5 AND 3600),
        CONSTRAINT CK_AutoEventConfig_InterRound CHECK (InterRoundDelaySeconds BETWEEN 0 AND 600),
        CONSTRAINT CK_AutoEventConfig_HwidLimit CHECK (HwidLimit BETWEEN 0 AND 32),
        CONSTRAINT CK_AutoEventConfig_Cooldown CHECK (AnswerCooldownMs BETWEEN 0 AND 10000),
        CONSTRAINT CK_AutoEventConfig_AlchemyPlus CHECK (AlchemyTargetPlus BETWEEN 1 AND 20)
    );
END
GO

IF COL_LENGTH(N'dbo._AutoEventConfig', N'HwidLimit') IS NULL
    ALTER TABLE dbo._AutoEventConfig ADD HwidLimit int NOT NULL CONSTRAINT DF_AutoEventConfig_HwidLimit DEFAULT (1) WITH VALUES;
GO

IF NOT EXISTS (
    SELECT 1
    FROM sys.check_constraints
    WHERE name = N'CK_AutoEventConfig_HwidLimit'
      AND parent_object_id = OBJECT_ID(N'dbo._AutoEventConfig')
)
    ALTER TABLE dbo._AutoEventConfig WITH CHECK ADD CONSTRAINT CK_AutoEventConfig_HwidLimit CHECK (HwidLimit BETWEEN 0 AND 32);
GO

IF OBJECT_ID(N'dbo._AutoEventRoundContent', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._AutoEventRoundContent
    (
        ContentID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_AutoEventRoundContent PRIMARY KEY,
        EventCode nvarchar(32) NOT NULL,
        IsActive bit NOT NULL CONSTRAINT DF_AutoEventRoundContent_Active DEFAULT (1),
        Prompt nvarchar(512) NOT NULL,
        Answer nvarchar(256) NOT NULL,
        Weight int NOT NULL CONSTRAINT DF_AutoEventRoundContent_Weight DEFAULT (1),
        CreatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_AutoEventRoundContent_Created DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT CK_AutoEventRoundContent_Weight CHECK (Weight BETWEEN 1 AND 1000)
    );
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID(N'dbo._AutoEventRoundContent') AND name = N'IX_AutoEventRoundContent_Event')
    CREATE INDEX IX_AutoEventRoundContent_Event ON dbo._AutoEventRoundContent(EventCode, IsActive, Weight);
GO

IF OBJECT_ID(N'dbo._AutoEventReward', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._AutoEventReward
    (
        RewardID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_AutoEventReward PRIMARY KEY,
        EventCode nvarchar(32) NOT NULL,
        Placement int NOT NULL CONSTRAINT DF_AutoEventReward_Placement DEFAULT (1),
        RewardType nvarchar(24) NOT NULL,
        Amount bigint NOT NULL CONSTRAINT DF_AutoEventReward_Amount DEFAULT (0),
        ItemCodeName128 varchar(128) NULL,
        ItemID int NULL,
        ItemCount int NOT NULL CONSTRAINT DF_AutoEventReward_ItemCount DEFAULT (1),
        Plus int NOT NULL CONSTRAINT DF_AutoEventReward_Plus DEFAULT (0),
        IsActive bit NOT NULL CONSTRAINT DF_AutoEventReward_Active DEFAULT (1),
        CreatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_AutoEventReward_Created DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT CK_AutoEventReward_Type CHECK (RewardType IN (N'SilkOwn', N'SilkGift', N'SilkPoint', N'Gold', N'ItemChest')),
        CONSTRAINT CK_AutoEventReward_ItemCount CHECK (ItemCount BETWEEN 1 AND 10000),
        CONSTRAINT CK_AutoEventReward_Plus CHECK (Plus BETWEEN 0 AND 20)
    );
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID(N'dbo._AutoEventReward') AND name = N'IX_AutoEventReward_Event')
    CREATE INDEX IX_AutoEventReward_Event ON dbo._AutoEventReward(EventCode, Placement, IsActive);
GO

IF OBJECT_ID(N'dbo._AutoEventRun', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._AutoEventRun
    (
        RunID bigint IDENTITY(1,1) NOT NULL CONSTRAINT PK_AutoEventRun PRIMARY KEY,
        EventCode nvarchar(32) NOT NULL,
        Status nvarchar(16) NOT NULL,
        StartedAtUtc datetime2(0) NOT NULL,
        FinishedAtUtc datetime2(0) NULL,
        StartedBy nvarchar(64) NOT NULL,
        Message nvarchar(512) NULL
    );
END
GO

IF OBJECT_ID(N'dbo._AutoEventRound', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._AutoEventRound
    (
        RoundID bigint IDENTITY(1,1) NOT NULL CONSTRAINT PK_AutoEventRound PRIMARY KEY,
        RunID bigint NOT NULL,
        RoundNo int NOT NULL,
        Prompt nvarchar(512) NOT NULL,
        AnswerMasked nvarchar(256) NOT NULL,
        Status nvarchar(16) NOT NULL,
        StartedAtUtc datetime2(0) NOT NULL,
        EndedAtUtc datetime2(0) NULL,
        WinnerCharID int NULL,
        WinnerCharName nvarchar(64) NULL
    );
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID(N'dbo._AutoEventRound') AND name = N'IX_AutoEventRound_Run')
    CREATE INDEX IX_AutoEventRound_Run ON dbo._AutoEventRound(RunID, RoundNo);
GO

IF OBJECT_ID(N'dbo._AutoEventWinnerLog', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._AutoEventWinnerLog
    (
        LogID bigint IDENTITY(1,1) NOT NULL CONSTRAINT PK_AutoEventWinnerLog PRIMARY KEY,
        RunID bigint NOT NULL,
        RoundID bigint NOT NULL,
        EventCode nvarchar(32) NOT NULL,
        RoundNo int NOT NULL,
        CharID int NOT NULL,
        CharName nvarchar(64) NOT NULL,
        JID int NOT NULL,
        Hwid nvarchar(128) NOT NULL,
        ClientIP nvarchar(64) NOT NULL,
        Answer nvarchar(256) NOT NULL,
        WonAtUtc datetime2(0) NOT NULL,
        RewardSummary nvarchar(512) NULL
    );
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID(N'dbo._AutoEventWinnerLog') AND name = N'UX_AutoEventWinnerLog_Round')
    CREATE UNIQUE INDEX UX_AutoEventWinnerLog_Round ON dbo._AutoEventWinnerLog(RoundID);
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID(N'dbo._AutoEventWinnerLog') AND name = N'IX_AutoEventWinnerLog_Event')
    CREATE INDEX IX_AutoEventWinnerLog_Event ON dbo._AutoEventWinnerLog(EventCode, WonAtUtc DESC);
GO

IF OBJECT_ID(N'dbo._AutoEventCommandQueue', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._AutoEventCommandQueue
    (
        CommandID bigint IDENTITY(1,1) NOT NULL CONSTRAINT PK_AutoEventCommandQueue PRIMARY KEY,
        CommandType nvarchar(16) NOT NULL,
        EventCode nvarchar(32) NOT NULL CONSTRAINT DF_AutoEventCommandQueue_Event DEFAULT (N''),
        RequestedBy nvarchar(64) NOT NULL CONSTRAINT DF_AutoEventCommandQueue_By DEFAULT (N'SQL'),
        Status tinyint NOT NULL CONSTRAINT DF_AutoEventCommandQueue_Status DEFAULT (0),
        CreatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_AutoEventCommandQueue_Created DEFAULT (SYSUTCDATETIME()),
        ProcessedAtUtc datetime2(0) NULL,
        Message nvarchar(512) NULL,
        CONSTRAINT CK_AutoEventCommandQueue_Status CHECK (Status BETWEEN 0 AND 3)
    );
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID(N'dbo._AutoEventCommandQueue') AND name = N'IX_AutoEventCommandQueue_Pending')
    CREATE INDEX IX_AutoEventCommandQueue_Pending ON dbo._AutoEventCommandQueue(Status, CommandID);
GO

DELETE FROM dbo._AutoEventReward
WHERE EventCode COLLATE DATABASE_DEFAULT = N'UniqueEvent' COLLATE DATABASE_DEFAULT;
GO

DELETE FROM dbo._AutoEventRoundContent
WHERE EventCode COLLATE DATABASE_DEFAULT = N'UniqueEvent' COLLATE DATABASE_DEFAULT;
GO

DELETE FROM dbo._AutoEventConfig
WHERE EventCode COLLATE DATABASE_DEFAULT = N'UniqueEvent' COLLATE DATABASE_DEFAULT;
GO

IF DB_ID(N'KMTGuard') IS NOT NULL AND OBJECT_ID(N'KMTGuard.dbo._AutoEventReward', N'U') IS NOT NULL
    DELETE FROM KMTGuard.dbo._AutoEventReward
    WHERE EventCode COLLATE DATABASE_DEFAULT = N'UniqueEvent' COLLATE DATABASE_DEFAULT;
GO

IF DB_ID(N'KMTGuard') IS NOT NULL AND OBJECT_ID(N'KMTGuard.dbo._AutoEventRoundContent', N'U') IS NOT NULL
    DELETE FROM KMTGuard.dbo._AutoEventRoundContent
    WHERE EventCode COLLATE DATABASE_DEFAULT = N'UniqueEvent' COLLATE DATABASE_DEFAULT;
GO

IF DB_ID(N'KMTGuard') IS NOT NULL AND OBJECT_ID(N'KMTGuard.dbo._AutoEventConfig', N'U') IS NOT NULL
    DELETE FROM KMTGuard.dbo._AutoEventConfig
    WHERE EventCode COLLATE DATABASE_DEFAULT = N'UniqueEvent' COLLATE DATABASE_DEFAULT;
GO

IF DB_ID(N'KMTGuard') IS NOT NULL AND OBJECT_ID(N'KMTGuard.dbo._AutoEventConfig', N'U') IS NOT NULL
BEGIN
    DECLARE @ConfigSql nvarchar(max);
    DECLARE @StartDelayExpression nvarchar(64) =
        CASE WHEN COL_LENGTH(N'KMTGuard.dbo._AutoEventConfig', N'StartDelaySeconds') IS NULL
             THEN N'60'
             ELSE N'StartDelaySeconds'
        END;
    DECLARE @HwidLimitExpression nvarchar(64) =
        CASE WHEN COL_LENGTH(N'KMTGuard.dbo._AutoEventConfig', N'HwidLimit') IS NULL
             THEN N'1'
             ELSE N'HwidLimit'
        END;

    SET @ConfigSql = N'
INSERT INTO Events.dbo._AutoEventConfig
    (EventCode, DisplayName, Enabled, StartDelaySeconds, RoundCount, RoundDurationSeconds,
     InterRoundDelaySeconds, MinLevel, HwidLimit, UniqueWinnerPerRun, RequireHwid, AnswerCooldownMs, AlchemyTargetPlus, UpdatedAtUtc)
SELECT EventCode, DisplayName, Enabled, ' + @StartDelayExpression + N', RoundCount, RoundDurationSeconds,
       InterRoundDelaySeconds, MinLevel, ' + @HwidLimitExpression + N', UniqueWinnerPerRun, RequireHwid, AnswerCooldownMs, AlchemyTargetPlus, UpdatedAtUtc
FROM KMTGuard.dbo._AutoEventConfig old WITH (NOLOCK)
WHERE NOT EXISTS (
    SELECT 1
    FROM Events.dbo._AutoEventConfig target WITH (NOLOCK)
    WHERE target.EventCode COLLATE DATABASE_DEFAULT = old.EventCode COLLATE DATABASE_DEFAULT
);';

    EXEC sys.sp_executesql @ConfigSql;
END
GO

IF DB_ID(N'KMTGuard') IS NOT NULL AND OBJECT_ID(N'KMTGuard.dbo._AutoEventRoundContent', N'U') IS NOT NULL
BEGIN
    INSERT INTO dbo._AutoEventRoundContent (EventCode, IsActive, Prompt, Answer, Weight, CreatedAtUtc)
    SELECT old.EventCode, old.IsActive, old.Prompt, old.Answer, old.Weight, old.CreatedAtUtc
    FROM KMTGuard.dbo._AutoEventRoundContent old WITH (NOLOCK)
    WHERE NOT EXISTS (
        SELECT 1
        FROM dbo._AutoEventRoundContent target WITH (NOLOCK)
        WHERE target.EventCode COLLATE DATABASE_DEFAULT = old.EventCode COLLATE DATABASE_DEFAULT
          AND target.Prompt COLLATE DATABASE_DEFAULT = old.Prompt COLLATE DATABASE_DEFAULT
          AND target.Answer COLLATE DATABASE_DEFAULT = old.Answer COLLATE DATABASE_DEFAULT
    );
END
GO

IF DB_ID(N'KMTGuard') IS NOT NULL AND OBJECT_ID(N'KMTGuard.dbo._AutoEventReward', N'U') IS NOT NULL
BEGIN
    INSERT INTO dbo._AutoEventReward
        (EventCode, Placement, RewardType, Amount, ItemCodeName128, ItemID, ItemCount, Plus, IsActive, CreatedAtUtc)
    SELECT old.EventCode, old.Placement, old.RewardType, old.Amount, old.ItemCodeName128, old.ItemID,
           old.ItemCount, old.Plus, old.IsActive, old.CreatedAtUtc
    FROM KMTGuard.dbo._AutoEventReward old WITH (NOLOCK)
    WHERE NOT EXISTS (
        SELECT 1
        FROM dbo._AutoEventReward target WITH (NOLOCK)
        WHERE target.EventCode COLLATE DATABASE_DEFAULT = old.EventCode COLLATE DATABASE_DEFAULT
          AND target.Placement = old.Placement
          AND target.RewardType COLLATE DATABASE_DEFAULT = old.RewardType COLLATE DATABASE_DEFAULT
          AND ISNULL(target.Amount, 0) = ISNULL(old.Amount, 0)
          AND ISNULL(target.ItemCodeName128, '') COLLATE DATABASE_DEFAULT = ISNULL(old.ItemCodeName128, '') COLLATE DATABASE_DEFAULT
          AND ISNULL(target.ItemID, 0) = ISNULL(old.ItemID, 0)
          AND target.ItemCount = old.ItemCount
          AND target.Plus = old.Plus
    );
END
GO

MERGE dbo._AutoEventConfig AS target
USING (VALUES
    (N'Retype',        N'Retype Event',        1, 60, 3, 45, 10, 1, 1, 1, 1, 750, 7),
    (N'Trivia',        N'Trivia Event',        1, 60, 3, 60, 10, 1, 1, 1, 1, 750, 7),
    (N'FirstType',     N'First Type Event',    1, 60, 3, 45, 10, 1, 1, 1, 1, 750, 7),
    (N'Math',          N'Math Event',          1, 60, 3, 45, 10, 1, 1, 1, 1, 750, 7),
    (N'LongestOnline', N'Longest Online Event',1, 60, 3, 60, 10, 1, 1, 1, 1, 750, 7),
    (N'LuckyStaller',  N'Lucky Staller Event', 1, 60, 3, 60, 10, 1, 1, 1, 1, 750, 7),
    (N'LuckyStall',    N'Lucky Stall Event',   1, 60, 3, 60, 10, 1, 1, 1, 1, 750, 7),
    (N'LuckyParty',    N'Lucky Party Event',   1, 60, 3, 60, 10, 1, 1, 1, 1, 750, 7),
    (N'LuckyGlobal',   N'Lucky Global Event',  1, 60, 3, 60, 10, 1, 1, 1, 1, 750, 7),
    (N'Alchemy',       N'Alchemy Event',       1, 60, 3, 120, 10, 1, 1, 1, 1, 750, 7)
) AS source
    (EventCode, DisplayName, Enabled, StartDelaySeconds, RoundCount, RoundDurationSeconds, InterRoundDelaySeconds, MinLevel,
     HwidLimit, UniqueWinnerPerRun, RequireHwid, AnswerCooldownMs, AlchemyTargetPlus)
ON target.EventCode = source.EventCode
WHEN NOT MATCHED THEN
    INSERT (EventCode, DisplayName, Enabled, StartDelaySeconds, RoundCount, RoundDurationSeconds, InterRoundDelaySeconds, MinLevel, HwidLimit,
            UniqueWinnerPerRun, RequireHwid, AnswerCooldownMs, AlchemyTargetPlus)
    VALUES (source.EventCode, source.DisplayName, source.Enabled, source.StartDelaySeconds, source.RoundCount, source.RoundDurationSeconds,
            source.InterRoundDelaySeconds, source.MinLevel, source.HwidLimit, source.UniqueWinnerPerRun, source.RequireHwid,
            source.AnswerCooldownMs, source.AlchemyTargetPlus);
GO

IF NOT EXISTS (SELECT 1 FROM dbo._AutoEventReward WITH (NOLOCK))
BEGIN
    INSERT INTO dbo._AutoEventReward (EventCode, Placement, RewardType, Amount, ItemCodeName128, ItemCount, Plus)
    VALUES
        (N'Retype',        1, N'SilkOwn', 10, NULL, 1, 0),
        (N'Trivia',        1, N'SilkOwn', 10, NULL, 1, 0),
        (N'FirstType',     1, N'SilkOwn', 10, NULL, 1, 0),
        (N'Math',          1, N'SilkOwn', 10, NULL, 1, 0),
        (N'LongestOnline', 1, N'SilkOwn', 10, NULL, 1, 0),
        (N'LuckyStaller',  1, N'SilkOwn', 10, NULL, 1, 0),
        (N'LuckyStall',    1, N'SilkOwn', 10, NULL, 1, 0),
        (N'LuckyParty',    1, N'SilkOwn', 10, NULL, 1, 0),
        (N'LuckyGlobal',   1, N'SilkOwn', 10, NULL, 1, 0),
        (N'Alchemy',       1, N'SilkOwn', 25, NULL, 1, 0);
END
GO

INSERT INTO dbo._AutoEventReward (EventCode, Placement, RewardType, Amount, ItemCodeName128, ItemCount, Plus)
SELECT seed.EventCode, seed.Placement, seed.RewardType, seed.Amount, seed.ItemCodeName128, seed.ItemCount, seed.Plus
FROM (VALUES
    (N'Retype',        1, N'SilkOwn', 10, CAST(NULL AS varchar(128)), 1, 0),
    (N'Trivia',        1, N'SilkOwn', 10, CAST(NULL AS varchar(128)), 1, 0),
    (N'FirstType',     1, N'SilkOwn', 10, CAST(NULL AS varchar(128)), 1, 0),
    (N'Math',          1, N'SilkOwn', 10, CAST(NULL AS varchar(128)), 1, 0),
    (N'LongestOnline', 1, N'SilkOwn', 10, CAST(NULL AS varchar(128)), 1, 0),
    (N'LuckyStaller',  1, N'SilkOwn', 10, CAST(NULL AS varchar(128)), 1, 0),
    (N'LuckyStall',    1, N'SilkOwn', 10, CAST(NULL AS varchar(128)), 1, 0),
    (N'LuckyParty',    1, N'SilkOwn', 10, CAST(NULL AS varchar(128)), 1, 0),
    (N'LuckyGlobal',   1, N'SilkOwn', 10, CAST(NULL AS varchar(128)), 1, 0),
    (N'Alchemy',       1, N'SilkOwn', 25, CAST(NULL AS varchar(128)), 1, 0)
) AS seed(EventCode, Placement, RewardType, Amount, ItemCodeName128, ItemCount, Plus)
WHERE NOT EXISTS (
    SELECT 1
    FROM dbo._AutoEventReward existing WITH (NOLOCK)
    WHERE existing.EventCode COLLATE DATABASE_DEFAULT = seed.EventCode COLLATE DATABASE_DEFAULT
      AND existing.Placement = seed.Placement
      AND existing.RewardType COLLATE DATABASE_DEFAULT = seed.RewardType COLLATE DATABASE_DEFAULT
      AND existing.IsActive = 1
);
GO

INSERT INTO dbo._AutoEventRoundContent (EventCode, Prompt, Answer, Weight)
SELECT seed.EventCode, seed.Prompt, seed.Answer, seed.Weight
FROM (VALUES
    (N'Retype',    N'Type this exactly: KMTGUARD2026',      N'KMTGUARD2026',   10),
    (N'Retype',    N'Type this exactly: KMTGUARD',        N'KMTGUARD',     10),
    (N'Retype',    N'Type this exactly: FILTERPRO',      N'FILTERPRO',   10),
    (N'Retype',    N'Type this exactly: SILKROAD',       N'SILKROAD',    10),
    (N'Retype',    N'Type this exactly: ALCHEMYPLUS',    N'ALCHEMYPLUS', 10),
    (N'FirstType', N'First to type: FILTER',             N'FILTER',      10),
    (N'FirstType', N'First to type: KMTGUARD',              N'KMTGUARD',       10),
    (N'FirstType', N'First to type: UNIQUE',             N'UNIQUE',      10),
    (N'FirstType', N'First to type: STALLER',            N'STALLER',     10),
    (N'FirstType', N'First to type: CHAMPION',           N'CHAMPION',    10),
    (N'Trivia',    N'What is 12D sun called? Type SUN',  N'SUN',         10),
    (N'Trivia',    N'What is the common currency? Type GOLD', N'GOLD',   10),
    (N'Trivia',    N'What event rewards unique kills? Type UNIQUE', N'UNIQUE', 10),
    (N'Trivia',    N'What system upgrades item plus? Type ALCHEMY', N'ALCHEMY', 10),
    (N'Trivia',    N'What chat event asks fastest answer? Type TRIVIA', N'TRIVIA', 10),
    (N'Alchemy',   N'Make +5 alchemy.',                  N'5',           10),
    (N'Alchemy',   N'Make +6 alchemy.',                  N'6',           10),
    (N'Alchemy',   N'Make +7 alchemy.',                  N'7',           10)
) AS seed(EventCode, Prompt, Answer, Weight)
WHERE NOT EXISTS (
    SELECT 1
    FROM dbo._AutoEventRoundContent existing WITH (NOLOCK)
    WHERE existing.EventCode = seed.EventCode
      AND existing.Prompt = seed.Prompt
      AND existing.Answer = seed.Answer
);
GO

CREATE OR ALTER PROCEDURE dbo._AutoEventEnqueueStart
    @EventCode nvarchar(32),
    @RequestedBy nvarchar(64) = N'Scheduler'
AS
BEGIN
    SET NOCOUNT ON;

    INSERT INTO dbo._AutoEventCommandQueue (CommandType, EventCode, RequestedBy)
    VALUES (N'START', @EventCode, @RequestedBy);
END
GO

CREATE OR ALTER PROCEDURE dbo._AutoEventEnqueueStop
    @RequestedBy nvarchar(64) = N'Scheduler'
AS
BEGIN
    SET NOCOUNT ON;

    INSERT INTO dbo._AutoEventCommandQueue (CommandType, EventCode, RequestedBy)
    VALUES (N'STOP', N'', @RequestedBy);
END
GO

CREATE OR ALTER PROCEDURE dbo._AutoEventEnqueueReload
    @RequestedBy nvarchar(64) = N'Scheduler'
AS
BEGIN
    SET NOCOUNT ON;

    INSERT INTO dbo._AutoEventCommandQueue (CommandType, EventCode, RequestedBy)
    VALUES (N'RELOAD', N'', @RequestedBy);
END
GO

PRINT 'Events database installed. Use EXEC Events.dbo._AutoEventEnqueueStart N''Retype'' from _Scheduler or SQL.';
GO
