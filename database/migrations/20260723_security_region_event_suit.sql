USE [KMTGuard];
GO

SET XACT_ABORT ON;
GO

BEGIN TRY
    BEGIN TRANSACTION;

    IF OBJECT_ID(N'dbo.Security_RegionFeatures', N'U') IS NULL
    BEGIN
        CREATE TABLE dbo.Security_RegionFeatures
        (
            ID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_FilterRegionControl PRIMARY KEY,
            WorldID int NOT NULL CONSTRAINT DF_FilterRegionControl_WorldID DEFAULT (0),
            RegionID int NOT NULL,
            Allow_IntCharacter bit NOT NULL CONSTRAINT DF_FilterRegionControl_AllowInt DEFAULT (1),
            Allow_StrCharacter bit NOT NULL CONSTRAINT DF_FilterRegionControl_AllowStr DEFAULT (1),
            Enable_AdvElixir bit NOT NULL CONSTRAINT DF_FilterRegionControl_AdvElixir DEFAULT (1),
            Enable_Alchemy bit NOT NULL CONSTRAINT DF_FilterRegionControl_Alchemy DEFAULT (1),
            Enable_AutoPvP tinyint NOT NULL CONSTRAINT DF_FilterRegionControl_AutoPvP DEFAULT (0),
            Enable_Chat bit NOT NULL CONSTRAINT DF_FilterRegionControl_Chat DEFAULT (1),
            Enable_Exchange bit NOT NULL CONSTRAINT DF_FilterRegionControl_Exchange DEFAULT (1),
            Enable_EventSuit bit NOT NULL CONSTRAINT DF_FilterRegionControl_EventSuit DEFAULT (0),
            EventSuit_Team tinyint NOT NULL CONSTRAINT DF_FilterRegionControl_EventSuitTeam DEFAULT (0),
            Enable_FellowScroll bit NOT NULL CONSTRAINT DF_FilterRegionControl_FellowScroll DEFAULT (1),
            Enable_Global bit NOT NULL CONSTRAINT DF_FilterRegionControl_Global DEFAULT (1),
            Enable_JobMode bit NOT NULL CONSTRAINT DF_FilterRegionControl_JobMode DEFAULT (1),
            Enable_Move bit NOT NULL CONSTRAINT DF_FilterRegionControl_Move DEFAULT (1),
            Enable_Party bit NOT NULL CONSTRAINT DF_FilterRegionControl_Party DEFAULT (1),
            Enable_PvP bit NOT NULL CONSTRAINT DF_FilterRegionControl_PvP DEFAULT (1),
            Enable_ResurrectionScroll bit NOT NULL CONSTRAINT DF_FilterRegionControl_Resurrection DEFAULT (1),
            Enable_Reverse bit NOT NULL CONSTRAINT DF_FilterRegionControl_Reverse DEFAULT (1),
            Enable_Stall bit NOT NULL CONSTRAINT DF_FilterRegionControl_Stall DEFAULT (1),
            Enable_Trace bit NOT NULL CONSTRAINT DF_FilterRegionControl_Trace DEFAULT (1),
            Enable_Zerk bit NOT NULL CONSTRAINT DF_FilterRegionControl_Zerk DEFAULT (1),
            Enable_NoBot bit NOT NULL CONSTRAINT DF_FilterRegionControl_NoBot DEFAULT (0),
            NoBot_TimeSeconds int NOT NULL CONSTRAINT DF_FilterRegionControl_NoBotTime DEFAULT (600),
            NoBot_Action tinyint NOT NULL CONSTRAINT DF_FilterRegionControl_NoBotAction DEFAULT (1),
            NoBot_WarningSeconds int NOT NULL CONSTRAINT DF_FilterRegionControl_NoBotWarning DEFAULT (30),
            NoBot_LogOnly bit NOT NULL CONSTRAINT DF_FilterRegionControl_NoBotLogOnly DEFAULT (0),
            Enabled bit NOT NULL CONSTRAINT DF_FilterRegionControl_Enabled DEFAULT (1),
            ManagedEventCode nvarchar(32) NULL,
            ManagedAtUtc datetime2(0) NULL,
            CreatedAt datetime2 NOT NULL CONSTRAINT DF_FilterRegionControl_CreatedAt DEFAULT SYSUTCDATETIME(),
            UpdatedAt datetime2 NULL
        );
    END;

    IF COL_LENGTH(N'dbo.Security_RegionFeatures', N'WorldID') IS NULL
        ALTER TABLE dbo.Security_RegionFeatures
            ADD WorldID int NOT NULL CONSTRAINT DF_FilterRegionControl_WorldID DEFAULT (0) WITH VALUES;

    IF COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enable_EventSuit') IS NULL
        ALTER TABLE dbo.Security_RegionFeatures
            ADD Enable_EventSuit bit NOT NULL CONSTRAINT DF_FilterRegionControl_EventSuit DEFAULT (0) WITH VALUES;

    IF COL_LENGTH(N'dbo.Security_RegionFeatures', N'EventSuit_Team') IS NULL
        ALTER TABLE dbo.Security_RegionFeatures
            ADD EventSuit_Team tinyint NOT NULL CONSTRAINT DF_FilterRegionControl_EventSuitTeam DEFAULT (0) WITH VALUES;

    IF COL_LENGTH(N'dbo.Security_RegionFeatures', N'NoBot_Action') IS NULL
        ALTER TABLE dbo.Security_RegionFeatures
            ADD NoBot_Action tinyint NOT NULL CONSTRAINT DF_FilterRegionControl_NoBotAction DEFAULT (1) WITH VALUES;

    IF COL_LENGTH(N'dbo.Security_RegionFeatures', N'NoBot_WarningSeconds') IS NULL
        ALTER TABLE dbo.Security_RegionFeatures
            ADD NoBot_WarningSeconds int NOT NULL CONSTRAINT DF_FilterRegionControl_NoBotWarning DEFAULT (30) WITH VALUES;

    IF COL_LENGTH(N'dbo.Security_RegionFeatures', N'NoBot_LogOnly') IS NULL
        ALTER TABLE dbo.Security_RegionFeatures
            ADD NoBot_LogOnly bit NOT NULL CONSTRAINT DF_FilterRegionControl_NoBotLogOnly DEFAULT (0) WITH VALUES;

    IF COL_LENGTH(N'dbo.Security_RegionFeatures', N'ManagedEventCode') IS NULL
        ALTER TABLE dbo.Security_RegionFeatures ADD ManagedEventCode nvarchar(32) NULL;

    IF COL_LENGTH(N'dbo.Security_RegionFeatures', N'ManagedAtUtc') IS NULL
        ALTER TABLE dbo.Security_RegionFeatures ADD ManagedAtUtc datetime2(0) NULL;

    EXEC sys.sp_executesql N'
        UPDATE dbo.Security_RegionFeatures
        SET EventSuit_Team = CASE WHEN EventSuit_Team = 0 THEN 0 ELSE 1 END
        WHERE EventSuit_Team NOT IN (0, 1);';

    IF NOT EXISTS
    (
        SELECT 1
        FROM sys.check_constraints
        WHERE parent_object_id = OBJECT_ID(N'dbo.Security_RegionFeatures')
          AND name = N'CK_FilterRegionControl_EventSuitTeam'
    )
    BEGIN
        EXEC sys.sp_executesql N'
            ALTER TABLE dbo.Security_RegionFeatures WITH CHECK
                ADD CONSTRAINT CK_FilterRegionControl_EventSuitTeam
                    CHECK (EventSuit_Team IN (0, 1));';
    END;

    DECLARE @LegacyUniqueName sysname, @LegacyUniqueConstraint bit;
    SELECT TOP (1)
        @LegacyUniqueName = i.name,
        @LegacyUniqueConstraint = i.is_unique_constraint
    FROM sys.indexes AS i
    WHERE i.object_id = OBJECT_ID(N'dbo.Security_RegionFeatures')
      AND i.is_unique = 1
      AND
      (
          SELECT COUNT(*)
          FROM sys.index_columns AS ic
          WHERE ic.object_id = i.object_id
            AND ic.index_id = i.index_id
            AND ic.key_ordinal > 0
      ) = 1
      AND EXISTS
      (
          SELECT 1
          FROM sys.index_columns AS ic
          INNER JOIN sys.columns AS c
              ON c.object_id = ic.object_id
             AND c.column_id = ic.column_id
          WHERE ic.object_id = i.object_id
            AND ic.index_id = i.index_id
            AND ic.key_ordinal = 1
            AND c.name = N'RegionID'
      );

    IF @LegacyUniqueName IS NOT NULL
    BEGIN
        DECLARE @DropLegacyUnique nvarchar(max) =
            CASE WHEN @LegacyUniqueConstraint = 1
                 THEN N'ALTER TABLE dbo.Security_RegionFeatures DROP CONSTRAINT ' + QUOTENAME(@LegacyUniqueName)
                 ELSE N'DROP INDEX ' + QUOTENAME(@LegacyUniqueName) + N' ON dbo.Security_RegionFeatures' END;
        EXEC sys.sp_executesql @DropLegacyUnique;
    END;

    IF NOT EXISTS
    (
        SELECT 1
        FROM sys.indexes
        WHERE object_id = OBJECT_ID(N'dbo.Security_RegionFeatures')
          AND name = N'UX_FilterRegionControl_World_Region'
    )
    BEGIN
        EXEC sys.sp_executesql N'
            CREATE UNIQUE INDEX UX_FilterRegionControl_World_Region
                ON dbo.Security_RegionFeatures(WorldID, RegionID);';
    END;

    CREATE TABLE #EventProfiles
    (
        EventCode nvarchar(32) COLLATE DATABASE_DEFAULT NOT NULL,
        WorldID int NOT NULL,
        RegionID int NOT NULL,
        TeamMode tinyint NOT NULL,
        AllowParty bit NOT NULL,
        UpdatedAtUtc datetime2(0) NOT NULL
    );

    IF DB_ID(N'Events') IS NOT NULL
       AND OBJECT_ID(N'Events.dbo._SurvivalPartyConfig', N'U') IS NOT NULL
    BEGIN
        EXEC sys.sp_executesql N'
            INSERT INTO #EventProfiles (EventCode, WorldID, RegionID, TeamMode, AllowParty, UpdatedAtUtc)
            SELECT EventCode COLLATE DATABASE_DEFAULT, ArenaWorldID, ArenaRegionID, 1, 1, UpdatedAtUtc
            FROM Events.dbo._SurvivalPartyConfig
            WHERE Enabled = 1 AND ArenaWorldID > 0 AND ArenaRegionID > 0;';
    END;

    IF DB_ID(N'Events') IS NOT NULL
       AND OBJECT_ID(N'Events.dbo._SurvivalSoloConfig', N'U') IS NOT NULL
    BEGIN
        EXEC sys.sp_executesql N'
            INSERT INTO #EventProfiles (EventCode, WorldID, RegionID, TeamMode, AllowParty, UpdatedAtUtc)
            SELECT EventCode COLLATE DATABASE_DEFAULT, ArenaWorldID, ArenaRegionID, 0, 0, UpdatedAtUtc
            FROM Events.dbo._SurvivalSoloConfig
            WHERE Enabled = 1 AND ArenaWorldID > 0 AND ArenaRegionID > 0;';
    END;

    IF DB_ID(N'Events') IS NOT NULL
       AND OBJECT_ID(N'Events.dbo._CompetitiveEventConfig', N'U') IS NOT NULL
    BEGIN
        EXEC sys.sp_executesql N'
            INSERT INTO #EventProfiles (EventCode, WorldID, RegionID, TeamMode, AllowParty, UpdatedAtUtc)
            SELECT EventCode COLLATE DATABASE_DEFAULT,
                   ArenaWorldID,
                   ArenaRegionID,
                   CASE WHEN EventCode COLLATE DATABASE_DEFAULT = N''DTT'' THEN 1 ELSE 0 END,
                   CASE WHEN RequireNoParty = 1 THEN 0 ELSE 1 END,
                   UpdatedAtUtc
            FROM Events.dbo._CompetitiveEventConfig
            WHERE Enabled = 1 AND ArenaWorldID > 0 AND ArenaRegionID > 0;';
    END;

    EXEC sys.sp_executesql N'
        ;WITH RankedProfiles AS
        (
            SELECT EventCode, WorldID, RegionID, TeamMode, AllowParty,
                   ROW_NUMBER() OVER
                   (
                       PARTITION BY WorldID, RegionID
                       ORDER BY UpdatedAtUtc DESC, EventCode
                   ) AS RowNumber
            FROM #EventProfiles
        )
        MERGE dbo.Security_RegionFeatures WITH (HOLDLOCK) AS target
        USING
        (
            SELECT EventCode, WorldID, RegionID, TeamMode, AllowParty
            FROM RankedProfiles
            WHERE RowNumber = 1
        ) AS source
        ON target.WorldID = source.WorldID AND target.RegionID = source.RegionID
        WHEN MATCHED THEN
            UPDATE SET
                Allow_IntCharacter = 1,
                Allow_StrCharacter = 1,
                Enable_AdvElixir = 0,
                Enable_Alchemy = 0,
                Enable_AutoPvP = 0,
                Enable_Chat = 1,
                Enable_Exchange = 0,
                Enable_EventSuit = 1,
                EventSuit_Team = source.TeamMode,
                Enable_FellowScroll = 0,
                Enable_Global = 1,
                Enable_JobMode = 0,
                Enable_Move = 1,
                Enable_Party = source.AllowParty,
                Enable_PvP = 1,
                Enable_ResurrectionScroll = 0,
                Enable_Reverse = 0,
                Enable_Stall = 0,
                Enable_Trace = 0,
                Enable_Zerk = 1,
                Enable_NoBot = 0,
                Enabled = 1,
                ManagedEventCode = source.EventCode,
                ManagedAtUtc = SYSUTCDATETIME(),
                UpdatedAt = SYSUTCDATETIME()
        WHEN NOT MATCHED THEN
            INSERT
            (
                WorldID, RegionID, Allow_IntCharacter, Allow_StrCharacter, Enable_AdvElixir, Enable_Alchemy,
                Enable_AutoPvP, Enable_Chat, Enable_Exchange, Enable_EventSuit, EventSuit_Team,
                Enable_FellowScroll, Enable_Global, Enable_JobMode, Enable_Move, Enable_Party, Enable_PvP,
                Enable_ResurrectionScroll, Enable_Reverse, Enable_Stall, Enable_Trace, Enable_Zerk,
                Enable_NoBot, Enabled, ManagedEventCode, ManagedAtUtc
            )
            VALUES
            (
                source.WorldID, source.RegionID, 1, 1, 0, 0, 0, 1, 0, 1, source.TeamMode,
                0, 1, 0, 1, source.AllowParty, 1, 0, 0, 0, 0, 1,
                0, 1, source.EventCode, SYSUTCDATETIME()
            );';

    DROP TABLE #EventProfiles;
    COMMIT TRANSACTION;
END TRY
BEGIN CATCH
    IF @@TRANCOUNT > 0
        ROLLBACK TRANSACTION;
    THROW;
END CATCH;
GO

SELECT WorldID,
       RegionID,
       ManagedEventCode,
       Enable_EventSuit,
       EventSuit_Team,
       Enable_PvP,
       Enable_Party,
       Enabled
FROM dbo.Security_RegionFeatures
ORDER BY WorldID, RegionID;
GO
