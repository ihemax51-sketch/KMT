/*
    Hook_SpawnComplete — OnSpawnComplete hook procedure
    Called by KMTGuard Filter after the character is fully spawned (500ms after 0x3012).

    Parameters:
        @CharID      INT           — Character database ID
        @CharName    NVARCHAR(64)  — Character name
        @RegionID    INT           — Current region ID
        @WorldID     SMALLINT      — Current world/game world ID
        @JobType     TINYINT       — Job type (0=None, 1=Trader, 2=Thief, 3=Hunter, 4=No job)
        @InParty     BIT           — Whether the character is currently in a party
        @FirstSpawn  BIT           — True only on the first spawn after login (not on teleport/region change)

    Usage:
        Extend this procedure to add custom spawn actions such as:
        - VIP welcome messages
        - Spawn buffs or icons
        - PvP mode initialization
        - Announcements
        - Online tracking
        - Any post-spawn logic

    IMPORTANT:
        Keep execution time short. Heavy operations should be queued
        via Command_FilterQueue or Command_GameServerQueue instead.
*/

IF EXISTS (SELECT 1 FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[Hook_SpawnComplete]') AND type = N'P')
    DROP PROCEDURE [dbo].[Hook_SpawnComplete];
GO

CREATE PROCEDURE [dbo].[Hook_SpawnComplete]
    @CharID      INT,
    @CharName    NVARCHAR(64),
    @RegionID    INT,
    @WorldID     SMALLINT,
    @JobType     TINYINT,
    @InParty     BIT,
    @FirstSpawn  BIT
AS
BEGIN
    SET NOCOUNT ON;

    -- =============================================
    -- Placeholder: Add your custom spawn logic here.
    -- This procedure is called once per character spawn.
    -- @FirstSpawn = 1 only on the first spawn after login.
    -- =============================================

    -- Example: Log the spawn event (uncomment to use)
    -- INSERT INTO [dbo].[Log_SpawnComplete] (CharID, CharName, RegionID, WorldID, JobType, InParty, FirstSpawn, SpawnDateUtc)
    -- VALUES (@CharID, @CharName, @RegionID, @WorldID, @JobType, @InParty, @FirstSpawn, GETUTCDATE());

END;
GO
