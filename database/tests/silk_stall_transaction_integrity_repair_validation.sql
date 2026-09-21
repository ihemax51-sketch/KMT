SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.Stall_SilkTransactions', N'U') IS NULL
    THROW 51000, 'dbo.Stall_SilkTransactions is missing.', 1;

IF EXISTS
(
    SELECT required.name
    FROM (VALUES
        (N'RequestToken'), (N'InventorySlot'), (N'Quantity'), (N'ItemTid'),
        (N'AcceptedAt'), (N'UpdatedAt')) AS required(name)
    WHERE COL_LENGTH(N'dbo.Stall_SilkTransactions', required.name) IS NULL
)
    THROW 51000, 'Silk Stall transaction recovery columns are missing.', 1;

IF NOT EXISTS
(
    SELECT 1 FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Stall_SilkTransactions')
      AND name = N'UX_Stall_SilkTransactions_RequestToken'
      AND is_unique = 1
)
    THROW 51000, 'Silk Stall request-token uniqueness is not enforced.', 1;

IF NOT EXISTS
(
    SELECT 1 FROM sys.check_constraints
    WHERE parent_object_id = OBJECT_ID(N'dbo.Stall_SilkTransactions')
      AND name = N'CK_Stall_SilkTransactions_Status'
      AND is_disabled = 0
      AND is_not_trusted = 0
)
    THROW 51000, 'Silk Stall durable settlement states are not enforced.', 1;

SELECT N'Corrected Silk Stall integrity migration validation passed.' AS Result;
