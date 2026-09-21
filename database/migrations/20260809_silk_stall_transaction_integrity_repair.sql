/*
    Corrected, self-contained Silk Stall integrity migration.
    Run on the KMTGuard/proxy database. Safe to run more than once.
*/

SET NOCOUNT ON;
SET XACT_ABORT ON;

BEGIN TRY
BEGIN TRANSACTION;

IF OBJECT_ID(N'dbo.Stall_SilkTransactions', N'U') IS NULL
   AND OBJECT_ID(N'dbo.SilkStallTransactions', N'U') IS NOT NULL
BEGIN
    EXEC sys.sp_rename N'dbo.SilkStallTransactions', N'Stall_SilkTransactions';
END;

IF OBJECT_ID(N'dbo.Stall_SilkTransactions', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Stall_SilkTransactions
    (
        ID BIGINT IDENTITY(1,1) NOT NULL,
        RequestToken UNIQUEIDENTIFIER NOT NULL,
        BuyerJID INT NOT NULL,
        BuyerCharID INT NOT NULL,
        BuyerCharName NVARCHAR(64) NOT NULL,
        BuyerUniqueID BIGINT NOT NULL,
        SellerJID INT NOT NULL,
        SellerCharID INT NOT NULL,
        SellerCharName NVARCHAR(64) NOT NULL,
        SellerUniqueID BIGINT NOT NULL,
        StallSlot TINYINT NOT NULL,
        InventorySlot TINYINT NOT NULL,
        Quantity SMALLINT NOT NULL,
        ItemTid BIGINT NOT NULL,
        SilkAmount INT NOT NULL,
        [Status] TINYINT NOT NULL CONSTRAINT DF_Stall_SilkTransactions_Status DEFAULT (0),
        FailureReason NVARCHAR(256) NULL,
        CreatedAt DATETIME2(0) NOT NULL CONSTRAINT DF_Stall_SilkTransactions_CreatedAt DEFAULT (SYSUTCDATETIME()),
        ReservedAt DATETIME2(0) NULL,
        AcceptedAt DATETIME2(0) NULL,
        CompletedAt DATETIME2(0) NULL,
        RefundedAt DATETIME2(0) NULL,
        UpdatedAt DATETIME2(0) NOT NULL CONSTRAINT DF_Stall_SilkTransactions_UpdatedAt DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT PK_Stall_SilkTransactions PRIMARY KEY CLUSTERED (ID),
        CONSTRAINT CK_Stall_SilkTransactions_SilkAmount CHECK (SilkAmount > 0),
        CONSTRAINT CK_Stall_SilkTransactions_Status CHECK ([Status] IN (0, 1, 2, 3, 4, 5)),
        CONSTRAINT CK_Stall_SilkTransactions_StallSlot CHECK (StallSlot BETWEEN 0 AND 9),
        CONSTRAINT CK_Stall_SilkTransactions_Quantity CHECK (Quantity > 0)
    );
END;

IF COL_LENGTH(N'dbo.Stall_SilkTransactions', N'RequestToken') IS NULL
    ALTER TABLE dbo.Stall_SilkTransactions ADD RequestToken UNIQUEIDENTIFIER NULL;
IF COL_LENGTH(N'dbo.Stall_SilkTransactions', N'InventorySlot') IS NULL
    ALTER TABLE dbo.Stall_SilkTransactions ADD InventorySlot TINYINT NULL;
IF COL_LENGTH(N'dbo.Stall_SilkTransactions', N'Quantity') IS NULL
    ALTER TABLE dbo.Stall_SilkTransactions ADD Quantity SMALLINT NULL;
IF COL_LENGTH(N'dbo.Stall_SilkTransactions', N'ItemTid') IS NULL
    ALTER TABLE dbo.Stall_SilkTransactions ADD ItemTid BIGINT NULL;
IF COL_LENGTH(N'dbo.Stall_SilkTransactions', N'AcceptedAt') IS NULL
    ALTER TABLE dbo.Stall_SilkTransactions ADD AcceptedAt DATETIME2(0) NULL;
IF COL_LENGTH(N'dbo.Stall_SilkTransactions', N'UpdatedAt') IS NULL
    ALTER TABLE dbo.Stall_SilkTransactions ADD UpdatedAt DATETIME2(0) NULL;

EXEC sys.sp_executesql N'
UPDATE dbo.Stall_SilkTransactions
SET RequestToken = COALESCE(RequestToken, NEWID()),
    InventorySlot = COALESCE(InventorySlot, 0),
    Quantity = COALESCE(Quantity, 1),
    ItemTid = COALESCE(ItemTid, 0),
    UpdatedAt = COALESCE(UpdatedAt, CreatedAt, SYSUTCDATETIME())
WHERE RequestToken IS NULL
   OR InventorySlot IS NULL
   OR Quantity IS NULL
   OR ItemTid IS NULL
   OR UpdatedAt IS NULL;

ALTER TABLE dbo.Stall_SilkTransactions ALTER COLUMN RequestToken UNIQUEIDENTIFIER NOT NULL;
ALTER TABLE dbo.Stall_SilkTransactions ALTER COLUMN InventorySlot TINYINT NOT NULL;
ALTER TABLE dbo.Stall_SilkTransactions ALTER COLUMN Quantity SMALLINT NOT NULL;
ALTER TABLE dbo.Stall_SilkTransactions ALTER COLUMN ItemTid BIGINT NOT NULL;
ALTER TABLE dbo.Stall_SilkTransactions ALTER COLUMN UpdatedAt DATETIME2(0) NOT NULL;';

DECLARE @statusConstraint SYSNAME;
DECLARE @dropStatusConstraintSql NVARCHAR(1000);

SELECT TOP (1) @statusConstraint = cc.name
FROM sys.check_constraints AS cc
WHERE cc.parent_object_id = OBJECT_ID(N'dbo.Stall_SilkTransactions')
  AND cc.definition LIKE N'%Status%';

IF @statusConstraint IS NOT NULL
BEGIN
    SET @dropStatusConstraintSql =
        N'ALTER TABLE dbo.Stall_SilkTransactions DROP CONSTRAINT ' + QUOTENAME(@statusConstraint) + N';';
    EXEC sys.sp_executesql @dropStatusConstraintSql;
END;

ALTER TABLE dbo.Stall_SilkTransactions WITH CHECK
ADD CONSTRAINT CK_Stall_SilkTransactions_Status CHECK ([Status] IN (0, 1, 2, 3, 4, 5));

IF NOT EXISTS
(
    SELECT 1 FROM sys.default_constraints
    WHERE parent_object_id = OBJECT_ID(N'dbo.Stall_SilkTransactions')
      AND parent_column_id = COLUMNPROPERTY(OBJECT_ID(N'dbo.Stall_SilkTransactions'), N'UpdatedAt', 'ColumnId')
)
    EXEC sys.sp_executesql N'
        ALTER TABLE dbo.Stall_SilkTransactions
        ADD CONSTRAINT DF_Stall_SilkTransactions_UpdatedAt
        DEFAULT (SYSUTCDATETIME()) FOR UpdatedAt;';

IF NOT EXISTS
(
    SELECT 1 FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Stall_SilkTransactions')
      AND name = N'UX_Stall_SilkTransactions_RequestToken'
)
    EXEC sys.sp_executesql N'
        CREATE UNIQUE NONCLUSTERED INDEX UX_Stall_SilkTransactions_RequestToken
        ON dbo.Stall_SilkTransactions(RequestToken);';

IF NOT EXISTS
(
    SELECT 1 FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Stall_SilkTransactions')
      AND name = N'IX_Stall_SilkTransactions_Recovery'
)
    EXEC sys.sp_executesql N'
        CREATE NONCLUSTERED INDEX IX_Stall_SilkTransactions_Recovery
        ON dbo.Stall_SilkTransactions([Status], UpdatedAt)
        INCLUDE (BuyerJID, SellerJID, SilkAmount);';

IF OBJECT_ID(N'dbo.SilkStallTransactions', N'U') IS NULL
BEGIN
    IF OBJECT_ID(N'dbo.SilkStallTransactions', N'SN') IS NOT NULL
        DROP SYNONYM dbo.SilkStallTransactions;
    CREATE SYNONYM dbo.SilkStallTransactions FOR dbo.Stall_SilkTransactions;
END;

COMMIT TRANSACTION;

PRINT 'Corrected Silk Stall transaction integrity migration completed.';
END TRY
BEGIN CATCH
    IF XACT_STATE() <> 0
        ROLLBACK TRANSACTION;
    THROW;
END CATCH;
