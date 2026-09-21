/*
    KMTGuard GameServer command bridge deadlock readiness check
    -----------------------------------------------------------------------
    Purpose:
      Read-only diagnostics for ShardManager red log:
        ODBC [40001] ... deadlocked ... native=1205
        GameServer command database operation failed: operation=claim command=0 action=0

    Safety:
      This script is read-only. It does not ALTER, UPDATE, INSERT, DELETE, or execute
      Command_ClaimGameServer. It only checks metadata and queue status.

    Expected database:
      KMTGuard
*/

SET NOCOUNT ON;

DECLARE @TargetDatabase sysname = N'KMTGuard';

IF DB_ID(@TargetDatabase) IS NULL
BEGIN
    SELECT
        N'FAIL' AS [Status],
        N'DatabaseExists' AS [CheckName],
        N'Database [KMTGuard] was not found on this SQL Server instance.' AS [Details];
    RETURN;
END;

EXEC (N'USE [KMTGuard];

SET NOCOUNT ON;

DECLARE @ClaimProcObjectId int = OBJECT_ID(N''dbo.Command_ClaimGameServer'', N''P'');
DECLARE @QueueObjectId int = OBJECT_ID(N''dbo.Command_GameServerQueue'', N''U'');
DECLARE @ClaimDefinition nvarchar(max) = CASE
    WHEN @ClaimProcObjectId IS NULL THEN NULL
    ELSE OBJECT_DEFINITION(@ClaimProcObjectId)
END;

SELECT
    CASE WHEN DB_NAME() = N''KMTGuard'' THEN N''PASS'' ELSE N''FAIL'' END AS [Status],
    N''CurrentDatabase'' AS [CheckName],
    DB_NAME() AS [Details];

SELECT
    CASE WHEN @QueueObjectId IS NOT NULL THEN N''PASS'' ELSE N''FAIL'' END AS [Status],
    N''Command_GameServerQueue table'' AS [CheckName],
    CASE WHEN @QueueObjectId IS NOT NULL
        THEN N''Found dbo.Command_GameServerQueue.''
        ELSE N''Missing dbo.Command_GameServerQueue.''
    END AS [Details];

SELECT
    CASE WHEN @ClaimProcObjectId IS NOT NULL THEN N''PASS'' ELSE N''FAIL'' END AS [Status],
    N''Command_ClaimGameServer procedure'' AS [CheckName],
    CASE WHEN @ClaimProcObjectId IS NOT NULL
        THEN N''Found dbo.Command_ClaimGameServer.''
        ELSE N''Missing dbo.Command_ClaimGameServer.''
    END AS [Details];

SELECT
    CASE
        WHEN DATABASEPROPERTYEX(DB_NAME(), ''IsReadCommittedSnapshotOn'') = 1 THEN N''INFO''
        ELSE N''INFO''
    END AS [Status],
    N''READ_COMMITTED_SNAPSHOT'' AS [CheckName],
    CONCAT(N''IsReadCommittedSnapshotOn='', CONVERT(int, DATABASEPROPERTYEX(DB_NAME(), ''IsReadCommittedSnapshotOn''))) AS [Details];

SELECT
    CASE
        WHEN @ClaimDefinition IS NULL THEN N''FAIL''
        WHEN @ClaimDefinition LIKE N''%READPAST%'' THEN N''PASS''
        ELSE N''WARN''
    END AS [Status],
    N''Claim uses READPAST'' AS [CheckName],
    CASE
        WHEN @ClaimDefinition IS NULL THEN N''Cannot inspect procedure; it is missing.''
        WHEN @ClaimDefinition LIKE N''%READPAST%'' THEN N''Claim procedure contains READPAST.''
        ELSE N''Claim procedure does not contain READPAST; locked rows may block polling.''
    END AS [Details];

SELECT
    CASE
        WHEN @ClaimDefinition IS NULL THEN N''FAIL''
        WHEN @ClaimDefinition LIKE N''%READCOMMITTEDLOCK%'' THEN N''PASS''
        WHEN DATABASEPROPERTYEX(DB_NAME(), ''IsReadCommittedSnapshotOn'') = 1 THEN N''WARN''
        ELSE N''INFO''
    END AS [Status],
    N''Claim RCSI compatibility'' AS [CheckName],
    CASE
        WHEN @ClaimDefinition IS NULL THEN N''Cannot inspect procedure; it is missing.''
        WHEN @ClaimDefinition LIKE N''%READCOMMITTEDLOCK%'' THEN N''Claim procedure contains READCOMMITTEDLOCK.''
        WHEN DATABASEPROPERTYEX(DB_NAME(), ''IsReadCommittedSnapshotOn'') = 1 THEN N''RCSI is ON but claim procedure lacks READCOMMITTEDLOCK; apply 20260810_readpast_rcsi_compatibility.sql.''
        ELSE N''RCSI is OFF and procedure lacks READCOMMITTEDLOCK; lower risk, but latest procedure is still recommended.''
    END AS [Details];

SELECT
    CASE
        WHEN @ClaimDefinition IS NULL THEN N''FAIL''
        WHEN @ClaimDefinition LIKE N''%UPDLOCK%'' AND @ClaimDefinition LIKE N''%ROWLOCK%'' THEN N''PASS''
        ELSE N''WARN''
    END AS [Status],
    N''Claim lock hints'' AS [CheckName],
    CASE
        WHEN @ClaimDefinition IS NULL THEN N''Cannot inspect procedure; it is missing.''
        WHEN @ClaimDefinition LIKE N''%UPDLOCK%'' AND @ClaimDefinition LIKE N''%ROWLOCK%'' THEN N''Claim procedure contains UPDLOCK and ROWLOCK.''
        ELSE N''Claim procedure is missing one or more expected lock hints.''
    END AS [Details];

;WITH RequiredColumns AS
(
    SELECT N''AttemptCount'' AS ColumnName UNION ALL
    SELECT N''ClaimToken'' UNION ALL
    SELECT N''ClaimedAtUtc'' UNION ALL
    SELECT N''ClaimState'' UNION ALL
    SELECT N''LastError''
)
SELECT
    CASE WHEN COUNT(c.name) = 5 THEN N''PASS'' ELSE N''FAIL'' END AS [Status],
    N''Queue runtime columns'' AS [CheckName],
    CASE WHEN COUNT(c.name) = 5
        THEN N''All runtime claim columns exist.''
        ELSE CONCAT(N''Missing: '',
            STUFF((
                SELECT N'', '' + rc2.ColumnName
                FROM RequiredColumns rc2
                LEFT JOIN sys.columns c2
                    ON c2.object_id = @QueueObjectId
                   AND c2.name = rc2.ColumnName
                WHERE c2.name IS NULL
                FOR XML PATH(N''''), TYPE
            ).value(N''.'', N''nvarchar(max)''), 1, 2, N'''')
        )
    END AS [Details]
FROM RequiredColumns rc
LEFT JOIN sys.columns c
    ON c.object_id = @QueueObjectId
   AND c.name = rc.ColumnName;

SELECT
    CASE WHEN EXISTS
    (
        SELECT 1
        FROM sys.indexes
        WHERE object_id = @QueueObjectId
          AND name = N''IX_Command_GameServerQueue_Claim''
    )
    THEN N''PASS'' ELSE N''WARN'' END AS [Status],
    N''IX_Command_GameServerQueue_Claim index'' AS [CheckName],
    CASE WHEN EXISTS
    (
        SELECT 1
        FROM sys.indexes
        WHERE object_id = @QueueObjectId
          AND name = N''IX_Command_GameServerQueue_Claim''
    )
    THEN N''Found claim index.''
    ELSE N''Missing claim index; claim polling may lock/scan more rows and increase deadlock risk.''
    END AS [Details];

IF @QueueObjectId IS NOT NULL
BEGIN
    SELECT
        N''INFO'' AS [Status],
        N''Queue rows by ClaimState'' AS [CheckName],
        CONCAT(N''ClaimState='', CONVERT(varchar(20), ClaimState), N'', Rows='', CONVERT(varchar(20), COUNT_BIG(*))) AS [Details]
    FROM dbo.Command_GameServerQueue WITH (NOLOCK)
    GROUP BY ClaimState
    ORDER BY ClaimState;

    SELECT
        N''INFO'' AS [Status],
        N''Pending due commands'' AS [CheckName],
        CONCAT(N''Rows='', CONVERT(varchar(20), COUNT_BIG(*))) AS [Details]
    FROM dbo.Command_GameServerQueue WITH (NOLOCK)
    WHERE ClaimState = 0
      AND (PlannedTime IS NULL OR PlannedTime <= GETDATE());

    SELECT
        CASE WHEN COUNT_BIG(*) = 0 THEN N''PASS'' ELSE N''WARN'' END AS [Status],
        N''Expired claimed commands older than 2 minutes'' AS [CheckName],
        CONCAT(N''Rows='', CONVERT(varchar(20), COUNT_BIG(*))) AS [Details]
    FROM dbo.Command_GameServerQueue WITH (NOLOCK)
    WHERE ClaimState = 1
      AND ClaimedAtUtc < DATEADD(MINUTE, -2, SYSUTCDATETIME());
END;

BEGIN TRY
    SELECT TOP (20)
        N''INFO'' AS [Status],
        N''Current locks on Command_GameServerQueue'' AS [CheckName],
        CONCAT(
            N''session_id='', CONVERT(varchar(20), l.request_session_id),
            N'', mode='', l.request_mode,
            N'', status='', l.request_status,
            N'', resource='', l.resource_type
        ) AS [Details]
    FROM sys.dm_tran_locks l
    WHERE l.resource_database_id = DB_ID()
      AND
      (
          l.resource_associated_entity_id = @QueueObjectId
          OR l.resource_associated_entity_id IN
          (
              SELECT p.hobt_id
              FROM sys.partitions p
              WHERE p.object_id = @QueueObjectId
          )
      )
    ORDER BY l.request_session_id, l.request_mode;
END TRY
BEGIN CATCH
    SELECT
        N''INFO'' AS [Status],
        N''Current locks on Command_GameServerQueue'' AS [CheckName],
        CONCAT(N''Could not read lock DMV: '', ERROR_MESSAGE()) AS [Details];
END CATCH;

SELECT
    CASE
        WHEN @QueueObjectId IS NULL OR @ClaimProcObjectId IS NULL THEN N''FAIL''
        WHEN @ClaimDefinition NOT LIKE N''%READPAST%'' THEN N''WARN''
        WHEN DATABASEPROPERTYEX(DB_NAME(), ''IsReadCommittedSnapshotOn'') = 1
             AND @ClaimDefinition NOT LIKE N''%READCOMMITTEDLOCK%'' THEN N''WARN''
        WHEN NOT EXISTS
        (
            SELECT 1
            FROM sys.indexes
            WHERE object_id = @QueueObjectId
              AND name = N''IX_Command_GameServerQueue_Claim''
        ) THEN N''WARN''
        ELSE N''PASS''
    END AS [Status],
    N''Overall'' AS [CheckName],
    CASE
        WHEN @QueueObjectId IS NULL OR @ClaimProcObjectId IS NULL THEN N''Required GameServer command queue objects are missing.''
        WHEN @ClaimDefinition NOT LIKE N''%READPAST%'' THEN N''Claim procedure is old or incomplete. Apply latest GameServer command SQL updates.''
        WHEN DATABASEPROPERTYEX(DB_NAME(), ''IsReadCommittedSnapshotOn'') = 1
             AND @ClaimDefinition NOT LIKE N''%READCOMMITTEDLOCK%'' THEN N''Likely missing 20260810_readpast_rcsi_compatibility.sql.''
        WHEN NOT EXISTS
        (
            SELECT 1
            FROM sys.indexes
            WHERE object_id = @QueueObjectId
              AND name = N''IX_Command_GameServerQueue_Claim''
        ) THEN N''Claim index is missing. Apply 20260810_gameserver_runtime_integrity.sql.''
        ELSE N''Command bridge SQL objects look ready. If deadlocks continue, capture SQL deadlock graph and check for duplicate ShardManager/bridge instances.''
    END AS [Details];
');
