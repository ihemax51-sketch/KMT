USE [KMTGuard];
GO

SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.System_Settings', N'U') IS NULL
    THROW 51000, 'KMTGuard.dbo.System_Settings was not found.', 1;

IF
(
    SELECT COUNT_BIG(*)
    FROM dbo.System_Settings
    WHERE SettingName = N'DisableGreenBook'
) <> 1
    THROW 51001, 'DisableGreenBook must exist exactly once in dbo.System_Settings.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.System_Settings
    WHERE SettingName = N'DisableGreenBook'
      AND Value NOT IN (N'True', N'False')
)
    THROW 51002, 'DisableGreenBook must contain True or False.', 1;

IF OBJECT_ID(N'dbo.vw_Settings_InvalidValues', N'V') IS NULL
    THROW 51003, 'dbo.vw_Settings_InvalidValues was not found.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.vw_Settings_InvalidValues
    WHERE SettingName = N'DisableGreenBook'
)
    THROW 51004, 'DisableGreenBook is reported as invalid.', 1;

SELECT SettingName, Value, Category, DisplayOrder, Description
FROM dbo.System_Settings
WHERE SettingName = N'DisableGreenBook';
GO
