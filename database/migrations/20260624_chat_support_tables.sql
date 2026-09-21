/*
    Chat support tables for KMTGuard.

    Fixes runtime warnings such as:
    - Invalid object name 'dbo.region_control'
    - Invalid object name 'KMTGuard.dbo.BlockedWords'

    Run on the KMTGuard/proxy database.
*/

SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.region_control', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.region_control
    (
        RegionID INT NOT NULL,
        DisableChat BIT NOT NULL CONSTRAINT DF_region_control_DisableChat DEFAULT (0),
        CreatedAt DATETIME2(0) NOT NULL CONSTRAINT DF_region_control_CreatedAt DEFAULT (SYSDATETIME()),
        UpdatedAt DATETIME2(0) NULL,
        CONSTRAINT PK_region_control PRIMARY KEY CLUSTERED (RegionID)
    );
END;

IF COL_LENGTH(N'dbo.region_control', N'DisableChat') IS NULL
BEGIN
    ALTER TABLE dbo.region_control
        ADD DisableChat BIT NOT NULL CONSTRAINT DF_region_control_DisableChat DEFAULT (0);
END;

IF OBJECT_ID(N'dbo.BlockedWords', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.BlockedWords
    (
        ID INT IDENTITY(1,1) NOT NULL,
        Word NVARCHAR(128) NOT NULL,
        MatchMode TINYINT NOT NULL CONSTRAINT DF_BlockedWords_MatchMode DEFAULT (0),
        IsActive BIT NOT NULL CONSTRAINT DF_BlockedWords_IsActive DEFAULT (1),
        CreatedAt DATETIME2(0) NOT NULL CONSTRAINT DF_BlockedWords_CreatedAt DEFAULT (SYSDATETIME()),
        UpdatedAt DATETIME2(0) NULL,
        CONSTRAINT PK_BlockedWords PRIMARY KEY CLUSTERED (ID)
    );
END;

IF COL_LENGTH(N'dbo.BlockedWords', N'MatchMode') IS NULL
BEGIN
    ALTER TABLE dbo.BlockedWords
        ADD MatchMode TINYINT NOT NULL CONSTRAINT DF_BlockedWords_MatchMode DEFAULT (0);
END;

IF COL_LENGTH(N'dbo.BlockedWords', N'IsActive') IS NULL
BEGIN
    ALTER TABLE dbo.BlockedWords
        ADD IsActive BIT NOT NULL CONSTRAINT DF_BlockedWords_IsActive DEFAULT (1);
END;

IF COL_LENGTH(N'dbo.BlockedWords', N'Word') IS NOT NULL
   AND NOT EXISTS
   (
       SELECT 1
       FROM sys.indexes
       WHERE object_id = OBJECT_ID(N'dbo.BlockedWords')
         AND name = N'UX_BlockedWords_Word_MatchMode'
   )
BEGIN
    CREATE UNIQUE NONCLUSTERED INDEX UX_BlockedWords_Word_MatchMode
        ON dbo.BlockedWords(Word, MatchMode);
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.BlockedWords')
      AND name = N'IX_BlockedWords_IsActive'
)
BEGIN
    CREATE NONCLUSTERED INDEX IX_BlockedWords_IsActive
        ON dbo.BlockedWords(IsActive)
        INCLUDE (Word, MatchMode);
END;

IF OBJECT_ID(N'dbo.ChatLog', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.ChatLog
    (
        ID BIGINT IDENTITY(1,1) NOT NULL,
        [Timestamp] DATETIME2(0) NOT NULL,
        Sender NVARCHAR(64) NOT NULL,
        Receiver NVARCHAR(64) NULL,
        ChatType TINYINT NOT NULL,
        [Message] NVARCHAR(MAX) NOT NULL,
        CONSTRAINT PK_ChatLog PRIMARY KEY CLUSTERED (ID)
    );
END;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.ChatLog')
      AND name = N'IX_ChatLog_Timestamp'
)
BEGIN
    CREATE NONCLUSTERED INDEX IX_ChatLog_Timestamp
        ON dbo.ChatLog([Timestamp] DESC)
        INCLUDE (Sender, Receiver, ChatType);
END;

PRINT 'Chat support tables migration completed.';
