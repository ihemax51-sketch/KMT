USE [KMTGuard];
GO
SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.Command_GameServerQueue', N'U') IS NULL
    THROW 51000, 'dbo.Command_GameServerQueue is required.', 1;
GO

IF COL_LENGTH(N'dbo.Command_GameServerQueue', N'AttemptCount') IS NULL
    ALTER TABLE dbo.Command_GameServerQueue ADD AttemptCount int NOT NULL
        CONSTRAINT DF_Command_GameServerQueue_AttemptCount DEFAULT (0) WITH VALUES;
IF COL_LENGTH(N'dbo.Command_GameServerQueue', N'ClaimToken') IS NULL
    ALTER TABLE dbo.Command_GameServerQueue ADD ClaimToken uniqueidentifier NULL;
IF COL_LENGTH(N'dbo.Command_GameServerQueue', N'ClaimedAtUtc') IS NULL
    ALTER TABLE dbo.Command_GameServerQueue ADD ClaimedAtUtc datetime2(0) NULL;
IF COL_LENGTH(N'dbo.Command_GameServerQueue', N'ClaimState') IS NULL
    ALTER TABLE dbo.Command_GameServerQueue ADD ClaimState tinyint NOT NULL
        CONSTRAINT DF_Command_GameServerQueue_ClaimState DEFAULT (0) WITH VALUES;
IF COL_LENGTH(N'dbo.Command_GameServerQueue', N'LastError') IS NULL
    ALTER TABLE dbo.Command_GameServerQueue ADD LastError nvarchar(256) NULL;
GO

IF OBJECT_ID(N'dbo.Command_GameServerResult', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Command_GameServerResult
    (
        CommandID bigint NOT NULL CONSTRAINT PK_Command_GameServerResult PRIMARY KEY,
        ActionID int NOT NULL,
        Status varchar(16) NOT NULL,
        ResultCode int NOT NULL,
        Reason nvarchar(256) NULL,
        AttemptCount int NOT NULL,
        CompletedAtUtc datetime2(0) NOT NULL
            CONSTRAINT DF_Command_GameServerResult_CompletedAtUtc DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT CK_Command_GameServerResult_Status
            CHECK (Status IN ('Dispatched','Rejected','Failed','Indeterminate'))
    );
END;
GO

IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id=OBJECT_ID(N'dbo.Command_GameServerResult') AND name=N'IX_Command_GameServerResult_CompletedAtUtc')
    CREATE INDEX IX_Command_GameServerResult_CompletedAtUtc
        ON dbo.Command_GameServerResult(CompletedAtUtc);
IF NOT EXISTS (SELECT 1 FROM sys.indexes WHERE object_id=OBJECT_ID(N'dbo.Command_GameServerQueue') AND name=N'IX_Command_GameServerQueue_Claim')
    CREATE INDEX IX_Command_GameServerQueue_Claim
        ON dbo.Command_GameServerQueue(ClaimState, PlannedTime, ID)
        INCLUDE(Action_ID, AttemptCount);
GO

IF OBJECT_ID(N'dbo._ServerFortressDpsInfo', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._ServerFortressDpsInfo
    (
        StructObjID int NOT NULL CONSTRAINT PK_ServerFortressDpsInfo PRIMARY KEY,
        Enabled bit NOT NULL CONSTRAINT DF_ServerFortressDpsInfo_Enabled DEFAULT (1)
    );
END
ELSE IF COL_LENGTH(N'dbo._ServerFortressDpsInfo', N'Enabled') IS NULL
    ALTER TABLE dbo._ServerFortressDpsInfo ADD Enabled bit NOT NULL
        CONSTRAINT DF_ServerFortressDpsInfo_Enabled DEFAULT (1) WITH VALUES;

IF COL_LENGTH(N'dbo._ServerFortressDpsInfo', N'StructObjID') IS NULL
    THROW 51004, 'Existing _ServerFortressDpsInfo table is incompatible: StructObjID is required.', 1;
IF EXISTS(SELECT 1 FROM dbo._ServerFortressDpsInfo WHERE StructObjID IS NULL)
    THROW 51005, 'Existing _ServerFortressDpsInfo contains a null StructObjID.', 1;
IF EXISTS
(
    SELECT StructObjID FROM dbo._ServerFortressDpsInfo
    GROUP BY StructObjID HAVING COUNT(*)>1
)
    THROW 51006, 'Existing _ServerFortressDpsInfo contains duplicate StructObjID values.', 1;
IF EXISTS
(
    SELECT 1 FROM sys.columns
    WHERE object_id=OBJECT_ID(N'dbo._ServerFortressDpsInfo')
      AND name=N'StructObjID' AND (system_type_id<>56 OR is_nullable=1)
)
    ALTER TABLE dbo._ServerFortressDpsInfo ALTER COLUMN StructObjID int NOT NULL;
IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes i
    INNER JOIN sys.index_columns ic ON ic.object_id=i.object_id AND ic.index_id=i.index_id
    INNER JOIN sys.columns c ON c.object_id=ic.object_id AND c.column_id=ic.column_id
    WHERE i.object_id=OBJECT_ID(N'dbo._ServerFortressDpsInfo')
      AND i.is_unique=1 AND ic.key_ordinal=1 AND c.name=N'StructObjID'
      AND NOT EXISTS
      (
          SELECT 1 FROM sys.index_columns extra
          WHERE extra.object_id=i.object_id AND extra.index_id=i.index_id AND extra.key_ordinal>1
      )
)
    CREATE UNIQUE INDEX UX_ServerFortressDpsInfo_StructObjID
        ON dbo._ServerFortressDpsInfo(StructObjID);
GO

CREATE OR ALTER PROCEDURE dbo.Command_ClaimGameServer
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;
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
        FROM dbo.Command_GameServerQueue WITH(UPDLOCK,READPAST,ROWLOCK)
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

CREATE OR ALTER PROCEDURE dbo.Command_CompleteGameServer
    @CommandID bigint,
    @ClaimToken uniqueidentifier,
    @Status varchar(16),
    @ResultCode int,
    @Reason nvarchar(256)=NULL
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;
    IF @Status NOT IN ('Dispatched','Rejected','Failed','Indeterminate')
        THROW 51001, 'Invalid GameServer command result status.', 1;

    BEGIN TRANSACTION;
    DECLARE @ActionID int,@AttemptCount int;
    SELECT @ActionID=Action_ID,@AttemptCount=AttemptCount
    FROM dbo.Command_GameServerQueue WITH(UPDLOCK,HOLDLOCK)
    WHERE ID=@CommandID AND ClaimState=1 AND ClaimToken=@ClaimToken;

    IF @ActionID IS NULL
    BEGIN
        IF EXISTS(SELECT 1 FROM dbo.Command_GameServerResult WHERE CommandID=@CommandID)
        BEGIN
            COMMIT TRANSACTION;
            RETURN;
        END;
        ROLLBACK TRANSACTION;
        THROW 51002, 'GameServer command claim no longer exists.', 1;
    END;

    INSERT dbo.Command_GameServerResult(CommandID,ActionID,Status,ResultCode,Reason,AttemptCount,CompletedAtUtc)
    VALUES(@CommandID,@ActionID,@Status,@ResultCode,LEFT(@Reason,256),@AttemptCount,SYSUTCDATETIME());
    DELETE dbo.Command_GameServerQueue WHERE ID=@CommandID AND ClaimToken=@ClaimToken;
    COMMIT TRANSACTION;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Command_RetryGameServer
    @CommandID bigint,
    @ClaimToken uniqueidentifier,
    @Reason nvarchar(256)
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;
    BEGIN TRANSACTION;
    DECLARE @ActionID int,@AttemptCount int;
    SELECT @ActionID=Action_ID,@AttemptCount=AttemptCount
    FROM dbo.Command_GameServerQueue WITH(UPDLOCK,HOLDLOCK)
    WHERE ID=@CommandID AND ClaimState=1 AND ClaimToken=@ClaimToken;
    IF @ActionID IS NULL
    BEGIN
        ROLLBACK TRANSACTION;
        THROW 51003, 'GameServer command retry claim no longer exists.', 1;
    END;

    IF @AttemptCount>=5
    BEGIN
        INSERT dbo.Command_GameServerResult(CommandID,ActionID,Status,ResultCode,Reason,AttemptCount,CompletedAtUtc)
        VALUES(@CommandID,@ActionID,'Failed',1003,LEFT(@Reason,256),@AttemptCount,SYSUTCDATETIME());
        DELETE dbo.Command_GameServerQueue WHERE ID=@CommandID AND ClaimToken=@ClaimToken;
    END
    ELSE
    BEGIN
        DECLARE @DelaySeconds int=CASE @AttemptCount WHEN 1 THEN 5 WHEN 2 THEN 15 WHEN 3 THEN 30 ELSE 60 END;
        UPDATE dbo.Command_GameServerQueue
           SET ClaimState=0,ClaimToken=NULL,ClaimedAtUtc=NULL,
               LastError=LEFT(@Reason,256),PlannedTime=DATEADD(SECOND,@DelaySeconds,GETDATE())
         WHERE ID=@CommandID AND ClaimToken=@ClaimToken;
    END;
    COMMIT TRANSACTION;
END;
GO

BEGIN TRANSACTION;
DECLARE @RejectedCommands TABLE
(
    CommandID bigint NOT NULL PRIMARY KEY,
    ActionID int NOT NULL,
    Reason nvarchar(256) NOT NULL
);
INSERT @RejectedCommands(CommandID,ActionID,Reason)
SELECT q.ID,q.Action_ID,
       CASE WHEN q.Action_ID=131 THEN N'Retired GameServer action.'
            ELSE N'Invalid queued GameServer command payload.' END
FROM dbo.Command_GameServerQueue q WITH(UPDLOCK,HOLDLOCK)
WHERE q.Action_ID=131
   OR (q.Action_ID=2 AND
       (ISNULL(TRY_CONVERT(int,q.Data1),0)<=0 OR ISNULL(TRY_CONVERT(int,q.Data2),0) NOT BETWEEN 1 AND 65535 OR
        ISNULL(TRY_CONVERT(int,q.Data3),0) NOT BETWEEN 1 AND 65535 OR
        ISNULL(TRY_CONVERT(int,q.Data4),1000001) NOT BETWEEN -1000000 AND 1000000 OR
        ISNULL(TRY_CONVERT(int,q.Data5),1000001) NOT BETWEEN -1000000 AND 1000000 OR
        ISNULL(TRY_CONVERT(int,q.Data6),1000001) NOT BETWEEN -1000000 AND 1000000 OR
        ISNULL(TRY_CONVERT(int,q.Data7),-1) NOT BETWEEN 0 AND 1000000))
   OR (q.Action_ID IN (14,23,24) AND
       (ISNULL(TRY_CONVERT(int,q.Data1),0)<=0 OR ISNULL(TRY_CONVERT(int,q.Data2),0) NOT BETWEEN 1 AND 65535 OR
        ISNULL(TRY_CONVERT(int,q.Data3),0) NOT BETWEEN 1 AND 65535 OR
        ISNULL(TRY_CONVERT(int,q.Data4),1000001) NOT BETWEEN -1000000 AND 1000000 OR
        ISNULL(TRY_CONVERT(int,q.Data5),1000001) NOT BETWEEN -1000000 AND 1000000 OR
        ISNULL(TRY_CONVERT(int,q.Data6),1000001) NOT BETWEEN -1000000 AND 1000000))
   OR (q.Action_ID=17 AND
       (ISNULL(TRY_CONVERT(int,q.Data1),0)<=0 OR ISNULL(TRY_CONVERT(int,q.Data2),-1) NOT BETWEEN 0 AND 255 OR
        NULLIF(LTRIM(RTRIM(CONVERT(varchar(8000),q.Data3))),'') IS NULL))
   OR (q.Action_ID=18 AND
       (ISNULL(TRY_CONVERT(int,q.Data1),0)<=0 OR ISNULL(TRY_CONVERT(int,q.Data2),-1) NOT BETWEEN 0 AND 255 OR
        ISNULL(TRY_CONVERT(int,q.Data3),0)<=0))
   OR (q.Action_ID=19 AND
       (ISNULL(TRY_CONVERT(int,q.Data1),0)<=0 OR ISNULL(TRY_CONVERT(int,q.Data2),-1) NOT BETWEEN 0 AND 255 OR
        NULLIF(LTRIM(RTRIM(CONVERT(varchar(8000),q.Data3))),'') IS NULL OR
        ISNULL(TRY_CONVERT(int,q.Data4),-1) NOT BETWEEN 0 AND 255 OR
        TRY_CONVERT(int,q.Data2)=TRY_CONVERT(int,q.Data4) OR ISNULL(TRY_CONVERT(int,q.Data5),0)<=0))
   OR (q.Action_ID=21 AND
       (ISNULL(TRY_CONVERT(int,q.Data1),0)<=0 OR ISNULL(TRY_CONVERT(bigint,q.Data2),0)<=0 OR
        ISNULL(TRY_CONVERT(int,q.Data3),-1) NOT IN (0,1)))
   OR (q.Action_ID=22 AND
       (ISNULL(TRY_CONVERT(int,q.Data1),0) NOT BETWEEN 1 AND 65535 OR
        ISNULL(TRY_CONVERT(int,q.Data2),-1) NOT BETWEEN 0 AND 65535))
   OR (q.Action_ID=38 AND
       (ISNULL(TRY_CONVERT(int,q.Data1),-1) NOT IN (0,1) OR
        (TRY_CONVERT(int,q.Data1)=1 AND
         (ISNULL(TRY_CONVERT(int,q.Data2),0) NOT BETWEEN 1 AND 65535 OR ISNULL(TRY_CONVERT(int,q.Data3),0) NOT BETWEEN 1 AND 65535 OR
          ISNULL(TRY_CONVERT(int,q.Data4),0)<=0 OR ISNULL(TRY_CONVERT(int,q.Data5),-1) NOT BETWEEN 0 AND 5 OR
          ISNULL(TRY_CONVERT(int,q.Data6),0)<=0 OR ISNULL(TRY_CONVERT(int,q.Data7),-1) NOT BETWEEN 0 AND 5))))
   OR (q.Action_ID=39 AND
       (ISNULL(TRY_CONVERT(int,q.Data1),-1) NOT IN (0,1) OR
        (TRY_CONVERT(int,q.Data1)=1 AND
         (ISNULL(TRY_CONVERT(int,q.Data2),0) NOT BETWEEN 1 AND 65535 OR
          ISNULL(TRY_CONVERT(int,q.Data3),0) NOT BETWEEN 1 AND 65535))));

INSERT dbo.Command_GameServerResult(CommandID,ActionID,Status,ResultCode,Reason,AttemptCount,CompletedAtUtc)
SELECT rejected.CommandID,rejected.ActionID,'Rejected',1001,rejected.Reason,q.AttemptCount,SYSUTCDATETIME()
FROM @RejectedCommands rejected
INNER JOIN dbo.Command_GameServerQueue q ON q.ID=rejected.CommandID
WHERE NOT EXISTS(SELECT 1 FROM dbo.Command_GameServerResult r WHERE r.CommandID=rejected.CommandID);
DELETE q
FROM dbo.Command_GameServerQueue q
INNER JOIN @RejectedCommands rejected ON rejected.CommandID=q.ID;
COMMIT TRANSACTION;
GO

CREATE OR ALTER PROCEDURE dbo.Live_Gold
    @CharID int,@Gold bigint,@AddOrRemove int
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;
    IF @CharID<=0 OR @Gold<=0 OR @AddOrRemove NOT IN (0,1)
        THROW 51010, 'Invalid live Gold request.', 1;
    BEGIN TRANSACTION;
    DECLARE @CurrentGold bigint;
    SELECT @CurrentGold=RemainGold
    FROM SRO_VT_SHARD.dbo._Char WITH(UPDLOCK,HOLDLOCK)
    WHERE CharID=@CharID;
    IF @CurrentGold IS NULL
    BEGIN
        ROLLBACK TRANSACTION;
        THROW 51011, 'Character does not exist.', 1;
    END;
    IF (@AddOrRemove=0 AND @CurrentGold<@Gold) OR
       (@AddOrRemove=1 AND @CurrentGold>9223372036854775807-@Gold)
    BEGIN
        ROLLBACK TRANSACTION;
        THROW 51021, 'Gold balance or result is outside the valid range.', 1;
    END;
    INSERT dbo.Command_GameServerQueue(Action_ID,Data1,Data2,Data3)
    VALUES(21,@CharID,@Gold,@AddOrRemove);
    COMMIT TRANSACTION;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Live_ChangeItem
    @CharID int,@Slot tinyint,@CodeName128 varchar(128)
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;
    IF @CharID<=0 OR NULLIF(LTRIM(RTRIM(@CodeName128)),'') IS NULL
        THROW 51012, 'Invalid live item mutation request.', 1;
    BEGIN TRANSACTION;
    IF NOT EXISTS(SELECT 1 FROM SRO_VT_SHARD.dbo._Inventory WITH(UPDLOCK,HOLDLOCK) WHERE CharID=@CharID AND Slot=@Slot AND ItemID>0)
    BEGIN ROLLBACK TRANSACTION; THROW 51013, 'Inventory item does not exist.', 1; END;
    IF NOT EXISTS(SELECT 1 FROM SRO_VT_SHARD.dbo._RefObjCommon WHERE Service=1 AND CodeName128=@CodeName128)
    BEGIN ROLLBACK TRANSACTION; THROW 51014, 'Target item code does not exist.', 1; END;
    INSERT dbo.Command_GameServerQueue(Action_ID,Data1,Data2,Data3) VALUES(17,@CharID,@Slot,@CodeName128);
    COMMIT TRANSACTION;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Live_UseItem
    @CharID int,@Slot tinyint,@ReduceAmount int
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;
    IF @CharID<=0 OR @ReduceAmount<=0 THROW 51015, 'Invalid live item consumption request.', 1;
    BEGIN TRANSACTION;
    IF NOT EXISTS
    (
        SELECT 1
        FROM SRO_VT_SHARD.dbo._Inventory inventory WITH(UPDLOCK,HOLDLOCK)
        INNER JOIN SRO_VT_SHARD.dbo._Items item WITH(UPDLOCK,HOLDLOCK) ON item.ID64=inventory.ItemID
        WHERE inventory.CharID=@CharID AND inventory.Slot=@Slot AND inventory.ItemID>0
          AND item.Data>=@ReduceAmount
    )
    BEGIN ROLLBACK TRANSACTION; THROW 51016, 'Inventory item or quantity is invalid.', 1; END;
    INSERT dbo.Command_GameServerQueue(Action_ID,Data1,Data2,Data3) VALUES(18,@CharID,@Slot,@ReduceAmount);
    COMMIT TRANSACTION;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Live_UseAndChangeItem
    @CharID int,@MutateSlot tinyint,@CodeName128 varchar(128),@ConsumeSlot tinyint,@ConsumeAmount int
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;
    IF @CharID<=0 OR @MutateSlot=@ConsumeSlot OR @ConsumeAmount<=0 OR NULLIF(LTRIM(RTRIM(@CodeName128)),'') IS NULL
        THROW 51017, 'Invalid consume-and-mutate request.', 1;
    BEGIN TRANSACTION;
    IF (SELECT COUNT(*) FROM SRO_VT_SHARD.dbo._Inventory WITH(UPDLOCK,HOLDLOCK)
        WHERE CharID=@CharID AND Slot IN (@MutateSlot,@ConsumeSlot) AND ItemID>0)<>2
    BEGIN ROLLBACK TRANSACTION; THROW 51018, 'Required inventory items do not exist.', 1; END;
    IF NOT EXISTS
    (
        SELECT 1
        FROM SRO_VT_SHARD.dbo._Inventory inventory WITH(UPDLOCK,HOLDLOCK)
        INNER JOIN SRO_VT_SHARD.dbo._Items item WITH(UPDLOCK,HOLDLOCK) ON item.ID64=inventory.ItemID
        WHERE inventory.CharID=@CharID AND inventory.Slot=@ConsumeSlot
          AND inventory.ItemID>0 AND item.Data>=@ConsumeAmount
    )
    BEGIN ROLLBACK TRANSACTION; THROW 51020, 'Consumed item quantity is insufficient.', 1; END;
    IF NOT EXISTS(SELECT 1 FROM SRO_VT_SHARD.dbo._RefObjCommon WHERE Service=1 AND CodeName128=@CodeName128)
    BEGIN ROLLBACK TRANSACTION; THROW 51019, 'Target item code does not exist.', 1; END;
    INSERT dbo.Command_GameServerQueue(Action_ID,Data1,Data2,Data3,Data4,Data5)
    VALUES(19,@CharID,@MutateSlot,@CodeName128,@ConsumeSlot,@ConsumeAmount);
    COMMIT TRANSACTION;
END;
GO
