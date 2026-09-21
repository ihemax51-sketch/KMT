USE [KMTGuard];
GO

SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.Command_GameServerQueue', N'U') IS NOT NULL
BEGIN
    DELETE FROM dbo.Command_GameServerQueue
    WHERE Action_ID = 200;
END;
GO

IF OBJECT_ID(N'dbo.Command_GetHonorRankRefreshStatus', N'P') IS NOT NULL
    DROP PROCEDURE dbo.Command_GetHonorRankRefreshStatus;
GO

IF OBJECT_ID(N'dbo.Command_RefreshHonorRank', N'P') IS NOT NULL
    DROP PROCEDURE dbo.Command_RefreshHonorRank;
GO

IF OBJECT_ID(N'dbo.HonorRankRefreshHistory', N'U') IS NOT NULL
    DROP TABLE dbo.HonorRankRefreshHistory;
GO

PRINT 'Custom Honor Rank runtime refresh components removed. Original game Honor Rank data was not changed.';
GO
