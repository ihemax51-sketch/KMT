using System.Collections.Concurrent;
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

public sealed record SurvivalSoloRuntimeConfig(
    string EventCode,
    string DisplayName,
    int EventID,
    bool Enabled,
    int StartDelaySeconds,
    int RegistrationSeconds,
    int FightSeconds,
    int MinLevel,
    int HwidLimit,
    bool RequireHwid,
    int MaxPlayers,
    int ArenaWorldID,
    int ArenaRegionID,
    int ArenaX,
    int ArenaY,
    int ArenaZ);

public static class SurvivalSoloEventService
{
    private const string EventDatabaseName = "Events";
    private const int ReturnWorldId = 1;
    private const int ReturnRegionId = 25000;
    private const int ReturnPosX = 982;
    private const int ReturnPosY = 0;
    private const int ReturnPosZ = 140;
    private static readonly TimeSpan KillReviveDelay = TimeSpan.FromSeconds(10);
    private static readonly SemaphoreSlim StateLock = new(1, 1);
    private static readonly int[] SuitTeams = { 1, 2, 3, 4 };
    private static ActiveSurvivalSoloRun? _active;
    private static int _registrationEventId = 13;

    private static string EventTable(string tableName)
    {
        return $"{SqlIdentifier.Quote(EventDatabaseName)}.dbo.{SqlIdentifier.Quote(tableName)}";
    }

    public static bool IsActive => _active?.Phase == SurvivalSoloPhase.Fighting;
    public static bool IsRegistrationEventId(int eventId) =>
        eventId == (_active?.Config.EventID ?? Volatile.Read(ref _registrationEventId));

    public static async Task<SurvivalSoloRuntimeConfig> LoadRuntimeConfigAsync()
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await EnsureSchemaAsync(connection);

        var config = await connection.QueryFirstAsync<SurvivalSoloConfigRow>($@"
SELECT TOP (1) *
FROM {EventTable("_SurvivalSoloConfig")} WITH (NOLOCK)
WHERE EventCode = N'SSOLO';");

        return new SurvivalSoloRuntimeConfig(
            "SSOLO",
            string.IsNullOrWhiteSpace(config.DisplayName) ? "Survival Solo" : config.DisplayName,
            Math.Max(1, config.EventID),
            config.Enabled,
            Math.Clamp(config.StartDelaySeconds, 0, 3600),
            Math.Clamp(config.RegistrationSeconds, 0, 3600),
            Math.Clamp(config.FightSeconds, 30, 7200),
            Math.Max(0, config.MinLevel),
            Math.Clamp(config.HwidLimit, 0, 32),
            config.RequireHwid,
            Math.Clamp(config.MaxPlayers, 2, 100),
            Math.Max(1, config.ArenaWorldID),
            Math.Max(1, config.ArenaRegionID),
            config.ArenaX,
            config.ArenaY,
            config.ArenaZ);
    }

    public static async Task EnsureSchemaAsync(SqlConnection connection)
    {
        var ready = await connection.ExecuteScalarAsync<int>(@"
SELECT CASE WHEN OBJECT_ID(N'Events.dbo._SurvivalSoloConfig', N'U') IS NOT NULL
                  AND OBJECT_ID(N'Events.dbo._SurvivalSoloReward', N'U') IS NOT NULL
                  AND OBJECT_ID(N'Events.dbo._SurvivalSoloSchedule', N'U') IS NOT NULL
                  AND OBJECT_ID(N'Events.dbo.SSOLO_Score', N'U') IS NOT NULL
                  AND OBJECT_ID(N'Events.dbo.survival_solo_kill', N'U') IS NOT NULL
                  AND COL_LENGTH(N'Events.dbo._SurvivalSoloConfig', N'RequireHwid') IS NOT NULL
             THEN 1 ELSE 0 END;");
        if (ready != 1)
            throw new InvalidOperationException(
                "Survival Solo schema is incomplete. Apply the packaged database updates.");
    }
    public static async Task<string> HandleRegistrationAsync(ISession session)
    {
        var run = _active;
        if (run == null || run.Phase != SurvivalSoloPhase.Registration || DateTime.UtcNow >= run.RegistrationEndsAtUtc)
        {
            await SendPersonalNoticeAsync(session, PlayerLanguage.Get("SurvivalSolo.RegistrationClosed"));
            return "Registration closed.";
        }

        if (!ServerManager.IsOnlinePlayer(session))
            return "Character is not online.";

        if (run.Config.MinLevel > 0 && session.SessionData.CurLevel < run.Config.MinLevel)
        {
            await SendPersonalNoticeAsync(session, PlayerLanguage.Get("SurvivalSolo.MinimumLevel", run.Config.MinLevel));
            return "Minimum level failed.";
        }

        if (run.Config.RequireHwid && string.IsNullOrWhiteSpace(session.SessionData.Hwid))
        {
            await SendPersonalNoticeAsync(session, PlayerLanguage.Get("SurvivalSolo.HwidRequired"));
            return "HWID required.";
        }

        await StateLock.WaitAsync();
        try
        {
            if (_active == null || !ReferenceEquals(run, _active) ||
                run.Phase != SurvivalSoloPhase.Registration || DateTime.UtcNow >= run.RegistrationEndsAtUtc)
                return "Registration closed.";

            if (await IsInPartyAsync(session.SessionData.Charid))
            {
                await SendPersonalNoticeAsync(session, PlayerLanguage.Get("SurvivalSolo.LeaveParty"));
                return "Player is in a party.";
            }

            var charId = session.SessionData.Charid;
            if (run.Participants.ContainsKey(charId))
            {
                await SendPersonalNoticeAsync(session, PlayerLanguage.Get("SurvivalSolo.AlreadyRegistered"));
                return "Duplicate player.";
            }

            if (run.Participants.Count >= run.Config.MaxPlayers)
            {
                await SendPersonalNoticeAsync(session, PlayerLanguage.Get("SurvivalSolo.RegistrationFull", run.Config.MaxPlayers));
                return "Full.";
            }

            var hwid = session.SessionData.Hwid ?? string.Empty;
            if (run.Config.HwidLimit > 0 && !string.IsNullOrWhiteSpace(hwid) &&
                run.Participants.Values.Count(x => string.Equals(x.Hwid, hwid, StringComparison.OrdinalIgnoreCase)) >= run.Config.HwidLimit)
            {
                await SendPersonalNoticeAsync(session, PlayerLanguage.Get("SurvivalSolo.HwidLimit", run.Config.HwidLimit));
                return "HWID limit reached.";
            }

            var team = SuitTeams[run.RegisteredParties.Count % SuitTeams.Length];
            var member = new PartyMember
            {
                PartyID = charId,
                CharID = charId,
                CharName = session.SessionData.Charname ?? $"#{charId}",
                IsMaster = true,
                JID = session.SessionData.JID,
                Hwid = hwid,
                ClientIP = session.ClientIp ?? string.Empty
            };
            var participant = new SurvivalSoloParticipant(
                member.CharID,
                member.CharName,
                member.JID,
                member.Hwid,
                member.ClientIP,
                charId,
                charId,
                member.CharName,
                team);

            if (DateTime.UtcNow >= run.RegistrationEndsAtUtc)
                return "Registration closed.";

            await PersistRegistrationAsync(run, new[] { member }, team);

            run.RegisteredParties[charId] = new SurvivalSoloState(charId, charId, member.CharName, team);
            run.Participants[charId] = participant;

            await TrySendPersonalNoticeAsync(session, PlayerLanguage.Get("SurvivalSolo.Registered"));
            await TryBroadcastNoticeAsync(
                PlayerLanguage.Get(
                    "SurvivalSolo.PlayerRegisteredBroadcast",
                    run.Config.DisplayName,
                    member.CharName,
                    run.Participants.Count,
                    run.Config.MaxPlayers),
                NoticeType.NOTICE);
            return $"Registered player {charId}.";
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Survival Solo registration failed for CharID={CharID}", session.SessionData.Charid);
            await TrySendPersonalNoticeAsync(session, PlayerLanguage.Get("SurvivalSolo.RegistrationFailed"));
            return ex.Message;
        }
        finally
        {
            StateLock.Release();
        }
    }

    public static async Task HandleCharacterKillAsync(
        int worldId,
        int regionId,
        int killerCharId,
        string killerCharName,
        int deadCharId)
    {
        var run = _active;
        if (run == null || run.Phase != SurvivalSoloPhase.Fighting || killerCharId <= 0 || deadCharId <= 0 || killerCharId == deadCharId)
            return;

        if (worldId != run.Config.ArenaWorldID || regionId != run.Config.ArenaRegionID)
            return;

        if (!run.Participants.TryGetValue(killerCharId, out var killer) ||
            !run.Participants.ContainsKey(deadCharId))
        {
            return;
        }

        var deathUtc = DateTime.UtcNow;
        if (!AutoEventService.TryRecordDeath(run.LastDeathUtc, deadCharId, deathUtc, TimeSpan.FromSeconds(1)))
            return;
        run.DeadCharacters[deadCharId] = 0;
        _ = QueueGetUpAfterKillDelayAsync(run, deadCharId, deathUtc);

        var score = run.Scores.AddOrUpdate(
            killer.PartyID,
            _ => new SurvivalSoloScore(killer.PartyID, killer.MasterCharID, killer.MasterName, killer.Team, 1),
            (_, current) => current with { KillCount = current.KillCount + 1 });

        await UpsertScoreAsync(run, score);
        await BroadcastScoreAsync(run);
    }

    public static async Task RunAsync(SurvivalSoloRuntimeConfig config, long runId, string startedBy, CancellationToken cancellationToken)
    {
        await StateLock.WaitAsync(cancellationToken);
        try
        {
            if (_active != null)
                throw new InvalidOperationException("Survival Solo is already active.");

            await RegionControlService.EnsureEventProfileAsync(
                config.EventCode,
                config.ArenaWorldID,
                config.ArenaRegionID,
                teams: false,
                allowParty: false);

            var now = DateTime.UtcNow;
            _active = new ActiveSurvivalSoloRun(runId, config, startedBy, now, now.AddSeconds(config.RegistrationSeconds));
        }
        finally
        {
            StateLock.Release();
        }

        var run = _active!;
        try
        {
            run.RoundID = await CreateAutoEventRoundAsync(run.RunID, config.DisplayName);
            await ResetRoundStorageAsync(config.EventID);
            await OpenControlAsync(config.EventID, config.EventCode, run.RegistrationStartsAtUtc, run.RegistrationEndsAtUtc, true);
            await BroadcastNoticeAsync(
                PlayerLanguage.Get(
                    "SurvivalSolo.RegistrationOpen",
                    config.DisplayName,
                    PlayerLanguage.FormatDurationSeconds(config.RegistrationSeconds)),
                NoticeType.NOTICE);
            await Task.Delay(TimeSpan.FromSeconds(config.RegistrationSeconds), cancellationToken);

            await StateLock.WaitAsync(cancellationToken);
            try
            {
                if (!ReferenceEquals(_active, run))
                    throw new OperationCanceledException(cancellationToken);
                run.Phase = SurvivalSoloPhase.Preparing;
            }
            finally
            {
                StateLock.Release();
            }
            await OpenControlAsync(config.EventID, config.EventCode, run.RegistrationStartsAtUtc, run.RegistrationEndsAtUtc, false);
            await RemoveIneligibleParticipantsBeforeFightAsync(run);

            if (run.Participants.Count < 2 || run.RegisteredParties.Count < 2)
            {
                await BroadcastNoticeAsync(PlayerLanguage.Get("SurvivalSolo.CancelledNotEnoughPlayers", config.DisplayName), NoticeType.WARNING);
                await CompleteAutoEventRoundAsync(run, null, null, "Cancelled");
                return;
            }

            await BroadcastNoticeAsync(PlayerLanguage.Get("SurvivalSolo.Teleporting", config.DisplayName), NoticeType.NOTICE);
            await TeleportParticipantsAsync(run, toArena: true);
            await RemoveUnteleportedParticipantsAsync(run);
            if (run.Participants.Count < 2 || run.RegisteredParties.Count < 2)
            {
                await BroadcastNoticeAsync(PlayerLanguage.Get("SurvivalSolo.CancelledNotEnoughPlayers", config.DisplayName), NoticeType.WARNING);
                await CompleteAutoEventRoundAsync(run, null, null, "Cancelled");
                return;
            }
            await Task.Delay(TimeSpan.FromSeconds(3), cancellationToken);

            await InitializeScoresAsync(run);
            await ShowScoreboardAsync(run);

            run.RegionTimerStarted = true;
            await DatabaseCommands.SetRegionTimerAsync(config.ArenaRegionID, config.FightSeconds);

            var fightStartsAtUtc = DateTime.UtcNow;
            var fightEndsAtUtc = fightStartsAtUtc.AddSeconds(config.FightSeconds);
            run.FightStartsAtUtc = fightStartsAtUtc;
            run.FightEndsAtUtc = fightEndsAtUtc;
            run.Phase = SurvivalSoloPhase.Fighting;
            await OpenControlAsync(config.EventID, config.EventCode, fightStartsAtUtc, fightEndsAtUtc, true);
            await BroadcastNoticeAsync(PlayerLanguage.Get("SurvivalSolo.Started", config.DisplayName), NoticeType.NOTICE);

            var remainingFightTime = fightEndsAtUtc - DateTime.UtcNow;
            if (remainingFightTime > TimeSpan.Zero)
                await Task.Delay(remainingFightTime, cancellationToken);

            run.Phase = SurvivalSoloPhase.Finishing;
            await OpenControlAsync(config.EventID, config.EventCode, fightStartsAtUtc, fightEndsAtUtc, false);
            await FinishRoundAsync(run);
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
            await CleanupAsync(run);
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

    private static async Task FinishRoundAsync(ActiveSurvivalSoloRun run)
    {
        var standings = run.Scores.Values
            .OrderByDescending(x => x.KillCount)
            .ThenBy(x => x.MasterCharID)
            .ToList();
        var winner = standings.FirstOrDefault();

        if (winner == null || winner.KillCount <= 0)
        {
            await TryBroadcastNoticeAsync(PlayerLanguage.Get("SurvivalSolo.NoWinner", run.Config.DisplayName), NoticeType.WARNING);
            await CompleteAutoEventRoundAsync(run, null, null, "TimedOut");
            return;
        }

        var tied = standings.Where(x => x.KillCount == winner.KillCount).ToList();
        if (tied.Count > 1)
        {
            var names = string.Join(", ", tied.Select(x => x.MasterName));
            await TryBroadcastNoticeAsync(
                PlayerLanguage.Get("SurvivalSolo.Draw", run.Config.DisplayName, winner.KillCount, names),
                NoticeType.WARNING);
            await CompleteAutoEventRoundAsync(run, null, null, "Draw");
            return;
        }

        await TryBroadcastNoticeAsync(PlayerLanguage.Get("SurvivalSolo.Winner", run.Config.DisplayName, winner.MasterName, winner.KillCount), NoticeType.NOTICE);
        var winnerSummary = await RewardParticipantsAsync(run, winner.PartyID, winnerPlacement: true);
        var loserSummary = await RewardParticipantsAsync(run, winner.PartyID, winnerPlacement: false);
        await LogEventWinAsync(run, winner);
        await InsertAutoEventWinnerLogAsync(run, winner, winnerSummary, loserSummary);
        await CompleteAutoEventRoundAsync(run, winner.MasterCharID, winner.MasterName, "Won");
        await TryBroadcastNoticeAsync(PlayerLanguage.Get("SurvivalSolo.RewardsDelivered", run.Config.DisplayName, winnerSummary, loserSummary), NoticeType.NOTICE);
    }

    private static async Task<string> RewardParticipantsAsync(ActiveSurvivalSoloRun run, int winnerPartyId, bool winnerPlacement)
    {
        var placement = winnerPlacement ? 1 : 2;
        var rewards = await LoadRewardsAsync(run.Config.EventCode, placement);
        if (rewards.Count == 0)
            return PlayerLanguage.Get("Common.NoRewardConfigured");

        var participants = run.Participants.Values
            .Where(x => winnerPlacement ? x.PartyID == winnerPartyId : x.PartyID != winnerPartyId)
            .GroupBy(x => x.CharID)
            .Select(x => x.First())
            .ToList();

        var rewarded = 0;
        foreach (var participant in participants)
        {
            var session = FindOnlineCharacter(participant.CharID);
            foreach (var reward in rewards)
            {
                try
                {
                    await ApplyRewardAsync(session, participant, reward);
                }
                catch (Exception ex)
                {
                    Log.Warning(ex, "Survival Solo reward failed for CharID={CharID}, RewardType={RewardType}",
                        participant.CharID,
                        reward.RewardType);
                }
            }
            rewarded++;
        }

        return PlayerLanguage.Get("Common.PlayerCount", rewarded);
    }

    private static async Task ApplyRewardAsync(ISession? session, SurvivalSoloParticipant participant, SurvivalSoloReward reward)
    {
        if (reward.RewardType.Equals("ItemChest", StringComparison.OrdinalIgnoreCase))
        {
            bool added = false;
            if (!string.IsNullOrWhiteSpace(reward.ItemCodeName128))
                added = await sqlQueryHelper.AddItemToChest(participant.CharID, reward.ItemCodeName128, Math.Clamp(reward.ItemCount, 1, 10000), "SurvivalSolo", Math.Clamp(reward.Plus, 0, 20));
            else
            {
                var itemId = reward.ItemID.GetValueOrDefault();
                if (itemId > 0)
                    added = await sqlQueryHelper.AddItemToChest2(participant.CharID, itemId, Math.Clamp(reward.ItemCount, 1, 10000), "SurvivalSolo", Math.Clamp(reward.Plus, 0, 20));
            }
            if (!added)
                Log.Error("Survival Solo Item Chest reward failed for CharID={CharID}, ItemCode={ItemCode}, ItemID={ItemID}.", participant.CharID, reward.ItemCodeName128, reward.ItemID);
            return;
        }

        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();

        if (reward.RewardType.Equals("Gold", StringComparison.OrdinalIgnoreCase))
        {
            if (reward.Amount <= 0)
                return;
            var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
            await connection.ExecuteAsync(
                $@"UPDATE {shardDb}.._Char SET RemainGold = RemainGold + @Amount WHERE CharID = @CharID",
                new { Amount = reward.Amount, participant.CharID });
            return;
        }

        var silkColumn = reward.RewardType.ToUpperInvariant() switch
        {
            "SILKOWN" => "silk_own",
            "SILKGIFT" => "silk_gift",
            "SILKPOINT" => "silk_point",
            _ => string.Empty
        };

        var jid = session?.SessionData.JID ?? participant.JID;
        if (silkColumn.Length == 0 || jid <= 0 || reward.Amount <= 0)
            return;

        var accountDb = SqlIdentifier.Quote(_serverSettings.AccountDB);
        await connection.ExecuteAsync($@"
IF NOT EXISTS (SELECT 1 FROM {accountDb}..SK_Silk WITH (UPDLOCK, HOLDLOCK) WHERE JID = @JID)
    INSERT INTO {accountDb}..SK_Silk (JID, silk_own, silk_gift, silk_point) VALUES (@JID, 0, 0, 0);

UPDATE {accountDb}..SK_Silk
SET {silkColumn} = {silkColumn} + @Amount
WHERE JID = @JID",
            new { JID = jid, Amount = (int)Math.Clamp(reward.Amount, 0, int.MaxValue) });
    }

    private static async Task<bool> IsInPartyAsync(int charId)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        var partyDataTable = await PartyDataLocator.GetTableNameAsync(connection);
        return await connection.ExecuteScalarAsync<int>(
            $"SELECT CASE WHEN EXISTS (SELECT 1 FROM {partyDataTable} WITH (READCOMMITTED) WHERE CharID = @CharID) THEN 1 ELSE 0 END",
            new { CharID = charId }) != 0;
    }

    private static async Task RemoveIneligibleParticipantsBeforeFightAsync(ActiveSurvivalSoloRun run)
    {
        var charIds = run.Participants.Keys.ToArray();
        if (charIds.Length == 0)
            return;

        var onlineByCharId = ServerManager.AgentSessions
            .Where(ServerManager.IsOnlinePlayer)
            .GroupBy(x => x.SessionData.Charid)
            .ToDictionary(x => x.Key, x => x.First());
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        var partyDataTable = await PartyDataLocator.GetTableNameAsync(connection);
        var partyMembers = (await connection.QueryAsync<int>(
            $"SELECT DISTINCT CharID FROM {partyDataTable} WITH (READCOMMITTED) WHERE CharID IN @CharIDs",
            new { CharIDs = charIds })).ToHashSet();
        var hwidCounts = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase);
        var disqualified = new List<int>();

        foreach (var participant in run.Participants.Values.OrderBy(x => x.CharID))
        {
            if (!onlineByCharId.TryGetValue(participant.CharID, out var session) ||
                partyMembers.Contains(participant.CharID) ||
                (run.Config.MinLevel > 0 && session.SessionData.CurLevel < run.Config.MinLevel) ||
                (run.Config.RequireHwid && string.IsNullOrWhiteSpace(session.SessionData.Hwid)))
            {
                disqualified.Add(participant.CharID);
                continue;
            }

            var currentHwid = session.SessionData.Hwid ?? string.Empty;
            if (run.Config.HwidLimit > 0 && !string.IsNullOrWhiteSpace(currentHwid))
            {
                hwidCounts.TryGetValue(currentHwid, out var count);
                if (count >= run.Config.HwidLimit)
                {
                    disqualified.Add(participant.CharID);
                    continue;
                }
                hwidCounts[currentHwid] = count + 1;
            }

            run.Participants[participant.CharID] = participant with
            {
                JID = session.SessionData.JID,
                Hwid = currentHwid,
                ClientIP = session.ClientIp ?? string.Empty
            };
        }

        foreach (var charId in disqualified)
        {
            run.Participants.TryRemove(charId, out _);
            run.RegisteredParties.TryRemove(charId, out _);
            run.Scores.TryRemove(charId, out _);
            if (onlineByCharId.TryGetValue(charId, out var session))
                await TrySendPersonalNoticeAsync(session, PlayerLanguage.Get("SurvivalSolo.FinalEligibilityFailed"));
        }

        if (disqualified.Count > 0)
        {
            await connection.ExecuteAsync(@"
DELETE FROM [dbo].[Event_CurrentTeams] WHERE CharID IN @CharIDs;
DELETE FROM Events.dbo.Event_RegPlayers
WHERE EventID = @EventID AND EventStartUtc = @EventStartUtc AND CharID IN @CharIDs;",
                new
                {
                    CharIDs = disqualified,
                    EventID = run.Config.EventID,
                    EventStartUtc = run.RegistrationStartsAtUtc
                });
        }
    }

    private static async Task PersistRegistrationAsync(ActiveSurvivalSoloRun run, IReadOnlyList<PartyMember> members, int team)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await using var transaction = await connection.BeginTransactionAsync();
        try
        {
            foreach (var member in members)
            {
                await connection.ExecuteAsync($@"
IF NOT EXISTS (
    SELECT 1 FROM {EventTable("Event_RegPlayers")} WITH (UPDLOCK, HOLDLOCK)
    WHERE EventID = @EventID AND EventStartUtc = @EventStartUtc AND CharID = @CharID
)
BEGIN
    INSERT INTO {EventTable("Event_RegPlayers")}
        (EventID, EventCode, EventStartUtc, CharID, CharName, Amount, Currency)
    VALUES (@EventID, @EventCode, @EventStartUtc, @CharID, @CharName, 0, N'silk');
END",
                new
                {
                    EventID = run.Config.EventID,
                    run.Config.EventCode,
                    EventStartUtc = run.RegistrationStartsAtUtc,
                    member.CharID,
                    CharName = Trim(member.CharName, 64)
                }, transaction);

                await connection.ExecuteAsync(@"
MERGE [dbo].[Event_CurrentTeams] AS target
USING (SELECT @CharID AS CharID) AS source
ON target.CharID = source.CharID
WHEN MATCHED THEN
    UPDATE SET CharName = @CharName, Team = @Team, EventName = @EventName
WHEN NOT MATCHED THEN
    INSERT (CharID, CharName, Team, EventName)
    VALUES (@CharID, @CharName, @Team, @EventName);",
                    new
                    {
                        member.CharID,
                        CharName = Trim(member.CharName, 25),
                        Team = team,
                        EventName = "SSOLO"
                    }, transaction);
            }

            await transaction.CommitAsync();
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }
    }

    private static async Task ResetRoundStorageAsync(int eventId)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await connection.ExecuteAsync($@"
DELETE FROM {EventTable("SSOLO_Score")};
DELETE ect
FROM [dbo].[Event_CurrentTeams] ect
WHERE EXISTS (
    SELECT 1 FROM {EventTable("Event_RegPlayers")} er WITH (NOLOCK)
    WHERE er.CharID = ect.CharID AND er.EventID = @EventID
);
DELETE FROM {EventTable("Event_RegPlayers")} WHERE EventID = @EventID;",
            new { EventID = eventId });
    }

    private static async Task OpenControlAsync(int eventId, string eventCode, DateTime startUtc, DateTime endUtc, bool isOpen)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await connection.ExecuteAsync($@"
MERGE {EventTable("Event_Control")} AS target
USING (SELECT @EventID AS EventID) AS source
ON target.EventID = source.EventID
WHEN MATCHED THEN
    UPDATE SET EventCode = @EventCode, IsOpen = @IsOpen, StartUtc = @StartUtc, EndUtc = @EndUtc, UpdatedAtUtc = SYSUTCDATETIME()
WHEN NOT MATCHED THEN
    INSERT (EventID, EventCode, IsOpen, StartUtc, EndUtc, UpdatedAtUtc)
    VALUES (@EventID, @EventCode, @IsOpen, @StartUtc, @EndUtc, SYSUTCDATETIME());",
            new { EventID = eventId, EventCode = eventCode, IsOpen = isOpen, StartUtc = startUtc, EndUtc = endUtc });
    }

    private static async Task UpsertScoreAsync(ActiveSurvivalSoloRun run, SurvivalSoloScore score)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await connection.ExecuteAsync($@"
MERGE {EventTable("SSOLO_Score")} AS target
USING (SELECT @RunID AS RunID, @PlayerCharID AS PlayerCharID) AS source
ON target.RunID = source.RunID AND target.PlayerCharID = source.PlayerCharID
WHEN MATCHED THEN
    UPDATE SET KillCount = @KillCount, UpdatedAtUtc = SYSUTCDATETIME()
WHEN NOT MATCHED THEN
    INSERT (RunID, EventStartUtc, PlayerCharID, PlayerName, Team, KillCount, UpdatedAtUtc)
    VALUES (@RunID, @EventStartUtc, @PlayerCharID, @PlayerName, @Team, @KillCount, SYSUTCDATETIME());",
            new
            {
                run.RunID,
                EventStartUtc = run.RegistrationStartsAtUtc,
                PlayerCharID = score.MasterCharID,
                PlayerName = Trim(score.MasterName, 64),
                score.Team,
                score.KillCount
            });
    }

    private static async Task InitializeScoresAsync(ActiveSurvivalSoloRun run)
    {
        foreach (var party in run.RegisteredParties.Values)
        {
            var score = new SurvivalSoloScore(party.PartyID, party.MasterCharID, party.MasterName, party.Team, 0);
            run.Scores[party.PartyID] = score;
            await UpsertScoreAsync(run, score);
        }
    }

    private static async Task<IReadOnlyList<SurvivalSoloReward>> LoadRewardsAsync(string eventCode, int placement)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        var rewards = await connection.QueryAsync<SurvivalSoloReward>($@"
SELECT RewardType, Amount, ItemCodeName128, ItemID, ItemCount, Plus
FROM {EventTable("_SurvivalSoloReward")} WITH (NOLOCK)
WHERE Placement = @Placement AND IsActive = 1
ORDER BY RewardID",
            new { Placement = placement });
        return rewards.AsList();
    }

    private static async Task LogEventWinAsync(ActiveSurvivalSoloRun run, SurvivalSoloScore winner)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        var memberCount = run.Participants.Values.Count(x => x.PartyID == winner.PartyID);
        var winnerReward = (await LoadRewardsAsync(run.Config.EventCode, 1)).FirstOrDefault();
        var totalPot = winnerReward == null ? 0 : memberCount * winnerReward.Amount;
        await connection.ExecuteAsync($@"
INSERT INTO {EventTable("Event_Wins")}
    (EventID, EventCode, EventStartUtc, EventEndUtc, WinnerCharID, WinnerCharName, WinnerPartyNo, TotalPot, Currency)
VALUES
    (@EventID, @EventCode, @EventStartUtc, @EventEndUtc, @WinnerCharID, @WinnerCharName, @WinnerPartyNo, @TotalPot, N'silk');",
            new
            {
                EventID = run.Config.EventID,
                run.Config.EventCode,
                EventStartUtc = run.RegistrationStartsAtUtc,
                EventEndUtc = run.FightEndsAtUtc ?? DateTime.UtcNow,
                WinnerCharID = winner.MasterCharID,
                WinnerCharName = Trim(winner.MasterName, 64),
                WinnerPartyNo = (int?)null,
                TotalPot = totalPot
            });
    }

    private static async Task InsertAutoEventWinnerLogAsync(ActiveSurvivalSoloRun run, SurvivalSoloScore winner, string winnerSummary, string loserSummary)
    {
        var winnerParticipant = run.Participants.Values.FirstOrDefault(x => x.CharID == winner.MasterCharID) ??
                                run.Participants.Values.First(x => x.PartyID == winner.PartyID);

        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await connection.ExecuteAsync($@"
INSERT INTO {EventTable("_AutoEventWinnerLog")}
    (RunID, RoundID, EventCode, RoundNo, CharID, CharName, JID, Hwid, ClientIP, Answer, WonAtUtc, RewardSummary)
VALUES
    (@RunID, @RoundID, @EventCode, 1, @CharID, @CharName, @JID, @Hwid, @ClientIP, @Answer, SYSUTCDATETIME(), @RewardSummary);",
            new
            {
                run.RunID,
                RoundID = run.RoundID,
                run.Config.EventCode,
                winnerParticipant.CharID,
                CharName = Trim(winnerParticipant.CharName, 64),
                winnerParticipant.JID,
                Hwid = Trim(winnerParticipant.Hwid, 128),
                ClientIP = Trim(winnerParticipant.ClientIP, 64),
                Answer = $"CharID={winner.MasterCharID};Kills={winner.KillCount}",
                RewardSummary = Trim($"Winners: {winnerSummary}; Others: {loserSummary}", 512)
            });
    }

    private static async Task CompleteAutoEventRoundAsync(ActiveSurvivalSoloRun run, int? winnerCharId, string? winnerName, string status)
    {
        if (run.RoundID <= 0)
            return;
        if (Interlocked.CompareExchange(ref run.RoundFinalizeState, 1, 0) != 0)
            return;

        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await connection.ExecuteAsync($@"
UPDATE {EventTable("_AutoEventRound")}
SET Status = @Status,
    WinnerCharID = @WinnerCharID,
    WinnerCharName = @WinnerCharName,
    EndedAtUtc = SYSUTCDATETIME()
WHERE RoundID = @RoundID AND EndedAtUtc IS NULL",
                new { run.RoundID, Status = status, WinnerCharID = winnerCharId, WinnerCharName = Trim(winnerName ?? string.Empty, 64) });
        }
        catch
        {
            Interlocked.Exchange(ref run.RoundFinalizeState, 0);
            throw;
        }
    }

    public static async Task<long> CreateAutoEventRoundAsync(long runId, string displayName)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        return await connection.ExecuteScalarAsync<long>($@"
INSERT INTO {EventTable("_AutoEventRound")} (RunID, RoundNo, Prompt, AnswerMasked, Status, StartedAtUtc)
VALUES (@RunID, 1, @Prompt, N'individual-kills', N'Running', SYSUTCDATETIME());
SELECT CONVERT(bigint, SCOPE_IDENTITY());",
            new { RunID = runId, Prompt = $"{displayName}: highest individual kill count wins." });
    }

    private static async Task ShowScoreboardAsync(ActiveSurvivalSoloRun run)
    {
        var scoreboardTitle = PlayerLanguage.Get("SurvivalSolo.ScoreboardTitle", run.Config.DisplayName);
        ActionManager.CreatedKillCounterWorldID[run.Config.ArenaWorldID] = scoreboardTitle;
        ActionManager.CreatedFullKillCounterWorldID[run.Config.ArenaWorldID] = 1;
        foreach (var entry in ActionManager.KillCounterKillList.Where(x => x.Value.WorldID == run.Config.ArenaWorldID).ToArray())
            ActionManager.KillCounterKillList.TryRemove(entry.Key, out _);
        await BroadcastScoreWindowStateAsync(run.Config.ArenaWorldID, show: true, scoreboardTitle);
        await BroadcastScoreAsync(run);
    }

    private static async Task HideScoreboardAsync(ActiveSurvivalSoloRun run)
    {
        ActionManager.CreatedKillCounterWorldID.TryRemove(run.Config.ArenaWorldID, out _);
        ActionManager.CreatedFullKillCounterWorldID.TryRemove(run.Config.ArenaWorldID, out _);
        foreach (var entry in ActionManager.KillCounterKillList.Where(x => x.Value.WorldID == run.Config.ArenaWorldID).ToArray())
            ActionManager.KillCounterKillList.TryRemove(entry.Key, out _);

        await BroadcastScoreWindowStateAsync(run.Config.ArenaWorldID, show: false, string.Empty);
    }

    private static async Task BroadcastScoreWindowStateAsync(int worldId, bool show, string title)
    {
        var packet = new Packet(0x207A, false, false);
        packet.WriteUInt8(show ? (byte)1 : (byte)0);
        packet.WriteAscii(show ? title : string.Empty);
        await ServerManager.BroadcastPacketbyWorldID(worldId, packet);
    }

    private static async Task BroadcastScoreAsync(ActiveSurvivalSoloRun run)
    {
        var top = run.Scores.Values
            .OrderByDescending(x => x.KillCount)
            .ThenBy(x => x.MasterCharID)
            .ToList();

        foreach (var score in top)
        {
            ActionManager.KillCounterKillList[score.Label] = new SCreatedKillCounterKillList
            {
                WorldID = run.Config.ArenaWorldID,
                CharName16 = score.Label,
                Kill = score.KillCount
            };
        }

        var packet = new Packet(0x207C);
        packet.WriteUInt8(top.Count);
        foreach (var score in top)
        {
            packet.WriteAscii(score.Label);
            packet.WriteInt32(score.KillCount);
        }

        await ServerManager.BroadcastPacketbyWorldID(run.Config.ArenaWorldID, packet);
    }

    private static async Task TeleportParticipantsAsync(ActiveSurvivalSoloRun run, bool toArena)
    {
        if (!toArena && run.DeadCharacters.Count > 0)
        {
            foreach (var deadCharId in run.DeadCharacters.Keys)
                await QueueGetUpAsync(deadCharId);

            await Task.Delay(TimeSpan.FromSeconds(1));
        }

        var participants = toArena
            ? run.Participants.Values
            : run.Participants.Values.Where(x => run.ArenaTeleportedCharIds.ContainsKey(x.CharID));
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
                        Log.Information("Survival Solo skipped teleport for CharID={CharID} — wearing job suit ({JobType})", participant.CharID, jobType);
                        continue;
                    }

                    await TeleportFreezeService.TeleportToPositionAsync(
                        session,
                        run.Config.ArenaWorldID,
                        run.Config.ArenaRegionID,
                        run.Config.ArenaX,
                        run.Config.ArenaY,
                        run.Config.ArenaZ,
                        3);
                    run.ArenaTeleportedCharIds[participant.CharID] = 0;
                    try
                    {
                        await ApplySuitAsync(session, participant.Team);
                    }
                    catch (Exception ex)
                    {
                        Log.Warning(ex, "Survival Solo suit apply failed for CharID={CharID}", participant.CharID);
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
                        Log.Warning(ex, "Survival Solo suit clear failed for CharID={CharID}", participant.CharID);
                    }
                    await TeleportFreezeService.TeleportToPositionAsync(
                        session,
                        ReturnWorldId,
                        ReturnRegionId,
                        ReturnPosX,
                        ReturnPosY,
                        ReturnPosZ,
                        0);
                    run.ArenaTeleportedCharIds.TryRemove(participant.CharID, out _);
                }
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Survival Solo teleport failed for CharID={CharID}, ToArena={ToArena}", participant.CharID, toArena);
            }
        }
    }

    private static async Task RemoveUnteleportedParticipantsAsync(ActiveSurvivalSoloRun run)
    {
        var removedCharIds = run.Participants.Keys
            .Where(charId => !run.ArenaTeleportedCharIds.ContainsKey(charId))
            .ToArray();
        foreach (var charId in removedCharIds)
        {
            run.Participants.TryRemove(charId, out _);
            run.RegisteredParties.TryRemove(charId, out _);
        }

        if (removedCharIds.Length == 0)
            return;

        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await connection.ExecuteAsync(@"
DELETE FROM [dbo].[Event_CurrentTeams]
WHERE CharID IN @CharIds;", new { CharIds = removedCharIds });
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Survival Solo failed to prune players that could not enter the arena.");
        }
    }

    private static async Task QueueGetUpAsync(int charId)
    {
        if (charId <= 0)
            return;

        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await connection.ExecuteAsync(@"
INSERT INTO [dbo].[Command_GameServerQueue] (Action_ID, Data1)
VALUES (15, @CharID);", new { CharID = charId });
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Survival Solo immediate revive failed for CharID={CharID}", charId);
        }
    }

    private static async Task QueueGetUpAfterKillDelayAsync(
        ActiveSurvivalSoloRun run,
        int charId,
        DateTime deathUtc)
    {
        try
        {
            await Task.Delay(KillReviveDelay);

            if (!ReferenceEquals(run, _active) ||
                run.Phase != SurvivalSoloPhase.Fighting ||
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
            Log.Warning(ex, "Survival Solo delayed revive failed for CharID={CharID}", charId);
        }
    }

    private static async Task ApplySuitAsync(ISession session, int team)
    {
        var packet = new Packet(0x3502);
        packet.WriteAscii(session.GameServerPacketKey);
        packet.WriteUInt8((byte)Math.Clamp(team, 0, 5));
        await session.SendToServer(packet);
    }

    private static async Task CleanupAsync(ActiveSurvivalSoloRun run)
    {
        run.Phase = SurvivalSoloPhase.Cleanup;
        try
        {
            var startUtc = run.FightStartsAtUtc ?? run.RegistrationStartsAtUtc;
            await OpenControlAsync(run.Config.EventID, run.Config.EventCode, startUtc, DateTime.UtcNow, false);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Survival Solo event control cleanup failed.");
        }

        if (!run.RoundFinalized)
        {
            try
            {
                await CompleteAutoEventRoundAsync(
                    run,
                    null,
                    null,
                    run.PendingTerminalStatus ?? "Failed");
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Survival Solo round cleanup failed.");
            }
        }

        if (run.RegionTimerStarted)
        {
            try
            {
                await DatabaseCommands.SetRegionTimerAsync(run.Config.ArenaRegionID, 0);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Survival Solo region timer cleanup failed.");
            }
            finally
            {
                run.RegionTimerStarted = false;
            }
        }

        try
        {
            await HideScoreboardAsync(run);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Survival Solo scoreboard cleanup failed.");
        }

        try
        {
            await TeleportParticipantsAsync(run, toArena: false);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Survival Solo player return failed.");
        }

        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            var charIds = run.Participants.Keys.ToArray();
            if (charIds.Length > 0)
            {
                await connection.ExecuteAsync(@"
DELETE FROM [dbo].[Event_CurrentTeams]
WHERE CharID IN @CharIds;",
                    new { CharIds = charIds });
            }
            await connection.ExecuteAsync($@"DELETE FROM {EventTable("survival_solo_kill")};");
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Survival Solo database cleanup failed.");
        }
    }

    private static ISession? FindOnlineCharacter(int charId)
    {
        return ServerManager.AgentSessions.FindByCharId(charId, ServerManager.IsOnlinePlayer);
    }

    private static async Task SendPersonalNoticeAsync(ISession session, string message)
    {
        var packet = new Packet(0x168A);
        packet.WriteUInt8(NoticeType.WARNING);
        packet.WriteUnicode(message);
        await session.SendToClient(packet);
    }

    private static async Task TrySendPersonalNoticeAsync(ISession session, string message)
    {
        try
        {
            await SendPersonalNoticeAsync(session, message);
        }
        catch (Exception ex)
        {
            Log.Debug(ex, "Survival Solo personal notice failed for CharID={CharID}", session.SessionData.Charid);
        }
    }

    private static async Task TryBroadcastNoticeAsync(string message, NoticeType type)
    {
        try
        {
            await BroadcastNoticeAsync(message, type);
        }
        catch (Exception ex)
        {
            Log.Debug(ex, "Survival Solo broadcast notice failed.");
        }
    }

    private static async Task BroadcastNoticeAsync(string message, NoticeType type)
    {
        var packet = new Packet(0x168A);
        packet.WriteUInt8(type);
        packet.WriteUnicode(message);
        await ServerManager.BroadcastPacket(packet);
    }

    private static string Trim(string? value, int max)
    {
        value ??= string.Empty;
        value = value.Trim();
        return value.Length <= max ? value : value[..max];
    }

    private sealed class ActiveSurvivalSoloRun
    {
        public ActiveSurvivalSoloRun(long runId, SurvivalSoloRuntimeConfig config, string startedBy, DateTime registrationStartsAtUtc, DateTime registrationEndsAtUtc)
        {
            RunID = runId;
            Config = config;
            StartedBy = startedBy;
            RegistrationStartsAtUtc = registrationStartsAtUtc;
            RegistrationEndsAtUtc = registrationEndsAtUtc;
        }

        public long RunID { get; }
        public long RoundID { get; set; }
        public SurvivalSoloRuntimeConfig Config { get; }
        public string StartedBy { get; }
        public DateTime RegistrationStartsAtUtc { get; }
        public DateTime RegistrationEndsAtUtc { get; }
        public DateTime? FightStartsAtUtc { get; set; }
        public DateTime? FightEndsAtUtc { get; set; }
        public bool RegionTimerStarted { get; set; }
        public int RoundFinalizeState;
        public bool RoundFinalized => Volatile.Read(ref RoundFinalizeState) != 0;
        public string? PendingTerminalStatus { get; set; }
        public SurvivalSoloPhase Phase { get; set; } = SurvivalSoloPhase.Registration;
        public ConcurrentDictionary<int, SurvivalSoloParticipant> Participants { get; } = new();
        public ConcurrentDictionary<int, SurvivalSoloState> RegisteredParties { get; } = new();
        public ConcurrentDictionary<int, SurvivalSoloScore> Scores { get; } = new();
        public ConcurrentDictionary<int, DateTime> LastDeathUtc { get; } = new();
        public ConcurrentDictionary<int, byte> DeadCharacters { get; } = new();
        public ConcurrentDictionary<int, byte> ArenaTeleportedCharIds { get; } = new();
    }

    private enum SurvivalSoloPhase
    {
        Registration,
        Preparing,
        Fighting,
        Finishing,
        Cleanup
    }

    private sealed record SurvivalSoloParticipant(
        int CharID,
        string CharName,
        int JID,
        string Hwid,
        string ClientIP,
        int PartyID,
        int MasterCharID,
        string MasterName,
        int Team);

    private sealed record SurvivalSoloState(int PartyID, int MasterCharID, string MasterName, int Team);

    private sealed record SurvivalSoloScore(int PartyID, int MasterCharID, string MasterName, int Team, int KillCount)
    {
        public string Label => MasterName;
    }

    private sealed class PartyMember
    {
        public int PartyID { get; set; }
        public int CharID { get; set; }
        public string CharName { get; set; } = string.Empty;
        public bool IsMaster { get; set; }
        public int JID { get; set; }
        public string Hwid { get; set; } = string.Empty;
        public string ClientIP { get; set; } = string.Empty;
    }

    private sealed class SurvivalSoloConfigRow
    {
        public string DisplayName { get; set; } = "Survival Solo";
        public bool Enabled { get; set; } = true;
        public int EventID { get; set; } = 13;
        public int StartDelaySeconds { get; set; } = 60;
        public int RegistrationSeconds { get; set; } = 60;
        public int FightSeconds { get; set; } = 600;
        public int MinLevel { get; set; } = 1;
        public int HwidLimit { get; set; } = 1;
        public bool RequireHwid { get; set; } = true;
        public int MaxPlayers { get; set; } = 100;
        public int ArenaWorldID { get; set; } = 107;
        public int ArenaRegionID { get; set; } = 25580;
        public int ArenaX { get; set; } = 500;
        public int ArenaY { get; set; }
        public int ArenaZ { get; set; } = 500;
    }

    private sealed class SurvivalSoloReward
    {
        public string RewardType { get; set; } = string.Empty;
        public long Amount { get; set; }
        public string? ItemCodeName128 { get; set; }
        public int? ItemID { get; set; }
        public int ItemCount { get; set; } = 1;
        public int Plus { get; set; }
    }
}
