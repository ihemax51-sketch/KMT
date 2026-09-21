using Dapper;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.ServerManagers;
using Microsoft.Data.SqlClient;
using Serilog;

namespace KMTGuard.Clientless;

public static class ClientlessManager
{
    public const string PartyModeSoloForms = "SoloForms";
    public const string PartyModeGroupsOf8 = "GroupsOf8";

    // vSRO _RefSkill parameter tags. These values are the little-endian integer
    // representations used by the media records (att, dura and reqi).
    internal const int AttackSkillParameter = 6386804;
    internal const int DurationSkillParameter = 1685418593;
    internal const int RequiredItemSkillParameter = 1919250793;
    internal const int PrimaryBuffSkillType = 3;
    internal const int AnyWeaponRequirement = 255;

    internal static int ResolvePrimaryWeaponMastery(int weaponTypeId4) => weaponTypeId4 switch
    {
        2 or 3 => 257,
        4 or 5 => 258,
        6 => 259,
        7 or 8 or 9 => 513,
        11 => 514,
        12 or 13 => 515,
        10 => 516,
        14 => 517,
        15 => 518,
        _ => 0
    };

    public sealed class PartyFormPolicy
    {
        public PartyFormPolicy()
        {
        }

        public PartyFormPolicy(
            bool enabled,
            string title,
            byte minLevel,
            byte maxLevel,
            byte purpose,
            byte settingsFlag,
            string mode = PartyModeGroupsOf8)
        {
            Enabled = enabled;
            Mode = mode;
            Title = title;
            MinLevel = minLevel;
            MaxLevel = maxLevel;
            Purpose = purpose;
            SettingsFlag = settingsFlag;
        }

        public bool Enabled { get; set; }
        public string Mode { get; set; } = PartyModeGroupsOf8;
        public string Title { get; set; } = "{CharacterName}";
        public byte MinLevel { get; set; } = 1;
        public byte MaxLevel { get; set; } = 140;
        public byte Purpose { get; set; }
        public byte SettingsFlag { get; set; } = 7;
    }

    private const string PartyFormSettingsTable = "[dbo].[ClientlessPartyFormSettings]";
    private static readonly List<ClientlessSession> Sessions = new();
    private static readonly SemaphoreSlim LifecycleLock = new(1, 1);
    private static readonly object SessionsLock = new();
    private static readonly Dictionary<int, PendingClientlessLaunch> PendingLaunches = new();
    private static readonly SemaphoreSlim HuntPolicyLock = new(1, 1);
    private static readonly SemaphoreSlim SpawnSkillMetadataLock = new(1, 1);
    private static readonly SemaphoreSlim PartyGroupingLock = new(1, 1);
    private static CancellationTokenSource? _shutdown;
    private static Task? _launcherTask;
    private static long _launchVersion;
    private static ClientlessHuntPolicy _huntPolicy = new();
    private static DateTime _huntPolicyExpiresUtc = DateTime.MinValue;
    private static HashSet<uint> _creatorFlagBuffSkillIds = new();
    private static DateTime _spawnSkillMetadataExpiresUtc = DateTime.MinValue;
    private static long _partyGroupingVersion;

    public static bool IsRunning => _shutdown != null && !_shutdown.IsCancellationRequested;
    public static int SessionCount
    {
        get
        {
            lock (SessionsLock)
                return Sessions.Count;
        }
    }

    public static async Task<string> StartAsync(int? maxAccounts = null, string? city = null)
    {
        await LifecycleLock.WaitAsync();
        try
        {
            city = NormalizeCity(city);
            var botConfig = await BotProtectionService.GetConfigAsync();
            if (!botConfig.AllowBotLogin)
            {
                Log.Warning("Clientless startup blocked because AllowBotLogin=False.");
                return "Clientless startup blocked: AllowBotLogin is disabled.";
            }

            await EnsureSchemaAsync();
            var accounts = await LoadAccountsAsync(maxAccounts, city);
            if (accounts.Count == 0)
            {
                var scope = FormatScope(city);
                Log.Information("Clientless startup skipped: no enabled accounts in scope {Scope}.", scope);
                return $"Clientless startup skipped: no enabled accounts in {scope}.";
            }

            var gateway = ServerManager.FindConfiguredService(ServerType.GatewayServer);
            if (gateway == null)
            {
                Log.Warning("Clientless startup skipped: no GatewayServer service is configured.");
                return "Clientless startup skipped: no GatewayServer service is configured.";
            }

            _shutdown ??= new CancellationTokenSource();
            var missing = ReserveMissingAccounts(accounts);
            if (missing.Count == 0)
                return $"No additional accounts to start in {FormatScope(city)}. {SessionCount} session(s) are already active.";

            var cancellationToken = _shutdown.Token;
            _launcherTask = Task.Run(() => LaunchSessionsAsync(missing, gateway, cancellationToken), cancellationToken);

            Log.Information("Clientless startup queued {Count} account(s) for {Scope}.", missing.Count, FormatScope(city));
            return $"Clientless queued {missing.Count} account(s) from {FormatScope(city)}.";
        }
        finally
        {
            LifecycleLock.Release();
        }
    }

    public static async Task<string> ReloadAsync(int? maxAccounts = null, string? city = null)
    {
        await StopAsync(city);
        // AgentServer keeps the previous character session briefly after the
        // managed socket closes. Reusing a fresh Gateway token immediately can
        // race that cleanup and produce A103 auth failures/reconnect loops.
        await Task.Delay(TimeSpan.FromSeconds(5));
        return await StartAsync(maxAccounts, city);
    }

    public static async Task<string> RefreshHuntingAsync(string? city = null, int? accountId = null)
    {
        city = NormalizeCity(city);
        await EnsureSchemaAsync();
        var accounts = await LoadAccountsAsync(null, city);
        if (accountId is > 0)
            accounts = accounts.Where(account => account.ID == accountId.Value).ToList();

        var byId = accounts.ToDictionary(account => account.ID);
        _huntPolicyExpiresUtc = DateTime.MinValue;
        ClientlessSession[] sessions;
        PendingClientlessLaunch[] pending;
        lock (SessionsLock)
        {
            sessions = Sessions
                .Where(session => string.IsNullOrWhiteSpace(session.SystemRole) &&
                                  byId.ContainsKey(session.AccountId))
                .ToArray();
            pending = PendingLaunches.Values
                .Where(launch => byId.ContainsKey(launch.Account.ID))
                .ToArray();
        }

        foreach (var session in sessions)
            session.RefreshHuntingConfiguration(byId[session.AccountId]);
        foreach (var launch in pending)
            CopyHuntingConfiguration(launch.Account, byId[launch.Account.ID]);

        var scope = accountId is > 0 ? $"account row {accountId.Value}" : FormatScope(city);
        Log.Information(
            "Clientless hunting configuration refreshed live for {SessionCount} active session(s) and {PendingCount} pending launch(es) in {Scope}.",
            sessions.Length,
            pending.Length,
            scope);
        return $"Applied hunting settings live to {sessions.Length} active session(s) in {scope}; no accounts were disconnected.";
    }

    public static async Task<string> StopAsync(string? city = null)
    {
        city = NormalizeCity(city);
        await LifecycleLock.WaitAsync();
        try
        {
            List<ClientlessSession> stopped;
            var noRemainingSessions = false;
            lock (SessionsLock)
            {
                stopped = Sessions
                    .Where(session => string.IsNullOrWhiteSpace(session.SystemRole) &&
                                      (city == null || string.Equals(session.City, city, StringComparison.OrdinalIgnoreCase)))
                    .ToList();
                foreach (var session in stopped)
                    Sessions.Remove(session);

                var pendingIds = PendingLaunches
                    .Where(item => city == null || string.Equals(item.Value.City, city, StringComparison.OrdinalIgnoreCase))
                    .Select(item => item.Key)
                    .ToArray();
                foreach (var accountId in pendingIds)
                    PendingLaunches.Remove(accountId);

                noRemainingSessions = Sessions.Count == 0;
            }

            if (city == null && noRemainingSessions)
            {
                try { _shutdown?.Cancel(); } catch { }
                _launcherTask = null;
                _shutdown?.Dispose();
                _shutdown = null;
            }

            foreach (var session in stopped)
            {
                session.Dispose();
                await UpdateStatusAsync(
                    session.AccountId,
                    "Stopped",
                    city == null ? "Stopped all managed Clientless sessions" : $"Stopped city group: {city}");
            }

            Log.Information("Clientless stopped {Count} active session(s) for {Scope}.", stopped.Count, FormatScope(city));
            return city == null
                ? $"Stopped {stopped.Count} active managed clientless session(s) across all cities."
                : $"Stopped {stopped.Count} active clientless session(s) from {city}.";
        }
        finally
        {
            LifecycleLock.Release();
        }
    }

    public static async Task<string> StopAccountAsync(int accountId)
    {
        if (accountId <= 0)
            throw new ArgumentOutOfRangeException(nameof(accountId));

        await LifecycleLock.WaitAsync();
        try
        {
            ClientlessSession? stopped;
            lock (SessionsLock)
            {
                PendingLaunches.Remove(accountId);
                stopped = Sessions.FirstOrDefault(session =>
                    session.AccountId == accountId && string.IsNullOrWhiteSpace(session.SystemRole));
                if (stopped != null)
                    Sessions.Remove(stopped);
            }

            if (stopped == null)
                return $"Clientless account row {accountId} is not active.";

            stopped.Dispose();
            await UpdateStatusAsync(accountId, "Stopped", "Stopped from Account Manager");
            return $"Stopped clientless account row {accountId}.";
        }
        finally
        {
            LifecycleLock.Release();
        }
    }

    public static void Stop()
    {
        try { _shutdown?.Cancel(); } catch { }
        lock (SessionsLock)
        {
            foreach (var session in Sessions)
                session.Dispose();

            Sessions.Clear();
            PendingLaunches.Clear();
        }

        _launcherTask = null;
        _shutdown?.Dispose();
        _shutdown = null;
    }

    public static async Task<string> GetStatusAsync()
    {
        await EnsureSchemaAsync();

        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();

        var enabled = await connection.ExecuteScalarAsync<int>(
            @"SELECT COUNT(1) FROM [dbo].[Clientless_Accounts] WITH (NOLOCK)
              WHERE Enabled = 1
                AND NULLIF(LTRIM(RTRIM(ISNULL(SystemRole, ''))), '') IS NULL");
        var rows = await connection.QueryAsync<ClientlessStatusCount>(
            @"SELECT LastStatus, COUNT(1) AS Total
              FROM [dbo].[Clientless_Accounts] WITH (NOLOCK)
              WHERE Enabled = 1
              GROUP BY LastStatus
              ORDER BY LastStatus");
        var cityRows = await connection.QueryAsync<ClientlessCityCount>(
            @"SELECT ISNULL(NULLIF(LTRIM(RTRIM(City)), ''), 'Unassigned') AS City, COUNT(1) AS Total
              FROM [dbo].[Clientless_Accounts] WITH (NOLOCK)
              WHERE Enabled = 1
                AND NULLIF(LTRIM(RTRIM(ISNULL(SystemRole, ''))), '') IS NULL
              GROUP BY ISNULL(NULLIF(LTRIM(RTRIM(City)), ''), 'Unassigned')
              ORDER BY City");

        var grouped = rows
            .Select(row => $"{row.LastStatus}:{row.Total}")
            .DefaultIfEmpty("none")
            .ToArray();

        var cities = cityRows
            .Select(row => $"{row.City}:{row.Total}")
            .DefaultIfEmpty("none");

        return $"Clientless manager={(IsRunning ? "Running" : "Stopped")}, active={SessionCount}, enabled={enabled}, cities={string.Join(", ", cities)}, statuses={string.Join(", ", grouped)}";
    }

    public static async Task<PartyFormPolicy> GetPartyFormPolicyAsync()
    {
        await EnsureSchemaAsync();
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        var policy = await connection.QuerySingleAsync<PartyFormPolicy>(
            $@"SELECT Enabled,
                      ISNULL(NULLIF(LTRIM(RTRIM(Mode)), ''), '{PartyModeGroupsOf8}') AS Mode,
                      Title, MinLevel, MaxLevel, Purpose, SettingsFlag
               FROM {PartyFormSettingsTable} WITH (NOLOCK)
               WHERE SettingID = 1;");
        policy.Mode = NormalizePartyMode(policy.Mode);
        return policy;
    }

    public static async Task<string> SetPartyFormPolicyAsync(
        bool enabled,
        string mode,
        string title,
        byte minLevel,
        byte maxLevel,
        byte purpose,
        byte settingsFlag)
    {
        mode = NormalizePartyMode(mode);
        title = (title ?? string.Empty).Trim();
        if (title.Length is < 1 or > 64)
            throw new ArgumentException("Party Form title must contain 1 to 64 characters.", nameof(title));
        if (minLevel > maxLevel)
            throw new ArgumentException("Party Form minimum level cannot exceed maximum level.");

        await EnsureSchemaAsync();
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await connection.ExecuteAsync(
            $@"UPDATE {PartyFormSettingsTable}
               SET Enabled = @Enabled,
                   Mode = @Mode,
                   Title = @Title,
                   MinLevel = @MinLevel,
                   MaxLevel = @MaxLevel,
                   Purpose = @Purpose,
                   SettingsFlag = @SettingsFlag,
                   UpdatedAt = SYSDATETIME()
               WHERE SettingID = 1;",
            new
            {
                Enabled = enabled,
                Mode = mode,
                Title = title,
                MinLevel = minLevel,
                MaxLevel = maxLevel,
                Purpose = purpose,
                SettingsFlag = settingsFlag
            });

        var policy = new PartyFormPolicy(enabled, title, minLevel, maxLevel, purpose, settingsFlag, mode);
        Interlocked.Increment(ref _partyGroupingVersion);

        if (!enabled)
            return await ClearManagedPartyAutomationAsync(policy);

        if (mode == PartyModeSoloForms)
            return await ApplySoloPartyFormsAsync(policy, resetExistingState: true);

        ScheduleManagedPartyBuild(TimeSpan.FromMilliseconds(250));
        ClientlessSession[] sessions;
        lock (SessionsLock)
            sessions = Sessions.ToArray();
        return $"Groups of 8 mode saved. {sessions.Count(session => session.IsReadyForManagedParty):N0} online character(s) are queued by city; only each confirmed leader will publish a Party Form.";
    }

    internal static string NormalizePartyMode(string? mode)
    {
        if (string.IsNullOrWhiteSpace(mode) ||
            string.Equals(mode, PartyModeGroupsOf8, StringComparison.OrdinalIgnoreCase))
            return PartyModeGroupsOf8;
        if (string.Equals(mode, PartyModeSoloForms, StringComparison.OrdinalIgnoreCase))
            return PartyModeSoloForms;

        throw new ArgumentException(
            $"Unknown Clientless party mode '{mode}'. Use {PartyModeSoloForms} or {PartyModeGroupsOf8}.",
            nameof(mode));
    }

    private static async Task<string> ClearManagedPartyAutomationAsync(PartyFormPolicy policy)
    {
        await PartyGroupingLock.WaitAsync();
        try
        {
            var sessions = GetReadyPartySessions(null);
            var cancellationToken = _shutdown?.Token ?? CancellationToken.None;
            var removed = await ClearManagedPartyStateAsync(sessions, cancellationToken);
            foreach (var session in sessions)
                await session.ApplyPartyFormPolicyAsync(policy);

            return $"Party automation disabled. Removed {removed:N0} Party Form request(s) and left all managed Clientless parties.";
        }
        finally
        {
            PartyGroupingLock.Release();
        }
    }

    private static async Task<string> ApplySoloPartyFormsAsync(
        PartyFormPolicy policy,
        bool resetExistingState)
    {
        await PartyGroupingLock.WaitAsync();
        try
        {
            var sessions = GetReadyPartySessions(null);
            var cancellationToken = _shutdown?.Token ?? CancellationToken.None;
            if (resetExistingState)
                await ClearManagedPartyStateAsync(sessions, cancellationToken);
            else
            {
                // A reconnect can restore a server-side party that existed
                // before Solo mode was selected. Clean only those stale
                // memberships without disturbing valid solo forms on every
                // other online character.
                var stalePartySessions = sessions
                    .Where(session => session.HasManagedPartyMembership)
                    .ToArray();
                if (stalePartySessions.Length > 0)
                    await ClearManagedPartyStateAsync(stalePartySessions, cancellationToken);
            }

            var submitted = 0;
            foreach (var session in sessions)
            {
                if (await session.ApplyPartyFormPolicyAsync(policy))
                    submitted++;
            }

            return $"Solo Party Forms mode applied. {submitted:N0} of {sessions.Length:N0} online character(s) submitted an individual Party Form and no managed Clientless parties remain.";
        }
        finally
        {
            PartyGroupingLock.Release();
        }
    }

    private static ClientlessSession[] GetReadyPartySessions(string? city)
    {
        lock (SessionsLock)
        {
            return Sessions
                .Where(session => string.IsNullOrWhiteSpace(session.SystemRole) &&
                                  session.IsReadyForManagedParty &&
                                  (city == null || string.Equals(session.City, city, StringComparison.OrdinalIgnoreCase)))
                .OrderBy(session => session.City, StringComparer.OrdinalIgnoreCase)
                .ThenBy(session => session.HuntAreaId ?? int.MaxValue)
                .ThenBy(session => session.AccountId)
                .ToArray();
        }
    }

    private static async Task<int> ClearManagedPartyStateAsync(
        ClientlessSession[] sessions,
        CancellationToken cancellationToken)
    {
        var removeRequests = sessions.Count(session => session.HasManagedPartyForm);
        foreach (var session in sessions)
            await session.RemoveManagedPartyFormAsync(cancellationToken);

        await Task.WhenAll(sessions.Select(session =>
            session.WaitForManagedPartyFormRemovalAsync(TimeSpan.FromSeconds(3), cancellationToken)));

        foreach (var session in sessions)
            await session.LeaveManagedPartyAsync(cancellationToken);
        if (sessions.Length > 0)
            await Task.Delay(500, cancellationToken);

        return removeRequests;
    }

    public static async Task<string> BuildManagedPartiesAsync(string? city = null)
    {
        await PartyGroupingLock.WaitAsync();
        try
        {
            return await BuildManagedPartiesCoreAsync(city);
        }
        finally
        {
            PartyGroupingLock.Release();
        }
    }

    private static async Task<string> BuildManagedPartiesCoreAsync(string? city)
    {
        city = NormalizeCity(city);
        var policy = await GetPartyFormPolicyAsync();
        if (!policy.Enabled)
            return "Party automation is disabled. Select Groups of 8 and press Save & Apply first.";
        if (policy.Mode != PartyModeGroupsOf8)
            return "Solo Party Forms mode is active. Select Groups of 8 and press Save & Apply before building grouped parties.";
        var settingsFlag = (byte)(policy.SettingsFlag | 0x04); // invitation enabled => eight-member vSRO party
        var cancellationToken = _shutdown?.Token ?? CancellationToken.None;

        var readySessions = GetReadyPartySessions(city);

        if (readySessions.Length == 0)
            return $"No fully online Clientless characters are ready for party grouping in {FormatScope(city)}.";

        // Individual listings cannot represent a real grouped party. Remove
        // every stale form first; after confirmations only each party leader
        // publishes a form linked to the GameServer party ID.
        await ClearManagedPartyStateAsync(readySessions, cancellationToken);

        var groups = BuildManagedPartyGroups(readySessions, session => session.City);

        var completedParties = 0;
        var groupedCharacters = 0;
        var failedInvites = 0;
        var publishedPartyForms = 0;
        var standaloneRemainders = 0;
        foreach (var group in groups)
        {
            if (group.Length == 0)
                continue;

            var leader = group[0];
            var actualSize = 1;
            for (var index = 1; index < group.Length; index++)
            {
                var member = group[index];
                member.ExpectManagedPartyInvite(leader.SelfUniqueId);
                await leader.InviteManagedPartyMemberAsync(
                    member.SelfUniqueId,
                    settingsFlag,
                    cancellationToken);

                if (await leader.WaitForManagedPartySizeAsync(
                        actualSize + 1,
                        TimeSpan.FromSeconds(4),
                        cancellationToken))
                {
                    actualSize++;
                }
                else
                {
                    failedInvites++;
                    Log.Warning(
                        "Managed Clientless party leader {Leader} did not receive join confirmation for {Member}.",
                        leader.CharacterName,
                        member.CharacterName);
                }
            }

            if (actualSize > 1)
            {
                completedParties++;
                groupedCharacters += actualSize;
                if (await leader.PublishManagedPartyFormAsync(policy, cancellationToken))
                    publishedPartyForms++;
            }
            else
            {
                // A city's final remainder may contain only one online
                // character. It still needs a visible Party Matching form;
                // party id 0 is the native vSRO create-party listing.
                standaloneRemainders++;
                if (await leader.ApplyPartyFormPolicyAsync(policy))
                    publishedPartyForms++;
            }
        }

        var result = $"Created {completedParties:N0} real vSRO party/parties for {groupedCharacters:N0} Clientless character(s) in {FormatScope(city)}; each party is capped at 8 and {publishedPartyForms:N0} leader Party Form(s) were submitted.";
        if (standaloneRemainders > 0)
            result += $" {standaloneRemainders:N0} single-character city remainder(s) also received a visible Party Form.";
        if (failedInvites > 0)
            result += $" {failedInvites:N0} invite(s) were not confirmed; keep those characters online and press Create Parties again.";
        return result;
    }

    internal static T[][] BuildManagedPartyGroups<T>(
        IEnumerable<T> candidates,
        Func<T, string> citySelector)
    {
        ArgumentNullException.ThrowIfNull(candidates);
        ArgumentNullException.ThrowIfNull(citySelector);
        return candidates
            .GroupBy(
                candidate => NormalizeCity(citySelector(candidate)) ?? "Unassigned",
                StringComparer.OrdinalIgnoreCase)
            .SelectMany(cityGroup => cityGroup.Chunk(8))
            .ToArray();
    }

    internal static void ScheduleManagedPartyBuild(TimeSpan? delay = null)
    {
        var version = Interlocked.Increment(ref _partyGroupingVersion);
        var cancellationToken = _shutdown?.Token ?? CancellationToken.None;
        _ = Task.Run(async () =>
        {
            try
            {
                // Every login updates the generation. Only the last login in
                // the burst performs the rebuild, after AgentServer has sent
                // the stable character UID and party state to every session.
                await Task.Delay(delay ?? TimeSpan.FromSeconds(6), cancellationToken);
                if (version != Volatile.Read(ref _partyGroupingVersion))
                    return;

                var policy = await GetPartyFormPolicyAsync();
                if (!policy.Enabled)
                    return;

                var result = policy.Mode == PartyModeSoloForms
                    ? await ApplySoloPartyFormsAsync(policy, resetExistingState: false)
                    : await BuildManagedPartiesAsync();
                Log.Information("Automatic managed Clientless party policy applied: {Result}", result);
            }
            catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
            {
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Automatic managed Clientless party grouping failed without interrupting online sessions.");
            }
        });
    }

    private sealed class ClientlessStatusCount
    {
        public string LastStatus { get; set; } = string.Empty;
        public int Total { get; set; }
    }

    private sealed class ClientlessCityCount
    {
        public string City { get; set; } = string.Empty;
        public int Total { get; set; }
    }

    public static async Task<string> UpsertAccountAsync(
        string accountName,
        string password,
        string characterName,
        ushort shardId,
        byte locale,
        string city = "Unassigned")
    {
        if (string.IsNullOrWhiteSpace(accountName))
            throw new ArgumentException("Account name is required.", nameof(accountName));
        if (string.IsNullOrWhiteSpace(characterName))
            throw new ArgumentException("Character name is required.", nameof(characterName));

        await EnsureSchemaAsync();
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();

        await connection.ExecuteAsync(
            @"IF EXISTS (SELECT 1 FROM [dbo].[Clientless_Accounts] WHERE AccountName = @AccountName AND CharacterName = @CharacterName)
              BEGIN
                  UPDATE [dbo].[Clientless_Accounts]
                  SET Enabled = 1,
                      AccountPassword = @AccountPassword,
                      ShardID = @ShardID,
                      Locale = @Locale,
                      City = @City,
                      LastStatus = 'Pending',
                      LastMessage = NULL,
                      UpdatedAt = SYSDATETIME()
                  WHERE AccountName = @AccountName AND CharacterName = @CharacterName;
              END
              ELSE
              BEGIN
                  INSERT INTO [dbo].[Clientless_Accounts]
                      (Enabled, Locale, ShardID, AccountName, AccountPassword, CharacterName, City, LastStatus)
                  VALUES
                      (1, @Locale, @ShardID, @AccountName, @AccountPassword, @CharacterName, @City, 'Pending');
              END",
            new
            {
                AccountName = accountName.Trim(),
                AccountPassword = password,
                CharacterName = characterName.Trim(),
                ShardID = shardId,
                Locale = locale,
                City = string.IsNullOrWhiteSpace(city) ? "Unassigned" : city.Trim()
            });

        return $"Clientless account saved: {accountName.Trim()}/{characterName.Trim()}.";
    }

    public static async Task<string> SetEnabledAsync(int id, bool enabled)
    {
        await EnsureSchemaAsync();
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();

        var affected = await connection.ExecuteAsync(
            @"UPDATE [dbo].[Clientless_Accounts]
              SET Enabled = @Enabled,
                  LastStatus = CASE WHEN @Enabled = 1 THEN 'Pending' ELSE 'Disabled' END,
                  LastMessage = NULL,
                  UpdatedAt = SYSDATETIME()
              WHERE ID = @ID",
            new { ID = id, Enabled = enabled });

        return affected == 0
            ? $"Clientless account row {id} was not found."
            : $"Clientless account row {id} {(enabled ? "enabled" : "disabled")}.";
    }

    public static async Task<string> EnsureSystemAccountRunningAsync(string systemRole)
    {
        systemRole = (systemRole ?? string.Empty).Trim().ToUpperInvariant();
        if (systemRole.Length == 0)
            throw new ArgumentException("System role is required.", nameof(systemRole));

        await LifecycleLock.WaitAsync();
        try
        {
            await EnsureSchemaAsync();
            var account = await LoadSystemAccountAsync(systemRole);
            if (account == null)
                return $"System clientless role {systemRole} is not configured.";

            ClientlessSession? staleSession = null;
            lock (SessionsLock)
            {
                var existing = Sessions.FirstOrDefault(session =>
                    string.Equals(session.SystemRole, systemRole, StringComparison.OrdinalIgnoreCase));
                if (existing != null)
                {
                    if (existing.Matches(account))
                        return $"System clientless role {systemRole} is already running.";

                    Sessions.Remove(existing);
                    staleSession = existing;
                }
            }
            staleSession?.Dispose();

            var gateway = ServerManager.FindConfiguredService(ServerType.GatewayServer);
            if (gateway == null)
                return $"System clientless role {systemRole} is waiting for the Gateway service.";

            _shutdown ??= new CancellationTokenSource();
            var session = new ClientlessSession(account, gateway);
            lock (SessionsLock)
                Sessions.Add(session);

            var cancellationToken = _shutdown.Token;
            _ = Task.Run(() => session.RunAsync(cancellationToken), cancellationToken);
            Log.Information(
                "System clientless launched. Role={SystemRole} Account={Account} Character={Character}",
                systemRole,
                account.AccountName,
                account.CharacterName);
            return $"System clientless role {systemRole} queued.";
        }
        finally
        {
            LifecycleLock.Release();
        }
    }

    public static async Task<string> StopSystemAccountAsync(string systemRole)
    {
        systemRole = (systemRole ?? string.Empty).Trim().ToUpperInvariant();
        if (systemRole.Length == 0)
            throw new ArgumentException("System role is required.", nameof(systemRole));

        ClientlessSession? session;
        lock (SessionsLock)
        {
            session = Sessions.FirstOrDefault(item =>
                string.Equals(item.SystemRole, systemRole, StringComparison.OrdinalIgnoreCase));
            if (session != null)
                Sessions.Remove(session);
        }

        if (session == null)
            return $"System clientless role {systemRole} is already stopped.";

        session.Dispose();
        await UpdateStatusAsync(session.AccountId, "Stopped", "Stopped manually");
        Log.Information(
            "System clientless stopped manually. Role={SystemRole} Account={Account} Character={Character}",
            systemRole,
            session.AccountName,
            session.CharacterName);
        return $"System clientless role {systemRole} stopped.";
    }

    private static async Task LaunchSessionsAsync(
        IReadOnlyList<PendingClientlessLaunch> launches,
        __ProxyServices gatewayService,
        CancellationToken cancellationToken)
    {
        try
        {
            for (var index = 0; index < launches.Count; index++)
            {
                if (cancellationToken.IsCancellationRequested)
                    break;

                var launch = launches[index];
                var account = launch.Account;
                var session = new ClientlessSession(account, gatewayService);
                lock (SessionsLock)
                {
                    if (!PendingLaunches.TryGetValue(account.ID, out var pending) || pending.Version != launch.Version)
                    {
                        session.Dispose();
                        continue;
                    }

                    PendingLaunches.Remove(account.ID);
                    if (Sessions.Any(existing => existing.AccountId == account.ID))
                    {
                        session.Dispose();
                        continue;
                    }

                    Sessions.Add(session);
                }

                _ = Task.Run(() => session.RunAsync(cancellationToken), cancellationToken);
                Log.Information("Clientless launched {Index}/{Total}: {Account}/{Character} City={City}",
                    index + 1,
                    launches.Count,
                    account.AccountName,
                    account.CharacterName,
                    account.City);

                var delay = Math.Clamp(account.LaunchDelayMs, 0, 60000);
                if (delay > 0 && index + 1 < launches.Count)
                    await Task.Delay(delay, cancellationToken);
            }
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
        }
        finally
        {
            lock (SessionsLock)
            {
                foreach (var launch in launches)
                {
                    if (PendingLaunches.TryGetValue(launch.Account.ID, out var pending) &&
                        pending.Version == launch.Version)
                        PendingLaunches.Remove(launch.Account.ID);
                }
            }
        }
    }

    private static List<PendingClientlessLaunch> ReserveMissingAccounts(IReadOnlyList<ClientlessAccount> accounts)
    {
        lock (SessionsLock)
        {
            var launchedIds = Sessions.Select(session => session.AccountId).ToHashSet();
            var version = Interlocked.Increment(ref _launchVersion);
            var launches = new List<PendingClientlessLaunch>();
            foreach (var account in accounts)
            {
                if (launchedIds.Contains(account.ID) || PendingLaunches.ContainsKey(account.ID))
                    continue;

                var launch = new PendingClientlessLaunch(account, version, account.City);
                PendingLaunches[account.ID] = launch;
                launches.Add(launch);
            }

            return launches;
        }
    }

    private static async Task<List<ClientlessAccount>> LoadAccountsAsync(int? maxAccounts, string? city)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        var loadedAccounts = (await connection.QueryAsync<ClientlessAccount>(
            @"SELECT
                     ID, Enabled, Locale, ShardID, AccountName, AccountPassword, CharacterName,
                     AgentAuthMode, AgentAuthDelayMs, AgentAuthPaddingHex, LaunchDelayMs, ReconnectDelaySeconds,
                     ISNULL(NULLIF(LTRIM(RTRIM(City)), ''), 'Unassigned') AS City,
                     ISNULL(SystemRole, '') AS SystemRole, HuntEnabled,
                     HomeRegionID, HomeX, HomeY, HomeZ
              FROM [dbo].[Clientless_Accounts] WITH (NOLOCK)
              WHERE Enabled = 1
                AND NULLIF(LTRIM(RTRIM(ISNULL(SystemRole, ''))), '') IS NULL
                AND (@City IS NULL OR City = @City)
              ORDER BY ID",
            new { City = city })).ToList();

        // A single vSRO account can own only one active character session, and
        // a character cannot be logged by two account rows. Keep a stable first
        // row for each identity so an accidental duplicate dashboard/import row
        // cannot kick the same character in an endless reconnect loop.
        var seenAccounts = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var seenCharacters = new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        var accounts = new List<ClientlessAccount>();
        foreach (var account in loadedAccounts.OrderBy(item => item.ID))
        {
            if (seenAccounts.Contains(account.AccountName) || seenCharacters.Contains(account.CharacterName))
            {
                Log.Warning(
                    "Clientless row {ID} ({Account}/{Character}) was skipped because an earlier enabled row already owns the same account or character identity.",
                    account.ID,
                    account.AccountName,
                    account.CharacterName);
                continue;
            }

            seenAccounts.Add(account.AccountName);
            seenCharacters.Add(account.CharacterName);
            accounts.Add(account);
            if (accounts.Count >= Math.Clamp(maxAccounts ?? int.MaxValue, 1, int.MaxValue))
                break;
        }

        foreach (var cityAccounts in accounts.GroupBy(item => item.City, StringComparer.OrdinalIgnoreCase))
        {
            var orderedAccounts = cityAccounts.OrderBy(item => item.ID).ToArray();
            var townPositions = ClientlessTownPositions.ResolveAll(cityAccounts.Key, orderedAccounts.Length);
            for (var index = 0; index < orderedAccounts.Length; index++)
            {
                var position = townPositions[index];
                orderedAccounts[index].TownParkingRegionID = position.RegionID;
                orderedAccounts[index].TownParkingX = position.X;
                orderedAccounts[index].TownParkingY = position.Y;
                orderedAccounts[index].TownParkingZ = position.Z;
            }
        }

        var areas = (await connection.QueryAsync<ClientlessHuntArea>(
            @"SELECT ID, City, SlotNumber, DisplayName, RegionID, PosX, PosY, PosZ, Radius
              FROM dbo.Clientless_HuntAreas WITH (NOLOCK)
              WHERE Enabled = 1 AND RegionID > 0
                AND (@City IS NULL OR City = @City)
              ORDER BY City, SlotNumber;",
            new { City = city })).ToList();

        foreach (var cityAccounts in accounts.GroupBy(item => item.City, StringComparer.OrdinalIgnoreCase))
        {
            var cityAreas = areas.Where(area => area.City.Equals(cityAccounts.Key, StringComparison.OrdinalIgnoreCase)).ToArray();
            if (cityAreas.Length == 0)
                continue;

            var index = 0;
            foreach (var account in cityAccounts.OrderBy(item => item.ID))
            {
                var area = cityAreas[index++ % cityAreas.Length];
                account.HuntAreaID = area.ID;
                account.HuntAreaName = area.DisplayName;
                account.HuntRegionID = area.RegionID;
                account.HuntX = area.PosX;
                account.HuntY = area.PosY;
                account.HuntZ = area.PosZ;
                account.HuntRadius = area.Radius;
            }
        }

        return accounts;
    }

    internal static bool CopyHuntingConfiguration(ClientlessAccount destination, ClientlessAccount source)
    {
        var assignedAreaChanged = destination.HuntAreaID != source.HuntAreaID ||
                                  destination.HuntRegionID != source.HuntRegionID ||
                                  Math.Abs(destination.HuntX - source.HuntX) >= 0.1f ||
                                  Math.Abs(destination.HuntY - source.HuntY) >= 0.1f ||
                                  Math.Abs(destination.HuntZ - source.HuntZ) >= 0.1f ||
                                  Math.Abs(destination.HuntRadius - source.HuntRadius) >= 0.1f;

        destination.HuntEnabled = source.HuntEnabled;
        destination.HuntAreaID = source.HuntAreaID;
        destination.HuntAreaName = source.HuntAreaName;
        destination.HuntRegionID = source.HuntRegionID;
        destination.HuntX = source.HuntX;
        destination.HuntY = source.HuntY;
        destination.HuntZ = source.HuntZ;
        destination.HuntRadius = source.HuntRadius;
        destination.TownParkingRegionID = source.TownParkingRegionID;
        destination.TownParkingX = source.TownParkingX;
        destination.TownParkingY = source.TownParkingY;
        destination.TownParkingZ = source.TownParkingZ;
        return assignedAreaChanged;
    }

    private static async Task<ClientlessAccount?> LoadSystemAccountAsync(string systemRole)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        return await connection.QueryFirstOrDefaultAsync<ClientlessAccount>(
            @"SELECT TOP (1)
                     ID, Enabled, Locale, ShardID, AccountName, AccountPassword, CharacterName,
                     AgentAuthMode, AgentAuthDelayMs, AgentAuthPaddingHex, LaunchDelayMs,
                     ReconnectDelaySeconds, ISNULL(NULLIF(LTRIM(RTRIM(City)), ''), 'Unassigned') AS City,
                     ISNULL(SystemRole, '') AS SystemRole, HuntEnabled,
                     HomeRegionID, HomeX, HomeY, HomeZ
              FROM [dbo].[Clientless_Accounts] WITH (NOLOCK)
              WHERE Enabled = 1 AND SystemRole = @SystemRole
              ORDER BY ID",
            new { SystemRole = systemRole });
    }

    private static async Task EnsureSchemaAsync()
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await connection.ExecuteAsync(@"
IF OBJECT_ID(N'dbo.Clientless_Accounts', N'U') IS NOT NULL
BEGIN
    IF COL_LENGTH(N'dbo.Clientless_Accounts', N'HomeRegionID') IS NULL
        ALTER TABLE dbo.Clientless_Accounts ADD HomeRegionID INT NULL;
    IF COL_LENGTH(N'dbo.Clientless_Accounts', N'HomeX') IS NULL
        ALTER TABLE dbo.Clientless_Accounts ADD HomeX REAL NULL;
    IF COL_LENGTH(N'dbo.Clientless_Accounts', N'HomeY') IS NULL
        ALTER TABLE dbo.Clientless_Accounts ADD HomeY REAL NULL;
    IF COL_LENGTH(N'dbo.Clientless_Accounts', N'HomeZ') IS NULL
        ALTER TABLE dbo.Clientless_Accounts ADD HomeZ REAL NULL;
END;");
        await connection.ExecuteAsync(@"
IF OBJECT_ID(N'dbo.ClientlessPartyFormSettings', N'U') IS NOT NULL
   AND COL_LENGTH(N'dbo.ClientlessPartyFormSettings', N'Mode') IS NULL
BEGIN
    ALTER TABLE dbo.ClientlessPartyFormSettings ADD Mode VARCHAR(16) NOT NULL
        CONSTRAINT DF_ClientlessPartyForm_Mode DEFAULT ('GroupsOf8') WITH VALUES;
END;");
        var ready = await connection.ExecuteScalarAsync<int>(@"
SELECT CASE WHEN OBJECT_ID(N'dbo.Clientless_Accounts',N'U') IS NOT NULL
                  AND OBJECT_ID(N'dbo.ClientlessPartyFormSettings',N'U') IS NOT NULL
                  AND COL_LENGTH(N'dbo.ClientlessPartyFormSettings',N'Mode') IS NOT NULL
                  AND OBJECT_ID(N'dbo.Clientless_HuntAreas',N'U') IS NOT NULL
                  AND OBJECT_ID(N'dbo.Clientless_HuntPolicy',N'U') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Clientless_Accounts',N'AgentAuthMode') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Clientless_Accounts',N'AgentAuthDelayMs') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Clientless_Accounts',N'AgentAuthPaddingHex') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Clientless_Accounts',N'LaunchDelayMs') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Clientless_Accounts',N'City') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Clientless_Accounts',N'SystemRole') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Clientless_Accounts',N'HuntEnabled') IS NOT NULL
                  AND COL_LENGTH(N'dbo.Clientless_Accounts',N'HuntStatus') IS NOT NULL
             THEN 1 ELSE 0 END;");
        if (ready != 1)
            throw new InvalidOperationException(
                "Clientless schema is incomplete. Apply the packaged database updates before startup.");
    }

    internal static async Task UpdateStatusAsync(int id, string status, string message)
    {
        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();

            var online = status.Equals("Online", StringComparison.OrdinalIgnoreCase);
            var disconnected = status.Equals("Disconnected", StringComparison.OrdinalIgnoreCase);
            await connection.ExecuteAsync(
                @"UPDATE [dbo].[Clientless_Accounts]
                  SET LastStatus = @Status,
                      LastMessage = @Message,
                      LastLoginAt = CASE WHEN @Online = 1 THEN SYSDATETIME() ELSE LastLoginAt END,
                      LastDisconnectAt = CASE WHEN @Disconnected = 1 THEN SYSDATETIME() ELSE LastDisconnectAt END,
                      UpdatedAt = SYSDATETIME()
                  WHERE ID = @ID",
                new { ID = id, Status = status, Message = message, Online = online, Disconnected = disconnected });
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to update clientless status for account row {ID}", id);
        }
    }

    internal static async Task<ClientlessHuntPolicy> GetHuntPolicyAsync(bool force = false)
    {
        if (!force && DateTime.UtcNow < _huntPolicyExpiresUtc)
            return _huntPolicy;

        await HuntPolicyLock.WaitAsync();
        try
        {
            if (!force && DateTime.UtcNow < _huntPolicyExpiresUtc)
                return _huntPolicy;

            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            _huntPolicy = await connection.QuerySingleAsync<ClientlessHuntPolicy>(
                @"SELECT Enabled, AttackNormal, AttackUnique, UniquePriority, UseSkills, UseBasicAttack,
                         HpPotionPercent, MpPotionPercent, StuckSeconds, TargetTimeoutSeconds
                  FROM dbo.Clientless_HuntPolicy WITH (NOLOCK)
                  WHERE SettingID = 1;");
            _huntPolicyExpiresUtc = DateTime.UtcNow.AddSeconds(3);
            return _huntPolicy;
        }
        finally
        {
            HuntPolicyLock.Release();
        }
    }

    internal static async Task<IReadOnlyList<ClientlessCombatSkill>> LoadCombatSkillsAsync(string characterName)
    {
        try
        {
            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
            var combatSkillPredicate = BuildMonsterAttackSkillPredicate("Skill");
            var weaponCompatibilityPredicate = BuildSkillWeaponCompatibilityPredicate("Skill", "Weapon", "Offhand");
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            var rows = await connection.QueryAsync<ClientlessCombatSkill>($@"
SELECT CONVERT(BIGINT, Skill.ID) AS ID,
       Skill.Basic_Code AS CodeName128,
       Skill.Basic_Level AS BasicLevel,
       ISNULL(Skill.GroupID, 0) AS GroupID,
       CASE WHEN ISNULL(Skill.Action_ReuseDelay, 0) > ISNULL(Skill.Action_CoolTime, 0)
            THEN ISNULL(Skill.Action_ReuseDelay, 0) ELSE ISNULL(Skill.Action_CoolTime, 0) END AS CooldownMs,
       ISNULL(Skill.Action_PreparingTime, 0) + ISNULL(Skill.Action_CastingTime, 0) +
       ISNULL(Skill.Action_ActionDuration, 0) AS CastDurationMs,
       ISNULL(Skill.Action_Range, 0) AS ActionRange,
       ISNULL(Skill.Consume_MP, 0) AS ConsumeMp,
       ISNULL(Skill.ReqCast_Weapon1, 255) AS ReqCastWeapon1,
       ISNULL(Skill.ReqCast_Weapon2, 255) AS ReqCastWeapon2,
       CONVERT(BIT, 0) AS TargetsSelf,
       CONVERT(BIT, 0) AS IsImbue
FROM {shardDb}.dbo._Char C WITH (NOLOCK)
INNER JOIN {shardDb}.dbo._CharSkill Learned WITH (NOLOCK) ON Learned.CharID = C.CharID AND Learned.Enable = 1
INNER JOIN {shardDb}.dbo._RefSkill Skill WITH (NOLOCK) ON Skill.ID = Learned.SkillID
INNER JOIN {shardDb}.dbo._Inventory WeaponSlot WITH (NOLOCK) ON WeaponSlot.CharID = C.CharID AND WeaponSlot.Slot = 6 AND WeaponSlot.ItemID > 0
INNER JOIN {shardDb}.dbo._Items WeaponItem WITH (NOLOCK) ON WeaponItem.ID64 = WeaponSlot.ItemID AND WeaponItem.Data > 0
INNER JOIN {shardDb}.dbo._RefObjCommon Weapon WITH (NOLOCK) ON Weapon.ID = WeaponItem.RefItemID
LEFT JOIN {shardDb}.dbo._Inventory OffhandSlot WITH (NOLOCK) ON OffhandSlot.CharID = C.CharID AND OffhandSlot.Slot = 7 AND OffhandSlot.ItemID > 0
LEFT JOIN {shardDb}.dbo._Items OffhandItem WITH (NOLOCK) ON OffhandItem.ID64 = OffhandSlot.ItemID AND OffhandItem.Data > 0
LEFT JOIN {shardDb}.dbo._RefObjCommon Offhand WITH (NOLOCK) ON Offhand.ID = OffhandItem.RefItemID
WHERE C.CharName16 = @CharacterName
  AND Skill.Service = 1
  AND ({combatSkillPredicate})
  AND ISNULL(Skill.ReqCommon_Mastery1, 0) = CASE
      WHEN Weapon.TypeID4 IN (2, 3) THEN 257
      WHEN Weapon.TypeID4 IN (4, 5) THEN 258
      WHEN Weapon.TypeID4 = 6 THEN 259
      WHEN Weapon.TypeID4 IN (7, 8, 9) THEN 513
      WHEN Weapon.TypeID4 = 11 THEN 514
      WHEN Weapon.TypeID4 IN (12, 13) THEN 515
      WHEN Weapon.TypeID4 = 10 THEN 516
      WHEN Weapon.TypeID4 = 14 THEN 517
      WHEN Weapon.TypeID4 = 15 THEN 518
      ELSE -1 END
  AND Skill.Basic_Code NOT LIKE '%DOWNATTACK%'
  AND Skill.Basic_Code NOT LIKE '%RESURRECT%'
  AND Skill.Basic_Code NOT LIKE '%[_]BASE[_]%'
  AND Skill.Basic_Code NOT LIKE '%SACRIFICE%'
  AND Weapon.TypeID1 = 3 AND Weapon.TypeID2 = 1 AND Weapon.TypeID3 = 6
  AND ({weaponCompatibilityPredicate})
ORDER BY Skill.Basic_Level DESC, Skill.ID DESC;", new
            {
                CharacterName = characterName,
                AttackParam = AttackSkillParameter,
                RequiredItemParam = RequiredItemSkillParameter
            });
            var skills = KeepHighestSkillRankPerBranch(rows);
            Log.Debug(
                "Clientless {Character} loaded {SkillCount} learned monster-attack skill(s) for the equipped weapon.",
                characterName,
                skills.Length);
            return skills;
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Clientless {Character} skill catalog could not be loaded; basic attack fallback remains available.", characterName);
            return Array.Empty<ClientlessCombatSkill>();
        }
    }

    internal static async Task<IReadOnlyList<ClientlessCombatSkill>> LoadSelfBuffSkillsAsync(string characterName)
    {
        try
        {
            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
            var attackParamPredicate = BuildSkillParamPredicate("Skill", "@AttackParam");
            var selfBuffSkillPredicate = BuildSelfBuffSkillPredicate("Skill");
            var weaponCompatibilityPredicate = BuildSkillWeaponCompatibilityPredicate("Skill", "Weapon", "Offhand");
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            var rows = await connection.QueryAsync<ClientlessCombatSkill>($@"
SELECT CONVERT(BIGINT, Skill.ID) AS ID,
       Skill.Basic_Code AS CodeName128,
       Skill.Basic_Level AS BasicLevel,
       ISNULL(Skill.GroupID, 0) AS GroupID,
       CASE WHEN ISNULL(Skill.Action_ReuseDelay, 0) > ISNULL(Skill.Action_CoolTime, 0)
            THEN ISNULL(Skill.Action_ReuseDelay, 0) ELSE ISNULL(Skill.Action_CoolTime, 0) END AS CooldownMs,
       ISNULL(Skill.Action_PreparingTime, 0) + ISNULL(Skill.Action_CastingTime, 0) +
       ISNULL(Skill.Action_ActionDuration, 0) AS CastDurationMs,
       ISNULL(Skill.Action_Range, 0) AS ActionRange,
       ISNULL(Skill.Consume_MP, 0) AS ConsumeMp,
       ISNULL(Skill.ReqCast_Weapon1, 255) AS ReqCastWeapon1,
       ISNULL(Skill.ReqCast_Weapon2, 255) AS ReqCastWeapon2,
       CONVERT(BIT, CASE WHEN ISNULL(Skill.TargetGroup_Self, 0) = 1 OR ISNULL(Skill.TargetGroup_Party, 0) = 1
                         THEN 1 ELSE 0 END) AS TargetsSelf,
       CONVERT(BIT, CASE WHEN ISNULL(Skill.Basic_Activity, 0) = 1 AND ({attackParamPredicate})
                         THEN 1 ELSE 0 END) AS IsImbue
FROM {shardDb}.dbo._Char C WITH (NOLOCK)
INNER JOIN {shardDb}.dbo._CharSkill Learned WITH (NOLOCK) ON Learned.CharID = C.CharID AND Learned.Enable = 1
INNER JOIN {shardDb}.dbo._RefSkill Skill WITH (NOLOCK) ON Skill.ID = Learned.SkillID
INNER JOIN {shardDb}.dbo._Inventory WeaponSlot WITH (NOLOCK) ON WeaponSlot.CharID = C.CharID AND WeaponSlot.Slot = 6 AND WeaponSlot.ItemID > 0
INNER JOIN {shardDb}.dbo._Items WeaponItem WITH (NOLOCK) ON WeaponItem.ID64 = WeaponSlot.ItemID AND WeaponItem.Data > 0
INNER JOIN {shardDb}.dbo._RefObjCommon Weapon WITH (NOLOCK) ON Weapon.ID = WeaponItem.RefItemID
LEFT JOIN {shardDb}.dbo._Inventory OffhandSlot WITH (NOLOCK) ON OffhandSlot.CharID = C.CharID AND OffhandSlot.Slot = 7 AND OffhandSlot.ItemID > 0
LEFT JOIN {shardDb}.dbo._Items OffhandItem WITH (NOLOCK) ON OffhandItem.ID64 = OffhandSlot.ItemID AND OffhandItem.Data > 0
LEFT JOIN {shardDb}.dbo._RefObjCommon Offhand WITH (NOLOCK) ON Offhand.ID = OffhandItem.RefItemID
WHERE C.CharName16 = @CharacterName
  AND Skill.Service = 1
  AND ({selfBuffSkillPredicate})
  AND Skill.Basic_Code NOT LIKE '%PASSIVE%'
  AND Skill.Basic_Code NOT LIKE '%RESURRECT%'
  AND Skill.Basic_Code NOT LIKE '%TELEPORT%'
  AND Skill.Basic_Code NOT LIKE '%SUMMON%'
  AND Skill.Basic_Code NOT LIKE '%TRANSFORM%'
  AND Skill.Basic_Code NOT LIKE '%HEAL%'
  AND Weapon.TypeID1 = 3 AND Weapon.TypeID2 = 1 AND Weapon.TypeID3 = 6
  AND ({weaponCompatibilityPredicate})
ORDER BY CASE WHEN Skill.Basic_Code LIKE '%[_]FIRE[_]%' THEN 0
              WHEN Skill.Basic_Code LIKE '%[_]LIGHTNING[_]%' THEN 1 ELSE 2 END,
         Skill.Basic_Level DESC, Skill.ID DESC;", new
            {
                CharacterName = characterName,
                AttackParam = AttackSkillParameter,
                DurationParam = DurationSkillParameter,
                BuffPrimaryType = PrimaryBuffSkillType,
                RequiredItemParam = RequiredItemSkillParameter
            });
            var materialized = KeepHighestSkillRankPerBranch(rows);
            var preferredImbue = materialized.FirstOrDefault(skill => skill.IsImbue);
            var skills = materialized
                .Where(skill => !skill.IsImbue || skill.ID == preferredImbue?.ID)
                .ToArray();
            Log.Debug(
                "Clientless {Character} loaded {SkillCount} learned self-buff skill(s) for the equipped weapon.",
                characterName,
                skills.Length);
            return skills;
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Clientless {Character} self-buff catalog could not be loaded.", characterName);
            return Array.Empty<ClientlessCombatSkill>();
        }
    }

    internal static async Task<IReadOnlySet<uint>> LoadCreatorFlagBuffSkillIdsAsync()
    {
        if (DateTime.UtcNow < _spawnSkillMetadataExpiresUtc)
            return _creatorFlagBuffSkillIds;

        await SpawnSkillMetadataLock.WaitAsync();
        try
        {
            if (DateTime.UtcNow < _spawnSkillMetadataExpiresUtc)
                return _creatorFlagBuffSkillIds;

            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
            var predicate = BuildSkillParamPredicate("Skill", "@CreatorFlagParam");
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            var ids = await connection.QueryAsync<uint>($@"
SELECT CONVERT(BIGINT, Skill.ID)
FROM {shardDb}.dbo._RefSkill Skill WITH (NOLOCK)
WHERE Skill.Service = 1 AND ({predicate});",
                new { CreatorFlagParam = 1701213281 });
            _creatorFlagBuffSkillIds = ids.ToHashSet();
            _spawnSkillMetadataExpiresUtc = DateTime.UtcNow.AddMinutes(30);
            return _creatorFlagBuffSkillIds;
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Clientless spawn-skill metadata could not be loaded; creator-flag buffs will use compatibility parsing.");
            _creatorFlagBuffSkillIds = new HashSet<uint>();
            _spawnSkillMetadataExpiresUtc = DateTime.UtcNow.AddMinutes(1);
            return _creatorFlagBuffSkillIds;
        }
        finally
        {
            SpawnSkillMetadataLock.Release();
        }
    }

    private static string BuildSkillParamPredicate(string alias, string valueExpression) =>
        string.Join(" OR ", Enumerable.Range(1, 50).Select(index => $"ISNULL({alias}.Param{index}, 0) = {valueExpression}"));

    internal static string BuildMonsterAttackSkillPredicate(string alias)
    {
        var attackParamPredicate = BuildSkillParamPredicate(alias, "@AttackParam");
        return $@"
({attackParamPredicate})
AND ISNULL({alias}.TargetEtc_SelectDeadBody, 0) = 0
AND NOT
(
    ISNULL({alias}.Basic_Activity, 0) = 1
    AND ISNULL({alias}.Target_Required, 0) = 0
    AND ISNULL({alias}.TargetGroup_Enemy_M, 0) = 0
)";
    }

    internal static string BuildSelfBuffSkillPredicate(string alias)
    {
        var attackParamPredicate = BuildSkillParamPredicate(alias, "@AttackParam");
        var durationParamPredicate = BuildSkillParamPredicate(alias, "@DurationParam");
        return $@"
ISNULL({alias}.Basic_Activity, 0) <> 0
AND ({durationParamPredicate})
AND
(
    ISNULL({alias}.Param1, -1) = @BuffPrimaryType
    OR (ISNULL({alias}.Basic_Activity, 0) = 1 AND ({attackParamPredicate}))
)
AND ISNULL({alias}.TargetGroup_Enemy_M, 0) = 0
AND ISNULL({alias}.TargetGroup_Enemy_P, 0) = 0
AND ISNULL({alias}.TargetEtc_SelectDeadBody, 0) = 0
AND
(
    ISNULL({alias}.Target_Required, 0) = 0
    OR ISNULL({alias}.TargetGroup_Self, 0) = 1
    OR ISNULL({alias}.TargetGroup_Party, 0) = 1
)";
    }

    internal static string BuildSkillWeaponCompatibilityPredicate(
        string skillAlias,
        string weaponAlias,
        string offhandAlias)
    {
        var markerPredicate = BuildSkillParamPredicate(skillAlias, "@RequiredItemParam");
        var equippedItemMatches = string.Join(
            " OR ",
            Enumerable.Range(1, 48).Select(index => $@"(
                {skillAlias}.Param{index} = @RequiredItemParam
                AND
                (
                    ({skillAlias}.Param{index + 1} = {weaponAlias}.TypeID3 AND {skillAlias}.Param{index + 2} = {weaponAlias}.TypeID4)
                    OR ({skillAlias}.Param{index + 1} = {offhandAlias}.TypeID3 AND {skillAlias}.Param{index + 2} = {offhandAlias}.TypeID4)
                )
            )"));

        return $@"
(
    (
        ISNULL({skillAlias}.ReqCast_Weapon1, {AnyWeaponRequirement}) = {AnyWeaponRequirement}
        AND (NOT ({markerPredicate}) OR ({equippedItemMatches}))
    )
    OR {skillAlias}.ReqCast_Weapon1 = {weaponAlias}.TypeID4
    OR
    (
        ISNULL({skillAlias}.ReqCast_Weapon2, {AnyWeaponRequirement}) <> {AnyWeaponRequirement}
        AND {skillAlias}.ReqCast_Weapon2 = {weaponAlias}.TypeID4
    )
)";
    }

    internal static bool IsMonsterAttackSkill(
        int basicActivity,
        bool targetRequired,
        bool targetsEnemyMonster,
        bool targetsSelf,
        bool targetsParty,
        bool selectsDeadBody,
        IReadOnlyList<int> parameters) =>
        !selectsDeadBody &&
        parameters.Contains(AttackSkillParameter) &&
        !(basicActivity == 1 && !targetRequired && !targetsEnemyMonster);

    internal static bool IsSelfBuffSkill(
        int basicActivity,
        bool targetRequired,
        bool targetsSelf,
        bool targetsParty,
        bool targetsEnemyMonster,
        bool targetsEnemyPlayer,
        bool selectsDeadBody,
        IReadOnlyList<int> parameters)
    {
        var isImbue = basicActivity == 1 && parameters.Contains(AttackSkillParameter);
        var isPersistentBuff = parameters.Count > 0 && parameters[0] == PrimaryBuffSkillType;
        return basicActivity != 0 &&
               parameters.Contains(DurationSkillParameter) &&
               (isPersistentBuff || isImbue) &&
               !targetsEnemyMonster &&
               !targetsEnemyPlayer &&
               !selectsDeadBody &&
               (!targetRequired || targetsSelf || targetsParty);
    }

    internal static bool IsSkillCompatibleWithEquippedItems(
        int requiredWeapon1,
        int requiredWeapon2,
        int weaponTypeId3,
        int weaponTypeId4,
        int? offhandTypeId3,
        int? offhandTypeId4,
        IReadOnlyList<int> parameters)
    {
        if (requiredWeapon1 != AnyWeaponRequirement)
            return requiredWeapon1 == weaponTypeId4 ||
                   (requiredWeapon2 != AnyWeaponRequirement && requiredWeapon2 == weaponTypeId4);

        var foundRequiredItemMarker = false;
        for (var index = 0; index < parameters.Count; index++)
        {
            if (parameters[index] != RequiredItemSkillParameter)
                continue;

            foundRequiredItemMarker = true;
            if (index + 2 >= parameters.Count)
                continue;

            var requiredTypeId3 = parameters[index + 1];
            var requiredTypeId4 = parameters[index + 2];
            if ((requiredTypeId3 == weaponTypeId3 && requiredTypeId4 == weaponTypeId4) ||
                (offhandTypeId3.HasValue && offhandTypeId4.HasValue &&
                 requiredTypeId3 == offhandTypeId3.Value && requiredTypeId4 == offhandTypeId4.Value))
                return true;
        }

        return !foundRequiredItemMarker;
    }

    private static ClientlessCombatSkill[] KeepHighestSkillRankPerBranch(
        IEnumerable<ClientlessCombatSkill> skills) =>
        skills
            .GroupBy(skill => skill.GroupID > 0
                ? $"G:{skill.GroupID}"
                : $"S:{skill.ID}", StringComparer.Ordinal)
            .Select(group => group
                .OrderByDescending(skill => skill.BasicLevel)
                .ThenByDescending(skill => skill.ID)
                .First())
            .ToArray();

    internal static async Task<IReadOnlyList<ClientlessPotionSlot>> LoadPotionSlotsAsync(string characterName)
    {
        try
        {
            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            var rows = await connection.QueryAsync<ClientlessPotionSlot>($@"
;WITH Potions AS
(
    SELECT I.Slot, C.TypeID4,
           CONVERT(INT, C.CashItem | C.Bionic | (C.TypeID1 * 4) | (C.TypeID2 * 32) |
                        (C.TypeID3 * 128) | (C.TypeID4 * 2048)) AS TypeID,
           CASE WHEN EXISTS
           (
               SELECT 1
               FROM {shardDb}.dbo._Inventory Equipped WITH (NOLOCK)
               INNER JOIN {shardDb}.dbo._Items EquippedItem WITH (NOLOCK)
                   ON EquippedItem.ID64 = Equipped.ItemID AND EquippedItem.Data > 0
               INNER JOIN {shardDb}.dbo._RefObjCommon EquippedCommon WITH (NOLOCK)
                   ON EquippedCommon.ID = EquippedItem.RefItemID
               WHERE Equipped.CharID = Ch.CharID AND Equipped.Slot = 6
                 AND EquippedCommon.TypeID4 BETWEEN 2 AND 6
           ) THEN 1100 ELSE 15100 END AS CooldownMs,
           ROW_NUMBER() OVER(PARTITION BY C.TypeID4 ORDER BY C.ReqLevel1 DESC, C.ID DESC) AS rn
    FROM {shardDb}.dbo._Char Ch WITH (NOLOCK)
    INNER JOIN {shardDb}.dbo._Inventory I WITH (NOLOCK) ON I.CharID = Ch.CharID AND I.ItemID > 0
    INNER JOIN {shardDb}.dbo._Items It WITH (NOLOCK) ON It.ID64 = I.ItemID AND It.Data > 0
    INNER JOIN {shardDb}.dbo._RefObjCommon C WITH (NOLOCK) ON C.ID = It.RefItemID
    WHERE Ch.CharName16 = @CharacterName
      AND C.Service = 1 AND C.TypeID1 = 3 AND C.TypeID2 = 3 AND C.TypeID3 = 1
      AND C.TypeID4 IN (1, 2)
)
SELECT CONVERT(TINYINT, Slot) AS Slot, TypeID4, CONVERT(INT, TypeID) AS TypeID, CooldownMs
FROM Potions WHERE rn = 1;", new { CharacterName = characterName });
            return rows.ToArray();
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Clientless {Character} potion slots could not be loaded.", characterName);
            return Array.Empty<ClientlessPotionSlot>();
        }
    }

    internal static async Task<ClientlessSpeedSlot?> LoadSpeedSlotAsync(string characterName)
    {
        try
        {
            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            return await connection.QueryFirstOrDefaultAsync<ClientlessSpeedSlot>($@"
SELECT TOP (1)
       CONVERT(TINYINT, InventoryRow.Slot) AS Slot,
       CONVERT(INT, Common.CashItem | Common.Bionic | (Common.TypeID1 * 4) | (Common.TypeID2 * 32) |
                    (Common.TypeID3 * 128) | (Common.TypeID4 * 2048)) AS TypeID,
       Common.CodeName128
FROM {shardDb}.dbo._Char AS CharacterRow WITH (NOLOCK)
INNER JOIN {shardDb}.dbo._Inventory AS InventoryRow WITH (NOLOCK)
    ON InventoryRow.CharID = CharacterRow.CharID AND InventoryRow.ItemID > 0
INNER JOIN {shardDb}.dbo._Items AS ItemRow WITH (NOLOCK)
    ON ItemRow.ID64 = InventoryRow.ItemID AND ItemRow.Data > 0
INNER JOIN {shardDb}.dbo._RefObjCommon AS Common WITH (NOLOCK)
    ON Common.ID = ItemRow.RefItemID
WHERE CharacterRow.CharName16 = @CharacterName
  AND Common.Service = 1
  AND Common.TypeID1 = 3 AND Common.TypeID2 = 3 AND Common.TypeID3 = 13 AND Common.TypeID4 = 1
  AND Common.CodeName128 LIKE '%SPEED%'
ORDER BY Common.ReqLevel1 DESC, Common.ID DESC;", new { CharacterName = characterName });
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Clientless {Character} speed-scroll slot could not be loaded.", characterName);
            return null;
        }
    }

    internal static async Task<IReadOnlyList<ClientlessPetSlot>> LoadPetSlotsAsync(string characterName)
    {
        try
        {
            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            var rows = await connection.QueryAsync<ClientlessPetSlot>($@"
;WITH Pets AS
(
    SELECT InventoryRow.Slot,
           Common.TypeID4 AS PetKind,
           CONVERT(INT, Common.CashItem | Common.Bionic | (Common.TypeID1 * 4) | (Common.TypeID2 * 32) |
                        (Common.TypeID3 * 128) | (Common.TypeID4 * 2048)) AS TypeID,
           Common.CodeName128,
           ROW_NUMBER() OVER(PARTITION BY Common.TypeID4 ORDER BY InventoryRow.Slot, Common.ID) AS RankOrder
    FROM {shardDb}.dbo._Char AS CharacterRow WITH (NOLOCK)
    INNER JOIN {shardDb}.dbo._Inventory AS InventoryRow WITH (NOLOCK)
        ON InventoryRow.CharID = CharacterRow.CharID AND InventoryRow.ItemID > 0
    INNER JOIN {shardDb}.dbo._Items AS ItemRow WITH (NOLOCK)
        ON ItemRow.ID64 = InventoryRow.ItemID
    INNER JOIN {shardDb}.dbo._RefObjCommon AS Common WITH (NOLOCK)
        ON Common.ID = ItemRow.RefItemID
    WHERE CharacterRow.CharName16 = @CharacterName
      AND Common.Service = 1
      AND Common.TypeID1 = 3 AND Common.TypeID2 = 2 AND Common.TypeID3 = 1
      AND Common.CodeName128 IN
      (
          'ITEM_COS_P_FLUTE',
          'ITEM_COS_P_FLUTE_SILK',
          'ITEM_COS_P_FLUTE_WHITE',
          'ITEM_COS_P_FLUTE_WHITE_SMALL',
          'ITEM_COS_P_JINN_SCROLL',
          'ITEM_COS_P_RAVEN_SCROLL',
          'ITEM_COS_P_PENGUIN_SCROLL',
          'ITEM_COS_P_KANGAROO_SCROLL',
          'ITEM_COS_P_BEAR_SCROLL',
          'ITEM_COS_P_FOX_SCROLL',
          'ITEM_COS_P_MYOWON_SCROLL',
          'ITEM_COS_P_SEOWON_SCROLL',
          'ITEM_COS_P_SPOT_RABBIT_SCROLL',
          'ITEM_COS_P_RABBIT_SCROLL_SILK',
          'ITEM_COS_P_GOLDPIG_SCROLL_SILK',
          'ITEM_COS_P_GGLIDER_SCROLL',
          'ITEM_COS_P_PINKPIG_SCROLL',
          'ITEM_COS_P_CAT_SCROLL',
          'ITEM_COS_P_RACCOONDOG_SCROLL',
          'ITEM_COS_P_BROWNIE_SCROLL'
      )
)
SELECT CONVERT(TINYINT, Slot) AS Slot,
       CONVERT(TINYINT, PetKind) AS PetKind,
       CONVERT(INT, TypeID) AS TypeID,
       CodeName128
FROM Pets
WHERE RankOrder = 1
ORDER BY PetKind;", new { CharacterName = characterName });
            return rows.ToArray();
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Clientless {Character} pet slots could not be loaded.", characterName);
            return Array.Empty<ClientlessPetSlot>();
        }
    }

    internal static async Task<IReadOnlyList<ClientlessPetSupplySlot>> LoadPetSupplySlotsAsync(string characterName)
    {
        try
        {
            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            var rows = await connection.QueryAsync<ClientlessPetSupplySlot>($@"
;WITH Supplies AS
(
    SELECT InventoryRow.Slot,
           CASE
               WHEN Common.TypeID3 = 1 AND Common.TypeID4 = 4 THEN 1
               WHEN Common.TypeID3 = 1 AND Common.TypeID4 = 6 THEN 2
               WHEN Common.TypeID3 = 1 AND Common.TypeID4 = 9 AND RefItem.Param1 = 10 THEN 3
               ELSE 0
           END AS SupplyKind,
           CONVERT(INT, Common.CashItem | Common.Bionic | (Common.TypeID1 * 4) | (Common.TypeID2 * 32) |
                        (Common.TypeID3 * 128) | (Common.TypeID4 * 2048)) AS TypeID,
           Common.CodeName128,
           ROW_NUMBER() OVER
           (
               PARTITION BY CASE
                   WHEN Common.TypeID3 = 1 AND Common.TypeID4 = 4 THEN 1
                   WHEN Common.TypeID3 = 1 AND Common.TypeID4 = 6 THEN 2
                   WHEN Common.TypeID3 = 1 AND Common.TypeID4 = 9 AND RefItem.Param1 = 10 THEN 3
                   ELSE 0
               END
               ORDER BY Common.ReqLevel1 DESC, Common.ID DESC
           ) AS RankOrder
    FROM {shardDb}.dbo._Char AS CharacterRow WITH (NOLOCK)
    INNER JOIN {shardDb}.dbo._Inventory AS InventoryRow WITH (NOLOCK)
        ON InventoryRow.CharID = CharacterRow.CharID AND InventoryRow.ItemID > 0
    INNER JOIN {shardDb}.dbo._Items AS ItemRow WITH (NOLOCK)
        ON ItemRow.ID64 = InventoryRow.ItemID AND ItemRow.Data > 0
    INNER JOIN {shardDb}.dbo._RefObjCommon AS Common WITH (NOLOCK)
        ON Common.ID = ItemRow.RefItemID
    INNER JOIN {shardDb}.dbo._RefObjItem AS RefItem WITH (NOLOCK)
        ON RefItem.ID = Common.Link
    WHERE CharacterRow.CharName16 = @CharacterName
      AND Common.Service = 1 AND Common.TypeID1 = 3 AND Common.TypeID2 = 3
      AND ((Common.TypeID3 = 1 AND Common.TypeID4 IN (4, 6))
           OR (Common.TypeID3 = 1 AND Common.TypeID4 = 9 AND RefItem.Param1 = 10))
)
SELECT CONVERT(TINYINT, Slot) AS Slot,
       CONVERT(TINYINT, SupplyKind) AS SupplyKind,
       CONVERT(INT, TypeID) AS TypeID,
       CodeName128
FROM Supplies
WHERE SupplyKind > 0 AND RankOrder = 1
ORDER BY SupplyKind;", new { CharacterName = characterName });
            return rows.ToArray();
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Clientless {Character} pet supplies could not be loaded.", characterName);
            return Array.Empty<ClientlessPetSupplySlot>();
        }
    }

    private static string? NormalizeCity(string? city) =>
        string.IsNullOrWhiteSpace(city) || city.Equals("All", StringComparison.OrdinalIgnoreCase)
            ? null
            : city.Trim();

    private static string FormatScope(string? city) => city == null ? "all cities" : city;

    private sealed record PendingClientlessLaunch(ClientlessAccount Account, long Version, string City);
}
