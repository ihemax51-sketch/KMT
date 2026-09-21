/*
    Auto Equip level-aware contract for KMTGuard.

    AutoEquipMaxLevel rules:
      0       = no level limit
      1..255  = highest character level allowed to see and use Auto Equip

    KMTGuard supplies both procedure parameters from the authenticated Agent
    session. The client does not supply either value.
*/

USE [KMTGuard];
GO

SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.System_Settings', N'U') IS NULL
    THROW 51000, 'KMTGuard.dbo.System_Settings was not found.', 1;
GO

IF NOT EXISTS
(
    SELECT 1
    FROM dbo.System_Settings WITH (UPDLOCK, HOLDLOCK)
    WHERE SettingName = N'AutoEquipMaxLevel'
)
BEGIN
    INSERT dbo.System_Settings (SettingName, Value)
    VALUES (N'AutoEquipMaxLevel', N'90');
END;
GO

CREATE OR ALTER PROCEDURE dbo.Hook_AutoEquip
    @CharID INT,
    @CharLevel INT
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @CharID <= 0 OR @CharLevel <= 0
        RETURN;

    EXEC hema_system.dbo._AutoEquipt
        @CharID = @CharID,
        @data2 = @CharLevel;
END;
GO

PRINT 'Auto Equip now receives the authenticated character ID and level.';
GO
