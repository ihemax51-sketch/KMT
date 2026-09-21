USE [master];
GO

IF DB_ID(N'Events') IS NULL
    CREATE DATABASE [Events];
GO

USE [Events];
GO

IF OBJECT_ID(N'dbo.Event_Control', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Event_Control
    (
        EventID int NOT NULL CONSTRAINT PK_Event_Control PRIMARY KEY,
        EventCode sysname NOT NULL,
        IsOpen bit NOT NULL CONSTRAINT DF_Event_Control_IsOpen DEFAULT (0),
        StartUtc datetime2(3) NULL,
        EndUtc datetime2(3) NULL,
        UpdatedAtUtc datetime2(3) NOT NULL CONSTRAINT DF_Event_Control_Updated DEFAULT (SYSUTCDATETIME())
    );
END;
GO

IF OBJECT_ID(N'dbo.Event_RegPlayers', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Event_RegPlayers
    (
        ID bigint IDENTITY(1,1) NOT NULL CONSTRAINT PK_Event_RegPlayers PRIMARY KEY,
        EventID int NOT NULL,
        EventCode sysname NOT NULL,
        EventStartUtc datetime2(3) NOT NULL,
        CharID int NOT NULL,
        CharName nvarchar(64) NOT NULL,
        Amount bigint NOT NULL CONSTRAINT DF_Event_RegPlayers_Amount DEFAULT (0),
        Currency nvarchar(24) NOT NULL CONSTRAINT DF_Event_RegPlayers_Currency DEFAULT (N'silk'),
        RegisteredAtUtc datetime2(3) NOT NULL CONSTRAINT DF_Event_RegPlayers_Registered DEFAULT (SYSUTCDATETIME())
    );
END;
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id=OBJECT_ID(N'dbo.Event_RegPlayers') AND name=N'UX_Event_RegPlayers_RoundChar')
    CREATE UNIQUE INDEX UX_Event_RegPlayers_RoundChar ON dbo.Event_RegPlayers(EventID, EventStartUtc, CharID);
GO

IF OBJECT_ID(N'dbo.Event_Wins', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Event_Wins
    (
        WinID bigint IDENTITY(1,1) NOT NULL CONSTRAINT PK_Event_Wins PRIMARY KEY,
        EventID int NOT NULL,
        EventCode sysname NOT NULL,
        EventStartUtc datetime2(3) NOT NULL,
        EventEndUtc datetime2(3) NOT NULL,
        WinnerCharID int NOT NULL,
        WinnerCharName nvarchar(64) NOT NULL,
        WinnerPartyNo int NULL,
        TotalPot bigint NOT NULL CONSTRAINT DF_Event_Wins_TotalPot DEFAULT (0),
        Currency nvarchar(24) NOT NULL CONSTRAINT DF_Event_Wins_Currency DEFAULT (N'silk'),
        CreatedAtUtc datetime2(3) NOT NULL CONSTRAINT DF_Event_Wins_Created DEFAULT (SYSUTCDATETIME())
    );
END;
GO

IF OBJECT_ID(N'dbo._SurvivalSoloConfig', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._SurvivalSoloConfig
    (
        EventCode nvarchar(32) NOT NULL CONSTRAINT PK_SurvivalSoloConfig PRIMARY KEY,
        DisplayName nvarchar(64) NOT NULL,
        Enabled bit NOT NULL CONSTRAINT DF_SurvivalSoloConfig_Enabled DEFAULT (1),
        EventID int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_EventID DEFAULT (13),
        StartDelaySeconds int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_StartDelay DEFAULT (60),
        RegistrationSeconds int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_Registration DEFAULT (60),
        FightSeconds int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_Fight DEFAULT (600),
        MinLevel int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_MinLevel DEFAULT (1),
        HwidLimit int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_HwidLimit DEFAULT (1),
        RequireHwid bit NOT NULL CONSTRAINT DF_SurvivalSoloConfig_RequireHwid DEFAULT (1),
        MaxPlayers int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_MaxPlayers DEFAULT (100),
        ArenaWorldID int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_ArenaWorld DEFAULT (107),
        ArenaRegionID int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_ArenaRegion DEFAULT (25580),
        ArenaX int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_ArenaX DEFAULT (500),
        ArenaY int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_ArenaY DEFAULT (0),
        ArenaZ int NOT NULL CONSTRAINT DF_SurvivalSoloConfig_ArenaZ DEFAULT (500),
        UpdatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_SurvivalSoloConfig_Updated DEFAULT (SYSUTCDATETIME())
    );
END;
GO

IF NOT EXISTS (SELECT 1 FROM dbo._SurvivalSoloConfig WHERE EventCode=N'SSOLO')
BEGIN
    INSERT dbo._SurvivalSoloConfig
        (EventCode,DisplayName,Enabled,EventID,StartDelaySeconds,RegistrationSeconds,FightSeconds,
         MinLevel,HwidLimit,RequireHwid,MaxPlayers,ArenaWorldID,ArenaRegionID,ArenaX,ArenaY,ArenaZ)
    VALUES
        (N'SSOLO',N'Survival Solo',1,13,60,60,600,1,1,1,100,107,25580,500,0,500);
END;
GO

IF OBJECT_ID(N'dbo._SurvivalSoloReward', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._SurvivalSoloReward
    (
        RewardID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_SurvivalSoloReward PRIMARY KEY,
        Placement int NOT NULL,
        RewardType nvarchar(24) NOT NULL,
        Amount bigint NOT NULL CONSTRAINT DF_SurvivalSoloReward_Amount DEFAULT (0),
        ItemCodeName128 varchar(128) NULL,
        ItemID int NULL,
        ItemCount int NOT NULL CONSTRAINT DF_SurvivalSoloReward_ItemCount DEFAULT (1),
        Plus int NOT NULL CONSTRAINT DF_SurvivalSoloReward_Plus DEFAULT (0),
        IsActive bit NOT NULL CONSTRAINT DF_SurvivalSoloReward_Active DEFAULT (1),
        CreatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_SurvivalSoloReward_Created DEFAULT (SYSUTCDATETIME())
    );
END;
GO

IF NOT EXISTS (SELECT 1 FROM dbo._SurvivalSoloReward WHERE Placement=1 AND IsActive=1)
    INSERT dbo._SurvivalSoloReward (Placement,RewardType,Amount,ItemCount,Plus) VALUES (1,N'SilkOwn',500,1,0);
IF NOT EXISTS (SELECT 1 FROM dbo._SurvivalSoloReward WHERE Placement=2 AND IsActive=1)
    INSERT dbo._SurvivalSoloReward (Placement,RewardType,Amount,ItemCount,Plus) VALUES (2,N'SilkOwn',50,1,0);
GO

IF OBJECT_ID(N'dbo._SurvivalSoloSchedule', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._SurvivalSoloSchedule
    (
        ScheduleID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_SurvivalSoloSchedule PRIMARY KEY,
        StartTime time(0) NOT NULL,
        DaysMask tinyint NOT NULL CONSTRAINT DF_SurvivalSoloSchedule_DaysMask DEFAULT (127),
        IsActive bit NOT NULL CONSTRAINT DF_SurvivalSoloSchedule_Active DEFAULT (1),
        LastRunLocalDate date NULL,
        UpdatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_SurvivalSoloSchedule_Updated DEFAULT (SYSUTCDATETIME())
    );
END;
GO

IF OBJECT_ID(N'dbo.SSOLO_Score', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.SSOLO_Score
    (
        RunID bigint NOT NULL,
        EventStartUtc datetime2(3) NOT NULL,
        PlayerCharID int NOT NULL,
        PlayerName nvarchar(64) NOT NULL,
        Team int NOT NULL,
        KillCount int NOT NULL CONSTRAINT DF_SSOLO_Score_KillCount DEFAULT (0),
        UpdatedAtUtc datetime2(3) NOT NULL CONSTRAINT DF_SSOLO_Score_Updated DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT PK_SSOLO_Score PRIMARY KEY (RunID, PlayerCharID)
    );
END;
GO

IF OBJECT_ID(N'dbo.survival_solo_kill', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.survival_solo_kill
    (
        ID bigint IDENTITY(1,1) NOT NULL CONSTRAINT PK_survival_solo_kill PRIMARY KEY,
        CreatedAtUtc datetime2(3) NOT NULL CONSTRAINT DF_survival_solo_kill_Created DEFAULT (SYSUTCDATETIME())
    );
END;
GO

USE [KMTGuard];
GO

IF OBJECT_ID(N'dbo.Event_CurrentTeams', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Event_CurrentTeams
    (
        CharID int NOT NULL CONSTRAINT PK_Event_CurrentTeams PRIMARY KEY,
        CharName varchar(25) NOT NULL,
        Team tinyint NOT NULL,
        EventName varchar(32) NULL
    );
END;
GO

IF OBJECT_ID(N'dbo.Event_RegisterSettings', N'U') IS NOT NULL
BEGIN
    IF EXISTS
    (
        SELECT 1
        FROM dbo.Event_RegisterSettings
        WHERE ID = 13
          AND ISNULL(Name, N'') <> N'Survival Solo'
    )
        THROW 51013, 'Event ID 13 is already used. Change the Survival Solo EventID in this script before running it.', 1;

    MERGE dbo.Event_RegisterSettings AS target
    USING (SELECT 13 AS ID) AS source
    ON target.ID=source.ID
    WHEN MATCHED THEN UPDATE SET Name=N'Survival Solo', Description=N'Individual free-for-all survival arena event'
    WHEN NOT MATCHED THEN INSERT (ID,Name,Description) VALUES (13,N'Survival Solo',N'Individual free-for-all survival arena event');
END;
GO
