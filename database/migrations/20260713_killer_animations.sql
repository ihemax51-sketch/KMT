/*
    Killer Animations shop for KMTGuard.

    Run on the KMTGuard/proxy database, then restart KMTGuard.
    PaymentType: 0 = Silk, 1 = Gold.
*/

SET NOCOUNT ON;

IF OBJECT_ID(N'dbo._KillerAnimations', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._KillerAnimations
    (
        ID INT IDENTITY(1,1) NOT NULL,
        Service BIT NOT NULL CONSTRAINT DF_KillerAnimations_Service DEFAULT (1),
        SortOrder INT NOT NULL CONSTRAINT DF_KillerAnimations_SortOrder DEFAULT (0),
        CodeName VARCHAR(64) NOT NULL,
        DisplayName NVARCHAR(96) NOT NULL,
        AnimationID INT NOT NULL,
        Price INT NOT NULL CONSTRAINT DF_KillerAnimations_Price DEFAULT (0),
        PaymentType TINYINT NOT NULL CONSTRAINT DF_KillerAnimations_PaymentType DEFAULT (0),
        CreatedAt DATETIME2(0) NOT NULL CONSTRAINT DF_KillerAnimations_CreatedAt DEFAULT (SYSUTCDATETIME()),
        UpdatedAt DATETIME2(0) NULL,
        CONSTRAINT PK_KillerAnimations PRIMARY KEY CLUSTERED (ID),
        CONSTRAINT UQ_KillerAnimations_CodeName UNIQUE (CodeName),
        CONSTRAINT CK_KillerAnimations_AnimationID CHECK (AnimationID > 0 AND AnimationID <= 500),
        CONSTRAINT CK_KillerAnimations_Price CHECK (Price >= 0),
        CONSTRAINT CK_KillerAnimations_PaymentType CHECK (PaymentType IN (0, 1))
    );
END;

IF OBJECT_ID(N'dbo._KillerAnimationOwned', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._KillerAnimationOwned
    (
        CharID INT NOT NULL,
        JID INT NOT NULL,
        AnimationRefID INT NOT NULL,
        PurchasedAt DATETIME2(0) NOT NULL CONSTRAINT DF_KillerAnimationOwned_PurchasedAt DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT PK_KillerAnimationOwned PRIMARY KEY CLUSTERED (CharID, AnimationRefID),
        CONSTRAINT FK_KillerAnimationOwned_Animation
            FOREIGN KEY (AnimationRefID) REFERENCES dbo._KillerAnimations(ID)
    );
END;

IF OBJECT_ID(N'dbo._KillerAnimationActive', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._KillerAnimationActive
    (
        CharID INT NOT NULL,
        AnimationRefID INT NOT NULL,
        UpdatedAt DATETIME2(0) NOT NULL CONSTRAINT DF_KillerAnimationActive_UpdatedAt DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT PK_KillerAnimationActive PRIMARY KEY CLUSTERED (CharID),
        CONSTRAINT FK_KillerAnimationActive_Animation
            FOREIGN KEY (AnimationRefID) REFERENCES dbo._KillerAnimations(ID)
    );
END;

IF OBJECT_ID(N'dbo._KillerAnimationPurchaseLog', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._KillerAnimationPurchaseLog
    (
        ID BIGINT IDENTITY(1,1) NOT NULL,
        CharID INT NOT NULL,
        JID INT NOT NULL,
        AnimationRefID INT NOT NULL,
        Price INT NOT NULL,
        PaymentType NVARCHAR(8) NOT NULL,
        CreatedAt DATETIME2(0) NOT NULL CONSTRAINT DF_KillerAnimationPurchaseLog_CreatedAt DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT PK_KillerAnimationPurchaseLog PRIMARY KEY CLUSTERED (ID),
        CONSTRAINT FK_KillerAnimationPurchaseLog_Animation
            FOREIGN KEY (AnimationRefID) REFERENCES dbo._KillerAnimations(ID),
        CONSTRAINT CK_KillerAnimationPurchaseLog_PaymentType CHECK (PaymentType IN (N'Silk', N'Gold')),
        CONSTRAINT CK_KillerAnimationPurchaseLog_Price CHECK (Price >= 0)
    );
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo._KillerAnimations')
      AND name = N'IX_KillerAnimations_Service_SortOrder'
)
BEGIN
    CREATE NONCLUSTERED INDEX IX_KillerAnimations_Service_SortOrder
        ON dbo._KillerAnimations(Service, SortOrder, ID)
        INCLUDE (DisplayName, AnimationID, Price, PaymentType);
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo._KillerAnimationOwned')
      AND name = N'IX_KillerAnimationOwned_AnimationRefID'
)
BEGIN
    CREATE NONCLUSTERED INDEX IX_KillerAnimationOwned_AnimationRefID
        ON dbo._KillerAnimationOwned(AnimationRefID, CharID);
END;

MERGE dbo._KillerAnimations AS target
USING
(
    VALUES
        -- Verified player motion IDs. IDs 52, 53 and 57 are shared by all
        -- Chinese/European male/female adventurer animation resources.
        ('KILLER_VICTORY_POSE', 1, N'Victory Pose', 31, 100, 0, 10),
        ('KILLER_BATTLE_CRUSH', 1, N'Battle Clash', 54, 150, 0, 20),
        ('KILLER_CHAMPION_SALUTE', 1, N'Champion Salute', 32, 100, 0, 30),
        ('KILLER_BLADE_FLOURISH', 1, N'Blade Flourish', 52, 120, 0, 40),
        ('KILLER_HERO_STANCE', 1, N'Hero Stance', 53, 150, 0, 50),
        ('KILLER_FINAL_SHOW', 1, N'Final Show', 57, 180, 0, 60)
) AS source(CodeName, Service, DisplayName, AnimationID, Price, PaymentType, SortOrder)
    ON target.CodeName = source.CodeName
WHEN MATCHED THEN
    UPDATE SET
        Service = source.Service,
        DisplayName = source.DisplayName,
        AnimationID = source.AnimationID,
        Price = source.Price,
        PaymentType = source.PaymentType,
        SortOrder = source.SortOrder,
        UpdatedAt = SYSUTCDATETIME()
WHEN NOT MATCHED THEN
    INSERT (CodeName, Service, DisplayName, AnimationID, Price, PaymentType, SortOrder)
    VALUES (source.CodeName, source.Service, source.DisplayName, source.AnimationID, source.Price, source.PaymentType, source.SortOrder);

PRINT 'Killer Animations migration completed.';
