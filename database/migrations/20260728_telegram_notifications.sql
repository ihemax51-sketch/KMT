/*
    Telegram notification delivery for KMTGuard.

    Run on the KMTGuard/proxy database. The Filter and Admin Desktop also apply
    this schema idempotently for compatible existing installations.
*/

SET NOCOUNT ON;
SET XACT_ABORT ON;

IF OBJECT_ID(N'dbo.TelegramNotificationSettings', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.TelegramNotificationSettings
    (
        SettingID TINYINT NOT NULL CONSTRAINT PK_TelegramNotificationSettings PRIMARY KEY,
        Enabled BIT NOT NULL CONSTRAINT DF_TelegramSettings_Enabled DEFAULT (0),
        BotTokenProtected NVARCHAR(2048) NULL,
        ChannelId NVARCHAR(128) NULL,
        UniqueSpawnEnabled BIT NOT NULL CONSTRAINT DF_TelegramSettings_UniqueSpawn DEFAULT (1),
        UniqueKillEnabled BIT NOT NULL CONSTRAINT DF_TelegramSettings_UniqueKill DEFAULT (1),
        ShowKillerName BIT NOT NULL CONSTRAINT DF_TelegramSettings_ShowKiller DEFAULT (1),
        EventReminderEnabled BIT NOT NULL CONSTRAINT DF_TelegramSettings_EventReminder DEFAULT (1),
        ReminderMinutes NVARCHAR(128) NOT NULL CONSTRAINT DF_TelegramSettings_Reminders DEFAULT (N'15,5'),
        EventStartedEnabled BIT NOT NULL CONSTRAINT DF_TelegramSettings_EventStarted DEFAULT (1),
        EventFinishedEnabled BIT NOT NULL CONSTRAINT DF_TelegramSettings_EventFinished DEFAULT (1),
        ServerOnlineEnabled BIT NOT NULL CONSTRAINT DF_TelegramSettings_ServerOnline DEFAULT (0),
        FortressWarEnabled BIT NOT NULL CONSTRAINT DF_TelegramSettings_Fortress DEFAULT (1),
        UpdatedAtUtc DATETIME2(0) NOT NULL CONSTRAINT DF_TelegramSettings_Updated DEFAULT (SYSUTCDATETIME()),
        CONSTRAINT CK_TelegramSettings_SingleRow CHECK (SettingID = 1)
    );
END;

IF NOT EXISTS (SELECT 1 FROM dbo.TelegramNotificationSettings WHERE SettingID = 1)
    INSERT dbo.TelegramNotificationSettings (SettingID) VALUES (1);

IF OBJECT_ID(N'dbo.TelegramNotificationQueue', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.TelegramNotificationQueue
    (
        NotificationID BIGINT IDENTITY(1,1) NOT NULL CONSTRAINT PK_TelegramNotificationQueue PRIMARY KEY,
        EventKey NVARCHAR(180) NOT NULL,
        Category NVARCHAR(32) NOT NULL,
        MessageHtml NVARCHAR(4000) NOT NULL,
        Status TINYINT NOT NULL CONSTRAINT DF_TelegramQueue_Status DEFAULT (0),
        Attempts INT NOT NULL CONSTRAINT DF_TelegramQueue_Attempts DEFAULT (0),
        NextAttemptUtc DATETIME2(0) NOT NULL CONSTRAINT DF_TelegramQueue_NextAttempt DEFAULT (SYSUTCDATETIME()),
        ProcessingAtUtc DATETIME2(0) NULL,
        CreatedAtUtc DATETIME2(0) NOT NULL CONSTRAINT DF_TelegramQueue_Created DEFAULT (SYSUTCDATETIME()),
        SentAtUtc DATETIME2(0) NULL,
        LastError NVARCHAR(1000) NULL,
        CONSTRAINT CK_TelegramQueue_Status CHECK (Status BETWEEN 0 AND 3),
        CONSTRAINT CK_TelegramQueue_Attempts CHECK (Attempts >= 0)
    );

    CREATE UNIQUE INDEX UX_TelegramNotificationQueue_EventKey
        ON dbo.TelegramNotificationQueue(EventKey);

    CREATE INDEX IX_TelegramNotificationQueue_Delivery
        ON dbo.TelegramNotificationQueue(Status, NextAttemptUtc, NotificationID)
        INCLUDE (Attempts, Category);
END;

IF OBJECT_ID(N'dbo.TelegramUniqueDisplayNames', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.TelegramUniqueDisplayNames
    (
        MobID INT NOT NULL CONSTRAINT PK_TelegramUniqueDisplayNames PRIMARY KEY,
        CodeName128 NVARCHAR(128) NULL,
        NameStrID128 NVARCHAR(128) NULL,
        DisplayName NVARCHAR(128) NULL,
        IsCustom BIT NOT NULL CONSTRAINT DF_TelegramUniqueNames_Custom DEFAULT (0),
        UpdatedAtUtc DATETIME2(0) NOT NULL CONSTRAINT DF_TelegramUniqueNames_Updated DEFAULT (SYSUTCDATETIME())
    );
END;

GO

CREATE OR ALTER PROCEDURE dbo.Telegram_Notification
    @Message NVARCHAR(4000),
    @Category NVARCHAR(32) = N'Announcement',
    @EventKey NVARCHAR(180) = NULL
AS
BEGIN
    SET NOCOUNT ON;
    SET XACT_ABORT ON;

    SET @Message = NULLIF(LTRIM(RTRIM(@Message)), N'');
    SET @Category = COALESCE(NULLIF(LTRIM(RTRIM(@Category)), N''), N'Announcement');
    SET @EventKey = NULLIF(LTRIM(RTRIM(@EventKey)), N'');

    IF @Message IS NULL
        THROW 51001, 'Telegram notification message is required.', 1;

    IF LEN(@Message) > 4000
        THROW 51002, 'Telegram notification message cannot exceed 4000 characters.', 1;

    IF @EventKey IS NULL
        SET @EventKey = CONCAT(N'telegram-procedure:', CONVERT(nvarchar(36), NEWID()));

    IF EXISTS (SELECT 1 FROM dbo.TelegramNotificationQueue WITH (UPDLOCK, HOLDLOCK) WHERE EventKey = @EventKey)
    BEGIN
        SELECT NotificationID
        FROM dbo.TelegramNotificationQueue
        WHERE EventKey = @EventKey;
        RETURN;
    END;

    INSERT dbo.TelegramNotificationQueue
        (EventKey, Category, MessageHtml, Status, Attempts, NextAttemptUtc, CreatedAtUtc)
    VALUES
        (@EventKey, LEFT(@Category, 32), @Message, 0, 0, SYSUTCDATETIME(), SYSUTCDATETIME());

    SELECT CONVERT(bigint, SCOPE_IDENTITY()) AS NotificationID;
END;
