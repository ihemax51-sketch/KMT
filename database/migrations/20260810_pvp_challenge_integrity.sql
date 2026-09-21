USE [KMTGuard];
GO

SET ANSI_NULLS ON;
SET QUOTED_IDENTIFIER ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.PVP_Settings', N'U') IS NULL AND OBJECT_ID(N'dbo._PvpChallengeConfig', N'U') IS NOT NULL
    EXEC sys.sp_rename N'dbo._PvpChallengeConfig', N'PVP_Settings';
GO

IF OBJECT_ID(N'dbo.PVP_Arenas', N'U') IS NULL AND OBJECT_ID(N'dbo._PvpChallengeArena', N'U') IS NOT NULL
    EXEC sys.sp_rename N'dbo._PvpChallengeArena', N'PVP_Arenas';
GO

IF OBJECT_ID(N'dbo.PVP_Matches', N'U') IS NULL AND OBJECT_ID(N'dbo._PvpChallengeMatch', N'U') IS NOT NULL
    EXEC sys.sp_rename N'dbo._PvpChallengeMatch', N'PVP_Matches';
GO

IF OBJECT_ID(N'dbo.PVP_KillLog', N'U') IS NULL AND OBJECT_ID(N'dbo._PvpChallengeKillLog', N'U') IS NOT NULL
    EXEC sys.sp_rename N'dbo._PvpChallengeKillLog', N'PVP_KillLog';
GO

IF OBJECT_ID(N'dbo.Teleport_FreezeQueue', N'U') IS NULL AND OBJECT_ID(N'dbo._TeleportFreezeCommand', N'U') IS NOT NULL
    EXEC sys.sp_rename N'dbo._TeleportFreezeCommand', N'Teleport_FreezeQueue';
GO

IF OBJECT_ID(N'dbo.PVP_Settings', N'U') IS NULL OR
   OBJECT_ID(N'dbo.PVP_Arenas', N'U') IS NULL OR
   OBJECT_ID(N'dbo.PVP_Matches', N'U') IS NULL OR
   OBJECT_ID(N'dbo.PVP_KillLog', N'U') IS NULL OR
   OBJECT_ID(N'dbo.Teleport_FreezeQueue', N'U') IS NULL
BEGIN
    THROW 51040, 'PvP Challenge database objects are incomplete. Apply the earlier PvP Challenge migrations first.', 1;
END;
GO

UPDATE dbo.PVP_Settings
SET MinGold = 1,
    UpdatedAt = SYSUTCDATETIME()
WHERE MinGold < 1 OR MinGold > 4611686018427387903;
GO

IF NOT EXISTS
(
    SELECT 1
    FROM sys.check_constraints
    WHERE parent_object_id = OBJECT_ID(N'dbo.PVP_Settings')
      AND name = N'CK_PVP_Settings_MinGoldSafe'
)
BEGIN
    ALTER TABLE dbo.PVP_Settings WITH CHECK
        ADD CONSTRAINT CK_PVP_Settings_MinGoldSafe
        CHECK (MinGold BETWEEN 1 AND 4611686018427387903);
END;
GO

CREATE OR ALTER TRIGGER dbo.TR_PVP_Matches_WagerSafe
ON dbo.PVP_Matches
AFTER INSERT, UPDATE
AS
BEGIN
    SET NOCOUNT ON;

    IF UPDATE(WagerGold) AND EXISTS
    (
        SELECT 1
        FROM inserted
        WHERE WagerGold < 1 OR WagerGold > 4611686018427387903
    )
    BEGIN
        THROW 51041, 'PvP Challenge wager is outside the safe payout range.', 1;
    END;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Teleport_PositionFreeze
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

    INSERT INTO dbo.Teleport_FreezeQueue
        (CharID, GameWorldID, RegionID, PosX, PosY, PosZ, FreezeSeconds, Status)
    VALUES
        (@CharID, @GameWorldID, @RegionId, @PosX, @PosY, @PosZ, @FreezeSeconds, 0);

    SELECT CONVERT(bigint, SCOPE_IDENTITY()) AS CommandID;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Teleport_PlayerToTown
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
    FROM dbo.PVP_Settings WITH (NOLOCK)
    ORDER BY ID;

    IF @GameWorldID IS NULL OR @RegionID IS NULL
        THROW 51042, 'PvP Challenge town destination is not configured.', 1;

    EXEC dbo.Teleport_Position
        @CharID = @CharID,
        @GameWorldID = @GameWorldID,
        @RegionId = @RegionID,
        @PosX = @PosX,
        @PosY = @PosY,
        @PosZ = @PosZ;
END;
GO

IF OBJECT_ID(N'dbo.Teleport_PositionFreeze', N'P') IS NULL OR
   OBJECT_DEFINITION(OBJECT_ID(N'dbo.Teleport_PositionFreeze')) NOT LIKE N'%dbo.Teleport_FreezeQueue%'
    THROW 51043, 'PvP Challenge teleport-freeze procedure was not installed correctly.', 1;
GO

IF OBJECT_ID(N'dbo.Teleport_PlayerToTown', N'P') IS NULL OR
   OBJECT_DEFINITION(OBJECT_ID(N'dbo.Teleport_PlayerToTown')) NOT LIKE N'%dbo.PVP_Settings%' OR
   OBJECT_DEFINITION(OBJECT_ID(N'dbo.Teleport_PlayerToTown')) NOT LIKE N'%dbo.Teleport_Position%'
    THROW 51044, 'PvP Challenge town-teleport procedure was not installed correctly.', 1;
GO

PRINT 'PvP Challenge database contract and wager safeguards installed.';
GO
