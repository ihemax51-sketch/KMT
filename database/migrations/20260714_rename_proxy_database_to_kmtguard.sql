/*
    Rename the proxy/filter database to KMTGuard.

    Run from master with a login that can ALTER DATABASE.
    The script will not overwrite an existing KMTGuard database.
*/
USE [master];
GO

DECLARE @OldDb sysname = N'JT' + N'Guard';
DECLARE @NewDb sysname = N'KMTGuard';
DECLARE @Sql nvarchar(max);

IF DB_ID(@NewDb) IS NOT NULL
BEGIN
    PRINT N'KMTGuard database already exists. No rename was performed.';
END
ELSE IF DB_ID(@OldDb) IS NOT NULL
BEGIN
    SET @Sql = N'ALTER DATABASE ' + QUOTENAME(@OldDb) + N' SET SINGLE_USER WITH ROLLBACK IMMEDIATE;';
    EXEC sys.sp_executesql @Sql;

    SET @Sql = N'ALTER DATABASE ' + QUOTENAME(@OldDb) + N' MODIFY NAME = ' + QUOTENAME(@NewDb) + N';';
    EXEC sys.sp_executesql @Sql;

    SET @Sql = N'ALTER DATABASE ' + QUOTENAME(@NewDb) + N' SET MULTI_USER;';
    EXEC sys.sp_executesql @Sql;

    PRINT N'Renamed proxy database to KMTGuard.';
END
ELSE
BEGIN
    PRINT N'Old proxy database was not found. Create or restore the proxy database as KMTGuard.';
END
GO
