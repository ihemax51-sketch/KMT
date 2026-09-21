USE [KMTGuard];
GO

IF OBJECT_ID(N'[dbo].[_BattlePassAdminPlayerStatus]', N'V') IS NOT NULL
    DROP VIEW [dbo].[_BattlePassAdminPlayerStatus];
GO

IF OBJECT_ID(N'[dbo].[_BattlePass_CreateMission]', N'SN') IS NOT NULL
    DROP SYNONYM [dbo].[_BattlePass_CreateMission];
IF OBJECT_ID(N'[dbo].[_BattlePass_CreateMissionByCodeName]', N'SN') IS NOT NULL
    DROP SYNONYM [dbo].[_BattlePass_CreateMissionByCodeName];
IF OBJECT_ID(N'[dbo].[_BattlePass_GrantXP]', N'SN') IS NOT NULL
    DROP SYNONYM [dbo].[_BattlePass_GrantXP];
GO

IF SCHEMA_ID(N'battlepass') IS NOT NULL
BEGIN
    IF OBJECT_ID(N'[battlepass].[CreateMission]', N'SN') IS NOT NULL
        DROP SYNONYM [battlepass].[CreateMission];
    IF OBJECT_ID(N'[battlepass].[CreateMissionByCodeName]', N'SN') IS NOT NULL
        DROP SYNONYM [battlepass].[CreateMissionByCodeName];
    IF OBJECT_ID(N'[battlepass].[GrantExperience]', N'SN') IS NOT NULL
        DROP SYNONYM [battlepass].[GrantExperience];
END;
GO

IF OBJECT_ID(N'[dbo].[BattlePass_AddMission]', N'P') IS NOT NULL
    DROP PROCEDURE [dbo].[BattlePass_AddMission];
IF OBJECT_ID(N'[dbo].[BattlePass_AddMissionByCode]', N'P') IS NOT NULL
    DROP PROCEDURE [dbo].[BattlePass_AddMissionByCode];
IF OBJECT_ID(N'[dbo].[BattlePass_AddXP]', N'P') IS NOT NULL
    DROP PROCEDURE [dbo].[BattlePass_AddXP];
GO

IF OBJECT_ID(N'[dbo].[_BattlePassPlayers]', N'U') IS NOT NULL
    DROP TABLE [dbo].[_BattlePassPlayers];
IF OBJECT_ID(N'[dbo].[_BattlePassMissions]', N'U') IS NOT NULL
    DROP TABLE [dbo].[_BattlePassMissions];
IF OBJECT_ID(N'[dbo].[_BattlePassMissionTypes]', N'U') IS NOT NULL
    DROP TABLE [dbo].[_BattlePassMissionTypes];
IF OBJECT_ID(N'[dbo].[_BattlePassLevels]', N'U') IS NOT NULL
    DROP TABLE [dbo].[_BattlePassLevels];
IF OBJECT_ID(N'[dbo].[_BattlePassSeasons]', N'U') IS NOT NULL
    DROP TABLE [dbo].[_BattlePassSeasons];
GO

IF SCHEMA_ID(N'battlepass') IS NOT NULL
   AND NOT EXISTS (SELECT 1 FROM sys.objects WHERE schema_id = SCHEMA_ID(N'battlepass'))
   AND NOT EXISTS (SELECT 1 FROM sys.synonyms WHERE schema_id = SCHEMA_ID(N'battlepass'))
    EXEC(N'DROP SCHEMA [battlepass]');
GO
