using System.Collections.Concurrent;
using System.Data;
using Dapper;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Features.AutoEvents;

public enum CompetitiveEventMode
{
    LastManStanding,
    MadnessSolo,
    DefendTower
}

public sealed record CompetitiveEventRuntimeConfig(
    string EventCode,
    string DisplayName,
    CompetitiveEventMode Mode,
    int EventID,
    bool Enabled,
    int StartDelaySeconds,
    int RegistrationSeconds,
    int PrepareSeconds,
    int FightSeconds,
    int MinPlayers,
    int MaxPlayers,
    int MinLevel,
    int HwidLimit,
    bool RequireHwid,
    bool RequireNoParty,
    int ArenaWorldID,
    int ArenaRegionID,
    int ArenaX,
    int ArenaY,
    int ArenaZ,
    int Team1X,
    int Team1Y,
    int Team1Z,
    int Team2X,
    int Team2Y,
    int Team2Z,
    int MadnessMobID,
    int MadnessMobCount,
    int MadnessMobX,
    int MadnessMobY,
    int MadnessMobZ,
    int MobSpawnDelaySeconds,
    int PairKillLimit,
    int TotalKillLimit,
    string KillRewardItemCode,
    int KillRewardItemCount,
    int KillRewardLimit,
    int Team1TowerMobID,
    int Team1TowerX,
    int Team1TowerY,
    int Team1TowerZ,
    int Team2TowerMobID,
    int Team2TowerX,
    int Team2TowerY,
    int Team2TowerZ);

public static class CompetitiveEventService
{
    private const int TowerSpawnRadius = 5;
    private const string EventDatabaseName = "Events";
    private const int ReturnWorldId = 1;
    private const int ReturnRegionId = 25000;
    private const int ReturnPosX = 982;
    private const int ReturnPosY = 0;
    private const int ReturnPosZ = 140;
    private const int Team1Cape = 1;
    private const int Team2Cape = 3;
    private static readonly TimeSpan KillReviveDelay = TimeSpan.FromSeconds(10);
    private static readonly SemaphoreSlim StateLock = new(1, 1);
    private static readonly ConcurrentDictionary<int, string> RegistrationCodes = new(new[]
    {
        new KeyValuePair<int, string>(14, "LMS"),
        new KeyValuePair<int, string>(15, "MADNESS"),
        new KeyValuePair<int, string>(16, "DTT")
    });
    private static ActiveCompetitiveRun? _active;

    private static string EventTable(string tableName) =>
        $"{SqlIdentifier.Quote(EventDatabaseName)}.dbo.{SqlIdentifier.Quote(tableName)}";

    public static bool IsRegistrationEventId(int eventId) =>
        (_active?.Config.EventID == eventId) || RegistrationCodes.ContainsKey(eventId);

    public static async Task EnsureSchemaAsync(SqlConnection connection)
    {
        var ready = await connection.ExecuteScalarAsync<int>(@"
SELECT CASE WHEN OBJECT_ID(N'Events.dbo._CompetitiveEventConfig', N'U') IS NOT NULL
                  AND OBJECT_ID(N'Events.dbo._CompetitiveEventReward', N'U') IS NOT NULL
                  AND OBJECT_ID(N'Events.dbo._CompetitiveEventSchedule', N'U') IS NOT NULL
                  AND OBJECT_ID(N'Events.dbo._CompetitiveEventScore', N'U') IS NOT NULL
                  AND OBJECT_ID(N'Events.dbo._CompetitiveEventKillLedger', N'U') IS NOT NULL
                  AND OBJECT_ID(N'dbo.Command_GameServerResult', N'U') IS NOT NULL
                  AND OBJECT_ID(N'dbo.Command_ClaimGameServer', N'P') IS NOT NULL
                  AND OBJECT_ID(N'dbo.Command_CompleteGameServer', N'P') IS NOT NULL
                  AND OBJECT_ID(N'dbo.Command_RetryGameServer', N'P') IS NOT NULL
             THEN 1 ELSE 0 END;");
        if (ready != 1)
            throw new InvalidOperationException(
                "Competitive Events schema is incomplete. Apply the packaged database updates.");
    }
    public static async Task<CompetitiveEventRuntimeConfig> LoadRuntimeConfigAsync(string eventCode)
    {
        eventCode = NormalizeCode(eventCode);
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await EnsureSchemaAsync(connection);
        var row = await connection.QuerySingleOrDefaultAsync<CompetitiveConfigRow>(@"
SELECT * FROM Events.dbo._CompetitiveEventConfig WITH (NOLOCK) WHERE EventCode=@EventCode;",
            new { EventCode = eventCode });
        if (row == null)
            throw new InvalidOperationException($"Competitive event '{eventCode}' is not configured.");

        var mode = row.EventMode switch
        {
            "LastManStanding" => CompetitiveEventMode.LastManStanding,
            "MadnessSolo" => CompetitiveEventMode.MadnessSolo,
            "DefendTower" => CompetitiveEventMode.DefendTower,
            _ => throw new InvalidOperationException($"Unsupported competitive event mode '{row.EventMode}'.")
        };

        RegistrationCodes[Math.Clamp(row.EventID, 1, 255)] = eventCode;
        return new CompetitiveEventRuntimeConfig(
            eventCode, Trim(row.DisplayName, 64), mode, Math.Clamp(row.EventID, 1, 255), row.Enabled,
            Math.Clamp(row.StartDelaySeconds, 0, 3600), Math.Clamp(row.RegistrationSeconds, 10, 3600),
            Math.Clamp(row.PrepareSeconds, 0, 300), Math.Clamp(row.FightSeconds, 30, 7200),
            Math.Clamp(row.MinPlayers, 2, 100), Math.Clamp(row.MaxPlayers, 2, 100), Math.Max(0, row.MinLevel),
            Math.Clamp(row.HwidLimit, 0, 32), row.RequireHwid, row.RequireNoParty,
            Math.Max(1, row.ArenaWorldID), Math.Max(1, row.ArenaRegionID), row.ArenaX, row.ArenaY, row.ArenaZ,
            row.Team1X, row.Team1Y, row.Team1Z, row.Team2X, row.Team2Y, row.Team2Z,
            Math.Max(0, row.MadnessMobID), Math.Clamp(row.MadnessMobCount, 0, 20),
            row.MadnessMobX, row.MadnessMobY, row.MadnessMobZ, Math.Clamp(row.MobSpawnDelaySeconds, 0, 3600),
            Math.Clamp(row.PairKillLimit, 0, 100), Math.Clamp(row.TotalKillLimit, 0, 100000),
            Trim(row.KillRewardItemCode, 128), Math.Clamp(row.KillRewardItemCount, 1, 10000),
            Math.Clamp(row.KillRewardLimit, 0, 100000),
            Math.Max(0, row.Team1TowerMobID), row.Team1TowerX, row.Team1TowerY, row.Team1TowerZ,
            Math.Max(0, row.Team2TowerMobID), row.Team2TowerX, row.Team2TowerY, row.Team2TowerZ);
    }

    public static string? ValidateForStart(CompetitiveEventRuntimeConfig config)
    {
        if (config.MinPlayers > config.MaxPlayers)
            return "Minimum players cannot be greater than maximum players.";
        if (config.Mode == CompetitiveEventMode.MadnessSolo && config.MadnessMobCount > 0 && config.MadnessMobID <= 0)
            return "Madness MobID must be configured when mob count is greater than zero.";
        if (config.Mode == CompetitiveEventMode.DefendTower)
        {
            if (config.Team1TowerMobID <= 0 || config.Team2TowerMobID <= 0)
                return "Both tower MobIDs must be configured before starting Defend The Tower.";
            if (config.Team1TowerMobID == config.Team2TowerMobID)
                return "Each tower must use a different MobID.";
        }
        return null;
    }

    public static async Task<string> HandleRegistrationAsync(ISession session, int eventId)
    {
        var run = _active;
        if (run == null || run.Config.EventID != eventId || run.Phase != CompetitivePhase.Registration ||
            DateTime.UtcNow >= run.RegistrationEndsAtUtc)
        {
            await TrySendPersonalNoticeAsync(session, PlayerLanguage.Get("Competitive.RegistrationClosed"));
            return "Registration closed.";
        }
        if (!ServerManager.IsOnlinePlayer(session))
            return "Character is not online.";
        if (run.Config.MinLevel > 0 && session.SessionData.CurLevel < run.Config.MinLevel)
        {
            await TrySendPersonalNoticeAsync(session, PlayerLanguage.Get("Competitive.MinimumLevel", run.Config.DisplayName, run.Config.MinLevel));
            return "Minimum level failed.";
        }
        if (run.Config.RequireHwid && string.IsNullOrWhiteSpace(session.SessionData.Hwid))
        {
            await TrySendPersonalNoticeAsync(session, PlayerLanguage.Get("Competitive.HwidRequired", run.Config.DisplayName));
            return "HWID required.";
        }

        await StateLock.WaitAsync();
        try
        {
            if (!ReferenceEquals(run, _active) || run.Phase != CompetitivePhase.Registration ||
                DateTime.UtcNow >= run.RegistrationEndsAtUtc)
            {
                await TrySendPersonalNoticeAsync(session, PlayerLanguage.Get("Competitive.RegistrationClosed"));
                return "Registration closed.";
            }
            var charId = session.SessionData.Charid;
            if (run.Config.RequireNoParty && await IsInPartyAsync(charId))
            {
                await TrySendPersonalNoticeAsync(session, PlayerLanguage.Get("Competitive.LeaveParty", run.Config.DisplayName));
                return "Player is in a party.";
            }
            if (run.Participants.ContainsKey(charId))
            {
                await TrySendPersonalNoticeAsync(session, PlayerLanguage.Get("Competitive.AlreadyRegistered", run.Config.DisplayName));
                return "Duplicate player.";
            }
            if (run.Participants.Count >= run.Config.MaxPlayers)
            {
                await TrySendPersonalNoticeAsync(session, PlayerLanguage.Get("Competitive.RegistrationFull", run.Config.DisplayName));
                return "Registration full.";
            }
            var hwid = session.SessionData.Hwid ?? string.Empty;
            if (run.Config.HwidLimit > 0 && !string.IsNullOrWhiteSpace(hwid) &&
                run.Participants.Values.Count(x => x.Hwid.Equals(hwid, StringComparison.OrdinalIgnoreCase)) >= run.Config.HwidLimit)
            {
                await TrySendPersonalNoticeAsync(session, PlayerLanguage.Get("Competitive.HwidLimit", run.Config.DisplayName, run.Config.HwidLimit));
                return "HWID limit reached.";
            }
            if (DateTime.UtcNow >= run.RegistrationEndsAtUtc)
            {
                await TrySendPersonalNoticeAsync(session, PlayerLanguage.Get("Competitive.RegistrationClosed"));
                return "Registration closed.";
            }

            var team = run.Config.Mode == CompetitiveEventMode.DefendTower
                ? (run.Participants.Values.Count(x => x.Team == 1) <= run.Participants.Values.Count(x => x.Team == 3) ? 1 : 3)
                : 0;
            var participant = new CompetitiveParticipant(
                charId,
                Trim(session.SessionData.Charname ?? $"#{charId}", 64),
                session.SessionData.JID,
                hwid,
                session.ClientIp ?? string.Empty,
                team,
                run.Participants.Count + 1);
            await PersistRegistrationAsync(run, participant);
            run.Participants[charId] = participant;
            run.Scores[charId] = new CompetitiveScore(charId, participant.CharName, team, 0, true);
            await TrySendPersonalNoticeAsync(session, PlayerLanguage.Get("Competitive.Registered", run.Config.DisplayName));
            await TryBroadcastNoticeAsync(
                PlayerLanguage.Get(
                    "Competitive.PlayerRegisteredBroadcast",
                    run.Config.DisplayName,
                    participant.CharName,
                    run.Participants.Count,
                    run.Config.MaxPlayers),
                NoticeType.NOTICE);
            return $"Registered player {charId}.";
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "{EventCode} registration failed for CharID={CharID}", run.Config.EventCode, session.SessionData.Charid);
            await TrySendPersonalNoticeAsync(session, PlayerLanguage.Get("Competitive.RegistrationFailed", run.Config.DisplayName));
            return ex.Message;
        }
        finally
        {
            StateLock.Release();
        }
    }

    public static async Task HandleCharacterKillAsync(int worldId, int regionId, int killerCharId, string killerName, int deadCharId)
    {
        var run = _active;
        if (run == null || run.Phase != CompetitivePhase.Fighting || worldId != run.Config.ArenaWorldID ||
            regionId != run.Config.ArenaRegionID || killerCharId <= 0 || deadCharId <= 0 || killerCharId == deadCharId ||
            !run.Participants.TryGetValue(killerCharId, out var killer) || !run.Participants.ContainsKey(deadCharId))
            return;

        if (run.Config.Mode == CompetitiveEventMode.DefendTower &&
            run.Participants.TryGetValue(deadCharId, out var deadParticipant) && deadParticipant.Team == killer.Team)
            return;
        if (run.Config.Mode == CompetitiveEventMode.LastManStanding &&
            (!run.Scores.TryGetValue(deadCharId, out var lmsVictim) || !lmsVictim.IsAlive))
            return;

        var now = DateTime.UtcNow;
        if (!AutoEventService.TryRecordDeath(run.LastDeathUtc, deadCharId, now, TimeSpan.FromSeconds(1)))
            return;
        if (run.Config.Mode != CompetitiveEventMode.LastManStanding)
            run.DeadCharacters[deadCharId] = 0;

        if (run.Config.Mode == CompetitiveEventMode.MadnessSolo)
        {
            var pair = (killerCharId, deadCharId);
            var pairKills = run.PairKills.AddOrUpdate(pair, 1, (_, value) => value + 1);
            if (run.Config.PairKillLimit > 0 && pairKills > run.Config.PairKillLimit)
            {
                await TrySendPersonalNoticeAsync(
                    FindOnlineCharacter(killerCharId),
                    PlayerLanguage.Get("Competitive.PairKillLimitReached", run.Config.DisplayName));
                _ = QueueGetUpAfterKillDelayAsync(run, deadCharId, now);
                return;
            }
            var currentKills = run.Scores.TryGetValue(killerCharId, out var current) ? current.KillCount : 0;
            if (run.Config.TotalKillLimit > 0 && currentKills >= run.Config.TotalKillLimit)
            {
                _ = QueueGetUpAfterKillDelayAsync(run, deadCharId, now);
                return;
            }
            await UpsertPairKillsAsync(run.RunID, killerCharId, deadCharId, pairKills);
        }

        var updated = run.Scores.AddOrUpdate(killerCharId,
            _ => new CompetitiveScore(killerCharId, Trim(killerName, 64), killer.Team, 1, true),
            (_, score) => score with { KillCount = score.KillCount + 1 });

        if (run.Config.Mode == CompetitiveEventMode.LastManStanding)
        {
            if (run.Scores.TryGetValue(deadCharId, out var victim) && victim.IsAlive)
            {
                run.Scores[deadCharId] = victim with { IsAlive = false };
                await UpsertScoreAsync(run, run.Scores[deadCharId]);
                _ = EliminatePlayerSafelyAsync(run, deadCharId);
            }
        }
        else
        {
            _ = QueueGetUpAfterKillDelayAsync(run, deadCharId, now);
        }

        await UpsertScoreAsync(run, updated);
        await BroadcastScoreAsync(run);

        if (run.Config.Mode == CompetitiveEventMode.MadnessSolo &&
            updated.KillCount <= run.Config.KillRewardLimit &&
            !string.IsNullOrWhiteSpace(run.Config.KillRewardItemCode))
        {
            try
            {
                bool added = await sqlQueryHelper.AddItemToChest(
                    killerCharId,
                    run.Config.KillRewardItemCode,
                    run.Config.KillRewardItemCount,
                    "MadnessKill",
                    0);
                if (!added)
                    Log.Warning("Madness kill Item Chest reward was not added for CharID={CharID}.", killerCharId);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Madness kill reward failed for CharID={CharID}.", killerCharId);
            }
        }

        if (run.Config.Mode == CompetitiveEventMode.LastManStanding)
        {
            var alive = run.Scores.Values.Where(x => x.IsAlive).ToList();
            if (alive.Count == 1)
                run.Completion.TrySetResult(new CompletionSignal(alive[0].Team, alive[0].CharID, "Last survivor"));
        }
    }

    public static Task HandleMobKillAsync(ISession killerSession, int mobId, int worldId, int regionId)
    {
        var run = _active;
        if (run == null || run.Phase != CompetitivePhase.Fighting || run.Config.Mode != CompetitiveEventMode.DefendTower ||
            Volatile.Read(ref run.TowersReady) != 1 ||
            worldId != run.Config.ArenaWorldID || regionId != run.Config.ArenaRegionID ||
            !run.Participants.TryGetValue(killerSession.SessionData.Charid, out var killer))
            return Task.CompletedTask;

        if (mobId == run.Config.Team1TowerMobID && killer.Team == 3)
            run.Completion.TrySetResult(new CompletionSignal(3, killer.CharID, "Team 1 tower destroyed"));
        else if (mobId == run.Config.Team2TowerMobID && killer.Team == 1)
            run.Completion.TrySetResult(new CompletionSignal(1, killer.CharID, "Team 2 tower destroyed"));
        return Task.CompletedTask;
    }

    public static async Task RunAsync(CompetitiveEventRuntimeConfig config, long runId, string startedBy, CancellationToken cancellationToken)
    {
        await StateLock.WaitAsync(cancellationToken);
        ActiveCompetitiveRun run;
        try
        {
            if (_active != null)
                throw new InvalidOperationException("Another competitive event is already active.");
            await RegionControlService.EnsureEventProfileAsync(
                config.EventCode,
                config.ArenaWorldID,
                config.ArenaRegionID,
                teams: config.Mode == CompetitiveEventMode.DefendTower,
                allowParty: !config.RequireNoParty);
            var now = DateTime.UtcNow;
            run = new ActiveCompetitiveRun(runId, config, startedBy, now, now.AddSeconds(config.RegistrationSeconds));
            _active = run;
        }
        finally
        {
            StateLock.Release();
        }

        try
        {
            run.RoundID = await CreateAutoEventRoundAsync(runId, config.DisplayName);
            await ResetRoundStorageAsync(run);
            await SetControlAsync(run, true, run.RegistrationStartsAtUtc, run.RegistrationEndsAtUtc);
            var registrationKey = config.RequireNoParty
                ? "Competitive.RegistrationOpenNoParty"
                : "Competitive.RegistrationOpen";
            await BroadcastNoticeAsync(
                PlayerLanguage.Get(
                    registrationKey,
                    config.DisplayName,
                    PlayerLanguage.FormatDurationSeconds(config.RegistrationSeconds)),
                NoticeType.NOTICE);
            await Task.Delay(TimeSpan.FromSeconds(config.RegistrationSeconds), cancellationToken);

            await StateLock.WaitAsync(cancellationToken);
            try
            {
                if (!ReferenceEquals(run, _active) || run.Phase != CompetitivePhase.Registration)
                    throw new InvalidOperationException("Competitive event registration state changed unexpectedly.");
                run.Phase = CompetitivePhase.Preparing;
            }
            finally
            {
                StateLock.Release();
            }
            await SetControlAsync(run, false, run.RegistrationStartsAtUtc, run.RegistrationEndsAtUtc);
            await RemoveIneligiblePlayersAsync(run);
            if (run.Participants.Count < config.MinPlayers)
            {
                await BroadcastNoticeAsync(PlayerLanguage.Get("Competitive.CancelledNotEnoughPlayers", config.DisplayName, config.MinPlayers), NoticeType.WARNING);
                await CompleteRoundAsync(run, null, null, "Cancelled");
                return;
            }

            if (config.Mode == CompetitiveEventMode.DefendTower)
                await RebalanceTowerTeamsAsync(run);

            await SetRegionAttackAsync(run, false);
            await TeleportParticipantsAsync(run, toArena: true);
            await RemoveUnteleportedParticipantsAsync(run);
            var hasEnoughTeams = config.Mode != CompetitiveEventMode.DefendTower ||
                                 (run.Participants.Values.Any(x => x.Team == 1) &&
                                  run.Participants.Values.Any(x => x.Team == 3));
            if (run.Participants.Count < config.MinPlayers || !hasEnoughTeams)
            {
                await BroadcastNoticeAsync(PlayerLanguage.Get("Competitive.CancelledNotEnoughPlayers", config.DisplayName, config.MinPlayers), NoticeType.WARNING);
                await CompleteRoundAsync(run, null, null, "Cancelled");
                return;
            }
            await InitializeScoresAsync(run);
            await ShowScoreboardAsync(run);
            if (config.Mode == CompetitiveEventMode.DefendTower)
                await QueueTowerProtectionAsync(run, enabled: true, cancellationToken);

            if (config.PrepareSeconds > 0)
            {
                await DatabaseCommands.SetRegionTimerAsync(config.ArenaRegionID, config.PrepareSeconds);
                await BroadcastNoticeAsync(PlayerLanguage.Get("Competitive.Prepare", config.DisplayName, config.PrepareSeconds), NoticeType.NOTICE);
                await Task.Delay(TimeSpan.FromSeconds(config.PrepareSeconds), cancellationToken);
            }

            if (config.Mode != CompetitiveEventMode.DefendTower)
                await QueueFreeForAllAsync(run, enabled: true, cancellationToken);
            await SetRegionAttackAsync(run, true);
            await DatabaseCommands.SetRegionTimerAsync(config.ArenaRegionID, config.FightSeconds);
            run.RegionTimerStarted = true;
            run.FightStartsAtUtc = DateTime.UtcNow;
            run.FightEndsAtUtc = run.FightStartsAtUtc.Value.AddSeconds(config.FightSeconds);
            run.Phase = CompetitivePhase.Fighting;
            await SetControlAsync(run, true, run.FightStartsAtUtc.Value, run.FightEndsAtUtc.Value);
            await BroadcastNoticeAsync(StartMessage(config), NoticeType.NOTICE);

            run.SpawnTask = StartSpawnSequenceAsync(run, cancellationToken);
            var timeout = Task.Delay(TimeSpan.FromSeconds(config.FightSeconds), cancellationToken);
            Task finished;
            while (true)
            {
                var participantCheck = Task.Delay(TimeSpan.FromSeconds(1), cancellationToken);
                finished = await Task.WhenAny(timeout, run.Completion.Task, participantCheck);
                if (finished != participantCheck)
                    break;
                if (config.Mode == CompetitiveEventMode.LastManStanding)
                    await EliminateDisconnectedLmsPlayersAsync(run);
                else if (config.Mode == CompetitiveEventMode.DefendTower)
                    await RemoveDisconnectedTowerPlayersAsync(run);
            }
            cancellationToken.ThrowIfCancellationRequested();
            run.Phase = CompetitivePhase.Finishing;
            if (config.Mode != CompetitiveEventMode.DefendTower)
                await QueueFreeForAllAsync(run, enabled: false, cancellationToken);
            await SetRegionAttackAsync(run, false);
            await SetControlAsync(run, false, run.FightStartsAtUtc.Value, DateTime.UtcNow);
            var signal = finished == run.Completion.Task ? await run.Completion.Task : null;
            if (config.Mode == CompetitiveEventMode.DefendTower && Volatile.Read(ref run.TowersReady) != 1)
            {
                await BroadcastNoticeAsync(
                    PlayerLanguage.Get("Competitive.TowersNotConfirmed", config.DisplayName),
                    NoticeType.WARNING);
                await CompleteRoundAsync(run, null, null, "Cancelled");
                return;
            }
            await FinishRoundAsync(run, signal);
        }
        catch (OperationCanceledException)
        {
            run.PendingTerminalStatus = "Stopped";
            throw;
        }
        catch
        {
            run.PendingTerminalStatus = "Failed";
            throw;
        }
        finally
        {
            try
            {
                await CleanupAsync(run);
            }
            catch (Exception ex)
            {
                Log.Error(ex, "{EventCode} cleanup failed. The active event state will still be released.", run.Config.EventCode);
            }
            finally
            {
                await StateLock.WaitAsync();
                try
                {
                    if (ReferenceEquals(_active, run))
                        _active = null;
                }
                finally
                {
                    StateLock.Release();
                }
            }
        }
    }

    private static async Task FinishRoundAsync(ActiveCompetitiveRun run, CompletionSignal? signal)
    {
        if (run.Config.Mode == CompetitiveEventMode.MadnessSolo)
        {
            var ranking = run.Scores.Values.OrderByDescending(x => x.KillCount).ThenBy(x => x.CharID).Take(3).ToList();
            if (ranking.Count == 0 || ranking[0].KillCount <= 0)
            {
                await TryBroadcastNoticeAsync(PlayerLanguage.Get("Competitive.NoWinner", run.Config.DisplayName), NoticeType.WARNING);
                await CompleteRoundAsync(run, null, null, "TimedOut");
                return;
            }
            for (var i = 0; i < ranking.Count; i++)
            {
                run.RewardedPlayerCount += await RewardPlayersAsync(run, new[] { ranking[i].CharID }, i + 1);
                await TryBroadcastNoticeAsync(
                    PlayerLanguage.Get(
                        "Competitive.Rank",
                        run.Config.DisplayName,
                        i + 1,
                        ranking[i].CharName,
                        ranking[i].KillCount),
                    NoticeType.NOTICE);
            }
            await LogWinAsync(run, ranking[0].CharID, ranking[0].CharName, null);
            await InsertAutoEventWinnerLogAsync(
                run,
                run.Participants[ranking[0].CharID],
                $"Kills={ranking[0].KillCount}",
                $"Rewarded placements: {run.RewardedPlayerCount}");
            await CompleteRoundAsync(run, ranking[0].CharID, ranking[0].CharName, "Won");
            return;
        }

        if (run.Config.Mode == CompetitiveEventMode.LastManStanding)
        {
            if (signal == null || signal.WinnerCharID == null)
            {
                await TryBroadcastNoticeAsync(PlayerLanguage.Get("Competitive.MultipleSurvivors", run.Config.DisplayName), NoticeType.WARNING);
                await CompleteRoundAsync(run, null, null, "TimedOut");
                return;
            }
            var winner = run.Scores[signal.WinnerCharID.Value];
            var winnerRewards = await RewardPlayersAsync(run, new[] { winner.CharID }, 1);
            var otherRewards = await RewardPlayersAsync(run, run.Participants.Keys.Where(x => x != winner.CharID), 2);
            await LogWinAsync(run, winner.CharID, winner.CharName, null);
            await InsertAutoEventWinnerLogAsync(
                run,
                run.Participants[winner.CharID],
                signal.Reason,
                $"Winner: {winnerRewards}; Others: {otherRewards}");
            await CompleteRoundAsync(run, winner.CharID, winner.CharName, "Won");
            await TryBroadcastNoticeAsync(PlayerLanguage.Get("Competitive.LastSurvivorWinner", run.Config.DisplayName, winner.CharName), NoticeType.NOTICE);
            return;
        }

        var winningTeam = signal?.WinningTeam;
        if (winningTeam == null)
        {
            var team1Kills = run.Scores.Values.Where(x => x.Team == 1).Sum(x => x.KillCount);
            var team2Kills = run.Scores.Values.Where(x => x.Team == 3).Sum(x => x.KillCount);
            if (team1Kills != team2Kills)
                winningTeam = team1Kills > team2Kills ? 1 : 3;
        }
        if (winningTeam == null)
        {
            await TryBroadcastNoticeAsync(PlayerLanguage.Get("Competitive.Draw", run.Config.DisplayName), NoticeType.WARNING);
            await CompleteRoundAsync(run, null, null, "Draw");
            return;
        }
        var winners = run.Participants.Values.Where(x => x.Team == winningTeam).Select(x => x.CharID).ToArray();
        var losers = run.Participants.Values.Where(x => x.Team != winningTeam).Select(x => x.CharID).ToArray();
        var rewardedWinners = await RewardPlayersAsync(run, winners, 1);
        var rewardedLosers = await RewardPlayersAsync(run, losers, 2);
        var representative = run.Participants[winners[0]];
        await LogWinAsync(run, representative.CharID, representative.CharName, winningTeam);
        await InsertAutoEventWinnerLogAsync(
            run,
            representative,
            signal?.Reason ?? "Highest score",
            $"Winners: {rewardedWinners}; Others: {rewardedLosers}");
        await CompleteRoundAsync(run, representative.CharID, representative.CharName, "Won");
        var winningTeamName = PlayerLanguage.Get(winningTeam == 1 ? "Competitive.Team.Red" : "Competitive.Team.Blue");
        var reason = FormatCompletionReason(signal?.Reason);
        await TryBroadcastNoticeAsync(PlayerLanguage.Get("Competitive.TeamWinner", run.Config.DisplayName, winningTeamName, reason), NoticeType.NOTICE);
    }

    private static async Task StartSpawnSequenceAsync(ActiveCompetitiveRun run, CancellationToken cancellationToken)
    {
        try
        {
            if (run.Config.Mode == CompetitiveEventMode.LastManStanding)
                return;
            if (run.Config.MobSpawnDelaySeconds > 0)
                await Task.Delay(TimeSpan.FromSeconds(run.Config.MobSpawnDelaySeconds), cancellationToken);
            if (run.Phase != CompetitivePhase.Fighting)
                return;

            if (run.Config.Mode == CompetitiveEventMode.MadnessSolo)
            {
                for (var i = 0; i < run.Config.MadnessMobCount; i++)
                    await QueueSpawnMobAsync(run.Config.MadnessMobID, run.Config.ArenaWorldID, run.Config.ArenaRegionID,
                        run.Config.MadnessMobX, run.Config.MadnessMobY, run.Config.MadnessMobZ, 5);
                if (run.Config.MadnessMobCount > 0)
                    await TryBroadcastNoticeAsync(PlayerLanguage.Get("Competitive.ArenaMobsSpawned", run.Config.DisplayName), NoticeType.NOTICE);
            }
            else
            {
                await QueueTowerSpawnProcedureAsync(run.Config.Team1TowerMobID, run.Config.ArenaWorldID,
                    run.Config.ArenaRegionID, run.Config.Team1TowerX, run.Config.Team1TowerY,
                    run.Config.Team1TowerZ, TowerSpawnRadius, cancellationToken);
                await QueueTowerSpawnProcedureAsync(run.Config.Team2TowerMobID, run.Config.ArenaWorldID,
                    run.Config.ArenaRegionID, run.Config.Team2TowerX, run.Config.Team2TowerY,
                    run.Config.Team2TowerZ, TowerSpawnRadius, cancellationToken);
                Volatile.Write(ref run.TowersReady, 1);
                await TryBroadcastNoticeAsync(PlayerLanguage.Get("Competitive.TowersSpawned", run.Config.DisplayName), NoticeType.NOTICE);
            }
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            // Normal Stop Now cancellation; RunAsync owns the cleanup path.
        }
        catch (Exception ex)
        {
            Log.Error(ex, "{EventCode} arena spawn sequence failed.", run.Config.EventCode);
            if (run.Config.Mode == CompetitiveEventMode.DefendTower)
                run.Completion.TrySetResult(new CompletionSignal(null, null, "Tower spawn failed"));
            else
                run.Completion.TrySetException(new InvalidOperationException(
                    $"{run.Config.DisplayName} arena spawn failed.", ex));
        }
    }

    private static async Task EliminateDisconnectedLmsPlayersAsync(ActiveCompetitiveRun run)
    {
        foreach (var score in run.Scores.Values.Where(x => x.IsAlive).ToArray())
        {
            if (FindOnlineCharacter(score.CharID) != null)
                continue;
            var eliminated = score with { IsAlive = false };
            run.Scores[score.CharID] = eliminated;
            await UpsertScoreAsync(run, eliminated);
        }

        var alive = run.Scores.Values.Where(x => x.IsAlive).ToArray();
        if (alive.Length == 1)
            run.Completion.TrySetResult(new CompletionSignal(alive[0].Team, alive[0].CharID, "Last online survivor"));
        else if (alive.Length == 0)
            run.Completion.TrySetResult(new CompletionSignal(null, null, "No survivors"));
    }

    private static async Task RemoveDisconnectedTowerPlayersAsync(ActiveCompetitiveRun run)
    {
        var disconnected = run.Participants.Values
            .Where(x => FindOnlineCharacter(x.CharID) == null)
            .ToArray();
        if (disconnected.Length == 0)
            return;

        foreach (var participant in disconnected)
        {
            run.Participants.TryRemove(participant.CharID, out _);
            run.Scores.TryRemove(participant.CharID, out _);
            run.DeadCharacters.TryRemove(participant.CharID, out _);
            run.LastDeathUtc.TryRemove(participant.CharID, out _);
        }

        var charIds = disconnected.Select(x => x.CharID).ToArray();
        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.ExecuteAsync(
                "DELETE FROM dbo.Event_CurrentTeams WHERE EventName=@EventCode AND CharID IN @CharIDs;",
                new { run.Config.EventCode, CharIDs = charIds });
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "{EventCode} failed to remove disconnected tower players from persistent teams.", run.Config.EventCode);
        }

        foreach (var participant in disconnected)
            Log.Information("{EventCode} removed disconnected participant CharID={CharID}.", run.Config.EventCode, participant.CharID);

        await BroadcastScoreAsync(run);

        var redPlayers = run.Participants.Values.Count(x => x.Team == 1);
        var bluePlayers = run.Participants.Values.Count(x => x.Team == 3);
        if (redPlayers == 0 && bluePlayers == 0)
            run.Completion.TrySetResult(new CompletionSignal(null, null, "All participants disconnected"));
        else if (redPlayers == 0)
            run.Completion.TrySetResult(new CompletionSignal(3, null, "Red team disconnected"));
        else if (bluePlayers == 0)
            run.Completion.TrySetResult(new CompletionSignal(1, null, "Blue team disconnected"));
    }

    private static async Task EliminatePlayerAsync(ActiveCompetitiveRun run, int charId)
    {
        await Task.Delay(KillReviveDelay);
        if (!ReferenceEquals(run, _active) ||
            run.Phase != CompetitivePhase.Fighting ||
            !run.Scores.TryGetValue(charId, out var score) ||
            score.IsAlive)
        {
            return;
        }

        await QueueGetUpAsync(charId);
        await Task.Delay(TimeSpan.FromSeconds(1));

        if (!ReferenceEquals(run, _active) || run.Phase != CompetitivePhase.Fighting)
            return;

        var session = FindOnlineCharacter(charId);
        if (session == null)
            return;
        try
        {
            await ApplySuitAsync(session, 0);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "{EventCode} suit clear failed for eliminated CharID={CharID}", run.Config.EventCode, charId);
        }
        await TeleportFreezeService.TeleportToPositionAsync(session, ReturnWorldId, ReturnRegionId, ReturnPosX, ReturnPosY, ReturnPosZ, 0);
        run.ArenaParticipants.TryRemove(charId, out _);
    }

    private static async Task EliminatePlayerSafelyAsync(ActiveCompetitiveRun run, int charId)
    {
        try
        {
            await EliminatePlayerAsync(run, charId);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "{EventCode} elimination cleanup failed for CharID={CharID}", run.Config.EventCode, charId);
        }
    }

    private static async Task RemoveIneligiblePlayersAsync(ActiveCompetitiveRun run)
    {
        var removedCharIds = new List<int>();
        foreach (var participant in run.Participants.Values.ToArray())
        {
            var session = FindOnlineCharacter(participant.CharID);
            var disqualified = session == null || (run.Config.RequireNoParty && await IsInPartyAsync(participant.CharID));
            if (!disqualified)
                continue;
            run.Participants.TryRemove(participant.CharID, out _);
            run.Scores.TryRemove(participant.CharID, out _);
            removedCharIds.Add(participant.CharID);
            if (session != null)
                await TrySendPersonalNoticeAsync(session, PlayerLanguage.Get("Competitive.CancelledJoinedParty", run.Config.DisplayName));
        }
        if (removedCharIds.Count == 0)
            return;
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.ExecuteAsync(@"
DELETE FROM dbo.Event_CurrentTeams WHERE EventName=@EventCode AND CharID IN @CharIDs;
DELETE FROM Events.dbo.Event_RegPlayers WHERE EventID=@EventID AND EventStartUtc=@EventStartUtc AND CharID IN @CharIDs;",
            new { run.Config.EventCode, run.Config.EventID, EventStartUtc = run.RegistrationStartsAtUtc, CharIDs = removedCharIds });
    }

    private static async Task RebalanceTowerTeamsAsync(ActiveCompetitiveRun run)
    {
        var ordered = run.Participants.Values.OrderBy(_ => Random.Shared.Next()).ThenBy(x => x.RegistrationOrder).ToArray();
        for (var i = 0; i < ordered.Length; i++)
        {
            var updated = ordered[i] with { Team = i % 2 == 0 ? 1 : 3 };
            run.Participants[updated.CharID] = updated;
            run.Scores[updated.CharID] = new CompetitiveScore(updated.CharID, updated.CharName, updated.Team, 0, true);
            await PersistTeamAsync(run, updated);
        }
    }

    private static async Task InitializeScoresAsync(ActiveCompetitiveRun run)
    {
        foreach (var participant in run.Participants.Values)
        {
            var score = new CompetitiveScore(participant.CharID, participant.CharName, participant.Team, 0, true);
            run.Scores[participant.CharID] = score;
            await UpsertScoreAsync(run, score);
        }
    }

    private static async Task ShowScoreboardAsync(ActiveCompetitiveRun run)
    {
        if (run.Config.Mode == CompetitiveEventMode.DefendTower)
        {
            var scoreboardTitle = PlayerLanguage.Get("Competitive.TeamScoreboardTitle", run.Config.DisplayName);
            ActionManager.CreatedTeamKillCounterWorldID[run.Config.ArenaWorldID] = scoreboardTitle;
            foreach (var entry in ActionManager.TeamKillCounterKillList.Where(x => x.Value.WorldID == run.Config.ArenaWorldID).ToArray())
                ActionManager.TeamKillCounterKillList.TryRemove(entry.Key, out _);
            var packet = new Packet(0x189A, false, false);
            packet.WriteUInt8(1);
            packet.WriteAscii(scoreboardTitle);
            await ServerManager.BroadcastPacketbyWorldID(run.Config.ArenaWorldID, packet);
        }
        else
        {
            var scoreboardTitle = PlayerLanguage.Get("Competitive.PlayerScoreboardTitle", run.Config.DisplayName);
            ActionManager.CreatedKillCounterWorldID[run.Config.ArenaWorldID] = scoreboardTitle;
            ActionManager.CreatedFullKillCounterWorldID[run.Config.ArenaWorldID] = 1;
            foreach (var entry in ActionManager.KillCounterKillList.Where(x => x.Value.WorldID == run.Config.ArenaWorldID).ToArray())
                ActionManager.KillCounterKillList.TryRemove(entry.Key, out _);
            var packet = new Packet(0x207A, false, false);
            packet.WriteUInt8(1);
            packet.WriteAscii(scoreboardTitle);
            await ServerManager.BroadcastPacketbyWorldID(run.Config.ArenaWorldID, packet);
        }
        await BroadcastScoreAsync(run);
    }

    private static async Task BroadcastScoreAsync(ActiveCompetitiveRun run)
    {
        var scores = run.Scores.Values.OrderByDescending(x => x.KillCount).ThenByDescending(x => x.IsAlive).ThenBy(x => x.CharID).ToList();
        if (run.Config.Mode == CompetitiveEventMode.DefendTower)
        {
            var redTeamKills = scores.Where(x => x.Team == 1).Sum(x => x.KillCount);
            var blueTeamKills = scores.Where(x => x.Team == 3).Sum(x => x.KillCount);
            foreach (var entry in ActionManager.TeamKillCounterKillList.Where(x => x.Value.WorldID == run.Config.ArenaWorldID).ToArray())
                ActionManager.TeamKillCounterKillList.TryRemove(entry.Key, out _);
            foreach (var score in scores)
                ActionManager.TeamKillCounterKillList[score.CharName] = new SCreatedTeamKillCounterKillList
                {
                    WorldID = run.Config.ArenaWorldID, CharName16 = score.CharName, Kill = score.KillCount, Team = score.Team
                };
            var packet = new Packet(0x189B);
            packet.WriteUInt8(scores.Count);
            foreach (var score in scores)
            {
                packet.WriteAscii(score.CharName);
                packet.WriteUInt8((byte)score.Team);
                packet.WriteInt32(score.KillCount);
                packet.WriteInt32(redTeamKills);
                packet.WriteInt32(blueTeamKills);
            }
            await ServerManager.BroadcastPacketbyWorldID(run.Config.ArenaWorldID, packet);
            return;
        }

        foreach (var entry in ActionManager.KillCounterKillList.Where(x => x.Value.WorldID == run.Config.ArenaWorldID).ToArray())
            ActionManager.KillCounterKillList.TryRemove(entry.Key, out _);
        foreach (var score in scores)
            ActionManager.KillCounterKillList[score.CharName] = new SCreatedKillCounterKillList
            {
                WorldID = run.Config.ArenaWorldID, CharName16 = score.CharName, Kill = score.KillCount
            };
        var individual = new Packet(0x207C);
        individual.WriteUInt8(scores.Count);
        foreach (var score in scores)
        {
            individual.WriteAscii(score.CharName);
            individual.WriteInt32(score.KillCount);
        }
        await ServerManager.BroadcastPacketbyWorldID(run.Config.ArenaWorldID, individual);
    }

    private static async Task HideScoreboardAsync(ActiveCompetitiveRun run)
    {
        if (run.Config.Mode == CompetitiveEventMode.DefendTower)
        {
            ActionManager.CreatedTeamKillCounterWorldID.TryRemove(run.Config.ArenaWorldID, out _);
            foreach (var entry in ActionManager.TeamKillCounterKillList.Where(x => x.Value.WorldID == run.Config.ArenaWorldID).ToArray())
                ActionManager.TeamKillCounterKillList.TryRemove(entry.Key, out _);
            var packet = new Packet(0x189A, false, false);
            packet.WriteUInt8(0);
            await ServerManager.BroadcastPacketbyWorldID(run.Config.ArenaWorldID, packet);
        }
        else
        {
            ActionManager.CreatedKillCounterWorldID.TryRemove(run.Config.ArenaWorldID, out _);
            ActionManager.CreatedFullKillCounterWorldID.TryRemove(run.Config.ArenaWorldID, out _);
            foreach (var entry in ActionManager.KillCounterKillList.Where(x => x.Value.WorldID == run.Config.ArenaWorldID).ToArray())
                ActionManager.KillCounterKillList.TryRemove(entry.Key, out _);
            var packet = new Packet(0x207A, false, false);
            packet.WriteUInt8(0);
            packet.WriteAscii(string.Empty);
            await ServerManager.BroadcastPacketbyWorldID(run.Config.ArenaWorldID, packet);
        }
    }

    private static async Task TeleportParticipantsAsync(ActiveCompetitiveRun run, bool toArena)
    {
        if (!toArena)
        {
            foreach (var charId in run.DeadCharacters.Keys.Concat(run.Scores.Values.Where(x => !x.IsAlive).Select(x => x.CharID)).Distinct())
                await QueueGetUpAsync(charId);
            await Task.Delay(TimeSpan.FromSeconds(1));
        }
        var participants = toArena
            ? run.Participants.Values.AsEnumerable()
            : run.ArenaParticipants.Values.AsEnumerable();
        foreach (var participant in participants)
        {
            var session = FindOnlineCharacter(participant.CharID);
            if (session == null)
                continue;
            try
            {
                if (toArena)
                {
                    // Skip players wearing a Job suit (Trader=1, Thief=2, Hunter=3).
                    byte jobType = session.SessionData.JobType;
                    if (jobType >= 1 && jobType <= 3)
                    {
                        try
                        {
                            var notice = new Packet(0x168A);
                            notice.WriteUInt8(NoticeType.WARNING);
                            notice.WriteUnicode(PlayerLanguage.Get("Event.JobSuitTeleportBlocked"));
                            await session.SendToClient(notice);
                        }
                        catch { /* best-effort */ }
                        Log.Information("{EventCode} skipped teleport for CharID={CharID} — wearing job suit ({JobType})", run.Config.EventCode, participant.CharID, jobType);
                        continue;
                    }

                    var x = run.Config.Mode == CompetitiveEventMode.DefendTower ? (participant.Team == 1 ? run.Config.Team1X : run.Config.Team2X) : run.Config.ArenaX;
                    var y = run.Config.Mode == CompetitiveEventMode.DefendTower ? (participant.Team == 1 ? run.Config.Team1Y : run.Config.Team2Y) : run.Config.ArenaY;
                    var z = run.Config.Mode == CompetitiveEventMode.DefendTower ? (participant.Team == 1 ? run.Config.Team1Z : run.Config.Team2Z) : run.Config.ArenaZ;
                    await TeleportFreezeService.TeleportToPositionAsync(session, run.Config.ArenaWorldID, run.Config.ArenaRegionID, x, y, z, run.Config.PrepareSeconds);
                    run.ArenaParticipants[participant.CharID] = participant;
                    try
                    {
                        await ApplySuitAsync(session, participant.Team);
                    }
                    catch (Exception ex)
                    {
                        Log.Warning(ex, "{EventCode} suit apply failed for CharID={CharID}", run.Config.EventCode, participant.CharID);
                    }
                }
                else
                {
                    try
                    {
                        await ApplySuitAsync(session, 0);
                    }
                    catch (Exception ex)
                    {
                        Log.Warning(ex, "{EventCode} suit clear failed for CharID={CharID}", run.Config.EventCode, participant.CharID);
                    }
                    await TeleportFreezeService.TeleportToPositionAsync(session, ReturnWorldId, ReturnRegionId, ReturnPosX, ReturnPosY, ReturnPosZ, 0);
                    run.ArenaParticipants.TryRemove(participant.CharID, out _);
                }
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "{EventCode} teleport failed for CharID={CharID}, ToArena={ToArena}", run.Config.EventCode, participant.CharID, toArena);
            }
        }
    }

    private static async Task RemoveUnteleportedParticipantsAsync(ActiveCompetitiveRun run)
    {
        var removedCharIds = run.Participants.Keys
            .Where(charId => !run.ArenaParticipants.ContainsKey(charId))
            .ToArray();
        foreach (var charId in removedCharIds)
        {
            run.Participants.TryRemove(charId, out _);
            run.Scores.TryRemove(charId, out _);
        }

        if (removedCharIds.Length == 0)
            return;

        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.ExecuteAsync(
                "DELETE FROM dbo.Event_CurrentTeams WHERE EventName=@EventCode AND CharID IN @CharIDs;",
                new { run.Config.EventCode, CharIDs = removedCharIds });
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "{EventCode} failed to prune players that could not enter the arena.", run.Config.EventCode);
        }
    }

    private static async Task CleanupAsync(ActiveCompetitiveRun run)
    {
        run.Phase = CompetitivePhase.Cleanup;
        if (!run.RoundFinalized)
        {
            try { await CompleteRoundAsync(run, null, null, run.PendingTerminalStatus ?? "Failed"); }
            catch (Exception ex) { Log.Warning(ex, "Competitive round cleanup failed."); }
        }
        try { await SetControlAsync(run, false, run.RegistrationStartsAtUtc, DateTime.UtcNow); } catch (Exception ex) { Log.Warning(ex, "Competitive control cleanup failed."); }
        try { await DatabaseCommands.SetRegionTimerAsync(run.Config.ArenaRegionID, 0); } catch (Exception ex) { Log.Warning(ex, "Competitive timer cleanup failed."); }
        try { await HideScoreboardAsync(run); } catch (Exception ex) { Log.Warning(ex, "Competitive scoreboard cleanup failed."); }
        if (run.Config.Mode == CompetitiveEventMode.DefendTower)
        {
            Volatile.Write(ref run.TowersReady, 0);
            try { await QueueTowerProtectionAsync(run, enabled: false); } catch (Exception ex) { Log.Warning(ex, "Tower protection cleanup failed."); }
            try { await QueueRemoveMobAsync(run.Config.ArenaWorldID, run.Config.Team1TowerMobID); } catch (Exception ex) { Log.Warning(ex, "Team 1 tower cleanup failed."); }
            try { await QueueRemoveMobAsync(run.Config.ArenaWorldID, run.Config.Team2TowerMobID); } catch (Exception ex) { Log.Warning(ex, "Team 2 tower cleanup failed."); }
        }
        else
        {
            try { await QueueFreeForAllAsync(run, enabled: false); } catch (Exception ex) { Log.Warning(ex, "Free-for-all cleanup failed."); }
            if (run.Config.Mode == CompetitiveEventMode.MadnessSolo && run.Config.MadnessMobID > 0)
            {
                try { await QueueRemoveMobAsync(run.Config.ArenaWorldID, run.Config.MadnessMobID); }
                catch (Exception ex) { Log.Warning(ex, "Madness arena mob cleanup failed."); }
            }
        }
        try { await TeleportParticipantsAsync(run, toArena: false); } catch (Exception ex) { Log.Warning(ex, "Competitive player return failed."); }
        RestoreRegionAttack(run);
        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await connection.ExecuteAsync(@"
DELETE FROM dbo.Event_CurrentTeams WHERE EventName=@EventCode;
DELETE FROM Events.dbo._CompetitiveEventKillLedger WHERE RunID=@RunID;",
                new { run.Config.EventCode, run.RunID });
        }
        catch (Exception ex) { Log.Warning(ex, "Competitive database cleanup failed."); }
    }

    private static async Task<int> RewardPlayersAsync(ActiveCompetitiveRun run, IEnumerable<int> charIds, int placement)
    {
        var rewards = (await LoadRewardsAsync(run.Config.EventCode, placement)).ToArray();
        if (rewards.Length == 0)
            return 0;
        var rewarded = 0;
        foreach (var charId in charIds.Distinct())
        {
            if (!run.Participants.TryGetValue(charId, out var participant))
                continue;
            var session = FindOnlineCharacter(charId);
            if (run.Config.Mode == CompetitiveEventMode.DefendTower && session == null)
            {
                Log.Information("{EventCode} skipped reward for disconnected participant CharID={CharID}.",
                    run.Config.EventCode, charId);
                continue;
            }
            foreach (var reward in rewards)
            {
                try
                {
                    await ApplyRewardAsync(session, participant, reward, run.Config.EventCode);
                }
                catch (Exception ex)
                {
                    Log.Warning(ex, "{EventCode} reward failed for CharID={CharID}, RewardType={RewardType}",
                        run.Config.EventCode,
                        participant.CharID,
                        reward.RewardType);
                }
            }
            rewarded++;
        }
        return rewarded;
    }

    private static async Task ApplyRewardAsync(ISession? session, CompetitiveParticipant participant, CompetitiveReward reward, string source)
    {
        if (reward.RewardType.Equals("ItemChest", StringComparison.OrdinalIgnoreCase))
        {
            bool added = false;
            if (!string.IsNullOrWhiteSpace(reward.ItemCodeName128))
                added = await sqlQueryHelper.AddItemToChest(participant.CharID, reward.ItemCodeName128, Math.Clamp(reward.ItemCount, 1, 10000), source, Math.Clamp(reward.Plus, 0, 20));
            else if (reward.ItemID.GetValueOrDefault() > 0)
                added = await sqlQueryHelper.AddItemToChest2(participant.CharID, reward.ItemID!.Value, Math.Clamp(reward.ItemCount, 1, 10000), source, Math.Clamp(reward.Plus, 0, 20));
            if (!added)
                Log.Error("Competitive Item Chest reward failed. CharID={CharID}, ItemCode={ItemCode}, ItemID={ItemID}, Source={Source}.", participant.CharID, reward.ItemCodeName128, reward.ItemID, source);
            return;
        }
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        if (reward.RewardType.Equals("Gold", StringComparison.OrdinalIgnoreCase))
        {
            if (reward.Amount <= 0)
                return;
            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
            await connection.ExecuteAsync($"UPDATE {shardDb}.._Char SET RemainGold=RemainGold+@Amount WHERE CharID=@CharID",
                new { reward.Amount, participant.CharID });
            return;
        }
        var column = reward.RewardType.ToUpperInvariant() switch
        {
            "SILKOWN" => "silk_own", "SILKGIFT" => "silk_gift", "SILKPOINT" => "silk_point", _ => string.Empty
        };
        var jid = session?.SessionData.JID ?? participant.JID;
        if (column.Length == 0 || jid <= 0 || reward.Amount <= 0)
            return;
        var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
        await connection.ExecuteAsync($@"
IF NOT EXISTS(SELECT 1 FROM {accountDb}..SK_Silk WITH(UPDLOCK,HOLDLOCK) WHERE JID=@JID)
    INSERT {accountDb}..SK_Silk(JID,silk_own,silk_gift,silk_point) VALUES(@JID,0,0,0);
UPDATE {accountDb}..SK_Silk SET {column}={column}+@Amount WHERE JID=@JID;",
            new { JID = jid, Amount = (int)Math.Clamp(reward.Amount, 0, int.MaxValue) });
    }

    private static async Task PersistRegistrationAsync(ActiveCompetitiveRun run, CompetitiveParticipant participant)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync();
        await connection.ExecuteAsync(@"
IF NOT EXISTS(SELECT 1 FROM Events.dbo.Event_RegPlayers WITH(UPDLOCK,HOLDLOCK)
              WHERE EventID=@EventID AND EventStartUtc=@EventStartUtc AND CharID=@CharID)
    INSERT Events.dbo.Event_RegPlayers(EventID,EventCode,EventStartUtc,CharID,CharName,Amount,Currency)
    VALUES(@EventID,@EventCode,@EventStartUtc,@CharID,@CharName,0,N'silk');",
            new { run.Config.EventID, run.Config.EventCode, EventStartUtc = run.RegistrationStartsAtUtc, participant.CharID, participant.CharName }, transaction);
        await PersistTeamAsync(run, participant, connection, transaction);
        await transaction.CommitAsync();
    }

    private static async Task PersistTeamAsync(ActiveCompetitiveRun run, CompetitiveParticipant participant,
        SqlConnection? connection = null, SqlTransaction? transaction = null)
    {
        var ownsConnection = connection == null;
        connection ??= new SqlConnection(Program.Connectionstring);
        if (ownsConnection)
            await connection.OpenAsync();
        await connection.ExecuteAsync(@"
MERGE dbo.Event_CurrentTeams AS target USING(SELECT @CharID CharID) source ON target.CharID=source.CharID
WHEN MATCHED THEN UPDATE SET CharName=@CharName,Team=@Team,EventName=@EventCode
WHEN NOT MATCHED THEN INSERT(CharID,CharName,Team,EventName) VALUES(@CharID,@CharName,@Team,@EventCode);",
            new { participant.CharID, CharName = Trim(participant.CharName, 25), participant.Team, run.Config.EventCode }, transaction);
        if (ownsConnection)
            await connection.DisposeAsync();
    }

    private static async Task UpsertScoreAsync(ActiveCompetitiveRun run, CompetitiveScore score)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.ExecuteAsync(@"
MERGE Events.dbo._CompetitiveEventScore target
USING(SELECT @RunID RunID,@CharID CharID) source ON target.RunID=source.RunID AND target.CharID=source.CharID
WHEN MATCHED THEN UPDATE SET CharName=@CharName,Team=@Team,KillCount=@KillCount,IsAlive=@IsAlive,UpdatedAtUtc=SYSUTCDATETIME()
WHEN NOT MATCHED THEN INSERT(RunID,EventCode,CharID,CharName,Team,KillCount,IsAlive)
VALUES(@RunID,@EventCode,@CharID,@CharName,@Team,@KillCount,@IsAlive);",
            new { run.RunID, run.Config.EventCode, score.CharID, score.CharName, score.Team, score.KillCount, score.IsAlive });
    }

    private static async Task UpsertPairKillsAsync(long runId, int killerCharId, int deadCharId, int kills)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.ExecuteAsync(@"
MERGE Events.dbo._CompetitiveEventKillLedger target
USING(SELECT @RunID RunID,@KillerCharID KillerCharID,@DeadCharID DeadCharID) source
ON target.RunID=source.RunID AND target.KillerCharID=source.KillerCharID AND target.DeadCharID=source.DeadCharID
WHEN MATCHED THEN UPDATE SET KillCount=@Kills,UpdatedAtUtc=SYSUTCDATETIME()
WHEN NOT MATCHED THEN INSERT(RunID,KillerCharID,DeadCharID,KillCount) VALUES(@RunID,@KillerCharID,@DeadCharID,@Kills);",
            new { RunID = runId, KillerCharID = killerCharId, DeadCharID = deadCharId, Kills = kills });
    }

    private static async Task ResetRoundStorageAsync(ActiveCompetitiveRun run)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.ExecuteAsync(@"
DELETE FROM Events.dbo._CompetitiveEventScore WHERE EventCode=@EventCode;
DELETE FROM Events.dbo._CompetitiveEventKillLedger WHERE RunID=@RunID;
DELETE FROM dbo.Event_CurrentTeams WHERE EventName=@EventCode;
DELETE FROM Events.dbo.Event_RegPlayers WHERE EventID=@EventID;",
            new { run.Config.EventCode, run.RunID, run.Config.EventID });
    }

    private static async Task SetControlAsync(ActiveCompetitiveRun run, bool isOpen, DateTime startUtc, DateTime endUtc)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.ExecuteAsync(@"
MERGE Events.dbo.Event_Control target USING(SELECT @EventID EventID) source ON target.EventID=source.EventID
WHEN MATCHED THEN UPDATE SET EventCode=@EventCode,IsOpen=@IsOpen,StartUtc=@StartUtc,EndUtc=@EndUtc,UpdatedAtUtc=SYSUTCDATETIME()
WHEN NOT MATCHED THEN INSERT(EventID,EventCode,IsOpen,StartUtc,EndUtc,UpdatedAtUtc)
VALUES(@EventID,@EventCode,@IsOpen,@StartUtc,@EndUtc,SYSUTCDATETIME());",
            new { run.Config.EventID, run.Config.EventCode, IsOpen = isOpen, StartUtc = startUtc, EndUtc = endUtc });
    }

    private static async Task<long> CreateAutoEventRoundAsync(long runId, string displayName)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        return await connection.ExecuteScalarAsync<long>(@"
INSERT Events.dbo._AutoEventRound(RunID,RoundNo,Prompt,AnswerMasked,Status,StartedAtUtc)
VALUES(@RunID,1,@Prompt,N'competitive-event',N'Running',SYSUTCDATETIME());
SELECT CONVERT(bigint,SCOPE_IDENTITY());", new { RunID = runId, Prompt = displayName });
    }

    private static async Task CompleteRoundAsync(ActiveCompetitiveRun run, int? winnerCharId, string? winnerName, string status)
    {
        if (run.RoundID <= 0) return;
        if (Interlocked.CompareExchange(ref run.RoundFinalizeState, 1, 0) != 0)
            return;
        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.ExecuteAsync(@"
UPDATE Events.dbo._AutoEventRound SET Status=@Status,WinnerCharID=@WinnerCharID,WinnerCharName=@WinnerName,
EndedAtUtc=SYSUTCDATETIME() WHERE RoundID=@RoundID AND EndedAtUtc IS NULL;",
                new { run.RoundID, Status = status, WinnerCharID = winnerCharId, WinnerName = Trim(winnerName, 64) });
        }
        catch
        {
            Interlocked.Exchange(ref run.RoundFinalizeState, 0);
            throw;
        }
    }

    private static async Task LogWinAsync(ActiveCompetitiveRun run, int winnerCharId, string winnerName, int? winnerTeam)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.ExecuteAsync(@"
INSERT Events.dbo.Event_Wins(EventID,EventCode,EventStartUtc,EventEndUtc,WinnerCharID,WinnerCharName,WinnerPartyNo,TotalPot,Currency)
VALUES(@EventID,@EventCode,@StartUtc,SYSUTCDATETIME(),@WinnerCharID,@WinnerName,@WinnerTeam,0,N'configured');",
            new { run.Config.EventID, run.Config.EventCode, StartUtc = run.RegistrationStartsAtUtc, WinnerCharID = winnerCharId, WinnerName = Trim(winnerName, 64), WinnerTeam = winnerTeam });
    }

    private static async Task InsertAutoEventWinnerLogAsync(
        ActiveCompetitiveRun run,
        CompetitiveParticipant winner,
        string answer,
        string rewardSummary)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.ExecuteAsync(@"
IF NOT EXISTS (SELECT 1 FROM Events.dbo._AutoEventWinnerLog WITH (UPDLOCK, HOLDLOCK) WHERE RoundID=@RoundID)
BEGIN
    INSERT Events.dbo._AutoEventWinnerLog
        (RunID,RoundID,EventCode,RoundNo,CharID,CharName,JID,Hwid,ClientIP,Answer,WonAtUtc,RewardSummary)
    VALUES
        (@RunID,@RoundID,@EventCode,1,@CharID,@CharName,@JID,@Hwid,@ClientIP,@Answer,SYSUTCDATETIME(),@RewardSummary);
END;",
            new
            {
                run.RunID,
                run.RoundID,
                run.Config.EventCode,
                winner.CharID,
                CharName = Trim(winner.CharName, 64),
                winner.JID,
                Hwid = Trim(winner.Hwid, 128),
                ClientIP = Trim(winner.ClientIP, 64),
                Answer = Trim(answer, 256),
                RewardSummary = Trim(rewardSummary, 512)
            });
    }

    private static async Task<IEnumerable<CompetitiveReward>> LoadRewardsAsync(string eventCode, int placement)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        return await connection.QueryAsync<CompetitiveReward>(@"
SELECT RewardType,Amount,ItemCodeName128,ItemID,ItemCount,Plus
FROM Events.dbo._CompetitiveEventReward WITH(NOLOCK)
WHERE EventCode=@EventCode AND Placement=@Placement AND IsActive=1 ORDER BY RewardID;",
            new { EventCode = eventCode, Placement = placement });
    }

    private static async Task QueueSpawnMobAsync(
        int mobId,
        int worldId,
        int regionId,
        int x,
        int y,
        int z,
        int radius,
        bool waitForConsumption = false,
        CancellationToken cancellationToken = default)
    {
        if (mobId <= 0) return;
        await using var connection = new SqlConnection(Program.Connectionstring);
        var commandId = await connection.ExecuteScalarAsync<long>(@"
INSERT dbo.Command_GameServerQueue(Action_ID,Data1,Data2,Data3,Data4,Data5,Data6,Data7)
VALUES(2,@MobID,@WorldID,@RegionID,@X,@Y,@Z,@Radius);
SELECT CONVERT(bigint,SCOPE_IDENTITY());",
            new { MobID = mobId, WorldID = worldId, RegionID = regionId, X = x, Y = y, Z = z, Radius = radius });
        if (waitForConsumption)
            await WaitForGameServerCommandAsync(connection, commandId, cancellationToken);
    }

    private static async Task QueueTowerSpawnProcedureAsync(
        int mobId,
        int worldId,
        int regionId,
        int x,
        int y,
        int z,
        int radius,
        CancellationToken cancellationToken)
    {
        if (mobId <= 0)
            throw new InvalidOperationException("Tower spawn requires a valid MobID.");

        await using var connection = new SqlConnection(Program.Connectionstring);
        var command = new CommandDefinition(
            "dbo.NPC_SpawnAtPosition",
            new
            {
                MonsterID = mobId,
                GameWorldID = worldId,
                RegionId = regionId,
                PosX = x,
                PosY = y,
                PosZ = z,
                GenerateRadius = radius
            },
            commandType: CommandType.StoredProcedure,
            commandTimeout: 10,
            cancellationToken: cancellationToken);
        await connection.ExecuteAsync(command);
    }

    private static async Task QueueRemoveMobAsync(int worldId, int mobId)
    {
        if (mobId <= 0) return;
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.ExecuteAsync("INSERT dbo.Command_GameServerQueue(Action_ID,Data1,Data2) VALUES(5,@WorldID,@MobID);",
            new { WorldID = worldId, MobID = mobId });
    }

    private static async Task QueueTowerProtectionAsync(ActiveCompetitiveRun run, bool enabled, CancellationToken cancellationToken = default)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        var commandId = await connection.ExecuteScalarAsync<long>(@"
INSERT dbo.Command_GameServerQueue(Action_ID,Data1,Data2,Data3,Data4,Data5,Data6,Data7)
VALUES(38,@Enabled,@WorldID,@RegionID,@Team1MobID,@Team1Cape,@Team2MobID,@Team2Cape);
SELECT CONVERT(bigint,SCOPE_IDENTITY());",
            new
            {
                Enabled = enabled ? 1 : 0, WorldID = run.Config.ArenaWorldID, RegionID = run.Config.ArenaRegionID,
                Team1MobID = run.Config.Team1TowerMobID, Team1Cape, Team2MobID = run.Config.Team2TowerMobID, Team2Cape
            });
        await WaitForGameServerCommandAsync(connection, commandId, cancellationToken);
    }

    private static async Task QueueFreeForAllAsync(ActiveCompetitiveRun run, bool enabled, CancellationToken cancellationToken = default)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        var commandId = await connection.ExecuteScalarAsync<long>(@"
INSERT dbo.Command_GameServerQueue(Action_ID,Data1,Data2,Data3)
VALUES(39,@Enabled,@WorldID,@RegionID);
SELECT CONVERT(bigint,SCOPE_IDENTITY());",
            new { Enabled = enabled ? 1 : 0, WorldID = run.Config.ArenaWorldID, RegionID = run.Config.ArenaRegionID });
        await WaitForGameServerCommandAsync(connection, commandId, cancellationToken);
    }

    private sealed class GameServerCommandResultRow
    {
        public string Status { get; init; } = string.Empty;
        public int ResultCode { get; init; }
        public string? Reason { get; init; }
    }

    private static async Task WaitForGameServerCommandAsync(SqlConnection connection, long commandId, CancellationToken cancellationToken)
    {
        // Covers the durable queue's complete retry schedule, including the
        // longest backoff, without treating a still-pending row as success.
        var deadline = DateTime.UtcNow.AddMinutes(8);
        while (DateTime.UtcNow < deadline)
        {
            cancellationToken.ThrowIfCancellationRequested();
            var result = await connection.QueryFirstOrDefaultAsync<GameServerCommandResultRow>(
                "SELECT TOP(1) Status,ResultCode,Reason FROM dbo.Command_GameServerResult WITH(NOLOCK) WHERE CommandID=@ID;",
                new { ID = commandId });
            if (result != null)
            {
                if (string.Equals(result.Status, "Dispatched", StringComparison.Ordinal))
                    return;
                throw new InvalidOperationException(
                    $"GameServer command {commandId} ended as {result.Status} ({result.ResultCode}): {result.Reason}");
            }

            var pending = await connection.ExecuteScalarAsync<int>(
                "SELECT CASE WHEN EXISTS(SELECT 1 FROM dbo.Command_GameServerQueue WITH(NOLOCK) WHERE ID=@ID) THEN 1 ELSE 0 END;",
                new { ID = commandId });
            if (pending == 0)
                throw new InvalidOperationException($"GameServer command {commandId} disappeared without a durable result.");
            await Task.Delay(250, cancellationToken);
        }
        throw new TimeoutException("GameServer command was not consumed. Verify that ShardManager is running and connected to KMTGuard SQL.");
    }

    private static async Task QueueGetUpAsync(int charId)
    {
        if (charId <= 0) return;
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.ExecuteAsync("INSERT dbo.Command_GameServerQueue(Action_ID,Data1) VALUES(15,@CharID);", new { CharID = charId });
    }

    private static async Task QueueGetUpAfterKillDelayAsync(
        ActiveCompetitiveRun run,
        int charId,
        DateTime deathUtc)
    {
        try
        {
            await Task.Delay(KillReviveDelay);

            if (!ReferenceEquals(run, _active) ||
                run.Phase != CompetitivePhase.Fighting ||
                !run.Participants.ContainsKey(charId) ||
                !run.LastDeathUtc.TryGetValue(charId, out var latestDeathUtc) ||
                latestDeathUtc != deathUtc)
            {
                return;
            }

            await QueueGetUpAsync(charId);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "{EventCode} delayed revive failed for CharID={CharID}", run.Config.EventCode, charId);
        }
    }

    private static async Task SetRegionAttackAsync(ActiveCompetitiveRun run, bool enabled)
    {
        lock (ActionManager.m_CanAttackbyregionId)
        {
            if (!run.RegionAttackTouched)
            {
                run.HadPreviousRegionAttack = ActionManager.m_CanAttackbyregionId.TryGetValue(run.Config.ArenaRegionID, out var previous);
                run.PreviousRegionAttack = previous;
                run.RegionAttackTouched = true;
            }
            ActionManager.m_CanAttackbyregionId[run.Config.ArenaRegionID] = enabled;
        }
        await Task.CompletedTask;
    }

    private static void RestoreRegionAttack(ActiveCompetitiveRun run)
    {
        if (!run.RegionAttackTouched) return;
        lock (ActionManager.m_CanAttackbyregionId)
        {
            if (run.HadPreviousRegionAttack)
                ActionManager.m_CanAttackbyregionId[run.Config.ArenaRegionID] = run.PreviousRegionAttack;
            else
                ActionManager.m_CanAttackbyregionId.TryRemove(run.Config.ArenaRegionID, out _);
        }
    }

    private static async Task<bool> IsInPartyAsync(int charId)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        var partyDataTable = await PartyDataLocator.GetTableNameAsync(connection);
        return await connection.ExecuteScalarAsync<int>(
            $"SELECT CASE WHEN EXISTS(SELECT 1 FROM {partyDataTable} WITH(READCOMMITTED) WHERE CharID=@CharID) THEN 1 ELSE 0 END;",
            new { CharID = charId }) != 0;
    }

    private static async Task ApplySuitAsync(ISession session, int cape)
    {
        var packet = new Packet(0x3502);
        packet.WriteAscii(session.GameServerPacketKey);
        packet.WriteUInt8((byte)Math.Clamp(cape, 0, 5));
        await session.SendToServer(packet);
    }

    private static ISession? FindOnlineCharacter(int charId) =>
        ServerManager.AgentSessions.FindByCharId(charId, ServerManager.IsOnlinePlayer);

    private static async Task TrySendPersonalNoticeAsync(ISession? session, string message)
    {
        if (session == null) return;
        try
        {
            var packet = new Packet(0x168A);
            packet.WriteUInt8(NoticeType.WARNING);
            packet.WriteUnicode(message);
            await session.SendToClient(packet);
        }
        catch (Exception ex) { Log.Debug(ex, "Competitive personal notice failed."); }
    }

    private static async Task TryBroadcastNoticeAsync(string message, NoticeType type)
    {
        try { await BroadcastNoticeAsync(message, type); }
        catch (Exception ex) { Log.Debug(ex, "Competitive broadcast notice failed."); }
    }

    private static async Task BroadcastNoticeAsync(string message, NoticeType type)
    {
        var packet = new Packet(0x168A);
        packet.WriteUInt8(type);
        packet.WriteUnicode(message);
        await ServerManager.BroadcastPacket(packet);
    }

    private static string StartMessage(CompetitiveEventRuntimeConfig config) => config.Mode switch
    {
        CompetitiveEventMode.LastManStanding => PlayerLanguage.Get("Competitive.Started.Lms", config.DisplayName),
        CompetitiveEventMode.MadnessSolo => PlayerLanguage.Get("Competitive.Started.Madness", config.DisplayName),
        _ => PlayerLanguage.Get("Competitive.Started.Tower", config.DisplayName)
    };

    private static string FormatCompletionReason(string? reason) => reason switch
    {
        "Last survivor" => PlayerLanguage.Get("Competitive.Reason.LastSurvivor"),
        "Team 1 tower destroyed" => PlayerLanguage.Get("Competitive.Reason.RedTowerDestroyed"),
        "Team 2 tower destroyed" => PlayerLanguage.Get("Competitive.Reason.BlueTowerDestroyed"),
        "Last online survivor" => PlayerLanguage.Get("Competitive.Reason.LastOnlineSurvivor"),
        "Red team disconnected" => PlayerLanguage.Get("Competitive.Reason.RedTeamDisconnected"),
        "Blue team disconnected" => PlayerLanguage.Get("Competitive.Reason.BlueTeamDisconnected"),
        _ => PlayerLanguage.Get("Competitive.Reason.HighestScore")
    };

    private static string NormalizeCode(string? value) => (value ?? string.Empty).Trim().ToUpperInvariant();
    private static string Trim(string? value, int max) { value = (value ?? string.Empty).Trim(); return value.Length <= max ? value : value[..max]; }
    private sealed class ActiveCompetitiveRun
    {
        public ActiveCompetitiveRun(long runId, CompetitiveEventRuntimeConfig config, string startedBy, DateTime starts, DateTime ends)
        { RunID = runId; Config = config; StartedBy = startedBy; RegistrationStartsAtUtc = starts; RegistrationEndsAtUtc = ends; }
        public long RunID { get; }
        public long RoundID { get; set; }
        public CompetitiveEventRuntimeConfig Config { get; }
        public string StartedBy { get; }
        public DateTime RegistrationStartsAtUtc { get; }
        public DateTime RegistrationEndsAtUtc { get; }
        public DateTime? FightStartsAtUtc { get; set; }
        public DateTime? FightEndsAtUtc { get; set; }
        public CompetitivePhase Phase { get; set; } = CompetitivePhase.Registration;
        public bool RegionTimerStarted { get; set; }
        public bool RegionAttackTouched { get; set; }
        public bool HadPreviousRegionAttack { get; set; }
        public bool PreviousRegionAttack { get; set; }
        public string? PendingTerminalStatus { get; set; }
        public int RoundFinalizeState;
        public bool RoundFinalized => Volatile.Read(ref RoundFinalizeState) != 0;
        public int RewardedPlayerCount { get; set; }
        public Task? SpawnTask { get; set; }
        public ConcurrentDictionary<int, CompetitiveParticipant> Participants { get; } = new();
        public ConcurrentDictionary<int, CompetitiveScore> Scores { get; } = new();
        public ConcurrentDictionary<(int Killer, int Dead), int> PairKills { get; } = new();
        public ConcurrentDictionary<int, DateTime> LastDeathUtc { get; } = new();
        public ConcurrentDictionary<int, byte> DeadCharacters { get; } = new();
        public ConcurrentDictionary<int, CompetitiveParticipant> ArenaParticipants { get; } = new();
        public TaskCompletionSource<CompletionSignal> Completion { get; } = new(TaskCreationOptions.RunContinuationsAsynchronously);
        public int TowersReady;
    }

    private enum CompetitivePhase { Registration, Preparing, Fighting, Finishing, Cleanup }
    private sealed record CompetitiveParticipant(int CharID, string CharName, int JID, string Hwid, string ClientIP, int Team, int RegistrationOrder);
    private sealed record CompetitiveScore(int CharID, string CharName, int Team, int KillCount, bool IsAlive);
    private sealed record CompletionSignal(int? WinningTeam, int? WinnerCharID, string Reason);
    private sealed class CompetitiveReward
    {
        public string RewardType { get; set; } = string.Empty;
        public long Amount { get; set; }
        public string? ItemCodeName128 { get; set; }
        public int? ItemID { get; set; }
        public int ItemCount { get; set; } = 1;
        public int Plus { get; set; }
    }

    private sealed class CompetitiveConfigRow
    {
        public string EventCode { get; set; } = string.Empty;
        public string DisplayName { get; set; } = string.Empty;
        public string EventMode { get; set; } = string.Empty;
        public int EventID { get; set; }
        public bool Enabled { get; set; }
        public int StartDelaySeconds { get; set; }
        public int RegistrationSeconds { get; set; }
        public int PrepareSeconds { get; set; }
        public int FightSeconds { get; set; }
        public int MinPlayers { get; set; }
        public int MaxPlayers { get; set; }
        public int MinLevel { get; set; }
        public int HwidLimit { get; set; }
        public bool RequireHwid { get; set; }
        public bool RequireNoParty { get; set; }
        public int ArenaWorldID { get; set; }
        public int ArenaRegionID { get; set; }
        public int ArenaX { get; set; }
        public int ArenaY { get; set; }
        public int ArenaZ { get; set; }
        public int Team1X { get; set; }
        public int Team1Y { get; set; }
        public int Team1Z { get; set; }
        public int Team2X { get; set; }
        public int Team2Y { get; set; }
        public int Team2Z { get; set; }
        public int MadnessMobID { get; set; }
        public int MadnessMobCount { get; set; }
        public int MadnessMobX { get; set; }
        public int MadnessMobY { get; set; }
        public int MadnessMobZ { get; set; }
        public int MobSpawnDelaySeconds { get; set; }
        public int PairKillLimit { get; set; }
        public int TotalKillLimit { get; set; }
        public string KillRewardItemCode { get; set; } = string.Empty;
        public int KillRewardItemCount { get; set; }
        public int KillRewardLimit { get; set; }
        public int Team1TowerMobID { get; set; }
        public int Team1TowerX { get; set; }
        public int Team1TowerY { get; set; }
        public int Team1TowerZ { get; set; }
        public int Team2TowerMobID { get; set; }
        public int Team2TowerX { get; set; }
        public int Team2TowerY { get; set; }
        public int Team2TowerZ { get; set; }
    }
}
