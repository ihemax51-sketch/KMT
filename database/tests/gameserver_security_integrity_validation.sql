SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.Item_Locked', N'U') IS NULL
    THROW 51000, 'dbo.Item_Locked is missing.', 1;

IF OBJECT_ID(N'dbo._KmtSetItemLockState', N'P') IS NULL
    THROW 51000, 'dbo._KmtSetItemLockState is missing.', 1;

IF NOT EXISTS
(
    SELECT 1 FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Item_Locked')
      AND name = N'UX_Item_Locked_ItemID64'
      AND is_unique = 1
)
    THROW 51000, 'Item-lock uniqueness is not enforced.', 1;

DECLARE @Secret VARCHAR(256) =
(
    SELECT TOP (1) Value FROM dbo.System_Settings
    WHERE SettingName = N'Security_InternalPacketSharedSecret'
);

IF @Secret IS NULL OR LEN(@Secret) <> 64 OR @Secret LIKE '%[^0-9A-F]%'
    THROW 51000, 'The internal packet shared secret is missing or invalid.', 1;

BEGIN TRANSACTION;

DECLARE @TestItemID BIGINT = 9223372036854775000;
DECLARE @Result TABLE (ResultCode INT NOT NULL);

DELETE dbo.Item_Locked WHERE ItemID64 = @TestItemID;
INSERT @Result EXEC dbo._KmtSetItemLockState @TestItemID, 1;
IF NOT EXISTS (SELECT 1 FROM @Result WHERE ResultCode = 1)
    THROW 51000, 'Initial item lock did not report changed.', 1;

DELETE FROM @Result;
INSERT @Result EXEC dbo._KmtSetItemLockState @TestItemID, 1;
IF NOT EXISTS (SELECT 1 FROM @Result WHERE ResultCode = 2)
    THROW 51000, 'Repeated item lock was not idempotent.', 1;

DELETE FROM @Result;
INSERT @Result EXEC dbo._KmtSetItemLockState @TestItemID, 0;
IF NOT EXISTS (SELECT 1 FROM @Result WHERE ResultCode = 1)
    THROW 51000, 'Item unlock did not report changed.', 1;

ROLLBACK TRANSACTION;

SELECT N'GameServer security integrity validation passed.' AS Result;
