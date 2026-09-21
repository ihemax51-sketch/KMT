USE [KMTGuard];
GO

IF OBJECT_ID(N'dbo._LockedItemList', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._LockedItemList
    (
        ID INT IDENTITY(1,1) NOT NULL CONSTRAINT PK__LockedItemList PRIMARY KEY,
        ItemID64 BIGINT NOT NULL,
        [Password] INT NOT NULL CONSTRAINT DF__LockedItemList__Password DEFAULT (0)
    );
END
GO

IF COL_LENGTH(N'dbo._LockedItemList', N'Password') IS NULL
BEGIN
    ALTER TABLE dbo._LockedItemList
        ADD [Password] INT NOT NULL CONSTRAINT DF__LockedItemList__Password DEFAULT (0) WITH VALUES;
END
GO

IF NOT EXISTS (
    SELECT 1
    FROM sys.indexes
    WHERE name = N'UX__LockedItemList__ItemID64'
      AND object_id = OBJECT_ID(N'dbo._LockedItemList')
)
BEGIN
    CREATE UNIQUE INDEX UX__LockedItemList__ItemID64
        ON dbo._LockedItemList(ItemID64);
END
GO

IF OBJECT_ID(N'dbo._TimedItemPlus', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._TimedItemPlus
    (
        ID INT IDENTITY(1,1) NOT NULL CONSTRAINT PK__TimedItemPlus PRIMARY KEY,
        CharID INT NOT NULL,
        OrjPlus INT NOT NULL,
        ID64 BIGINT NOT NULL,
        EndTime BIGINT NOT NULL
    );
END
GO

IF OBJECT_ID(N'dbo._TimedDevillPlus', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._TimedDevillPlus
    (
        ID INT IDENTITY(1,1) NOT NULL CONSTRAINT PK__TimedDevillPlus PRIMARY KEY,
        CharID INT NOT NULL,
        OrjPlus INT NOT NULL,
        ID64 BIGINT NOT NULL,
        EndTime BIGINT NOT NULL
    );
END
GO

IF OBJECT_ID(N'dbo._ServerAutoCapebyRegionID', N'U') IS NULL
BEGIN
    CREATE TABLE dbo._ServerAutoCapebyRegionID
    (
        ID INT IDENTITY(1,1) NOT NULL CONSTRAINT PK__ServerAutoCapebyRegionID PRIMARY KEY,
        RegionID SMALLINT NOT NULL
    );
END
GO
