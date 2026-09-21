/*
    Special Offers shop for KMTGuard.

    Run on the KMTGuard/proxy database, then restart KMTGuard.
    Add rows to dbo._SpecialOffers, then enable EnableSpecialOffers in dbo.__Settings.
*/

SET NOCOUNT ON;

IF OBJECT_ID(N'dbo._SpecialOffers', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._SpecialOffers
    (
        ID INT IDENTITY(1,1) NOT NULL,
        Service BIT NOT NULL CONSTRAINT DF_SpecialOffers_Service DEFAULT (1),
        SortOrder INT NOT NULL CONSTRAINT DF_SpecialOffers_SortOrder DEFAULT (0),
        Title NVARCHAR(128) NOT NULL,
        ItemID INT NOT NULL,
        ItemCount INT NOT NULL CONSTRAINT DF_SpecialOffers_ItemCount DEFAULT (1),
        CodeName128 VARCHAR(128) NULL,
        MainPrice INT NOT NULL,
        SalePrice INT NOT NULL,
        PaymentType TINYINT NOT NULL CONSTRAINT DF_SpecialOffers_PaymentType DEFAULT (0),
        PreviewMode TINYINT NOT NULL CONSTRAINT DF_SpecialOffers_PreviewMode DEFAULT (0),
        PreviewRefObjID INT NOT NULL CONSTRAINT DF_SpecialOffers_PreviewRefObjID DEFAULT (0),
        PreviewImagePath VARCHAR(260) NULL,
        StartDate DATETIME2(0) NULL,
        EndDate DATETIME2(0) NULL,
        CreatedAt DATETIME2(0) NOT NULL CONSTRAINT DF_SpecialOffers_CreatedAt DEFAULT (SYSUTCDATETIME()),
        UpdatedAt DATETIME2(0) NULL,
        CONSTRAINT PK_SpecialOffers PRIMARY KEY CLUSTERED (ID),
        CONSTRAINT CK_SpecialOffers_ItemID CHECK (ItemID > 0),
        CONSTRAINT CK_SpecialOffers_ItemCount CHECK (ItemCount > 0),
        CONSTRAINT CK_SpecialOffers_MainPrice CHECK (MainPrice >= 0),
        CONSTRAINT CK_SpecialOffers_SalePrice CHECK (SalePrice > 0),
        CONSTRAINT CK_SpecialOffers_PaymentType CHECK (PaymentType IN (0, 1)),
        CONSTRAINT CK_SpecialOffers_PreviewMode CHECK (PreviewMode BETWEEN 0 AND 3)
    );
END;

IF COL_LENGTH(N'dbo._SpecialOffers', N'CodeName128') IS NULL
BEGIN
    ALTER TABLE dbo._SpecialOffers ADD CodeName128 VARCHAR(128) NULL;
END;

IF COL_LENGTH(N'dbo._SpecialOffers', N'PreviewImagePath') IS NULL
BEGIN
    ALTER TABLE dbo._SpecialOffers ADD PreviewImagePath VARCHAR(260) NULL;
END;

IF COL_LENGTH(N'dbo._SpecialOffers', N'PreviewMode') IS NULL
BEGIN
    ALTER TABLE dbo._SpecialOffers
        ADD PreviewMode TINYINT NOT NULL CONSTRAINT DF_SpecialOffers_PreviewMode DEFAULT (0);
END;

IF COL_LENGTH(N'dbo._SpecialOffers', N'PreviewRefObjID') IS NULL
BEGIN
    ALTER TABLE dbo._SpecialOffers
        ADD PreviewRefObjID INT NOT NULL CONSTRAINT DF_SpecialOffers_PreviewRefObjID DEFAULT (0);
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo._SpecialOffers')
      AND name = N'IX_SpecialOffers_Service_SortOrder'
)
BEGIN
    CREATE NONCLUSTERED INDEX IX_SpecialOffers_Service_SortOrder
        ON dbo._SpecialOffers(Service, SortOrder, ID)
        INCLUDE (Title, ItemID, ItemCount, CodeName128, MainPrice, SalePrice, PaymentType, PreviewMode, PreviewRefObjID, PreviewImagePath, StartDate, EndDate);
END;

IF OBJECT_ID(N'dbo._SpecialOfferLog', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._SpecialOfferLog
    (
        ID BIGINT IDENTITY(1,1) NOT NULL,
        OfferID INT NOT NULL,
        CharID INT NOT NULL,
        JID INT NOT NULL,
        ItemID INT NOT NULL,
        ItemCount INT NOT NULL,
        Price INT NOT NULL,
        PaymentType NVARCHAR(8) NOT NULL,
        CreatedAt DATETIME2(0) NOT NULL CONSTRAINT DF_SpecialOfferLog_CreatedAt DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT PK_SpecialOfferLog PRIMARY KEY CLUSTERED (ID),
        CONSTRAINT CK_SpecialOfferLog_ItemCount CHECK (ItemCount > 0),
        CONSTRAINT CK_SpecialOfferLog_Price CHECK (Price > 0),
        CONSTRAINT CK_SpecialOfferLog_PaymentType CHECK (PaymentType IN (N'Silk', N'Gold'))
    );
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo._SpecialOfferLog')
      AND name = N'IX_SpecialOfferLog_CharID_CreatedAt'
)
BEGIN
    CREATE NONCLUSTERED INDEX IX_SpecialOfferLog_CharID_CreatedAt
        ON dbo._SpecialOfferLog(CharID, CreatedAt DESC)
        INCLUDE (OfferID, ItemID, ItemCount, Price, PaymentType);
END;

IF OBJECT_ID(N'dbo.__Settings', N'U') IS NOT NULL
BEGIN
    MERGE dbo.__Settings AS target
    USING
    (
        VALUES
            (N'EnableSpecialOffers', N'False'),
            (N'ShowGuideSpecialOffers', N'True')
    ) AS source(SettingName, Value)
        ON target.SettingName = source.SettingName
    WHEN NOT MATCHED THEN
        INSERT (SettingName, Value)
        VALUES (source.SettingName, source.Value);
END;

/*
    PaymentType: 0 = Silk, 1 = Gold
    PreviewImagePath is optional. Example path after importing the DDJ to Media.pk2:
        clientlibrary\specialoffers\rengar.ddj

    INSERT INTO dbo._SpecialOffers
        (Title, ItemID, ItemCount, CodeName128, MainPrice, SalePrice, PaymentType, PreviewImagePath, SortOrder)
    VALUES
        (N'Rengar Summon Scroll', 12345, 1, 'ITEM_CODE_NAME_HERE', 10000, 500, 0, 'clientlibrary\specialoffers\rengar.ddj', 10),
        (N'Cyber Hound Summon Scroll', 12346, 1, 'ITEM_CODE_NAME_HERE', 10000, 750, 0, 'clientlibrary\specialoffers\cyber_hound.ddj', 20);

    UPDATE dbo.__Settings SET Value = N'True' WHERE SettingName = N'EnableSpecialOffers';
*/

PRINT 'Special Offers migration completed.';
