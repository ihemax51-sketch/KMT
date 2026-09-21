USE [KMTGuard];
GO

IF OBJECT_ID(N'dbo.Clientless_Accounts', N'U') IS NULL
    THROW 51000, 'dbo.Clientless_Accounts is missing. Apply the earlier Clientless database update first.', 1;
GO

IF COL_LENGTH(N'dbo.Clientless_Accounts', N'City') IS NULL
BEGIN
    ALTER TABLE dbo.Clientless_Accounts
        ADD City varchar(32) NOT NULL
            CONSTRAINT DF_ClientlessAccounts_City DEFAULT('Unassigned');
END;
GO

UPDATE dbo.Clientless_Accounts
SET City = 'Unassigned'
WHERE NULLIF(LTRIM(RTRIM(City)), '') IS NULL;
GO

DECLARE @ShardDatabase sysname =
(
    SELECT TOP (1) NULLIF(LTRIM(RTRIM(CONVERT(nvarchar(128), Value))), N'')
    FROM dbo.System_Settings
    WHERE SettingName = N'ShardDB'
);

IF @ShardDatabase IS NOT NULL AND DB_ID(@ShardDatabase) IS NOT NULL
BEGIN
    DECLARE @BackfillSql nvarchar(max) = N'
UPDATE accounts
SET City = CASE
    WHEN characters.LatestRegion IN (24999, 25000, 25001) THEN ''Jangan''
    WHEN characters.LatestRegion IN (26265, 26521) THEN ''Donwhang''
    WHEN characters.LatestRegion IN (23431, 23686, 23687, 23688, 23943) THEN ''Hotan''
    WHEN characters.LatestRegion IN (27243, 27244, 27499, 27500) THEN ''SamarKand''
    WHEN characters.LatestRegion IN (26702, 26957, 26958, 26959, 27471) THEN ''Constantinople''
    WHEN characters.LatestRegion IN (23602, 23603) THEN ''Alexandria North (SD)''
    ELSE accounts.City
END
FROM dbo.Clientless_Accounts AS accounts
INNER JOIN ' + QUOTENAME(@ShardDatabase) + N'.dbo._Char AS characters
    ON characters.CharName16 = accounts.CharacterName
WHERE accounts.City = ''Unassigned'';';

    EXEC sys.sp_executesql @BackfillSql;
END;
GO

IF NOT EXISTS
(
    SELECT 1
    FROM sys.check_constraints
    WHERE parent_object_id = OBJECT_ID(N'dbo.Clientless_Accounts')
      AND name = N'CK_ClientlessAccounts_City'
)
BEGIN
    ALTER TABLE dbo.Clientless_Accounts WITH CHECK
        ADD CONSTRAINT CK_ClientlessAccounts_City CHECK
        (
            City IN
            (
                'Unassigned',
                'Jangan',
                'Donwhang',
                'Hotan',
                'SamarKand',
                'Constantinople',
                'Alexandria North (SD)'
            )
        );
END;
GO

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Clientless_Accounts')
      AND name = N'IX_ClientlessAccounts_CityEnabled'
)
BEGIN
    CREATE INDEX IX_ClientlessAccounts_CityEnabled
        ON dbo.Clientless_Accounts(City, Enabled, ID);
END;
GO
