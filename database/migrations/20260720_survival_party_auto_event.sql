USE [master]
GO

IF DB_ID(N'Events') IS NULL
    CREATE DATABASE [Events];
GO

USE [Events]
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
END
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
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id = OBJECT_ID(N'dbo.Event_RegPlayers') AND name = N'UX_Event_RegPlayers_RoundChar')
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
END
GO

IF OBJECT_ID(N'dbo.SPARTY_Score', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.SPARTY_Score
    (
        RunID bigint NOT NULL,
        EventStartUtc datetime2(3) NOT NULL,
        PartyID int NOT NULL,
        MasterCharID int NOT NULL,
        MasterName nvarchar(64) NOT NULL,
        Team int NOT NULL,
        KillCount int NOT NULL CONSTRAINT DF_SPARTY_Score_KillCount DEFAULT (0),
        UpdatedAtUtc datetime2(3) NOT NULL CONSTRAINT DF_SPARTY_Score_Updated DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT PK_SPARTY_Score PRIMARY KEY (RunID, PartyID)
    );
END
GO

IF OBJECT_ID(N'dbo.survival_kill', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.survival_kill
    (
        ID bigint IDENTITY(1,1) NOT NULL CONSTRAINT PK_survival_kill PRIMARY KEY,
        CreatedAtUtc datetime2(3) NOT NULL CONSTRAINT DF_survival_kill_Created DEFAULT (SYSUTCDATETIME())
    );
END
GO

IF OBJECT_ID(N'dbo._SurvivalPartyConfig', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._SurvivalPartyConfig
    (
        EventCode nvarchar(32) NOT NULL CONSTRAINT PK_SurvivalPartyConfig PRIMARY KEY,
        DisplayName nvarchar(64) NOT NULL,
        Enabled bit NOT NULL CONSTRAINT DF_SurvivalPartyConfig_Enabled DEFAULT (1),
        EventID int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_EventID DEFAULT (12),
        StartDelaySeconds int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_StartDelay DEFAULT (60),
        RegistrationSeconds int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_Registration DEFAULT (60),
        FightSeconds int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_Fight DEFAULT (600),
        MinLevel int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_MinLevel DEFAULT (1),
        HwidLimit int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_HwidLimit DEFAULT (1),
        RequireHwid bit NOT NULL CONSTRAINT DF_SurvivalPartyConfig_RequireHwid DEFAULT (1),
        MaxPlayers int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_MaxPlayers DEFAULT (100),
        ArenaWorldID int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_ArenaWorld DEFAULT (107),
        ArenaRegionID int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_ArenaRegion DEFAULT (25580),
        ArenaX int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_ArenaX DEFAULT (500),
        ArenaY int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_ArenaY DEFAULT (0),
        ArenaZ int NOT NULL CONSTRAINT DF_SurvivalPartyConfig_ArenaZ DEFAULT (500),
        UpdatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_SurvivalPartyConfig_Updated DEFAULT (SYSUTCDATETIME())
    );
END
GO

IF COL_LENGTH(N'dbo._SurvivalPartyConfig', N'DisplayName') IS NULL
    ALTER TABLE dbo._SurvivalPartyConfig ADD DisplayName nvarchar(64) NOT NULL DEFAULT (N'Survival Party') WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyConfig', N'Enabled') IS NULL
    ALTER TABLE dbo._SurvivalPartyConfig ADD Enabled bit NOT NULL DEFAULT (1) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyConfig', N'EventID') IS NULL
    ALTER TABLE dbo._SurvivalPartyConfig ADD EventID int NOT NULL DEFAULT (12) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyConfig', N'StartDelaySeconds') IS NULL
    ALTER TABLE dbo._SurvivalPartyConfig ADD StartDelaySeconds int NOT NULL DEFAULT (60) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyConfig', N'RegistrationSeconds') IS NULL
    ALTER TABLE dbo._SurvivalPartyConfig ADD RegistrationSeconds int NOT NULL DEFAULT (60) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyConfig', N'FightSeconds') IS NULL
    ALTER TABLE dbo._SurvivalPartyConfig ADD FightSeconds int NOT NULL DEFAULT (600) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyConfig', N'MinLevel') IS NULL
    ALTER TABLE dbo._SurvivalPartyConfig ADD MinLevel int NOT NULL DEFAULT (1) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyConfig', N'HwidLimit') IS NULL
    ALTER TABLE dbo._SurvivalPartyConfig ADD HwidLimit int NOT NULL DEFAULT (1) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyConfig', N'RequireHwid') IS NULL
    ALTER TABLE dbo._SurvivalPartyConfig ADD RequireHwid bit NOT NULL DEFAULT (1) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyConfig', N'MaxPlayers') IS NULL
    ALTER TABLE dbo._SurvivalPartyConfig ADD MaxPlayers int NOT NULL DEFAULT (100) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyConfig', N'ArenaWorldID') IS NULL
    ALTER TABLE dbo._SurvivalPartyConfig ADD ArenaWorldID int NOT NULL DEFAULT (107) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyConfig', N'ArenaRegionID') IS NULL
    ALTER TABLE dbo._SurvivalPartyConfig ADD ArenaRegionID int NOT NULL DEFAULT (25580) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyConfig', N'ArenaX') IS NULL
    ALTER TABLE dbo._SurvivalPartyConfig ADD ArenaX int NOT NULL DEFAULT (500) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyConfig', N'ArenaY') IS NULL
    ALTER TABLE dbo._SurvivalPartyConfig ADD ArenaY int NOT NULL DEFAULT (0) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyConfig', N'ArenaZ') IS NULL
    ALTER TABLE dbo._SurvivalPartyConfig ADD ArenaZ int NOT NULL DEFAULT (500) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyConfig', N'UpdatedAtUtc') IS NULL
    ALTER TABLE dbo._SurvivalPartyConfig ADD UpdatedAtUtc datetime2(0) NOT NULL DEFAULT (SYSUTCDATETIME()) WITH VALUES;
GO

IF NOT EXISTS (SELECT 1 FROM dbo._SurvivalPartyConfig WITH (NOLOCK) WHERE EventCode = N'SPARTY')
BEGIN
    INSERT INTO dbo._SurvivalPartyConfig
        (EventCode, DisplayName, Enabled, EventID, StartDelaySeconds, RegistrationSeconds, FightSeconds,
         MinLevel, HwidLimit, RequireHwid, MaxPlayers, ArenaWorldID, ArenaRegionID, ArenaX, ArenaY, ArenaZ)
    VALUES
        (N'SPARTY', N'Survival Party', 1, 12, 60, 60, 600,
         1, 1, 1, 100, 107, 25580, 500, 0, 500);
END
GO

IF OBJECT_ID(N'dbo._SurvivalPartyReward', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._SurvivalPartyReward
    (
        RewardID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_SurvivalPartyReward PRIMARY KEY,
        Placement int NOT NULL,
        RewardType nvarchar(24) NOT NULL,
        Amount bigint NOT NULL CONSTRAINT DF_SurvivalPartyReward_Amount DEFAULT (0),
        ItemCodeName128 varchar(128) NULL,
        ItemID int NULL,
        ItemCount int NOT NULL CONSTRAINT DF_SurvivalPartyReward_ItemCount DEFAULT (1),
        Plus int NOT NULL CONSTRAINT DF_SurvivalPartyReward_Plus DEFAULT (0),
        IsActive bit NOT NULL CONSTRAINT DF_SurvivalPartyReward_Active DEFAULT (1),
        CreatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_SurvivalPartyReward_Created DEFAULT (SYSUTCDATETIME())
    );
END
GO

IF COL_LENGTH(N'dbo._SurvivalPartyReward', N'Placement') IS NULL
    ALTER TABLE dbo._SurvivalPartyReward ADD Placement int NOT NULL DEFAULT (1) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyReward', N'RewardType') IS NULL
    ALTER TABLE dbo._SurvivalPartyReward ADD RewardType nvarchar(24) NOT NULL DEFAULT (N'SilkOwn') WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyReward', N'Amount') IS NULL
    ALTER TABLE dbo._SurvivalPartyReward ADD Amount bigint NOT NULL DEFAULT (0) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyReward', N'ItemID') IS NULL
    ALTER TABLE dbo._SurvivalPartyReward ADD ItemID int NULL;
IF COL_LENGTH(N'dbo._SurvivalPartyReward', N'ItemCodeName128') IS NULL
    ALTER TABLE dbo._SurvivalPartyReward ADD ItemCodeName128 varchar(128) NULL;
IF COL_LENGTH(N'dbo._SurvivalPartyReward', N'ItemCount') IS NULL
    ALTER TABLE dbo._SurvivalPartyReward ADD ItemCount int NOT NULL DEFAULT (1) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyReward', N'Plus') IS NULL
    ALTER TABLE dbo._SurvivalPartyReward ADD Plus int NOT NULL DEFAULT (0) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyReward', N'IsActive') IS NULL
    ALTER TABLE dbo._SurvivalPartyReward ADD IsActive bit NOT NULL DEFAULT (1) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartyReward', N'CreatedAtUtc') IS NULL
    ALTER TABLE dbo._SurvivalPartyReward ADD CreatedAtUtc datetime2(0) NOT NULL DEFAULT (SYSUTCDATETIME()) WITH VALUES;
GO

IF NOT EXISTS (SELECT 1 FROM dbo._SurvivalPartyReward WITH (NOLOCK) WHERE Placement = 1 AND IsActive = 1)
    INSERT INTO dbo._SurvivalPartyReward (Placement, RewardType, Amount, ItemCodeName128, ItemCount, Plus)
    VALUES (1, N'SilkOwn', 500, NULL, 1, 0);

IF NOT EXISTS (SELECT 1 FROM dbo._SurvivalPartyReward WITH (NOLOCK) WHERE Placement = 2 AND IsActive = 1)
    INSERT INTO dbo._SurvivalPartyReward (Placement, RewardType, Amount, ItemCodeName128, ItemCount, Plus)
    VALUES (2, N'SilkOwn', 50, NULL, 1, 0);
GO

IF OBJECT_ID(N'dbo._SurvivalPartySchedule', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._SurvivalPartySchedule
    (
        ScheduleID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_SurvivalPartySchedule PRIMARY KEY,
        StartTime time(0) NOT NULL,
        DaysMask tinyint NOT NULL CONSTRAINT DF_SurvivalPartySchedule_DaysMask DEFAULT (127),
        IsActive bit NOT NULL CONSTRAINT DF_SurvivalPartySchedule_Active DEFAULT (1),
        LastRunLocalDate date NULL,
        UpdatedAtUtc datetime2(0) NOT NULL CONSTRAINT DF_SurvivalPartySchedule_Updated DEFAULT (SYSUTCDATETIME())
    );
END
GO

IF COL_LENGTH(N'dbo._SurvivalPartySchedule', N'StartTime') IS NULL
    ALTER TABLE dbo._SurvivalPartySchedule ADD StartTime time(0) NOT NULL DEFAULT ('20:00') WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartySchedule', N'DaysMask') IS NULL
    ALTER TABLE dbo._SurvivalPartySchedule ADD DaysMask tinyint NOT NULL DEFAULT (127) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartySchedule', N'IsActive') IS NULL
    ALTER TABLE dbo._SurvivalPartySchedule ADD IsActive bit NOT NULL DEFAULT (1) WITH VALUES;
IF COL_LENGTH(N'dbo._SurvivalPartySchedule', N'LastRunLocalDate') IS NULL
    ALTER TABLE dbo._SurvivalPartySchedule ADD LastRunLocalDate date NULL;
IF COL_LENGTH(N'dbo._SurvivalPartySchedule', N'UpdatedAtUtc') IS NULL
    ALTER TABLE dbo._SurvivalPartySchedule ADD UpdatedAtUtc datetime2(0) NOT NULL DEFAULT (SYSUTCDATETIME()) WITH VALUES;
GO

IF OBJECT_ID(N'dbo._AutoEventConfig', N'U') IS NOT NULL
BEGIN
    DELETE FROM dbo._AutoEventConfig WHERE EventCode = N'SPARTY';

    DECLARE @ColumnName sysname;
    DECLARE @ConstraintName sysname;
    DECLARE @DropSql nvarchar(max);

    DECLARE SurvivalColumns CURSOR LOCAL FAST_FORWARD FOR
        SELECT ColName
        FROM (VALUES
            (N'SurvivalEventID'),
            (N'SurvivalRegistrationSeconds'),
            (N'SurvivalMaxPlayers'),
            (N'SurvivalArenaWorldID'),
            (N'SurvivalArenaRegionID'),
            (N'SurvivalArenaX'),
            (N'SurvivalArenaY'),
            (N'SurvivalArenaZ')
        ) AS v(ColName)
        WHERE COL_LENGTH(N'dbo._AutoEventConfig', v.ColName) IS NOT NULL;

    OPEN SurvivalColumns;
    FETCH NEXT FROM SurvivalColumns INTO @ColumnName;

    WHILE @@FETCH_STATUS = 0
    BEGIN
        SET @ConstraintName = NULL;

        SELECT @ConstraintName = dc.name
        FROM sys.default_constraints dc
        INNER JOIN sys.columns c
                ON c.default_object_id = dc.object_id
               AND c.object_id = dc.parent_object_id
        WHERE dc.parent_object_id = OBJECT_ID(N'dbo._AutoEventConfig')
          AND c.name = @ColumnName;

        IF @ConstraintName IS NOT NULL
        BEGIN
            SET @DropSql = N'ALTER TABLE dbo._AutoEventConfig DROP CONSTRAINT ' + QUOTENAME(@ConstraintName);
            EXEC (@DropSql);
        END

        SET @DropSql = N'ALTER TABLE dbo._AutoEventConfig DROP COLUMN ' + QUOTENAME(@ColumnName);
        EXEC (@DropSql);

        FETCH NEXT FROM SurvivalColumns INTO @ColumnName;
    END

    CLOSE SurvivalColumns;
    DEALLOCATE SurvivalColumns;
END
GO

IF OBJECT_ID(N'dbo._AutoEventReward', N'U') IS NOT NULL
    DELETE FROM dbo._AutoEventReward WHERE EventCode = N'SPARTY';
GO

USE [KMTGuard]
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
END
GO

IF COL_LENGTH(N'dbo.Event_CurrentTeams', N'CharName') IS NULL
    ALTER TABLE dbo.Event_CurrentTeams ADD CharName varchar(25) NOT NULL DEFAULT ('') WITH VALUES;
IF COL_LENGTH(N'dbo.Event_CurrentTeams', N'EventName') IS NULL
    ALTER TABLE dbo.Event_CurrentTeams ADD EventName varchar(32) NULL;
GO

IF OBJECT_ID(N'dbo.Event_RegisterSettings', N'U') IS NOT NULL
BEGIN
    IF EXISTS
    (
        SELECT 1
        FROM dbo.Event_RegisterSettings
        WHERE ID = 12
          AND ISNULL(Name, N'') <> N'Survival Party'
    )
        THROW 51012, 'Event ID 12 is already used. Change the Survival Party EventID in this script before running it.', 1;

    MERGE dbo.Event_RegisterSettings AS target
    USING (SELECT 12 AS ID) AS source
    ON target.ID = source.ID
    WHEN MATCHED THEN UPDATE SET Name = N'Survival Party', Description = N'Party survival arena event'
    WHEN NOT MATCHED THEN INSERT (ID, Name, Description) VALUES (12, N'Survival Party', N'Party survival arena event');
END
GO
