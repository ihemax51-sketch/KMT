/*
    Discord bot notifications for KMTGuard.

    Run on the KMTGuard/proxy database. The Filter and Admin Desktop also apply
    this schema idempotently for compatible existing installations.
*/

SET NOCOUNT ON;
SET XACT_ABORT ON;

IF OBJECT_ID(N'dbo.DiscordNotificationSettings', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.DiscordNotificationSettings
    (
        SettingID TINYINT NOT NULL CONSTRAINT PK_DiscordNotificationSettings PRIMARY KEY,
        Enabled BIT NOT NULL CONSTRAINT DF_DiscordSettings_Enabled DEFAULT (0),
        BotTokenProtected NVARCHAR(2048) NULL,
        UpdatedAtUtc DATETIME2(0) NOT NULL CONSTRAINT DF_DiscordSettings_Updated DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT CK_DiscordSettings_SingleRow CHECK (SettingID = 1)
    );
END;

IF NOT EXISTS (SELECT 1 FROM dbo.DiscordNotificationSettings WHERE SettingID = 1)
    INSERT dbo.DiscordNotificationSettings (SettingID) VALUES (1);

IF OBJECT_ID(N'dbo.DiscordNotificationChannels', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.DiscordNotificationChannels
    (
        ChannelRecordID INT IDENTITY(1,1) NOT NULL CONSTRAINT PK_DiscordNotificationChannels PRIMARY KEY,
        ChannelName NVARCHAR(80) NOT NULL,
        DiscordChannelID VARCHAR(32) NOT NULL,
        Enabled BIT NOT NULL CONSTRAINT DF_DiscordChannels_Enabled DEFAULT (1),
        SortOrder INT NOT NULL CONSTRAINT DF_DiscordChannels_SortOrder DEFAULT (0),
        CreatedAtUtc DATETIME2(0) NOT NULL CONSTRAINT DF_DiscordChannels_Created DEFAULT (SYSUTCDATETIME()),
        UpdatedAtUtc DATETIME2(0) NOT NULL CONSTRAINT DF_DiscordChannels_Updated DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT UQ_DiscordChannels_Name UNIQUE (ChannelName),
        CONSTRAINT UQ_DiscordChannels_DiscordID UNIQUE (DiscordChannelID),
        CONSTRAINT CK_DiscordChannels_Name CHECK (LEN(LTRIM(RTRIM(ChannelName))) BETWEEN 2 AND 80),
        CONSTRAINT CK_DiscordChannels_ID CHECK
        (
            LEN(DiscordChannelID) BETWEEN 17 AND 20
            AND DiscordChannelID NOT LIKE '%[^0-9]%'
        )
    );
END;

IF OBJECT_ID(N'dbo.DiscordNotificationQueue', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.DiscordNotificationQueue
    (
        NotificationID BIGINT IDENTITY(1,1) NOT NULL CONSTRAINT PK_DiscordNotificationQueue PRIMARY KEY,
        EventKey NVARCHAR(180) NOT NULL
            CONSTRAINT DF_DiscordQueue_EventKey DEFAULT (CONVERT(nvarchar(36), NEWID())),
        ChannelName NVARCHAR(80) NOT NULL,
        MessageText NVARCHAR(2000) NOT NULL,
        Status TINYINT NOT NULL CONSTRAINT DF_DiscordQueue_Status DEFAULT (0),
        Attempts INT NOT NULL CONSTRAINT DF_DiscordQueue_Attempts DEFAULT (0),
        NextAttemptUtc DATETIME2(0) NOT NULL CONSTRAINT DF_DiscordQueue_NextAttempt DEFAULT (SYSUTCDATETIME()),
        ProcessingAtUtc DATETIME2(0) NULL,
        CreatedAtUtc DATETIME2(0) NOT NULL CONSTRAINT DF_DiscordQueue_Created DEFAULT (SYSUTCDATETIME()),
        SentAtUtc DATETIME2(0) NULL,
        LastError NVARCHAR(1000) NULL,
        CONSTRAINT CK_DiscordQueue_Status CHECK (Status BETWEEN 0 AND 3),
        CONSTRAINT CK_DiscordQueue_Attempts CHECK (Attempts >= 0),
        CONSTRAINT CK_DiscordQueue_Message CHECK (LEN(LTRIM(RTRIM(MessageText))) BETWEEN 1 AND 2000)
    );

    CREATE UNIQUE INDEX UX_DiscordNotificationQueue_EventKey
        ON dbo.DiscordNotificationQueue(EventKey);

    CREATE INDEX IX_DiscordNotificationQueue_Delivery
        ON dbo.DiscordNotificationQueue(Status, NextAttemptUtc, NotificationID)
        INCLUDE (Attempts, ChannelName);
END;

GO

CREATE OR ALTER PROCEDURE dbo.Discord_AddChannel
    @ChannelName NVARCHAR(80),
    @DiscordChannelID VARCHAR(32),
    @Enabled BIT = 1,
    @SortOrder INT = 0
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    SET @ChannelName = NULLIF(LTRIM(RTRIM(@ChannelName)), N'');
    SET @DiscordChannelID = NULLIF(LTRIM(RTRIM(@DiscordChannelID)), '');

    IF @ChannelName IS NULL OR LEN(@ChannelName) < 2
        THROW 51101, 'Discord channel name must contain at least 2 characters.', 1;

    IF @DiscordChannelID IS NULL OR LEN(@DiscordChannelID) NOT BETWEEN 17 AND 20
       OR @DiscordChannelID LIKE '%[^0-9]%'
        THROW 51102, 'Discord Channel ID must contain 17 to 20 digits.', 1;

    IF EXISTS
    (
        SELECT 1
        FROM dbo.DiscordNotificationChannels
        WHERE DiscordChannelID = @DiscordChannelID
          AND ChannelName <> @ChannelName
    )
        THROW 51103, 'This Discord Channel ID is already assigned to another channel name.', 1;

    MERGE dbo.DiscordNotificationChannels WITH (HOLDLOCK) AS target
    USING (SELECT @ChannelName AS ChannelName) AS source
       ON target.ChannelName = source.ChannelName
    WHEN MATCHED THEN UPDATE SET
        DiscordChannelID = @DiscordChannelID,
        Enabled = @Enabled,
        SortOrder = @SortOrder,
        UpdatedAtUtc = SYSUTCDATETIME()
    WHEN NOT MATCHED THEN INSERT
        (ChannelName, DiscordChannelID, Enabled, SortOrder, CreatedAtUtc, UpdatedAtUtc)
    VALUES
        (@ChannelName, @DiscordChannelID, @Enabled, @SortOrder, SYSUTCDATETIME(), SYSUTCDATETIME());

    SELECT ChannelRecordID, ChannelName, DiscordChannelID, Enabled, SortOrder, UpdatedAtUtc
    FROM dbo.DiscordNotificationChannels
    WHERE ChannelName = @ChannelName;
END;

GO

CREATE OR ALTER PROCEDURE dbo.Discord_Notification
    @ChannelName NVARCHAR(80),
    @Message NVARCHAR(2000),
    @EventKey NVARCHAR(180) = NULL
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    SET @ChannelName = NULLIF(LTRIM(RTRIM(@ChannelName)), N'');
    SET @Message = NULLIF(LTRIM(RTRIM(@Message)), N'');
    SET @EventKey = NULLIF(LTRIM(RTRIM(@EventKey)), N'');

    IF @ChannelName IS NULL
        THROW 51111, 'Discord channel name is required.', 1;

    IF NOT EXISTS
    (
        SELECT 1
        FROM dbo.DiscordNotificationChannels WITH (UPDLOCK, HOLDLOCK)
        WHERE ChannelName = @ChannelName AND Enabled = 1
    )
        THROW 51112, 'The requested Discord notification channel does not exist or is disabled.', 1;

    IF @Message IS NULL
        THROW 51113, 'Discord notification message is required.', 1;

    IF LEN(@Message) > 2000
        THROW 51114, 'Discord notification message cannot exceed 2000 characters.', 1;

    IF @EventKey IS NULL
        SET @EventKey = CONCAT(N'discord-procedure:', CONVERT(nvarchar(36), NEWID()));

    IF EXISTS
    (
        SELECT 1
        FROM dbo.DiscordNotificationQueue WITH (UPDLOCK, HOLDLOCK)
        WHERE EventKey = @EventKey
    )
    BEGIN
        SELECT NotificationID
        FROM dbo.DiscordNotificationQueue
        WHERE EventKey = @EventKey;
        RETURN;
    END;

    INSERT dbo.DiscordNotificationQueue
        (EventKey, ChannelName, MessageText, Status, Attempts, NextAttemptUtc, CreatedAtUtc)
    VALUES
        (@EventKey, @ChannelName, @Message, 0, 0, SYSUTCDATETIME(), SYSUTCDATETIME());

    SELECT CONVERT(bigint, SCOPE_IDENTITY()) AS NotificationID;
END;
