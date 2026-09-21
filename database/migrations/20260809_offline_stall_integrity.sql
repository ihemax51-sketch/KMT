/*
    Offline Stall runtime/schema integrity update.
    Run on the KMTGuard database while the Filter is stopped.
*/

SET NOCOUNT ON;
SET XACT_ABORT ON;

BEGIN TRANSACTION;

IF OBJECT_ID(N'dbo.System_Settings', N'U') IS NULL
    THROW 51000, 'KMTGuard.dbo.System_Settings was not found.', 1;

MERGE dbo.System_Settings WITH (HOLDLOCK) AS target
USING (VALUES
    (N'EnableOfflineStall', N'True'),
    (N'OfflineStallMaxHours', N'24')
) AS source(SettingName, Value)
ON target.SettingName = source.SettingName
WHEN NOT MATCHED THEN
    INSERT (SettingName, Value) VALUES (source.SettingName, source.Value);

UPDATE dbo.System_Settings
SET Value = N'24'
WHERE SettingName = N'OfflineStallMaxHours'
  AND
  (
      TRY_CONVERT(INT, Value) IS NULL OR
      TRY_CONVERT(INT, Value) < 0 OR
      TRY_CONVERT(INT, Value) > 8760
  );

DELETE dbo.System_Settings
WHERE SettingName = N'OfflineStallConfirmSeconds';

IF OBJECT_ID(N'dbo.Stall_Offline', N'U') IS NULL
   AND OBJECT_ID(N'dbo.OfflineStalls', N'U') IS NOT NULL
BEGIN
    EXEC sys.sp_rename N'dbo.OfflineStalls', N'Stall_Offline';
END;

IF OBJECT_ID(N'dbo.Stall_Offline', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Stall_Offline
    (
        ID BIGINT IDENTITY(1,1) NOT NULL
            CONSTRAINT PK_Stall_Offline PRIMARY KEY CLUSTERED,
        JID INT NOT NULL,
        AccountName VARCHAR(128) NOT NULL,
        CharID INT NOT NULL,
        CharName VARCHAR(64) NOT NULL,
        UniqueID BIGINT NOT NULL,
        FilterSessionGuid UNIQUEIDENTIFIER NOT NULL,
        [Status] TINYINT NOT NULL,
        ActivatedAtUtc DATETIME2(0) NOT NULL,
        DetachedAtUtc DATETIME2(0) NULL,
        ExpiresAtUtc DATETIME2(0) NULL,
        LastHeartbeatAtUtc DATETIME2(0) NULL,
        ClosedAtUtc DATETIME2(0) NULL,
        CloseReason NVARCHAR(256) NULL,
        CONSTRAINT CK_Stall_Offline_Status CHECK ([Status] IN (0, 1, 2))
    );
END;

UPDATE dbo.Stall_Offline
SET [Status] = 2,
    ClosedAtUtc = COALESCE(ClosedAtUtc, SYSUTCDATETIME()),
    CloseReason = COALESCE(CloseReason, N'Offline Stall integrity update')
WHERE [Status] IN (0, 1);

IF OBJECT_ID(N'dbo.OfflineStalls', N'U') IS NOT NULL
   AND OBJECT_ID(N'dbo.Stall_Offline', N'U') IS NOT NULL
BEGIN
    INSERT dbo.Stall_Offline
    (
        JID, AccountName, CharID, CharName, UniqueID, FilterSessionGuid,
        [Status], ActivatedAtUtc, DetachedAtUtc, ExpiresAtUtc,
        LastHeartbeatAtUtc, ClosedAtUtc, CloseReason
    )
    SELECT
        source.JID,
        source.AccountName,
        source.CharID,
        source.CharName,
        source.UniqueID,
        source.FilterSessionGuid,
        2,
        source.ActivatedAtUtc,
        source.DetachedAtUtc,
        source.ExpiresAtUtc,
        source.LastHeartbeatAtUtc,
        COALESCE(source.ClosedAtUtc, SYSUTCDATETIME()),
        COALESCE(source.CloseReason, N'migrated from dbo.OfflineStalls')
    FROM dbo.OfflineStalls AS source
    WHERE NOT EXISTS
    (
        SELECT 1
        FROM dbo.Stall_Offline AS target
        WHERE target.FilterSessionGuid = source.FilterSessionGuid
    );
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Stall_Offline')
      AND name IN (N'UX_Stall_Offline_ActiveAccount', N'UX_OfflineStalls_ActiveAccount')
)
BEGIN
    CREATE UNIQUE NONCLUSTERED INDEX UX_Stall_Offline_ActiveAccount
        ON dbo.Stall_Offline(AccountName)
        WHERE [Status] < 2;
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Stall_Offline')
      AND name IN (N'UX_Stall_Offline_ActiveCharID', N'UX_OfflineStalls_ActiveCharID')
)
BEGIN
    CREATE UNIQUE NONCLUSTERED INDEX UX_Stall_Offline_ActiveCharID
        ON dbo.Stall_Offline(CharID)
        WHERE [Status] < 2;
END;

IF OBJECT_ID(N'dbo.Security_Whitelist', N'U') IS NOT NULL
BEGIN
    IF NOT EXISTS
    (
        SELECT 1
        FROM dbo.Security_Whitelist
        WHERE ServerType = 3 AND MsgId = 6352
    )
        INSERT dbo.Security_Whitelist(ServerType, MsgId) VALUES (3, 6352);

    DELETE dbo.Security_Whitelist
    WHERE MsgId = 6355 AND ServerType IN (2, 3);
END;

IF OBJECT_ID(N'dbo.__Whitelist', N'U') IS NOT NULL
BEGIN
    DELETE dbo.__Whitelist
    WHERE MsgId = 6355 AND ServerType IN (2, 3);
END;

COMMIT TRANSACTION;
