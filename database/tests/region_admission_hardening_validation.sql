SET NOCOUNT ON;

IF DB_NAME() <> N'KMTGuard'
    THROW 51100, 'Run this validation in the KMTGuard database.', 1;
IF OBJECT_ID(N'dbo.Security_RegionFeatures', N'U') IS NULL
    THROW 51101, 'dbo.Security_RegionFeatures is missing.', 1;

DECLARE @Required table (ColumnName sysname NOT NULL PRIMARY KEY);
INSERT @Required VALUES
 (N'ID'),(N'WorldID'),(N'RegionID'),(N'RuleName'),(N'Enabled'),
 (N'BuildMode'),(N'JobMode'),(N'RaceMode'),(N'PartyMode'),(N'MinLevel'),(N'MaxLevel'),
 (N'AllowTeleport'),(N'AllowReverse'),(N'AllowTrace'),(N'AllowMovement'),(N'AllowChat'),
 (N'AllowGlobalChat'),(N'AllowParty'),(N'AllowExchange'),(N'AllowStall'),(N'AllowPvP'),
 (N'AllowAlchemy'),(N'AllowSpecialItems'),(N'AllowBerserk'),(N'AutoPvpCape'),
 (N'InactivityReturnSeconds'),(N'EventSuitMode'),(N'ManagedEventCode'),(N'ManagedAtUtc'),
 (N'CreatedAt'),(N'UpdatedAt');

IF EXISTS (SELECT 1 FROM @Required r WHERE COL_LENGTH(N'dbo.Security_RegionFeatures',r.ColumnName) IS NULL)
    THROW 51102, 'One or more clean Region Control columns are missing.', 1;

IF (SELECT COUNT(*) FROM sys.columns WHERE object_id=OBJECT_ID(N'dbo.Security_RegionFeatures')) <> 31
    THROW 51103, 'Security_RegionFeatures contains unexpected legacy or duplicate columns.', 1;

IF EXISTS
(
    SELECT 1 FROM dbo.Security_RegionFeatures
    WHERE RegionID=0 OR RegionID NOT BETWEEN -32768 AND 32767
       OR BuildMode NOT BETWEEN 0 AND 3 OR JobMode NOT BETWEEN 0 AND 5
       OR RaceMode NOT BETWEEN 0 AND 2 OR PartyMode NOT BETWEEN 0 AND 2
       OR EventSuitMode NOT BETWEEN 0 AND 2 OR AutoPvpCape NOT BETWEEN 0 AND 5
       OR InactivityReturnSeconds NOT BETWEEN 0 AND 86400
       OR (MaxLevel>0 AND MinLevel>0 AND MaxLevel<MinLevel)
)
    THROW 51104, 'Invalid Region Control values were found.', 1;

IF EXISTS (SELECT WorldID,RegionID FROM dbo.Security_RegionFeatures GROUP BY WorldID,RegionID HAVING COUNT_BIG(*)>1)
    THROW 51105, 'Duplicate WorldID/RegionID rules were found.', 1;

SELECT N'PASS' ValidationResult, COUNT_BIG(*) RegionRuleCount,
       SUM(CASE WHEN RegionID<0 THEN 1 ELSE 0 END) SignedRegionRuleCount
FROM dbo.Security_RegionFeatures;
