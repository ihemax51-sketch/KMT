/*
    Non-destructive Discord notification schema validation.

    Run after 20260729_discord_notifications.sql. All validation rows are
    created inside a transaction and rolled back.
*/

SET NOCOUNT ON;
SET XACT_ABORT ON;

IF OBJECT_ID(N'dbo.DiscordNotificationSettings', N'U') IS NULL
    THROW 51201, 'DiscordNotificationSettings is missing.', 1;
IF OBJECT_ID(N'dbo.DiscordNotificationChannels', N'U') IS NULL
    THROW 51202, 'DiscordNotificationChannels is missing.', 1;
IF OBJECT_ID(N'dbo.DiscordNotificationQueue', N'U') IS NULL
    THROW 51203, 'DiscordNotificationQueue is missing.', 1;
IF OBJECT_ID(N'dbo.Discord_AddChannel', N'P') IS NULL
    THROW 51204, 'Discord_AddChannel is missing.', 1;
IF OBJECT_ID(N'dbo.Discord_Notification', N'P') IS NULL
    THROW 51205, 'Discord_Notification is missing.', 1;

BEGIN TRANSACTION;

BEGIN TRY
    DECLARE @ValidationID varchar(32) = REPLACE(CONVERT(varchar(36), NEWID()), '-', '');
    DECLARE @ChannelName nvarchar(80) = CONCAT(N'__KMT_DISCORD_VALIDATION_', @ValidationID);
    DECLARE @ChannelID varchar(32) =
        CONCAT('99999999', RIGHT('0000000000' + CONVERT(varchar(10), ABS(CONVERT(bigint, CHECKSUM(NEWID())))), 10));
    DECLARE @EventKey nvarchar(180) = CONCAT(N'validation:discord:', @ValidationID);

    EXEC dbo.Discord_AddChannel
        @ChannelName = @ChannelName,
        @DiscordChannelID = @ChannelID,
        @Enabled = 1,
        @SortOrder = 999999;

    EXEC dbo.Discord_Notification
        @ChannelName = @ChannelName,
        @Message = N'KMTGuard Discord queue validation.',
        @EventKey = @EventKey;

    EXEC dbo.Discord_Notification
        @ChannelName = @ChannelName,
        @Message = N'KMTGuard Discord queue validation.',
        @EventKey = @EventKey;

    IF (SELECT COUNT(*) FROM dbo.DiscordNotificationQueue WHERE EventKey = @EventKey) <> 1
        THROW 51206, 'Discord duplicate-event protection failed.', 1;

    INSERT dbo.DiscordNotificationQueue (ChannelName, MessageText)
    VALUES (@ChannelName, N'KMTGuard direct-insert validation.');

    IF NOT EXISTS
    (
        SELECT 1
        FROM dbo.DiscordNotificationQueue
        WHERE ChannelName = @ChannelName
          AND MessageText = N'KMTGuard direct-insert validation.'
          AND Status = 0
          AND Attempts = 0
    )
        THROW 51207, 'Discord direct-insert defaults failed.', 1;

    ROLLBACK TRANSACTION;
    SELECT N'Discord notification schema validation passed.' AS Result;
END TRY
BEGIN CATCH
    IF @@TRANCOUNT > 0
        ROLLBACK TRANSACTION;
    THROW;
END CATCH;
