SET NOCOUNT ON;
SET XACT_ABORT ON;

IF DB_NAME() <> N'KMTGuard'
    THROW 51000, 'Run this update in the KMTGuard database.', 1;

BEGIN TRY
    BEGIN TRANSACTION;

    /* Rebuild once to give administrators a small, ordered, contradiction-free table. */
    IF OBJECT_ID(N'dbo.Security_RegionFeatures', N'U') IS NULL
       OR COL_LENGTH(N'dbo.Security_RegionFeatures', N'BuildMode') IS NULL
       OR COL_LENGTH(N'dbo.Security_RegionFeatures', N'AllowTeleport') IS NULL
    BEGIN
        IF OBJECT_ID(N'dbo.Security_RegionFeatures_v600_New', N'U') IS NOT NULL
            DROP TABLE dbo.Security_RegionFeatures_v600_New;

        CREATE TABLE dbo.Security_RegionFeatures_v600_New
        (
            ID int IDENTITY(1,1) NOT NULL CONSTRAINT PK_RegionFeatures_v600_New PRIMARY KEY,
            WorldID int NOT NULL CONSTRAINT DF_RegionFeatures_v600_World DEFAULT (0),
            RegionID int NOT NULL,
            RuleName nvarchar(64) NULL,
            Enabled bit NOT NULL CONSTRAINT DF_RegionFeatures_v600_Enabled DEFAULT (1),

            BuildMode tinyint NOT NULL CONSTRAINT DF_RegionFeatures_v600_Build DEFAULT (0),
            JobMode tinyint NOT NULL CONSTRAINT DF_RegionFeatures_v600_Job DEFAULT (0),
            RaceMode tinyint NOT NULL CONSTRAINT DF_RegionFeatures_v600_Race DEFAULT (0),
            PartyMode tinyint NOT NULL CONSTRAINT DF_RegionFeatures_v600_Party DEFAULT (0),
            MinLevel tinyint NOT NULL CONSTRAINT DF_RegionFeatures_v600_MinLevel DEFAULT (0),
            MaxLevel tinyint NOT NULL CONSTRAINT DF_RegionFeatures_v600_MaxLevel DEFAULT (0),

            AllowTeleport bit NOT NULL CONSTRAINT DF_RegionFeatures_v600_Teleport DEFAULT (1),
            AllowReverse bit NOT NULL CONSTRAINT DF_RegionFeatures_v600_Reverse DEFAULT (1),
            AllowTrace bit NOT NULL CONSTRAINT DF_RegionFeatures_v600_Trace DEFAULT (1),
            AllowMovement bit NOT NULL CONSTRAINT DF_RegionFeatures_v600_Movement DEFAULT (1),
            AllowChat bit NOT NULL CONSTRAINT DF_RegionFeatures_v600_Chat DEFAULT (1),
            AllowGlobalChat bit NOT NULL CONSTRAINT DF_RegionFeatures_v600_Global DEFAULT (1),
            AllowParty bit NOT NULL CONSTRAINT DF_RegionFeatures_v600_PartyAction DEFAULT (1),
            AllowExchange bit NOT NULL CONSTRAINT DF_RegionFeatures_v600_Exchange DEFAULT (1),
            AllowStall bit NOT NULL CONSTRAINT DF_RegionFeatures_v600_Stall DEFAULT (1),
            AllowPvP bit NOT NULL CONSTRAINT DF_RegionFeatures_v600_PvP DEFAULT (1),
            AllowAlchemy bit NOT NULL CONSTRAINT DF_RegionFeatures_v600_Alchemy DEFAULT (1),
            AllowSpecialItems bit NOT NULL CONSTRAINT DF_RegionFeatures_v600_Items DEFAULT (1),
            AllowBerserk bit NOT NULL CONSTRAINT DF_RegionFeatures_v600_Berserk DEFAULT (1),
            AutoPvpCape tinyint NOT NULL CONSTRAINT DF_RegionFeatures_v600_Cape DEFAULT (0),
            InactivityReturnSeconds int NOT NULL CONSTRAINT DF_RegionFeatures_v600_Inactivity DEFAULT (0),

            EventSuitMode tinyint NOT NULL CONSTRAINT DF_RegionFeatures_v600_EventSuit DEFAULT (0),
            ManagedEventCode nvarchar(32) NULL,
            ManagedAtUtc datetime2(0) NULL,
            CreatedAt datetime2(0) NOT NULL CONSTRAINT DF_RegionFeatures_v600_Created DEFAULT (SYSUTCDATETIME()),
            UpdatedAt datetime2(0) NULL
        );

        IF OBJECT_ID(N'dbo.Security_RegionFeatures', N'U') IS NOT NULL
        BEGIN
            IF OBJECT_ID(N'dbo.Security_RegionFeatures_Backup_v600', N'U') IS NULL
                SELECT * INTO dbo.Security_RegionFeatures_Backup_v600 FROM dbo.Security_RegionFeatures;

            DECLARE @World nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'WorldID') IS NULL THEN N'0' ELSE N'ISNULL(WorldID,0)' END;
            DECLARE @Region nvarchar(max) = N'CASE WHEN RegionID BETWEEN 32768 AND 65535 THEN RegionID-65536 ELSE RegionID END';
            DECLARE @RuleName nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'RuleName') IS NULL THEN N'NULL' ELSE N'NULLIF(LTRIM(RTRIM(RuleName)),N'''')' END;
            DECLARE @Enabled nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enabled') IS NULL THEN N'1' ELSE N'ISNULL(Enabled,1)' END;
            DECLARE @Build nvarchar(max) = CASE
                WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'BuildMode') IS NOT NULL THEN N'CASE WHEN BuildMode BETWEEN 0 AND 3 THEN BuildMode ELSE 0 END'
                ELSE N'CASE WHEN ISNULL(Allow_StrCharacter,1)=1 AND ISNULL(Allow_IntCharacter,1)=0 THEN 1 WHEN ISNULL(Allow_IntCharacter,1)=1 AND ISNULL(Allow_StrCharacter,1)=0 THEN 2 WHEN ISNULL(Allow_IntCharacter,1)=0 AND ISNULL(Allow_StrCharacter,1)=0 THEN 3 ELSE 0 END' END;
            DECLARE @Job nvarchar(max) = CASE
                WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'JobMode') IS NOT NULL THEN N'CASE WHEN JobMode BETWEEN 0 AND 5 THEN JobMode ELSE 0 END'
                WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Allow_Jobless') IS NOT NULL THEN N'CASE WHEN Allow_Jobless=1 AND Allow_Trader=0 AND Allow_Thief=0 AND Allow_Hunter=0 THEN 1 WHEN Allow_Jobless=0 AND Allow_Trader=1 AND Allow_Thief=1 AND Allow_Hunter=1 THEN 2 WHEN Allow_Jobless=0 AND Allow_Trader=1 AND Allow_Thief=0 AND Allow_Hunter=0 THEN 3 WHEN Allow_Jobless=0 AND Allow_Trader=0 AND Allow_Thief=1 AND Allow_Hunter=0 THEN 4 WHEN Allow_Jobless=0 AND Allow_Trader=0 AND Allow_Thief=0 AND Allow_Hunter=1 THEN 5 WHEN Allow_Jobless=1 AND Allow_Trader=1 AND Allow_Thief=1 AND Allow_Hunter=1 THEN 0 ELSE CASE WHEN ISNULL(Enable_JobMode,1)=0 THEN 1 ELSE 0 END END'
                ELSE N'CASE WHEN ISNULL(Enable_JobMode,1)=0 THEN 1 ELSE 0 END' END;
            DECLARE @Race nvarchar(max) = CASE
                WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'RaceMode') IS NOT NULL THEN N'CASE WHEN RaceMode BETWEEN 0 AND 2 THEN RaceMode ELSE 0 END'
                WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Allow_Chinese') IS NOT NULL THEN N'CASE WHEN Allow_Chinese=1 AND Allow_European=0 THEN 1 WHEN Allow_European=1 AND Allow_Chinese=0 THEN 2 ELSE 0 END'
                ELSE N'0' END;
            DECLARE @PartyMode nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'EntryPartyMode') IS NULL THEN N'0' ELSE N'CASE WHEN EntryPartyMode IN (0,1,2) THEN EntryPartyMode ELSE 0 END' END;
            DECLARE @MinLevel nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'MinLevel') IS NULL THEN N'0' ELSE N'ISNULL(MinLevel,0)' END;
            DECLARE @MaxLevel nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'MaxLevel') IS NULL THEN N'0' ELSE N'CASE WHEN MaxLevel>0 AND MinLevel>0 AND MaxLevel<MinLevel THEN 0 ELSE ISNULL(MaxLevel,0) END' END;
            DECLARE @Teleport nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enable_TeleportEntry') IS NULL THEN N'1' ELSE N'ISNULL(Enable_TeleportEntry,1)' END;
            DECLARE @Reverse nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enable_Reverse') IS NULL THEN N'1' ELSE N'ISNULL(Enable_Reverse,1)' END;
            DECLARE @Trace nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enable_Trace') IS NULL THEN N'1' ELSE N'ISNULL(Enable_Trace,1)' END;
            DECLARE @Movement nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enable_Move') IS NULL THEN N'1' ELSE N'ISNULL(Enable_Move,1)' END;
            DECLARE @Chat nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enable_Chat') IS NULL THEN N'1' ELSE N'ISNULL(Enable_Chat,1)' END;
            DECLARE @Global nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enable_Global') IS NULL THEN N'1' ELSE N'ISNULL(Enable_Global,1)' END;
            DECLARE @Party nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enable_Party') IS NULL THEN N'1' ELSE N'ISNULL(Enable_Party,1)' END;
            DECLARE @Exchange nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enable_Exchange') IS NULL THEN N'1' ELSE N'ISNULL(Enable_Exchange,1)' END;
            DECLARE @Stall nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enable_Stall') IS NULL THEN N'1' ELSE N'ISNULL(Enable_Stall,1)' END;
            DECLARE @Pvp nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enable_PvP') IS NULL THEN N'1' ELSE N'ISNULL(Enable_PvP,1)' END;
            DECLARE @Alchemy nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enable_Alchemy') IS NULL THEN N'1' ELSE N'ISNULL(Enable_Alchemy,1)' END;
            DECLARE @Items nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enable_FellowScroll') IS NULL THEN N'1' ELSE N'CASE WHEN ISNULL(Enable_FellowScroll,1)=1 AND ISNULL(Enable_ResurrectionScroll,1)=1 THEN 1 ELSE 0 END' END;
            DECLARE @Berserk nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enable_Zerk') IS NULL THEN N'1' ELSE N'ISNULL(Enable_Zerk,1)' END;
            DECLARE @Cape nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enable_AutoPvP') IS NULL THEN N'0' ELSE N'CASE WHEN Enable_AutoPvP BETWEEN 0 AND 5 THEN Enable_AutoPvP ELSE 0 END' END;
            DECLARE @Inactivity nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enable_InactivityReturn') IS NULL THEN N'0' ELSE N'CASE WHEN Enable_InactivityReturn=1 AND InactivitySeconds BETWEEN 1 AND 86400 THEN InactivitySeconds ELSE 0 END' END;
            DECLARE @EventSuit nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'Enable_EventSuit') IS NULL THEN N'0' ELSE N'CASE WHEN Enable_EventSuit=0 THEN 0 WHEN ISNULL(EventSuit_Team,0)=0 THEN 1 ELSE 2 END' END;
            DECLARE @EventCode nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'ManagedEventCode') IS NULL THEN N'NULL' ELSE N'ManagedEventCode' END;
            DECLARE @ManagedAt nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'ManagedAtUtc') IS NULL THEN N'NULL' ELSE N'ManagedAtUtc' END;
            DECLARE @Created nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'CreatedAt') IS NULL THEN N'SYSUTCDATETIME()' ELSE N'ISNULL(CreatedAt,SYSUTCDATETIME())' END;
            DECLARE @Updated nvarchar(max) = CASE WHEN COL_LENGTH(N'dbo.Security_RegionFeatures', N'UpdatedAt') IS NULL THEN N'NULL' ELSE N'UpdatedAt' END;

            DECLARE @CopySql nvarchar(max) = N'
SET IDENTITY_INSERT dbo.Security_RegionFeatures_v600_New ON;
INSERT dbo.Security_RegionFeatures_v600_New
(ID,WorldID,RegionID,RuleName,Enabled,BuildMode,JobMode,RaceMode,PartyMode,MinLevel,MaxLevel,
 AllowTeleport,AllowReverse,AllowTrace,AllowMovement,AllowChat,AllowGlobalChat,AllowParty,
 AllowExchange,AllowStall,AllowPvP,AllowAlchemy,AllowSpecialItems,AllowBerserk,AutoPvpCape,
 InactivityReturnSeconds,EventSuitMode,ManagedEventCode,ManagedAtUtc,CreatedAt,UpdatedAt)
SELECT ID,'+@World+N','+@Region+N','+@RuleName+N','+@Enabled+N','+@Build+N','+@Job+N','+@Race+N','+@PartyMode+N','+@MinLevel+N','+@MaxLevel+N','+
@Teleport+N','+@Reverse+N','+@Trace+N','+@Movement+N','+@Chat+N','+@Global+N','+@Party+N','+
@Exchange+N','+@Stall+N','+@Pvp+N','+@Alchemy+N','+@Items+N','+@Berserk+N','+@Cape+N','+
@Inactivity+N','+@EventSuit+N','+@EventCode+N','+@ManagedAt+N','+@Created+N','+@Updated+N'
FROM dbo.Security_RegionFeatures
WHERE RegionID<>0;
SET IDENTITY_INSERT dbo.Security_RegionFeatures_v600_New OFF;';
            EXEC sys.sp_executesql @CopySql;

            ;WITH duplicates AS
            (
                SELECT ID, ROW_NUMBER() OVER (PARTITION BY WorldID,RegionID ORDER BY ID DESC) AS rn
                FROM dbo.Security_RegionFeatures_v600_New
            )
            DELETE FROM duplicates WHERE rn > 1;

            DROP TABLE dbo.Security_RegionFeatures;
        END;

        EXEC sys.sp_rename N'dbo.Security_RegionFeatures_v600_New', N'Security_RegionFeatures';
        EXEC sys.sp_rename N'dbo.PK_RegionFeatures_v600_New', N'PK_Security_RegionFeatures', N'OBJECT';
    END;

    EXEC(N'
    UPDATE dbo.Security_RegionFeatures
    SET RegionID = CASE WHEN RegionID BETWEEN 32768 AND 65535 THEN RegionID-65536 ELSE RegionID END,
        BuildMode = CASE WHEN BuildMode BETWEEN 0 AND 3 THEN BuildMode ELSE 0 END,
        JobMode = CASE WHEN JobMode BETWEEN 0 AND 5 THEN JobMode ELSE 0 END,
        RaceMode = CASE WHEN RaceMode BETWEEN 0 AND 2 THEN RaceMode ELSE 0 END,
        PartyMode = CASE WHEN PartyMode BETWEEN 0 AND 2 THEN PartyMode ELSE 0 END,
        MaxLevel = CASE WHEN MaxLevel>0 AND MinLevel>0 AND MaxLevel<MinLevel THEN 0 ELSE MaxLevel END,
        AutoPvpCape = CASE WHEN AutoPvpCape BETWEEN 0 AND 5 THEN AutoPvpCape ELSE 0 END,
        InactivityReturnSeconds = CASE WHEN InactivityReturnSeconds BETWEEN 0 AND 86400 THEN InactivityReturnSeconds ELSE 0 END,
        EventSuitMode = CASE WHEN EventSuitMode BETWEEN 0 AND 2 THEN EventSuitMode ELSE 0 END;');

    IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id=OBJECT_ID(N'dbo.Security_RegionFeatures') AND name=N'UX_Security_RegionFeatures_World_Region')
        EXEC(N'CREATE UNIQUE INDEX UX_Security_RegionFeatures_World_Region ON dbo.Security_RegionFeatures(WorldID,RegionID);');
    IF NOT EXISTS (SELECT 1 FROM sys.check_constraints WHERE parent_object_id=OBJECT_ID(N'dbo.Security_RegionFeatures') AND name=N'CK_Security_RegionFeatures_RegionID')
        EXEC(N'ALTER TABLE dbo.Security_RegionFeatures WITH CHECK ADD CONSTRAINT CK_Security_RegionFeatures_RegionID CHECK (RegionID BETWEEN -32768 AND 32767 AND RegionID<>0);');
    IF NOT EXISTS (SELECT 1 FROM sys.check_constraints WHERE parent_object_id=OBJECT_ID(N'dbo.Security_RegionFeatures') AND name=N'CK_Security_RegionFeatures_Modes')
        EXEC(N'ALTER TABLE dbo.Security_RegionFeatures WITH CHECK ADD CONSTRAINT CK_Security_RegionFeatures_Modes CHECK (BuildMode BETWEEN 0 AND 3 AND JobMode BETWEEN 0 AND 5 AND RaceMode BETWEEN 0 AND 2 AND PartyMode BETWEEN 0 AND 2 AND EventSuitMode BETWEEN 0 AND 2);');
    IF NOT EXISTS (SELECT 1 FROM sys.check_constraints WHERE parent_object_id=OBJECT_ID(N'dbo.Security_RegionFeatures') AND name=N'CK_Security_RegionFeatures_Values')
        EXEC(N'ALTER TABLE dbo.Security_RegionFeatures WITH CHECK ADD CONSTRAINT CK_Security_RegionFeatures_Values CHECK ((MaxLevel=0 OR MinLevel=0 OR MaxLevel>=MinLevel) AND AutoPvpCape BETWEEN 0 AND 5 AND InactivityReturnSeconds BETWEEN 0 AND 86400);');

    COMMIT TRANSACTION;
END TRY
BEGIN CATCH
    IF @@TRANCOUNT > 0 ROLLBACK TRANSACTION;
    THROW;
END CATCH;
