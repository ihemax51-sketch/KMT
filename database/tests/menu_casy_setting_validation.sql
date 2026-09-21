USE [KMTGuard];
GO

SET NOCOUNT ON;
GO

IF OBJECT_ID(N'dbo.System_Settings', N'U') IS NULL
    THROW 51640, 'KMTGuard.dbo.System_Settings was not found.', 1;

IF
(
    SELECT COUNT_BIG(*)
    FROM dbo.System_Settings
    WHERE SettingName = N'MenuCasy'
) <> 1
    THROW 51641, 'MenuCasy must exist exactly once in dbo.System_Settings.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.System_Settings
    WHERE SettingName = N'MenuCasy'
      AND Value NOT IN (N'True', N'False')
)
    THROW 51642, 'MenuCasy must contain True or False.', 1;

IF OBJECT_ID(N'dbo.vw_Settings_InvalidValues', N'V') IS NULL
    THROW 51643, 'dbo.vw_Settings_InvalidValues was not found.', 1;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.sql_modules AS modules
    WHERE modules.object_id = OBJECT_ID(N'dbo.vw_Settings_InvalidValues', N'V')
      AND modules.definition LIKE N'%(N''MenuCasy'', N''bool'')%'
)
    THROW 51644, 'MenuCasy is not registered as a boolean in the settings type catalog.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.vw_Settings_InvalidValues
    WHERE SettingName = N'MenuCasy'
)
    THROW 51645, 'MenuCasy is reported as an invalid setting value.', 1;

SELECT SettingName, Value, Category, DisplayOrder, Description
FROM dbo.System_Settings
WHERE SettingName = N'MenuCasy';
GO
