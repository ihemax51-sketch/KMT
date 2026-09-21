USE [master];
GO

IF DB_ID(N'Events') IS NULL
    CREATE DATABASE [Events];
GO

USE [Events];
GO

SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

BEGIN TRY
    BEGIN TRANSACTION;

    IF OBJECT_ID(N'dbo._CompetitiveEventConfig', N'U') IS NULL
    BEGIN
        CREATE TABLE dbo._CompetitiveEventConfig
        (
            EventCode nvarchar(32) NOT NULL CONSTRAINT PK_CompetitiveEventConfig PRIMARY KEY,
            DisplayName nvarchar(64) NOT NULL,
            EventMode nvarchar(24) NOT NULL,
            EventID int NOT NULL,
            Enabled bit NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Enabled DEFAULT (1),
            StartDelaySeconds int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_StartDelay DEFAULT (60),
            RegistrationSeconds int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Registration DEFAULT (600),
            PrepareSeconds int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Prepare DEFAULT (20),
            FightSeconds int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Fight DEFAULT (600),
            MinPlayers int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_MinPlayers DEFAULT (2),
            MaxPlayers int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_MaxPlayers DEFAULT (100),
            MinLevel int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_MinLevel DEFAULT (1),
            HwidLimit int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_HwidLimit DEFAULT (1),
            RequireHwid bit NOT NULL CONSTRAINT DF_CompetitiveEventConfig_RequireHwid DEFAULT (1),
            RequireNoParty bit NOT NULL CONSTRAINT DF_CompetitiveEventConfig_NoParty DEFAULT (1),
            ArenaWorldID int NOT NULL,
            ArenaRegionID int NOT NULL,
            ArenaX int NOT NULL,
            ArenaY int NOT NULL,
            ArenaZ int NOT NULL,
            Team1X int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Team1X DEFAULT (0),
            Team1Y int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Team1Y DEFAULT (0),
            Team1Z int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Team1Z DEFAULT (0),
            Team2X int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Team2X DEFAULT (0),
            Team2Y int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Team2Y DEFAULT (0),
            Team2Z int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Team2Z DEFAULT (0),
            MadnessMobID int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_MadnessMob DEFAULT (0),
            MadnessMobCount int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_MadnessCount DEFAULT (3),
            MadnessMobX int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_MadnessX DEFAULT (0),
            MadnessMobY int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_MadnessY DEFAULT (0),
            MadnessMobZ int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_MadnessZ DEFAULT (0),
            MobSpawnDelaySeconds int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_SpawnDelay DEFAULT (60),
            PairKillLimit int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_PairLimit DEFAULT (3),
            TotalKillLimit int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_TotalLimit DEFAULT (200),
            KillRewardItemCode varchar(128) NULL,
            KillRewardItemCount int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_KillItemCount DEFAULT (1),
            KillRewardLimit int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_KillRewardLimit DEFAULT (10),
            Team1TowerMobID int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Tower1Mob DEFAULT (0),
            Team1TowerX int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Tower1X DEFAULT (0),
            Team1TowerY int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Tower1Y DEFAULT (0),
            Team1TowerZ int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Tower1Z DEFAULT (0),
            Team2TowerMobID int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Tower2Mob DEFAULT (0),
            Team2TowerX int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Tower2X DEFAULT (0),
            Team2TowerY int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Tower2Y DEFAULT (0),
            Team2TowerZ int NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Tower2Z DEFAULT (0),
            UpdatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_CompetitiveEventConfig_Updated DEFAULT (SYSUTCDATETIME()),
            CONSTRAINT CK_CompetitiveEventConfig_EventID CHECK (EventID BETWEEN 1 AND 255),
            CONSTRAINT CK_CompetitiveEventConfig_Mode CHECK (EventMode IN (N'LastManStanding', N'MadnessSolo', N'DefendTower')),
            CONSTRAINT CK_CompetitiveEventConfig_Players CHECK (MinPlayers >= 2 AND MaxPlayers >= MinPlayers),
            CONSTRAINT CK_CompetitiveEventConfig_Durations CHECK (StartDelaySeconds >= 0 AND RegistrationSeconds >= 10 AND PrepareSeconds >= 0 AND FightSeconds >= 30)
        );
    END;

    IF DB_ID(N'KMTGuard') IS NULL
        THROW 51000, 'KMTGuard database is required before installing competitive events.', 1;
    IF OBJECT_ID(N'KMTGuard.dbo.Command_GameServerQueue', N'U') IS NULL
        THROW 51000, 'KMTGuard.dbo.Command_GameServerQueue is required.', 1;
    IF OBJECT_ID(N'KMTGuard.dbo.Event_RegisterSettings', N'U') IS NULL
        THROW 51000, 'KMTGuard.dbo.Event_RegisterSettings is required.', 1;
    IF OBJECT_ID(N'KMTGuard.dbo.Event_CurrentTeams', N'U') IS NULL
        THROW 51000, 'KMTGuard.dbo.Event_CurrentTeams is required. Run the base Events migration first.', 1;
    IF OBJECT_ID(N'KMTGuard.dbo.Party_Members', N'U') IS NULL
        THROW 51000, 'KMTGuard.dbo.Party_Members is required.', 1;
    IF OBJECT_ID(N'dbo.Event_RegPlayers', N'U') IS NULL OR
       OBJECT_ID(N'dbo.Event_Control', N'U') IS NULL OR
       OBJECT_ID(N'dbo.Event_Wins', N'U') IS NULL OR
       OBJECT_ID(N'dbo._AutoEventRun', N'U') IS NULL OR
       OBJECT_ID(N'dbo._AutoEventRound', N'U') IS NULL
        THROW 51000, 'Base Events tables are missing. Run the base Auto Events migration before this file.', 1;

    IF NOT EXISTS (SELECT 1 FROM dbo._CompetitiveEventConfig WHERE EventCode = N'LMS')
    BEGIN
        DECLARE @LmsEventID int = NULL;
        IF OBJECT_ID(N'KMTGuard.dbo.Event_RegisterSettings', N'U') IS NOT NULL
            SELECT TOP (1) @LmsEventID = r.ID
            FROM KMTGuard.dbo.Event_RegisterSettings r
            WHERE r.Name = N'Last Man Standing'
              AND NOT EXISTS (SELECT 1 FROM dbo._CompetitiveEventConfig c WHERE c.EventID = r.ID);
        IF @LmsEventID IS NULL
        BEGIN
            SET @LmsEventID = 14;
            WHILE @LmsEventID <= 255 AND
                 (EXISTS (SELECT 1 FROM dbo._CompetitiveEventConfig WHERE EventID = @LmsEventID) OR
                  EXISTS (SELECT 1 FROM KMTGuard.dbo.Event_RegisterSettings WHERE ID = @LmsEventID))
                SET @LmsEventID += 1;
        END;
        IF @LmsEventID > 255 THROW 51000, 'No free Event Register ID is available for LMS.', 1;
        INSERT dbo._CompetitiveEventConfig
            (EventCode, DisplayName, EventMode, EventID, ArenaWorldID, ArenaRegionID, ArenaX, ArenaY, ArenaZ)
        VALUES
            (N'LMS', N'Last Man Standing', N'LastManStanding', @LmsEventID, 107, 25580, 500, 0, 500);
    END;

    IF NOT EXISTS (SELECT 1 FROM dbo._CompetitiveEventConfig WHERE EventCode = N'MADNESS')
    BEGIN
        DECLARE @MadnessEventID int = NULL;
        IF OBJECT_ID(N'KMTGuard.dbo.Event_RegisterSettings', N'U') IS NOT NULL
            SELECT TOP (1) @MadnessEventID = r.ID
            FROM KMTGuard.dbo.Event_RegisterSettings r
            WHERE r.Name = N'Madness Solo'
              AND NOT EXISTS (SELECT 1 FROM dbo._CompetitiveEventConfig c WHERE c.EventID = r.ID);
        IF @MadnessEventID IS NULL
        BEGIN
            SET @MadnessEventID = 14;
            WHILE @MadnessEventID <= 255 AND
                 (EXISTS (SELECT 1 FROM dbo._CompetitiveEventConfig WHERE EventID = @MadnessEventID) OR
                  EXISTS (SELECT 1 FROM KMTGuard.dbo.Event_RegisterSettings WHERE ID = @MadnessEventID))
                SET @MadnessEventID += 1;
        END;
        IF @MadnessEventID > 255 THROW 51000, 'No free Event Register ID is available for Madness.', 1;
        INSERT dbo._CompetitiveEventConfig
            (EventCode, DisplayName, EventMode, EventID, ArenaWorldID, ArenaRegionID, ArenaX, ArenaY, ArenaZ,
             MadnessMobX, MadnessMobY, MadnessMobZ, MobSpawnDelaySeconds)
        VALUES
            (N'MADNESS', N'Madness Solo', N'MadnessSolo', @MadnessEventID, 108, 25584, 1165, 595, 1083,
             1646, 503, 636, 120);
    END;

    IF NOT EXISTS (SELECT 1 FROM dbo._CompetitiveEventConfig WHERE EventCode = N'DTT')
    BEGIN
        DECLARE @DttEventID int = NULL;
        IF OBJECT_ID(N'KMTGuard.dbo.Event_RegisterSettings', N'U') IS NOT NULL
            SELECT TOP (1) @DttEventID = r.ID
            FROM KMTGuard.dbo.Event_RegisterSettings r
            WHERE r.Name = N'Defend The Tower'
              AND NOT EXISTS (SELECT 1 FROM dbo._CompetitiveEventConfig c WHERE c.EventID = r.ID);
        IF @DttEventID IS NULL
        BEGIN
            SET @DttEventID = 14;
            WHILE @DttEventID <= 255 AND
                 (EXISTS (SELECT 1 FROM dbo._CompetitiveEventConfig WHERE EventID = @DttEventID) OR
                  EXISTS (SELECT 1 FROM KMTGuard.dbo.Event_RegisterSettings WHERE ID = @DttEventID))
                SET @DttEventID += 1;
        END;
        IF @DttEventID > 255 THROW 51000, 'No free Event Register ID is available for DTT.', 1;
        INSERT dbo._CompetitiveEventConfig
            (EventCode, DisplayName, EventMode, EventID, MinPlayers, ArenaWorldID, ArenaRegionID, ArenaX, ArenaY, ArenaZ,
             Team1X, Team1Y, Team1Z, Team2X, Team2Y, Team2Z, MobSpawnDelaySeconds,
             Team1TowerX, Team1TowerY, Team1TowerZ, Team2TowerX, Team2TowerY, Team2TowerZ)
        VALUES
            (N'DTT', N'Defend The Tower', N'DefendTower', @DttEventID, 4, 109, 32471, 960, 215, 960,
             960, 215, 262, 961, 215, 1646, 60, 960, 218, 603, 958, 218, 1289);
    END;

    IF EXISTS
    (
        SELECT EventID FROM dbo._CompetitiveEventConfig
        WHERE EventCode IN (N'LMS', N'MADNESS', N'DTT')
        GROUP BY EventID HAVING COUNT(*) > 1
    )
        THROW 51000, 'Competitive event EventID values must be unique.', 1;

    IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID(N'dbo._CompetitiveEventConfig') AND name = N'UX_CompetitiveEventConfig_EventID')
        CREATE UNIQUE INDEX UX_CompetitiveEventConfig_EventID ON dbo._CompetitiveEventConfig(EventID);

    IF OBJECT_ID(N'dbo._CompetitiveEventReward', N'U') IS NULL
    BEGIN
        CREATE TABLE dbo._CompetitiveEventReward
        (
            RewardID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_CompetitiveEventReward PRIMARY KEY,
            EventCode nvarchar(32) NOT NULL,
            Placement int NOT NULL,
            RewardType nvarchar(24) NOT NULL,
            Amount bigint NOT NULL CONSTRAINT DF_CompetitiveEventReward_Amount DEFAULT (0),
            ItemCodeName128 varchar(128) NULL,
            ItemID int NULL,
            ItemCount int NOT NULL CONSTRAINT DF_CompetitiveEventReward_ItemCount DEFAULT (1),
            Plus int NOT NULL CONSTRAINT DF_CompetitiveEventReward_Plus DEFAULT (0),
            IsActive bit NOT NULL CONSTRAINT DF_CompetitiveEventReward_Active DEFAULT (1),
            CreatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_CompetitiveEventReward_Created DEFAULT (SYSUTCDATETIME()),
            CONSTRAINT CK_CompetitiveEventReward_Placement CHECK (Placement > 0),
            CONSTRAINT CK_CompetitiveEventReward_Type CHECK (RewardType IN (N'SilkOwn', N'SilkGift', N'SilkPoint', N'Gold', N'ItemChest'))
        );
        CREATE INDEX IX_CompetitiveEventReward_EventPlacement ON dbo._CompetitiveEventReward(EventCode, Placement, IsActive);
    END;

    IF NOT EXISTS (SELECT 1 FROM dbo._CompetitiveEventReward WHERE EventCode=N'LMS' AND Placement=1)
        INSERT dbo._CompetitiveEventReward(EventCode,Placement,RewardType,Amount) VALUES(N'LMS',1,N'SilkOwn',100);
    IF NOT EXISTS (SELECT 1 FROM dbo._CompetitiveEventReward WHERE EventCode=N'MADNESS' AND Placement=1)
        INSERT dbo._CompetitiveEventReward(EventCode,Placement,RewardType,Amount) VALUES(N'MADNESS',1,N'SilkOwn',100);
    IF NOT EXISTS (SELECT 1 FROM dbo._CompetitiveEventReward WHERE EventCode=N'MADNESS' AND Placement=2)
        INSERT dbo._CompetitiveEventReward(EventCode,Placement,RewardType,Amount) VALUES(N'MADNESS',2,N'SilkOwn',50);
    IF NOT EXISTS (SELECT 1 FROM dbo._CompetitiveEventReward WHERE EventCode=N'MADNESS' AND Placement=3)
        INSERT dbo._CompetitiveEventReward(EventCode,Placement,RewardType,Amount) VALUES(N'MADNESS',3,N'SilkOwn',25);
    IF NOT EXISTS (SELECT 1 FROM dbo._CompetitiveEventReward WHERE EventCode=N'DTT' AND Placement=1)
        INSERT dbo._CompetitiveEventReward(EventCode,Placement,RewardType,Amount) VALUES(N'DTT',1,N'SilkOwn',500);
    IF NOT EXISTS (SELECT 1 FROM dbo._CompetitiveEventReward WHERE EventCode=N'DTT' AND Placement=2)
        INSERT dbo._CompetitiveEventReward(EventCode,Placement,RewardType,Amount) VALUES(N'DTT',2,N'SilkOwn',50);

    IF OBJECT_ID(N'dbo._CompetitiveEventSchedule', N'U') IS NULL
    BEGIN
        CREATE TABLE dbo._CompetitiveEventSchedule
        (
            ScheduleID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_CompetitiveEventSchedule PRIMARY KEY,
            EventCode nvarchar(32) NOT NULL,
            StartTime time(0) NOT NULL,
            DaysMask tinyint NOT NULL CONSTRAINT DF_CompetitiveEventSchedule_Days DEFAULT (127),
            IsActive bit NOT NULL CONSTRAINT DF_CompetitiveEventSchedule_Active DEFAULT (1),
            LastRunLocalDate date NULL,
            UpdatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_CompetitiveEventSchedule_Updated DEFAULT (SYSUTCDATETIME()),
            CONSTRAINT CK_CompetitiveEventSchedule_Days CHECK (DaysMask BETWEEN 1 AND 127)
        );
        CREATE INDEX IX_CompetitiveEventSchedule_Due ON dbo._CompetitiveEventSchedule(EventCode, IsActive, StartTime);
    END;

    IF OBJECT_ID(N'dbo._CompetitiveEventScore', N'U') IS NULL
    BEGIN
        CREATE TABLE dbo._CompetitiveEventScore
        (
            RunID bigint NOT NULL,
            EventCode nvarchar(32) NOT NULL,
            CharID int NOT NULL,
            CharName nvarchar(64) NOT NULL,
            Team tinyint NOT NULL,
            KillCount int NOT NULL CONSTRAINT DF_CompetitiveEventScore_Kills DEFAULT (0),
            IsAlive bit NOT NULL CONSTRAINT DF_CompetitiveEventScore_Alive DEFAULT (1),
            UpdatedAtUtc datetime2(3) NOT NULL CONSTRAINT DF_CompetitiveEventScore_Updated DEFAULT (SYSUTCDATETIME()),
            CONSTRAINT PK_CompetitiveEventScore PRIMARY KEY (RunID, CharID)
        );
    END;

    IF OBJECT_ID(N'dbo._CompetitiveEventKillLedger', N'U') IS NULL
    BEGIN
        CREATE TABLE dbo._CompetitiveEventKillLedger
        (
            RunID bigint NOT NULL,
            KillerCharID int NOT NULL,
            DeadCharID int NOT NULL,
            KillCount int NOT NULL,
            UpdatedAtUtc datetime2(3) NOT NULL CONSTRAINT DF_CompetitiveEventKillLedger_Updated DEFAULT (SYSUTCDATETIME()),
            CONSTRAINT PK_CompetitiveEventKillLedger PRIMARY KEY (RunID, KillerCharID, DeadCharID)
        );
    END;

    IF EXISTS
    (
        SELECT 1
        FROM dbo._CompetitiveEventConfig c
        JOIN KMTGuard.dbo.Event_RegisterSettings r ON r.ID = c.EventID
        WHERE c.EventCode IN (N'LMS',N'MADNESS',N'DTT')
          AND r.Name COLLATE DATABASE_DEFAULT NOT IN
              (c.DisplayName,
               CASE c.EventCode
                   WHEN N'LMS' THEN N'Last Man Standing'
                   WHEN N'MADNESS' THEN N'Madness Solo'
                   ELSE N'Defend The Tower'
               END)
    )
        THROW 51000, 'A competitive EventID is already assigned to another registered event.', 1;

    MERGE KMTGuard.dbo.Event_RegisterSettings AS target
    USING
    (
        SELECT EventID, DisplayName
        FROM dbo._CompetitiveEventConfig
        WHERE EventCode IN (N'LMS',N'MADNESS',N'DTT')
    ) AS source ON target.ID = source.EventID
    WHEN MATCHED THEN
        UPDATE SET Name=source.DisplayName, Description=N'KMTGuard competitive arena event'
    WHEN NOT MATCHED THEN
        INSERT(ID,Name,Description) VALUES(source.EventID,source.DisplayName,N'KMTGuard competitive arena event');

    COMMIT TRANSACTION;
END TRY
BEGIN CATCH
    IF @@TRANCOUNT > 0 ROLLBACK TRANSACTION;
    THROW;
END CATCH;
GO

SELECT EventCode, DisplayName, EventID, Enabled, ArenaWorldID, ArenaRegionID
FROM dbo._CompetitiveEventConfig
WHERE EventCode IN (N'LMS',N'MADNESS',N'DTT')
ORDER BY EventID;
GO
