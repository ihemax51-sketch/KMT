USE [Events];
GO

IF OBJECT_ID(N'dbo.EventRewardOutbox', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.EventRewardOutbox
    (
        OutboxID bigint IDENTITY(1,1) NOT NULL
            CONSTRAINT PK_EventRewardOutbox PRIMARY KEY,
        RunID bigint NOT NULL,
        RoundID bigint NOT NULL,
        CharID int NOT NULL,
        RewardID int NOT NULL,
        RewardType nvarchar(32) NOT NULL,
        RewardValue nvarchar(512) NOT NULL,
        Status nvarchar(16) NOT NULL,
        Attempts int NOT NULL CONSTRAINT DF_EventRewardOutbox_Attempts DEFAULT (0),
        CreatedDate datetime2(0) NOT NULL
            CONSTRAINT DF_EventRewardOutbox_CreatedDate DEFAULT (SYSUTCDATETIME()),
        CompletedDate datetime2(0) NULL,
        CONSTRAINT UQ_EventRewardOutbox_Reward UNIQUE (RunID, RoundID, CharID, RewardID),
        CONSTRAINT CK_EventRewardOutbox_Status CHECK (Status IN (N'Pending', N'Processing', N'Completed'))
    );

    CREATE INDEX IX_EventRewardOutbox_StatusCreated
        ON dbo.EventRewardOutbox (Status, CreatedDate)
        INCLUDE (RunID, RoundID, CharID, RewardID, Attempts);
END;
GO
