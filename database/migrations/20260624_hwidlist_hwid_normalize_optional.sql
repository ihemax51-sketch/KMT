/*
    OPTIONAL production hardening for dbo._HwidList.Hwid.

    Run this only if dbo._HwidList.Hwid is currently text/ntext/varchar(max)/nvarchar(max).
    SQL Server cannot use those types as index key columns, which is why IX_HwidList_Hwid_Active
    may fail or be skipped.

    This script is defensive:
      - It does nothing if the table/column does not exist.
      - It does nothing if Hwid is already a bounded, indexable type.
      - It aborts if any HWID value is longer than 128 characters.
      - It converts Hwid to NVARCHAR(128), then creates the useful active HWID index.

    Recommended window: maintenance/offline, because ALTER COLUMN can take a schema lock.
*/
SET NOCOUNT ON;

IF OBJECT_ID('dbo._HwidList', 'U') IS NOT NULL
   AND COL_LENGTH('dbo._HwidList', 'Hwid') IS NOT NULL
BEGIN
    DECLARE @type sysname;
    DECLARE @max_length smallint;
    DECLARE @nullable bit;

    SELECT
        @type = t.name,
        @max_length = c.max_length,
        @nullable = c.is_nullable
    FROM sys.columns c
    JOIN sys.types t ON c.user_type_id = t.user_type_id
    WHERE c.object_id = OBJECT_ID('dbo._HwidList')
      AND c.name = 'Hwid';

    IF @type IN ('text', 'ntext') OR @max_length = -1
    BEGIN
        IF EXISTS
        (
            SELECT 1
            FROM dbo._HwidList
            WHERE Hwid IS NOT NULL
              AND LEN(CONVERT(nvarchar(max), Hwid)) > 128
        )
        BEGIN
            RAISERROR('dbo._HwidList.Hwid contains values longer than 128 characters. Review data before ALTER COLUMN.', 16, 1);
            RETURN;
        END;

        IF @nullable = 1
            ALTER TABLE dbo._HwidList ALTER COLUMN Hwid NVARCHAR(128) NULL;
        ELSE
            ALTER TABLE dbo._HwidList ALTER COLUMN Hwid NVARCHAR(128) NOT NULL;
    END;
END;
GO

IF OBJECT_ID('dbo._HwidList', 'U') IS NOT NULL
   AND COL_LENGTH('dbo._HwidList', 'Hwid') IS NOT NULL
   AND COL_LENGTH('dbo._HwidList', 'Active') IS NOT NULL
   AND EXISTS
   (
       SELECT 1
       FROM sys.columns c
       JOIN sys.types t ON c.user_type_id = t.user_type_id
       WHERE c.object_id = OBJECT_ID('dbo._HwidList')
         AND c.name = 'Hwid'
         AND t.name IN ('char', 'varchar', 'nchar', 'nvarchar', 'binary', 'varbinary')
         AND c.max_length <> -1
   )
   AND NOT EXISTS
   (
       SELECT 1
       FROM sys.indexes
       WHERE object_id = OBJECT_ID('dbo._HwidList')
         AND name = 'IX_HwidList_Hwid_Active'
   )
BEGIN
    CREATE INDEX IX_HwidList_Hwid_Active
    ON dbo._HwidList(Hwid, Active);
END;
GO
