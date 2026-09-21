USE [KMTGuard];
GO

SET NOCOUNT ON;
GO

IF OBJECT_ID(N'dbo.System_Settings', N'U') IS NULL
    THROW 51020, 'KMTGuard.dbo.System_Settings was not found.', 1;

IF
(
    SELECT COUNT_BIG(*)
    FROM dbo.System_Settings
    WHERE SettingName = N'VipSystemEnabled'
) <> 1
    THROW 51021, 'VipSystemEnabled must exist exactly once in dbo.System_Settings.', 1;

IF OBJECT_DEFINITION(OBJECT_ID(N'dbo.Vip_OnCharacterLogin')) NOT LIKE N'%VipSystemEnabled%'
    THROW 51022, 'Vip_OnCharacterLogin does not contain the VIP enable guard.', 1;

SELECT SettingName, Value
FROM dbo.System_Settings
WHERE SettingName = N'VipSystemEnabled';
GO
