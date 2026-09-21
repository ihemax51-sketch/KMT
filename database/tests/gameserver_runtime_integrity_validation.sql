USE [KMTGuard];
GO
SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.Command_GameServerResult',N'U') IS NULL THROW 51100,'Command_GameServerResult is missing.',1;
IF OBJECT_ID(N'dbo._ServerFortressDpsInfo',N'U') IS NULL THROW 51101,'_ServerFortressDpsInfo is missing.',1;
IF COL_LENGTH(N'dbo._ServerFortressDpsInfo',N'StructObjID') IS NULL OR
   COL_LENGTH(N'dbo._ServerFortressDpsInfo',N'Enabled') IS NULL
    THROW 51108,'_ServerFortressDpsInfo columns are incomplete.',1;
IF NOT EXISTS
(
    SELECT 1 FROM sys.indexes i
    INNER JOIN sys.index_columns ic ON ic.object_id=i.object_id AND ic.index_id=i.index_id
    INNER JOIN sys.columns c ON c.object_id=ic.object_id AND c.column_id=ic.column_id
    WHERE i.object_id=OBJECT_ID(N'dbo._ServerFortressDpsInfo') AND i.is_unique=1
      AND ic.key_ordinal=1 AND c.name=N'StructObjID'
)
    THROW 51109,'_ServerFortressDpsInfo StructObjID is not uniquely indexed.',1;
IF OBJECT_ID(N'dbo.Command_ClaimGameServer',N'P') IS NULL THROW 51102,'Command_ClaimGameServer is missing.',1;
IF OBJECT_ID(N'dbo.Command_CompleteGameServer',N'P') IS NULL THROW 51103,'Command_CompleteGameServer is missing.',1;
IF OBJECT_ID(N'dbo.Command_RetryGameServer',N'P') IS NULL THROW 51104,'Command_RetryGameServer is missing.',1;
IF COL_LENGTH(N'dbo.Command_GameServerQueue',N'AttemptCount') IS NULL OR
   COL_LENGTH(N'dbo.Command_GameServerQueue',N'ClaimToken') IS NULL OR
   COL_LENGTH(N'dbo.Command_GameServerQueue',N'ClaimedAtUtc') IS NULL OR
   COL_LENGTH(N'dbo.Command_GameServerQueue',N'ClaimState') IS NULL OR
   COL_LENGTH(N'dbo.Command_GameServerQueue',N'LastError') IS NULL
    THROW 51105,'GameServer command claim columns are incomplete.',1;
IF EXISTS(SELECT 1 FROM dbo.Command_GameServerQueue WHERE Action_ID=131)
    THROW 51106,'Retired action 131 remains queued.',1;
IF EXISTS(SELECT 1 FROM dbo.Command_GameServerResult WHERE Status NOT IN ('Dispatched','Rejected','Failed','Indeterminate'))
    THROW 51107,'Invalid GameServer command result status exists.',1;

SELECT N'GameServer runtime integrity validation passed.' AS Result;
GO

