using System.Data;
using KMTGuard.AdminDesktop.Services;
using KMTGuard.RuntimeContract;
using Microsoft.Data.SqlClient;

var service = new FilterRuntimeService();

static void Require(bool condition, string message)
{
    if (!condition)
        throw new InvalidOperationException(message);
}

if (args.Contains("--emit-clientless-avatar-sql", StringComparer.OrdinalIgnoreCase))
{
    Console.Write(SqlAdminService.BuildClientlessAvatarEquipmentSql(
        "[SRO_VT_SHARD]", "@CharID", "@Gender", "@CharID"));
    return;
}

if (args.Contains("--emit-clientless-pet-sql", StringComparer.OrdinalIgnoreCase))
{
    Console.Write(SqlAdminService.BuildClientlessPetEquipmentSql(
        "[SRO_VT_SHARD]", "@CharID", "@Level", "@CharID"));
    return;
}

var validatePetSqlIndex = Array.FindIndex(args, value =>
    value.Equals("--validate-clientless-pet-sql", StringComparison.OrdinalIgnoreCase));
if (validatePetSqlIndex >= 0)
{
    Require(validatePetSqlIndex + 1 < args.Length,
        "Settings.json path is required for the Clientless pet SQL parser check.");
    var settings = new SettingsFileService().Load(args[validatePetSqlIndex + 1]);
    await using var connection = new SqlConnection(settings.BuildConnectionString());
    await connection.OpenAsync();
    await using (var noExecute = new SqlCommand("SET NOEXEC ON;", connection))
        await noExecute.ExecuteNonQueryAsync();
    try
    {
        foreach (var option in new[]
                 {
                     (Attack: true, Grab: true),
                     (Attack: true, Grab: false),
                     (Attack: false, Grab: true)
                 })
        {
            var petSql = SqlAdminService.BuildOptionalClientlessPetEquipmentSql(
                "[SRO_VT_SHARD]", "@CharID", "@Level", "@CharID", option.Attack, option.Grab);
            await using var command = new SqlCommand(
                $"DECLARE @CharID INT = 1, @Level INT = 110; {petSql}",
                connection)
            {
                CommandTimeout = 120
            };
            await command.ExecuteNonQueryAsync();
        }
    }
    finally
    {
        await using var executeAgain = new SqlCommand("SET NOEXEC OFF;", connection);
        await executeAgain.ExecuteNonQueryAsync();
    }

    Console.WriteLine("Clientless both/attack-only/grab-only pet SQL parser checks passed.");
    return;
}

var applyIndependentPetsIndex = Array.FindIndex(args, value =>
    value.Equals("--apply-clientless-independent-pets", StringComparison.OrdinalIgnoreCase));
if (applyIndependentPetsIndex >= 0)
{
    Require(applyIndependentPetsIndex + 3 < args.Length,
        "Settings, migration, and validation paths are required for the independent pet update.");
    var settings = new SettingsFileService().Load(args[applyIndependentPetsIndex + 1]);
    var migrationSql = await File.ReadAllTextAsync(args[applyIndependentPetsIndex + 2]);
    var validationSql = await File.ReadAllTextAsync(args[applyIndependentPetsIndex + 3]);
    await using var connection = new SqlConnection(settings.BuildConnectionString());
    await connection.OpenAsync();

    // Execute twice to prove that the packaged schema update is safe to reapply.
    for (var pass = 1; pass <= 2; pass++)
    {
        await using var migration = new SqlCommand(migrationSql, connection) { CommandTimeout = 120 };
        await migration.ExecuteNonQueryAsync();
    }

    await using (var validation = new SqlCommand(validationSql, connection) { CommandTimeout = 120 })
    await using (var reader = await validation.ExecuteReaderAsync())
    {
        Require(await reader.ReadAsync(), "Independent pet validation returned no result.");
        Require(string.Equals(
                reader.GetString(0),
                "Independent Clientless pet options validation passed.",
                StringComparison.Ordinal),
            "Independent pet validation returned an unexpected result.");
    }

    Console.WriteLine("Independent Clientless pet migration passed twice and validation succeeded.");
    return;
}

var logTailIndex = Array.FindIndex(args, value =>
    value.Equals("--log-tail-read", StringComparison.OrdinalIgnoreCase));
if (logTailIndex >= 0)
{
    Require(logTailIndex + 1 < args.Length, "A log path is required for the bounded-tail check.");
    var stopwatch = System.Diagnostics.Stopwatch.StartNew();
    var tail = new LogService().ReadTail(args[logTailIndex + 1]);
    stopwatch.Stop();
    Console.WriteLine(
        $"Bounded log tail passed. SourceBytes={new FileInfo(args[logTailIndex + 1]).Length:N0}, " +
        $"TailCharacters={tail.Length:N0}, ElapsedMs={stopwatch.ElapsedMilliseconds:N0}.");
    return;
}

var skillCatalogIndex = Array.FindIndex(args, value =>
    value.Equals("--clientless-skill-catalog", StringComparison.OrdinalIgnoreCase));
if (skillCatalogIndex >= 0)
{
    Require(skillCatalogIndex + 1 < args.Length, "Settings.json path is required for the skill catalog check.");
    var settings = new SettingsFileService().Load(args[skillCatalogIndex + 1]);
    await using var connection = new SqlConnection(settings.BuildConnectionString());
    await connection.OpenAsync();

    await using var shardCommand = new SqlCommand(@"
SELECT TOP (1) CONVERT(NVARCHAR(128), Value)
FROM dbo.System_Settings WITH (NOLOCK)
WHERE SettingName = N'ShardDB';", connection);
    var shardDb = Convert.ToString(await shardCommand.ExecuteScalarAsync()) ?? "SRO_VT_SHARD";
    Require(System.Text.RegularExpressions.Regex.IsMatch(shardDb, "^[A-Za-z0-9_]+$"),
        "The configured shard database name is unsafe.");
    var shard = $"[{shardDb}]";
    var parameterColumns = string.Join(", ", Enumerable.Range(1, 50).Select(index => $"Param{index}"));
    await using var catalogCommand = new SqlCommand($@"
SELECT ID, GroupID, Basic_Level, ReqCommon_Mastery1, ReqCommon_MasteryLevel1,
       ReqCommon_Mastery2, ReqCommon_MasteryLevel2, ReqCommon_Str, ReqCommon_Int,
       ReqCast_Weapon1, ReqCast_Weapon2, Basic_Code, Basic_Activity,
       Target_Required, TargetGroup_Enemy_M, TargetEtc_SelectDeadBody,
       {parameterColumns}
FROM {shard}.dbo._RefSkill WITH (NOLOCK)
WHERE Service = 1 AND ReqCommon_Mastery1 IN (257, 258, 259, 513, 514, 515, 516, 517, 518);", connection)
    {
        CommandTimeout = 120
    };
    var skillCatalog = new DataTable();
    await using (var catalogReader = await catalogCommand.ExecuteReaderAsync())
        skillCatalog.Load(catalogReader);

    static int SkillInt(DataRow row, string column) =>
        row.IsNull(column) ? 0 : Convert.ToInt32(row[column]);

    static bool IsMonsterAttack(DataRow skill)
    {
        var parameters = Enumerable.Range(1, 50).Select(index => SkillInt(skill, $"Param{index}")).ToArray();
        var code = Convert.ToString(skill["Basic_Code"]) ?? string.Empty;
        return parameters.Contains(6386804) &&
               SkillInt(skill, "TargetEtc_SelectDeadBody") == 0 &&
               !code.Contains("DOWNATTACK", StringComparison.OrdinalIgnoreCase) &&
               !code.Contains("RESURRECT", StringComparison.OrdinalIgnoreCase) &&
               !code.Contains("_BASE_", StringComparison.OrdinalIgnoreCase) &&
               !code.Contains("SACRIFICE", StringComparison.OrdinalIgnoreCase) &&
               !(SkillInt(skill, "Basic_Activity") == 1 &&
                 SkillInt(skill, "Target_Required") == 0 &&
                 SkillInt(skill, "TargetGroup_Enemy_M") == 0);
    }

    static bool MatchesWeapon(DataRow skill, int weaponTypeId4)
    {
        var parameters = Enumerable.Range(1, 50).Select(index => SkillInt(skill, $"Param{index}")).ToArray();
        var hasRequiredItemMarker = parameters.Contains(1919250793);
        var hasMatchingMarker = Enumerable.Range(0, 48).Any(index =>
            parameters[index] == 1919250793 && parameters[index + 1] == 6 &&
            parameters[index + 2] == weaponTypeId4);
        var weapon1 = skill.IsNull("ReqCast_Weapon1") ? 255 : SkillInt(skill, "ReqCast_Weapon1");
        var weapon2 = skill.IsNull("ReqCast_Weapon2") ? 255 : SkillInt(skill, "ReqCast_Weapon2");
        return (weapon1 == 255 && (!hasRequiredItemMarker || hasMatchingMarker)) ||
               weapon1 == weaponTypeId4 || (weapon2 != 255 && weapon2 == weaponTypeId4);
    }

    foreach (var weapon in SqlAdminService.GetClientlessWeaponChoices("Random")
                 .Where(value => !value.Equals("Random", StringComparison.OrdinalIgnoreCase)))
    {
        var profile = SqlAdminService.ResolveClientlessWeaponProfile("Random", weapon);
        foreach (var build in new[] { "Strength", "Intelligence" })
        {
            var missingLevels = new List<int>();
            foreach (var level in Enumerable.Range(1, 101))
            {
                var (strength, intellect) = SqlAdminService.CalculateClientlessCombatStats(level, build);
                var chosenSkills = skillCatalog.AsEnumerable()
                    .Where(skill => SkillInt(skill, "ReqCommon_Mastery1") == profile.MasteryId)
                    .Where(skill => SkillInt(skill, "GroupID") > 0)
                    .Where(skill => SkillInt(skill, "Basic_Level") <= level)
                    .Where(skill => SkillInt(skill, "ReqCommon_MasteryLevel1") <= level)
                    .Where(skill => SkillInt(skill, "ReqCommon_Str") <= strength)
                    .Where(skill => SkillInt(skill, "ReqCommon_Int") <= intellect)
                    .Where(skill => SkillInt(skill, "ReqCommon_Mastery2") == 0)
                    .GroupBy(skill => SkillInt(skill, "GroupID"))
                    .Select(group => group
                        .OrderByDescending(skill => SkillInt(skill, "Basic_Level"))
                        .ThenByDescending(skill => SkillInt(skill, "ReqCommon_MasteryLevel1"))
                        .ThenByDescending(skill => SkillInt(skill, "ID"))
                        .First());
                if (!chosenSkills.Any(skill => IsMonsterAttack(skill) &&
                                               MatchesWeapon(skill, profile.WeaponTypeId4)))
                    missingLevels.Add(level);
            }

            var expectedNaturallyLockedLevels = profile.Race == "Chinese"
                ? Enumerable.Range(1, 4)
                : Enumerable.Range(1, 3);
            Require(missingLevels.SequenceEqual(expectedNaturallyLockedLevels),
                $"{profile.Race} {profile.Weapon} {build} has an unexpected attack-skill gap: " +
                string.Join(',', missingLevels));

            Console.WriteLine($"{profile.Race} | {profile.Weapon} | {build} | " +
                              (missingLevels.Count == 0
                                  ? "all levels ready"
                                  : $"missing: {string.Join(',', missingLevels)}"));
        }
    }
    return;
}

var clientlessOptionsMigrationIndex = Array.FindIndex(args, value =>
    value.Equals("--apply-clientless-creation-options", StringComparison.OrdinalIgnoreCase));
if (clientlessOptionsMigrationIndex >= 0)
{
    Require(clientlessOptionsMigrationIndex + 3 < args.Length,
        "Settings, migration, and validation paths are required.");
    var settings = new SettingsFileService().Load(args[clientlessOptionsMigrationIndex + 1]);
    var migrationPath = Path.GetFullPath(args[clientlessOptionsMigrationIndex + 2]);
    var validationPath = Path.GetFullPath(args[clientlessOptionsMigrationIndex + 3]);
    Require(Path.GetFileName(migrationPath).Equals(
                "20260814_clientless_creation_options.sql", StringComparison.OrdinalIgnoreCase) &&
            Path.GetFileName(validationPath).Equals(
                "clientless_creation_options_validation.sql", StringComparison.OrdinalIgnoreCase),
        "Only the Clientless creation-options migration can be applied by this check.");

    await using var connection = new SqlConnection(settings.BuildConnectionString());
    await connection.OpenAsync();
    foreach (var path in new[] { migrationPath, validationPath })
    {
        await using var command = new SqlCommand(await File.ReadAllTextAsync(path), connection)
        {
            CommandTimeout = 120
        };
        await command.ExecuteNonQueryAsync();
    }
    Console.WriteLine("Clientless creation-options migration and validation passed.");
    return;
}

if (args.Contains("--contracts-only", StringComparer.OrdinalIgnoreCase))
{
    var largeLogPath = Path.Combine(Path.GetTempPath(), $"kmtguard-tail-{Guid.NewGuid():N}.log");
    try
    {
        using (var writer = new StreamWriter(largeLogPath, append: false))
        {
            writer.WriteLine("FIRST-LINE-MUST-NOT-BE-SCANNED");
            for (var index = 0; index < 30_000; index++)
                writer.WriteLine($"{index:D6} {new string('x', 72)}");
            writer.WriteLine("FINAL-LINE-MUST-BE-VISIBLE");
        }

        Require(new FileInfo(largeLogPath).Length > LogService.DefaultTailByteLimit,
            "The log-tail contract fixture is not larger than the bounded read window.");
        var logTail = LogService.ReadLastLines(largeLogPath, 20, 64 * 1024);
        Require(logTail.Contains("FINAL-LINE-MUST-BE-VISIBLE", StringComparison.Ordinal) &&
                !logTail.Contains("FIRST-LINE-MUST-NOT-BE-SCANNED", StringComparison.Ordinal),
            "Large service logs are not read from a bounded trailing window.");
    }
    finally
    {
        if (File.Exists(largeLogPath))
            File.Delete(largeLogPath);
    }

    var packetAliases = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase)
    {
        ["__Whitelist"] = "[dbo].[Security_Whitelist]",
        ["_Whitelist"] = "[dbo].[Security_Whitelist]",
        ["Whitelist"] = "[dbo].[Security_Whitelist]",
        ["[dbo].[Security_Whitelist]"] = "[dbo].[Security_Whitelist]",
        ["__Blacklist"] = "[dbo].[Security_Blacklist]",
        ["_Blacklist"] = "[dbo].[Security_Blacklist]",
        ["Blacklist"] = "[dbo].[Security_Blacklist]",
        ["[dbo].[Security_Blacklist]"] = "[dbo].[Security_Blacklist]"
    };

    foreach (var (input, expected) in packetAliases)
    {
        Require(SqlAdminService.NormalizePacketRuleTableName(input) == expected,
            $"Packet table alias failed: {input}");
    }

    foreach (var tableName in new[]
             {
                 "_AutoEventConfig",
                 "_SurvivalPartyConfig",
                 "_SurvivalPartyReward",
                 "_SurvivalPartySchedule",
                 "_SurvivalSoloConfig",
                 "_SurvivalSoloReward",
                 "_SurvivalSoloSchedule",
                 "_CompetitiveEventConfig",
                 "_CompetitiveEventReward",
                 "_CompetitiveEventSchedule",
                 "_CompetitiveEventScore",
                 "_HideAndSeekConfig",
                 "_HideAndSeekLocation",
                 "_HideAndSeekReward",
                 "_HideAndSeekSchedule",
                 "_HideAndSeekRun"
             })
    {
        Require(SqlAdminService.EventTable(tableName).EndsWith($".[{tableName}]", StringComparison.Ordinal),
            $"Event table was rejected: {tableName}");
    }

    var unsafePacketRejected = false;
    try
    {
        _ = SqlAdminService.NormalizePacketRuleTableName("dbo.Users");
    }
    catch (InvalidOperationException)
    {
        unsafePacketRejected = true;
    }

    var unsafeEventRejected = false;
    try
    {
        _ = SqlAdminService.EventTable("_OtherTable");
    }
    catch (InvalidOperationException)
    {
        unsafeEventRejected = true;
    }

    Require(unsafePacketRejected, "Unsafe packet table was accepted.");
    Require(unsafeEventRejected, "Unsafe event table was accepted.");
    Require(
        SqlAdminService.AutoEventSchedulePatternKeyColumnName == "ScheduleKey",
        "Auto Event schedule conflict query regressed to a reserved SQL identifier.");

    var hourlySchedule = new AutoEventSchedulePattern(
        "candidate", "Retype", new TimeSpan(0, 0, 0), 1 << (int)DayOfWeek.Sunday, 60);
    var tenMinuteConflict = SqlAdminService.FindAutoEventScheduleConflict(
        hourlySchedule,
        new[]
        {
            new AutoEventSchedulePattern(
                "existing", "Alchemy", new TimeSpan(4, 10, 0), 1 << (int)DayOfWeek.Sunday, 0)
        },
        TimeSpan.FromMinutes(15));
    Require(
        tenMinuteConflict?.EventCode == "Alchemy" && tenMinuteConflict.Time == new TimeSpan(4, 10, 0),
        "Hourly Auto Event schedule did not detect a conflicting event occurrence.");

    var exactGapConflict = SqlAdminService.FindAutoEventScheduleConflict(
        hourlySchedule,
        new[]
        {
            new AutoEventSchedulePattern(
                "existing", "Trivia", new TimeSpan(4, 15, 0), 1 << (int)DayOfWeek.Sunday, 0)
        },
        TimeSpan.FromMinutes(15));
    Require(exactGapConflict == null, "Auto Event schedules exactly 15 minutes apart were rejected.");

    var weekBoundaryConflict = SqlAdminService.FindAutoEventScheduleConflict(
        new AutoEventSchedulePattern(
            "candidate", "LuckyGlobal", new TimeSpan(0, 5, 0), 1 << (int)DayOfWeek.Sunday, 0),
        new[]
        {
            new AutoEventSchedulePattern(
                "existing", "Math", new TimeSpan(23, 55, 0), 1 << (int)DayOfWeek.Saturday, 0)
        },
        TimeSpan.FromMinutes(15));
    Require(weekBoundaryConflict?.EventCode == "Math",
        "Auto Event schedule conflict detection missed the Saturday-to-Sunday boundary.");

    SqlAdminService.ValidateDiscordChannel("Unique Notifications", "123456789012345678");
    SqlAdminService.ValidateDiscordBotToken(
        "MTIzNDU2Nzg5MDEyMzQ1Njc4.example.signaturevalue");

    var invalidDiscordChannelRejected = false;
    try
    {
        SqlAdminService.ValidateDiscordChannel("Unique", "not-a-channel-id");
    }
    catch (InvalidOperationException)
    {
        invalidDiscordChannelRejected = true;
    }

    var invalidDiscordTokenRejected = false;
    try
    {
        SqlAdminService.ValidateDiscordBotToken("https://discord.com/api/webhooks/example");
    }
    catch (InvalidOperationException)
    {
        invalidDiscordTokenRejected = true;
    }

    Require(invalidDiscordChannelRejected, "An invalid Discord Channel ID was accepted.");
    Require(invalidDiscordTokenRejected, "A Discord webhook URL was accepted as a Bot Token.");

    var databasePackageRoot = Directory.CreateTempSubdirectory("kmtguard-database-layout-");
    try
    {
        var v190 = Directory.CreateDirectory(Path.Combine(databasePackageRoot.FullName, "v1.9.0"));
        var v1100 = Directory.CreateDirectory(Path.Combine(databasePackageRoot.FullName, "v1.10.0"));
        var validation = Directory.CreateDirectory(Path.Combine(v190.FullName, "validation"));
        File.WriteAllText(Path.Combine(v190.FullName, "002_second.sql"), "SELECT 2;");
        File.WriteAllText(Path.Combine(v190.FullName, "001_first.sql"), "SELECT 1;");
        File.WriteAllText(Path.Combine(v1100.FullName, "001_future.sql"), "SELECT 3;");
        File.WriteAllText(Path.Combine(validation.FullName, "must_not_load.sql"), "SELECT 4;");

        var discoveredUpdates = SqlAdminService.DiscoverMigrationFiles(databasePackageRoot.FullName);
        Require(discoveredUpdates.Count == 3, "Versioned database discovery included a validation script.");
        Require(
            Path.GetFileName(discoveredUpdates[0]) == "001_first.sql" &&
            Path.GetFileName(discoveredUpdates[1]) == "002_second.sql" &&
            Path.GetFileName(discoveredUpdates[2]) == "001_future.sql",
            "Versioned database updates were not ordered by semantic filter version and file name.");
    }
    finally
    {
        databasePackageRoot.Delete(true);
    }

    var generatedNames = Enumerable.Range(0, 1000)
        .Select(_ => SqlAdminService.CreateClientlessGamingNameCandidate())
        .ToArray();
    Require(generatedNames.All(name =>
            name.Length is >= 4 and <= 16 &&
            name.All(char.IsLetterOrDigit) &&
            char.IsLetter(name[0])),
        "Clientless gaming-name generation produced an invalid character name.");
    Require(generatedNames.Distinct(StringComparer.OrdinalIgnoreCase).Count() >= 800,
        "Clientless gaming-name generation does not provide enough natural variation.");
    Require(SqlAdminService.NormalizeClientlessTownOrUnassigned("sd2") == "Alexandria North (SD)",
        "Clientless Alexandria city alias was not normalized.");
    Require(SqlAdminService.NormalizeClientlessTownOrUnassigned("unknown") == "Unassigned",
        "Unknown Clientless city input was not isolated in the Unassigned group.");
    Require(RuntimeProtocol.Version == 2,
        "Clientless city-scoped commands require runtime protocol version 2.");

    var hunterCandidates = new[]
    {
        new ClientlessHunterCandidate(9, "Jangan", true, true, null),
        new ClientlessHunterCandidate(2, "Jangan", true, true, null),
        new ClientlessHunterCandidate(1, "Jangan", false, true, null),
        new ClientlessHunterCandidate(3, "Jangan", true, false, null),
        new ClientlessHunterCandidate(4, "Jangan", true, true, "HNS"),
        new ClientlessHunterCandidate(8, "Hotan", true, true, null),
        new ClientlessHunterCandidate(7, "Hotan", true, true, null)
    };
    var oneHunterPerCity = SqlAdminService.BuildClientlessHunterSelections(hunterCandidates, 1);
    var janganHunters = oneHunterPerCity.Single(item => item.City == "Jangan");
    var hotanHunters = oneHunterPerCity.Single(item => item.City == "Hotan");
    Require(
        janganHunters.SelectedAccountIds.SequenceEqual(new[] { 2 }) &&
        janganHunters.TotalAccounts == 4 &&
        janganHunters.ReadyEnabledAccounts == 2,
        "Clientless hunter selection is not stable by account ID or did not exclude a system account.");
    Require(
        hotanHunters.SelectedAccountIds.SequenceEqual(new[] { 7 }) &&
        hotanHunters.ReadyEnabledAccounts == 2,
        "Clientless hunter count was applied globally instead of independently per city.");
    var zeroHunters = SqlAdminService.BuildClientlessHunterSelections(hunterCandidates, 0);
    Require(
        zeroHunters.All(item => item.SelectedAccountIds.Count == 0) &&
        zeroHunters.Sum(item => item.ReadyEnabledAccounts) == 4,
        "A zero Clientless hunter count did not keep every ready account parked in its city.");

    var runtimePlans = SqlAdminService.BuildClientlessRuntimePlanSelections(
        hunterCandidates,
        onlinePerCity: 2,
        huntersPerCity: 1);
    var janganPlan = runtimePlans.Single(item => item.City == "Jangan");
    var hotanPlan = runtimePlans.Single(item => item.City == "Hotan");
    Require(
        janganPlan.OnlineAccountIds.SequenceEqual(new[] { 1, 2 }) &&
        janganPlan.HunterAccountIds.SequenceEqual(new[] { 1 }) &&
        janganPlan.ReadyAccounts == 3 &&
        janganPlan.TotalAccounts == 4,
        "The per-city runtime plan did not independently select stable online, hunter, parked, and offline accounts.");
    Require(
        hotanPlan.OnlineAccountIds.SequenceEqual(new[] { 7, 8 }) &&
        hotanPlan.HunterAccountIds.SequenceEqual(new[] { 7 }),
        "The per-city runtime plan leaked account limits between cities.");
    var parkedPlan = SqlAdminService.BuildClientlessRuntimePlanSelections(
        hunterCandidates,
        onlinePerCity: 2,
        huntersPerCity: 0);
    Require(
        parkedPlan.All(item => item.HunterAccountIds.Count == 0) &&
        parkedPlan.Sum(item => item.OnlineAccountIds.Count) == 4,
        "A zero-hunter city plan disconnected the requested online standby accounts.");

    var expectedClientlessWeapons = new Dictionary<string, (string Race, int MasteryId, bool UsesShield)>(StringComparer.OrdinalIgnoreCase)
    {
        ["Sword + Shield"] = ("Chinese", 257, true),
        ["Blade + Shield"] = ("Chinese", 257, true),
        ["Spear"] = ("Chinese", 258, false),
        ["Glaive"] = ("Chinese", 258, false),
        ["Bow"] = ("Chinese", 259, false),
        ["One-Hand Sword + Shield"] = ("Europe", 513, true),
        ["Two-Hand Sword"] = ("Europe", 513, false),
        ["Dual Axe"] = ("Europe", 513, false),
        ["Warlock Rod"] = ("Europe", 516, false),
        ["Wizard Staff"] = ("Europe", 514, false),
        ["Crossbow"] = ("Europe", 515, false),
        ["Dagger"] = ("Europe", 515, false),
        ["Harp"] = ("Europe", 517, false),
        ["Cleric Rod + Shield"] = ("Europe", 518, true)
    };
    foreach (var (weapon, expected) in expectedClientlessWeapons)
    {
        var profile = SqlAdminService.ResolveClientlessWeaponProfile(expected.Race, weapon);
        Require(
            profile.Race == expected.Race &&
            profile.Weapon == weapon &&
            profile.MasteryId == expected.MasteryId &&
            profile.UseShield == expected.UsesShield,
            $"Clientless weapon profile mapping is incorrect for {weapon}.");
    }

    Require(
        SqlAdminService.GetClientlessWeaponChoices("Chinese").Count == 6 &&
        SqlAdminService.GetClientlessWeaponChoices("Europe").Count == 10 &&
        SqlAdminService.GetClientlessWeaponChoices("Random").Count == 15,
        "Clientless race weapon choices are incomplete or contain duplicates.");
    var crossRaceWeaponRejected = false;
    try
    {
        _ = SqlAdminService.ResolveClientlessWeaponProfile("Chinese", "Dagger");
    }
    catch (InvalidOperationException)
    {
        crossRaceWeaponRejected = true;
    }
    Require(crossRaceWeaponRejected, "A European weapon was accepted for a Chinese clientless character.");
    Require(
        SqlAdminService.ResolveClientlessWeaponProfile("Europe", "Wizard Staff").ArmorTypeId3 == 9 &&
        SqlAdminService.ResolveClientlessWeaponProfile("Europe", "Warlock Rod").ArmorTypeId3 == 9 &&
        SqlAdminService.ResolveClientlessWeaponProfile("Europe", "Harp").ArmorTypeId3 == 9 &&
        SqlAdminService.ResolveClientlessWeaponProfile("Europe", "Cleric Rod + Shield").ArmorTypeId3 == 9 &&
        SqlAdminService.ResolveClientlessWeaponProfile("Europe", "One-Hand Sword + Shield").ArmorTypeId3 == 11 &&
        SqlAdminService.ResolveClientlessWeaponProfile("Europe", "Two-Hand Sword").ArmorTypeId3 == 11 &&
        SqlAdminService.ResolveClientlessWeaponProfile("Europe", "Dual Axe").ArmorTypeId3 == 11 &&
        SqlAdminService.ResolveClientlessWeaponProfile("Europe", "Crossbow").ArmorTypeId3 == 10,
        "European Clientless armor families are not mapped to robe/light/heavy vSRO TypeID3 values correctly.");

    var clientlessAvatarSql = SqlAdminService.BuildClientlessAvatarEquipmentSql(
        "[SRO_VT_SHARD]", "@CharID", "@Gender", "@CharID");
    Require(
        clientlessAvatarSql.Contains("ITEM_MALL_AVATAR_", StringComparison.Ordinal) &&
        clientlessAvatarSql.Contains("HALLOWEEN", StringComparison.Ordinal) &&
        clientlessAvatarSql.Contains("PIRATE", StringComparison.Ordinal) &&
        clientlessAvatarSql.Contains("ARABIA", StringComparison.Ordinal) &&
        clientlessAvatarSql.Contains("CLOWN", StringComparison.Ordinal) &&
        clientlessAvatarSql.Contains("CARNIVAL", StringComparison.Ordinal) &&
        clientlessAvatarSql.Contains("SPARTA", StringComparison.Ordinal) &&
        clientlessAvatarSql.Contains("_STRG_ALLOC_ITEM_NoTX", StringComparison.Ordinal) &&
        clientlessAvatarSql.Contains("_InventoryForAvatar", StringComparison.Ordinal) &&
        clientlessAvatarSql.Contains("AvatarItem.ReqGender = @Gender", StringComparison.Ordinal) &&
        clientlessAvatarSql.Contains("SET @AvatarSerial = 0", StringComparison.Ordinal) &&
        clientlessAvatarSql.Contains("THROW 51050", StringComparison.Ordinal),
        "Clientless avatar creation is not varied, gender-compatible, allocator-backed, and inventory-validated.");

    var clientlessPetSql = SqlAdminService.BuildClientlessPetEquipmentSql(
        "[SRO_VT_SHARD]", "@CharID", "@Level", "@CharID");
    Require(
        clientlessPetSql.Contains("(1, 'ITEM_COS_P_FLUTE')", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("ITEM_COS_P_MYOWON_SCROLL", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("ITEM_COS_P_JINN_SCROLL", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("ITEM_COS_P_RAVEN_SCROLL", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("ITEM_COS_P_CAT_SCROLL", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("CandidateCount", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("* 7919", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("COLLATE Latin1_General_CI_AS", StringComparison.Ordinal) &&
        !clientlessPetSql.Contains("FELLOW", StringComparison.Ordinal) &&
        !clientlessPetSql.Contains("TREASUREGOBLIN", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("@ClientlessUnsafePets", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("_CharCOS", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("_InvCOS", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("ExistingItem.RefItemID <> Expected.RefItemID", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("THROW 51063", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("Common.TypeID4 = 4", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("Common.TypeID4 = 6", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("Common.TypeID4 = 9", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("RefItem.Param1 = 10", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("@ClientlessPetRefItemID, 0", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("Data = Request.DataValue", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("@ClientlessAllocationRequests", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("@ClientlessAllocatedItems", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("_LatestItemSerial", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("_ItemPool", StringComparison.Ordinal) &&
        clientlessPetSql.Contains("THROW 51061", StringComparison.Ordinal),
        "Clientless pet provisioning does not enforce, vary, and repair the standard vSRO pet allowlist with safe supplies and batched allocation.");

    Require(
        SqlAdminService.CalculateClientlessStats(1, "Full STR") == (20, 20) &&
        SqlAdminService.CalculateClientlessStats(100, "Full STR") == (416, 119) &&
        SqlAdminService.CalculateClientlessStats(100, "Full INT") == (119, 416),
        "Clientless level stat allocation does not apply all earned points to the selected build.");
    Require(
        SqlAdminService.CalculateClientlessInitialHealth(416, 100) > 200 &&
        SqlAdminService.CalculateClientlessInitialMana(416, 100) > 200 &&
        SqlAdminService.CalculateClientlessInitialHealth(20, 1) == 200,
        "Clientless high-level characters still use the unsafe level-1 HP/MP seed.");
    var strongLevel110 = SqlAdminService.CalculateClientlessCombatStats(110, "Full STR");
    Require(
        strongLevel110.Strength > 3_500 && strongLevel110.Intellect > 3_000 &&
        SqlAdminService.CalculateClientlessInitialHealth(strongLevel110.Strength, 110) > 70_000 &&
        SqlAdminService.CalculateClientlessInitialMana(strongLevel110.Intellect, 110) > 60_000,
        "Clientless level-110 survival stats do not create the requested high HP and MP reserve.");
    var unresolvedRandomBuildRejected = false;
    try
    {
        _ = SqlAdminService.CalculateClientlessStats(100, "Random");
    }
    catch (InvalidOperationException)
    {
        unresolvedRandomBuildRejected = true;
    }
    Require(unresolvedRandomBuildRejected,
        "Clientless stat calculation silently accepted an unresolved random build.");

    var clientlessOwnershipJoin = SqlAdminService.BuildClientlessOwnedCharacterJoinSql(
        "[SRO_VT_ACCOUNT]", "[SRO_VT_SHARD]");
    Require(
        clientlessOwnershipJoin.Contains("[SRO_VT_ACCOUNT].dbo.TB_User AS AccountOwner", StringComparison.Ordinal) &&
        clientlessOwnershipJoin.Contains("[SRO_VT_SHARD].dbo._User AS CharacterOwner", StringComparison.Ordinal) &&
        clientlessOwnershipJoin.Contains("CharacterOwner.UserJID = AccountOwner.JID", StringComparison.Ordinal) &&
        clientlessOwnershipJoin.Contains("C.CharID = CharacterOwner.CharID", StringComparison.Ordinal) &&
        clientlessOwnershipJoin.Split("COLLATE Latin1_General_CI_AS", StringSplitOptions.None).Length - 1 == 4 &&
        !clientlessOwnershipJoin.Contains("COLLATE DATABASE_DEFAULT", StringComparison.Ordinal) &&
        !clientlessOwnershipJoin.Contains("NOLOCK", StringComparison.OrdinalIgnoreCase),
        "Prepare Existing does not prove account/JID character ownership with an explicit cross-database collation.");

    var clientlessMasteryReset = SqlAdminService.BuildClientlessUnusedMasteryResetSql("[SRO_VT_SHARD]");
    Require(
        clientlessMasteryReset.Contains("SET Level = 0", StringComparison.Ordinal) &&
        clientlessMasteryReset.Contains("#ClientlessCharacters AS ManagedCharacter", StringComparison.Ordinal) &&
        clientlessMasteryReset.Contains("#ClientlessSkillTargets AS RequiredMastery", StringComparison.Ordinal) &&
        clientlessMasteryReset.Contains("NOT EXISTS", StringComparison.Ordinal),
        "Prepare Existing does not reset obsolete masteries only for verified managed characters.");

    var clientlessMasteryCapGuard = SqlAdminService.BuildClientlessMasteryCapGuardSql();
    Require(
        clientlessMasteryCapGuard.Contains("GROUP BY Target.CharID, ManagedCharacter.TotalMasteryCap", StringComparison.Ordinal) &&
        clientlessMasteryCapGuard.Contains("SUM(CONVERT(BIGINT, Target.MasteryLevel)) > ManagedCharacter.TotalMasteryCap", StringComparison.Ordinal) &&
        clientlessMasteryCapGuard.Contains("THROW 51035", StringComparison.Ordinal),
        "Prepare Existing does not reject a per-character mastery allocation above the configured cap.");

    var clientlessSkillCleanup = SqlAdminService.BuildClientlessObsoleteSkillDeleteSql("[SRO_VT_SHARD]");
    Require(
        clientlessSkillCleanup.Contains("#ClientlessCharacters AS ManagedCharacter", StringComparison.Ordinal) &&
        clientlessSkillCleanup.Contains("ISNULL(CurrentSkill.ReqCommon_Mastery1, 0) > 0", StringComparison.Ordinal) &&
        clientlessSkillCleanup.Contains("#ClientlessDesiredSkills AS Desired", StringComparison.Ordinal) &&
        clientlessSkillCleanup.Contains("Desired.SkillID = Learned.SkillID", StringComparison.Ordinal) &&
        clientlessSkillCleanup.Contains("NOT EXISTS", StringComparison.Ordinal),
        "Prepare Existing skill cleanup can remove default/common skills or retain obsolete mastery ranks.");

    var clientlessAttackSkill = SqlAdminService.BuildClientlessMonsterAttackSkillPredicate("Skill");
    var clientlessWeaponSkill = SqlAdminService.BuildClientlessWeaponSkillCompatibilityPredicate("Skill", "Weapon.TypeID4");
    Require(
        clientlessAttackSkill.Contains("TargetGroup_Enemy_M", StringComparison.Ordinal) &&
        clientlessAttackSkill.Contains("Target_Required, 0) = 0", StringComparison.Ordinal) &&
        clientlessAttackSkill.Contains("@AttackParam", StringComparison.Ordinal) &&
        clientlessAttackSkill.Contains("%[_]BASE[_]%", StringComparison.Ordinal),
        "Clientless preparation can reject real European damage skills or admit base placeholders.");
    Require(
        clientlessWeaponSkill.Contains("@RequiredItemParam", StringComparison.Ordinal) &&
        clientlessWeaponSkill.Contains("Param48", StringComparison.Ordinal) &&
        clientlessWeaponSkill.Contains("Weapon.TypeID4", StringComparison.Ordinal),
        "Clientless preparation does not verify all vSRO required-item markers against the selected weapon.");

    var unlockedAttackGuard = SqlAdminService.BuildClientlessUnlockedAttackSkillExistsSql("[SRO_VT_SHARD]");
    Require(
        unlockedAttackGuard.Contains("AvailableAttackSkill.Basic_Level <= @Level", StringComparison.Ordinal) &&
        unlockedAttackGuard.Contains("AvailableAttackSkill.ReqCommon_MasteryLevel1 <= @Level", StringComparison.Ordinal) &&
        unlockedAttackGuard.Contains("@ClientlessDesiredMasteries", StringComparison.Ordinal) &&
        unlockedAttackGuard.Contains("@WeaponTypeID4", StringComparison.Ordinal),
        "Low-level Clientless creation cannot distinguish a naturally locked skill from failed preparation.");

    Require(
        SqlAdminService.BuildOptionalClientlessAvatarEquipmentSql(
            "[SRO_VT_SHARD]", "@CharID", "@Gender", "@CharID", false).Length == 0 &&
        SqlAdminService.BuildOptionalClientlessAvatarEquipmentSql(
            "[SRO_VT_SHARD]", "@CharID", "@Gender", "@CharID", true)
            .Contains("@AvatarSetEquipped", StringComparison.Ordinal) &&
        SqlAdminService.BuildOptionalClientlessPetEquipmentSql(
            "[SRO_VT_SHARD]", "@CharID", "@Level", "@CharID", false, false).Length == 0 &&
        SqlAdminService.BuildOptionalClientlessPetEquipmentSql(
            "[SRO_VT_SHARD]", "@CharID", "@Level", "@CharID", true, true)
            .Contains("@ClientlessPetsAdded", StringComparison.Ordinal),
        "Clientless avatar or pet creation cannot be disabled without changing existing provisioning logic.");

    var attackPetOnlySql = SqlAdminService.BuildOptionalClientlessPetEquipmentSql(
        "[SRO_VT_SHARD]", "@CharID", "@Level", "@CharID", true, false);
    var grabPetOnlySql = SqlAdminService.BuildOptionalClientlessPetEquipmentSql(
        "[SRO_VT_SHARD]", "@CharID", "@Level", "@CharID", false, true);
    Require(
        attackPetOnlySql.Contains(
            "@ClientlessEnableAttackPet BIT = CASE WHEN ISNULL(CONVERT(INT, 1)",
            StringComparison.Ordinal) &&
        attackPetOnlySql.Contains(
            "@ClientlessEnableGrabPet BIT = CASE WHEN ISNULL(CONVERT(INT, 0)",
            StringComparison.Ordinal) &&
        grabPetOnlySql.Contains(
            "@ClientlessEnableAttackPet BIT = CASE WHEN ISNULL(CONVERT(INT, 0)",
            StringComparison.Ordinal) &&
        grabPetOnlySql.Contains(
            "@ClientlessEnableGrabPet BIT = CASE WHEN ISNULL(CONVERT(INT, 1)",
            StringComparison.Ordinal) &&
        attackPetOnlySql.Contains("IF @ClientlessEnableAttackPet = 1", StringComparison.Ordinal) &&
        grabPetOnlySql.Contains("@ClientlessExpectedPetCount", StringComparison.Ordinal),
        "Attack and grab pets cannot be provisioned independently.");

    var huntAreasFromSql = new DataTable();
    huntAreasFromSql.Columns.Add("ID", typeof(int));
    huntAreasFromSql.Columns.Add("Area", typeof(byte));
    huntAreasFromSql.Columns.Add("Name", typeof(string));
    huntAreasFromSql.Columns.Add("Enabled", typeof(bool));
    huntAreasFromSql.Columns.Add("RegionID", typeof(int));
    huntAreasFromSql.Columns.Add("X", typeof(decimal));
    huntAreasFromSql.Columns.Add("Y", typeof(decimal));
    huntAreasFromSql.Columns.Add("Z", typeof(decimal));
    huntAreasFromSql.Columns.Add("Radius", typeof(decimal));
    huntAreasFromSql.Rows.Add(1, (byte)1, "Area 1", true, 25000, -123.5m, 4.25m, 991.75m, 50m);
    var huntAreaEditor = SqlAdminService.CreateClientlessHuntAreaEditorTable(huntAreasFromSql);
    Require(
        huntAreaEditor.Columns["RegionID"]!.DataType == typeof(string) &&
        huntAreaEditor.Columns["X"]!.DataType == typeof(string) &&
        Convert.ToString(huntAreaEditor.Rows[0]["X"]) == "-123.5",
        "Clientless hunting editor retained SQL numeric types that can trap WPF cell focus.");
    huntAreaEditor.Rows[0]["RegionID"] = "26476";
    huntAreaEditor.Rows[0]["X"] = "-";
    huntAreaEditor.Rows[0]["Y"] = "110.5";
    huntAreaEditor.Rows[0]["Z"] = "557";
    Require(
        Convert.ToString(huntAreaEditor.Rows[0]["RegionID"]) == "26476" &&
        Convert.ToString(huntAreaEditor.Rows[0]["X"]) == "-" &&
        Convert.ToString(huntAreaEditor.Rows[0]["Y"]) == "110.5" &&
        Convert.ToString(huntAreaEditor.Rows[0]["Z"]) == "557",
        "Clientless hunting editor converted an in-progress coordinate while focus moved between fields.");
    var incompleteCoordinateRejectedOnSave = false;
    try
    {
        _ = SqlAdminService.ParseClientlessHuntNumber(huntAreaEditor.Rows[0]["X"], "X");
    }
    catch (InvalidOperationException)
    {
        incompleteCoordinateRejectedOnSave = true;
    }
    Require(incompleteCoordinateRejectedOnSave,
        "Clientless hunting validation accepted an incomplete coordinate instead of deferring rejection to Save.");
    Require(
        SqlAdminService.ParseClientlessHuntInteger("٢٥٠٠٠", "Region") == 25000 &&
        Math.Abs(SqlAdminService.ParseClientlessHuntNumber("−١٢٣٫٥", "X") + 123.5f) < 0.001f &&
        Math.Abs(SqlAdminService.ParseClientlessHuntNumber("50,5", "Radius") - 50.5f) < 0.001f,
        "Clientless hunting coordinate parsing does not accept operator decimal or Arabic digit formats.");

    var schedulerTable = new DataTable();
    schedulerTable.Columns.Add("RepeatType", typeof(string));
    schedulerTable.Columns.Add("Time", typeof(TimeSpan));
    schedulerTable.Columns.Add("IntervalSeconds", typeof(int));
    schedulerTable.Columns.Add("StartDateTime", typeof(DateTime));
    schedulerTable.Columns.Add("ScheduledDate", typeof(DateTime));
    schedulerTable.Columns.Add("DaysOfWeekMask", typeof(byte));
    schedulerTable.Columns.Add("RepeatDayOfWeek", typeof(byte));
    schedulerTable.Columns.Add("IsEnabled", typeof(bool));
    schedulerTable.Columns.Add("RunningToken", typeof(Guid));
    schedulerTable.Columns.Add("LeaseUntilUtc", typeof(DateTime));
    schedulerTable.Columns.Add("LastStatus", typeof(string));
    var schedulerRow = schedulerTable.NewRow();
    schedulerRow["RepeatType"] = "Interval";
    schedulerRow["Time"] = TimeSpan.Zero;
    schedulerRow["IntervalSeconds"] = 600;
    schedulerRow["StartDateTime"] = new DateTime(2026, 7, 29, 12, 0, 0);
    schedulerRow["IsEnabled"] = true;
    schedulerTable.Rows.Add(schedulerRow);
    SqlAdminService.AddSchedulerDisplayColumns(schedulerTable, new DateTime(2026, 7, 29, 12, 1, 0));
    Require(
        Convert.ToString(schedulerRow["ScheduleSummary"])!.StartsWith("Every 10 minute", StringComparison.Ordinal) &&
        Convert.ToDateTime(schedulerRow["NextRunLocal"]) == new DateTime(2026, 7, 29, 12, 10, 0),
        "Scheduler interval summary or next-run preview is incorrect.");
    var schedulerCompatibilityTable = schedulerTable.Copy();
    schedulerCompatibilityTable.Columns.Remove("RunningToken");
    SqlAdminService.AddSchedulerDisplayColumns(
        schedulerCompatibilityTable,
        new DateTime(2026, 7, 29, 12, 1, 0));
    Require(
        Convert.ToString(schedulerCompatibilityTable.Rows[0]["JobState"]) == "Ready",
        "Scheduler display compatibility failed when optional running-state data was absent.");

    var schedulerQuery = SqlAdminService.BuildSchedulerExecQuery(
        "SRO_VT_SHARD",
        "dbo.RefreshRanking",
        "@Force = 1");
    Require(
        schedulerQuery == "EXEC [SRO_VT_SHARD].[dbo].[RefreshRanking] @Force = 1",
        "Scheduler did not build the manually entered database and procedure target correctly.");
    var schedulerTarget = SqlAdminService.ParseSchedulerExecQuery(
        schedulerQuery,
        "KMTGuard");
    Require(
        schedulerTarget.DatabaseName == "SRO_VT_SHARD" &&
        schedulerTarget.ProcedureName == "dbo.RefreshRanking" &&
        schedulerTarget.Arguments == "@Force = 1",
        "Scheduler did not restore the manually entered database and procedure target correctly.");
    var legacySchedulerTarget = SqlAdminService.ParseSchedulerExecQuery(
        "EXEC [dbo].[LegacyJob]",
        "KMTGuard");
    Require(
        legacySchedulerTarget.DatabaseName == "KMTGuard" &&
        legacySchedulerTarget.ProcedureName == "dbo.LegacyJob",
        "Scheduler did not preserve compatibility with a legacy two-part procedure target.");

    static (float X, float Z) ToClientlessWorldPosition(SqlAdminService.ClientlessSpawnPosition position)
    {
        const float regionSize = 1920f;
        return (
            ((position.RegionId & 0xFF) * regionSize) + position.PosX,
            (((position.RegionId >> 8) & 0xFF) * regionSize) + position.PosZ);
    }

    foreach (var town in new[] { "Jangan", "Donwhang", "Hotan", "SamarKand", "Constantinople", "Alexandria North (SD)" })
    {
        var positions = SqlAdminService.CreateClientlessSpawnPositions(town, 50);
        Require(positions.Count == 50, $"Clientless spawn allocation failed for {town}.");
        var worldPositions = positions.Select(ToClientlessWorldPosition).ToArray();
        for (var first = 0; first < positions.Count; first++)
        {
            for (var second = first + 1; second < positions.Count; second++)
            {
                var dx = worldPositions[first].X - worldPositions[second].X;
                var dz = worldPositions[first].Z - worldPositions[second].Z;
                Require((dx * dx) + (dz * dz) >= 79.9f * 79.9f,
                    $"Clientless city distribution placed two {town} characters too close together.");
            }
        }

        var citySpan = MathF.Sqrt(worldPositions.Max(first =>
            worldPositions.Max(second =>
                MathF.Pow(first.X - second.X, 2f) + MathF.Pow(first.Z - second.Z, 2f))));
        Require(citySpan >= 1000f,
            $"Clientless city distribution did not spread {town} characters across the town.");
    }

    var alexandriaNorthPositions = SqlAdminService.CreateClientlessSpawnPositions("Alexandria North (SD)", 50);
    Require(alexandriaNorthPositions.All(position => position.RegionId is 23602 or 23603),
        "Alexandria North clientless allocation left the SD2 town regions.");

    Console.WriteLine("Admin Desktop SQL contract tests passed.");
    return;
}

var sqlReadOnlyIndex = Array.FindIndex(args, value =>
    value.Equals("--sql-read-only", StringComparison.OrdinalIgnoreCase));
if (sqlReadOnlyIndex >= 0)
{
    Require(sqlReadOnlyIndex + 1 < args.Length, "Settings.json path is required for SQL read-only checks.");
    var settings = new SettingsFileService().Load(args[sqlReadOnlyIndex + 1]);
    var sqlService = new SqlAdminService(settings.BuildConnectionString());
    var health = await sqlService.TestAsync();
    Require(health.IsConnected, $"SQL connection failed: {health.Message}");

    var whitelist = await sqlService.LoadPacketRulesAsync("__Whitelist");
    var blacklist = await sqlService.LoadPacketRulesAsync("__Blacklist");
    var hideAndSeekConfig = await sqlService.LoadHideAndSeekConfigAsync();
    var hideAndSeekLocations = await sqlService.LoadHideAndSeekLocationsAsync();
    var hideAndSeekRewards = await sqlService.LoadHideAndSeekRewardsAsync();
    var hideAndSeekSchedules = await sqlService.LoadHideAndSeekSchedulesAsync();
    var hideAndSeekRuns = await sqlService.LoadHideAndSeekRunsAsync();
    var vipTiers = await sqlService.LoadVipTiersAsync();
    var dashboard = await sqlService.LoadDashboardOperationsAsync();
    var diagnostics = await sqlService.LoadDiagnosticsAsync();
    var schedules = await sqlService.LoadSchedulerAsync();
    var schedulerDatabase = await sqlService.LoadCurrentDatabaseNameAsync();
    Require(hideAndSeekConfig.Rows.Count == 1, "Hide and Seek config row is missing.");
    Require(hideAndSeekLocations.Rows.Count > 0, "Hide and Seek has no locations.");
    Require(hideAndSeekRewards.Rows.Count > 0, "Hide and Seek has no rewards.");
    Require(vipTiers.Count == 6, "VIP tier configuration must contain six rows.");
    Require(vipTiers.Select(tier => tier.MinSilk).Distinct().Count() == vipTiers.Count,
        "VIP tier thresholds are not unique.");
    Require(vipTiers.All(tier => tier.IconID > 0 && !string.IsNullOrWhiteSpace(tier.IconPath)),
        "A VIP tier has no valid icon mapping.");
    Require(dashboard.UpcomingEvents.Columns.Contains("Event") &&
            dashboard.UpcomingEvents.Columns.Contains("Time") &&
            dashboard.RecentAdminActivity.Columns.Contains("Action"),
        "Dashboard operations tables do not expose the expected columns.");
    Require(diagnostics.Rows.Count > 0, "Dashboard readiness catalog is empty.");
    Require(schedules.Columns.Contains("ScheduleSummary") &&
            schedules.Columns.Contains("NextRunLocal") &&
            schedules.Columns.Contains("JobState"),
        "Procedure scheduler does not expose customer-friendly schedule columns.");
    Require(!string.IsNullOrWhiteSpace(schedulerDatabase),
        "Procedure scheduler could not resolve its default database name.");
    Console.WriteLine(
        $"Admin Desktop SQL read-only checks passed. Whitelist={whitelist.Rows.Count}, " +
        $"Blacklist={blacklist.Rows.Count}, HNSLocations={hideAndSeekLocations.Rows.Count}, " +
        $"HNSRewards={hideAndSeekRewards.Rows.Count}, HNSSchedules={hideAndSeekSchedules.Rows.Count}, " +
        $"HNSRuns={hideAndSeekRuns.Rows.Count}, VipTiers={vipTiers.Count}, " +
        $"ActiveEvents={dashboard.ActiveEvents}, ActiveSchedules={dashboard.ActiveSchedules}, " +
        $"Upcoming={dashboard.UpcomingEvents.Rows.Count}, Diagnostics={diagnostics.Rows.Count}, " +
        $"ProcedureSchedules={schedules.Rows.Count}, SchedulerDatabase={schedulerDatabase}.");
    return;
}

var vipRoundTripIndex = Array.FindIndex(args, value =>
    value.Equals("--vip-roundtrip", StringComparison.OrdinalIgnoreCase));
if (vipRoundTripIndex >= 0)
{
    Require(vipRoundTripIndex + 1 < args.Length, "Settings.json path is required for the VIP round-trip check.");
    var settings = new SettingsFileService().Load(args[vipRoundTripIndex + 1]);
    var sqlService = new SqlAdminService(settings.BuildConnectionString());
    var before = await sqlService.LoadVipTiersAsync();
    await sqlService.SaveVipTiersAsync(before);
    var after = await sqlService.LoadVipTiersAsync();

    Require(after.Count == 6, "VIP round-trip did not preserve all six tiers.");
    Require(before.OrderBy(tier => tier.RankCode)
            .Zip(after.OrderBy(tier => tier.RankCode))
            .All(pair =>
                pair.First.RankCode == pair.Second.RankCode &&
                pair.First.MinSilk == pair.Second.MinSilk &&
                pair.First.IconID == pair.Second.IconID &&
                pair.First.BuffSkillCode == pair.Second.BuffSkillCode),
        "VIP round-trip changed a configured threshold, icon, or buff.");
    Console.WriteLine("Admin Desktop VIP save/apply round-trip passed.");
    return;
}

if (args.Contains("--runtime-read-only", StringComparer.OrdinalIgnoreCase))
{
    var status = await service.GetStatusAsync();
    var players = await service.LoadOnlinePlayersSnapshotAsync();
    Console.WriteLine(
        $"Runtime read-only check: Running={status.IsRunning}, Services={status.Services.Count}, OnlinePlayers={players.Rows.Count}.");
    foreach (var item in status.Services)
        Console.WriteLine($"{item.Role}: Running={item.IsRunning}, Sessions={item.Sessions}, PID={item.ProcessId}");
    return;
}

if (args.Contains("--party-policy-read-only", StringComparer.OrdinalIgnoreCase))
{
    var policy = await service.GetClientlessPartyFormPolicyAsync()
        ?? throw new InvalidOperationException("Clientless Party Form policy IPC returned no payload.");
    Require(policy.Mode is "SoloForms" or "GroupsOf8",
        "Clientless Party Form policy returned an unsupported mode.");
    Console.WriteLine(
        $"Party policy read-only check: Enabled={policy.Enabled}, Mode={policy.Mode}, Title={policy.Title}.");
    return;
}

if (args.Contains("--party-policy-roundtrip", StringComparer.OrdinalIgnoreCase))
{
    var original = await service.GetClientlessPartyFormPolicyAsync()
        ?? throw new InvalidOperationException("Clientless Party Form policy IPC returned no payload.");

    ClientlessPartyFormCommandPayload WithMode(string mode, bool enabled) => new()
    {
        Enabled = enabled,
        Mode = mode,
        Title = original.Title,
        MinLevel = original.MinLevel,
        MaxLevel = original.MaxLevel,
        Purpose = original.Purpose,
        SettingsFlag = original.SettingsFlag
    };

    try
    {
        await service.SetClientlessPartyFormPolicyAsync(WithMode("SoloForms", true));
        var solo = await service.GetClientlessPartyFormPolicyAsync();
        Require(solo?.Enabled == true && solo.Mode == "SoloForms",
            "The Individual Party Forms policy did not persist through Agent IPC and SQL.");

        await service.SetClientlessPartyFormPolicyAsync(WithMode("GroupsOf8", true));
        var grouped = await service.GetClientlessPartyFormPolicyAsync();
        Require(grouped?.Enabled == true && grouped.Mode == "GroupsOf8",
            "The Groups of 8 policy did not persist through Agent IPC and SQL.");
    }
    finally
    {
        await service.SetClientlessPartyFormPolicyAsync(WithMode(original.Mode, original.Enabled));
    }

    Console.WriteLine("Clientless Party Form mode IPC/SQL round-trip passed and the original policy was restored.");
    return;
}

if (args.Contains("--runtime-stop-only", StringComparer.OrdinalIgnoreCase))
{
    var status = await service.StopAsync();
    Require(!status.IsRunning, "Runtime services did not stop cleanly.");
    Console.WriteLine("KMTGuard runtime services stopped cleanly.");
    return;
}

if (args.Contains("--runtime-start-only", StringComparer.OrdinalIgnoreCase))
{
    var status = await service.StartAsync();
    Require(status.IsRunning && status.Services.Count == 3 && status.Services.All(item => item.IsRunning),
        "Runtime services did not start cleanly.");
    Console.WriteLine("KMTGuard runtime services started cleanly.");
    return;
}

try
{
    await service.StopAsync();

    var started = await service.StartAsync();
    Require(started.IsRunning, "Start All did not start every service.");
    Require(started.Services.Count == 3, "The runtime did not report three services.");
    Require(started.Services.All(item => item.IsRunning), "At least one service is not running.");
    Require(started.Services.All(item => item.Listeners > 0), "At least one service has no active listener.");

    var agentPid = started.Services.Single(item => item.Role == FilterRole.Agent).ProcessId;
    var gatewayPid = started.Services.Single(item => item.Role == FilterRole.Gateway).ProcessId;

    var downloadStopped = await service.StopRoleAsync(FilterRole.Download);
    Require(!downloadStopped.Services.Single(item => item.Role == FilterRole.Download).IsRunning,
        "Download did not stop independently.");
    Require(downloadStopped.Services.Single(item => item.Role == FilterRole.Agent).ProcessId == agentPid,
        "Stopping Download changed the Agent process.");
    Require(downloadStopped.Services.Single(item => item.Role == FilterRole.Gateway).ProcessId == gatewayPid,
        "Stopping Download changed the Gateway process.");

    var downloadRestarted = await service.StartRoleAsync(FilterRole.Download);
    Require(downloadRestarted.IsRunning, "Download did not restart independently.");

    _ = await service.LoadOnlinePlayersSnapshotAsync();
    var clientlessStatus = await service.GetClientlessStatusAsync();
    Require(!string.IsNullOrWhiteSpace(clientlessStatus), "Clientless status IPC returned no data.");

    var languageServices = await service.SetPlayerLanguageAsync("Turkish", "English");
    Require(languageServices == 3, "The live language command did not reach every running service.");
    var invalidLanguageRejected = false;
    try
    {
        await service.SetPlayerLanguageAsync("LanguageFileThatDoesNotExist", "Turkish");
    }
    catch (InvalidOperationException)
    {
        invalidLanguageRejected = true;
    }

    Require(invalidLanguageRejected, "The live language command accepted a missing language file.");
    languageServices = await service.SetPlayerLanguageAsync("English", "Turkish");
    Require(languageServices == 3, "The live language command did not restore every running service.");

    Console.WriteLine("Split runtime integration tests passed.");
    foreach (var item in downloadRestarted.Services)
    {
        Console.WriteLine(
            $"{item.Role}: PID={item.ProcessId}, RAM={item.MemoryMb:N1} MB, " +
            $"Listeners={item.Listeners}, Sessions={item.Sessions}");
    }
}
finally
{
    await service.StopAsync();
}
