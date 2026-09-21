SET NOCOUNT ON;
SET XACT_ABORT ON;

DECLARE @Missing TABLE(ObjectName SYSNAME NOT NULL);

INSERT @Missing(ObjectName)
SELECT Required.ObjectName
FROM
(
    VALUES
        (N'dbo.Item_ChestClaim', N'U'),
        (N'dbo.Item_ChestClaimResolutionLog', N'U'),
        (N'dbo.Item_ChestBroadcast', N'U'),
        (N'dbo.Item_ChestBroadcastRecipient', N'U'),
        (N'dbo.Item_AddChest', N'P'),
        (N'dbo.Item_AddChestByCodeName', N'P'),
        (N'dbo.Item_ChestProcessClaim', N'P'),
        (N'dbo.Item_ChestSendToOnline', N'P'),
        (N'dbo.Item_ChestSendToAll', N'P')
) AS Required(ObjectName, ObjectType)
WHERE OBJECT_ID(Required.ObjectName, Required.ObjectType) IS NULL;

IF EXISTS (SELECT 1 FROM @Missing)
BEGIN
    SELECT ObjectName AS MissingObject FROM @Missing ORDER BY ObjectName;
    THROW 51100, 'Item Chest integrity installation is incomplete.', 1;
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.check_constraints
    WHERE parent_object_id = OBJECT_ID(N'dbo.Item_ChestClaim')
      AND name = N'CK_Item_ChestClaim_State'
)
    THROW 51101, 'Item Chest claim state constraint is missing.', 1;

IF EXISTS
(
    SELECT ChestID
    FROM dbo.Item_ChestClaim
    GROUP BY ChestID
    HAVING COUNT_BIG(*) > 1
)
    THROW 51102, 'Duplicate Item Chest claim rows exist.', 1;

SELECT
    CAST(1 AS BIT) AS ValidationSucceeded,
    (SELECT COUNT_BIG(*) FROM dbo.Item_ChestClaim WHERE State = 0) AS PendingClaims,
    (SELECT COUNT_BIG(*) FROM dbo.Item_ChestClaim WHERE State = 1) AS DeliveredClaims,
    (SELECT COUNT_BIG(*) FROM dbo.Item_ChestBroadcast) AS BroadcastBatches,
    (SELECT COUNT_BIG(*) FROM dbo.Item_ChestBroadcastRecipient) AS BroadcastRecipients;
