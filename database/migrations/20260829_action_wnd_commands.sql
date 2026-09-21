SET ANSI_NULLS ON;
GO

SET QUOTED_IDENTIFIER ON;
GO

IF DB_NAME() <> N'KMTGuard'
    THROW 51000, 'Run this update in the KMTGuard database.', 1;
GO

IF OBJECT_ID(N'[dbo].[_OnActionWndCommandPet]', N'P') IS NULL
BEGIN
    EXEC
    (
        N'
        CREATE PROCEDURE [dbo].[_OnActionWndCommandPet]
            @CharID INT,
            @ActionWndID INT
        AS
        BEGIN
            SET NOCOUNT ON;
        END
        '
    );
END
GO

ALTER PROCEDURE [dbo].[_OnActionWndCommandPet]
    @CharID INT,
    @ActionWndID INT
AS
BEGIN
    SET NOCOUNT ON;

    
    IF @ActionWndID = 10001
    BEGIN
        EXEC [dbo].[__TeleportToTownbyCharID] @CharID;
        RETURN;
    END;
END
GO
