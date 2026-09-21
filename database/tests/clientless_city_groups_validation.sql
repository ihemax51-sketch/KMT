USE [KMTGuard];
GO

IF OBJECT_ID(N'dbo.Clientless_Accounts', N'U') IS NULL
    THROW 51000, 'dbo.Clientless_Accounts is missing.', 1;

IF COL_LENGTH(N'dbo.Clientless_Accounts', N'City') IS NULL
    THROW 51001, 'Clientless city grouping column is missing.', 1;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.check_constraints
    WHERE parent_object_id = OBJECT_ID(N'dbo.Clientless_Accounts')
      AND name = N'CK_ClientlessAccounts_City'
      AND is_disabled = 0
      AND is_not_trusted = 0
)
    THROW 51002, 'Clientless city grouping constraint is missing or not trusted.', 1;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Clientless_Accounts')
      AND name = N'IX_ClientlessAccounts_CityEnabled'
      AND is_disabled = 0
)
    THROW 51003, 'Clientless city grouping index is missing or disabled.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.Clientless_Accounts
    WHERE City NOT IN
    (
        'Unassigned', 'Jangan', 'Donwhang', 'Hotan', 'SamarKand',
        'Constantinople', 'Alexandria North (SD)'
    )
)
    THROW 51004, 'An unsupported Clientless city group was found.', 1;

SELECT City, COUNT_BIG(*) AS AccountCount
FROM dbo.Clientless_Accounts
GROUP BY City
ORDER BY City;
GO
