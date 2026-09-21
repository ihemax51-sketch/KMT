USE [KMTGuard];
GO

SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.Command_GameServerQueue', N'U') IS NULL
    THROW 51000, 'KMTGuard.dbo.Command_GameServerQueue is required.', 1;
GO

IF DB_ID(N'SRO_VT_SHARD') IS NULL
    THROW 51000, 'SRO_VT_SHARD database is required.', 1;
GO

IF OBJECT_ID(N'SRO_VT_SHARD.dbo._TRAINING_CAMP_UPDATEHONORRANK', N'P') IS NULL
    THROW 51000, 'SRO_VT_SHARD.dbo._TRAINING_CAMP_UPDATEHONORRANK is required.', 1;
GO

IF OBJECT_ID(N'dbo.HonorRankRefreshHistory', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.HonorRankRefreshHistory
    (
        RequestID int NOT NULL
            CONSTRAINT PK_HonorRankRefreshHistory PRIMARY KEY,
        RequestedBy nvarchar(128) NOT NULL,
        RequestedAtUtc datetime2(3) NOT NULL
            CONSTRAINT DF_HonorRankRefreshHistory_RequestedAtUtc DEFAULT (SYSUTCDATETIME()),
        StartedAtUtc datetime2(3) NULL,
        CompletedAtUtc datetime2(3) NULL,
        Status nvarchar(16) NOT NULL,
        ProcedureResult int NULL,
        ErrorMessage nvarchar(512) NULL,
        CONSTRAINT CK_HonorRankRefreshHistory_Status
            CHECK (Status IN (N'Queued', N'Running', N'Succeeded', N'Failed'))
    );

    CREATE INDEX IX_HonorRankRefreshHistory_RequestedAtUtc
        ON dbo.HonorRankRefreshHistory(RequestedAtUtc DESC);
END;
GO

CREATE OR ALTER PROCEDURE dbo.Command_RefreshHonorRank
    @RequestedBy nvarchar(128) = N'Manual'
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    DECLARE @RequestID int;
    DECLARE @Queued bit = 0;

    IF NULLIF(LTRIM(RTRIM(@RequestedBy)), N'') IS NULL
        SET @RequestedBy = N'Manual';

    BEGIN TRANSACTION;

    SELECT TOP (1) @RequestID = q.ID
    FROM dbo.Command_GameServerQueue q WITH (UPDLOCK, HOLDLOCK)
    WHERE q.Action_ID = 200
    ORDER BY q.ID;

    IF @RequestID IS NULL
    BEGIN
        INSERT dbo.Command_GameServerQueue(Action_ID, Data1, PlannedTime)
        VALUES (200, @RequestedBy, GETDATE());

        SET @RequestID = CONVERT(int, SCOPE_IDENTITY());
        SET @Queued = 1;

        INSERT dbo.HonorRankRefreshHistory
            (RequestID, RequestedBy, Status)
        VALUES
            (@RequestID, LEFT(@RequestedBy, 128), N'Queued');
    END
    ELSE IF NOT EXISTS
    (
        SELECT 1
        FROM dbo.HonorRankRefreshHistory WITH (UPDLOCK, HOLDLOCK)
        WHERE RequestID = @RequestID
    )
    BEGIN
        INSERT dbo.HonorRankRefreshHistory
            (RequestID, RequestedBy, Status)
        VALUES
            (@RequestID, LEFT(@RequestedBy, 128), N'Queued');
    END;

    COMMIT TRANSACTION;

    SELECT
        @RequestID AS RequestID,
        @Queued AS Queued,
        CASE WHEN @Queued = 1
             THEN N'Honor Rank refresh queued.'
             ELSE N'An Honor Rank refresh is already queued or running.'
        END AS Message;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Command_GetHonorRankRefreshStatus
    @RequestID int = NULL
AS
BEGIN
    SET NOCOUNT ON;

    SELECT TOP (CASE WHEN @RequestID IS NULL THEN 20 ELSE 1 END)
        RequestID,
        RequestedBy,
        RequestedAtUtc,
        StartedAtUtc,
        CompletedAtUtc,
        Status,
        ProcedureResult,
        ErrorMessage
    FROM dbo.HonorRankRefreshHistory
    WHERE @RequestID IS NULL OR RequestID = @RequestID
    ORDER BY RequestID DESC;
END;
GO

PRINT 'Honor Rank runtime refresh command installed successfully.';
PRINT 'Run: EXEC KMTGuard.dbo.Command_RefreshHonorRank @RequestedBy = N''Admin'';';
PRINT 'Status: EXEC KMTGuard.dbo.Command_GetHonorRankRefreshStatus;';
GO
