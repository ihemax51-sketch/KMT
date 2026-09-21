USE [master];
GO

IF DB_ID(N'Events') IS NULL
    CREATE DATABASE [Events];
GO

USE [KMTGuard];
GO

IF OBJECT_ID(N'dbo.Clientless_Accounts', N'U') IS NOT NULL
   AND COL_LENGTH(N'dbo.Clientless_Accounts', N'SystemRole') IS NULL
BEGIN
    ALTER TABLE dbo.Clientless_Accounts
        ADD SystemRole varchar(32) NULL;
END;
GO

IF OBJECT_ID(N'dbo.Clientless_Accounts', N'U') IS NOT NULL
   AND NOT EXISTS
   (
       SELECT 1
       FROM sys.indexes
       WHERE object_id = OBJECT_ID(N'dbo.Clientless_Accounts')
         AND name = N'UX_ClientlessAccounts_SystemRole'
   )
BEGIN
    CREATE UNIQUE INDEX UX_ClientlessAccounts_SystemRole
        ON dbo.Clientless_Accounts(SystemRole)
        WHERE SystemRole IS NOT NULL;
END;
GO

USE [Events];
GO

IF OBJECT_ID(N'dbo._HideAndSeekConfig', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._HideAndSeekConfig
    (
        EventCode nvarchar(32) NOT NULL
            CONSTRAINT PK_HideAndSeekConfig PRIMARY KEY,
        DisplayName nvarchar(64) NOT NULL,
        Enabled bit NOT NULL
            CONSTRAINT DF_HideAndSeekConfig_Enabled DEFAULT (1),
        StartDelaySeconds int NOT NULL
            CONSTRAINT DF_HideAndSeekConfig_StartDelay DEFAULT (60),
        SearchSeconds int NOT NULL
            CONSTRAINT DF_HideAndSeekConfig_SearchSeconds DEFAULT (600),
        ReminderIntervalSeconds int NOT NULL
            CONSTRAINT DF_HideAndSeekConfig_Reminder DEFAULT (120),
        MinLevel int NOT NULL
            CONSTRAINT DF_HideAndSeekConfig_MinLevel DEFAULT (1),
        HwidLimit int NOT NULL
            CONSTRAINT DF_HideAndSeekConfig_HwidLimit DEFAULT (1),
        RequireHwid bit NOT NULL
            CONSTRAINT DF_HideAndSeekConfig_RequireHwid DEFAULT (0),
        BotAccountName varchar(24) NOT NULL,
        BotPassword varchar(64) NULL,
        BotCharacterName varchar(16) NOT NULL,
        BotShardID smallint NOT NULL
            CONSTRAINT DF_HideAndSeekConfig_ShardID DEFAULT (64),
        BotLocale tinyint NOT NULL
            CONSTRAINT DF_HideAndSeekConfig_Locale DEFAULT (22),
        ReturnWorldID int NOT NULL
            CONSTRAINT DF_HideAndSeekConfig_ReturnWorld DEFAULT (1),
        ReturnRegionID int NOT NULL
            CONSTRAINT DF_HideAndSeekConfig_ReturnRegion DEFAULT (25000),
        ReturnX int NOT NULL
            CONSTRAINT DF_HideAndSeekConfig_ReturnX DEFAULT (982),
        ReturnY int NOT NULL
            CONSTRAINT DF_HideAndSeekConfig_ReturnY DEFAULT (0),
        ReturnZ int NOT NULL
            CONSTRAINT DF_HideAndSeekConfig_ReturnZ DEFAULT (140),
        UpdatedAtUtc datetime2(0) NOT NULL
            CONSTRAINT DF_HideAndSeekConfig_Updated DEFAULT (SYSUTCDATETIME())
    );
END;
GO

IF NOT EXISTS (SELECT 1 FROM dbo._HideAndSeekConfig WHERE EventCode = N'HNS')
BEGIN
    INSERT INTO dbo._HideAndSeekConfig
    (
        EventCode,
        DisplayName,
        Enabled,
        StartDelaySeconds,
        SearchSeconds,
        ReminderIntervalSeconds,
        MinLevel,
        HwidLimit,
        RequireHwid,
        BotAccountName,
        BotCharacterName,
        BotShardID,
        BotLocale,
        ReturnWorldID,
        ReturnRegionID,
        ReturnX,
        ReturnY,
        ReturnZ
    )
    VALUES
    (
        N'HNS',
        N'Hide and Seek',
        1,
        60,
        600,
        120,
        1,
        1,
        0,
        'kmthnsmaster',
        'KMT_HideMaster',
        64,
        22,
        1,
        25000,
        982,
        0,
        140
    );
END;
GO

IF OBJECT_ID(N'dbo._HideAndSeekLocation', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._HideAndSeekLocation
    (
        LocationID int IDENTITY(1,1) NOT NULL
            CONSTRAINT PK_HideAndSeekLocation PRIMARY KEY,
        LocationName nvarchar(128) NOT NULL,
        WorldID int NOT NULL,
        RegionID int NOT NULL,
        PosX int NOT NULL,
        PosY int NOT NULL,
        PosZ int NOT NULL,
        Weight int NOT NULL
            CONSTRAINT DF_HideAndSeekLocation_Weight DEFAULT (1),
        IsActive bit NOT NULL
            CONSTRAINT DF_HideAndSeekLocation_Active DEFAULT (1),
        LastUsedAtUtc datetime2(0) NULL,
        CreatedAtUtc datetime2(0) NOT NULL
            CONSTRAINT DF_HideAndSeekLocation_Created DEFAULT (SYSUTCDATETIME()),
        UpdatedAtUtc datetime2(0) NOT NULL
            CONSTRAINT DF_HideAndSeekLocation_Updated DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT CK_HideAndSeekLocation_World CHECK (WorldID > 0),
        CONSTRAINT CK_HideAndSeekLocation_Region CHECK (RegionID > 0),
        CONSTRAINT CK_HideAndSeekLocation_Weight CHECK (Weight BETWEEN 1 AND 1000)
    );
END;
GO

IF NOT EXISTS (SELECT 1 FROM dbo._HideAndSeekLocation)
BEGIN
    INSERT INTO dbo._HideAndSeekLocation
        (LocationName, WorldID, RegionID, PosX, PosY, PosZ)
    VALUES
        (N'Yeohas Forest at Jangan', 1, 23971, 93, 1210, 4),
        (N'North-Tiger Mountain at Jangan', 1, 23710, 940, 1021, 1169),
        (N'North-Tiger Mountain at Jangan', 1, 23200, 210, 1411, 889),
        (N'North-Tiger Mountain at Jangan', 1, 23201, 323, 1364, 868),
        (N'North-Tiger Mountain at Jangan', 1, 24221, 1814, 755, 686),
        (N'North-Tiger Mountain at Jangan', 1, 23965, 1721, 765, 1297),
        (N'North-Tiger Mountain at Jangan', 1, 23708, 803, 775, 1532),
        (N'North-Tiger Mountain at Jangan', 1, 23708, 1549, 859, 30),
        (N'North-Tiger Mountain at Jangan', 1, 23453, 1903, 867, 670),
        (N'North-Tiger Mountain at Jangan', 1, 23453, 978, 669, 880),
        (N'North-Tiger Mountain at Jangan', 1, 23196, 1592, 931, 1401),
        (N'North-Tiger Mountain at Jangan', 1, 23452, 1373, 903, 956),
        (N'North-Tiger Mountain at Jangan', 1, 23197, 545, 899, 654),
        (N'North-Tiger Mountain at Jangan', 1, 23198, 419, 901, 576),
        (N'North-Tiger Mountain at Jangan', 1, 23198, 904, 1015, 1498),
        (N'North-Tiger Mountain at Jangan', 1, 23199, 43, 1108, 519),
        (N'North-Tiger Mountain at Jangan', 1, 23458, 682, 1280, 628),
        (N'North-Tiger Mountain at Jangan', 1, 23459, 405, 1061, 268),
        (N'North-Tiger Mountain at Jangan', 1, 24222, 1669, 1017, 297),
        (N'North-Tiger Mountain at Jangan', 1, 23966, 513, 825, 129),
        (N'North-Tiger Mountain at Jangan', 1, 23708, 1576, 761, 1207),
        (N'North-Tiger Mountain at Jangan', 1, 24220, 1198, 228, 705),
        (N'North-Tiger Mountain at Jangan', 1, 23453, 1031, 676, 1414),
        (N'Bandit Mountain Stronghold at Jangan', 1, 23455, 566, 981, 1658),
        (N'Bandit Mountain Stronghold at Jangan', 1, 23712, 778, 1414, 711),
        (N'Bandit Mountain Stronghold at Jangan', 1, 23713, 186, 1402, 532),
        (N'Bandit Mountain Stronghold at Jangan', 1, 23457, 1062, 1428, 1645),
        (N'North-Tiger Mountain at Jangan', 1, 23714, 548, 1303, 0),
        (N'Bandit Mountain Stronghold at Jangan', 1, 23712, 1434, 1401, 672),
        (N'Bandit Mountain Stronghold at Jangan', 1, 23455, 1303, 1403, 1442),
        (N'North-Tiger Mountain at Jangan', 1, 24477, 1801, 604, 630),
        (N'North-Tiger Mountain at Jangan', 1, 24221, 698, 422, 1711),
        (N'North-Tiger Mountain at Jangan', 1, 23454, 614, 837, 1821),
        (N'North-Tiger Mountain at Jangan', 1, 23196, 724, 1179, 1663),
        (N'Bandit Mountain Stronghold at Jangan', 1, 23457, 306, 1402, 1367),
        (N'Bandit Mountain Stronghold at Jangan', 1, 23457, 194, 1406, 1716),
        (N'North-Tiger Mountain at Jangan', 1, 23202, 1717, 1129, 1054),
        (N'North-Tiger Mountain at Jangan', 1, 23203, 991, 765, 885),
        (N'Bandit Mountain Stronghold at Jangan', 1, 23456, 1612, 1407, 1053),
        (N'Jangan Ferry', 1, 24734, 0, 298, 422),
        (N'Jangan Ferry', 1, 24992, 955, 247, 368);
END;
GO

IF OBJECT_ID(N'dbo._HideAndSeekReward', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._HideAndSeekReward
    (
        RewardID int IDENTITY(1,1) NOT NULL
            CONSTRAINT PK_HideAndSeekReward PRIMARY KEY,
        RewardType nvarchar(24) NOT NULL,
        Amount bigint NOT NULL
            CONSTRAINT DF_HideAndSeekReward_Amount DEFAULT (0),
        ItemCodeName128 varchar(128) NULL,
        ItemID int NULL,
        ItemCount int NOT NULL
            CONSTRAINT DF_HideAndSeekReward_ItemCount DEFAULT (1),
        Plus int NOT NULL
            CONSTRAINT DF_HideAndSeekReward_Plus DEFAULT (0),
        IsActive bit NOT NULL
            CONSTRAINT DF_HideAndSeekReward_Active DEFAULT (1),
        CreatedAtUtc datetime2(0) NOT NULL
            CONSTRAINT DF_HideAndSeekReward_Created DEFAULT (SYSUTCDATETIME())
    );
END;
GO

IF NOT EXISTS (SELECT 1 FROM dbo._HideAndSeekReward)
BEGIN
    INSERT INTO dbo._HideAndSeekReward
        (RewardType, Amount, ItemCount, Plus, IsActive)
    VALUES
        (N'SilkOwn', 100, 1, 0, 1);
END;
GO

IF OBJECT_ID(N'dbo._HideAndSeekSchedule', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._HideAndSeekSchedule
    (
        ScheduleID int IDENTITY(1,1) NOT NULL
            CONSTRAINT PK_HideAndSeekSchedule PRIMARY KEY,
        StartTime time(0) NOT NULL,
        DaysMask tinyint NOT NULL
            CONSTRAINT DF_HideAndSeekSchedule_DaysMask DEFAULT (127),
        IsActive bit NOT NULL
            CONSTRAINT DF_HideAndSeekSchedule_Active DEFAULT (1),
        LastRunLocalDate date NULL,
        UpdatedAtUtc datetime2(0) NOT NULL
            CONSTRAINT DF_HideAndSeekSchedule_Updated DEFAULT (SYSUTCDATETIME())
    );
END;
GO

IF OBJECT_ID(N'dbo._HideAndSeekRun', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._HideAndSeekRun
    (
        HnsRunID bigint IDENTITY(1,1) NOT NULL
            CONSTRAINT PK_HideAndSeekRun PRIMARY KEY,
        AutoRunID bigint NOT NULL,
        LocationID int NOT NULL,
        LocationName nvarchar(128) NOT NULL,
        WorldID int NOT NULL,
        RegionID int NOT NULL,
        PosX int NOT NULL,
        PosY int NOT NULL,
        PosZ int NOT NULL,
        BotCharID int NOT NULL,
        BotCharacterName varchar(16) NOT NULL,
        Status nvarchar(24) NOT NULL,
        StartedAtUtc datetime2(3) NOT NULL
            CONSTRAINT DF_HideAndSeekRun_Started DEFAULT (SYSUTCDATETIME()),
        EndsAtUtc datetime2(3) NOT NULL,
        EndedAtUtc datetime2(3) NULL,
        WinnerCharID int NULL,
        WinnerCharName nvarchar(64) NULL,
        WinnerJID int NULL,
        WinnerHwid varchar(128) NULL,
        WinnerIP varchar(64) NULL,
        RewardSummary nvarchar(512) NULL,
        Message nvarchar(512) NULL
    );

    CREATE UNIQUE INDEX UX_HideAndSeekRun_AutoRunID
        ON dbo._HideAndSeekRun(AutoRunID);
END;
GO
