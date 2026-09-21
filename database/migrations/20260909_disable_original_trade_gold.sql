/*
    Adds the startup-cached GameServer switch that disables only the original
    Silkroad trade-goods gold payout.

    System_GameServerSettings uses the existing varchar value contract for all
    setting types. The logical BIT default is stored as 0 (OFF).
*/

USE [KMTGuard];
GO

SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.System_GameServerSettings', N'U') IS NULL
    THROW 51650, 'dbo.System_GameServerSettings was not found.', 1;

IF COL_LENGTH(N'dbo.System_GameServerSettings', N'ID') IS NULL OR
   COL_LENGTH(N'dbo.System_GameServerSettings', N'SettingName') IS NULL OR
   COL_LENGTH(N'dbo.System_GameServerSettings', N'Value') IS NULL
    THROW 51651, 'dbo.System_GameServerSettings has an incompatible schema.', 1;
GO

BEGIN TRY
    BEGIN TRANSACTION;

    IF
    (
        SELECT COUNT_BIG(*)
        FROM dbo.System_GameServerSettings WITH (UPDLOCK, HOLDLOCK)
        WHERE SettingName = 'DisableOriginalTradeGold'
    ) > 1
        THROW 51652, 'DisableOriginalTradeGold is duplicated in dbo.System_GameServerSettings.', 1;

    IF EXISTS
    (
        SELECT 1
        FROM dbo.System_GameServerSettings WITH (UPDLOCK, HOLDLOCK)
        WHERE SettingName = 'DisableOriginalTradeGold'
          AND LOWER(Value) NOT IN ('true', 'false', '1', '0')
    )
        THROW 51653, 'DisableOriginalTradeGold contains an invalid boolean value.', 1;

    IF NOT EXISTS
    (
        SELECT 1
        FROM dbo.System_GameServerSettings WITH (UPDLOCK, HOLDLOCK)
        WHERE SettingName = 'DisableOriginalTradeGold'
    )
    BEGIN
        IF COLUMNPROPERTY(OBJECT_ID(N'dbo.System_GameServerSettings'), N'ID', 'IsIdentity') = 1
        BEGIN
            INSERT dbo.System_GameServerSettings(SettingName, Value)
            VALUES ('DisableOriginalTradeGold', '0');
        END
        ELSE
        BEGIN
            DECLARE @NextSettingID int =
                ISNULL((SELECT MAX(ID) FROM dbo.System_GameServerSettings WITH (UPDLOCK, HOLDLOCK)), 0) + 1;

            INSERT dbo.System_GameServerSettings(ID, SettingName, Value)
            VALUES (@NextSettingID, 'DisableOriginalTradeGold', '0');
        END;
    END;

    COMMIT TRANSACTION;
END TRY
BEGIN CATCH
    IF XACT_STATE() <> 0
        ROLLBACK TRANSACTION;
    THROW;
END CATCH;
GO

PRINT 'DisableOriginalTradeGold was added with logical BIT default 0 (OFF).';
GO
