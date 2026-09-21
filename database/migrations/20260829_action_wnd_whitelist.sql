/*
    Allows the custom ActionWnd request packet through the AgentServer client
    packet whitelist.  The opcode is sent only by the KMTGuard Client DLL and
    is handled by dbo._OnActionWndCommandPet through CustomUIPackets.
*/

USE [KMTGuard];
GO

SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.Security_Whitelist', N'U') IS NULL
    THROW 51630, 'dbo.Security_Whitelist is required.', 1;
GO

IF NOT EXISTS
(
    SELECT 1
    FROM dbo.Security_Whitelist
    WHERE ServerType = 3
      AND MsgId = 52254 -- 0xCC1E
)
BEGIN
    INSERT dbo.Security_Whitelist (MsgId, ServerType, Comment)
    VALUES (52254, 3, N'KMTGuard ActionWnd custom command request');
END;
GO

PRINT 'ActionWnd custom command packet is whitelisted for AgentServer.';
GO
