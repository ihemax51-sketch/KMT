/* Removes the retired generic runtime event callback, if it was previously installed. */

USE [KMTGuard];
GO

SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.Hook_RuntimeEvent', N'P') IS NOT NULL
    DROP PROCEDURE dbo.Hook_RuntimeEvent;
GO

PRINT N'Retired runtime event callback removed.';
GO
