USE [KMTGuard];
GO
SET NOCOUNT ON;
GO

IF OBJECT_ID(N'dbo.Command_GameServerQueue', N'U') IS NULL
    THROW 51320, 'dbo.Command_GameServerQueue is required.', 1;
IF OBJECT_ID(N'dbo.Vip_ItemMallPurchaseEvents', N'U') IS NULL
    THROW 51321, 'dbo.Vip_ItemMallPurchaseEvents is required.', 1;
IF OBJECT_ID(N'dbo.Hook_ItemMallBuy', N'P') IS NULL
    THROW 51322, 'dbo.Hook_ItemMallBuy is required.', 1;
GO

CREATE OR ALTER PROCEDURE dbo.Command_ClaimGameServer
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    -- READPAST is valid only under READ COMMITTED or REPEATABLE READ. Set the
    -- procedure boundary explicitly so a pooled/caller session cannot leak a
    -- stronger isolation level into this queue claim.
    SET TRANSACTION ISOLATION LEVEL READ COMMITTED;
    BEGIN TRANSACTION;

    DELETE TOP (1000) dbo.Command_GameServerResult
    WHERE CompletedAtUtc < DATEADD(DAY, -90, SYSUTCDATETIME());

    INSERT dbo.Command_GameServerResult(CommandID,ActionID,Status,ResultCode,Reason,AttemptCount,CompletedAtUtc)
    SELECT q.ID,q.Action_ID,'Indeterminate',1004,N'Claim expired after an uncertain dispatch state.',q.AttemptCount,SYSUTCDATETIME()
    FROM dbo.Command_GameServerQueue q WITH(UPDLOCK,HOLDLOCK)
    WHERE q.ClaimState=1 AND q.ClaimedAtUtc < DATEADD(MINUTE,-2,SYSUTCDATETIME())
      AND NOT EXISTS(SELECT 1 FROM dbo.Command_GameServerResult r WHERE r.CommandID=q.ID);

    DELETE q
    FROM dbo.Command_GameServerQueue q
    WHERE q.ClaimState=1 AND q.ClaimedAtUtc < DATEADD(MINUTE,-2,SYSUTCDATETIME())
      AND EXISTS(SELECT 1 FROM dbo.Command_GameServerResult r WHERE r.CommandID=q.ID);

    ;WITH next_command AS
    (
        SELECT TOP (1) *
        FROM dbo.Command_GameServerQueue WITH(UPDLOCK,READPAST,READCOMMITTEDLOCK,ROWLOCK)
        WHERE ClaimState=0 AND (PlannedTime IS NULL OR PlannedTime<=GETDATE())
        ORDER BY ID
    )
    UPDATE next_command
       SET ClaimState=1,ClaimToken=NEWID(),ClaimedAtUtc=SYSUTCDATETIME(),
           AttemptCount=AttemptCount+1,LastError=NULL
    OUTPUT inserted.ID,inserted.Action_ID,inserted.Data1,inserted.Data2,inserted.Data3,
           inserted.Data4,inserted.Data5,inserted.Data6,inserted.Data7,inserted.Data8,
           inserted.Data9,inserted.Data10,inserted.Data11,inserted.PlannedTime,
           CONVERT(varchar(36),inserted.ClaimToken),inserted.AttemptCount;

    COMMIT TRANSACTION;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Vip_ProcessPendingItemMallPurchases
    @BatchSize INT = 50
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT OFF;

    -- Keep every loop transaction compatible with both READPAST and databases
    -- that have READ_COMMITTED_SNAPSHOT enabled.
    SET TRANSACTION ISOLATION LEVEL READ COMMITTED;

    SET @BatchSize =
        CASE WHEN @BatchSize < 1 THEN 1
             WHEN @BatchSize > 500 THEN 500
             ELSE @BatchSize
        END;

    DECLARE @Processed INT = 0;
    DECLARE @EventID BIGINT;
    DECLARE @JID INT;
    DECLARE @CharID INT;
    DECLARE @CharName16 VARCHAR(16);
    DECLARE @ItemID INT;
    DECLARE @SilkOwn INT;

    WHILE @Processed < @BatchSize
    BEGIN
        SET @EventID = NULL;

        BEGIN TRANSACTION;
        BEGIN TRY
            SELECT TOP (1)
                @EventID = purchase.EventID,
                @JID = purchase.JID,
                @CharID = purchase.CharID,
                @CharName16 = purchase.CharName16,
                @ItemID = purchase.ItemID,
                @SilkOwn = purchase.SilkOwn
            FROM dbo.Vip_ItemMallPurchaseEvents AS purchase
                WITH (UPDLOCK, READPAST, READCOMMITTEDLOCK, ROWLOCK)
            WHERE purchase.Status = 0
              AND purchase.NextAttemptAt <= SYSUTCDATETIME()
            ORDER BY purchase.EventID;

            IF @EventID IS NULL
            BEGIN
                COMMIT TRANSACTION;
                BREAK;
            END;

            IF NULLIF(@CharName16, '') IS NULL
                THROW 51001, 'VIP purchase character name could not be resolved.', 1;

            EXEC dbo.Hook_ItemMallBuy
                @JID = @JID,
                @CharID = @CharID,
                @CharName16 = @CharName16,
                @ItemID = @ItemID,
                @Silk = @SilkOwn;

            UPDATE dbo.Vip_ItemMallPurchaseEvents
            SET Status = 1,
                AttemptCount = AttemptCount + 1,
                ProcessedAt = SYSUTCDATETIME(),
                LastError = NULL
            WHERE EventID = @EventID;

            COMMIT TRANSACTION;
            SET @Processed += 1;
        END TRY
        BEGIN CATCH
            IF XACT_STATE() <> 0
                ROLLBACK TRANSACTION;

            IF @EventID IS NOT NULL
            BEGIN
                UPDATE dbo.Vip_ItemMallPurchaseEvents
                SET Status = CASE WHEN AttemptCount + 1 >= 10 THEN 2 ELSE 0 END,
                    AttemptCount = AttemptCount + 1,
                    NextAttemptAt = DATEADD(SECOND, 15, SYSUTCDATETIME()),
                    LastError = LEFT(ERROR_MESSAGE(), 1000)
                WHERE EventID = @EventID;
            END;

            SET @Processed += 1;
        END CATCH;
    END;
END;
GO
