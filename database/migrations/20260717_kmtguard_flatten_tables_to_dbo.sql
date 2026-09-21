USE [KMTGuard];
GO

SET XACT_ABORT ON;
GO

BEGIN TRANSACTION;

DECLARE @Tables table (TableName sysname NOT NULL PRIMARY KEY);

INSERT INTO @Tables (TableName)
SELECT t.name
FROM sys.tables t
JOIN sys.schemas s ON t.schema_id = s.schema_id
WHERE s.name = N'KMT';

DECLARE @Synonyms table
(
    SchemaName sysname NOT NULL,
    SynonymName sysname NOT NULL,
    TableName sysname NOT NULL,
    Recreate bit NOT NULL
);

INSERT INTO @Synonyms (SchemaName, SynonymName, TableName, Recreate)
SELECT SCHEMA_NAME(sy.schema_id),
       sy.name,
       t.TableName,
       CASE
           WHEN SCHEMA_NAME(sy.schema_id) = N'dbo' AND sy.name = t.TableName THEN 0
           ELSE 1
       END
FROM sys.synonyms sy
JOIN @Tables t
  ON sy.base_object_name = N'[KMTGuard].[KMT].[' + t.TableName + N']';

DECLARE @sql nvarchar(max) = N'';

SELECT @sql = @sql
    + N'DROP SYNONYM ' + QUOTENAME(SchemaName) + N'.' + QUOTENAME(SynonymName) + N';'
    + CHAR(13) + CHAR(10)
FROM @Synonyms
ORDER BY SchemaName, SynonymName;

EXEC sp_executesql @sql;

IF EXISTS (
    SELECT 1
    FROM @Tables t
    JOIN sys.objects o
      ON o.name = t.TableName
     AND SCHEMA_NAME(o.schema_id) = N'dbo'
)
BEGIN
    THROW 51001, 'Cannot move KMT tables to dbo because a dbo object with the same name exists.', 1;
END;

SET @sql = N'';

SELECT @sql = @sql
    + N'ALTER SCHEMA [dbo] TRANSFER [KMT].' + QUOTENAME(TableName) + N';'
    + CHAR(13) + CHAR(10)
FROM @Tables
ORDER BY TableName;

EXEC sp_executesql @sql;

SET @sql = N'';

SELECT @sql = @sql
    + N'CREATE SYNONYM ' + QUOTENAME(SchemaName) + N'.' + QUOTENAME(SynonymName)
    + N' FOR [KMTGuard].[dbo].' + QUOTENAME(TableName) + N';'
    + CHAR(13) + CHAR(10)
FROM @Synonyms
WHERE Recreate = 1
ORDER BY SchemaName, SynonymName;

EXEC sp_executesql @sql;

COMMIT TRANSACTION;
GO
