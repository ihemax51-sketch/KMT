using KMTGuard.PacketHandlerManager;
using KMTGuard.ServerManagers;
using System.Collections.Concurrent;
using KMTGuard.Features.AutoEvents;
using KMTGuard.Features.ItemChest;
using KMTGuard.Features.UniqueHistory;
using KMTGuard.Clientless;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Localization;
using KMTGuard.SessionManager;
using KMTGuard.SettingManager;
using KMTGuard.Helpers;
using KMTGuard.Server.AgentPacketHandler;
using KMTGuard.Server.GatewayPacketHandler;
using KMTGuard.Servers.PacketHandler;
using SilkroadSecurityAPI;
using System.Net;
using System.Net.Sockets;
using System.Buffers.Binary;
using System.Security.Cryptography;

const ushort BlockOpcode = 0x7770;
const ushort DisconnectOpcode = 0x7771;
const ushort OverrideOpcode = 0x7772;
const ushort DataOpcode = 0x7773;
const ushort ServerOpcode = 0x7774;

await VerifyClientBlockStopsPipeline();
await VerifyClientDisconnectStopsPipeline();
await VerifyOverrideFlowsThroughAndSurvives();
await VerifyPacketDataTracksPreviousHandler();
await VerifyServerBlockStopsPipeline();
await VerifyInternalPacketAuthentication();
await VerifyDelayedJobScheduling();
await VerifyDatabaseJobLifecycle();
await VerifyConcurrentPacketFinalization();
VerifyGatewayCredentialLifecycle();
VerifyRuntimeControlAuthentication();
VerifyQueuedServerPacketSurvivesHandshake();
VerifyConfiguredShardStatusIsDeterministic();
VerifyGmIpMaintenanceBypass();
VerifyNpcUniqueIdSnapshotCompatibility();
VerifyNewAlchemyRemainsUnavailable();
VerifyBotIdentityClassification();
VerifyRegionAdmissionPolicy();
VerifyManagedClientlessGatewayTrustBoundary();
VerifySessionRegistryIndexes();
VerifyClientlessPartyFormPacket();
VerifyClientlessHuntPackets();
VerifyClientlessSkillClassification();
VerifyClientlessTeleportProtocol();
VerifyClientlessInitialWorldSynchronization();
VerifyClientlessTownDistribution();
VerifyHideAndSeekVisibilityCommand();
VerifyAutoEventTableContracts();
VerifyReadPastCompatibility();
VerifySchedulerOccurrenceCalculation();
VerifySchedulerValidation();
VerifyItemChestContracts();
VerifyUniqueHistoryPacketContract();
VerifyPlayerLanguages();
VerifySilkStallContracts();
VerifyOfflineStallContracts();
VerifyMassivePacketLimits();
VerifyHwidV2Proofs();
VerifyGatewayLoginFailureAllowsRetry();
VerifyQuickLoginV2Crypto();
VerifyQuickLoginNonceRefresh();
VerifyQuickLoginSettingsBootstrap();
VerifySecondaryPasswordV2Format();
VerifyAuthenticatedSessionLiveness();

static async Task VerifyDatabaseJobLifecycle()
{
    var releaseWorkers = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
    var workersStarted = new CountdownEvent(2);
    Task first = DatabaseJobQueue.RunAsyncForTesting(
        async _ =>
        {
            workersStarted.Signal();
            await releaseWorkers.Task;
        },
        TimeSpan.FromSeconds(5),
        "queue lifecycle blocker one");
    Task second = DatabaseJobQueue.RunAsyncForTesting(
        async _ =>
        {
            workersStarted.Signal();
            await releaseWorkers.Task;
        },
        TimeSpan.FromSeconds(5),
        "queue lifecycle blocker two");
    Assert(workersStarted.Wait(TimeSpan.FromSeconds(2)), "Database queue workers did not start.");

    int cancelledMutationCount = 0;
    await AssertThrowsAsync<TimeoutException>(() => DatabaseJobQueue.RunAsyncForTesting(
        _ =>
        {
            Interlocked.Increment(ref cancelledMutationCount);
            return Task.CompletedTask;
        },
        TimeSpan.FromMilliseconds(50),
        "cancel before start"));
    Assert(cancelledMutationCount == 0, "A database job cancelled before start still executed.");

    int acceptedBackgroundJobs = 0;
    while (acceptedBackgroundJobs < 600 && DatabaseJobQueue.TryQueueBackground(
               _ => Task.CompletedTask,
               operation: "queue saturation probe"))
    {
        acceptedBackgroundJobs++;
    }
    Assert(acceptedBackgroundJobs < 600, "The bounded database queue did not reject saturation.");

    releaseWorkers.TrySetResult();
    await Task.WhenAll(first, second);

    var runningStarted = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
    var releaseRunning = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
    int runningMutationCount = 0;
    Task running = DatabaseJobQueue.RunAsyncForTesting(
        async _ =>
        {
            runningStarted.TrySetResult();
            await releaseRunning.Task;
            Interlocked.Increment(ref runningMutationCount);
        },
        TimeSpan.FromMilliseconds(50),
        "running definitive outcome");
    await runningStarted.Task;
    await Task.Delay(100);
    Assert(!running.IsCompleted, "A running database mutation abandoned its waiter at the wait deadline.");
    releaseRunning.TrySetResult();
    await running;
    Assert(runningMutationCount == 1, "A running database mutation did not complete exactly once.");

    await AssertThrowsAsync<InvalidOperationException>(() => DatabaseJobQueue.RunAsyncForTesting(
        _ => throw new InvalidOperationException("expected database job failure"),
        TimeSpan.FromSeconds(1),
        "exception propagation"));
}

static void VerifyGatewayCredentialLifecycle()
{
    using var credential = new GatewayCredential();
    credential.Replace("first-password");
    Assert(credential.HasValue, "Gateway credential did not retain an active credential.");
    Assert(credential.Reveal() == "first-password", "Gateway credential changed the password value.");
    credential.Replace("replacement-password");
    Assert(credential.Reveal() == "replacement-password", "Gateway credential replacement failed.");
    credential.Clear();
    Assert(!credential.HasValue, "Gateway credential was not cleared.");
    AssertThrows<InvalidOperationException>(
        () => credential.Reveal(),
        "A cleared Gateway credential remained readable.");
    credential.Clear();
}
VerifyExternalBotPacketCompatibility();

Console.WriteLine("Packet pipeline smoke tests passed.");

static async Task VerifyConcurrentPacketFinalization()
{
    var broadcast = new Packet(0x7775);
    broadcast.WriteUInt32(0x12345678);
    broadcast.WriteUnicode("broadcast-payload");

    var tasks = Enumerable.Range(0, 1000).Select(async index =>
    {
        await Task.Delay(index % 7);
        broadcast.ToReadOnly();
        var copy = broadcast.CreateReadOnlyClone();
        Assert(copy.ReadUInt32() == 0x12345678 &&
               copy.ReadUnicode() == "broadcast-payload" &&
               copy.RemainingRead() == 0,
            "Concurrent broadcast packet finalization corrupted a session copy.");
    });

    await Task.WhenAll(tasks);
}

static void VerifyRuntimeControlAuthentication()
{
    var request = new KMTGuard.RuntimeContract.RuntimeRequest
    {
        Command = "runtime.ping",
        Payload = "{}"
    };
    KMTGuard.RuntimeContract.RuntimeRequestAuthentication.Sign(request);
    Assert(KMTGuard.RuntimeContract.RuntimeRequestAuthentication.Verify(request, TimeSpan.FromMinutes(1)),
        "A signed runtime-control request did not verify.");
    request.Payload = "{\"tampered\":true}";
    Assert(!KMTGuard.RuntimeContract.RuntimeRequestAuthentication.Verify(request, TimeSpan.FromMinutes(1)),
        "Runtime-control authentication accepted a modified payload.");
}

static void VerifyExternalBotPacketCompatibility()
{
    var customNotice = new Packet(ExternalBotPacketCompatibility.CustomNoticeOpcode);
    customNotice.WriteUInt8(NoticeType.WARNING);
    customNotice.WriteUnicode("Blocked chat");

    Assert(ExternalBotPacketCompatibility.TryAdaptServerPacket(
               isExternalBot: true,
               customNotice,
               out var compatibleNotice) &&
           compatibleNotice != null &&
           compatibleNotice.Opcode == ExternalBotPacketCompatibility.NativeChatOpcode,
        "An external bot custom notice was not converted to the native chat notice protocol.");

    compatibleNotice!.ToReadOnly();
    Assert(compatibleNotice.ReadUInt8() == 7 &&
           compatibleNotice.ReadAscii() == "Blocked chat" &&
           compatibleNotice.RemainingRead() == 0,
        "The external bot native notice payload is malformed.");

    var normalClientNotice = new Packet(ExternalBotPacketCompatibility.CustomNoticeOpcode);
    normalClientNotice.WriteUInt8(NoticeType.WARNING);
    normalClientNotice.WriteUnicode("Normal client");
    Assert(ExternalBotPacketCompatibility.TryAdaptServerPacket(
               isExternalBot: false,
               normalClientNotice,
               out var unchangedNotice) &&
           ReferenceEquals(normalClientNotice, unchangedNotice),
        "A normal client custom notice was unexpectedly rewritten.");

    var malformedNotice = new Packet(ExternalBotPacketCompatibility.CustomNoticeOpcode);
    malformedNotice.WriteUInt8(NoticeType.WARNING);
    Assert(!ExternalBotPacketCompatibility.TryAdaptServerPacket(
               isExternalBot: true,
               malformedNotice,
               out var rejectedNotice) &&
           rejectedNotice == null,
        "A malformed custom notice was allowed to reach an external bot.");

    var nativeResponse = new Packet(0xB025);
    Assert(ExternalBotPacketCompatibility.TryAdaptServerPacket(
               isExternalBot: true,
               nativeResponse,
               out var unchangedNativeResponse) &&
           ReferenceEquals(nativeResponse, unchangedNativeResponse),
        "The native chat acknowledgement was unexpectedly rewritten.");
}

static void VerifyGatewayLoginFailureAllowsRetry()
{
    using var verifiedClient = new TcpClient();
    var verifiedSession = new Session(verifiedClient, null!)
    {
        GatewayAuthenticationState = GatewayAuthenticationState.Released,
        DeviceKeyThumbprint = new string('A', 64),
        DevicePublicKey = "verified-public-key",
        PendingQuickLogin = true,
        PendingPrimaryLogin = true,
        PendingPrimaryLoginStartedAt = 123
    };
    verifiedSession.SessionData.Hwid = new string('B', 64);

    KMTGuard.Server.GatewayServer.RestoreAuthenticationAfterFailedLogin(verifiedSession);

    Assert(verifiedSession.GatewayAuthenticationState ==
               GatewayAuthenticationState.AwaitingPrimaryCredentials &&
           !verifiedSession.PendingQuickLogin &&
           !verifiedSession.PendingPrimaryLogin &&
           verifiedSession.PendingPrimaryLoginStartedAt == 0,
        "A failed Gateway login did not restore an attested session for retry.");

    using var unattestedClient = new TcpClient();
    var unattestedSession = new Session(unattestedClient, null!)
    {
        GatewayAuthenticationState = GatewayAuthenticationState.Released
    };

    KMTGuard.Server.GatewayServer.RestoreAuthenticationAfterFailedLogin(unattestedSession);

    Assert(unattestedSession.GatewayAuthenticationState == GatewayAuthenticationState.AwaitingHwid,
        "A failed Gateway login incorrectly restored an unattested session.");
}

static void VerifyClientlessInitialWorldSynchronization()
{
    var sawSpawnBegin = false;
    Assert(!ClientlessSession.AdvanceInitialWorldSynchronization(0x3018, ref sawSpawnBegin),
        "A stray group-spawn end incorrectly unlocked Clientless automation.");
    Assert(!ClientlessSession.AdvanceInitialWorldSynchronization(0x3017, ref sawSpawnBegin) && sawSpawnBegin,
        "The initial vSRO group-spawn begin was not recorded.");
    Assert(!ClientlessSession.AdvanceInitialWorldSynchronization(0x3019, ref sawSpawnBegin),
        "A group-spawn data fragment incorrectly unlocked Clientless automation.");
    Assert(ClientlessSession.AdvanceInitialWorldSynchronization(0x3018, ref sawSpawnBegin),
        "The complete 0x3017/0x3019/0x3018 world-entry cycle did not unlock Clientless automation.");

    Assert(!ClientlessSession.HasInitialWorldDataReady(0, 0),
        "Initial world synchronization attempted a blocking socket read without ready data.");
    Assert(ClientlessSession.HasInitialWorldDataReady(1, 0),
        "A decrypted initial-world packet was not treated as ready data.");
    Assert(ClientlessSession.HasInitialWorldDataReady(0, 16),
        "Available initial-world TCP bytes were not treated as ready data.");
}

static void VerifyClientlessTownDistribution()
{
    static (float X, float Z) ToWorldPosition(ClientlessTownPositions.Position position)
    {
        const float regionSize = 1920f;
        return (
            ((position.RegionID & 0xFF) * regionSize) + position.X,
            (((position.RegionID >> 8) & 0xFF) * regionSize) + position.Z);
    }

    foreach (var town in new[]
             {
                 "Jangan", "Donwhang", "Hotan", "SamarKand", "Constantinople", "Alexandria North (SD)"
             })
    {
        var positions = ClientlessTownPositions.ResolveAll(town, 50).ToArray();
        var repeated = ClientlessTownPositions.ResolveAll(town, 50).ToArray();
        Assert(positions.Length == 50 && repeated.SequenceEqual(positions),
            $"Clientless town parking is not complete and stable for {town}.");

        var worldPositions = positions.Select(ToWorldPosition).ToArray();
        for (var first = 0; first < worldPositions.Length; first++)
        {
            for (var second = first + 1; second < worldPositions.Length; second++)
            {
                var dx = worldPositions[first].X - worldPositions[second].X;
                var dz = worldPositions[first].Z - worldPositions[second].Z;
                Assert((dx * dx) + (dz * dz) >= 79.9f * 79.9f,
                    $"Two parked Clientless characters are too close together in {town}.");
            }
        }

        var citySpan = MathF.Sqrt(worldPositions.Max(first =>
            worldPositions.Max(second =>
                MathF.Pow(first.X - second.X, 2f) + MathF.Pow(first.Z - second.Z, 2f))));
        Assert(citySpan >= 1000f,
            $"Clientless parking does not cover enough of {town}.");
    }

    var alexandria = ClientlessTownPositions.ResolveAll("Alexandria North (SD)", 50);
    Assert(alexandria.All(position => position.RegionID is 23602 or 23603),
        "Alexandria North parking left the configured SD town regions.");
}

static void VerifyClientlessSkillClassification()
{
    Assert(ClientlessManager.ResolvePrimaryWeaponMastery(2) == 257 &&
           ClientlessManager.ResolvePrimaryWeaponMastery(5) == 258 &&
           ClientlessManager.ResolvePrimaryWeaponMastery(6) == 259 &&
           ClientlessManager.ResolvePrimaryWeaponMastery(11) == 514 &&
           ClientlessManager.ResolvePrimaryWeaponMastery(12) == 515 &&
           ClientlessManager.ResolvePrimaryWeaponMastery(13) == 515 &&
           ClientlessManager.ResolvePrimaryWeaponMastery(10) == 516 &&
           ClientlessManager.ResolvePrimaryWeaponMastery(15) == 518,
        "The equipped weapon-to-primary-mastery mapping drifted from the clientless creation profiles.");
    var attackParameters = new[] { 1, ClientlessManager.AttackSkillParameter, 5, 100 };
    Assert(ClientlessManager.IsMonsterAttackSkill(
            basicActivity: 2,
            targetRequired: true,
            targetsEnemyMonster: true,
            targetsSelf: false,
            targetsParty: false,
            selectsDeadBody: false,
            parameters: attackParameters),
        "A learned entity-targeted monster attack was excluded from the clientless combat catalog.");
    Assert(!ClientlessManager.IsMonsterAttackSkill(
            basicActivity: 1,
            targetRequired: false,
            targetsEnemyMonster: false,
            targetsSelf: false,
            targetsParty: false,
            selectsDeadBody: false,
            parameters: attackParameters),
        "A Chinese imbue was incorrectly classified as a direct monster attack.");
    Assert(ClientlessManager.IsMonsterAttackSkill(
            basicActivity: 1,
            targetRequired: true,
            targetsEnemyMonster: true,
            targetsSelf: false,
            targetsParty: false,
            selectsDeadBody: false,
            parameters: attackParameters),
        "An entity-targeted European attack was rejected solely because its vSRO activity value is also used by imbues.");
    Assert(ClientlessManager.IsMonsterAttackSkill(
            basicActivity: 2,
            targetRequired: false,
            targetsEnemyMonster: true,
            targetsSelf: false,
            targetsParty: false,
            selectsDeadBody: false,
            parameters: attackParameters),
        "A valid European monster-area attack was excluded only because its vSRO Target_Required flag is not set.");

    var chineseBuffParameters = new[]
    {
        ClientlessManager.PrimaryBuffSkillType,
        ClientlessManager.DurationSkillParameter,
        60_000
    };
    Assert(ClientlessManager.IsSelfBuffSkill(
            basicActivity: 2,
            targetRequired: false,
            targetsSelf: false,
            targetsParty: false,
            targetsEnemyMonster: false,
            targetsEnemyPlayer: false,
            selectsDeadBody: false,
            parameters: chineseBuffParameters),
        "A persistent Chinese Fire/Lightning-style self buff was excluded.");

    var imbueParameters = new[]
    {
        0,
        ClientlessManager.AttackSkillParameter,
        ClientlessManager.DurationSkillParameter,
        30_000
    };
    Assert(ClientlessManager.IsSelfBuffSkill(
            basicActivity: 1,
            targetRequired: false,
            targetsSelf: false,
            targetsParty: false,
            targetsEnemyMonster: false,
            targetsEnemyPlayer: false,
            selectsDeadBody: false,
            parameters: imbueParameters),
        "A learned Chinese imbue was excluded from the self-buff catalog.");

    var timedAttackParameters = new[]
    {
        1,
        ClientlessManager.AttackSkillParameter,
        ClientlessManager.DurationSkillParameter,
        10_000
    };
    Assert(!ClientlessManager.IsSelfBuffSkill(
            basicActivity: 2,
            targetRequired: true,
            targetsSelf: false,
            targetsParty: false,
            targetsEnemyMonster: true,
            targetsEnemyPlayer: false,
            selectsDeadBody: false,
            parameters: timedAttackParameters),
        "An enemy damage-over-time attack was incorrectly classified as a self buff.");

    Assert(ClientlessManager.IsSkillCompatibleWithEquippedItems(
            requiredWeapon1: ClientlessManager.AnyWeaponRequirement,
            requiredWeapon2: ClientlessManager.AnyWeaponRequirement,
            weaponTypeId3: 6,
            weaponTypeId4: 11,
            offhandTypeId3: null,
            offhandTypeId4: null,
            parameters: Array.Empty<int>()),
        "A genuinely weapon-agnostic skill was rejected.");

    var staffRequirement = new[]
    {
        ClientlessManager.RequiredItemSkillParameter,
        6,
        11
    };
    Assert(ClientlessManager.IsSkillCompatibleWithEquippedItems(
            ClientlessManager.AnyWeaponRequirement,
            ClientlessManager.AnyWeaponRequirement,
            weaponTypeId3: 6,
            weaponTypeId4: 11,
            offhandTypeId3: null,
            offhandTypeId4: null,
            parameters: staffRequirement),
        "A Wizard Staff skill with a matching reqi marker was rejected.");
    Assert(!ClientlessManager.IsSkillCompatibleWithEquippedItems(
            ClientlessManager.AnyWeaponRequirement,
            ClientlessManager.AnyWeaponRequirement,
            weaponTypeId3: 6,
            weaponTypeId4: 10,
            offhandTypeId3: null,
            offhandTypeId4: null,
            parameters: staffRequirement),
        "A Wizard Staff-only reqi skill was admitted for a Warlock Rod.");

    var shieldRequirement = new[]
    {
        ClientlessManager.RequiredItemSkillParameter,
        4,
        2
    };
    Assert(ClientlessManager.IsSkillCompatibleWithEquippedItems(
            ClientlessManager.AnyWeaponRequirement,
            ClientlessManager.AnyWeaponRequirement,
            weaponTypeId3: 6,
            weaponTypeId4: 7,
            offhandTypeId3: 4,
            offhandTypeId4: 2,
            parameters: shieldRequirement),
        "A shield-dependent European buff did not recognize the equipped offhand shield.");

    Assert(ClientlessManager.IsSkillCompatibleWithEquippedItems(
            requiredWeapon1: 12,
            requiredWeapon2: 13,
            weaponTypeId3: 6,
            weaponTypeId4: 13,
            offhandTypeId3: null,
            offhandTypeId4: null,
            parameters: Array.Empty<int>()),
        "The second explicit weapon requirement was ignored.");

    var weaponSql = ClientlessManager.BuildSkillWeaponCompatibilityPredicate("Skill", "Weapon", "Offhand");
    Assert(weaponSql.Contains("Skill.Param1, 0) = @RequiredItemParam", StringComparison.Ordinal) &&
           weaponSql.Contains("Skill.Param48 = @RequiredItemParam", StringComparison.Ordinal) &&
           weaponSql.Contains("Offhand.TypeID4", StringComparison.Ordinal),
        "The runtime SQL does not scan all complete reqi marker triples or the equipped offhand.");

    var attackSql = ClientlessManager.BuildMonsterAttackSkillPredicate("Skill");
    Assert(attackSql.Contains("TargetGroup_Enemy_M", StringComparison.Ordinal) &&
           attackSql.Contains("Target_Required, 0) = 0", StringComparison.Ordinal) &&
           attackSql.Contains("@AttackParam", StringComparison.Ordinal),
        "The runtime attack-skill SQL drifted from the vSRO damage-skill contract.");

    var buffSql = ClientlessManager.BuildSelfBuffSkillPredicate("Skill");
    Assert(buffSql.Contains("Param1, -1) = @BuffPrimaryType", StringComparison.Ordinal) &&
           buffSql.Contains("@DurationParam", StringComparison.Ordinal) &&
           buffSql.Contains("Basic_Activity, 0) = 1", StringComparison.Ordinal),
        "The runtime self-buff SQL drifted from the persistent-buff/imbue contract.");
}

static void VerifyAuthenticatedSessionLiveness()
{
    const int timeoutSeconds = 180;
    const long sessionStart = 1_000;

    Assert(!Session.IsClientActivityDeadlineExceeded(
            sessionStart + (timeoutSeconds * 1000L) - 1,
            sessionStart,
            timeoutSeconds),
        "Authenticated session activity expired before its configured deadline.");
    Assert(Session.IsClientActivityDeadlineExceeded(
            sessionStart + (timeoutSeconds * 1000L),
            sessionStart,
            timeoutSeconds),
        "A silent authenticated session survived its configured inactivity deadline.");

    var recentCombatActivity = sessionStart + 179_500;
    Assert(!Session.IsClientActivityDeadlineExceeded(
            sessionStart + (timeoutSeconds * 1000L),
            recentCombatActivity,
            timeoutSeconds),
        "Recent authenticated client traffic did not refresh session liveness.");
}

static void VerifyReadPastCompatibility()
{
    var directory = new DirectoryInfo(AppContext.BaseDirectory);
    while (directory != null && !File.Exists(Path.Combine(directory.FullName, "PROJECT_REFERENCE.md")))
        directory = directory.Parent;

    Assert(directory != null, "The repository root could not be located for SQL lock-hint validation.");
    var runtimeSource = Path.Combine(
        directory!.FullName,
        "filter",
        "KMTGuardnew",
        "KMTGuard");

    foreach (var path in Directory.EnumerateFiles(runtimeSource, "*.cs", SearchOption.AllDirectories))
    {
        var lines = File.ReadAllLines(path);
        for (var index = 0; index < lines.Length; index++)
        {
            if (!lines[index].Contains("READPAST", StringComparison.OrdinalIgnoreCase))
                continue;

            Assert(
                lines[index].Contains("READCOMMITTEDLOCK", StringComparison.OrdinalIgnoreCase),
                $"READPAST lacks READCOMMITTEDLOCK compatibility in {path}:{index + 1}.");

            var searchStart = Math.Max(0, index - 16);
            var establishesReadCommitted = lines
                .Skip(searchStart)
                .Take(index - searchStart + 1)
                .Any(line => line.Contains(
                    "SET TRANSACTION ISOLATION LEVEL READ COMMITTED",
                    StringComparison.OrdinalIgnoreCase));
            Assert(
                establishesReadCommitted,
                $"READPAST does not establish READ COMMITTED isolation in {path}:{index + 1}.");
        }
    }
}

static void VerifyQuickLoginV2Crypto()
{
    var key = RandomNumberGenerator.GetBytes(32);
    const string password = "P@ssw0rd;with-leading-00";
    var encrypted = SERVER_DLL_SETTINGS_RESPONSE.EncryptQuickLoginPassword(key, password);
    Assert(encrypted.Nonce.Length == 12 && encrypted.Tag.Length == 16,
        "Quick Login v2 did not produce AES-GCM nonce/tag lengths.");
    Assert(SERVER_DLL_SETTINGS_RESPONSE.DecryptQuickLoginPassword(
               key, encrypted.Cipher, encrypted.Nonce, encrypted.Tag) == password,
        "Quick Login v2 AES-GCM round trip failed.");

    encrypted.Cipher[0] ^= 0x01;
    AssertThrows<AuthenticationTagMismatchException>(
        () => SERVER_DLL_SETTINGS_RESPONSE.DecryptQuickLoginPassword(
            key, encrypted.Cipher, encrypted.Nonce, encrypted.Tag),
        "Quick Login v2 accepted tampered ciphertext.");

    var now = (int)DateTimeOffset.UtcNow.ToUnixTimeSeconds();
    Assert(SERVER_DLL_SETTINGS_RESPONSE.ValidateQuickLoginTimestamp(now),
        "Quick Login rejected a current request timestamp.");
    Assert(!SERVER_DLL_SETTINGS_RESPONSE.ValidateQuickLoginTimestamp(now - 121),
        "Quick Login accepted a request outside its two-minute window.");
}

static void VerifyQuickLoginNonceRefresh()
{
    const string hwid = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
    using var rsa = new RSACryptoServiceProvider(2048) { PersistKeyInCsp = false };
    using var client = new TcpClient();
    var session = new Session(client, null!)
    {
        GatewayAuthenticationState = GatewayAuthenticationState.AwaitingPrimaryCredentials,
        VerifiedHwidNonce = new string('A', 48)
    };

    Assert(SERVER_DLL_SETTINGS_RESPONSE.CanAcceptQuickLoginAction(session),
        "A verified Gateway session could not start its first Quick Login action.");
    Assert(SERVER_DLL_SETTINGS_RESPONSE.TryPrepareNextQuickLoginNonce(
               session, out var challengePacket) && challengePacket?.Opcode == 0x165A,
        "Quick Login did not issue the next signed device challenge.");
    Assert(session.QuickLoginNonceRefreshPending && session.VerifiedHwidNonce.Length == 0 &&
           !SERVER_DLL_SETTINGS_RESPONSE.CanAcceptQuickLoginAction(session),
        "Quick Login accepted another action while its nonce refresh was pending.");

    var firstNonce = session.HwidChallenge;
    var firstChallenge = string.Join('|', HwidSecurity.ProtocolMarker,
        HwidSecurity.GatewayRole,
        session.HwidChallengeIssuedAtUnix.ToString(System.Globalization.CultureInfo.InvariantCulture),
        firstNonce);
    Assert(HwidSecurity.TryValidateResponse(
               session, CreateHwidResponse(firstChallenge, hwid, rsa), out _),
        "The Quick Login nonce-refresh proof was rejected.");
    Assert(SERVER_DLL_SETTINGS_RESPONSE.TakeQuickLoginNonceRefreshMarker(session) &&
           session.VerifiedHwidNonce.Length == 48 &&
           SERVER_DLL_SETTINGS_RESPONSE.CanAcceptQuickLoginAction(session),
        "Quick Login did not become reusable after the nonce-refresh proof.");

    Assert(SERVER_DLL_SETTINGS_RESPONSE.TryPrepareNextQuickLoginNonce(
               session, out var secondChallengePacket) && secondChallengePacket?.Opcode == 0x165A &&
           !string.Equals(firstNonce, session.HwidChallenge, StringComparison.Ordinal),
        "Quick Login reused its previous one-time nonce on the next action.");
}

static void VerifyQuickLoginSettingsBootstrap()
{
    var settingsPath = Path.Combine(
        Path.GetTempPath(),
        $"kmtguard-settings-test-{Guid.NewGuid():N}.json");
    var previousPath = Environment.GetEnvironmentVariable("KMTGUARD_SETTINGS_PATH");

    try
    {
        var testSettings = (Settings)new Settings().Init();
        testSettings.Password = "test-only";
        testSettings.QuickLoginCredentialTarget = $"KMTGuard/Test/{Guid.NewGuid():N}";
        File.WriteAllText(
            settingsPath,
            Newtonsoft.Json.JsonConvert.SerializeObject(testSettings, Newtonsoft.Json.Formatting.Indented));
        Environment.SetEnvironmentVariable("KMTGUARD_SETTINGS_PATH", settingsPath);

        byte[] firstKey;
        using (var manager = new SettingsManager())
            firstKey = manager.GetOrCreateQuickLoginMasterKey();

        var persisted = Newtonsoft.Json.JsonConvert.DeserializeObject<Settings>(File.ReadAllText(settingsPath));
        Assert(firstKey.Length == 32 && !string.IsNullOrWhiteSpace(persisted?.QuickLoginMasterKey),
            "Quick Login did not generate and persist its Settings.json key.");

        using var secondManager = new SettingsManager();
        var secondKey = secondManager.GetOrCreateQuickLoginMasterKey();
        Assert(CryptographicOperations.FixedTimeEquals(firstKey, secondKey),
            "Quick Login replaced its persisted key during restart.");

        CryptographicOperations.ZeroMemory(firstKey);
        CryptographicOperations.ZeroMemory(secondKey);
    }
    finally
    {
        Environment.SetEnvironmentVariable("KMTGUARD_SETTINGS_PATH", previousPath);
        if (File.Exists(settingsPath))
            File.Delete(settingsPath);
    }
}

static void VerifySecondaryPasswordV2Format()
{
    Assert(!SERVER_DLL_SETTINGS_RESPONSE.IsValidSecondaryPassword("12345"),
        "Secondary Password accepted five digits.");
    Assert(SERVER_DLL_SETTINGS_RESPONSE.IsValidSecondaryPassword("001234"),
        "Secondary Password rejected six digits with leading zeroes.");
    Assert(SERVER_DLL_SETTINGS_RESPONSE.IsValidSecondaryPassword("00123456"),
        "Secondary Password rejected eight digits with leading zeroes.");
    Assert(!SERVER_DLL_SETTINGS_RESPONSE.IsValidSecondaryPassword("001234567"),
        "Secondary Password accepted nine digits.");
    Assert(!SERVER_DLL_SETTINGS_RESPONSE.IsValidSecondaryPassword("12A456"),
        "Secondary Password accepted a non-digit value.");
}

static void VerifyMassivePacketLimits()
{
    var security = new Security(maxMassiveFragments: 2, maxMassiveBytes: 8);
    RecvRaw(security, BuildMassiveStart(2, 0x7777));
    RecvRaw(security, BuildRawPacket(0x600D, 0, 1, 2, 3, 4));
    RecvRaw(security, BuildRawPacket(0x600D, 0, 5, 6, 7, 8));
    var packets = security.TransferIncoming();
    Assert(packets is { Count: 1 } && packets[0].Opcode == 0x7777 && packets[0].GetBytes().Length == 8,
        "A valid bounded massive packet was not assembled correctly.");

    AssertThrows<InvalidDataException>(
        () => RecvRaw(new Security(2, 8), BuildMassiveStart(0, 0x7777)),
        "A zero-fragment massive packet was accepted.");
    AssertThrows<InvalidDataException>(
        () => RecvRaw(new Security(2, 8), BuildMassiveStart(3, 0x7777)),
        "An excessive massive fragment count was accepted.");

    var oversized = new Security(2, 4);
    RecvRaw(oversized, BuildMassiveStart(1, 0x7777));
    AssertThrows<InvalidDataException>(
        () => RecvRaw(oversized, BuildRawPacket(0x600D, 0, 1, 2, 3, 4, 5)),
        "An excessive massive payload was accepted.");

    var overlapping = new Security(2, 8);
    RecvRaw(overlapping, BuildMassiveStart(1, 0x7777));
    AssertThrows<InvalidDataException>(
        () => RecvRaw(overlapping, BuildMassiveStart(1, 0x7778)),
        "Overlapping massive packet starts were accepted.");
}

static void VerifyHwidV2Proofs()
{
    const string hwid = "0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF";
    using var rsa = new RSACryptoServiceProvider(2048) { PersistKeyInCsp = false };
    using var client = new TcpClient();
    var session = new Session(client, null!);
    var challenge = HwidSecurity.CreateChallenge(session, HwidSecurity.GatewayRole);
    var response = CreateHwidResponse(challenge, hwid, rsa);

    Assert(HwidSecurity.TryValidateResponse(session, response, out var verifiedHwid) &&
           verifiedHwid == hwid && session.DeviceKeyThumbprint.Length == 64,
        "A valid HWID v2 proof was rejected.");
    Assert(!HwidSecurity.TryValidateResponse(session, response, out _),
        "A consumed HWID v2 challenge was replayed successfully.");

    var tamperedSession = new Session(new TcpClient(), null!);
    var tamperedChallenge = HwidSecurity.CreateChallenge(tamperedSession, HwidSecurity.AgentRole);
    var tamperedResponse = CreateHwidResponse(tamperedChallenge, hwid, rsa)
        .Replace(hwid, new string('F', 64), StringComparison.Ordinal);
    Assert(!HwidSecurity.TryValidateResponse(tamperedSession, tamperedResponse, out _),
        "A tampered HWID was accepted.");

    var expiredSession = new Session(new TcpClient(), null!);
    var expiredChallenge = HwidSecurity.CreateChallenge(expiredSession, HwidSecurity.GatewayRole);
    expiredSession.HwidChallengeExpiresAt = DateTime.UtcNow.AddSeconds(-1);
    Assert(!HwidSecurity.TryValidateResponse(
            expiredSession, CreateHwidResponse(expiredChallenge, hwid, rsa), out _),
        "An expired HWID v2 challenge was accepted.");

    var legacySession = new Session(new TcpClient(), null!);
    HwidSecurity.CreateChallenge(legacySession, HwidSecurity.GatewayRole);
    Assert(!HwidSecurity.TryValidateResponse(
            legacySession, hwid + "|LEGACY|" + new string('0', 64), out _),
        "A legacy HWID proof was accepted after the v2 cutover.");

    var pendingSession = new Session(new TcpClient(), null!);
    Assert(HwidSecurity.TryCreateChallenge(
            pendingSession, HwidSecurity.GatewayRole, out var pendingChallenge),
        "The first HWID v2 challenge was not created.");
    Assert(!HwidSecurity.TryCreateChallenge(
            pendingSession, HwidSecurity.GatewayRole, out _),
        "A second HWID v2 challenge replaced an unexpired pending challenge.");
    Assert(HwidSecurity.TryValidateResponse(
            pendingSession, CreateHwidResponse(pendingChallenge, hwid, rsa), out _),
        "Suppressing a duplicate challenge invalidated the pending proof.");
}

static string CreateHwidResponse(string challenge, string hwid, RSACryptoServiceProvider rsa)
{
    var fields = challenge.Split('|');
    var canonical = HwidSecurity.BuildCanonicalProof(
        fields[1], long.Parse(fields[2], System.Globalization.CultureInfo.InvariantCulture), fields[3], hwid);
    var hash = SHA256.HashData(System.Text.Encoding.ASCII.GetBytes(canonical));
    var signature = rsa.SignHash(hash, "2.16.840.1.101.3.4.2.1");
    return string.Join('|', HwidSecurity.ProtocolMarker, hwid,
        Convert.ToBase64String(rsa.ExportCspBlob(false)), Convert.ToBase64String(signature));
}

static byte[] BuildMassiveStart(ushort fragmentCount, ushort containedOpcode)
{
    var payload = new byte[5];
    payload[0] = 1;
    BinaryPrimitives.WriteUInt16LittleEndian(payload.AsSpan(1, 2), fragmentCount);
    BinaryPrimitives.WriteUInt16LittleEndian(payload.AsSpan(3, 2), containedOpcode);
    return BuildRawPacket(0x600D, payload);
}

static void RecvRaw(Security security, byte[] raw)
{
    security.Recv(raw, 0, raw.Length);
}

static byte[] BuildRawPacket(ushort opcode, params byte[] payload)
{
    var raw = new byte[6 + payload.Length];
    BinaryPrimitives.WriteUInt16LittleEndian(raw.AsSpan(0, 2), (ushort)payload.Length);
    BinaryPrimitives.WriteUInt16LittleEndian(raw.AsSpan(2, 2), opcode);
    payload.CopyTo(raw, 6);
    return raw;
}

static void AssertThrows<TException>(Action action, string message) where TException : Exception
{
    try
    {
        action();
    }
    catch (TException)
    {
        return;
    }

    throw new InvalidOperationException(message);
}

static async Task AssertThrowsAsync<TException>(Func<Task> action) where TException : Exception
{
    try
    {
        await action();
    }
    catch (TException)
    {
        return;
    }

    throw new InvalidOperationException($"Expected {typeof(TException).Name} was not thrown.");
}

static async Task VerifyInternalPacketAuthentication()
{
    const string secretHex =
        "000102030405060708090A0B0C0D0E0F" +
        "101112131415161718191A1B1C1D1E1F";
    const string sessionKey =
        "808182838485868788898A8B8C8D8E8F" +
        "909192939495969798999A9B9C9D9E9F";
    const uint gameId = 0x12345678;
    GameServerPacketAuthenticator.InitializeForTesting(secretHex);

    var packet = GameServerPacketAuthenticator.BuildRegistrationPacket(sessionKey, gameId);
    Assert(packet.Opcode == 0x35FE, "Internal packet registration opcode changed.");
    var reader = new Packet(packet);
    reader.ToReadOnly();
    var version = reader.ReadUInt8();
    var serializedGameId = reader.ReadUInt32();
    var issuedAt = reader.ReadInt64();
    var nonce = reader.ReadUInt8Array(16);
    var sessionKeyBytes = reader.ReadUInt8Array(32);
    var mac = reader.ReadUInt8Array(32);

    Assert(version == 2 && serializedGameId == gameId,
        "Internal packet registration identity was serialized incorrectly.");
    Assert(Math.Abs(DateTimeOffset.UtcNow.ToUnixTimeSeconds() - issuedAt) <= 5,
        "Internal packet registration timestamp is outside the expected window.");

    var signedBytes = new byte[61];
    signedBytes[0] = version;
    BinaryPrimitives.WriteUInt32LittleEndian(signedBytes.AsSpan(1, 4), serializedGameId);
    BinaryPrimitives.WriteInt64LittleEndian(signedBytes.AsSpan(5, 8), issuedAt);
    nonce.CopyTo(signedBytes, 13);
    sessionKeyBytes.CopyTo(signedBytes, 29);
    using var hmac = new HMACSHA256(Convert.FromHexString(secretHex));
    Assert(CryptographicOperations.FixedTimeEquals(mac, hmac.ComputeHash(signedBytes)),
        "Internal packet registration HMAC is invalid.");

    var handler = new PacketHandler(
        new HashSet<ushort> { 0x35FE },
        new HashSet<ushort> { 0x35FE });
    var result = await handler.HandleClient(new Packet(0x35FE), null!);
    Assert(result.PacketResultType == PacketResultType.Disconnect,
        "A client-originated internal packet registration was not disconnected.");
}

static void VerifySilkStallContracts()
{
    Assert(StallPackets.IsStallSuccess(0x01), "Stall success result was rejected.");
    Assert(!StallPackets.IsStallSuccess(0x00) && !StallPackets.IsStallSuccess(0x02),
        "A failed stall response was treated as successful.");
    Assert(SilkStallService.IsValidSlotDefinition(0, 1, 1),
        "Silk Stall rejected the minimum valid slot definition.");
    Assert(SilkStallService.IsValidSlotDefinition(9, ushort.MaxValue, int.MaxValue),
        "Silk Stall rejected the maximum valid slot definition.");
    Assert(!SilkStallService.IsValidSlotDefinition(10, 1, 1),
        "Silk Stall accepted an out-of-range stall slot.");
    Assert(!SilkStallService.IsValidSlotDefinition(0, 0, 1),
        "Silk Stall accepted a zero quantity.");
    Assert(!SilkStallService.IsValidSlotDefinition(0, 1, 0) &&
           !SilkStallService.IsValidSlotDefinition(0, 1, (ulong)int.MaxValue + 1),
        "Silk Stall accepted an unsafe Silk price.");

    Assert(SilkStallService.CanCreditSilkBalance(int.MaxValue - 100, 100),
        "Silk Stall rejected a non-overflowing seller credit.");
    Assert(!SilkStallService.CanCreditSilkBalance(int.MaxValue - 99, 100),
        "Silk Stall accepted an overflowing seller credit.");

    var pending = new PendingSilkStallPurchase
    {
        TransactionId = 987654321,
        SellerUniqueId = 123456,
        StallSlot = 7,
        SilkAmount = 2500
    };
    var packet = SilkStallService.BuildGameServerPreparationPacket(
        new string('A', 64), pending);
    Assert(packet.Opcode == SilkStallService.GameServerPrepareBuyOpcode,
        "Silk Stall GameServer preparation opcode changed.");
    var reader = new Packet(packet);
    reader.ToReadOnly();
    Assert(reader.ReadAscii() == new string('A', 64),
        "Silk Stall preparation session key was serialized incorrectly.");
    Assert(reader.ReadInt64() == pending.TransactionId,
        "Silk Stall preparation transaction ID was serialized incorrectly.");
    Assert(reader.ReadUInt32() == pending.SellerUniqueId &&
           reader.ReadUInt8() == pending.StallSlot &&
           reader.ReadInt32() == pending.SilkAmount,
        "Silk Stall preparation payload was serialized incorrectly.");

    Assert((byte)SilkStallTransactionStatus.Reserved == 0 &&
           (byte)SilkStallTransactionStatus.Completed == 1 &&
           (byte)SilkStallTransactionStatus.Refunded == 2 &&
           (byte)SilkStallTransactionStatus.SaleAccepted == 4 &&
           (byte)SilkStallTransactionStatus.RefundPending == 5,
        "Silk Stall durable status contract changed.");
}

static void VerifyOfflineStallContracts()
{
    var sessionId = Guid.NewGuid();
    var silkSlot = new SilkStallSlot
    {
        StallSlot = 4,
        InventorySlot = 12,
        Quantity = 3,
        SilkPrice = 500,
        Tid = 1234
    };
    ActionManager.QueueStallAction(
        sessionId,
        new PendingStallAction(0x02, 4, false, silkSlot));
    ActionManager.QueueStallAction(
        sessionId,
        new PendingStallAction(0x03, 4, true, null));

    Assert(ActionManager.TryTakeStallAction(sessionId, 0x02, out var addAction) &&
           addAction is { StallSlot: 4 } && ReferenceEquals(addAction.SilkSlot, silkSlot),
        "Offline Stall did not preserve the first pending slot action.");
    Assert(ActionManager.TryTakeStallAction(sessionId, 0x03, out var removeAction) &&
           removeAction is { PreviousOperating: true },
        "Offline Stall pending actions were not completed in FIFO order.");
    Assert(!ActionManager.TryTakeStallAction(sessionId, 0x03, out _),
        "Offline Stall retained a completed pending action queue.");

    ActionManager.QueueStallAction(
        sessionId,
        new PendingStallAction(0x02, 5, false, null));
    Assert(!ActionManager.TryTakeStallAction(sessionId, 0x03, out var mismatchedAction) &&
           mismatchedAction is { ActionType: 0x02, StallSlot: 5 },
        "Offline Stall did not detect a mismatched GameServer action response.");
    Assert(!ActionManager.TryTakeStallAction(sessionId, 0x02, out _),
        "Offline Stall retained an unsafe queue after an action-response mismatch.");

    const uint uniqueId = 991122;
    ActionManager.OpenStall(uniqueId, 1, 2, "OfflineSlotTest");
    ActionManager.TrackStallSlot(uniqueId, 7);
    Assert(ActionManager.OpenStalls[uniqueId].Slots.ContainsKey(7),
        "Offline Stall did not track a confirmed slot.");
    Assert(ActionManager.RemoveStallSlot(uniqueId, 7) == 0,
        "Offline Stall did not remove a confirmed sold slot.");
    ActionManager.CloseStall(uniqueId);

    using var client = new TcpClient();
    var detached = new Session(client, null!)
    {
        ClientGuid = Guid.NewGuid(),
        CharacterGameReady = true
    };
    detached.SessionData.Charid = 700;
    detached.SessionData.UniqueCharId = 701;
    detached.SessionData.Charname = "DetachedCapacityTest";
    detached.SessionData.OfflineStall = true;
    typeof(Session)
        .GetField("_clientDetached", System.Reflection.BindingFlags.Instance | System.Reflection.BindingFlags.NonPublic)!
        .SetValue(detached, 1);
    Assert(!ServerManager.IsOnlinePlayer(detached),
        "A detached Offline Stall was exposed as a client-delivery target.");
    Assert(ServerManager.IsUpstreamOccupiedPlayer(detached),
        "A detached Offline Stall was omitted from upstream capacity accounting.");
}

static void VerifySessionRegistryIndexes()
{
    using var client = new TcpClient();
    var session = new Session(client, null!)
    {
        ClientGuid = Guid.NewGuid(),
        CharacterGameReady = true
    };
    session.SessionData.Charid = 101;
    session.SessionData.UniqueCharId = 1001;
    session.SessionData.Charname = "IndexPlayer";

    var sessions = new ConcurrentSessionSet();
    Assert(sessions.Add(session), "Session registry rejected a new session.");
    Assert(ReferenceEquals(sessions.FindByCharId(101), session), "CharID index did not resolve the session.");
    Assert(ReferenceEquals(sessions.FindByUniqueCharId(1001), session), "UniqueID index did not resolve the session.");
    Assert(ReferenceEquals(sessions.FindByCharName("indexplayer"), session), "CharName index was not case-insensitive.");

    session.SessionData.Charid = 202;
    session.SessionData.UniqueCharId = 2002;
    session.SessionData.Charname = "RenamedPlayer";

    Assert(sessions.FindByCharId(101) == null, "A stale CharID index returned the changed session.");
    Assert(sessions.FindByUniqueCharId(1001) == null, "A stale UniqueID index returned the changed session.");
    Assert(sessions.FindByCharName("IndexPlayer") == null, "A stale CharName index returned the changed session.");
    Assert(ReferenceEquals(sessions.FindByCharId(202), session), "CharID fallback did not repair the index.");
    Assert(ReferenceEquals(sessions.FindByUniqueCharId(2002), session), "UniqueID fallback did not repair the index.");
    Assert(ReferenceEquals(sessions.FindByCharName("renamedplayer"), session), "CharName fallback did not repair the index.");
    Assert(sessions.CountWhere(candidate => candidate.CharacterGameReady) == 1,
        "Allocation-free session counting returned the wrong result.");

    Assert(sessions.Remove(session), "Session registry failed to remove the session.");
    Assert(sessions.FindByCharId(202) == null && sessions.FindByUniqueCharId(2002) == null,
        "Session indexes survived after session removal.");
}

static async Task VerifyDelayedJobScheduling()
{
    using var manager = new DelayedJobManager();
    var lateJob = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
    var earlyJob = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);

    manager.Run();
    manager.CreateJob(new DelayedJobItem(
        1_000,
        new object(),
        null,
        (_, _) =>
        {
            lateJob.TrySetResult();
            return Task.CompletedTask;
        }));

    await Task.Delay(25);
    var startedAt = DateTime.UtcNow;
    manager.CreateJob(new DelayedJobItem(
        25,
        new object(),
        null,
        (_, _) =>
        {
            earlyJob.TrySetResult();
            return Task.CompletedTask;
        }));

    await earlyJob.Task.WaitAsync(TimeSpan.FromSeconds(1));
    Assert(DateTime.UtcNow - startedAt < TimeSpan.FromMilliseconds(500),
        "A newly-added earlier delayed job did not wake the scheduler promptly.");
    Assert(!lateJob.Task.IsCompleted, "A delayed job ran before its due time.");
}

static PacketHandler CreateHandler(params ushort[] allowedOpcodes)
{
    return new PacketHandler(
        new HashSet<ushort>(allowedOpcodes),
        new HashSet<ushort>());
}

static void VerifyClientlessPartyFormPacket()
{
    var originalServerMaxLevel = _serverSettings.ServerMaxLevel;
    try
    {
        Assert(ClientlessManager.NormalizePartyMode(null) == ClientlessManager.PartyModeGroupsOf8,
            "A missing persisted Clientless party mode did not retain the grouped compatibility default.");
        Assert(ClientlessManager.NormalizePartyMode("soloforms") == ClientlessManager.PartyModeSoloForms,
            "The individual Party Form mode is not normalized case-insensitively.");
        var invalidModeRejected = false;
        try
        {
            ClientlessManager.NormalizePartyMode("Unsupported");
        }
        catch (ArgumentException)
        {
            invalidModeRejected = true;
        }
        Assert(invalidModeRejected, "An unsupported Clientless party mode was accepted silently.");

        _serverSettings.ServerMaxLevel = 125;
        var packet = ClientlessSession.BuildPartyFormPacket(
            new ClientlessManager.PartyFormPolicy(
                true,
                "{CharacterName}",
                20,
                140,
                2,
                7,
                ClientlessManager.PartyModeSoloForms),
            "KmtBot");
        Assert(packet.Opcode == 0x7069, "Clientless Party Form opcode is incorrect.");

        var reader = new Packet(packet);
        reader.ToReadOnly();
        Assert(reader.ReadUInt32() == 0, "Clientless Party Form matching ID must start at zero.");
        Assert(reader.ReadUInt32() == 0, "Clientless Party Form party ID must start at zero.");
        Assert(reader.ReadUInt8() == 7, "Clientless Party Form sharing settings were serialized incorrectly.");
        Assert(reader.ReadUInt8() == 2, "Clientless Party Form purpose was serialized incorrectly.");
        Assert(reader.ReadUInt8() == 20, "Clientless Party Form minimum level was serialized incorrectly.");
        Assert(reader.ReadUInt8() == 125, "Clientless Party Form maximum level did not respect the configured server cap.");
        Assert(reader.ReadAscii() == "KmtBot", "Clientless Party Form title placeholder was not resolved.");

        const uint managedPartyId = 0x55667788;
        var groupedForm = new Packet(ClientlessSession.BuildPartyFormPacket(
            new ClientlessManager.PartyFormPolicy(true, "{CharacterName}", 20, 120, 2, 7),
            "KmtBot",
            managedPartyId));
        groupedForm.ToReadOnly();
        groupedForm.ReadUInt32();
        Assert(groupedForm.ReadUInt32() == managedPartyId,
            "A real managed party form was not linked to its GameServer party ID.");

        const uint memberUniqueId = 0x10203040;
        var createInvite = new Packet(ClientlessSession.BuildManagedPartyInvitePacket(memberUniqueId, true, 3));
        createInvite.ToReadOnly();
        Assert(createInvite.Opcode == 0x7060 && createInvite.ReadUInt32() == memberUniqueId &&
               createInvite.ReadUInt8() == 7 && createInvite.RemainingRead() == 0,
            "The first managed vSRO party invitation is malformed or does not enable eight-member parties.");

        var memberInvite = new Packet(ClientlessSession.BuildManagedPartyInvitePacket(memberUniqueId, false, 7));
        memberInvite.ToReadOnly();
        Assert(memberInvite.Opcode == 0x7062 && memberInvite.ReadUInt32() == memberUniqueId &&
               memberInvite.RemainingRead() == 0,
            "A managed invitation for an existing party is malformed.");

        var accept = new Packet(ClientlessSession.BuildManagedPartyAcceptPacket());
        accept.ToReadOnly();
        Assert(accept.Opcode == 0x3080 && accept.ReadUInt8() == 1 && accept.ReadUInt8() == 1 &&
               accept.RemainingRead() == 0,
            "The managed Clientless party acceptance packet is malformed.");

        var partyCandidates = Enumerable.Range(1, 10)
            .Select(index => (City: "Jangan", Account: index))
            .Concat(Enumerable.Range(1, 9).Select(index => (City: "SamarKand", Account: index)))
            .ToArray();
        var cityGroups = ClientlessManager.BuildManagedPartyGroups(
            partyCandidates,
            candidate => candidate.City);
        Assert(cityGroups.Select(group => group.Length).SequenceEqual(new[] { 8, 2, 8, 1 }),
            "Managed Clientless accounts were not chunked into real parties of eight per city.");
        Assert(cityGroups.All(group => group.Select(member => member.City).Distinct(StringComparer.OrdinalIgnoreCase).Count() == 1),
            "A managed Clientless party mixed characters from different cities.");

        var firstParkingPoint = ClientlessTownPositions.Resolve("SamarKand", 1);
        var secondParkingPoint = ClientlessTownPositions.Resolve("SamarKand", 2);
        Assert(firstParkingPoint.RegionID != secondParkingPoint.RegionID ||
               Math.Abs(firstParkingPoint.X - secondParkingPoint.X) > 0.1f ||
               Math.Abs(firstParkingPoint.Z - secondParkingPoint.Z) > 0.1f,
            "Two Clientless accounts still resolve to the same town parking point.");
    }
    finally
    {
        _serverSettings.ServerMaxLevel = originalServerMaxLevel;
    }
}

static void VerifyClientlessHuntPackets()
{
    const uint targetId = 0x12345678;
    const uint skillId = 0x10203040;

    var select = new Packet(ClientlessHuntProtocol.BuildSelectTarget(targetId));
    select.ToReadOnly();
    Assert(select.Opcode == 0x7045 && select.ReadUInt32() == targetId && select.RemainingRead() == 0,
        "Clientless hunting target-selection packet is incorrect.");

    var basic = new Packet(ClientlessHuntProtocol.BuildBasicAttack(targetId));
    basic.ToReadOnly();
    Assert(basic.Opcode == 0x7074 && basic.ReadUInt8() == 1 && basic.ReadUInt8() == 1 &&
           basic.ReadUInt8() == 1 && basic.ReadUInt32() == targetId && basic.RemainingRead() == 0,
        "Clientless hunting basic-attack packet is incorrect.");

    var skill = new Packet(ClientlessHuntProtocol.BuildSkillAttack(skillId, targetId));
    skill.ToReadOnly();
    Assert(skill.Opcode == 0x7074 && skill.ReadUInt8() == 1 && skill.ReadUInt8() == 4 &&
           skill.ReadUInt32() == skillId && skill.ReadUInt8() == 1 &&
           skill.ReadUInt32() == targetId && skill.RemainingRead() == 0,
        "Clientless hunting skill-attack packet is incorrect.");

    var buff = new Packet(ClientlessHuntProtocol.BuildSelfBuff(skillId, targetId, targetsSelf: false));
    buff.ToReadOnly();
    Assert(buff.Opcode == 0x7074 && buff.ReadUInt8() == 1 && buff.ReadUInt8() == 4 &&
           buff.ReadUInt32() == skillId && buff.ReadUInt8() == 0 && buff.RemainingRead() == 0,
        "Clientless hunting self-buff packet is incorrect.");

    var targetedBuff = new Packet(ClientlessHuntProtocol.BuildSelfBuff(skillId, targetId, targetsSelf: true));
    targetedBuff.ToReadOnly();
    Assert(targetedBuff.Opcode == 0x7074 && targetedBuff.ReadUInt8() == 1 && targetedBuff.ReadUInt8() == 4 &&
           targetedBuff.ReadUInt32() == skillId && targetedBuff.ReadUInt8() == 1 &&
           targetedBuff.ReadUInt32() == targetId && targetedBuff.RemainingRead() == 0,
        "Clientless hunting self-target buff packet is incorrect.");

    var potion = new Packet(ClientlessHuntProtocol.BuildUseItem(13, 0x499F));
    potion.ToReadOnly();
    Assert(potion.Opcode == 0x704C && potion.Encrypted && potion.ReadUInt8() == 13 &&
           potion.ReadUInt16() == 0x499F && potion.RemainingRead() == 0,
        "Clientless hunting encrypted vSRO item-use packet is incorrect.");

    var targetedPetPotion = new Packet(ClientlessHuntProtocol.BuildUseItemFor(18, 0x218C, targetId));
    targetedPetPotion.ToReadOnly();
    Assert(targetedPetPotion.Opcode == 0x704C && targetedPetPotion.Encrypted &&
           targetedPetPotion.ReadUInt8() == 18 && targetedPetPotion.ReadUInt16() == 0x218C &&
           targetedPetPotion.ReadUInt32() == targetId && targetedPetPotion.RemainingRead() == 0,
        "Clientless attack-pet health/hunger item packet is incorrect.");

    var revivePet = new Packet(ClientlessHuntProtocol.BuildUseItemForSlot(19, 0x318C, 16));
    revivePet.ToReadOnly();
    Assert(revivePet.Opcode == 0x704C && revivePet.Encrypted &&
           revivePet.ReadUInt8() == 19 && revivePet.ReadUInt16() == 0x318C &&
           revivePet.ReadUInt8() == 16 && revivePet.RemainingRead() == 0,
        "Clientless attack-pet revival packet is incorrect.");

    var petAttack = new Packet(ClientlessHuntProtocol.BuildCosAttack(0x01020304, targetId));
    petAttack.ToReadOnly();
    Assert(petAttack.Opcode == 0x70C5 && petAttack.ReadUInt32() == 0x01020304 &&
           petAttack.ReadUInt8() == 2 && petAttack.ReadUInt32() == targetId &&
           petAttack.RemainingRead() == 0,
        "Clientless attack-pet target command is incorrect.");

    var reactionSamples = Enumerable.Range(1, 64)
        .Select(index => ClientlessHuntEngine.CalculateTargetReactionMilliseconds(index, (uint)(targetId + index)))
        .ToArray();
    Assert(reactionSamples.All(value => value is >= 80 and <= 200) && reactionSamples.Distinct().Count() > 8,
        "Clientless post-kill reaction timing is not bounded or naturally varied.");

    var normalMove = new Packet(ClientlessHuntProtocol.BuildMove(25000, 101, 202, 303));
    normalMove.ToReadOnly();
    Assert(normalMove.Opcode == 0x7021 && normalMove.ReadUInt8() == 1 &&
           normalMove.ReadUInt16() == 25000 && normalMove.ReadInt16() == 101 &&
           normalMove.ReadInt16() == 303 && normalMove.ReadInt16() == 202 &&
           normalMove.RemainingRead() == 0,
        "Clientless hunting field-movement packet is incorrect.");

    var dungeonMove = new Packet(ClientlessHuntProtocol.BuildMove(0x8001, 100001, 200002, 300003));
    dungeonMove.ToReadOnly();
    Assert(dungeonMove.ReadUInt8() == 1 && dungeonMove.ReadUInt16() == 0x8001 &&
           dungeonMove.ReadInt32() == 100001 && dungeonMove.ReadInt32() == 300003 &&
           dungeonMove.ReadInt32() == 200002 && dungeonMove.RemainingRead() == 0,
        "Clientless hunting dungeon-movement packet is incorrect.");

    var movementSource = new Packet(0xB021);
    movementSource.WriteUInt16(25000);
    movementSource.WriteInt16(7000);
    movementSource.WriteSingle(557f);
    movementSource.WriteInt16(1100);
    movementSource.ToReadOnly();
    var parsedSource = ClientlessHuntEngine.ReadMovementSource(movementSource);
    Assert(parsedSource.Region == 25000 &&
           Math.Abs(parsedSource.X - 700f) < 0.01f &&
           Math.Abs(parsedSource.Y - 110f) < 0.01f &&
           Math.Abs(parsedSource.Z - 557f) < 0.01f,
        "Clientless B021 source parsing swapped vSRO height and horizontal Z coordinates.");

    var respawn = new Packet(ClientlessHuntProtocol.BuildRespawnInTown());
    respawn.ToReadOnly();
    Assert(respawn.Opcode == 0x3053 && respawn.ReadUInt8() == 1 && respawn.RemainingRead() == 0,
        "Clientless hunting town-respawn packet is incorrect.");

    var acceptedSkill = new Packet(0xB070);
    acceptedSkill.WriteUInt8(1);
    acceptedSkill.WriteUInt8(2);
    acceptedSkill.WriteUInt8(0x30);
    acceptedSkill.WriteUInt32(skillId);
    acceptedSkill.WriteUInt32(targetId);
    acceptedSkill.WriteUInt32(0xAABBCCDD);
    acceptedSkill.WriteUInt32(0x01020304);
    acceptedSkill.WriteUInt8(0);
    Assert(ClientlessHuntProtocol.TryReadSkillCastFeedback(
               acceptedSkill,
               targetId,
               out var acceptedFeedback) &&
           acceptedFeedback.Accepted && acceptedFeedback.OwnAction &&
           acceptedFeedback.SkillId == skillId && acceptedFeedback.ActionId == 0xAABBCCDD,
        "Clientless hunting did not parse a successful vSRO B070 skill response.");

    var rejectedSkill = new Packet(0xB070);
    rejectedSkill.WriteUInt8(2);
    rejectedSkill.WriteUInt8(0x05);
    rejectedSkill.WriteUInt8(0x30);
    Assert(ClientlessHuntProtocol.TryReadSkillCastFeedback(
               rejectedSkill,
               targetId,
               out var rejectedFeedback) &&
           !rejectedFeedback.Accepted && rejectedFeedback.OwnAction &&
           rejectedFeedback.ErrorCode == 0x05,
        "Clientless hunting did not keep the one-byte vSRO B070 error separate from its 0x30 extension.");

    var buffApplied = new Packet(0xB0BD);
    buffApplied.WriteUInt32(targetId);
    buffApplied.WriteUInt32(skillId);
    buffApplied.WriteUInt32(0x55667788);
    Assert(ClientlessHuntProtocol.TryReadBuffApplied(buffApplied, out var appliedFeedback) &&
           appliedFeedback.TargetId == targetId && appliedFeedback.SkillId == skillId &&
           appliedFeedback.Token == 0x55667788,
        "Clientless hunting did not parse the vSRO B0BD active-buff record.");

    var buffRemoved = new Packet(0xB072);
    buffRemoved.WriteUInt8(2);
    buffRemoved.WriteUInt32(0x55667788);
    buffRemoved.WriteUInt32(0x99AABBCC);
    Assert(ClientlessHuntProtocol.TryReadRemovedBuffTokens(buffRemoved, out var removedTokens) &&
           removedTokens.SequenceEqual(new uint[] { 0x55667788, 0x99AABBCC }),
        "Clientless hunting did not parse the vSRO B072 removed-buff token list.");

}

static void VerifyClientlessTeleportProtocol()
{
    var protocol = new ClientlessTeleportProtocol();

    Assert(ClientlessTeleportProtocol.ServerTeleportReadyRequest == 0x34B5,
        "Clientless teleport-ready request must match the RSBot/vSRO game-reset opcode.");
    Assert(ClientlessTeleportProtocol.ClientTeleportReadyResponse == 0x34B6,
        "Clientless teleport-ready response must match the RSBot/vSRO game-reset-complete opcode.");
    Assert(ClientlessTeleportProtocol.AlternateServerTeleportReadyRequest == 0x35B5 &&
           ClientlessTeleportProtocol.AlternateClientTeleportReadyResponse == 0x35B6,
        "Clientless teleport compatibility must retain the alternate game-reset pair.");
    Assert(protocol.HandleServerOpcode(ClientlessTeleportProtocol.ServerCharacterDataEnd) == null,
        "Clientless sent a duplicate initial character confirmation outside a teleport cycle.");
    Assert(protocol.HandleServerOpcode(ClientlessTeleportProtocol.ServerTeleportReadyRequest) ==
           ClientlessTeleportProtocol.ClientTeleportReadyResponse,
        "Clientless did not acknowledge the server teleport-ready request.");
    Assert(protocol.HandleServerOpcode(ClientlessTeleportProtocol.ServerCharacterDataBegin) == null,
        "Clientless responded unexpectedly at the start of teleported character data.");
    Assert(protocol.HandleServerOpcode(ClientlessTeleportProtocol.ServerCharacterData) == null,
        "Clientless responded unexpectedly while teleported character data was loading.");
    Assert(protocol.HandleServerOpcode(ClientlessTeleportProtocol.ServerCharacterDataEnd) ==
           ClientlessTeleportProtocol.ClientCharacterConfirmSpawn,
        "Clientless did not confirm spawn after teleported character data completed.");
    Assert(protocol.HandleServerOpcode(ClientlessTeleportProtocol.ServerCharacterDataEnd) == null,
        "Clientless confirmed the same teleported character data more than once.");

    Assert(protocol.HandleServerOpcode(ClientlessTeleportProtocol.AlternateServerTeleportReadyRequest) ==
           ClientlessTeleportProtocol.AlternateClientTeleportReadyResponse,
        "Clientless did not acknowledge the alternate teleport-ready request.");
    protocol.Reset();
    Assert(protocol.HandleServerOpcode(ClientlessTeleportProtocol.ServerCharacterDataEnd) == null,
        "Clientless retained teleport state across a connection reset.");
}

static void VerifyHideAndSeekVisibilityCommand()
{
    var packet = HideAndSeekEventService.CreateInvisibleTogglePacket();
    Assert(packet.Opcode == HideAndSeekEventService.OperatorCommandOpcode,
        "Hide and Seek visibility command used the wrong Agent opcode.");
    Assert(packet.Encrypted,
        "Hide and Seek visibility command must use the encrypted Agent request channel.");
    Assert(!packet.Massive,
        "Hide and Seek visibility command must not be a massive packet.");

    var reader = new Packet(packet);
    reader.ToReadOnly();
    Assert(reader.ReadUInt16() == HideAndSeekEventService.InvisibleOperatorCommand,
        "Hide and Seek visibility command used the wrong GM operator command.");
    Assert(reader.GetBytes().Length == sizeof(ushort),
        "Hide and Seek visibility command contains an unexpected payload.");
}

static void VerifyUniqueHistoryPacketContract()
{
    var damage = Enumerable.Range(1, 12)
        .Select(i => new KeyValuePair<string, int>($"Player{i:00}", i * 1_000))
        .ToArray();
    const int runtimeLayerId = 27;
    const int worldId = (1 << 16) | runtimeLayerId;
    var history = UniqueHistoryService.CreateKilled(
        1954,
        "Killer",
        1_785_500_000,
        0x5C87,
        1234.75f,
        20.5f,
        987.25f,
        worldId,
        damage);

    var packet = UniqueHistoryService.CreateUpdatePacket(history);
    Assert(packet.Opcode == 0x208A, "Unique History update opcode is incorrect.");

    var reader = new Packet(packet);
    reader.ToReadOnly();
    Assert(reader.ReadInt32() == 1954, "Unique History ID was serialized incorrectly.");
    Assert(reader.ReadUnicode() == "Killer", "Unique History killer was serialized incorrectly.");
    Assert(reader.ReadUInt8() == 0, "Unique History state was serialized incorrectly.");
    Assert(reader.ReadInt64() == 1_785_500_000, "Unique History timestamp was serialized incorrectly.");
    Assert(reader.ReadInt32() == 0x5C87, "Unique History region was serialized incorrectly.");
    Assert(Math.Abs(reader.ReadFloat() - 1234.75f) < 0.001f, "Unique History X precision was lost.");
    Assert(Math.Abs(reader.ReadFloat() - 20.5f) < 0.001f, "Unique History Y precision was lost.");
    Assert(Math.Abs(reader.ReadFloat() - 987.25f) < 0.001f, "Unique History Z precision was lost.");
    Assert(reader.ReadInt32() == worldId, "Unique History world was serialized incorrectly.");
    Assert(reader.ReadUInt8() == 0, "Unique History map type was serialized incorrectly.");
    Assert(reader.ReadInt32() == 1, "Unique History map index was serialized incorrectly.");

    var count = reader.ReadUInt8();
    Assert(count == UniqueHistoryService.MaximumDpsEntries, "Unique History DPS count did not match its payload.");
    var previousDamage = int.MaxValue;
    for (var i = 0; i < count; i++)
    {
        var player = reader.ReadAscii();
        var formattedDamage = reader.ReadAscii();
        var playerNumber = int.Parse(player[6..]);
        var rawDamage = playerNumber * 1_000;
        Assert(rawDamage <= previousDamage, "Unique History DPS order was not descending.");
        Assert(!string.IsNullOrWhiteSpace(formattedDamage), "Unique History DPS value was empty.");
        previousDamage = rawDamage;
    }
}

static async Task VerifyClientBlockStopsPipeline()
{
    var pipeline = CreateHandler(BlockOpcode);
    int laterHandlerCalls = 0;

    pipeline.RegisterClientHandler(
        BlockOpcode,
        1,
        (_, _, _) => Task.FromResult(new PacketResult(PacketResultType.Block)));
    pipeline.RegisterClientHandler(
        BlockOpcode,
        2,
        (_, _, _) =>
        {
            laterHandlerCalls++;
            return Task.FromResult(new PacketResult());
        });

    var result = await pipeline.HandleClient(new Packet(BlockOpcode), null!);

    Assert(result.PacketResultType == PacketResultType.Block, "Client Block was not final.");
    Assert(laterHandlerCalls == 0, "A handler ran after Client Block.");
}

static async Task VerifyClientDisconnectStopsPipeline()
{
    var pipeline = CreateHandler(DisconnectOpcode);
    int laterHandlerCalls = 0;

    pipeline.RegisterClientHandler(
        DisconnectOpcode,
        1,
        (_, _, _) => Task.FromResult(new PacketResult(PacketResultType.Disconnect)));
    pipeline.RegisterClientHandler(
        DisconnectOpcode,
        2,
        (_, _, _) =>
        {
            laterHandlerCalls++;
            return Task.FromResult(new PacketResult());
        });

    var result = await pipeline.HandleClient(new Packet(DisconnectOpcode), null!);

    Assert(result.PacketResultType == PacketResultType.Disconnect, "Client Disconnect was not final.");
    Assert(laterHandlerCalls == 0, "A handler ran after Client Disconnect.");
}

static async Task VerifyOverrideFlowsThroughAndSurvives()
{
    var pipeline = CreateHandler(OverrideOpcode);
    bool nextHandlerSawOverride = false;

    pipeline.RegisterClientHandler(
        OverrideOpcode,
        1,
        (_, _, _) =>
        {
            var replacement = new Packet(OverrideOpcode);
            replacement.WriteUInt8(0x2A);
            return Task.FromResult(new PacketResult(replacement, PacketResultType.Override));
        });
    pipeline.RegisterClientHandler(
        OverrideOpcode,
        2,
        (packet, _, _) =>
        {
            nextHandlerSawOverride = packet.ReadUInt8() == 0x2A;
            return Task.FromResult(new PacketResult());
        });

    var result = await pipeline.HandleClient(new Packet(OverrideOpcode), null!);

    Assert(nextHandlerSawOverride, "The next handler did not receive the overridden packet.");
    Assert(result.PacketResultType == PacketResultType.Override, "Override was lost after a Nothing result.");
    Assert(result.OverridePacket != null, "Final Override packet is missing.");
}

static async Task VerifyPacketDataTracksPreviousHandler()
{
    var pipeline = CreateHandler(DataOpcode);
    bool dataWasPreserved = false;

    pipeline.RegisterClientHandler(
        DataOpcode,
        4,
        (_, _, _) => Task.FromResult(new PacketResult("marker")));
    pipeline.RegisterClientHandler(
        DataOpcode,
        9,
        (_, _, data) =>
        {
            dataWasPreserved =
                Equals(data.Data, "marker") &&
                data.PreviousPriority == 4 &&
                data.PreviousResult == PacketResultType.Nothing;
            return Task.FromResult(new PacketResult());
        });

    await pipeline.HandleClient(new Packet(DataOpcode), null!);

    Assert(dataWasPreserved, "PacketData did not preserve previous handler metadata.");
}

static async Task VerifyServerBlockStopsPipeline()
{
    var pipeline = CreateHandler();
    int laterHandlerCalls = 0;

    pipeline.RegisterModuleHandler(
        ServerOpcode,
        1,
        (_, _, _) => Task.FromResult(new PacketResult(PacketResultType.Block)));
    pipeline.RegisterModuleHandler(
        ServerOpcode,
        2,
        (_, _, _) =>
        {
            laterHandlerCalls++;
            return Task.FromResult(new PacketResult());
        });

    var result = await pipeline.HandleServer(new Packet(ServerOpcode), null!);

    Assert(result.PacketResultType == PacketResultType.Block, "Server Block was not final.");
    Assert(laterHandlerCalls == 0, "A handler ran after Server Block.");
}

static void VerifyQueuedServerPacketSurvivesHandshake()
{
    const ushort shardListOpcode = 0xA101;
    var proxyFacingClient = new Security();
    var gameClient = new Security();
    var pendingPackets = new ClientHandshakePacketQueue();
    var trace = new List<string>();

    proxyFacingClient.GenerateSecurity(true, true, true);

    var shardList = new Packet(shardListOpcode);
    shardList.WriteUInt8(0x5A);
    Assert(
        pendingPackets.QueueOrReady(shardList) == ClientPacketQueueResult.Queued,
        "The pre-handshake shard list was not queued.");

    PumpSecurity(proxyFacingClient, gameClient, trace, "proxy->client");
    var clientPackets = new List<Packet>();
    DrainSecurity(gameClient, clientPackets);
    Assert(clientPackets.All(packet => packet.Opcode != shardListOpcode),
        "The shard list was released before the client handshake completed.");

    for (var round = 0; round < 8; round++)
    {
        PumpSecurity(gameClient, proxyFacingClient, trace, "client->proxy");
        DrainSecurity(proxyFacingClient);
        PumpSecurity(proxyFacingClient, gameClient, trace, "proxy->client");
        DrainSecurity(gameClient, clientPackets);
    }

    Assert(pendingPackets.BeginFlush(), "The handshake queue did not enter flushing state.");
    foreach (var packet in pendingPackets.TakePendingBatchOrComplete())
        proxyFacingClient.Send(packet);
    Assert(
        pendingPackets.TakePendingBatchOrComplete().Length == 0,
        "The handshake queue did not complete after its pending packets were drained.");

    PumpSecurity(proxyFacingClient, gameClient, trace, "proxy->client");
    DrainSecurity(gameClient, clientPackets);

    var shardPacket = clientPackets.SingleOrDefault(packet => packet.Opcode == shardListOpcode);
    Assert(shardPacket != null,
        $"A shard-list packet queued during handshake was lost. Trace: {string.Join(", ", trace)}");
    Assert(shardPacket!.ReadUInt8() == 0x5A, "The queued shard-list packet payload was corrupted.");

    var postHandshake = new Packet(0xA102);
    Assert(
        pendingPackets.QueueOrReady(postHandshake) == ClientPacketQueueResult.Ready,
        "A post-handshake packet was incorrectly queued.");
}

static void VerifyConfiguredShardStatusIsDeterministic()
{
    Assert(
        KMTGuard.Server.GatewayPacketHandler.SERVER_DLL_SETTINGS_RESPONSE
            .GetConfiguredShardStatus(checkStatus: false) == 1,
        "Normal shard presentation was allowed to fall back to Check.");
    Assert(
        KMTGuard.Server.GatewayPacketHandler.SERVER_DLL_SETTINGS_RESPONSE
            .GetConfiguredShardStatus(checkStatus: true) == 0,
        "The configured Check maintenance state was not preserved.");
}

static void VerifyGmIpMaintenanceBypass()
{
    Assert(
        KMTGuard.Server.GatewayPacketHandler.SERVER_DLL_SETTINGS_RESPONSE
            .ShouldPresentShardAsCheck(checkStatus: true, isGmIp: false),
        "A normal player bypassed the configured Check maintenance state.");
    Assert(
        !KMTGuard.Server.GatewayPacketHandler.SERVER_DLL_SETTINGS_RESPONSE
            .ShouldPresentShardAsCheck(checkStatus: true, isGmIp: true),
        "A whitelisted GM IP did not bypass the configured Check maintenance state.");
    Assert(
        !KMTGuard.Server.GatewayPacketHandler.SERVER_DLL_SETTINGS_RESPONSE
            .ShouldPresentShardAsCheck(checkStatus: false, isGmIp: false),
        "A normal player was shown Check while maintenance mode was disabled.");

    Assert(
        RefManager.TryNormalizeIpAddress(" 192.0.2.10 ", out var ipv4) && ipv4 == "192.0.2.10",
        "GM IPv4 normalization failed.");
    Assert(
        RefManager.TryNormalizeIpAddress("::ffff:192.0.2.10", out var mappedIpv4) && mappedIpv4 == ipv4,
        "IPv4-mapped GM address normalization failed.");
}

static void VerifyNpcUniqueIdSnapshotCompatibility()
{
    var rows = new[]
    {
        new ___SR_GSNpcUniqueIdList { ID = 228, UniqueID = 1119, RefObjId = 7537, CodeName128 = "NPC_CA_WAREHOUSE" },
        new ___SR_GSNpcUniqueIdList { ID = 52, UniqueID = 1119, RefObjId = 2077, CodeName128 = "NPC_KT_SPECIAL" },
        new ___SR_GSNpcUniqueIdList { ID = 53, UniqueID = 1120, RefObjId = 2078, CodeName128 = "NPC_KT_STORE" }
    };

    var snapshot = RefManager.BuildNpcUniqueIdSnapshot(rows, out var duplicateCount);

    Assert(snapshot.Count == 2, "Duplicate NPC UniqueIDs crashed or expanded the NPC snapshot.");
    Assert(duplicateCount == 1, "The NPC snapshot did not report its duplicate row.");
    Assert(snapshot[1119].ID == 52,
        "The NPC snapshot did not preserve the legacy first-row mapping deterministically.");
}

static void VerifyNewAlchemyRemainsUnavailable()
{
    Assert(
        !_serverSettings.IsNewAlchemyAvailable,
        "The retired custom New Alchemy pipeline became available again.");
}

static void PumpSecurity(
    Security sender,
    Security receiver,
    List<string>? trace = null,
    string direction = "")
{
    var outgoing = sender.TransferOutgoing();
    if (outgoing == null)
        return;

    foreach (var item in outgoing)
    {
        trace?.Add($"{direction}:0x{item.Value.Opcode:X4}");
        receiver.Recv(item.Key.Buffer, 0, item.Key.Buffer.Length);
    }
}

static void DrainSecurity(Security security, List<Packet>? destination = null)
{
    var incoming = security.TransferIncoming();
    if (incoming != null && destination != null)
        destination.AddRange(incoming);
}

static void Assert(bool condition, string message)
{
    if (!condition)
        throw new InvalidOperationException(message);
}

static void VerifyBotIdentityClassification()
{
    Assert(BotProtectionService.IsBotIdentity(true, string.Empty),
        "Managed clientless session was not classified as a bot.");
    Assert(BotProtectionService.IsBotIdentity(false, "CLIENTLESS:test-account"),
        "Clientless HWID marker was not classified as a bot.");
    Assert(BotProtectionService.IsBotIdentity(false, "clientless:test-account"),
        "Clientless HWID marker classification must be case-insensitive.");
    Assert(BotProtectionService.IsBotIdentity(false, true, string.Empty),
        "An admitted external bot session was not classified as a bot.");
    Assert(!BotProtectionService.IsBotIdentity(false, "ABCDEF0123456789"),
        "A verified regular client was classified as a bot.");

    var firstExternalBotHwid = BotProtectionService.CreateExternalBotHwid(" TestAccount ");
    var repeatedExternalBotHwid = BotProtectionService.CreateExternalBotHwid("testaccount");
    Assert(firstExternalBotHwid.Length == 64 && firstExternalBotHwid.All(Uri.IsHexDigit),
        "External bot identity must use the normal 64-character HWID contract.");
    Assert(firstExternalBotHwid == repeatedExternalBotHwid,
        "External bot identity must be stable and account-name case-insensitive.");
    Assert(firstExternalBotHwid != BotProtectionService.CreateExternalBotHwid("another-account"),
        "Different external bot accounts must not share an identity.");
}

static void VerifyManagedClientlessGatewayTrustBoundary()
{
    var handler = typeof(SERVER_DLL_SETTINGS_RESPONSE);
    var localAddresses = new[]
    {
        IPAddress.Parse("192.168.50.10"),
        IPAddress.Parse("2001:db8::10")
    };

    Assert(SERVER_DLL_SETTINGS_RESPONSE.CanUseManagedClientlessLogin(
            true, "127.0.0.1", "203.0.113.20", localAddresses),
        $"{handler.Name} rejected a managed Clientless loopback connection.");
    Assert(SERVER_DLL_SETTINGS_RESPONSE.CanUseManagedClientlessLogin(
            true, "::ffff:127.0.0.1", "203.0.113.20", localAddresses),
        $"{handler.Name} rejected an IPv4-mapped managed Clientless loopback connection.");
    Assert(SERVER_DLL_SETTINGS_RESPONSE.CanUseManagedClientlessLogin(
            true, "203.0.113.20", "203.0.113.20", localAddresses),
        $"{handler.Name} rejected the configured local Filter address.");
    Assert(SERVER_DLL_SETTINGS_RESPONSE.CanUseManagedClientlessLogin(
            true, "192.168.50.10", "203.0.113.20", localAddresses),
        $"{handler.Name} rejected a local network-interface address.");
    Assert(!SERVER_DLL_SETTINGS_RESPONSE.CanUseManagedClientlessLogin(
            false, "127.0.0.1", "203.0.113.20", localAddresses),
        $"{handler.Name} allowed an unmanaged loopback client to bypass device proof.");
    Assert(!SERVER_DLL_SETTINGS_RESPONSE.CanUseManagedClientlessLogin(
            true, "198.51.100.25", "203.0.113.20", localAddresses),
        $"{handler.Name} allowed a remote managed account to bypass device proof.");
    Assert(!SERVER_DLL_SETTINGS_RESPONSE.CanUseManagedClientlessLogin(
            true, "not-an-ip", "203.0.113.20", localAddresses),
        $"{handler.Name} trusted an invalid client address.");
}

static void VerifyAutoEventTableContracts()
{
    Assert(
        PartyDataLocator.QualifiedTableName == "[dbo].[Party_Members]",
        "Auto Events must read party membership from dbo.Party_Members.");

    Assert(AutoEventService.NormalizeEventCode("last man standing") == "LMS",
        "Last Man Standing console alias was not normalized.");
    Assert(AutoEventService.NormalizeEventCode("madness-solo") == "MADNESS",
        "Madness console alias was not normalized.");
    Assert(AutoEventService.NormalizeEventCode("defend_the_tower") == "DTT",
        "Defend The Tower console alias was not normalized.");
    Assert(AutoEventService.WasEventStartAccepted("Started LMS with 1 rounds.", "LMS"),
        "Immediate Auto Event start result was not recognized.");
    Assert(AutoEventService.WasEventStartAccepted("LMS scheduled. First round starts in 1 minute. Rounds: 1.", "LMS"),
        "Delayed Auto Event start result was not recognized.");
    Assert(AutoEventService.ShouldRetryScheduledStart("Another event is already running: HNS."),
        "A schedule blocked by another event must remain eligible for the day.");
    Assert(AutoEventService.ShouldRetryScheduledStart(
            "Scheduled event waiting for the 15-minute safety gap (4 minute(s) remaining)."),
        "A schedule delayed by the safety gap must remain eligible.");
    Assert(!AutoEventService.ShouldRetryScheduledStart("Event LMS is disabled."),
        "A disabled schedule must not retry every worker cycle.");

    var scheduleNow = new DateTime(2026, 8, 9, 12, 5, 0);
    Assert(AutoEventService.TryGetDueAutoEventScheduleOccurrence(
            new TimeSpan(10, 0, 0),
            1 << (int)scheduleNow.DayOfWeek,
            60,
            scheduleNow.Date,
            new DateTime(2026, 8, 9, 11, 0, 0),
            scheduleNow,
            out var hourlyOccurrence) &&
           hourlyOccurrence == new DateTime(2026, 8, 9, 12, 0, 0),
        "Hourly Auto Event schedule did not resolve the latest due occurrence.");
    Assert(!AutoEventService.TryGetDueAutoEventScheduleOccurrence(
            new TimeSpan(10, 0, 0),
            1 << (int)scheduleNow.DayOfWeek,
            60,
            scheduleNow.Date,
            hourlyOccurrence,
            scheduleNow,
            out _),
        "Hourly Auto Event schedule allowed the same occurrence twice.");
    Assert(AutoEventService.GetNextAutoEventScheduleOccurrence(
               new TimeSpan(10, 0, 0),
               1 << (int)scheduleNow.DayOfWeek,
               60,
               scheduleNow) == new DateTime(2026, 8, 9, 13, 0, 0),
        "Hourly Auto Event schedule did not calculate its next occurrence.");

    var lastFinishedUtc = new DateTime(2026, 8, 9, 12, 0, 0, DateTimeKind.Utc);
    Assert(!AutoEventService.HasAutomaticEventGapElapsed(lastFinishedUtc, lastFinishedUtc.AddMinutes(14).AddSeconds(59)),
        "Automatic event safety gap ended before 15 minutes.");
    Assert(AutoEventService.HasAutomaticEventGapElapsed(lastFinishedUtc, lastFinishedUtc.AddMinutes(15)),
        "Automatic event safety gap did not end at exactly 15 minutes.");

    Assert(AutoEventService.IsConfiguredAnswerCorrect("Retype", "KMTGuard", "KMTGuard"),
        "Retype rejected an exact case-sensitive answer.");
    Assert(!AutoEventService.IsConfiguredAnswerCorrect("Retype", "KMTGuard", "kmtguard"),
        "Retype accepted an answer with different letter casing.");
    Assert(AutoEventService.IsConfiguredAnswerCorrect("FirstType", "KMTGuard", "kmtguard"),
        "First Type unexpectedly inherited Retype's case-sensitive contract.");

    Assert(AutoEventService.ResolveAlchemyTargetPlus(12) == 12,
        "Alchemy did not retain the target configured by the dashboard.");
    Assert(AutoEventService.ResolveAlchemyTargetPlus(0) == 1,
        "Alchemy target was not clamped to the supported minimum.");
    Assert(AutoEventService.ResolveAlchemyTargetPlus(25) == 20,
        "Alchemy target was not clamped to the supported maximum.");

    var roundStartUtc = new DateTime(2026, 8, 9, 12, 0, 0, DateTimeKind.Utc);
    var roundEndUtc = roundStartUtc.AddSeconds(60);
    Assert(AutoEventService.IsWithinRoundWindow(roundStartUtc, roundEndUtc, roundStartUtc),
        "Auto Event rejected an action at the start of its round window.");
    Assert(AutoEventService.IsWithinRoundWindow(roundStartUtc, roundEndUtc, roundEndUtc),
        "Auto Event rejected an action at the exact round deadline.");
    Assert(!AutoEventService.IsWithinRoundWindow(roundStartUtc, roundEndUtc, roundEndUtc.AddTicks(1)),
        "Auto Event accepted an action after the round deadline.");

    var recentDeaths = new ConcurrentDictionary<int, DateTime>();
    var deathUtc = new DateTime(2026, 8, 8, 12, 0, 0, DateTimeKind.Utc);
    var acceptedDeaths = 0;
    Parallel.For(0, 64, _ =>
    {
        if (AutoEventService.TryRecordDeath(recentDeaths, 42, deathUtc, TimeSpan.FromSeconds(1)))
            Interlocked.Increment(ref acceptedDeaths);
    });
    Assert(acceptedDeaths == 1,
        "Concurrent duplicate kill packets were not collapsed into one death.");
    Assert(!AutoEventService.TryRecordDeath(recentDeaths, 42, deathUtc.AddMilliseconds(999), TimeSpan.FromSeconds(1)),
        "A kill inside the duplicate window was accepted.");
    Assert(AutoEventService.TryRecordDeath(recentDeaths, 42, deathUtc.AddSeconds(1), TimeSpan.FromSeconds(1)),
        "A legitimate later death was rejected.");
}

static void VerifySchedulerOccurrenceCalculation()
{
    var now = new DateTime(2026, 7, 27, 12, 2, 0);

    var daily = CreateSchedulerJob(
        RepeatType.Daily,
        new TimeSpan(12, 0, 0),
        catchUpSeconds: 300);
    Assert(
        Scheduler.TryGetDueOccurrence(daily, now, out var dailyOccurrence) &&
        dailyOccurrence == new DateTime(2026, 7, 27, 12, 0, 0),
        "Daily scheduler catch-up occurrence was not calculated correctly.");

    var expired = CreateSchedulerJob(
        RepeatType.Daily,
        new TimeSpan(11, 55, 0),
        catchUpSeconds: 300);
    Assert(
        !Scheduler.TryGetDueOccurrence(expired, now.AddSeconds(1), out _),
        "Scheduler accepted an occurrence outside its catch-up window.");

    var weekly = CreateSchedulerJob(
        RepeatType.Weekly,
        new TimeSpan(12, 0, 0),
        repeatDay: 1,
        catchUpSeconds: 300);
    Assert(
        Scheduler.TryGetDueOccurrence(weekly, now, out var weeklyOccurrence) &&
        weeklyOccurrence == new DateTime(2026, 7, 27, 12, 0, 0),
        "Weekly scheduler occurrence was not calculated correctly.");

    var multiDayWeekly = CreateSchedulerJob(
        RepeatType.Weekly,
        new TimeSpan(11, 59, 0),
        daysMask: 0b_0000_101,
        catchUpSeconds: 300);
    Assert(
        Scheduler.TryGetDueOccurrence(multiDayWeekly, now, out var multiDayOccurrence) &&
        multiDayOccurrence == new DateTime(2026, 7, 27, 11, 59, 0),
        "Multi-day weekly scheduler occurrence was not calculated correctly.");

    var interval = CreateSchedulerJob(
        RepeatType.Interval,
        TimeSpan.Zero,
        startDateTime: new DateTime(2026, 7, 27, 10, 0, 0),
        intervalSeconds: 600,
        catchUpSeconds: 600);
    Assert(
        Scheduler.TryGetDueOccurrence(interval, now, out var intervalOccurrence) &&
        intervalOccurrence == new DateTime(2026, 7, 27, 12, 0, 0),
        "Ten-minute scheduler occurrence was not calculated correctly.");

    var intervalAlreadyClaimed = CreateSchedulerJob(
        RepeatType.Interval,
        TimeSpan.Zero,
        startDateTime: new DateTime(2026, 7, 27, 10, 0, 0),
        intervalSeconds: 600,
        catchUpSeconds: 600,
        lastScheduled: new DateTime(2026, 7, 27, 12, 0, 0));
    Assert(
        !Scheduler.TryGetDueOccurrence(intervalAlreadyClaimed, now, out _),
        "Interval scheduler allowed an already claimed occurrence to run twice.");

    var alreadyClaimed = CreateSchedulerJob(
        RepeatType.Daily,
        new TimeSpan(12, 0, 0),
        catchUpSeconds: 300,
        lastScheduled: new DateTime(2026, 7, 27, 12, 0, 0));
    Assert(
        !Scheduler.TryGetDueOccurrence(alreadyClaimed, now, out _),
        "Scheduler allowed an already claimed occurrence to run twice.");

    var oneTime = CreateSchedulerJob(
        RepeatType.None,
        new TimeSpan(12, 0, 0),
        scheduledDate: new DateOnly(2026, 7, 27),
        catchUpSeconds: 300);
    Assert(
        Scheduler.TryGetDueOccurrence(oneTime, now, out var oneTimeOccurrence) &&
        oneTimeOccurrence == new DateTime(2026, 7, 27, 12, 0, 0),
        "One-time scheduler occurrence was not calculated correctly.");
}

static void VerifySchedulerValidation()
{
    Assert(Scheduler.IsAllowedScheduledSql("EXEC dbo.StartEvent @EventId = 7"), "Valid scheduler EXEC was rejected.");
    Assert(Scheduler.IsAllowedScheduledSql("EXEC [Events].[dbo].[StartEvent] @EventId = 7"), "Bracketed scheduler procedure was rejected.");
    Assert(!Scheduler.IsAllowedScheduledSql("DELETE dbo.Events"), "Unsafe scheduler SQL was accepted.");
    Assert(!Scheduler.IsAllowedScheduledSql("EXEC dbo.One; EXEC dbo.Two"), "Multiple scheduler commands were accepted.");
    Assert(!Scheduler.IsAllowedScheduledSql("EXEC dbo.One -- comment"), "Commented scheduler SQL was accepted.");
    Assert(!Scheduler.IsAllowedScheduledSql("EXEC sys.sp_executesql N'DELETE dbo.Events'"), "Dynamic scheduler SQL was accepted.");
    Assert(!Scheduler.IsAllowedScheduledSql("EXEC master.dbo.xp_cmdshell 'whoami'"), "Extended scheduler procedure was accepted.");
}

static void VerifyItemChestContracts()
{
    Assert(ItemChestService.SnapshotChunkSize > 0 &&
           ItemChestService.SnapshotChunkSize <= byte.MaxValue,
        "Item Chest snapshot chunk size does not fit its packet field.");
    Assert(ItemChestService.MaximumChestEntries >= ItemChestService.SnapshotChunkSize,
        "Item Chest maximum entry count is smaller than one snapshot chunk.");

    var valid = new _ItemChest
    {
        ID = 1,
        CharID = 2,
        ItemID = 3,
        ItemCodeName = "ITEM_TEST",
        Quantity = 1,
        Plus = 0
    };
    Assert(ItemChestService.IsValidItem(valid), "A valid Item Chest row was rejected.");

    valid.Quantity = 0;
    Assert(!ItemChestService.IsValidItem(valid), "A zero-quantity Item Chest row was accepted.");
    valid.Quantity = 1;
    valid.ItemCodeName = string.Empty;
    Assert(!ItemChestService.IsValidItem(valid), "An Item Chest row without CodeName was accepted.");

    var sessionData = new SessionData();
    Assert(sessionData.PendingChestClaims.TryAdd(10, Guid.NewGuid()),
        "The first Item Chest pending claim was rejected.");
    Assert(!sessionData.PendingChestClaims.TryAdd(10, Guid.NewGuid()),
        "A duplicate Item Chest pending claim was accepted.");
}

static void VerifyPlayerLanguages()
{
    var english = PlayerLanguage.Initialize("English");
    Assert(english.Language == "English", "English player language did not load.");
    Assert(english.MissingKeyCount == 0, "English player language contains missing keys.");
    Assert(english.InvalidPlaceholderCount == 0, "English player language contains invalid placeholders.");
    Assert(
        PlayerLanguage.Get("QuickLogin.Accepted").Contains("accepted", StringComparison.OrdinalIgnoreCase),
        "English player-language lookup returned the wrong text.");

    var turkish = PlayerLanguage.Initialize("tr-TR");
    Assert(turkish.Language == "Turkish", "Turkish language alias did not resolve.");
    Assert(turkish.MissingKeyCount == 0, "Turkish player language is missing English fallback keys.");
    Assert(turkish.InvalidPlaceholderCount == 0, "Turkish player language contains invalid placeholders.");
    Assert(
        PlayerLanguage.Get("QuickLogin.Accepted").Contains("kabul", StringComparison.OrdinalIgnoreCase),
        "Turkish player-language lookup returned the wrong text.");

    var formattedNotice = string.Format(
        PlayerLanguage.ResolveSystemNotice("MSG_REVERSE_DELAY", "database fallback {0}"),
        7);
    Assert(formattedNotice.Contains('7'), "Localized System_Notices placeholder was not preserved.");

    var switched = PlayerLanguage.Switch("English");
    Assert(switched.Language == "English", "Live player-language switch did not apply English.");
    try
    {
        PlayerLanguage.Switch("LanguageFileThatDoesNotExist");
        throw new InvalidOperationException("A missing live language file was accepted.");
    }
    catch (FileNotFoundException)
    {
        Assert(
            PlayerLanguage.CurrentLanguage == "English",
            "A failed live language switch replaced the active language.");
    }

    PlayerLanguage.Initialize("English");
}

static void VerifyRegionAdmissionPolicy()
{
    var joblessInt = new RegionAdmissionProfile(120, 300, 100, 4, true, false, false);
    var traderStr = new RegionAdmissionProfile(300, 120, 100, 1, true, false, true);

    Assert(
        RegionControlService.EvaluateAdmission(new _FilterRegionControl(), joblessInt, RegionTravelMethod.Teleport).Allowed,
        "A default region admission rule rejected a valid character.");

    var rule = new _FilterRegionControl { BuildMode = 1 };
    Assert(
        RegionControlService.EvaluateAdmission(rule, joblessInt, RegionTravelMethod.Reverse).LanguageKey == "Region.IntCharactersDisabled",
        "INT admission restriction was bypassed.");

    rule = new _FilterRegionControl { JobMode = 2 };
    Assert(
        RegionControlService.EvaluateAdmission(rule, joblessInt, RegionTravelMethod.Teleport).LanguageKey == "Region.JobRequired",
        "Job-required admission restriction was bypassed.");
    Assert(
        RegionControlService.EvaluateAdmission(rule, traderStr, RegionTravelMethod.Teleport).Allowed,
        "An allowed Trader was rejected by the job-required rule.");

    rule = new _FilterRegionControl { AllowReverse = false };
    Assert(
        RegionControlService.EvaluateAdmission(rule, traderStr, RegionTravelMethod.Reverse).LanguageKey == "Region.ReverseDisabled",
        "Destination Reverse restriction was bypassed.");

    rule = new _FilterRegionControl { BuildMode = 1 };
    var hybrid = traderStr with { Strength = 200, Intellect = 200 };
    Assert(
        RegionControlService.EvaluateAdmission(rule, hybrid, RegionTravelMethod.Trace).LanguageKey == "Region.HybridCharactersDisabled",
        "Hybrid admission restriction was bypassed.");

    rule = new _FilterRegionControl { PartyMode = 2 };
    Assert(
        RegionControlService.EvaluateAdmission(rule, traderStr, RegionTravelMethod.Teleport).LanguageKey == "Region.PartyNotAllowed",
        "Solo-only admission restriction was bypassed.");

    Assert(RegionControlService.NormalizeRegionId(32788) == -32748,
        "Unsigned packet RegionID was not normalized to its signed SQL value.");
    Assert(RegionControlService.IsValidRegionId(-32748),
        "A valid negative vSRO RegionID was rejected.");
    Assert(!RegionControlService.IsValidRegionId(0),
        "RegionID zero was accepted as a destination.");

    Assert(
        CharAction.TryParseTeleportUseRequest(new byte[] { 1, 0, 0, 0, 5, 2 }, out var lastRecall)
        && lastRecall.TeleportType == 5
        && lastRecall.DestinationSelector == 2,
        "The six-byte Reverse last-recall request was not parsed before travel.");
    Assert(
        CharAction.TryParseTeleportUseRequest(new byte[] { 1, 0, 0, 0, 5, 3 }, out var deathLocation)
        && deathLocation.DestinationSelector == 3,
        "The six-byte Reverse death-location request was not parsed before travel.");
    Assert(
        !CharAction.TryParseTeleportUseRequest(new byte[] { 1, 0, 0, 0, 5, 1 }, out _),
        "An unsupported Reverse destination selector was accepted.");
    Assert(
        CharAction.TryParseTeleportUseRequest(new byte[] { 1, 0, 0, 0, 2, 21, 0, 0, 0 }, out var normalGate)
        && normalGate.DestinationSelector == 21,
        "A normal RefTeleport request was not parsed correctly.");
    Assert(
        !CharAction.TryParseTeleportUseRequest(new byte[] { 1, 0, 0, 0, 2, 21 }, out _),
        "A truncated RefTeleport request was accepted.");
}

static SchedulerJob CreateSchedulerJob(
    RepeatType repeat,
    TimeSpan time,
    DateOnly? scheduledDate = null,
    byte? repeatDay = null,
    byte? daysMask = null,
    DateTime? startDateTime = null,
    int? intervalSeconds = null,
    int catchUpSeconds = 300,
    DateTime? lastScheduled = null)
{
    return new SchedulerJob
    {
        Idx = 1,
        Name = "Test",
        Query = "EXEC dbo.Test",
        ScheduledDate = scheduledDate,
        StartDateTime = startDateTime,
        Time = time,
        Repeat = repeat,
        RepeatDayOfWeek = repeatDay,
        DaysOfWeekMask = daysMask,
        IntervalSeconds = intervalSeconds,
        IsEnabled = true,
        LastScheduledDateTime = lastScheduled,
        ExecutionTimeoutSeconds = 7200,
        CatchUpWindowSeconds = catchUpSeconds
    };
}
