USE [KMTGuard]
GO

SET ANSI_NULLS ON
GO
SET QUOTED_IDENTIFIER ON
GO

/*
    Customer-owned Job control procedures.

    The filter calls these procedures before the corresponding Job action.
    Return 0 to reject the action or 1 to allow it. The filter does not send a
    rejection message, so customized procedures must notify the character when
    they return 0.

    Existing procedures are deliberately left untouched. This makes the
    migration safe to run again and preserves customer changes across filter
    restarts and upgrades.
*/

IF OBJECT_ID(N'dbo._JobJoin', N'P') IS NULL
BEGIN
    EXEC(N'
CREATE PROCEDURE [dbo].[_JobJoin]
    @CharID int,
    @JobType tinyint,
    @Charname varchar(25)
AS
BEGIN
    SET NOCOUNT ON;

    /*
        @JobType:
            1 = Trader
            2 = Thief
            3 = Hunter
    */
    RETURN 1;
END');
END
GO

IF OBJECT_ID(N'dbo._JobLeave', N'P') IS NULL
BEGIN
    EXEC(N'
CREATE PROCEDURE [dbo].[_JobLeave]
    @CharID int,
    @Charname varchar(25)
AS
BEGIN
    SET NOCOUNT ON;

    RETURN 1;
END');
END
GO

IF OBJECT_ID(N'dbo._JobSuitEquip', N'P') IS NULL
BEGIN
    EXEC(N'
CREATE PROCEDURE [dbo].[_JobSuitEquip]
    @CharID int,
    @SuitItemID int,
    @Charname varchar(25)
AS
BEGIN
    SET NOCOUNT ON;

    RETURN 1;
END');
END
GO

IF OBJECT_ID(N'dbo._JobSuitRemove', N'P') IS NULL
BEGIN
    EXEC(N'
CREATE PROCEDURE [dbo].[_JobSuitRemove]
    @CharID int,
    @Charname varchar(25)
AS
BEGIN
    SET NOCOUNT ON;

    RETURN 1;
END');
END
GO

PRINT 'KMTGuard Job control procedures are available. Existing customer definitions were preserved.';
GO
