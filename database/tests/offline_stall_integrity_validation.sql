SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.System_Settings', N'U') IS NULL
    THROW 51000, 'KMTGuard.dbo.System_Settings was not found.', 1;

IF OBJECT_ID(N'dbo.Stall_Offline', N'U') IS NULL
    THROW 51001, 'KMTGuard.dbo.Stall_Offline was not found.', 1;

IF EXISTS
(
    SELECT required.ColumnName
    FROM (VALUES
        (N'ID'), (N'JID'), (N'AccountName'), (N'CharID'), (N'CharName'),
        (N'UniqueID'), (N'FilterSessionGuid'), (N'Status'), (N'ActivatedAtUtc'),
        (N'DetachedAtUtc'), (N'ExpiresAtUtc'), (N'LastHeartbeatAtUtc'),
        (N'ClosedAtUtc'), (N'CloseReason')
    ) AS required(ColumnName)
    WHERE COL_LENGTH(N'dbo.Stall_Offline', required.ColumnName) IS NULL
)
    THROW 51002, 'dbo.Stall_Offline is missing one or more required columns.', 1;

IF (SELECT COUNT(*) FROM dbo.System_Settings WHERE SettingName = N'EnableOfflineStall') <> 1
    THROW 51003, 'EnableOfflineStall must exist exactly once.', 1;

IF (SELECT COUNT(*) FROM dbo.System_Settings WHERE SettingName = N'OfflineStallMaxHours') <> 1
    THROW 51004, 'OfflineStallMaxHours must exist exactly once.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.System_Settings
    WHERE SettingName = N'OfflineStallMaxHours'
      AND
      (
          TRY_CONVERT(INT, Value) IS NULL OR
          TRY_CONVERT(INT, Value) < 0 OR
          TRY_CONVERT(INT, Value) > 8760
      )
)
    THROW 51005, 'OfflineStallMaxHours is outside the supported range.', 1;

IF EXISTS
(
    SELECT 1
    FROM dbo.System_Settings
    WHERE SettingName = N'OfflineStallConfirmSeconds'
)
    THROW 51006, 'The retired OfflineStallConfirmSeconds setting still exists.', 1;

IF OBJECT_ID(N'dbo.Security_Whitelist', N'U') IS NOT NULL
   AND EXISTS
   (
       SELECT 1
       FROM dbo.Security_Whitelist
       WHERE MsgId = 6355 AND ServerType IN (2, 3)
   )
    THROW 51007, 'The retired Offline Stall decision opcode is still whitelisted.', 1;

SELECT
    (SELECT Value FROM dbo.System_Settings WHERE SettingName = N'EnableOfflineStall') AS EnableOfflineStall,
    (SELECT Value FROM dbo.System_Settings WHERE SettingName = N'OfflineStallMaxHours') AS OfflineStallMaxHours,
    (SELECT COUNT_BIG(*) FROM dbo.Stall_Offline) AS OfflineStallHistoryRows;

