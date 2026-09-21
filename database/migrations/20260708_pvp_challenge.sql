USE [KMTGuard]
GO

SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

IF OBJECT_ID(N'dbo._PvpChallengeConfig', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._PvpChallengeConfig
    (
        ID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_PvpChallengeConfig PRIMARY KEY,
        Enabled bit NOT NULL CONSTRAINT DF_PvpChallengeConfig_Enabled DEFAULT (1),
        GameWorldID int NOT NULL CONSTRAINT DF_PvpChallengeConfig_GameWorldID DEFAULT (1),
        RegionID int NOT NULL CONSTRAINT DF_PvpChallengeConfig_RegionID DEFAULT (0),
        PosX int NOT NULL CONSTRAINT DF_PvpChallengeConfig_PosX DEFAULT (0),
        PosY int NOT NULL CONSTRAINT DF_PvpChallengeConfig_PosY DEFAULT (0),
        PosZ int NOT NULL CONSTRAINT DF_PvpChallengeConfig_PosZ DEFAULT (0),
        TownGameWorldID int NOT NULL CONSTRAINT DF_PvpChallengeConfig_TownGameWorldID DEFAULT (1),
        TownRegionID int NOT NULL CONSTRAINT DF_PvpChallengeConfig_TownRegionID DEFAULT (25000),
        TownPosX int NOT NULL CONSTRAINT DF_PvpChallengeConfig_TownPosX DEFAULT (0),
        TownPosY int NOT NULL CONSTRAINT DF_PvpChallengeConfig_TownPosY DEFAULT (0),
        TownPosZ int NOT NULL CONSTRAINT DF_PvpChallengeConfig_TownPosZ DEFAULT (0),
        CapeType tinyint NOT NULL CONSTRAINT DF_PvpChallengeConfig_CapeType DEFAULT (5),
        MinGold bigint NOT NULL CONSTRAINT DF_PvpChallengeConfig_MinGold DEFAULT (1),
        RequestTimeoutSeconds int NOT NULL CONSTRAINT DF_PvpChallengeConfig_RequestTimeout DEFAULT (60),
        FightTimeoutSeconds int NOT NULL CONSTRAINT DF_PvpChallengeConfig_FightTimeout DEFAULT (300),
        ArenaStartDelaySeconds int NOT NULL CONSTRAINT DF_PvpChallengeConfig_ArenaStartDelay DEFAULT (10),
        FreezeSeconds int NOT NULL CONSTRAINT DF_PvpChallengeConfig_FreezeSeconds DEFAULT (3),
        UpdatedAt datetime2(0) NOT NULL CONSTRAINT DF_PvpChallengeConfig_UpdatedAt DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT CK_PvpChallengeConfig_CapeType CHECK (CapeType BETWEEN 1 AND 5),
        CONSTRAINT CK_PvpChallengeConfig_MinGold CHECK (MinGold > 0),
        CONSTRAINT CK_PvpChallengeConfig_RequestTimeout CHECK (RequestTimeoutSeconds >= 5),
        CONSTRAINT CK_PvpChallengeConfig_FightTimeout CHECK (FightTimeoutSeconds >= 30),
        CONSTRAINT CK_PvpChallengeConfig_ArenaStartDelay CHECK (ArenaStartDelaySeconds BETWEEN 0 AND 300),
        CONSTRAINT CK_PvpChallengeConfig_FreezeSeconds CHECK (FreezeSeconds BETWEEN 0 AND 600)
    );
END
GO

IF COL_LENGTH(N'dbo._PvpChallengeConfig', N'TownGameWorldID') IS NULL
    ALTER TABLE dbo._PvpChallengeConfig ADD TownGameWorldID int NOT NULL CONSTRAINT DF_PvpChallengeConfig_TownGameWorldID DEFAULT (1);
GO

IF COL_LENGTH(N'dbo._PvpChallengeConfig', N'TownRegionID') IS NULL
    ALTER TABLE dbo._PvpChallengeConfig ADD TownRegionID int NOT NULL CONSTRAINT DF_PvpChallengeConfig_TownRegionID DEFAULT (25000);
GO

IF COL_LENGTH(N'dbo._PvpChallengeConfig', N'TownPosX') IS NULL
    ALTER TABLE dbo._PvpChallengeConfig ADD TownPosX int NOT NULL CONSTRAINT DF_PvpChallengeConfig_TownPosX DEFAULT (0);
GO

IF COL_LENGTH(N'dbo._PvpChallengeConfig', N'TownPosY') IS NULL
    ALTER TABLE dbo._PvpChallengeConfig ADD TownPosY int NOT NULL CONSTRAINT DF_PvpChallengeConfig_TownPosY DEFAULT (0);
GO

IF COL_LENGTH(N'dbo._PvpChallengeConfig', N'TownPosZ') IS NULL
    ALTER TABLE dbo._PvpChallengeConfig ADD TownPosZ int NOT NULL CONSTRAINT DF_PvpChallengeConfig_TownPosZ DEFAULT (0);
GO

IF COL_LENGTH(N'dbo._PvpChallengeConfig', N'FreezeSeconds') IS NULL
    ALTER TABLE dbo._PvpChallengeConfig ADD FreezeSeconds int NOT NULL CONSTRAINT DF_PvpChallengeConfig_FreezeSeconds DEFAULT (3);
GO

IF COL_LENGTH(N'dbo._PvpChallengeConfig', N'ArenaStartDelaySeconds') IS NULL
    ALTER TABLE dbo._PvpChallengeConfig ADD ArenaStartDelaySeconds int NOT NULL CONSTRAINT DF_PvpChallengeConfig_ArenaStartDelay DEFAULT (10);
GO

IF NOT EXISTS (SELECT 1 FROM sys.check_constraints WHERE name = N'CK_PvpChallengeConfig_ArenaStartDelay' AND parent_object_id = OBJECT_ID(N'dbo._PvpChallengeConfig'))
    ALTER TABLE dbo._PvpChallengeConfig WITH CHECK ADD CONSTRAINT CK_PvpChallengeConfig_ArenaStartDelay CHECK (ArenaStartDelaySeconds BETWEEN 0 AND 300);
GO

IF NOT EXISTS (SELECT 1 FROM sys.check_constraints WHERE name = N'CK_PvpChallengeConfig_FreezeSeconds' AND parent_object_id = OBJECT_ID(N'dbo._PvpChallengeConfig'))
    ALTER TABLE dbo._PvpChallengeConfig WITH CHECK ADD CONSTRAINT CK_PvpChallengeConfig_FreezeSeconds CHECK (FreezeSeconds BETWEEN 0 AND 600);
GO

IF NOT EXISTS (SELECT 1 FROM dbo._PvpChallengeConfig WITH (NOLOCK))
BEGIN
    INSERT INTO dbo._PvpChallengeConfig
        (Enabled, GameWorldID, RegionID, PosX, PosY, PosZ, TownGameWorldID, TownRegionID, TownPosX, TownPosY, TownPosZ, CapeType, MinGold, RequestTimeoutSeconds, FightTimeoutSeconds, ArenaStartDelaySeconds, FreezeSeconds)
    VALUES
        (1, 1, 0, 0, 0, 0, 1, 25000, 0, 0, 0, 5, 1, 60, 300, 10, 3);
END
GO

IF OBJECT_ID(N'dbo._PvpChallengeArena', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._PvpChallengeArena
    (
        ArenaID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_PvpChallengeArena PRIMARY KEY,
        Enabled bit NOT NULL CONSTRAINT DF_PvpChallengeArena_Enabled DEFAULT (1),
        ArenaName nvarchar(64) NOT NULL CONSTRAINT DF_PvpChallengeArena_ArenaName DEFAULT (N'Arena'),
        GameWorldID int NOT NULL,
        RegionID int NOT NULL,
        PosX int NOT NULL,
        PosY int NOT NULL,
        PosZ int NOT NULL,
        SortOrder int NOT NULL CONSTRAINT DF_PvpChallengeArena_SortOrder DEFAULT (0),
        UpdatedAt datetime2(0) NOT NULL CONSTRAINT DF_PvpChallengeArena_UpdatedAt DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT CK_PvpChallengeArena_Target CHECK (GameWorldID > 0 AND RegionID > 0)
    );
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE name = N'IX_PvpChallengeArena_EnabledSort' AND object_id = OBJECT_ID(N'dbo._PvpChallengeArena'))
    CREATE INDEX IX_PvpChallengeArena_EnabledSort ON dbo._PvpChallengeArena(Enabled, SortOrder, ArenaID);
GO

IF NOT EXISTS (SELECT 1 FROM dbo._PvpChallengeArena WITH (NOLOCK))
BEGIN
    INSERT INTO dbo._PvpChallengeArena
        (Enabled, ArenaName, GameWorldID, RegionID, PosX, PosY, PosZ, SortOrder)
    SELECT TOP (1)
        1, N'Arena 1', GameWorldID, RegionID, PosX, PosY, PosZ, 1
    FROM dbo._PvpChallengeConfig WITH (NOLOCK)
    WHERE GameWorldID > 0 AND RegionID > 0
    ORDER BY ID;
END
GO

IF OBJECT_ID(N'dbo._PvpChallengeMatch', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._PvpChallengeMatch
    (
        MatchID bigint IDENTITY(1,1) NOT NULL CONSTRAINT PK_PvpChallengeMatch PRIMARY KEY,
        ChallengerCharID int NOT NULL,
        ChallengerCharName nvarchar(64) NOT NULL,
        OpponentCharID int NOT NULL,
        OpponentCharName nvarchar(64) NOT NULL,
        WagerGold bigint NOT NULL,
        ArenaID int NULL,
        ArenaGameWorldID int NOT NULL,
        ArenaRegionID int NOT NULL,
        ArenaPosX int NOT NULL,
        ArenaPosY int NOT NULL,
        ArenaPosZ int NOT NULL,
        CapeType tinyint NOT NULL,
        Status tinyint NOT NULL,
        RequestedAt datetime2(0) NOT NULL CONSTRAINT DF_PvpChallengeMatch_RequestedAt DEFAULT (SYSUTCDATETIME()),
        AcceptedAt datetime2(0) NULL,
        QueuedAt datetime2(0) NULL,
        StartedAt datetime2(0) NULL,
        FinishedAt datetime2(0) NULL,
        WinnerCharID int NULL,
        WinnerCharName nvarchar(64) NULL,
        LoserCharID int NULL,
        LoserCharName nvarchar(64) NULL,
        EndReason nvarchar(128) NULL,
        CONSTRAINT CK_PvpChallengeMatch_WagerGold CHECK (WagerGold > 0),
        CONSTRAINT CK_PvpChallengeMatch_Status CHECK (Status BETWEEN 0 AND 7)
    );
END
GO

IF COL_LENGTH(N'dbo._PvpChallengeMatch', N'ArenaID') IS NULL
    ALTER TABLE dbo._PvpChallengeMatch ADD ArenaID int NULL;
GO

IF COL_LENGTH(N'dbo._PvpChallengeMatch', N'QueuedAt') IS NULL
    ALTER TABLE dbo._PvpChallengeMatch ADD QueuedAt datetime2(0) NULL;
GO

IF EXISTS (SELECT 1 FROM sys.check_constraints WHERE name = N'CK_PvpChallengeMatch_Status' AND parent_object_id = OBJECT_ID(N'dbo._PvpChallengeMatch'))
    ALTER TABLE dbo._PvpChallengeMatch DROP CONSTRAINT CK_PvpChallengeMatch_Status;
GO

ALTER TABLE dbo._PvpChallengeMatch WITH CHECK ADD CONSTRAINT CK_PvpChallengeMatch_Status CHECK (Status BETWEEN 0 AND 7);
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE name = N'IX_PvpChallengeMatch_Status' AND object_id = OBJECT_ID(N'dbo._PvpChallengeMatch'))
    CREATE INDEX IX_PvpChallengeMatch_Status ON dbo._PvpChallengeMatch(Status, RequestedAt);
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE name = N'IX_PvpChallengeMatch_Waiting' AND object_id = OBJECT_ID(N'dbo._PvpChallengeMatch'))
    CREATE INDEX IX_PvpChallengeMatch_Waiting ON dbo._PvpChallengeMatch(Status, AcceptedAt, MatchID);
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE name = N'IX_PvpChallengeMatch_ActiveArena' AND object_id = OBJECT_ID(N'dbo._PvpChallengeMatch'))
    CREATE INDEX IX_PvpChallengeMatch_ActiveArena ON dbo._PvpChallengeMatch(ArenaID, Status) WHERE ArenaID IS NOT NULL;
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE name = N'IX_PvpChallengeMatch_Challenger' AND object_id = OBJECT_ID(N'dbo._PvpChallengeMatch'))
    CREATE INDEX IX_PvpChallengeMatch_Challenger ON dbo._PvpChallengeMatch(ChallengerCharID, Status);
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE name = N'IX_PvpChallengeMatch_Opponent' AND object_id = OBJECT_ID(N'dbo._PvpChallengeMatch'))
    CREATE INDEX IX_PvpChallengeMatch_Opponent ON dbo._PvpChallengeMatch(OpponentCharID, Status);
GO

IF OBJECT_ID(N'dbo._PvpChallengeKillLog', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._PvpChallengeKillLog
    (
        ID bigint IDENTITY(1,1) NOT NULL CONSTRAINT PK_PvpChallengeKillLog PRIMARY KEY,
        MatchID bigint NOT NULL,
        KillerCharID int NOT NULL,
        KillerCharName nvarchar(64) NOT NULL,
        DeadCharID int NOT NULL,
        DeadCharName nvarchar(64) NOT NULL,
        WorldID int NOT NULL,
        RegionID int NOT NULL,
        KillerPvpCape tinyint NOT NULL,
        DeadPvpCape tinyint NOT NULL,
        CreatedAt datetime2(0) NOT NULL CONSTRAINT DF_PvpChallengeKillLog_CreatedAt DEFAULT (SYSUTCDATETIME())
    );
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE name = N'IX_PvpChallengeKillLog_MatchID' AND object_id = OBJECT_ID(N'dbo._PvpChallengeKillLog'))
    CREATE INDEX IX_PvpChallengeKillLog_MatchID ON dbo._PvpChallengeKillLog(MatchID);
GO

IF OBJECT_ID(N'dbo._TeleportFreezeCommand', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._TeleportFreezeCommand
    (
        CommandID bigint IDENTITY(1,1) NOT NULL CONSTRAINT PK_TeleportFreezeCommand PRIMARY KEY,
        CharID int NOT NULL,
        GameWorldID int NOT NULL,
        RegionID int NOT NULL,
        PosX int NOT NULL,
        PosY int NOT NULL,
        PosZ int NOT NULL,
        FreezeSeconds int NOT NULL CONSTRAINT DF_TeleportFreezeCommand_FreezeSeconds DEFAULT (0),
        Status tinyint NOT NULL CONSTRAINT DF_TeleportFreezeCommand_Status DEFAULT (0),
        CreatedAt datetime2(0) NOT NULL CONSTRAINT DF_TeleportFreezeCommand_CreatedAt DEFAULT (SYSUTCDATETIME()),
        PickedAt datetime2(0) NULL,
        CompletedAt datetime2(0) NULL,
        ErrorMessage nvarchar(512) NULL,
        CONSTRAINT CK_TeleportFreezeCommand_PositiveTarget CHECK (CharID > 0 AND GameWorldID > 0 AND RegionID > 0),
        CONSTRAINT CK_TeleportFreezeCommand_FreezeSeconds CHECK (FreezeSeconds BETWEEN 0 AND 600),
        CONSTRAINT CK_TeleportFreezeCommand_Status CHECK (Status BETWEEN 0 AND 3)
    );
END
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE name = N'IX_TeleportFreezeCommand_Status' AND object_id = OBJECT_ID(N'dbo._TeleportFreezeCommand'))
    CREATE INDEX IX_TeleportFreezeCommand_Status ON dbo._TeleportFreezeCommand(Status, CommandID);
GO

IF OBJECT_ID(N'dbo.__Settings', N'U') IS NOT NULL
BEGIN
    IF NOT EXISTS (SELECT 1 FROM dbo.__Settings WITH (NOLOCK) WHERE SettingName = N'EnablePvpChallenge')
        INSERT INTO dbo.__Settings (SettingName, Value) VALUES (N'EnablePvpChallenge', N'True');

    IF NOT EXISTS (SELECT 1 FROM dbo.__Settings WITH (NOLOCK) WHERE SettingName = N'ShowGuidePvpChallenge')
        INSERT INTO dbo.__Settings (SettingName, Value) VALUES (N'ShowGuidePvpChallenge', N'True');
END
GO

CREATE OR ALTER PROCEDURE [dbo].[__TeleportToPositionWithFreeze]
    @CharID int,
    @GameWorldID int,
    @RegionId int,
    @PosX int,
    @PosY int,
    @PosZ int,
    @FreezeSeconds int = 0
AS
BEGIN
    SET NOCOUNT ON;

    IF @CharID <= 0
        THROW 50001, 'CharID must be greater than zero.', 1;

    IF @GameWorldID <= 0
        THROW 50002, 'GameWorldID must be greater than zero.', 1;

    IF @RegionId <= 0
        THROW 50003, 'RegionId must be greater than zero.', 1;

    IF @FreezeSeconds < 0 OR @FreezeSeconds > 600
        THROW 50004, 'FreezeSeconds must be between 0 and 600.', 1;

    INSERT INTO dbo._TeleportFreezeCommand
        (CharID, GameWorldID, RegionID, PosX, PosY, PosZ, FreezeSeconds, Status)
    VALUES
        (@CharID, @GameWorldID, @RegionId, @PosX, @PosY, @PosZ, @FreezeSeconds, 0);

    SELECT CONVERT(bigint, SCOPE_IDENTITY()) AS CommandID;
END
GO

CREATE OR ALTER PROCEDURE [dbo].[__TeleportToPosition]
    @CharID int,
    @GameWorldID int,
    @RegionId int,
    @PosX int,
    @PosY int,
    @PosZ int
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE @ShardDB sysname =
        COALESCE(NULLIF((SELECT TOP (1) [Value] FROM dbo.__Settings WITH (NOLOCK) WHERE SettingName = N'ShardDB'), N''), N'SRO_VT_SHARD');

    DECLARE @FullQueueName nvarchar(300) = QUOTENAME(@ShardDB) + N'.dbo._ExeGameServer';
    DECLARE @FullCharName nvarchar(300) = QUOTENAME(@ShardDB) + N'.dbo._Char';
    DECLARE @Sql nvarchar(max) = N'
IF OBJECT_ID(N''' + REPLACE(@FullQueueName, N'''', N'''''') + N''', N''U'') IS NULL
BEGIN
    CREATE TABLE ' + @FullQueueName + N'
    (
        ID INT IDENTITY(1,1) PRIMARY KEY,
        Action_ID INT NOT NULL,
        Action_Result SMALLINT NOT NULL DEFAULT 0,
        CharName16 VARCHAR(64) NOT NULL,
        Param01 VARCHAR(129) NULL,
        Param02 BIGINT NULL,
        Param03 BIGINT NULL,
        Param04 BIGINT NULL,
        Param05 BIGINT NULL,
        Param06 BIGINT NULL,
        Param07 BIGINT NULL,
        Param08 BIGINT NULL
    );
END;

INSERT INTO ' + @FullQueueName + N'
    (Action_ID, CharName16, Param02, Param03, Param04, Param05, Param06)
SELECT
    5,
    CharName16,
    @GameWorldID,
    @RegionId,
    @PosX,
    @PosY,
    @PosZ
FROM ' + @FullCharName + N' WITH (NOLOCK)
WHERE CharID = @CharID;';

    EXEC sys.sp_executesql
        @Sql,
        N'@CharID int, @GameWorldID int, @RegionId int, @PosX int, @PosY int, @PosZ int',
        @CharID = @CharID,
        @GameWorldID = @GameWorldID,
        @RegionId = @RegionId,
        @PosX = @PosX,
        @PosY = @PosY,
        @PosZ = @PosZ;
END
GO

CREATE OR ALTER PROCEDURE [dbo].[__TeleportToTownbyCharID]
    @CharID int
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE @GameWorldID int;
    DECLARE @RegionID int;
    DECLARE @PosX int;
    DECLARE @PosY int;
    DECLARE @PosZ int;

    SELECT TOP (1)
        @GameWorldID = TownGameWorldID,
        @RegionID = TownRegionID,
        @PosX = TownPosX,
        @PosY = TownPosY,
        @PosZ = TownPosZ
    FROM dbo._PvpChallengeConfig WITH (NOLOCK)
    ORDER BY ID;

    EXEC dbo.__TeleportToPosition
        @CharID = @CharID,
        @GameWorldID = @GameWorldID,
        @RegionId = @RegionID,
        @PosX = @PosX,
        @PosY = @PosY,
        @PosZ = @PosZ;
END
GO

PRINT 'PvP Challenge installed. Configure dbo._PvpChallengeArena rows, dbo._PvpChallengeConfig town coordinates, ArenaStartDelaySeconds and FreezeSeconds before live use.';
GO
