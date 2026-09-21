/*
Retires the old Unique/Honor point writers without changing the unique-kill
hook contract. The hook can continue achievements, event handling and its
filter command while dynamic ranks remain fully SQL-customer-defined.
*/

IF DB_ID(N'hema_system') IS NOT NULL
BEGIN
    EXEC hema_system.sys.sp_executesql N'
CREATE OR ALTER PROCEDURE dbo.Unique_Rank
    @RefObjID int,
    @KillerName varchar(64)
AS
BEGIN
    SET NOCOUNT ON;
    RETURN;
END;
';

    EXEC hema_system.sys.sp_executesql N'
CREATE OR ALTER PROCEDURE dbo.HonorRank_Uniques
    @RefObjID int,
    @KillerName varchar(64)
AS
BEGIN
    SET NOCOUNT ON;
    RETURN;
END;
';
END;
GO

PRINT 'Legacy fixed Unique/Honor ranking writers retired.';
GO
