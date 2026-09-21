USE [KMTGuard]
GO

SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

IF COL_LENGTH(N'dbo._PvpChallengeConfig', N'FreezeSeconds') IS NULL
    ALTER TABLE dbo._PvpChallengeConfig ADD FreezeSeconds int NOT NULL CONSTRAINT DF_PvpChallengeConfig_FreezeSeconds DEFAULT (3);
GO

IF NOT EXISTS (
    SELECT 1
    FROM sys.check_constraints
    WHERE name = N'CK_PvpChallengeConfig_FreezeSeconds'
      AND parent_object_id = OBJECT_ID(N'dbo._PvpChallengeConfig')
)
    ALTER TABLE dbo._PvpChallengeConfig WITH CHECK ADD CONSTRAINT CK_PvpChallengeConfig_FreezeSeconds CHECK (FreezeSeconds BETWEEN 0 AND 600);
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

IF NOT EXISTS (
    SELECT 1
    FROM sys.indexes
    WHERE name = N'IX_TeleportFreezeCommand_Status'
      AND object_id = OBJECT_ID(N'dbo._TeleportFreezeCommand')
)
    CREATE INDEX IX_TeleportFreezeCommand_Status ON dbo._TeleportFreezeCommand(Status, CommandID);
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

PRINT 'PvP Challenge freeze support installed.';
GO
