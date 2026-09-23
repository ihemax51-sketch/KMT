USE [Events];
GO

IF OBJECT_ID(N'dbo.EventRewardOutbox', N'U') IS NULL
    THROW 51000, 'EventRewardOutbox is missing.', 1;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.EventRewardOutbox')
      AND name = N'UQ_EventRewardOutbox_Reward'
      AND is_unique = 1
)
    THROW 51000, 'EventRewardOutbox idempotency key is missing.', 1;
GO
