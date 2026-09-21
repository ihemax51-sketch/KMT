/*
    Offline Stall support for KMTGuard.

    Run on the KMTGuard/proxy database, then restart KMTGuard.
*/

SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.__Settings', N'U') IS NULL
BEGIN
    RAISERROR('__Settings table was not found.', 16, 1);
    RETURN;
END;

MERGE dbo.__Settings AS target
USING (VALUES
    (N'EnableOfflineStall', N'True'),
    (N'OfflineStallMaxHours', N'24'),
    (N'OfflineStallConfirmSeconds', N'5')
) AS source(SettingName, Value)
ON target.SettingName = source.SettingName
WHEN NOT MATCHED THEN
    INSERT (SettingName, Value) VALUES (source.SettingName, source.Value);

IF OBJECT_ID(N'dbo.OfflineStalls', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.OfflineStalls
    (
        ID BIGINT IDENTITY(1,1) NOT NULL
            CONSTRAINT PK_OfflineStalls PRIMARY KEY CLUSTERED,
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
        CONSTRAINT CK_OfflineStalls_Status CHECK ([Status] IN (0, 1, 2))
    );
END;

IF NOT EXISTS
(
    SELECT 1 FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.OfflineStalls')
      AND name = N'UX_OfflineStalls_ActiveAccount'
)
BEGIN
    CREATE UNIQUE NONCLUSTERED INDEX UX_OfflineStalls_ActiveAccount
        ON dbo.OfflineStalls(AccountName)
        WHERE [Status] < 2;
END;

IF NOT EXISTS
(
    SELECT 1 FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.OfflineStalls')
      AND name = N'UX_OfflineStalls_ActiveCharID'
)
BEGIN
    CREATE UNIQUE NONCLUSTERED INDEX UX_OfflineStalls_ActiveCharID
        ON dbo.OfflineStalls(CharID)
        WHERE [Status] < 2;
END;

IF OBJECT_ID(N'dbo.__Whitelist', N'U') IS NOT NULL
BEGIN
    -- ServerType 2 = Gateway, 3 = Agent. 0x18D0 = 6352, 0x18D3 = 6355.
    IF NOT EXISTS (SELECT 1 FROM dbo.__Whitelist WHERE ServerType = 3 AND MsgId = 6352)
        INSERT INTO dbo.__Whitelist(ServerType, MsgId) VALUES (3, 6352);
    IF NOT EXISTS (SELECT 1 FROM dbo.__Whitelist WHERE ServerType = 3 AND MsgId = 6355)
        INSERT INTO dbo.__Whitelist(ServerType, MsgId) VALUES (3, 6355);
    IF NOT EXISTS (SELECT 1 FROM dbo.__Whitelist WHERE ServerType = 2 AND MsgId = 6355)
        INSERT INTO dbo.__Whitelist(ServerType, MsgId) VALUES (2, 6355);
END;

PRINT 'Offline Stall migration completed.';
