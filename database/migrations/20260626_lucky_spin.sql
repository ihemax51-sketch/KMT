/*
    Lucky Spin for KMTGuard.

    Run on the KMTGuard/proxy database, then restart KMTGuard.
    Add valid _RefObjCommon item IDs to dbo._LuckySpinRewards before enabling it.
*/

SET NOCOUNT ON;

IF OBJECT_ID(N'dbo._LuckySpinRewards', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._LuckySpinRewards
    (
        ID INT IDENTITY(1,1) NOT NULL,
        ItemID INT NOT NULL,
        Amount INT NOT NULL,
        Rate INT NOT NULL,
        IsActive BIT NOT NULL CONSTRAINT DF_LuckySpinRewards_IsActive DEFAULT (1),
        CreatedAt DATETIME2(0) NOT NULL CONSTRAINT DF_LuckySpinRewards_CreatedAt DEFAULT (SYSUTCDATETIME()),
        UpdatedAt DATETIME2(0) NULL,
        CONSTRAINT PK_LuckySpinRewards PRIMARY KEY CLUSTERED (ID),
        CONSTRAINT CK_LuckySpinRewards_ItemID CHECK (ItemID > 0),
        CONSTRAINT CK_LuckySpinRewards_Amount CHECK (Amount > 0),
        CONSTRAINT CK_LuckySpinRewards_Rate CHECK (Rate > 0)
    );
END;

IF COL_LENGTH(N'dbo._LuckySpinRewards', N'IsActive') IS NULL
BEGIN
    ALTER TABLE dbo._LuckySpinRewards
        ADD IsActive BIT NOT NULL CONSTRAINT DF_LuckySpinRewards_IsActive DEFAULT (1);
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo._LuckySpinRewards')
      AND name = N'IX_LuckySpinRewards_Active'
)
BEGIN
    CREATE NONCLUSTERED INDEX IX_LuckySpinRewards_Active
        ON dbo._LuckySpinRewards(IsActive, ID)
        INCLUDE (ItemID, Amount, Rate);
END;

/*
    The original Lucky Spin source calls this procedure by ItemID. Existing
    KMTGuard installations sometimes expose only _AddItemToChest with the same
    ItemID contract, so keep the source-compatible wrapper here.
*/
EXEC sys.sp_executesql N'
CREATE OR ALTER PROCEDURE dbo._AddItemToChest2
    @CharID INT,
    @ItemID INT,
    @Quantity INT,
    @Type VARCHAR(100),
    @Plus INT
AS
BEGIN
    SET NOCOUNT ON;
    EXEC dbo._AddItemToChest
        @CharID = @CharID,
        @ItemRefObjID = @ItemID,
        @Quantity = @Quantity,
        @From = @Type,
        @Plus = @Plus;
END;';

IF OBJECT_ID(N'dbo._LuckySpinLog', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._LuckySpinLog
    (
        ID BIGINT IDENTITY(1,1) NOT NULL,
        CharID INT NOT NULL,
        JID INT NOT NULL,
        RewardID INT NOT NULL,
        ItemID INT NOT NULL,
        Amount INT NOT NULL,
        Price INT NOT NULL,
        PaymentType NVARCHAR(8) NOT NULL,
        CreatedAt DATETIME2(0) NOT NULL CONSTRAINT DF_LuckySpinLog_CreatedAt DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT PK_LuckySpinLog PRIMARY KEY CLUSTERED (ID),
        CONSTRAINT CK_LuckySpinLog_Amount CHECK (Amount > 0),
        CONSTRAINT CK_LuckySpinLog_Price CHECK (Price > 0),
        CONSTRAINT CK_LuckySpinLog_PaymentType CHECK (PaymentType IN (N'Silk', N'Gold'))
    );
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo._LuckySpinLog')
      AND name = N'IX_LuckySpinLog_CharID_CreatedAt'
)
BEGIN
    CREATE NONCLUSTERED INDEX IX_LuckySpinLog_CharID_CreatedAt
        ON dbo._LuckySpinLog(CharID, CreatedAt DESC)
        INCLUDE (RewardID, ItemID, Amount, Price, PaymentType);
END;

IF OBJECT_ID(N'dbo.__Settings', N'U') IS NOT NULL
BEGIN
    MERGE dbo.__Settings AS target
    USING
    (
        VALUES
            (N'EnableLuckySpin', N'False'),
            (N'EnableLuckySpinSilk', N'True'),
            (N'LuckySpinPrice', N'1')
    ) AS source(SettingName, Value)
        ON target.SettingName = source.SettingName
    WHEN NOT MATCHED THEN
        INSERT (SettingName, Value)
        VALUES (source.SettingName, source.Value);
END;

/*
    Example after finding valid item IDs in your _RefObjCommon:

    INSERT INTO dbo._LuckySpinRewards (ItemID, Amount, Rate)
    VALUES
        (ITEM_ID_HERE, 1, 70),
        (ITEM_ID_HERE, 5, 25),
        (ITEM_ID_HERE, 1, 5);

    When rewards are ready:
    UPDATE dbo.__Settings SET Value = N'True' WHERE SettingName = N'EnableLuckySpin';
*/

PRINT 'Lucky Spin migration completed.';
