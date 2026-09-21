/*
    KMTGuard Item Chest integrity rebuild.

    Public reward procedures:
      dbo.Item_ChestSendToOnline - sends to the game-ready players currently
                                   connected through the Agent filter.
      dbo.Item_ChestSendToAll    - sends to every non-deleted character,
                                   whether online or offline.

    Internal procedures:
      dbo.Item_AddChest  - the single-character primitive (also used by the filter).
      dbo.Item_AddChestByCodeName - the mandatory CodeName entry point.
      dbo.Item_ChestProcessClaim - the complete claim state machine.

    Command_FilterQueue CommandID 42 is consumed by the Agent filter:
      Data1 = ItemRefObjID
      Data2 = Quantity
      Data3 = Source
      Data4 = Plus
      Data5 = Broadcast BatchID
*/

SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.Item_ChestClaim', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Item_ChestClaim
    (
        ChestID        INT              NOT NULL,
        CharID         INT              NOT NULL,
        ClaimToken     UNIQUEIDENTIFIER NOT NULL,
        State          TINYINT          NOT NULL,
        StartedAtUtc   DATETIME2(3)     NOT NULL
            CONSTRAINT DF_Item_ChestClaim_StartedAtUtc DEFAULT SYSUTCDATETIME(),
        CompletedAtUtc DATETIME2(3)     NULL,
        LastError      NVARCHAR(1000)   NULL,
        CONSTRAINT PK_Item_ChestClaim PRIMARY KEY CLUSTERED (ChestID),
        CONSTRAINT UQ_Item_ChestClaim_Token UNIQUE (ClaimToken),
        CONSTRAINT CK_Item_ChestClaim_State CHECK (State IN (0, 1))
    );

    CREATE INDEX IX_Item_ChestClaim_CharID_State
        ON dbo.Item_ChestClaim(CharID, State, ChestID);
END;
GO

IF OBJECT_ID(N'dbo.Item_ChestClaimResolutionLog', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Item_ChestClaimResolutionLog
    (
        ID           BIGINT IDENTITY(1, 1) NOT NULL,
        ChestID      INT                   NOT NULL,
        CharID       INT                   NOT NULL,
        ClaimToken   UNIQUEIDENTIFIER      NOT NULL,
        Action       VARCHAR(20)           NOT NULL,
        Reason       NVARCHAR(1000)        NOT NULL,
        CreatedAtUtc DATETIME2(3)          NOT NULL
            CONSTRAINT DF_Item_ChestClaimResolutionLog_CreatedAtUtc DEFAULT SYSUTCDATETIME(),
        CONSTRAINT PK_Item_ChestClaimResolutionLog PRIMARY KEY CLUSTERED (ID)
    );

    CREATE INDEX IX_Item_ChestClaimResolutionLog_ChestID
        ON dbo.Item_ChestClaimResolutionLog(ChestID, CreatedAtUtc);
END;
GO

IF OBJECT_ID(N'dbo.Item_ChestBroadcast', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Item_ChestBroadcast
    (
        BatchID        UNIQUEIDENTIFIER NOT NULL,
        Audience       VARCHAR(10)      NOT NULL,
        CommandQueueID INT              NULL,
        ItemRefObjID   INT              NOT NULL,
        Quantity       INT              NOT NULL,
        Source         VARCHAR(100)     NOT NULL,
        Plus           TINYINT          NOT NULL,
        CreatedAtUtc   DATETIME2(3)     NOT NULL
            CONSTRAINT DF_Item_ChestBroadcast_CreatedAtUtc DEFAULT SYSUTCDATETIME(),
        CONSTRAINT PK_Item_ChestBroadcast PRIMARY KEY CLUSTERED (BatchID),
        CONSTRAINT CK_Item_ChestBroadcast_Audience CHECK (Audience IN ('ONLINE', 'ALL'))
    );
END
ELSE IF COL_LENGTH(N'dbo.Item_ChestBroadcast', N'Audience') IS NULL
BEGIN
    ALTER TABLE dbo.Item_ChestBroadcast
        ADD Audience VARCHAR(10) NOT NULL
            CONSTRAINT DF_Item_ChestBroadcast_Audience DEFAULT ('ONLINE') WITH VALUES;

    ALTER TABLE dbo.Item_ChestBroadcast
        ADD CONSTRAINT CK_Item_ChestBroadcast_Audience
            CHECK (Audience IN ('ONLINE', 'ALL'));
END;
GO

IF OBJECT_ID(N'dbo.Item_ChestBroadcastRecipient', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Item_ChestBroadcastRecipient
    (
        BatchID      UNIQUEIDENTIFIER NOT NULL,
        CharID       INT              NOT NULL,
        ChestID      INT              NOT NULL,
        CreatedAtUtc DATETIME2(3)     NOT NULL
            CONSTRAINT DF_Item_ChestBroadcastRecipient_CreatedAtUtc DEFAULT SYSUTCDATETIME(),
        CONSTRAINT PK_Item_ChestBroadcastRecipient PRIMARY KEY CLUSTERED (BatchID, CharID),
        CONSTRAINT FK_Item_ChestBroadcastRecipient_Batch
            FOREIGN KEY (BatchID) REFERENCES dbo.Item_ChestBroadcast(BatchID)
    );

    CREATE INDEX IX_Item_ChestBroadcastRecipient_CharID
        ON dbo.Item_ChestBroadcastRecipient(CharID, BatchID);
END;
GO

CREATE OR ALTER PROCEDURE dbo.Item_AddChest
    @CharID       INT,
    @ItemRefObjID INT,
    @Quantity     INT,
    @From         VARCHAR(100),
    @Plus         INT,
    @BatchID      UNIQUEIDENTIFIER = NULL
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @CharID <= 0
        THROW 51000, 'CharID must be greater than zero.', 1;
    IF NOT EXISTS
    (
        SELECT 1
        FROM SRO_VT_SHARD.dbo._Char WITH (NOLOCK)
        WHERE CharID = @CharID
          AND Deleted = 0
    )
        THROW 51007, 'CharID was not found in the shard database.', 1;
    IF @ItemRefObjID <= 0
        THROW 51001, 'ItemRefObjID must be greater than zero.', 1;
    IF @Quantity <= 0 OR @Quantity > 1000000
        THROW 51002, 'Quantity must be between 1 and 1000000.', 1;
    IF @Plus < 0 OR @Plus > 255
        THROW 51003, 'Plus must be between 0 and 255.', 1;
    IF NULLIF(LTRIM(RTRIM(@From)), '') IS NULL
        SET @From = 'Reward';

    DECLARE
        @ItemCodeName  VARCHAR(128),
        @TypeID1       TINYINT,
        @NewID         INT,
        @FormattedDate VARCHAR(10);

    SELECT
        @ItemCodeName = ROC.CodeName128,
        @TypeID1 = ROC.TypeID1
    FROM SRO_VT_SHARD.dbo._RefObjCommon AS ROC WITH (NOLOCK)
    WHERE ROC.ID = @ItemRefObjID
      AND ROC.Service = 1;

    IF @ItemCodeName IS NULL
        THROW 51004, 'ItemRefObjID was not found or is not in service.', 1;
    IF @TypeID1 <> 3
        THROW 51005, 'ItemRefObjID does not reference an item.', 1;
    IF DATALENGTH(@ItemCodeName) > 120
        THROW 51008, 'ItemCodeName exceeds the Item_Chest column limit.', 1;

    SET @From = LEFT(@From, 100);
    SET @FormattedDate = CONVERT(VARCHAR(10), GETDATE(), 103);

    BEGIN TRANSACTION;

    IF @BatchID IS NOT NULL
    BEGIN
        IF NOT EXISTS
        (
            SELECT 1
            FROM dbo.Item_ChestBroadcast AS B WITH (UPDLOCK, HOLDLOCK)
            WHERE B.BatchID = @BatchID
              AND B.ItemRefObjID = @ItemRefObjID
              AND B.Quantity = @Quantity
              AND B.Source = @From
              AND B.Plus = CONVERT(TINYINT, @Plus)
        )
            THROW 51024, 'The Item Chest broadcast batch is missing or does not match the reward.', 1;

        SELECT @NewID = R.ChestID
        FROM dbo.Item_ChestBroadcastRecipient AS R WITH (UPDLOCK, HOLDLOCK)
        WHERE R.BatchID = @BatchID
          AND R.CharID = @CharID;

        IF @NewID IS NOT NULL
        BEGIN
            COMMIT TRANSACTION;
            SELECT @NewID AS ChestID;
            RETURN;
        END;
    END;

    INSERT dbo.Item_Chest
        (CharID, ItemCodeName, ItemID, Quantity, [Date], [Type], Plus)
    VALUES
        (@CharID, @ItemCodeName, @ItemRefObjID, @Quantity,
         @FormattedDate, @From, CONVERT(TINYINT, @Plus));

    SET @NewID = CONVERT(INT, SCOPE_IDENTITY());

    IF @BatchID IS NOT NULL
    BEGIN
        INSERT dbo.Item_ChestBroadcastRecipient(BatchID, CharID, ChestID)
        VALUES(@BatchID, @CharID, @NewID);
    END;

    INSERT dbo.Command_FilterQueue
        (CommandID, Data1, Data2, Data3, Data4, Data5, Data6, Data7, Data8, Status)
    VALUES
        (17, CONVERT(VARCHAR(20), @NewID), CONVERT(VARCHAR(20), @CharID),
         @ItemCodeName, CONVERT(VARCHAR(20), @ItemRefObjID),
         CONVERT(VARCHAR(20), @Quantity), @FormattedDate, @From,
         CONVERT(VARCHAR(3), @Plus), 1);

    COMMIT TRANSACTION;

    SELECT @NewID AS ChestID;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Item_AddChestByCodeName
    @CharID       INT,
    @ItemCodeName VARCHAR(128),
    @Quantity     INT,
    @From         VARCHAR(100),
    @Plus         INT,
    @BatchID      UNIQUEIDENTIFIER = NULL
AS
BEGIN
    SET NOCOUNT ON;

    DECLARE @ItemRefObjID INT;

    SELECT @ItemRefObjID = ROC.ID
    FROM SRO_VT_SHARD.dbo._RefObjCommon AS ROC WITH (NOLOCK)
    WHERE ROC.CodeName128 = @ItemCodeName
      AND ROC.Service = 1
      AND ROC.TypeID1 = 3;

    IF @ItemRefObjID IS NULL
        THROW 51006, 'ItemCodeName was not found or is not an active item.', 1;

    EXEC dbo.Item_AddChest
        @CharID = @CharID,
        @ItemRefObjID = @ItemRefObjID,
        @Quantity = @Quantity,
        @From = @From,
        @Plus = @Plus,
        @BatchID = @BatchID;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Item_ChestProcessClaim
    @Action     VARCHAR(10),
    @ChestID    INT,
    @CharID     INT = NULL,
    @ClaimToken UNIQUEIDENTIFIER = NULL,
    @Reason     NVARCHAR(1000) = NULL
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    SET @Action = UPPER(LTRIM(RTRIM(@Action)));

    IF @Action NOT IN ('BEGIN', 'SUCCESS', 'FAIL', 'RETRY', 'DELIVERED')
        THROW 51010, 'Action must be BEGIN, SUCCESS, FAIL, RETRY, or DELIVERED.', 1;
    IF @ChestID <= 0
        THROW 51011, 'A valid ChestID is required.', 1;

    IF @Action IN ('BEGIN', 'SUCCESS', 'FAIL')
       AND
       (
           ISNULL(@CharID, 0) <= 0
           OR @ClaimToken IS NULL
           OR @ClaimToken = '00000000-0000-0000-0000-000000000000'
       )
        THROW 51012, 'A valid CharID and non-empty ClaimToken are required.', 1;

    IF @Action IN ('RETRY', 'DELIVERED')
       AND NULLIF(LTRIM(RTRIM(@Reason)), N'') IS NULL
        THROW 51013, 'A resolution reason is required.', 1;

    BEGIN TRANSACTION;

    IF @Action = 'BEGIN'
    BEGIN
        IF EXISTS
        (
            SELECT 1
            FROM dbo.Item_ChestClaim WITH (UPDLOCK, HOLDLOCK)
            WHERE ChestID = @ChestID
        )
        OR NOT EXISTS
        (
            SELECT 1
            FROM dbo.Item_Chest WITH (UPDLOCK, HOLDLOCK)
            WHERE ID = @ChestID
              AND CharID = @CharID
        )
        BEGIN
            COMMIT TRANSACTION;
            RETURN;
        END;

        INSERT dbo.Item_ChestClaim(ChestID, CharID, ClaimToken, State)
        VALUES(@ChestID, @CharID, @ClaimToken, 0);

        SELECT
            C.ID,
            C.CharID,
            C.ItemCodeName,
            C.ItemID,
            C.Quantity,
            C.[Date],
            C.[Type],
            CONVERT(TINYINT, C.Plus) AS Plus
        FROM dbo.Item_Chest AS C
        WHERE C.ID = @ChestID
          AND C.CharID = @CharID;
    END
    ELSE IF @Action = 'SUCCESS'
    BEGIN
        UPDATE dbo.Item_ChestClaim WITH (UPDLOCK, HOLDLOCK)
        SET State = 1,
            CompletedAtUtc = SYSUTCDATETIME(),
            LastError = NULL
        WHERE ChestID = @ChestID
          AND CharID = @CharID
          AND ClaimToken = @ClaimToken
          AND State = 0;

        IF @@ROWCOUNT <> 1
            THROW 51014, 'The pending claim is missing or belongs to another request.', 1;

        DELETE dbo.Item_Chest
        WHERE ID = @ChestID
          AND CharID = @CharID;

        IF @@ROWCOUNT <> 1
            THROW 51015, 'The claimed chest row no longer exists.', 1;
    END
    ELSE IF @Action = 'FAIL'
    BEGIN
        INSERT dbo.Item_ChestClaimResolutionLog
            (ChestID, CharID, ClaimToken, Action, Reason)
        SELECT ChestID, CharID, ClaimToken, 'GAME_FAILED',
               LEFT(COALESCE(@Reason, N'GameServer rejected AddItem.'), 1000)
        FROM dbo.Item_ChestClaim WITH (UPDLOCK, HOLDLOCK)
        WHERE ChestID = @ChestID
          AND CharID = @CharID
          AND ClaimToken = @ClaimToken
          AND State = 0;

        DELETE dbo.Item_ChestClaim
        WHERE ChestID = @ChestID
          AND CharID = @CharID
          AND ClaimToken = @ClaimToken
          AND State = 0;

        IF @@ROWCOUNT <> 1
            THROW 51016, 'The pending claim could not be cancelled.', 1;
    END
    ELSE
    BEGIN
        INSERT dbo.Item_ChestClaimResolutionLog
            (ChestID, CharID, ClaimToken, Action, Reason)
        SELECT ChestID, CharID, ClaimToken, @Action, LEFT(@Reason, 1000)
        FROM dbo.Item_ChestClaim WITH (UPDLOCK, HOLDLOCK)
        WHERE ChestID = @ChestID
          AND State = 0;

        IF @@ROWCOUNT <> 1
            THROW 51017, 'No pending claim was found for the supplied ChestID.', 1;

        IF @Action = 'RETRY'
        BEGIN
            DELETE dbo.Item_ChestClaim
            WHERE ChestID = @ChestID
              AND State = 0;
        END
        ELSE
        BEGIN
            UPDATE dbo.Item_ChestClaim
            SET State = 1,
                CompletedAtUtc = SYSUTCDATETIME(),
                LastError = LEFT(@Reason, 1000)
            WHERE ChestID = @ChestID
              AND State = 0;

            DELETE dbo.Item_Chest
            WHERE ID = @ChestID;
        END;
    END;

    COMMIT TRANSACTION;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Item_ChestSendToOnline
    @ItemCodeName VARCHAR(128),
    @Quantity     INT = 1,
    @From         VARCHAR(100) = 'OnlineReward',
    @Plus         INT = 0
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    DECLARE
        @ItemRefObjID INT,
        @BatchID UNIQUEIDENTIFIER = NEWID(),
        @CommandQueueID INT;

    SELECT @ItemRefObjID = ROC.ID
    FROM SRO_VT_SHARD.dbo._RefObjCommon AS ROC WITH (NOLOCK)
    WHERE ROC.CodeName128 = @ItemCodeName
      AND ROC.Service = 1
      AND ROC.TypeID1 = 3;

    IF @ItemRefObjID IS NULL
        THROW 51020, 'ItemCodeName was not found or is not an active item.', 1;
    IF @Quantity <= 0 OR @Quantity > 1000000
        THROW 51021, 'Quantity must be between 1 and 1000000.', 1;
    IF @Plus < 0 OR @Plus > 255
        THROW 51022, 'Plus must be between 0 and 255.', 1;
    IF NULLIF(LTRIM(RTRIM(@From)), '') IS NULL
        SET @From = 'OnlineReward';

    SET @From = LEFT(@From, 100);

    BEGIN TRANSACTION;

    INSERT dbo.Item_ChestBroadcast
        (BatchID, Audience, ItemRefObjID, Quantity, Source, Plus)
    VALUES
        (@BatchID, 'ONLINE', @ItemRefObjID, @Quantity, @From, CONVERT(TINYINT, @Plus));

    INSERT dbo.Command_FilterQueue
        (CommandID, Data1, Data2, Data3, Data4, Data5, Status)
    VALUES
        (42, CONVERT(VARCHAR(20), @ItemRefObjID),
         CONVERT(VARCHAR(20), @Quantity), @From,
         CONVERT(VARCHAR(3), @Plus), CONVERT(VARCHAR(36), @BatchID), 1);

    SET @CommandQueueID = CONVERT(INT, SCOPE_IDENTITY());

    UPDATE dbo.Item_ChestBroadcast
    SET CommandQueueID = @CommandQueueID
    WHERE BatchID = @BatchID;

    COMMIT TRANSACTION;

    SELECT
        @BatchID AS BatchID,
        @CommandQueueID AS CommandQueueID,
        CAST('QUEUED' AS VARCHAR(10)) AS Result;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Item_ChestSendToAll
    @ItemCodeName VARCHAR(128),
    @Quantity     INT = 1,
    @From         VARCHAR(100) = 'GlobalReward',
    @Plus         INT = 0
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    DECLARE
        @ItemRefObjID INT,
        @BatchID UNIQUEIDENTIFIER = NEWID(),
        @FormattedDate VARCHAR(10) = CONVERT(VARCHAR(10), GETDATE(), 103);

    SELECT @ItemRefObjID = ROC.ID
    FROM SRO_VT_SHARD.dbo._RefObjCommon AS ROC WITH (NOLOCK)
    WHERE ROC.CodeName128 = @ItemCodeName
      AND ROC.Service = 1
      AND ROC.TypeID1 = 3;

    IF @ItemRefObjID IS NULL
        THROW 51030, 'ItemCodeName was not found or is not an active item.', 1;
    IF @Quantity <= 0 OR @Quantity > 1000000
        THROW 51031, 'Quantity must be between 1 and 1000000.', 1;
    IF @Plus < 0 OR @Plus > 255
        THROW 51032, 'Plus must be between 0 and 255.', 1;
    IF NULLIF(LTRIM(RTRIM(@From)), '') IS NULL
        SET @From = 'GlobalReward';

    SET @From = LEFT(@From, 100);

    DECLARE @Inserted TABLE
    (
        CharID  INT NOT NULL PRIMARY KEY,
        ChestID INT NOT NULL
    );

    BEGIN TRANSACTION;

    INSERT dbo.Item_ChestBroadcast
        (BatchID, Audience, ItemRefObjID, Quantity, Source, Plus)
    VALUES
        (@BatchID, 'ALL', @ItemRefObjID, @Quantity, @From, CONVERT(TINYINT, @Plus));

    MERGE dbo.Item_Chest AS Target
    USING
    (
        SELECT C.CharID
        FROM SRO_VT_SHARD.dbo._Char AS C WITH (HOLDLOCK)
        WHERE C.CharID > 0
          AND C.Deleted = 0
    ) AS Source
        ON 1 = 0
    WHEN NOT MATCHED THEN
        INSERT (CharID, ItemCodeName, ItemID, Quantity, [Date], [Type], Plus)
        VALUES (Source.CharID, @ItemCodeName, @ItemRefObjID, @Quantity,
                @FormattedDate, @From, CONVERT(TINYINT, @Plus))
    OUTPUT Source.CharID, Inserted.ID
        INTO @Inserted(CharID, ChestID);

    INSERT dbo.Item_ChestBroadcastRecipient(BatchID, CharID, ChestID)
    SELECT @BatchID, I.CharID, I.ChestID
    FROM @Inserted AS I;

    INSERT dbo.Command_FilterQueue
        (CommandID, Data1, Data2, Data3, Data4, Data5, Data6, Data7, Data8, Status)
    SELECT
        17,
        CONVERT(VARCHAR(20), I.ChestID),
        CONVERT(VARCHAR(20), I.CharID),
        @ItemCodeName,
        CONVERT(VARCHAR(20), @ItemRefObjID),
        CONVERT(VARCHAR(20), @Quantity),
        @FormattedDate,
        @From,
        CONVERT(VARCHAR(3), @Plus),
        1
    FROM @Inserted AS I;

    COMMIT TRANSACTION;

    SELECT
        @BatchID AS BatchID,
        COUNT(*) AS RecipientCount,
        CAST('COMPLETED' AS VARCHAR(10)) AS Result
    FROM @Inserted;
END;
GO

/*
    Remove procedure names from the superseded draft. Existing legacy
    Item_AddChestOld is intentionally left untouched for customer compatibility.
*/
DROP PROCEDURE IF EXISTS dbo.Item_ChestTryBeginClaim;
DROP PROCEDURE IF EXISTS dbo.Item_ChestCompleteClaim;
DROP PROCEDURE IF EXISTS dbo.Item_ChestResolvePendingClaim;
DROP PROCEDURE IF EXISTS dbo.Item_AddChestToOnlinePlayers;
DROP PROCEDURE IF EXISTS dbo.Item_AddChestToOnlinePlayersByCodeName;
DROP PROCEDURE IF EXISTS dbo.Item_AddChestBroadcastRecipient;
GO
