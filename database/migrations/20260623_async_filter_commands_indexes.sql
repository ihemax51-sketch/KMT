/*
    Stabilize KMTGuard async command timers.

    These indexes support:
      SELECT TOP (...) * FROM _AsyncFilterCommands WHERE Status = 1 ORDER BY ID
      SELECT TOP (...) * FROM _AsyncFilterCommandsPlanned
        WHERE DateToExecute <= @CurrentTime AND Status = 1
        ORDER BY DateToExecute, ID
*/
SET NOCOUNT ON;

IF OBJECT_ID('dbo._AsyncFilterCommands', 'U') IS NOT NULL
   AND NOT EXISTS
   (
       SELECT 1
       FROM sys.indexes
       WHERE object_id = OBJECT_ID('dbo._AsyncFilterCommands')
         AND name = 'IX_AFC_Status_ID'
   )
BEGIN
    CREATE INDEX IX_AFC_Status_ID
    ON dbo._AsyncFilterCommands(Status, ID);
END;
GO

IF OBJECT_ID('dbo._AsyncFilterCommandsPlanned', 'U') IS NOT NULL
   AND COL_LENGTH('dbo._AsyncFilterCommandsPlanned', 'DateToExecute') IS NOT NULL
   AND NOT EXISTS
   (
       SELECT 1
       FROM sys.indexes
       WHERE object_id = OBJECT_ID('dbo._AsyncFilterCommandsPlanned')
         AND name = 'IX_AFCP_Status_Date_ID'
   )
BEGIN
    CREATE INDEX IX_AFCP_Status_Date_ID
    ON dbo._AsyncFilterCommandsPlanned(Status, DateToExecute, ID);
END;
GO
