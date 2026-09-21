SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.Clientless_HuntAreas', N'U') IS NULL
    THROW 51000, 'Clientless_HuntAreas is missing.', 1;
IF OBJECT_ID(N'dbo.Clientless_HuntPolicy', N'U') IS NULL
    THROW 51001, 'Clientless_HuntPolicy is missing.', 1;
IF COL_LENGTH(N'dbo.Clientless_Accounts', N'HuntEnabled') IS NULL
    THROW 51002, 'Clientless_Accounts.HuntEnabled is missing.', 1;
IF COL_LENGTH(N'dbo.Clientless_Accounts', N'HuntStatus') IS NULL
    THROW 51003, 'Clientless_Accounts.HuntStatus is missing.', 1;
IF EXISTS
(
    SELECT City
    FROM dbo.Clientless_HuntAreas
    GROUP BY City
    HAVING COUNT(*) <> 5 OR MIN(SlotNumber) <> 1 OR MAX(SlotNumber) <> 5
)
    THROW 51004, 'Every configured Clientless hunting city must contain exactly five numbered areas.', 1;
IF NOT EXISTS (SELECT 1 FROM dbo.Clientless_HuntPolicy WHERE SettingID = 1)
    THROW 51005, 'Clientless hunting policy row is missing.', 1;

SELECT N'Clientless hunting schema validation passed.' AS ValidationResult;
