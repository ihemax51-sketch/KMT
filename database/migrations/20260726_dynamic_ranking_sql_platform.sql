USE [KMTGuard];
GO

SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.Rank_Categories', N'U') IS NULL
    THROW 51000, 'KMTGuard.dbo.Rank_Categories is required.', 1;

IF EXISTS
(
    SELECT 1
    FROM (VALUES
        (N'Rank_Data01'), (N'Rank_Data02'), (N'Rank_Data03'),
        (N'Rank_Data04'), (N'Rank_Data05'), (N'Rank_Data06'),
        (N'Rank_Data07'), (N'Rank_Data08'), (N'Rank_Data09')
    ) AS required(TableName)
    WHERE OBJECT_ID(N'dbo.' + required.TableName, N'U') IS NULL
)
    THROW 51000, 'All Rank_Data01 through Rank_Data09 tables are required.', 1;
GO

IF OBJECT_ID(N'dbo.Rank_DataArchive', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.Rank_DataArchive
    (
        ArchiveID bigint IDENTITY(1,1) NOT NULL
            CONSTRAINT PK_Rank_DataArchive PRIMARY KEY,
        CategoryID tinyint NOT NULL,
        CharID int NOT NULL,
        CharName16 varchar(16) NOT NULL,
        GuildName varchar(16) NOT NULL,
        Point int NOT NULL,
        ArchivedAtUtc datetime2(3) NOT NULL
            CONSTRAINT DF_Rank_DataArchive_ArchivedAtUtc DEFAULT SYSUTCDATETIME(),
        ArchiveReason nvarchar(256) NOT NULL
    );

    CREATE INDEX IX_Rank_DataArchive_Category_Char
        ON dbo.Rank_DataArchive(CategoryID, CharID, ArchivedAtUtc DESC);
END;
GO

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.Rank_Categories')
      AND name = N'UX_Rank_Categories_Category'
)
BEGIN
    IF EXISTS
    (
        SELECT Category
        FROM dbo.Rank_Categories
        GROUP BY Category
        HAVING COUNT(*) > 1
    )
        THROW 51000, 'Duplicate category names must be resolved before installing the ranking platform.', 1;

    CREATE UNIQUE INDEX UX_Rank_Categories_Category
        ON dbo.Rank_Categories(Category);
END;
GO

IF NOT EXISTS
(
    SELECT 1
    FROM dbo.System_Settings
    WHERE SettingName = N'DynamicRankingRefreshMinutes'
)
BEGIN
    INSERT dbo.System_Settings
        (SettingName, Value, Category, DisplayOrder, Description)
    VALUES
        (N'DynamicRankingRefreshMinutes', N'10', N'Interface', 0,
         N'Atomic Dynamic Ranking SQL snapshot refresh interval in minutes (1-1440).');
END;
GO

CREATE OR ALTER PROCEDURE dbo.Rank_SetCategory
    @CategoryID tinyint,
    @Category varchar(255),
    @Active bit = 1
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @CategoryID NOT BETWEEN 1 AND 9
        THROW 51000, 'CategoryID must be between 1 and 9.', 1;

    SET @Category = LTRIM(RTRIM(@Category));
    IF NULLIF(@Category, '') IS NULL
        THROW 51000, 'Category name is required.', 1;

    IF EXISTS
    (
        SELECT 1
        FROM dbo.Rank_Categories
        WHERE Category = @Category
          AND ID <> @CategoryID
    )
        THROW 51000, 'Category names must be unique.', 1;

    UPDATE dbo.Rank_Categories
       SET Category = @Category,
           Active = @Active
     WHERE ID = @CategoryID;

    IF @@ROWCOUNT = 0
    BEGIN
        INSERT dbo.Rank_Categories(ID, Active, Category)
        VALUES (@CategoryID, @Active, @Category);
    END;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Rank_UpsertEntry
    @CategoryID tinyint,
    @CharID int,
    @Point int
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @CategoryID NOT BETWEEN 1 AND 9
        THROW 51000, 'CategoryID must be between 1 and 9.', 1;
    IF @CharID <= 0
        THROW 51000, 'A valid CharID is required.', 1;

    DECLARE @ShardDB sysname =
    (
        SELECT TOP (1) Value
        FROM dbo.System_Settings
        WHERE SettingName = N'ShardDB'
    );

    IF NULLIF(@ShardDB, N'') IS NULL OR DB_ID(@ShardDB) IS NULL
        THROW 51000, 'System_Settings.ShardDB is missing or invalid.', 1;

    DECLARE
        @CharName16 varchar(16),
        @GuildName varchar(16),
        @IdentitySql nvarchar(max);

    SET @IdentitySql = N'
SELECT
    @CharNameOut = c.CharName16,
    @GuildNameOut = LEFT(COALESCE(g.Name, ''<No Guild>''), 16)
FROM ' + QUOTENAME(@ShardDB) + N'.dbo._Char AS c
LEFT JOIN ' + QUOTENAME(@ShardDB) + N'.dbo._GuildMember AS gm
    ON gm.CharID = c.CharID
LEFT JOIN ' + QUOTENAME(@ShardDB) + N'.dbo._Guild AS g
    ON g.ID = gm.GuildID
WHERE c.CharID = @RequestedCharID;';

    EXEC sys.sp_executesql
        @IdentitySql,
        N'@RequestedCharID int, @CharNameOut varchar(16) OUTPUT, @GuildNameOut varchar(16) OUTPUT',
        @RequestedCharID = @CharID,
        @CharNameOut = @CharName16 OUTPUT,
        @GuildNameOut = @GuildName OUTPUT;

    IF @CharName16 IS NULL
        THROW 51000, 'CharID does not exist in the configured shard database.', 1;

    DECLARE @TargetTable nvarchar(128) =
        N'dbo.' + QUOTENAME(N'Rank_Data' + RIGHT(N'0' + CONVERT(nvarchar(2), @CategoryID), 2));
    DECLARE @WriteSql nvarchar(max) = N'
SET XACT_ABORT ON;
BEGIN TRANSACTION;

DELETE FROM ' + @TargetTable + N'
WHERE CharName16 = @ResolvedName
  AND CharID <> @ResolvedCharID;

UPDATE ' + @TargetTable + N' WITH (UPDLOCK, SERIALIZABLE)
   SET CharName16 = @ResolvedName,
       GuildName = @ResolvedGuild,
       Point = @ResolvedPoint
 WHERE CharID = @ResolvedCharID;

IF @@ROWCOUNT = 0
BEGIN
    INSERT ' + @TargetTable + N'(CharID, CharName16, GuildName, Point)
    VALUES (@ResolvedCharID, @ResolvedName, @ResolvedGuild, @ResolvedPoint);
END;

COMMIT TRANSACTION;';

    EXEC sys.sp_executesql
        @WriteSql,
        N'@ResolvedCharID int, @ResolvedName varchar(16), @ResolvedGuild varchar(16), @ResolvedPoint int',
        @ResolvedCharID = @CharID,
        @ResolvedName = @CharName16,
        @ResolvedGuild = @GuildName,
        @ResolvedPoint = @Point;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Rank_RemoveEntry
    @CategoryID tinyint,
    @CharID int
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF @CategoryID NOT BETWEEN 1 AND 9
        THROW 51000, 'CategoryID must be between 1 and 9.', 1;

    DECLARE @Sql nvarchar(max) =
        N'DELETE dbo.' +
        QUOTENAME(N'Rank_Data' + RIGHT(N'0' + CONVERT(nvarchar(2), @CategoryID), 2)) +
        N' WHERE CharID = @TargetCharID;';

    EXEC sys.sp_executesql
        @Sql,
        N'@TargetCharID int',
        @TargetCharID = @CharID;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Rank_RequestImmediateRefresh
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    BEGIN TRANSACTION;
    IF NOT EXISTS
    (
        SELECT 1
        FROM dbo.Command_FilterQueue WITH (UPDLOCK, HOLDLOCK)
        WHERE CommandID = 40
          AND Status = 1
    )
    BEGIN
        INSERT dbo.Command_FilterQueue(CommandID, Data1, Data2, Status)
        VALUES (40, N'SQL Ranking', N'Immediate refresh', 1);
    END;
    COMMIT TRANSACTION;
END;
GO

CREATE OR ALTER TRIGGER dbo.TR_Rank_Categories_RequestRefresh
ON dbo.Rank_Categories
AFTER INSERT, UPDATE, DELETE
AS
BEGIN
    SET NOCOUNT ON;

    IF NOT EXISTS
    (
        SELECT 1
        FROM dbo.Command_FilterQueue WITH (UPDLOCK, HOLDLOCK)
        WHERE CommandID = 40
          AND Status = 1
    )
    BEGIN
        INSERT dbo.Command_FilterQueue(CommandID, Data1, Data2, Status)
        VALUES (40, N'Ranking categories', N'Configuration changed', 1);
    END;
END;
GO

CREATE OR ALTER PROCEDURE dbo.Rank_ArchiveAndResetAll
    @Reason nvarchar(256) = N'Dynamic ranking platform reset'
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    IF NULLIF(LTRIM(RTRIM(@Reason)), N'') IS NULL
        SET @Reason = N'Dynamic ranking platform reset';

    BEGIN TRANSACTION;

    DECLARE @CategoryID int = 1;
    WHILE @CategoryID <= 9
    BEGIN
        DECLARE @TableName nvarchar(128) =
            N'dbo.' +
            QUOTENAME(N'Rank_Data' + RIGHT(N'0' + CONVERT(nvarchar(2), @CategoryID), 2));
        DECLARE @Sql nvarchar(max) = N'
INSERT dbo.Rank_DataArchive
    (CategoryID, CharID, CharName16, GuildName, Point, ArchiveReason)
SELECT
    @ArchiveCategoryID, CharID, CharName16, GuildName, Point, @ArchiveReason
FROM ' + @TableName + N';

DELETE FROM ' + @TableName + N';';

        EXEC sys.sp_executesql
            @Sql,
            N'@ArchiveCategoryID tinyint, @ArchiveReason nvarchar(256)',
            @ArchiveCategoryID = @CategoryID,
            @ArchiveReason = @Reason;

        SET @CategoryID += 1;
    END;

    SET @CategoryID = 1;
    WHILE @CategoryID <= 9
    BEGIN
        DECLARE @CategoryName varchar(255) =
            'Custom Rank ' + CONVERT(varchar(2), @CategoryID);

        UPDATE dbo.Rank_Categories
           SET Category = @CategoryName,
               Active = 1
         WHERE ID = @CategoryID;

        IF @@ROWCOUNT = 0
        BEGIN
            INSERT dbo.Rank_Categories(ID, Active, Category)
            VALUES (@CategoryID, 1, @CategoryName);
        END;

        SET @CategoryID += 1;
    END;

    COMMIT TRANSACTION;

    EXEC dbo.Rank_RequestImmediateRefresh;
END;
GO

PRINT 'Dynamic ranking SQL platform installed.';
PRINT 'Configure: EXEC dbo.Rank_SetCategory 1, ''My Ranking'', 1;';
PRINT 'Write entry: EXEC dbo.Rank_UpsertEntry 1, @CharID, @Point;';
PRINT 'Refresh now: EXEC dbo.Rank_RequestImmediateRefresh;';
PRINT 'Optional recoverable reset: EXEC dbo.Rank_ArchiveAndResetAll;';
GO
