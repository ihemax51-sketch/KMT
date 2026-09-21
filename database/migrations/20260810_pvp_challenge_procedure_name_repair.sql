USE [KMTGuard];
GO

SET ANSI_NULLS ON;
SET QUOTED_IDENTIFIER ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.PVP_Settings', N'U') IS NULL OR
   OBJECT_ID(N'dbo.Teleport_FreezeQueue', N'U') IS NULL OR
   OBJECT_ID(N'dbo.Teleport_Position', N'P') IS NULL
BEGIN
    THROW 51045, 'PvP Challenge repair prerequisites are missing. Apply the v3.0.4 integrity update first.', 1;
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
    THROW 51046, 'Teleport_PositionFreeze repair verification failed.', 1;
GO

IF OBJECT_ID(N'dbo.Teleport_PlayerToTown', N'P') IS NULL OR
   OBJECT_DEFINITION(OBJECT_ID(N'dbo.Teleport_PlayerToTown')) NOT LIKE N'%dbo.PVP_Settings%' OR
   OBJECT_DEFINITION(OBJECT_ID(N'dbo.Teleport_PlayerToTown')) NOT LIKE N'%dbo.Teleport_Position%'
    THROW 51047, 'Teleport_PlayerToTown repair verification failed.', 1;
GO

PRINT 'PvP Challenge procedure-name repair installed successfully.';
GO
