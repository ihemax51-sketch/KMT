SET NOCOUNT ON;
SET XACT_ABORT ON;

IF OBJECT_ID(N'dbo.UniqueHistory', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.UniqueHistory
    (
        UniqueID int NOT NULL
            CONSTRAINT PK_UniqueHistory PRIMARY KEY,
        KillerName nvarchar(64) NOT NULL
            CONSTRAINT DF_UniqueHistory_KillerName DEFAULT (N''),
        State tinyint NOT NULL,
        EventTime bigint NOT NULL,
        RegionID int NOT NULL
            CONSTRAINT DF_UniqueHistory_RegionID DEFAULT (0),
        PosX real NOT NULL
            CONSTRAINT DF_UniqueHistory_PosX DEFAULT (0),
        PosY real NOT NULL
            CONSTRAINT DF_UniqueHistory_PosY DEFAULT (0),
        PosZ real NOT NULL
            CONSTRAINT DF_UniqueHistory_PosZ DEFAULT (0),
        WorldID int NOT NULL
            CONSTRAINT DF_UniqueHistory_WorldID DEFAULT (0),
        MapType tinyint NOT NULL
            CONSTRAINT DF_UniqueHistory_MapType DEFAULT (0),
        MapIndex int NOT NULL
            CONSTRAINT DF_UniqueHistory_MapIndex DEFAULT (0),
        DpsJson nvarchar(max) NOT NULL
            CONSTRAINT DF_UniqueHistory_DpsJson DEFAULT (N'{}'),
        UpdatedAt datetime2 NOT NULL
            CONSTRAINT DF_UniqueHistory_UpdatedAt DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT CK_UniqueHistory_State CHECK (State IN (0, 1)),
        CONSTRAINT CK_UniqueHistory_DpsJson CHECK (ISJSON(DpsJson) = 1)
    );
END;
