USE [KMTGuard];
GO
SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.Command_ClaimGameServer', N'P') IS NULL
    THROW 51330, 'dbo.Command_ClaimGameServer is missing.', 1;
IF OBJECT_ID(N'dbo.Vip_ProcessPendingItemMallPurchases', N'P') IS NULL
    THROW 51331, 'dbo.Vip_ProcessPendingItemMallPurchases is missing.', 1;

DECLARE @GameServerClaimDefinition nvarchar(max) = OBJECT_DEFINITION(OBJECT_ID(N'dbo.Command_ClaimGameServer'));
DECLARE @VipPurchaseDefinition nvarchar(max) = OBJECT_DEFINITION(OBJECT_ID(N'dbo.Vip_ProcessPendingItemMallPurchases'));

IF @GameServerClaimDefinition NOT LIKE N'%SET TRANSACTION ISOLATION LEVEL READ COMMITTED%'
    THROW 51332, 'GameServer command claiming does not establish READ COMMITTED isolation.', 1;
IF @VipPurchaseDefinition NOT LIKE N'%SET TRANSACTION ISOLATION LEVEL READ COMMITTED%'
    THROW 51333, 'VIP purchase claiming does not establish READ COMMITTED isolation.', 1;
IF @GameServerClaimDefinition NOT LIKE N'%READPAST%READCOMMITTEDLOCK%'
    THROW 51334, 'GameServer command claiming is not compatible with READ_COMMITTED_SNAPSHOT.', 1;
IF @VipPurchaseDefinition NOT LIKE N'%READPAST%READCOMMITTEDLOCK%'
    THROW 51335, 'VIP purchase claiming is not compatible with READ_COMMITTED_SNAPSHOT.', 1;

-- Reproduce the inherited-session condition without claiming or changing data.
SET TRANSACTION ISOLATION LEVEL SERIALIZABLE;
BEGIN TRANSACTION;
SET TRANSACTION ISOLATION LEVEL READ COMMITTED;

SELECT TOP (0) ID
FROM dbo.Command_GameServerQueue WITH (UPDLOCK, READPAST, READCOMMITTEDLOCK, ROWLOCK);

SELECT TOP (0) EventID
FROM dbo.Vip_ItemMallPurchaseEvents WITH (UPDLOCK, READPAST, READCOMMITTEDLOCK, ROWLOCK);

ROLLBACK TRANSACTION;
SET TRANSACTION ISOLATION LEVEL READ COMMITTED;

SELECT
    DB_NAME() AS DatabaseName,
    CONVERT(int, DATABASEPROPERTYEX(DB_NAME(), 'IsReadCommittedSnapshotOn')) AS IsReadCommittedSnapshotOn,
    N'PASS' AS ValidationResult;
GO
