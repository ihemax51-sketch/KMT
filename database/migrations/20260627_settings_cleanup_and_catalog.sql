/*
    KMTGuard __Settings cleanup and catalog.

    What this does:
    - Creates a backup table before touching dbo.__Settings.
    - Removes settings that are not used by the current filter/client code.
    - Adds metadata columns so the settings table is understandable in SSMS.
    - Normalizes malformed URL placeholder values.
    - Adds views for organized settings and invalid values.

    Run on the KMTGuard/proxy database.
*/

SET NOCOUNT ON;

IF OBJECT_ID(N'dbo.__Settings', N'U') IS NULL
BEGIN
    RAISERROR('__Settings table was not found.', 16, 1);
    RETURN;
END;

IF OBJECT_ID(N'dbo.__Settings_Backup', N'U') IS NULL
BEGIN
    CREATE TABLE dbo.__Settings_Backup
    (
        BackupID BIGINT IDENTITY(1,1) NOT NULL CONSTRAINT PK___Settings_Backup PRIMARY KEY,
        BackupAt DATETIME2(0) NOT NULL CONSTRAINT DF___Settings_Backup_BackupAt DEFAULT (SYSDATETIME()),
        SourceID INT NULL,
        SettingName VARCHAR(128) NOT NULL,
        Value VARCHAR(512) NOT NULL
    );
END;

INSERT INTO dbo.__Settings_Backup (SourceID, SettingName, Value)
SELECT ID, SettingName, CONVERT(VARCHAR(512), Value)
FROM dbo.__Settings WITH (NOLOCK);

IF COL_LENGTH(N'dbo.__Settings', N'Value') IS NOT NULL
BEGIN
    ALTER TABLE dbo.__Settings ALTER COLUMN Value VARCHAR(512) NOT NULL;
END;

IF COL_LENGTH(N'dbo.__Settings', N'Category') IS NULL
BEGIN
    ALTER TABLE dbo.__Settings ADD Category VARCHAR(64) NULL;
END;

IF COL_LENGTH(N'dbo.__Settings', N'DisplayOrder') IS NULL
BEGIN
    ALTER TABLE dbo.__Settings ADD DisplayOrder INT NULL;
END;

IF COL_LENGTH(N'dbo.__Settings', N'Description') IS NULL
BEGIN
    ALTER TABLE dbo.__Settings ADD Description NVARCHAR(256) NULL;
END;

GO

UPDATE dbo.__Settings
SET
    SettingName = LTRIM(RTRIM(SettingName)),
    Value = LTRIM(RTRIM(Value));

/*
    Removed:
    - AutoStart: not used by the current service startup path.
    - LogDB: not referenced by current filter/client code.
*/
DELETE FROM dbo.__Settings
WHERE SettingName IN (N'AutoStart', N'LogDB');

UPDATE dbo.__Settings
SET Value = N''
WHERE SettingName IN (N'FacebookURL', N'DiscordURL', N'WebsiteURL')
  AND Value IN (N'True', N'False');

UPDATE dbo.__Settings
SET Category = N'Core.Database', DisplayOrder = 10, Description = N'Database names used by the filter.'
WHERE SettingName IN (N'AccountDB', N'ShardDB');

UPDATE dbo.__Settings
SET Category = N'Gateway.Login', DisplayOrder = 20, Description = N'Gateway login, captcha, register and server-list behavior.'
WHERE SettingName IN
(
    N'RemoveCaptcha', N'CaptchaValue', N'SecondaryPassword', N'NewCharInfo', N'NewIdPw',
    N'OldLogin', N'CheckStatus', N'ShowOnlinePlayers', N'ServerName', N'FakePlayerCount'
);

UPDATE dbo.__Settings
SET Category = N'Client.MenuButtons', DisplayOrder = 30, Description = N'Controls visibility/behavior of buttons inside the KMTGuard menu.'
WHERE SettingName IN
(
    N'GrantNameButton', N'IconManagerButton', N'IconManagerRight', N'TitleManager',
    N'TitleManagerColor', N'DynamicRanking', N'UniqueHistory', N'EventRegister',
    N'EventSchedule', N'Achievements', N'Changelog', N'ShowChangelogFirstSpawn'
);

UPDATE dbo.__Settings
SET Category = N'Client.GuideIcons', DisplayOrder = 40, Description = N'Controls visibility of the small guide icons near the minimap.'
WHERE SettingName LIKE N'ShowGuide%';

UPDATE dbo.__Settings
SET Category = N'Client.UI', DisplayOrder = 50, Description = N'Client DLL user-interface patches and visual features.'
WHERE SettingName IN
(
    N'OldExpBar', N'OldAlchemy', N'OldMainPopup', N'HideTitleWhileTagActive',
    N'ItemComparison', N'AutoSort', N'AutoSkillUpdate', N'MasteryLimit',
    N'ServerMaxLevel', N'FixDamageText', N'AutoStrInt', N'PickupEffect',
    N'PermanentAlchemy', N'ShowGuildInJobMode', N'UniqueTarget', N'SecondarySlot',
    N'MoveSkillBoard', N'ServerInfoSkill', N'FixNewJobSuit', N'OldItemMall',
    N'InsertCommaPrices', N'WriteCharacterBound', N'NewItemMall', N'EmojiSystem',
    N'NewPartyMatch', N'NewJobUI', N'NewAlchemy', N'PartyMemberViewer',
    N'NonClosePTForm'
);

UPDATE dbo.__Settings
SET Category = N'Client.Links', DisplayOrder = 60, Description = N'External URLs opened by client social/web buttons. Leave empty if unused.'
WHERE SettingName IN (N'FacebookURL', N'DiscordURL', N'WebsiteURL');

UPDATE dbo.__Settings
SET Category = N'Client.ItemTranslation', DisplayOrder = 70, Description = N'Item translation feature pricing and payment type.'
WHERE SettingName IN (N'EnableItemTranslation', N'ItemTranslationPayment', N'ItemTranslationPrice');

UPDATE dbo.__Settings
SET Category = N'Client.Macro', DisplayOrder = 75, Description = N'Enables the custom SRO Macro client feature and its guide icon.'
WHERE SettingName IN (N'Macro');

UPDATE dbo.__Settings
SET Category = N'LuckySpin', DisplayOrder = 80, Description = N'Lucky Spin enable switch, currency and price.'
WHERE SettingName IN (N'EnableLuckySpin', N'EnableLuckySpinSilk', N'LuckySpinPrice');

UPDATE dbo.__Settings
SET Category = N'SpecialOffers', DisplayOrder = 85, Description = N'Special Offers shop enable switch. Offers are configured in dbo._SpecialOffers.'
WHERE SettingName IN (N'EnableSpecialOffers');

UPDATE dbo.__Settings
SET Category = N'Security.Limits', DisplayOrder = 90, Description = N'IP/HWID limits, job limits, alchemy limits and disabled systems.'
WHERE SettingName IN
(
    N'IPLimit', N'HWID_LIMIT', N'HWID_JOB_LIMIT', N'AlchemyItemLinkMinLevel',
    N'MaxPlus', N'MaxPlusDevil', N'DisableAcademy', N'DisableAutoAttack',
    N'AutoAttackMaxLevel', N'DisableTraceWhileJob', N'DisableReverseInJob'
);

UPDATE dbo.__Settings
SET Category = N'Gameplay.Delays', DisplayOrder = 100, Description = N'Cooldowns and minimum levels for player actions.'
WHERE SettingName IN
(
    N'ReverseDelay', N'StallDelay', N'StallLevel', N'ExchangeDelay',
    N'ExchangeLevel', N'GuildInviteDelay', N'UnionInviteDelay', N'GlobalDelay',
    N'GlobalLevel', N'LiveItemDelay', N'TradePetSpawnDelay', N'RestartDelay',
    N'ExitDelay', N'SHOW_CHAR_INFO_DELAY'
);

UPDATE dbo.__Settings
SET Category = N'Trade.Captcha', DisplayOrder = 110, Description = N'Trade goods selling verification captcha.'
WHERE SettingName IN
(
    N'EnableTradeSellCaptcha', N'TradeSellCaptchaTimeoutSeconds', N'TradeSellCaptchaMaxAttempts'
);

UPDATE dbo.__Settings
SET Category = N'Unsorted', DisplayOrder = 999, Description = N'Known setting that still needs a better category.'
WHERE Category IS NULL;

IF NOT EXISTS
(
    SELECT 1
    FROM sys.indexes
    WHERE object_id = OBJECT_ID(N'dbo.__Settings')
      AND name = N'UX___Settings_SettingName'
)
BEGIN
    CREATE UNIQUE NONCLUSTERED INDEX UX___Settings_SettingName
        ON dbo.__Settings(SettingName);
END;

IF OBJECT_ID(N'dbo.vw_Settings_Organized', N'V') IS NOT NULL
BEGIN
    DROP VIEW dbo.vw_Settings_Organized;
END;

EXEC(N'
CREATE VIEW dbo.vw_Settings_Organized
AS
SELECT
    Category,
    DisplayOrder,
    SettingName,
    Value,
    Description
FROM dbo.__Settings;
');

IF OBJECT_ID(N'dbo.vw_Settings_InvalidValues', N'V') IS NOT NULL
BEGIN
    DROP VIEW dbo.vw_Settings_InvalidValues;
END;

EXEC(N'
CREATE VIEW dbo.vw_Settings_InvalidValues
AS
WITH typed AS
(
    SELECT
        SettingName,
        Value,
        CASE
            WHEN SettingName IN
            (
                ''AccountDB'', ''ShardDB'', ''FacebookURL'', ''DiscordURL'', ''WebsiteURL'', ''ServerName'', ''CaptchaValue''
            ) THEN ''string''
            WHEN SettingName IN
            (
                ''MasteryLimit'', ''ServerMaxLevel'', ''FakePlayerCount'', ''HWID_LIMIT'', ''HWID_JOB_LIMIT'',
                ''AlchemyItemLinkMinLevel'', ''ReverseDelay'', ''MaxPlus'', ''AutoAttackMaxLevel'', ''StallDelay'',
                ''StallLevel'', ''ExchangeDelay'', ''ExchangeLevel'', ''GuildInviteDelay'', ''UnionInviteDelay'',
                ''GlobalDelay'', ''GlobalLevel'', ''LiveItemDelay'', ''TradePetSpawnDelay'', ''RestartDelay'',
                ''ExitDelay'', ''ItemTranslationPayment'', ''ItemTranslationPrice'', ''SHOW_CHAR_INFO_DELAY'',
                ''IPLimit'', ''MaxPlusDevil'', ''LuckySpinPrice'', ''TradeSellCaptchaTimeoutSeconds'',
                ''TradeSellCaptchaMaxAttempts''
            ) THEN ''int''
            ELSE ''bool''
        END AS ExpectedType
    FROM dbo.__Settings
)
SELECT SettingName, Value, ExpectedType
FROM typed
WHERE
    (ExpectedType = ''bool'' AND Value NOT IN (''True'', ''False''))
    OR (ExpectedType = ''int'' AND TRY_CONVERT(INT, Value) IS NULL);
');

PRINT 'KMTGuard settings cleanup and catalog completed.';
