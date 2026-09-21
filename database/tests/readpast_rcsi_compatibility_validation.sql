USE [KMTGuard];
GO
SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.Command_ClaimGameServer', N'P') IS NULL
    THROW 51310, 'dbo.Command_ClaimGameServer is missing.', 1;
IF OBJECT_ID(N'dbo.Vip_ProcessPendingItemMallPurchases', N'P') IS NULL
    THROW 51311, 'dbo.Vip_ProcessPendingItemMallPurchases is missing.', 1;

DECLARE @GameServerClaimDefinition nvarchar(max) = OBJECT_DEFINITION(OBJECT_ID(N'dbo.Command_ClaimGameServer'));
DECLARE @VipPurchaseDefinition nvarchar(max) = OBJECT_DEFINITION(OBJECT_ID(N'dbo.Vip_ProcessPendingItemMallPurchases'));

IF @GameServerClaimDefinition NOT LIKE N'%READPAST%READCOMMITTEDLOCK%'
    THROW 51312, 'GameServer command claiming is not compatible with READ_COMMITTED_SNAPSHOT.', 1;
IF @VipPurchaseDefinition NOT LIKE N'%READPAST%READCOMMITTEDLOCK%'
    THROW 51313, 'VIP purchase claiming is not compatible with READ_COMMITTED_SNAPSHOT.', 1;

SELECT
    DB_NAME() AS DatabaseName,
    CONVERT(int, DATABASEPROPERTYEX(DB_NAME(), 'IsReadCommittedSnapshotOn')) AS IsReadCommittedSnapshotOn,
    N'PASS' AS ValidationResult;
GO
