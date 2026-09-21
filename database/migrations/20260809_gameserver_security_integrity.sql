/*
    GameServer security bootstrap and authoritative item-lock persistence.
    Run on the KMTGuard/proxy database.
*/

SET NOCOUNT ON;
SET XACT_ABORT ON;

BEGIN TRANSACTION;

IF OBJECT_ID(N'dbo.System_Settings', N'U') IS NULL
    THROW 51000, 'dbo.System_Settings is missing.', 1;

IF NOT EXISTS
(
    SELECT 1 FROM dbo.System_Settings WITH (UPDLOCK, HOLDLOCK)
    WHERE SettingName = N'Security_InternalPacketSharedSecret'
)
BEGIN
    INSERT dbo.System_Settings (SettingName, Value)
    VALUES
    (
        N'Security_InternalPacketSharedSecret',
        CONVERT(VARCHAR(64), CRYPT_GEN_RANDOM(32), 2)
    );
END;

IF OBJECT_ID(N'dbo.Item_Locked', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Item_Locked
    (
        ItemID64 BIGINT NOT NULL,
        [Password] INT NOT NULL CONSTRAINT DF_Item_Locked_Password DEFAULT (0)
    );
END;

IF COL_LENGTH(N'dbo.Item_Locked', N'ItemID64') IS NULL
    THROW 51000, 'dbo.Item_Locked.ItemID64 is missing.', 1;

IF COL_LENGTH(N'dbo.Item_Locked', N'Password') IS NULL
BEGIN
    ALTER TABLE dbo.Item_Locked
        ADD [Password] INT NOT NULL
            CONSTRAINT DF_Item_Locked_Password DEFAULT (0) WITH VALUES;
END;

;WITH DuplicateLocks AS
(
    SELECT ItemID64,
           ROW_NUMBER() OVER (PARTITION BY ItemID64 ORDER BY ItemID64) AS RowNumber
    FROM dbo.Item_Locked
)
DELETE FROM DuplicateLocks WHERE RowNumber > 1;

IF NOT EXISTS
(
    SELECT 1 FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Item_Locked')
      AND name = N'UX_Item_Locked_ItemID64'
)
BEGIN
    CREATE UNIQUE INDEX UX_Item_Locked_ItemID64
        ON dbo.Item_Locked(ItemID64);
END;

COMMIT TRANSACTION;
GO

CREATE OR ALTER PROCEDURE dbo._KmtSetItemLockState
    @ItemID64 BIGINT,
    @Locked BIT
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @ItemID64 <= 0
        THROW 51000, 'ItemID64 must be positive.', 1;

    DECLARE @ResultCode INT = 2; -- 1=changed, 2=already in requested state
    DECLARE @ApplicationLockResult INT;
    DECLARE @ApplicationLockResource NVARCHAR(255) =
        N'KmtItemLock:' + CONVERT(NVARCHAR(32), @ItemID64);

    BEGIN TRANSACTION;

    EXEC @ApplicationLockResult = sys.sp_getapplock
        @Resource = @ApplicationLockResource,
        @LockMode = N'Exclusive',
        @LockOwner = N'Transaction',
        @LockTimeout = 5000;

    IF @ApplicationLockResult < 0
    BEGIN
        ROLLBACK TRANSACTION;
        THROW 51000, 'Unable to serialize the item-lock operation.', 1;
    END;

    IF @Locked = 1
    BEGIN
        IF NOT EXISTS
        (
            SELECT 1 FROM dbo.Item_Locked WITH (UPDLOCK, HOLDLOCK)
            WHERE ItemID64 = @ItemID64
        )
        BEGIN
            INSERT dbo.Item_Locked (ItemID64, [Password]) VALUES (@ItemID64, 0);
            SET @ResultCode = 1;
        END;
    END;
    ELSE
    BEGIN
        DELETE dbo.Item_Locked WHERE ItemID64 = @ItemID64;
        IF @@ROWCOUNT > 0 SET @ResultCode = 1;
    END;

    COMMIT TRANSACTION;
    SELECT @ResultCode AS ResultCode;
END;
GO
