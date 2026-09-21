USE [KMTGuard];
GO

SET NOCOUNT ON;
GO

DECLARE @Missing nvarchar(max) = N'';

SELECT @Missing = CONCAT(@Missing, CASE WHEN @Missing = N'' THEN N'' ELSE N', ' END, required.Name)
FROM (VALUES
    (N'PVP_Settings'),
    (N'PVP_Arenas'),
    (N'PVP_Matches'),
    (N'PVP_KillLog'),
    (N'Teleport_FreezeQueue')
) required(Name)
WHERE OBJECT_ID(N'dbo.' + required.Name, N'U') IS NULL;

IF @Missing <> N''
BEGIN
    DECLARE @MissingMessage nvarchar(2048) = CONCAT(N'Missing PvP Challenge tables: ', @Missing);
    THROW 51140, @MissingMessage, 1;
END;

IF OBJECT_ID(N'dbo.TR_PVP_Matches_WagerSafe', N'TR') IS NULL
    THROW 51141, 'PvP Challenge wager safeguard trigger is missing.', 1;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.check_constraints
    WHERE parent_object_id = OBJECT_ID(N'dbo.PVP_Settings')
      AND name = N'CK_PVP_Settings_MinGoldSafe'
      AND is_disabled = 0
)
    THROW 51142, 'PvP Challenge minimum wager safeguard is missing or disabled.', 1;

IF OBJECT_ID(N'dbo.Teleport_PositionFreeze', N'P') IS NULL OR
   OBJECT_DEFINITION(OBJECT_ID(N'dbo.Teleport_PositionFreeze')) NOT LIKE N'%dbo.Teleport_FreezeQueue%'
    THROW 51143, 'PvP Challenge teleport procedure does not use the active queue table.', 1;

IF OBJECT_ID(N'dbo.Teleport_PlayerToTown', N'P') IS NULL OR
   OBJECT_DEFINITION(OBJECT_ID(N'dbo.Teleport_PlayerToTown')) NOT LIKE N'%dbo.PVP_Settings%' OR
   OBJECT_DEFINITION(OBJECT_ID(N'dbo.Teleport_PlayerToTown')) NOT LIKE N'%dbo.Teleport_Position%'
    THROW 51144, 'PvP Challenge town procedure does not use the active settings table.', 1;

SELECT
    N'PASS' AS ValidationResult,
    (SELECT COUNT_BIG(*) FROM dbo.PVP_Arenas WHERE Enabled = 1) AS EnabledArenaCount,
    (SELECT COUNT_BIG(*) FROM dbo.PVP_Matches WHERE Status IN (0, 1, 7)) AS RecoverableOpenMatchCount;
GO
