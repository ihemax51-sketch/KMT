using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Dapper;
using Microsoft.Data.SqlClient;
using Serilog;

namespace KMTGuard.Database.Models
{
   public class _serverSettings
    {
        public static bool AutoStart { get; set; }
        public static string AccountDB { get; set; } = string.Empty;
        public static string LogDB { get; set; } = "SRO_VT_SHARDLOG";
        public static string ShardDB { get; set; } = string.Empty;
        public static bool RemoveCaptcha { get; set; }
        public static string CaptchaValue {get; set; } = string.Empty;
        public static bool OldLogin {get; set; }
        public static bool OldExpBar {get; set; }
        public static bool OldAlchemy {get; set; }
        public static bool GrantNameButton {get; set; }
        public static bool IconManagerButton {get; set; }
        public static bool IconManagerRight {get; set; }
        public static bool TitleManager {get; set; }
        public static bool TitleManagerColor {get; set; }
        public static bool DynamicRanking {get; set; }
        public static int DynamicRankingRefreshMinutes { get; set; } = 10;
        public static bool VipSystemEnabled { get; set; } = true;
        public static bool UniqueHistory {get; set; }
        public static bool EventRegister {get; set; }
        public static bool EventSchedule {get; set; }
        public static bool Achievements {get; set; }
        public static bool SecondarySlot {get; set; }
        public static bool MoveSkillBoard {get; set; }
        public static bool ServerInfoSkill {get; set; }
        public static bool OldMainPopup {get; set; }
        public static bool HideTitleWhileTagActive {get; set; }
        public static bool ItemComparison {get; set; }
        public static bool AutoSort {get; set; }
        public static bool PartyMemberViewer {get; set; }
        public static bool AutoSkillUpdate {get; set; }
        public static int MasteryLimit	{get; set; }
        public static int ChineseMasteryLimit { get; private set; }
        public static int EuropeanMasteryLimit { get; private set; }
        public static int ServerMaxLevel	{get; set; }
        public static bool FixDamageText {get; set; }
        public static bool AutoStrInt {get; set; }
        public static bool PickupEffect {get; set; }
        public static bool PermanentAlchemy {get; set; }
        public static bool ShowGuildInJobMode {get; set; }
        public static bool UniqueTarget {get; set; }
        public static bool Macro {get; set; }
        public static bool SecondaryPassword {get; set; }
        public static bool NewCharInfo {get; set; }
        public static bool NewIdPw {get; set; }
        public static bool EnableQuickLogin { get; set; } = true;
        public static string FacebookURL {get; set; } = string.Empty;
        public static string DiscordURL {get; set; } = string.Empty;
        public static string WebsiteURL  {get; set; } = string.Empty;
        public static bool Changelog   {get; set; }
        public static bool ShowChangelogFirstSpawn {get; set; }
        public static bool FixNewJobSuit   {get; set; }
        public static bool OldItemMall {get; set; }
        public static bool InsertCommaPrices   {get; set; }
        public static bool WriteCharacterBound {get; set; }
        public static bool NewItemMall {get; set; }
        public static bool EmojiSystem {get; set; }
        public static bool NewPartyMatch   {get; set; }
        public static bool NewJobUI    {get; set; }
        public static bool NewAlchemy  {get; set; }
        internal static bool IsNewAlchemyAvailable => false;
        public static bool ShowOnlinePlayers   {get; set; }
        public static string ServerName  {get; set; } = string.Empty;
        public static int FakePlayerCount	{get; set; }
        public static bool CheckStatus {get; set; }
        public static int HWID_LIMIT  {get; set; }
        public static int HWID_JOB_LIMIT  {get; set; }
        public static int AlchemyItemLinkMinLevel {get; set; }
        public static bool DisableReverseInJob {get; set; }
        public static int ReverseDelay    {get; set; }
        public static int MaxPlus {get; set; }
        public static bool DisableAcademy {get; set; }
        public static bool DisableAutoAttack   {get; set; }
        public static int AutoAttackMaxLevel  {get; set; }
        public static bool DisableTraceWhileJob {get; set; }
        public static int StallDelay  {get; set; }
        public static int StallLevel  {get; set; }
        public static int ExchangeDelay   {get; set; }
        public static int ExchangeLevel   {get; set; }
        public static int GuildInviteDelay    {get; set; }
        public static int UnionInviteDelay    {get; set; }
        public static int GlobalDelay {get; set; }
        public static int GlobalLevel {get; set; }
        public static int LiveItemDelay   {get; set; }
        public static int TradePetSpawnDelay  {get; set; }
        public static int RestartDelay    {get; set; }
        public static int ExitDelay   {get; set; }
        public static bool EnableItemTranslation {get; set; }
        public static int ItemTranslationPayment  {get; set; }
        public static int ItemTranslationPrice    {get; set; }
        public static int SHOW_CHAR_INFO_DELAY    {get; set; }
        public static bool NonClosePTForm { get; set; }
        public static bool EnableLuckySpin { get; set; }
        public static bool EnableLuckySpinSilk { get; set; }
        public static int LuckySpinPrice { get; set; }
        public static bool ShowGuideMenu { get; set; } = true;
        public static bool ShowGuideLuckySpin { get; set; } = true;
        public static bool ShowGuideItemChest { get; set; } = true;
        public static bool ShowGuideDropLogs { get; set; } = true;
        public static bool ShowGuideMacro { get; set; } = true;
        public static bool ShowGuideDailyLogin { get; set; } = true;
        public static bool ShowGuideDiscord { get; set; } = true;
        public static bool ShowGuideWebsite { get; set; } = true;
        public static bool ShowGuideFacebook { get; set; } = true;
        public static bool ShowGuideAutoEquip { get; set; } = true;
        public static int AutoEquipMaxLevel { get; set; } = 90;
        public static bool ShowGuideWebViewer { get; set; } = true;
        public static bool ShowGuideMapLocation { get; set; } = true;
        public static bool EnableSpecialOffers { get; set; }
        public static bool ShowGuideSpecialOffers { get; set; } = true;
        public static bool ShowGuideKillerAnimation { get; set; } = true;
        public static bool NewInventoryDesign { get; set; } = true;
        public static bool EnableOfflineStall { get; set; } = true;
        public static bool MenuLikeMaxi { get; set; }
        public static bool MenuCasy { get; set; }
        public static int OfflineStallMaxHours { get; set; } = 24;
        public static bool EnablePvpChallenge { get; set; } = true;
        public static bool ShowGuidePvpChallenge { get; set; } = true;
        public static bool EnableTradeSellCaptcha { get; set; }
        public static int TradeSellCaptchaTimeoutSeconds { get; set; }
        public static int TradeSellCaptchaMaxAttempts { get; set; }
        public static bool AllowBotLogin { get; set; } = true;
        public static bool AllowBotTrade { get; set; } = true;
        public static bool BotProtectionLogEnabled { get; set; } = true;

        public static int IPLimit { get; set; }
        public static int MaxPlusDevil { get; set; }
        #region InitialServerSettingset
        public static async Task InitServerSettings()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();
                    var schemaVersion = await connection.ExecuteScalarAsync<string?>(@"
SELECT CASE WHEN OBJECT_ID(N'dbo.System_Settings',N'U') IS NOT NULL
                  AND OBJECT_ID(N'dbo.System_ExternalSettings',N'U') IS NOT NULL
             THEN (SELECT Version FROM dbo.System_SchemaVersion WHERE Component=N'KMTGuard')
             ELSE NULL END;");
                    if (!string.Equals(schemaVersion, "3.0.0", StringComparison.Ordinal))
                        throw new InvalidOperationException(
                            "KMTGuard database schema 3.0.0 is required. Apply the packaged migration before startup.");

                    var query = "SELECT SettingName, Value FROM [dbo].[System_Settings] WITH (NOLOCK)";
                    var settings = await connection.QueryAsync<(string SettingName, string Value)>(query);

                    if (settings != null)
                    {
                        var properties = typeof(_serverSettings).GetProperties();
                        var appliedSettings = 0;

                        foreach (var setting in settings)
                        {
                            // These rows are intentionally stored in System_Settings but are
                            // not runtime properties. The packet secret is consumed directly by
                            // GameServerPacketAuthenticator, while the offline confirmation
                            // delay is a retired compatibility row removed by the v3 migration.
                            if (setting.SettingName.Equals("Security_InternalPacketSharedSecret", StringComparison.OrdinalIgnoreCase) ||
                                setting.SettingName.Equals("OfflineStallConfirmSeconds", StringComparison.OrdinalIgnoreCase))
                            {
                                appliedSettings++;
                                continue;
                            }

                            var propertyName = setting.SettingName.Equals("Menu-like-maxi", StringComparison.OrdinalIgnoreCase)
                                ? nameof(MenuLikeMaxi)
                                : setting.SettingName;
                            var property = properties.FirstOrDefault(p => p.Name.Equals(propertyName, StringComparison.OrdinalIgnoreCase));
                            if (property != null)
                            {
                                try
                                {
                                    if (property.PropertyType == typeof(int))
                                    {
                                        property.SetValue(null, int.Parse(setting.Value));
                                    }
                                    else if (property.PropertyType == typeof(bool))
                                    {
                                        if (bool.TryParse(setting.Value, out bool boolValue))
                                        {
                                            property.SetValue(null, boolValue);
                                        }
                                        else
                                        {
                                            throw new InvalidDataException(
                                                $"Invalid boolean value for Filter setting '{setting.SettingName}'.");
                                        }
                                    }
                                    else if (property.PropertyType == typeof(string))
                                    {
                                        property.SetValue(null, setting.Value);
                                    }
                                    else if (property.PropertyType == typeof(byte))
                                    {
                                        property.SetValue(null, byte.Parse(setting.Value));
                                    }
                                    else if (property.PropertyType == typeof(long))
                                    {
                                        property.SetValue(null, long.Parse(setting.Value));
                                    }
                                    else if (property.PropertyType == typeof(decimal))
                                    {
                                        property.SetValue(null, decimal.Parse(setting.Value));
                                    }
                                    else
                                    {
                                        throw new InvalidDataException(
                                            $"Unsupported Filter setting type for '{setting.SettingName}'.");
                                    }
                                }
                                catch (Exception ex)
                                {
                                    throw new InvalidDataException(
                                        $"Invalid value for Filter setting '{setting.SettingName}'.", ex);
                                }
                                appliedSettings++;
                            }
                            else
                            {
                                // Existing customer databases may contain settings
                                // owned by a GameServer add-on or a custom feature.
                                // They are not Filter runtime properties and must
                                // not prevent the proxy services from starting.
                                Log.Warning(
                                    "Ignoring external or legacy setting {SettingName}; it is not owned by this Filter build",
                                    setting.SettingName);
                            }
                        }

                        Log.Information(
                            "Configuration snapshot activated :: recognized options={OptionCount:N0}",
                            appliedSettings);
                    }

                    ChineseMasteryLimit = MasteryLimit;
                    EuropeanMasteryLimit = MasteryLimit;
                    try
                    {
                        var hasGameServerSettings = await connection.ExecuteScalarAsync<int>(@"
SELECT CASE WHEN OBJECT_ID(N'dbo.System_GameServerSettings', N'U') IS NULL THEN 0 ELSE 1 END;");
                        if (hasGameServerSettings == 1)
                        {
                            var masterySettings = await connection.QueryAsync<(string SettingName, string Value)>(@"
SELECT SettingName, Value
FROM dbo.System_GameServerSettings WITH (NOLOCK)
WHERE SettingName IN ('CH_MAX_MASTERY_LEVEL', 'EU_MAX_MASTERY_LEVEL');");

                            foreach (var setting in masterySettings)
                            {
                                if (!int.TryParse(setting.Value, out var masteryLimit) ||
                                    masteryLimit is < 1 or > 10000)
                                    continue;

                                if (setting.SettingName.Equals("CH_MAX_MASTERY_LEVEL", StringComparison.OrdinalIgnoreCase))
                                    ChineseMasteryLimit = masteryLimit;
                                else if (setting.SettingName.Equals("EU_MAX_MASTERY_LEVEL", StringComparison.OrdinalIgnoreCase))
                                    EuropeanMasteryLimit = masteryLimit;
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        Log.Warning(ex,
                            "GameServer mastery limits are unavailable; using the legacy Client mastery limit");
                    }

                    // The custom New Alchemy pipeline is retired. Keep the legacy
                    // setting readable for database compatibility, but never allow
                    // it to become active at runtime.
                    NewAlchemy = false;
                }
            }
            catch (Exception ex)
            {
                Log.Fatal(ex, "Filter settings validation failed");
                throw;
            }
        }
        #endregion
    }
}
