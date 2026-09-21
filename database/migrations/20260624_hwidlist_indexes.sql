/*
    Stabilize HWID tracking under production load.

    These indexes are intentionally non-destructive and conditional. They support:
      - UPDATE _HwidList SET Active = 0 WHERE CharID = @CharID
      - HWID active-session lookups commonly used by _HandleHwidList
*/
SET NOCOUNT ON;

IF OBJECT_ID('dbo._HwidList', 'U') IS NOT NULL
   AND COL_LENGTH('dbo._HwidList', 'CharID') IS NOT NULL
   AND NOT EXISTS
   (
       SELECT 1
       FROM sys.indexes
       WHERE object_id = OBJECT_ID('dbo._HwidList')
         AND name = 'IX_HwidList_CharID'
   )
BEGIN
    CREATE INDEX IX_HwidList_CharID
    ON dbo._HwidList(CharID);
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

IF OBJECT_ID('dbo._HwidList', 'U') IS NOT NULL
   AND COL_LENGTH('dbo._HwidList', 'Hwid') IS NOT NULL
   AND COL_LENGTH('dbo._HwidList', 'Active') IS NOT NULL
   AND NOT EXISTS
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
         AND name = 'IX_HwidList_Active'
   )
BEGIN
    CREATE INDEX IX_HwidList_Active
    ON dbo._HwidList(Active);
END;
GO
