USE [KMTGuard];
GO

SET XACT_ABORT ON;
GO

BEGIN TRANSACTION;

DECLARE @Procedures table (ProcName sysname NOT NULL PRIMARY KEY);

INSERT INTO @Procedures (ProcName)
SELECT p.name
FROM sys.procedures p
JOIN sys.schemas s ON p.schema_id = s.schema_id
WHERE s.name = N'KMT';

DECLARE @Synonyms table
(
    SchemaName sysname NOT NULL,
    SynonymName sysname NOT NULL,
    ProcName sysname NOT NULL,
    Recreate bit NOT NULL
);

INSERT INTO @Synonyms (SchemaName, SynonymName, ProcName, Recreate)
SELECT SCHEMA_NAME(sy.schema_id),
       sy.name,
       p.ProcName,
       CASE
           WHEN SCHEMA_NAME(sy.schema_id) = N'dbo' AND sy.name = p.ProcName THEN 0
           ELSE 1
       END
FROM sys.synonyms sy
JOIN @Procedures p
  ON sy.base_object_name = N'[KMTGuard].[KMT].[' + p.ProcName + N']';

DECLARE @sql nvarchar(max) = N'';

SELECT @sql = @sql
    + N'DROP SYNONYM ' + QUOTENAME(SchemaName) + N'.' + QUOTENAME(SynonymName) + N';'
    + CHAR(13) + CHAR(10)
FROM @Synonyms
ORDER BY SchemaName, SynonymName;

EXEC sp_executesql @sql;

IF EXISTS (
    SELECT 1
    FROM @Procedures p
    JOIN sys.objects o
      ON o.name = p.ProcName
     AND SCHEMA_NAME(o.schema_id) = N'dbo'
)
BEGIN
    THROW 51000, 'Cannot move KMT procedures to dbo because a dbo object with the same name exists.', 1;
END;

SET @sql = N'';

SELECT @sql = @sql
    + N'ALTER SCHEMA [dbo] TRANSFER [KMT].' + QUOTENAME(ProcName) + N';'
    + CHAR(13) + CHAR(10)
FROM @Procedures
ORDER BY ProcName;

EXEC sp_executesql @sql;

SET @sql = N'';

SELECT @sql = @sql
    + N'CREATE SYNONYM ' + QUOTENAME(SchemaName) + N'.' + QUOTENAME(SynonymName)
    + N' FOR [KMTGuard].[dbo].' + QUOTENAME(ProcName) + N';'
    + CHAR(13) + CHAR(10)
FROM @Synonyms
WHERE Recreate = 1
ORDER BY SchemaName, SynonymName;

EXEC sp_executesql @sql;

COMMIT TRANSACTION;
GO
