/*
    Keep settings diagnostics aligned with KMTGuard.Database.Models._serverSettings.

    This migration only replaces dbo.vw_Settings_InvalidValues. It does not insert,
    update, normalize, or delete any customer setting value.

    Unknown setting names are reported as "unknown" instead of being assumed to be
    boolean. This prevents newly introduced numeric or string settings from producing
    misleading boolean validation errors.
*/

USE [KMTGuard];
GO

SET NOCOUNT ON;
SET XACT_ABORT ON;
GO

IF OBJECT_ID(N'dbo.System_Settings', N'U') IS NULL
    THROW 51000, 'KMTGuard.dbo.System_Settings was not found.', 1;
GO

CREATE OR ALTER VIEW dbo.vw_Settings_InvalidValues
AS
WITH setting_type_catalog AS
(
    SELECT SettingName, ExpectedType
    FROM
    (
        VALUES
            (N'AutoStart', N'bool'),
            (N'AccountDB', N'string'),
            (N'LogDB', N'string'),
            (N'ShardDB', N'string'),
            (N'RemoveCaptcha', N'bool'),
            (N'CaptchaValue', N'string'),
            (N'OldLogin', N'bool'),
            (N'OldExpBar', N'bool'),
            (N'OldAlchemy', N'bool'),
            (N'GrantNameButton', N'bool'),
            (N'IconManagerButton', N'bool'),
            (N'IconManagerRight', N'bool'),
            (N'TitleManager', N'bool'),
            (N'TitleManagerColor', N'bool'),
            (N'DynamicRanking', N'bool'),
            (N'DynamicRankingRefreshMinutes', N'int'),
            (N'UniqueHistory', N'bool'),
            (N'EventRegister', N'bool'),
            (N'EventSchedule', N'bool'),
            (N'Achievements', N'bool'),
            (N'SecondarySlot', N'bool'),
            (N'MoveSkillBoard', N'bool'),
            (N'ServerInfoSkill', N'bool'),
            (N'OldMainPopup', N'bool'),
            (N'HideTitleWhileTagActive', N'bool'),
            (N'ItemComparison', N'bool'),
            (N'AutoSort', N'bool'),
            (N'PartyMemberViewer', N'bool'),
            (N'AutoSkillUpdate', N'bool'),
            (N'MasteryLimit', N'int'),
            (N'ServerMaxLevel', N'int'),
            (N'FixDamageText', N'bool'),
            (N'AutoStrInt', N'bool'),
            (N'PickupEffect', N'bool'),
            (N'PermanentAlchemy', N'bool'),
            (N'ShowGuildInJobMode', N'bool'),
            (N'UniqueTarget', N'bool'),
            (N'Macro', N'bool'),
            (N'SecondaryPassword', N'bool'),
            (N'NewCharInfo', N'bool'),
            (N'NewIdPw', N'bool'),
            (N'EnableQuickLogin', N'bool'),
            (N'FacebookURL', N'string'),
            (N'DiscordURL', N'string'),
            (N'WebsiteURL', N'string'),
            (N'Changelog', N'bool'),
            (N'ShowChangelogFirstSpawn', N'bool'),
            (N'FixNewJobSuit', N'bool'),
            (N'OldItemMall', N'bool'),
            (N'InsertCommaPrices', N'bool'),
            (N'WriteCharacterBound', N'bool'),
            (N'NewItemMall', N'bool'),
            (N'EmojiSystem', N'bool'),
            (N'NewPartyMatch', N'bool'),
            (N'NewJobUI', N'bool'),
            (N'NewAlchemy', N'bool'),
            (N'ShowOnlinePlayers', N'bool'),
            (N'ServerName', N'string'),
            (N'FakePlayerCount', N'int'),
            (N'CheckStatus', N'bool'),
            (N'HWID_LIMIT', N'int'),
            (N'HWID_JOB_LIMIT', N'int'),
            (N'AlchemyItemLinkMinLevel', N'int'),
            (N'DisableReverseInJob', N'bool'),
            (N'ReverseDelay', N'int'),
            (N'MaxPlus', N'int'),
            (N'DisableAcademy', N'bool'),
            (N'DisableAutoAttack', N'bool'),
            (N'AutoAttackMaxLevel', N'int'),
            (N'DisableTraceWhileJob', N'bool'),
            (N'StallDelay', N'int'),
            (N'StallLevel', N'int'),
            (N'ExchangeDelay', N'int'),
            (N'ExchangeLevel', N'int'),
            (N'GuildInviteDelay', N'int'),
            (N'UnionInviteDelay', N'int'),
            (N'GlobalDelay', N'int'),
            (N'GlobalLevel', N'int'),
            (N'LiveItemDelay', N'int'),
            (N'TradePetSpawnDelay', N'int'),
            (N'RestartDelay', N'int'),
            (N'ExitDelay', N'int'),
            (N'EnableItemTranslation', N'bool'),
            (N'ItemTranslationPayment', N'int'),
            (N'ItemTranslationPrice', N'int'),
            (N'SHOW_CHAR_INFO_DELAY', N'int'),
            (N'NonClosePTForm', N'bool'),
            (N'EnableLuckySpin', N'bool'),
            (N'EnableLuckySpinSilk', N'bool'),
            (N'LuckySpinPrice', N'int'),
            (N'ShowGuideMenu', N'bool'),
            (N'ShowGuideLuckySpin', N'bool'),
            (N'ShowGuideItemChest', N'bool'),
            (N'ShowGuideDropLogs', N'bool'),
            (N'ShowGuideMacro', N'bool'),
            (N'ShowGuideDailyLogin', N'bool'),
            (N'ShowGuideDiscord', N'bool'),
            (N'ShowGuideWebsite', N'bool'),
            (N'ShowGuideFacebook', N'bool'),
            (N'ShowGuideAutoEquip', N'bool'),
            (N'AutoEquipMaxLevel', N'int'),
            (N'ShowGuideWebViewer', N'bool'),
            (N'ShowGuideMapLocation', N'bool'),
            (N'EnableSpecialOffers', N'bool'),
            (N'ShowGuideSpecialOffers', N'bool'),
            (N'ShowGuideKillerAnimation', N'bool'),
            (N'NewInventoryDesign', N'bool'),
            (N'EnableOfflineStall', N'bool'),
            (N'Menu-like-maxi', N'bool'),
            (N'MenuLikeMaxi', N'bool'),
            (N'OfflineStallMaxHours', N'int'),
            (N'OfflineStallConfirmSeconds', N'int'),
            (N'EnablePvpChallenge', N'bool'),
            (N'ShowGuidePvpChallenge', N'bool'),
            (N'EnableTradeSellCaptcha', N'bool'),
            (N'TradeSellCaptchaTimeoutSeconds', N'int'),
            (N'TradeSellCaptchaMaxAttempts', N'int'),
            (N'AllowBotLogin', N'bool'),
            (N'AllowBotTrade', N'bool'),
            (N'BotProtectionLogEnabled', N'bool'),
            (N'IPLimit', N'int'),
            (N'MaxPlusDevil', N'int')
    ) AS catalog(SettingName, ExpectedType)
),
typed AS
(
    SELECT
        settings.SettingName,
        settings.Value,
        COALESCE(catalog.ExpectedType, N'unknown') AS ExpectedType
    FROM dbo.System_Settings AS settings
    LEFT JOIN setting_type_catalog AS catalog
        ON catalog.SettingName = settings.SettingName
)
SELECT SettingName, Value, ExpectedType
FROM typed
WHERE
    ExpectedType = N'unknown'
    OR (ExpectedType = N'bool' AND Value NOT IN (N'True', N'False'))
    OR (ExpectedType = N'int' AND TRY_CONVERT(INT, Value) IS NULL);
GO

PRINT 'Settings invalid-value diagnostics now use the explicit runtime setting type catalog.';
GO
