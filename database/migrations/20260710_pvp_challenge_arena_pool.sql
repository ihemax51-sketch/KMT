USE [KMTGuard]
GO

SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

IF COL_LENGTH(N'dbo._PvpChallengeConfig', N'ArenaStartDelaySeconds') IS NULL
    ALTER TABLE dbo._PvpChallengeConfig ADD ArenaStartDelaySeconds int NOT NULL CONSTRAINT DF_PvpChallengeConfig_ArenaStartDelay DEFAULT (10);
GO

IF NOT EXISTS (
    SELECT 1
    FROM sys.check_constraints
    WHERE name = N'CK_PvpChallengeConfig_ArenaStartDelay'
      AND parent_object_id = OBJECT_ID(N'dbo._PvpChallengeConfig')
)
    ALTER TABLE dbo._PvpChallengeConfig WITH CHECK ADD CONSTRAINT CK_PvpChallengeConfig_ArenaStartDelay CHECK (ArenaStartDelaySeconds BETWEEN 0 AND 300);
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

IF COL_LENGTH(N'dbo._PvpChallengeMatch', N'ArenaID') IS NULL
    ALTER TABLE dbo._PvpChallengeMatch ADD ArenaID int NULL;
GO

IF COL_LENGTH(N'dbo._PvpChallengeMatch', N'QueuedAt') IS NULL
    ALTER TABLE dbo._PvpChallengeMatch ADD QueuedAt datetime2(0) NULL;
GO

IF EXISTS (
    SELECT 1
    FROM sys.check_constraints
    WHERE name = N'CK_PvpChallengeMatch_Status'
      AND parent_object_id = OBJECT_ID(N'dbo._PvpChallengeMatch')
)
    ALTER TABLE dbo._PvpChallengeMatch DROP CONSTRAINT CK_PvpChallengeMatch_Status;
GO

ALTER TABLE dbo._PvpChallengeMatch WITH CHECK ADD CONSTRAINT CK_PvpChallengeMatch_Status CHECK (Status BETWEEN 0 AND 7);
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE name = N'IX_PvpChallengeMatch_Waiting' AND object_id = OBJECT_ID(N'dbo._PvpChallengeMatch'))
    CREATE INDEX IX_PvpChallengeMatch_Waiting ON dbo._PvpChallengeMatch(Status, AcceptedAt, MatchID);
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE name = N'IX_PvpChallengeMatch_ActiveArena' AND object_id = OBJECT_ID(N'dbo._PvpChallengeMatch'))
    CREATE INDEX IX_PvpChallengeMatch_ActiveArena ON dbo._PvpChallengeMatch(ArenaID, Status) WHERE ArenaID IS NOT NULL;
GO

PRINT 'PvP Challenge arena pool installed. Add arenas in dbo._PvpChallengeArena and set ArenaStartDelaySeconds in dbo._PvpChallengeConfig.';
GO
