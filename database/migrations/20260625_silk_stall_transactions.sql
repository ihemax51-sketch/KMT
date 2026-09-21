/*
    Silk stall transaction log for KMTGuard.

    Run on the KMTGuard/proxy database.
*/

SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.SilkStallTransactions', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.SilkStallTransactions
    (
        ID BIGINT IDENTITY(1,1) NOT NULL,
        BuyerJID INT NOT NULL,
        BuyerCharID INT NOT NULL,
        BuyerCharName NVARCHAR(64) NOT NULL,
        BuyerUniqueID BIGINT NOT NULL,
        SellerJID INT NOT NULL,
        SellerCharID INT NOT NULL,
        SellerCharName NVARCHAR(64) NOT NULL,
        SellerUniqueID BIGINT NOT NULL,
        StallSlot TINYINT NOT NULL,
        SilkAmount INT NOT NULL,
        [Status] TINYINT NOT NULL CONSTRAINT DF_SilkStallTransactions_Status DEFAULT (0),
        FailureReason NVARCHAR(256) NULL,
        CreatedAt DATETIME2(0) NOT NULL CONSTRAINT DF_SilkStallTransactions_CreatedAt DEFAULT (SYSUTCDATETIME()),
        ReservedAt DATETIME2(0) NULL,
        CompletedAt DATETIME2(0) NULL,
        RefundedAt DATETIME2(0) NULL,
        CONSTRAINT PK_SilkStallTransactions PRIMARY KEY CLUSTERED (ID),
        CONSTRAINT CK_SilkStallTransactions_SilkAmount CHECK (SilkAmount > 0),
        CONSTRAINT CK_SilkStallTransactions_Status CHECK ([Status] IN (0, 1, 2, 3))
    );
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.SilkStallTransactions')
      AND name = N'IX_SilkStallTransactions_Status_CreatedAt'
)
BEGIN
    CREATE NONCLUSTERED INDEX IX_SilkStallTransactions_Status_CreatedAt
        ON dbo.SilkStallTransactions([Status], CreatedAt)
        INCLUDE (BuyerJID, SellerJID, SilkAmount);
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.SilkStallTransactions')
      AND name = N'IX_SilkStallTransactions_BuyerCharID'
)
BEGIN
    CREATE NONCLUSTERED INDEX IX_SilkStallTransactions_BuyerCharID
        ON dbo.SilkStallTransactions(BuyerCharID, CreatedAt DESC);
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.SilkStallTransactions')
      AND name = N'IX_SilkStallTransactions_SellerCharID'
)
BEGIN
    CREATE NONCLUSTERED INDEX IX_SilkStallTransactions_SellerCharID
        ON dbo.SilkStallTransactions(SellerCharID, CreatedAt DESC);
END;

IF OBJECT_ID(N'dbo.__Whitelist', N'U') IS NOT NULL
BEGIN
    EXEC sys.sp_executesql N'
        IF NOT EXISTS
        (
            SELECT 1
            FROM dbo.__Whitelist
            WHERE ServerType = @ServerType
              AND MsgId = @MsgId
        )
        BEGIN
            INSERT INTO dbo.__Whitelist(ServerType, MsgId)
            VALUES (@ServerType, @MsgId);
        END;',
        N'@ServerType INT, @MsgId INT',
        @ServerType = 3,
        @MsgId = 6253;
END;

PRINT 'Silk stall transactions migration completed.';
