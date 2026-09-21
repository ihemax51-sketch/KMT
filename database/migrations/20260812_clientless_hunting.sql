SET NOCOUNT ON;
SET XACT_ABORT ON;

BEGIN TRANSACTION;

IF OBJECT_ID(N'dbo.Clientless_HuntAreas', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Clientless_HuntAreas
    (
        ID INT IDENTITY(1,1) NOT NULL CONSTRAINT PK_Clientless_HuntAreas PRIMARY KEY,
        City VARCHAR(32) NOT NULL,
        SlotNumber TINYINT NOT NULL,
        DisplayName NVARCHAR(64) NOT NULL,
        Enabled BIT NOT NULL CONSTRAINT DF_Clientless_HuntAreas_Enabled DEFAULT (0),
        RegionID INT NOT NULL CONSTRAINT DF_Clientless_HuntAreas_RegionID DEFAULT (0),
        PosX REAL NOT NULL CONSTRAINT DF_Clientless_HuntAreas_PosX DEFAULT (0),
        PosY REAL NOT NULL CONSTRAINT DF_Clientless_HuntAreas_PosY DEFAULT (0),
        PosZ REAL NOT NULL CONSTRAINT DF_Clientless_HuntAreas_PosZ DEFAULT (0),
        Radius REAL NOT NULL CONSTRAINT DF_Clientless_HuntAreas_Radius DEFAULT (50),
        CreatedAt DATETIME2 NOT NULL CONSTRAINT DF_Clientless_HuntAreas_CreatedAt DEFAULT (SYSDATETIME()),
        UpdatedAt DATETIME2 NOT NULL CONSTRAINT DF_Clientless_HuntAreas_UpdatedAt DEFAULT (SYSDATETIME()),
        CONSTRAINT UQ_Clientless_HuntAreas_CitySlot UNIQUE (City, SlotNumber),
        CONSTRAINT CK_Clientless_HuntAreas_SlotNumber CHECK (SlotNumber BETWEEN 1 AND 5),
        CONSTRAINT CK_Clientless_HuntAreas_Radius CHECK (Radius BETWEEN 5 AND 500)
    );
END;

IF OBJECT_ID(N'dbo.Clientless_HuntPolicy', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Clientless_HuntPolicy
    (
        SettingID TINYINT NOT NULL CONSTRAINT PK_Clientless_HuntPolicy PRIMARY KEY,
        Enabled BIT NOT NULL CONSTRAINT DF_Clientless_HuntPolicy_Enabled DEFAULT (0),
        AttackNormal BIT NOT NULL CONSTRAINT DF_Clientless_HuntPolicy_AttackNormal DEFAULT (1),
        AttackUnique BIT NOT NULL CONSTRAINT DF_Clientless_HuntPolicy_AttackUnique DEFAULT (1),
        UniquePriority BIT NOT NULL CONSTRAINT DF_Clientless_HuntPolicy_UniquePriority DEFAULT (1),
        UseSkills BIT NOT NULL CONSTRAINT DF_Clientless_HuntPolicy_UseSkills DEFAULT (1),
        UseBasicAttack BIT NOT NULL CONSTRAINT DF_Clientless_HuntPolicy_UseBasicAttack DEFAULT (1),
        HpPotionPercent TINYINT NOT NULL CONSTRAINT DF_Clientless_HuntPolicy_HpPotionPercent DEFAULT (60),
        MpPotionPercent TINYINT NOT NULL CONSTRAINT DF_Clientless_HuntPolicy_MpPotionPercent DEFAULT (40),
        StuckSeconds SMALLINT NOT NULL CONSTRAINT DF_Clientless_HuntPolicy_StuckSeconds DEFAULT (15),
        TargetTimeoutSeconds SMALLINT NOT NULL CONSTRAINT DF_Clientless_HuntPolicy_TargetTimeoutSeconds DEFAULT (30),
        UpdatedAt DATETIME2 NOT NULL CONSTRAINT DF_Clientless_HuntPolicy_UpdatedAt DEFAULT (SYSDATETIME()),
        CONSTRAINT CK_Clientless_HuntPolicy_SettingID CHECK (SettingID = 1),
        CONSTRAINT CK_Clientless_HuntPolicy_HpPercent CHECK (HpPotionPercent BETWEEN 1 AND 99),
        CONSTRAINT CK_Clientless_HuntPolicy_MpPercent CHECK (MpPotionPercent BETWEEN 1 AND 99),
        CONSTRAINT CK_Clientless_HuntPolicy_StuckSeconds CHECK (StuckSeconds BETWEEN 5 AND 120),
        CONSTRAINT CK_Clientless_HuntPolicy_TargetTimeout CHECK (TargetTimeoutSeconds BETWEEN 5 AND 300)
    );
END;

IF NOT EXISTS (SELECT 1 FROM dbo.Clientless_HuntPolicy WHERE SettingID = 1)
BEGIN
    INSERT dbo.Clientless_HuntPolicy
        (SettingID, Enabled, AttackNormal, AttackUnique, UniquePriority, UseSkills, UseBasicAttack,
         HpPotionPercent, MpPotionPercent, StuckSeconds, TargetTimeoutSeconds)
    VALUES (1, 0, 1, 1, 1, 1, 1, 60, 40, 15, 30);
END;

IF OBJECT_ID(N'dbo.Clientless_Accounts', N'U') IS NOT NULL
BEGIN
    IF COL_LENGTH(N'dbo.Clientless_Accounts', N'HuntEnabled') IS NULL
        ALTER TABLE dbo.Clientless_Accounts ADD HuntEnabled BIT NOT NULL
            CONSTRAINT DF_Clientless_Accounts_HuntEnabled DEFAULT (1);

    IF COL_LENGTH(N'dbo.Clientless_Accounts', N'HuntStatus') IS NULL
        ALTER TABLE dbo.Clientless_Accounts ADD HuntStatus VARCHAR(32) NOT NULL
            CONSTRAINT DF_Clientless_Accounts_HuntStatus DEFAULT ('Stopped');

    IF COL_LENGTH(N'dbo.Clientless_Accounts', N'HuntMessage') IS NULL
        ALTER TABLE dbo.Clientless_Accounts ADD HuntMessage NVARCHAR(256) NULL;

    IF COL_LENGTH(N'dbo.Clientless_Accounts', N'HuntAreaID') IS NULL
        ALTER TABLE dbo.Clientless_Accounts ADD HuntAreaID INT NULL;

    IF COL_LENGTH(N'dbo.Clientless_Accounts', N'CurrentTargetName') IS NULL
        ALTER TABLE dbo.Clientless_Accounts ADD CurrentTargetName NVARCHAR(128) NULL;

    IF COL_LENGTH(N'dbo.Clientless_Accounts', N'LastHuntAt') IS NULL
        ALTER TABLE dbo.Clientless_Accounts ADD LastHuntAt DATETIME2 NULL;

    IF NOT EXISTS
    (
        SELECT 1 FROM sys.indexes
        WHERE object_id = OBJECT_ID(N'dbo.Clientless_Accounts')
          AND name = N'IX_Clientless_Accounts_HuntStatus'
    )
        CREATE INDEX IX_Clientless_Accounts_HuntStatus
            ON dbo.Clientless_Accounts (HuntEnabled, HuntStatus, City, ID);
END;

DECLARE @Cities TABLE (City VARCHAR(32) NOT NULL PRIMARY KEY);
INSERT @Cities (City) VALUES
    ('Jangan'), ('Donwhang'), ('Hotan'), ('SamarKand'), ('Constantinople'), ('Alexandria North (SD)');

;WITH Slots AS
(
    SELECT CONVERT(TINYINT, 1) AS SlotNumber
    UNION ALL SELECT CONVERT(TINYINT, 2)
    UNION ALL SELECT CONVERT(TINYINT, 3)
    UNION ALL SELECT CONVERT(TINYINT, 4)
    UNION ALL SELECT CONVERT(TINYINT, 5)
)
INSERT dbo.Clientless_HuntAreas (City, SlotNumber, DisplayName, Enabled, RegionID, PosX, PosY, PosZ, Radius)
SELECT C.City, S.SlotNumber, CONCAT(N'Area ', S.SlotNumber), 0, 0, 0, 0, 0, 50
FROM @Cities C
CROSS JOIN Slots S
WHERE NOT EXISTS
(
    SELECT 1 FROM dbo.Clientless_HuntAreas A
    WHERE A.City = C.City AND A.SlotNumber = S.SlotNumber
);

COMMIT TRANSACTION;
