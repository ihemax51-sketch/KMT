using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Text;
using System.Threading.Tasks;
using Dapper;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Database.Shard;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using Microsoft.Data.SqlClient;
using Serilog;


namespace KMTGuard.ServerManagers
{
    public class RefManager
    {

        #region Filter Features
        public static Dictionary<string, int> m_bypassHwidbyIP { get; set; } = new();
        private static HashSet<string> _gmIpList = new(StringComparer.OrdinalIgnoreCase);
        public static List<int> m_RefLoggerMobKill { get; set; } = new List<int>();

        public static Dictionary<int, _LuckySpinRewards> m_LuckySpin { get; set; } = new();
        public static Dictionary<int, _SpecialOffer> m_SpecialOffers { get; set; } = new();
        public static Dictionary<int, _KillerAnimation> m_KillerAnimations { get; set; } = new();

        #endregion

        #region UI Custom
        public static ConcurrentDictionary<string, string> ActiveNameColors { get; set; } = new();
        public static ConcurrentDictionary<string, string> ActiveTitleColors { get; set; } = new();
        public static ConcurrentDictionary<string, int> ActiveLeftIcons { get; set; } = new();
        public static ConcurrentDictionary<string, int> ActiveRightIcons { get; set; } = new();
        public static ConcurrentDictionary<string, byte> ActiveTags { get; set; } = new();
        public static Dictionary<int, string> Icons { get; set; } = new();
        public static Dictionary<byte, string> Tags { get; set; } = new();
        public static Dictionary<int, _RefAchievement> m_RefAchievements { get; set; } = new Dictionary<int, _RefAchievement>();
        public static Dictionary<int, _RefAchievementCondition> m_RefAchievementsCondition { get; set; } = new Dictionary<int, _RefAchievementCondition>();
        public static Dictionary<int, _RefHideSkillEffect> m_HideSkillEffects { get; set; } = new Dictionary<int, _RefHideSkillEffect>();
        public static Dictionary<int, _RefMapSettings> m_RefEventMapSettings { get; set; } = new Dictionary<int, _RefMapSettings>();
        public static Dictionary<string, _RefFellowData> m_RefFellowData { get; set; } = new Dictionary<string, _RefFellowData>();
        public static ConcurrentDictionary<int, _UniqueHistory> UniqueLog { get; set; } = new();
        public static Dictionary<int, _RefEventRegister> m_RefEventRegister { get; set; } = new Dictionary<int, _RefEventRegister>();
        public static Dictionary<int, _RefEventSchedule> m_RefEventSchedule = new Dictionary<int, _RefEventSchedule>();
        public static Dictionary<int, string> RefGlobalColor = new Dictionary<int, string>();
        public static Dictionary<int, _RefNewAvatarMall> m_RefNewAvatarMall = new Dictionary<int, _RefNewAvatarMall>();
        public static Dictionary<int, _RefNewItemMall> m_RefNewItemMall = new Dictionary<int, _RefNewItemMall>();
        public static List<int> m_RefFellowPetRefObjID { get; set; } = new List<int>();

        public static IReadOnlyDictionary<int, _RefRankCategories> RankCategories { get; private set; } = new Dictionary<int, _RefRankCategories>();
        public static IReadOnlyDictionary<int, _Rank_Custom1> Rank_Custom1 { get; private set; } = new Dictionary<int, _Rank_Custom1>();
        public static IReadOnlyDictionary<int, _Rank_Custom1> Rank_Custom2 { get; private set; } = new Dictionary<int, _Rank_Custom1>();
        public static IReadOnlyDictionary<int, _Rank_Custom1> Rank_Custom3 { get; private set; } = new Dictionary<int, _Rank_Custom1>();
        public static IReadOnlyDictionary<int, _Rank_Custom1> Rank_Custom4 { get; private set; } = new Dictionary<int, _Rank_Custom1>();
        public static IReadOnlyDictionary<int, _Rank_Custom1> Rank_Custom5 { get; private set; } = new Dictionary<int, _Rank_Custom1>();
        public static IReadOnlyDictionary<int, _Rank_Custom1> Rank_Custom6 { get; private set; } = new Dictionary<int, _Rank_Custom1>();
        public static IReadOnlyDictionary<int, _Rank_Custom1> Rank_Custom7 { get; private set; } = new Dictionary<int, _Rank_Custom1>();
        public static IReadOnlyDictionary<int, _Rank_Custom1> Rank_Custom8 { get; private set; } = new Dictionary<int, _Rank_Custom1>();
        public static IReadOnlyDictionary<int, _Rank_Custom1> Rank_Custom9 { get; private set; } = new Dictionary<int, _Rank_Custom1>();
        #endregion
        public static Dictionary<byte, _RefHWANLevel> RefHwan { get; set; } = new();
        private static List<Notice> NoticesCache { get; set; } = new List<Notice>();

        public static DelayedJobManager g_DelayedJobMgr = new DelayedJobManager();

        private static readonly object _timerLock = new();
        private static readonly SemaphoreSlim _rankRefreshLock = new(1, 1);
        private static Timer? _timer;
        public static DateTime? CacheLastRefreshUtc { get; private set; }
        public static ConcurrentDictionary<int, _Scheduler> _Scheduler { get; set; } = new();
        public static void StartLoadRanksTimer()
        {
            lock (_timerLock)
            {
                if (_timer != null)
                {
                    return;
                }

                // Initialize() performs the first synchronous load. The timer owns
                // subsequent refreshes and never overlaps a command-driven refresh.
                var refreshMinutes = Math.Clamp(
                    _serverSettings.DynamicRankingRefreshMinutes,
                    1,
                    24 * 60);
                var refreshInterval = TimeSpan.FromMinutes(refreshMinutes);
                _timer = new Timer(
                    _ => _ = RunLoadRanksTimerAsync(),
                    null,
                    refreshInterval,
                    refreshInterval);
            }
        }

        public static void StopTimers()
        {
            lock (_timerLock)
            {
                _timer?.Dispose();
                _timer = null;
            }

            g_DelayedJobMgr.Stop();
        }

        private static async Task RunLoadRanksTimerAsync()
        {
            if (!await _rankRefreshLock.WaitAsync(0))
            {
                return;
            }

            try
            {
                await LoadRankingSnapshotCoreAsync(includeCategories: true);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "The scheduled dynamic-ranking refresh failed; the previous snapshot remains active");
            }
            finally
            {
                _rankRefreshLock.Release();
            }
        }
        public static async Task<_RefObjCommon?> GetRefObjCommonAsync(int id)
        {
            try
            {
                if (RefObjCommons.TryGetValue(id, out var cachedValue))
                {
                    return cachedValue;
                }

                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();
                    var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
                    var value = await connection.QueryFirstOrDefaultAsync<_RefObjCommon>(
                        $"SELECT * FROM {shardDb}.._RefObjCommon WITH (NOLOCK) WHERE ID = @ID",
                        new { ID = id });
                    if (value != null)
                    {
                        RefObjCommons.TryAdd(id, value);
                    }
                    return value;
                }

            }
            catch (Exception ex)
            {
                Log.Warning(ex.Message.ToString() + "GetRefObjCommonAsync");
                return null;
            }
        }
        public static Task<bool> GetRefObjCommonValidate(string id)
        {
            try
            {
                foreach (var line in RefObjCommons)
                {
                    if (line.Value.CodeName128 == id)
                    {
                        return Task.FromResult(true);
                    }
                }
                return Task.FromResult(false);
            }
            catch (Exception ex)
            {
                Log.Warning(ex.Message.ToString() + "GetRefObjCommonAsync");
                return Task.FromResult(false);
            }
        }
        public static async Task<_RefObjChar?> GetRefObjCharAsync(int id)
        {
            try
            {
                if (RefObjChars.TryGetValue(id, out var cachedValue))
                {
                    return cachedValue;
                }

                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();
                    var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
                    var value = await connection.QueryFirstOrDefaultAsync<_RefObjChar>(
                        $"SELECT * FROM {shardDb}.._RefObjChar WITH (NOLOCK) WHERE ID = @ID",
                        new { ID = id });
                    if (value != null)
                    {
                        RefObjChars.TryAdd(id, value);
                    }
                    return value;
                }

            }
            catch (Exception ex)
            {
                Log.Warning(ex.Message.ToString() + "GetRefObjCharAsync");
                return null;
            }
        }
        public static async Task Initialize()
        {
            await LoadScheduler();
            await LoadLuckySpin();

            await InitNPCUniqueIds();
            await InitRefObjCommon();
            await InitRefObjChar();
            await LoadNoticesIntoCache();
            await LoadRefHwan();

            await LoadbypassHwidbyIP();

            await LoadSomeTables();
            await LoadTags();
            await LoadIcons();
            await LoadRefAchievements();
            await LoadRefAchievementsCondition();
            await LoadRefHideSkillEffect();
            await LoadRefMapSettings();
            await LoadRefFellowData();
            await LoadRefLoggerMobKill();
            await Features.UniqueHistory.UniqueHistoryService.LoadAsync();
            await LoadRefEventRegister();
            await LoadRefEventSchedule();
            await LoadRefGlobalColor();
            await LoadRefNewAvatarMall();
            await LoadRefNewItemMall();
            await LoadSpecialOffers();
            await LoadKillerAnimations();
            await LoadRefFellowPetRefObjID();
            await RefreshRankingDataAsync(includeCategories: true);
            CacheLastRefreshUtc = DateTime.UtcNow;
            StartLoadRanksTimer();


            g_DelayedJobMgr.Run();
        }

        public static async Task InitializeGateway()
        {
            await LoadGmIPs();
            await LoadNoticesIntoCache();
            await LoadRefNewAvatarMall();
            await LoadRefNewItemMall();
            CacheLastRefreshUtc = DateTime.UtcNow;
        }
        public static async Task LoadLuckySpin()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();


                    string query1 = "SELECT * FROM [dbo].[LuckySpin_Rewards] WITH (NOLOCK) WHERE IsActive = 1 ORDER BY ID";
                    var result1 = await connection.QueryAsync<_LuckySpinRewards>(query1);
                    var snapshot = result1.ToDictionary(item => item.ID);
                    m_LuckySpin = snapshot;


                    //Log.Fatal("LoadRefLoggerMobKill loaded into cache. Total count: " + m_RefLoggerMobKill.Count);
                }
            }
            catch (Exception ex)
            {
                Log.Fatal($"Error LoadbypassHwidbyIP tables: {ex.Message}");
            }
        }
        public static async Task LoadSpecialOffers()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();

                    string query = "SELECT * FROM [dbo].[Offer_List] WITH (NOLOCK) WHERE Service = 1 ORDER BY SortOrder, ID";
                    var result = await connection.QueryAsync<_SpecialOffer>(query);
                    var snapshot = result.ToDictionary(item => item.ID);
                    m_SpecialOffers = snapshot;
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error LoadSpecialOffers table: {ex.Message}");
            }
        }
        public static async Task LoadKillerAnimations()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();

                    string query = "SELECT * FROM [dbo].[KillerAnimation_List] WITH (NOLOCK) WHERE Service = 1 ORDER BY SortOrder, ID";
                    var result = await connection.QueryAsync<_KillerAnimation>(query);
                    var snapshot = result.ToDictionary(item => item.ID);
                    m_KillerAnimations = snapshot;
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error LoadKillerAnimations table: {ex.Message}");
            }
        }
        public static async Task LoadScheduler()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();


                    string query1 = $"Select * from [dbo].[System_Schedule] with (nolock) where IsEnabled = 1";
                    var result1 = await connection.QueryAsync<_Scheduler>(query1);
                    var snapshot = new ConcurrentDictionary<int, _Scheduler>(
                        result1.ToDictionary(item => item.Idx));
                    _Scheduler = snapshot;
;
                }
            }
            catch (Exception ex)
            {
                Log.Fatal($"Error Scheduler tables: {ex.Message}");
            }
        }
        public static async Task LoadbypassHwidbyIP()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();


                    string query1 = "SELECT IP, Limit FROM [dbo].[Auth_HWIDBypassIPs] with (nolock)";
                    var result1 = await connection.QueryAsync<_bypassHwidbyIP>(query1);
                    var snapshot = result1.ToDictionary(
                        item => item.IP, item => item.Limit,
                        StringComparer.OrdinalIgnoreCase);
                    m_bypassHwidbyIP = snapshot;

                    //Log.Fatal("LoadRefLoggerMobKill loaded into cache. Total count: " + m_RefLoggerMobKill.Count);
                }
            }
            catch (Exception ex)
            {
                Log.Fatal($"Error LoadbypassHwidbyIP tables: {ex.Message}");
            }

            await LoadGmIPs();
        }

        public static async Task LoadGmIPs()
        {
            try
            {
                using var connection = new SqlConnection(Program.Connectionstring);
                await connection.OpenAsync();

                const string query = "SELECT IP FROM [dbo].[Auth_GMIPs] WITH (NOLOCK)";
                var rows = await connection.QueryAsync<_GMIPList>(query);
                var loadedIps = new HashSet<string>(StringComparer.OrdinalIgnoreCase);

                foreach (var row in rows)
                {
                    if (TryNormalizeIpAddress(row.IP, out var normalizedIp))
                        loadedIps.Add(normalizedIp);
                    else
                        Log.Warning("Ignoring invalid IP address in Auth_GMIPs: {Ip}", row.IP);
                }

                Volatile.Write(ref _gmIpList, loadedIps);
                Log.Information("Loaded {Count} GM IP address(es) for Gateway maintenance access", loadedIps.Count);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "Could not load Auth_GMIPs; no new Gateway maintenance access was granted");
            }
        }

        public static bool IsGmIp(string? clientIp)
        {
            return TryNormalizeIpAddress(clientIp, out var normalizedIp) &&
                   Volatile.Read(ref _gmIpList).Contains(normalizedIp);
        }

        internal static bool TryNormalizeIpAddress(string? value, out string normalizedIp)
        {
            normalizedIp = string.Empty;
            if (!IPAddress.TryParse(value?.Trim(), out var address))
                return false;

            if (address.IsIPv4MappedToIPv6)
                address = address.MapToIPv4();

            normalizedIp = address.ToString();
            return true;
        }

        public static ConcurrentDictionary<int, ___SR_GSNpcUniqueIdList> _GSNPCList = new();
        public static ConcurrentDictionary<int, _RefObjCommon> RefObjCommons = new();
        public static ConcurrentDictionary<int, _RefObjChar> RefObjChars = new();
        public async static Task InitNPCUniqueIds()
        {
            using (var connection = new SqlConnection(Program.Connectionstring))
            {
                await connection.OpenAsync();
                var proxyDb = SqlIdentifier.Quote(Program.ProxyDb);
                var result1 = await connection.QueryAsync<___SR_GSNpcUniqueIdList>(
                    $"SELECT ID, UniqueID, RefObjId, CodeName128 FROM {proxyDb}.[dbo].[NPC_GameServerIDs] WITH (NOLOCK) ORDER BY ID");
                _GSNPCList = BuildNpcUniqueIdSnapshot(result1, out var duplicateCount);
                if (duplicateCount > 0)
                {
                    Log.Warning(
                        "NPC index retained the first mapping for {DuplicateCount:N0} duplicate UniqueID row(s)",
                        duplicateCount);
                }
                Log.Information("World index sealed :: NPC records={RecordCount:N0}", _GSNPCList.Count);
            }
        }

        internal static ConcurrentDictionary<int, ___SR_GSNpcUniqueIdList> BuildNpcUniqueIdSnapshot(
            IEnumerable<___SR_GSNpcUniqueIdList> rows,
            out int duplicateCount)
        {
            var snapshot = new ConcurrentDictionary<int, ___SR_GSNpcUniqueIdList>();
            duplicateCount = 0;

            // Preserve the legacy loader's first-row-wins behavior. Building a
            // fresh snapshot keeps readers isolated from a partial reload while
            // allowing databases where different NPCs reuse a runtime UniqueID.
            foreach (var row in rows.OrderBy(row => row.ID))
            {
                if (!snapshot.TryAdd(row.UniqueID, row))
                    duplicateCount++;
            }

            return snapshot;
        }
        public static async Task InitRefObjCommon()
        {
            using (var connection = new SqlConnection(Program.Connectionstring))
            {
                await connection.OpenAsync();
                var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
                var value = await connection.QueryAsync<_RefObjCommon>(
                    $"SELECT * FROM {shardDb}.._RefObjCommon WITH (NOLOCK)");
                RefObjCommons = new ConcurrentDictionary<int, _RefObjCommon>(
                    value.ToDictionary(line => line.ID));
            }
            Log.Information("Object catalog indexed :: definitions={DefinitionCount:N0}", RefObjCommons.Count);
        }
        public static async Task InitRefObjChar()
        {
            using (var connection = new SqlConnection(Program.Connectionstring))
            {
                await connection.OpenAsync();
                var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
                var value = await connection.QueryAsync<_RefObjChar>(
                    $"SELECT * FROM {shardDb}.._RefObjChar WITH (NOLOCK)");
                RefObjChars = new ConcurrentDictionary<int, _RefObjChar>(
                    value.ToDictionary(line => line.ID));
            }
            Log.Information("Character catalog indexed :: archetypes={ArchetypeCount:N0}", RefObjChars.Count);
        }
        public static async Task LoadNoticesIntoCache()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();
                    string query = "SELECT Name, String FROM [dbo].[System_Notices]";
                    var result = await connection.QueryAsync<Notice>(query);
                    NoticesCache = result.ToList();
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error loading notices into cache: {ex.Message}");
            }
        }

        public static string GetNoticeMessage(string name)
        {
            try
            {
                if (NoticesCache == null || !NoticesCache.Any())
                {
                    Log.Warning("Notices cache is empty. Make sure to load notices first.");
                    return string.Empty; // Cache boşsa boş string döndür
                }

                var notice = NoticesCache.FirstOrDefault(n => n.Name == name);
                return PlayerLanguage.ResolveSystemNotice(name, notice?.String);
            }
            catch (Exception ex)
            {
                Log.Warning($"An error occurred in GetNoticeMessage: {ex.Message}");
                return string.Empty; // Hata durumunda boş string döndür
            }
        }
        public static async Task LoadRefHwan()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();

                    // İlk sorgu: _ActiveTitleColors tablosu
                    var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
                    string query1 = $"SELECT HwanLevel, Title_CH70, Title_EU70 FROM {shardDb}.._RefHWANLevel WITH (NOLOCK)";
                    var result1 = await connection.QueryAsync<_RefHWANLevel>(query1);
                    RefHwan = result1.ToDictionary(item => item.HwanLevel);
                    //Log.Fatal("LoadRefHwan loaded into cache. Total count: " + RefHwan.Count);
                }
            }
            catch (Exception ex)
            {
                Log.Fatal($"Error LoadRefHwan tables: {ex.Message}");
            }
        }
        public static Task LoadRanks()
        {
            return RefreshRankingDataAsync(includeCategories: false);
        }

        public static Task RefreshRanksAndCategories()
        {
            return RefreshRankingDataAsync(includeCategories: true);
        }

        public static Task LoadRefRankCagetorys()
        {
            return RefreshRankingDataAsync(includeCategories: true, ranks: false);
        }

        public static bool TryGetRank(byte categoryId, out IReadOnlyDictionary<int, _Rank_Custom1> rank)
        {
            rank = categoryId switch
            {
                1 => Rank_Custom1,
                2 => Rank_Custom2,
                3 => Rank_Custom3,
                4 => Rank_Custom4,
                5 => Rank_Custom5,
                6 => Rank_Custom6,
                7 => Rank_Custom7,
                8 => Rank_Custom8,
                9 => Rank_Custom9,
                _ => new Dictionary<int, _Rank_Custom1>()
            };

            return categoryId is >= 1 and <= 9 && RankCategories.ContainsKey(categoryId);
        }

        private static async Task RefreshRankingDataAsync(bool includeCategories, bool ranks = true)
        {
            await _rankRefreshLock.WaitAsync();
            try
            {
                await LoadRankingSnapshotCoreAsync(includeCategories, ranks);
            }
            finally
            {
                _rankRefreshLock.Release();
            }
        }

        private static async Task LoadRankingSnapshotCoreAsync(bool includeCategories, bool ranks = true)
        {
            try
            {
                await using var connection = new SqlConnection(Program.Connectionstring);
                await connection.OpenAsync();

                IReadOnlyDictionary<int, _RefRankCategories>? categories = null;
                if (includeCategories)
                {
                    var categoryRows = await connection.QueryAsync<_RefRankCategories>(
                        @"SELECT ID, Active, Category
                          FROM [dbo].[Rank_Categories]
                          WHERE Active = 1 AND ID BETWEEN 1 AND 9
                          ORDER BY ID");

                    categories = categoryRows
                        .Where(item => !string.IsNullOrWhiteSpace(item.Category))
                        .GroupBy(item => item.Category.Trim(), StringComparer.OrdinalIgnoreCase)
                        .Select(group => group.OrderBy(item => item.ID).First())
                        .GroupBy(item => item.ID)
                        .ToDictionary(group => group.Key, group => group.First());
                }

                if (!ranks)
                {
                    RankCategories = categories ?? RankCategories;
                    return;
                }

                var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
                var snapshots = new IReadOnlyDictionary<int, _Rank_Custom1>[9];
                for (var categoryId = 1; categoryId <= 9; categoryId++)
                {
                    var tableName = $"[dbo].[Rank_Data{categoryId:00}]";
                    var sql = $@"
SELECT r.CharID, r.CharName16, r.GuildName, r.Point
FROM {tableName} AS r
INNER JOIN {shardDb}.[dbo].[_Char] AS c
    ON c.CharID = r.CharID
   AND c.CharName16 COLLATE DATABASE_DEFAULT =
       r.CharName16 COLLATE DATABASE_DEFAULT;";

                    var rows = await connection.QueryAsync<_Rank_Custom1>(sql);
                    snapshots[categoryId - 1] = rows
                        .GroupBy(row => row.CharID)
                        .ToDictionary(group => group.Key, group => group.First());
                }

                // Publish only after every query succeeds. Readers therefore see
                // either the complete old snapshot or the complete new snapshot.
                Rank_Custom1 = snapshots[0];
                Rank_Custom2 = snapshots[1];
                Rank_Custom3 = snapshots[2];
                Rank_Custom4 = snapshots[3];
                Rank_Custom5 = snapshots[4];
                Rank_Custom6 = snapshots[5];
                Rank_Custom7 = snapshots[6];
                Rank_Custom8 = snapshots[7];
                Rank_Custom9 = snapshots[8];
                RankCategories = categories ?? RankCategories;
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Dynamic-ranking snapshot refresh failed; the previous snapshot remains active");
            }
        }
        public static async Task LoadRefFellowPetRefObjID()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();


                    string query1 = "SELECT * FROM [dbo].[Fellow_PetObjectIDs] with (nolock)";
                    var result1 = await connection.QueryAsync<int>(query1);
                    m_RefFellowPetRefObjID = result1.Distinct().ToList();
                    //Log.Warning("LoadRefFellowPetRefObjID loaded into cache. Total count: " + m_RefNewItemMall.Count);
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error LoadRefFellowPetRefObjID tables: {ex.Message}");
            }
        }

        public static async Task LoadRefNewItemMall()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();


                    string query1 = "SELECT * FROM [dbo].[Mall_Items] with (nolock) where Service = 1";
                    var result1 = await connection.QueryAsync<_RefNewItemMall>(query1);
                    m_RefNewItemMall = result1.ToDictionary(item => item.ID);
                    //Log.Warning("LoadRefNewItemMall loaded into cache. Total count: " + m_RefNewItemMall.Count);
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error LoadRefNewItemMall tables: {ex.Message}");
            }
        }
        public static async Task LoadRefNewAvatarMall()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();


                    string query1 = "SELECT * FROM [dbo].[Mall_Avatars] with (nolock) where Service = 1";
                    var result1 = await connection.QueryAsync<_RefNewAvatarMall>(query1);
                    m_RefNewAvatarMall = result1.ToDictionary(item => item.ID);
                    //Log.Warning("LoadRefNewAvatarMall loaded into cache. Total count: " + m_RefNewAvatarMall.Count);
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error LoadRefNewAvatarMall tables: {ex.Message}");
            }
        }

        public static async Task LoadRefGlobalColor()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();


                    string query1 = "SELECT GlobalItemID, Color FROM [dbo].[GlobalChatColors] with (nolock)";
                    var result1 = await connection.QueryAsync<(int GlobalItemID, string Color)>(query1);
                    RefGlobalColor = result1.ToDictionary(
                        item => item.GlobalItemID, item => item.Color);
                    //Log.Warning("LoadRefGlobalColor loaded into cache. Total count: " + RefGlobalColor.Count);
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error LoadRefGlobalColor table: {ex.Message}");
            }
        }
        public static async Task<bool> LoadRefEventSchedule()
        {
            const string querySelectAllRef = "SELECT ID, EventName, Day, Time FROM [dbo].[Event_ScheduleSettings] with (nolock)";

            int uniqueID = 1;
            var snapshot = new Dictionary<int, _RefEventSchedule>();


            using (var connection = new SqlConnection(Program.Connectionstring))
            {
                await connection.OpenAsync();


                using (var command = new SqlCommand(querySelectAllRef, connection))
                using (var reader = await command.ExecuteReaderAsync())
                {
                    while (await reader.ReadAsync())
                    {
                        var record = new _RefEventSchedule
                        {
                            ID = reader.GetInt32(0),
                            EventName = reader.GetString(1),
                            Day = reader.GetByte(2),
                            Time = reader.GetString(3)
                        };
                        snapshot[record.ID] = record;
                        uniqueID = record.ID + 1;
                    }
                }
            }

            m_RefEventSchedule = snapshot;

            //Log.Warning($"Loaded {m_RefEventSchedule.Count} RefEventSchedule");
            return true;
        }
        public static async Task LoadRefEventRegister()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();


                    string query1 = "SELECT * FROM [dbo].[Event_RegisterSettings] with (nolock)";
                    var result1 = await connection.QueryAsync<_RefEventRegister>(query1);
                    m_RefEventRegister = result1.ToDictionary(item => item.ID);
                    //Log.Warning("LoadRefEventRegister loaded into cache. Total count: " + m_RefEventRegister.Count);
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error LoadRefEventRegister tables: {ex.Message}");
            }
        }
        public static async Task LoadRefLoggerMobKill()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();


                    string query1 = "SELECT RefMobID FROM [dbo].[Log_MobKillSettings] with (nolock)";
                    var result1 = await connection.QueryAsync<int>(query1);
                    m_RefLoggerMobKill = result1.Distinct().ToList();
                    //Log.Warning("LoadRefLoggerMobKill loaded into cache. Total count: " + m_RefLoggerMobKill.Count);
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error LoadRefLoggerMobKill tables: {ex.Message}");
            }
        }
        public static async Task LoadRefFellowData()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();


                    string query1 = "SELECT * FROM [dbo].[Fellow_Settings] with (nolock)";
                    var result1 = await connection.QueryAsync<_RefFellowData>(query1);
                    m_RefFellowData = result1.ToDictionary(item => item.PetNameStrID);
                    //Log.Warning("LoadRefFellowData loaded into cache. Total count: " + m_RefFellowData.Count);
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error LoadRefFellowData tables: {ex.Message}");
            }
        }
        public static async Task LoadRefMapSettings()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();


                    string query1 = "SELECT * FROM [dbo].[Map_Settings] with (nolock)";
                    var result1 = await connection.QueryAsync<_RefMapSettings>(query1);
                    var snapshot = result1.ToDictionary(item => item.RegionID);
                    m_RefEventMapSettings = snapshot;
                    //Log.Warning("LoadRefMapSettings loaded into cache. Total count: " + m_RefEventMapSettings.Count);
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error LoadRefMapSettings tables: {ex.Message}");
            }
        }
        public static async Task LoadRefHideSkillEffect()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();


                    string query1 = "SELECT * FROM [dbo].[Security_HiddenSkillEffects] with (nolock)";
                    var result1 = await connection.QueryAsync<_RefHideSkillEffect>(query1);
                    var snapshot = result1.ToDictionary(item => item.SkillID);
                    m_HideSkillEffects = snapshot;
                    //Log.Warning("LoadRefHideSkillEffect loaded into cache. Total count: " + m_HideSkillEffects.Count);
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error LoadRefHideSkillEffect tables: {ex.Message}");
            }
        }
        public static async Task LoadRefAchievementsCondition()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();

                    string query1 = @"
                        SELECT condition.*
                        FROM [dbo].[Achievement_Conditions] AS condition
                        INNER JOIN [dbo].[Achievement_List] AS achievement
                            ON achievement.ID = condition.RefAchievementID
                        WHERE achievement.Service = 1";
                    var result1 = await connection.QueryAsync<_RefAchievementCondition>(query1);
                    var snapshot = new Dictionary<int, _RefAchievementCondition>();
                    foreach (var item in result1)
                    {
                        if (!snapshot.ContainsKey(item.ID))
                            snapshot.Add(item.ID, item);
                    }
                    m_RefAchievementsCondition = snapshot;
                    //Log.Warning("LoadRefAchievementsCondition loaded into cache. Total count: " + m_RefAchievementsCondition.Count);
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error LoadRefAchievementsCondition tables: {ex.Message}");
            }
        }
        public static async Task LoadRefAchievements()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();

                    string query1 = "SELECT * FROM [dbo].[Achievement_List] WHERE Service = 1";
                    var result1 = await connection.QueryAsync<_RefAchievement>(query1);
                    var snapshot = new Dictionary<int, _RefAchievement>();
                    foreach (var item in result1)
                    {
                        if (!snapshot.ContainsKey(item.ID))
                            snapshot.Add(item.ID, item);
                    }
                    m_RefAchievements = snapshot;
                    //Log.Warning("LoadRefAchievements loaded into cache. Total count: " + m_RefAchievements.Count);
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error LoadRefAchievements tables: {ex.Message}");
            }
        }
        public static async Task LoadTags()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();


                    string query1 = "SELECT TagID, TagName FROM [dbo].[Tags] with (nolock)";
                    var result1 = await connection.QueryAsync<(byte TagID, string TagName)>(query1);
                    Tags = result1.ToDictionary(item => item.TagID, item => item.TagName);
                    // Tags are cached by TagID for runtime broadcasts.
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error loading Tags table: {ex.Message}");
            }
        }
        public static async Task LoadIcons()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();


                    string query1 = "SELECT IconID, MediaPath FROM [dbo].[Icons] with (nolock)";
                    var result1 = await connection.QueryAsync<(int IconID, string MediaPath)>(query1);
                    Icons = result1.ToDictionary(item => item.IconID, item => item.MediaPath);
                    // Icon media paths are cached by IconID.
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error LoadRefIconMediaPath tables: {ex.Message}");
            }
        }











        // في RefManager.cs
        public static ConcurrentDictionary<int, HashSet<int>> m_BlockedSkillsByRegion
            = new ConcurrentDictionary<int, HashSet<int>>();

        public static async Task LoadBlockedSkillsByRegion()
        {
            try
            {
                var temp = new ConcurrentDictionary<int, HashSet<int>>();

                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();

                    var rows = await connection.QueryAsync<(int RegionID, int SkillID)>(
                        "SELECT RegionID, SkillID FROM [dbo].[Security_BlockedSkills] WITH (NOLOCK) WHERE Service = 1");

                    foreach (var r in rows)
                    {
                        var set = temp.GetOrAdd(r.RegionID, _ => new HashSet<int>());
                        set.Add(r.SkillID);
                    }
                }

                // Swap مرجعي آمن
                m_BlockedSkillsByRegion = temp;
                Serilog.Log.Information($"[blocked_skill] Loaded {m_BlockedSkillsByRegion.Count} regions.");
            }
            catch (Exception ex)
            {
                Serilog.Log.Warning($"[blocked_skill] load failed: {ex.Message}");
            }
        }





        public static async Task LoadSomeTables()
        {
            try
            {
                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();

                    string queryx1 = "SELECT CharName16, ColorCode FROM [dbo].[ActiveNameColors] with (nolock)";
                    var resultx1 = await connection.QueryAsync<(string CharName16, string ColorCode)>(queryx1);
                    var nameColors = new ConcurrentDictionary<string, string>(
                        resultx1.ToDictionary(item => item.CharName16, item => item.ColorCode,
                            StringComparer.OrdinalIgnoreCase),
                        StringComparer.OrdinalIgnoreCase);

                    string query1 = "SELECT CharName16, ColorCode FROM [dbo].[ActiveTitleColors] with (nolock)";
                    var result1 = await connection.QueryAsync<(string CharName16, string ColorCode)>(query1);
                    var titleColors = new ConcurrentDictionary<string, string>(
                        result1.ToDictionary(item => item.CharName16, item => item.ColorCode,
                            StringComparer.OrdinalIgnoreCase),
                        StringComparer.OrdinalIgnoreCase);
                    // Active title colors loaded.


                    string query2 = "SELECT CharName16, IconID FROM [dbo].[ActiveLeftIcons]";
                    var result2 = await connection.QueryAsync<(string CharName16, int IconID)>(query2);
                    var leftIcons = new ConcurrentDictionary<string, int>(
                        result2.ToDictionary(item => item.CharName16, item => item.IconID,
                            StringComparer.OrdinalIgnoreCase),
                        StringComparer.OrdinalIgnoreCase);
                    // Active left icons loaded.


                    string query3 = "SELECT CharName16, IconID FROM [dbo].[ActiveRightIcons]";
                    var result3 = await connection.QueryAsync<(string CharName16, int IconID)>(query3);
                    var rightIcons = new ConcurrentDictionary<string, int>(
                        result3.ToDictionary(item => item.CharName16, item => item.IconID,
                            StringComparer.OrdinalIgnoreCase),
                        StringComparer.OrdinalIgnoreCase);
                    // Active right icons loaded.


                    string query4 = "SELECT CharName16, TagID FROM [dbo].[ActiveTags]";
                    var result4 = await connection.QueryAsync<(string CharName16, byte TagID)>(query4);
                    var activeTitles = new ConcurrentDictionary<string, byte>(StringComparer.OrdinalIgnoreCase);
                    foreach (var item in result4)
                    {
                        activeTitles[item.CharName16] = item.TagID;
                    }
                    ActiveNameColors = nameColors;
                    ActiveTitleColors = titleColors;
                    ActiveLeftIcons = leftIcons;
                    ActiveRightIcons = rightIcons;
                    ActiveTags = activeTitles;
                    // Active player tags loaded.

                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error loading some tables: {ex.Message}");
            }
        }

    }




}



