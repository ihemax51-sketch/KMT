using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Data;
using System.Text.RegularExpressions;
using Dapper;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.PacketHandlerManager;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.ServerManagers;

public static partial class PvpChallengeService
{
    private const ushort ClientOpcode = 0x2076;
    private const byte IncomingRequestAction = 0;
    private const byte StatusAction = 1;
    private const byte FinishAction = 2;
    private const byte MatchStatusWaitingArena = 7;
    private const long MaxWagerGold = long.MaxValue / 2;
    private const int ArenaArrivalTimeoutSeconds = 15;
    private const int SettlementRetryCount = 5;
    private static readonly TimeSpan WaitingQueueRetryInterval = TimeSpan.FromSeconds(10);
    private static readonly TimeSpan SettlementRetryInterval = TimeSpan.FromSeconds(2);

    private static readonly SemaphoreSlim Gate = new(1, 1);
    private static readonly ConcurrentDictionary<long, ChallengeMatch> PendingByMatchId = new();
    private static readonly ConcurrentDictionary<int, long> PendingByCharId = new();
    private static readonly ConcurrentDictionary<long, ChallengeMatch> WaitingByMatchId = new();
    private static readonly ConcurrentDictionary<int, long> WaitingByCharId = new();
    private static readonly Queue<long> WaitingQueue = new();
    private static readonly ConcurrentDictionary<long, ChallengeMatch> StartingByMatchId = new();
    private static readonly ConcurrentDictionary<int, long> StartingByCharId = new();
    private static readonly ConcurrentDictionary<long, ChallengeMatch> ActiveByMatchId = new();
    private static readonly ConcurrentDictionary<int, long> ActiveByCharId = new();
    private static readonly ConcurrentDictionary<long, byte> SettlementByMatchId = new();
    private static int WaitingQueueRetryScheduled;
    private static int Initialized;
    private static int ShuttingDown;

    [GeneratedRegex("^[A-Za-z0-9_]{2,32}$")]
    private static partial Regex CharacterNamePattern();

    public static async Task InitializeAsync()
    {
        await Gate.WaitAsync();
        try
        {
            ClearRuntimeState();
            await RecoverInterruptedMatchesAsync("Filter restarted");
            Volatile.Write(ref ShuttingDown, 0);
            Volatile.Write(ref Initialized, 1);
        }
        finally
        {
            Gate.Release();
        }
    }

    public static void BeginShutdown()
    {
        Volatile.Write(ref ShuttingDown, 1);
    }

    public static async Task ShutdownAsync(string reason)
    {
        BeginShutdown();
        await Gate.WaitAsync();
        try
        {
            var sessions = StartingByMatchId.Values
                .Concat(ActiveByMatchId.Values)
                .SelectMany(match => new[]
                {
                    FindOnlineCharacter(match.ChallengerCharId),
                    FindOnlineCharacter(match.OpponentCharId)
                })
                .Where(session => session != null)
                .DistinctBy(session => session!.SessionData.Charid)
                .ToArray();

            await RecoverInterruptedMatchesAsync(reason);

            foreach (var session in sessions)
            {
                await ApplyCapeAsync(session, 0);
                await TeleportToTownAsync(session);
            }

            ClearRuntimeState();
            Volatile.Write(ref Initialized, 0);
        }
        finally
        {
            Gate.Release();
        }
    }

    public static bool IsCapeControlled(int charId) =>
        charId > 0 && (StartingByCharId.ContainsKey(charId) || ActiveByCharId.ContainsKey(charId));

    public static bool IsTravelLocked(int charId) => IsCapeControlled(charId);

    public static bool IsCombatFrozen(int charId) =>
        charId > 0 && ActiveByCharId.ContainsKey(charId) && TeleportFreezeService.IsFrozen(charId);

    public static async Task<PacketResult?> BlockTravelIfLockedAsync(ISession session)
    {
        if (!IsTravelLocked(session.SessionData.Charid))
            return null;

        await SendWarningAsync(session, PlayerLanguage.Get("PvpChallenge.TravelLocked"));
        return new PacketResult(PacketResultType.Block);
    }

    public static async Task<PacketResult?> BlockCapeChangeIfLockedAsync(ISession session)
    {
        if (!IsCapeControlled(session.SessionData.Charid))
            return null;

        await SendWarningAsync(session, PlayerLanguage.Get("PvpChallenge.CapeLocked"));
        return new PacketResult(PacketResultType.Block);
    }

    public static async Task<PacketResult?> BlockCombatIfFrozenAsync(ISession session)
    {
        if (!IsCombatFrozen(session.SessionData.Charid))
            return null;

        await SendWarningAsync(session, PlayerLanguage.Get("PvpChallenge.CombatFrozen"));
        return new PacketResult(PacketResultType.Block);
    }

    public static async Task<PacketResult?> BlockInteractionIfLockedAsync(ISession session)
    {
        if (!IsTravelLocked(session.SessionData.Charid))
            return null;

        await SendWarningAsync(session, PlayerLanguage.Get("PvpChallenge.InteractionLocked"));
        return new PacketResult(PacketResultType.Block);
    }

    public static async Task HandleChallengeRequestAsync(ISession challenger, string targetName, long wagerGold)
    {
        targetName = (targetName ?? string.Empty).Trim();

        if (Volatile.Read(ref Initialized) == 0 || Volatile.Read(ref ShuttingDown) != 0)
        {
            await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.TemporarilyUnavailable"));
            return;
        }

        if (!_serverSettings.EnablePvpChallenge)
        {
            await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.Disabled"));
            return;
        }

        if (challenger.SessionData.Charid <= 0 || !challenger.CharacterGameReady)
        {
            await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.CharacterNotReady"));
            return;
        }

        if (!CharacterNamePattern().IsMatch(targetName))
        {
            await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.InvalidTargetName"));
            return;
        }

        if (string.Equals(challenger.SessionData.Charname, targetName, StringComparison.OrdinalIgnoreCase))
        {
            await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.CannotChallengeSelf"));
            return;
        }

        var config = await GetConfigAsync();
        if (config == null)
        {
            await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.DatabaseMissing"));
            return;
        }

        if (!config.Enabled || !config.IsValid)
        {
            await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.NotConfigured"));
            return;
        }

        if (!await HasEnabledArenaAsync())
        {
            await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.NoArenas"));
            return;
        }

        if (wagerGold < config.MinGold)
        {
            await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.MinimumWager", config.MinGold));
            return;
        }

        if (wagerGold > MaxWagerGold)
        {
            await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.MaximumWager", MaxWagerGold));
            return;
        }

        if (!CanParticipate(challenger))
        {
            await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.PlayerStateBlocked"));
            return;
        }

        var opponent = FindOnlineCharacter(targetName);
        if (opponent == null)
        {
            await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.TargetOffline"));
            return;
        }

        if (!CanParticipate(opponent))
        {
            await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.TargetStateBlocked"));
            return;
        }

        await Gate.WaitAsync();
        try
        {
            if (IsBusy(challenger.SessionData.Charid))
            {
                await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.AlreadyActive"));
                return;
            }

            if (IsBusy(opponent.SessionData.Charid))
            {
                await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.TargetAlreadyActive"));
                return;
            }

            if (!CanParticipate(challenger) || !CanParticipate(opponent))
            {
                await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.PlayerStateChanged"));
                return;
            }

            long matchId = await CreatePendingMatchAsync(challenger, opponent, wagerGold, config);
            var match = ChallengeMatch.Create(matchId, challenger, opponent, wagerGold, config);

            PendingByMatchId[matchId] = match;
            PendingByCharId[challenger.SessionData.Charid] = matchId;
            PendingByCharId[opponent.SessionData.Charid] = matchId;

            await SendIncomingRequestAsync(opponent, match);
            await SendStatusAsync(challenger, true, PlayerLanguage.Get("PvpChallenge.RequestSent", opponent.SessionData.Charname));
            _ = ExpirePendingAsync(matchId, config.RequestTimeoutSeconds);
        }
        catch (Exception ex)
        {
            Log.Error(ex, "PvP Challenge request failed for {Challenger} -> {Opponent}", challenger.SessionData.Charname, targetName);
            await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.RequestFailed"));
        }
        finally
        {
            Gate.Release();
        }
    }

    public static async Task HandleChallengeAnswerAsync(ISession opponent, long matchId, bool accepted)
    {
        await Gate.WaitAsync();
        try
        {
            if (!PendingByMatchId.TryGetValue(matchId, out var match) ||
                match.OpponentCharId != opponent.SessionData.Charid)
            {
                await SendStatusAsync(opponent, false, PlayerLanguage.Get("PvpChallenge.RequestUnavailable"));
                return;
            }

            var challenger = FindOnlineCharacter(match.ChallengerCharId);
            if (challenger == null || !challenger.CharacterGameReady)
            {
                await MarkMatchCancelledAsync(match.MatchId, "Challenger offline");
                RemovePending(match);
                await SendStatusAsync(opponent, false, PlayerLanguage.Get("PvpChallenge.ChallengerOffline"));
                return;
            }

            if (!CanParticipate(challenger) || !CanParticipate(opponent))
            {
                await MarkMatchCancelledAsync(match.MatchId, "Player state changed before acceptance");
                RemovePending(match);
                string message = PlayerLanguage.Get("PvpChallenge.PlayerStateChanged");
                await SendStatusAsync(opponent, false, message);
                await SendStatusAsync(challenger, false, message);
                return;
            }

            if (!accepted)
            {
                await MarkMatchRejectedAsync(match.MatchId);
                RemovePending(match);
                await SendStatusAsync(opponent, true, PlayerLanguage.Get("PvpChallenge.Declined"));
                await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.DeclinedBy", opponent.SessionData.Charname));
                return;
            }

            var config = await GetConfigAsync();
            if (config == null || !config.Enabled || !config.IsValid)
            {
                await MarkMatchCancelledAsync(match.MatchId, "Arena not configured");
                RemovePending(match);
                await SendStatusAsync(opponent, false, PlayerLanguage.Get("PvpChallenge.NotConfigured"));
                await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.NotConfigured"));
                return;
            }

            if (!await HasEnabledArenaAsync())
            {
                await MarkMatchCancelledAsync(match.MatchId, "No arenas configured");
                RemovePending(match);
                await SendStatusAsync(opponent, false, PlayerLanguage.Get("PvpChallenge.NoArenas"));
                await SendStatusAsync(challenger, false, PlayerLanguage.Get("PvpChallenge.NoArenas"));
                return;
            }

            match = match with
            {
                CapeType = config.CapeType,
                FreezeSeconds = config.FreezeSeconds,
                FightTimeoutSeconds = config.FightTimeoutSeconds,
                ArenaStartDelaySeconds = config.ArenaStartDelaySeconds
            };

            var escrow = await TryAcceptAndEscrowAsync(match);
            if (!escrow.Success)
            {
                await MarkMatchCancelledAsync(match.MatchId, escrow.Message);
                RemovePending(match);
                await SendStatusAsync(opponent, false, escrow.Message);
                await SendStatusAsync(challenger, false, escrow.Message);
                return;
            }

            RemovePending(match);
            WaitingByMatchId[match.MatchId] = match;
            WaitingByCharId[match.ChallengerCharId] = match.MatchId;
            WaitingByCharId[match.OpponentCharId] = match.MatchId;
            WaitingQueue.Enqueue(match.MatchId);

            await SendStatusAsync(challenger, true, PlayerLanguage.Get("PvpChallenge.AcceptedBy", opponent.SessionData.Charname));
            await SendStatusAsync(opponent, true, PlayerLanguage.Get("PvpChallenge.Accepted"));

            await TryStartWaitingMatchesLockedAsync();

            if (WaitingByMatchId.ContainsKey(match.MatchId))
            {
                string waitingMessage = PlayerLanguage.Get("PvpChallenge.ArenasBusy");
                await SendStatusAsync(challenger, true, waitingMessage);
                await SendStatusAsync(opponent, true, waitingMessage);
                ScheduleWaitingQueueRetry();
            }
        }
        catch (Exception ex)
        {
            Log.Error(ex, "PvP Challenge answer failed for MatchID={MatchId}", matchId);
            await SendStatusAsync(opponent, false, PlayerLanguage.Get("PvpChallenge.AnswerFailed"));
        }
        finally
        {
            Gate.Release();
        }
    }

    public static async Task HandleKillAsync(
        int worldId,
        int regionId,
        int killerCharId,
        string killerName,
        byte killerPvpCape,
        int deadCharId,
        string deadName,
        byte deadPvpCape)
    {
        if (killerCharId <= 0 || deadCharId <= 0 || killerCharId == deadCharId)
            return;

        if (!ActiveByCharId.TryGetValue(killerCharId, out var matchId) ||
            !ActiveByMatchId.TryGetValue(matchId, out var match))
            return;

        if (!match.Contains(deadCharId))
            return;

        ChallengeMatch originalMatch = match;

        await Gate.WaitAsync();
        try
        {
            if (!ActiveByMatchId.TryGetValue(match.MatchId, out match))
                return;

            if (worldId != match.GameWorldId || regionId != match.RegionId)
            {
                Log.Warning(
                    "Ignored PvP Challenge kill outside the assigned arena. MatchID={MatchId} World={WorldId} Region={RegionId} ExpectedWorld={ExpectedWorld} ExpectedRegion={ExpectedRegion}",
                    match.MatchId,
                    worldId,
                    regionId,
                    match.GameWorldId,
                    match.RegionId);
                return;
            }

            if (!SettlementByMatchId.TryAdd(match.MatchId, 0))
                return;

            int loserCharId = killerCharId == match.ChallengerCharId ? match.OpponentCharId : match.ChallengerCharId;
            string winnerName = string.IsNullOrWhiteSpace(killerName) ? match.NameOf(killerCharId) : killerName;
            string loserName = string.IsNullOrWhiteSpace(deadName) ? match.NameOf(loserCharId) : deadName;
            long pot = checked(match.WagerGold * 2);

            var outcome = new WinOutcome(
                killerCharId,
                winnerName,
                loserCharId,
                loserName,
                pot,
                "Kill",
                true,
                worldId,
                regionId,
                killerPvpCape,
                deadPvpCape);

            if (await SettleWinAsync(match, outcome))
                await CompleteWinRuntimeLockedAsync(match, outcome);
            else
                await CleanupAlreadySettledRuntimeLockedAsync(match);
        }
        catch (Exception ex)
        {
            Log.Error(ex, "PvP Challenge kill finish failed. MatchID={MatchId}", matchId);
            _ = RetryWinSettlementAsync(originalMatch, new WinOutcome(
                killerCharId,
                string.IsNullOrWhiteSpace(killerName) ? originalMatch.NameOf(killerCharId) : killerName,
                deadCharId,
                string.IsNullOrWhiteSpace(deadName) ? originalMatch.NameOf(deadCharId) : deadName,
                originalMatch.WagerGold <= MaxWagerGold ? originalMatch.WagerGold * 2 : 0,
                "Kill",
                true,
                worldId,
                regionId,
                killerPvpCape,
                deadPvpCape));
        }
        finally
        {
            Gate.Release();
        }
    }

    public static void HandleSessionStopping(ISession session, string reason)
    {
        if (session.SessionData.Charid <= 0 || Volatile.Read(ref ShuttingDown) != 0)
            return;

        _ = HandleParticipantUnavailableAsync(session.SessionData.Charid, reason);
    }

    public static async Task HandleRegionChangedAsync(ISession session, int worldId, int regionId)
    {
        int charId = session.SessionData.Charid;
        if (charId <= 0 || !ActiveByCharId.TryGetValue(charId, out var matchId))
            return;

        if (!ActiveByMatchId.TryGetValue(matchId, out var match) ||
            (match.GameWorldId == worldId && match.RegionId == regionId))
            return;

        await ForfeitActiveMatchAsync(charId, "Left arena", "PvpChallenge.LeftArenaForfeit");
    }

    private static async Task HandleParticipantUnavailableAsync(int charId, string reason)
    {
        await Gate.WaitAsync();
        try
        {
            if (Volatile.Read(ref ShuttingDown) != 0)
                return;

            if (PendingByCharId.TryGetValue(charId, out var pendingId) &&
                PendingByMatchId.TryGetValue(pendingId, out var pending))
            {
                await MarkMatchCancelledAsync(pending.MatchId, "Player disconnected before acceptance");
                RemovePending(pending);
                await SendFinishAsync(
                    FindOnlineCharacter(pending.OtherCharId(charId)),
                    PlayerLanguage.Get("PvpChallenge.CancelledOffline"));
                return;
            }

            ChallengeMatch? escrowed = null;
            if (WaitingByCharId.TryGetValue(charId, out var waitingId))
                WaitingByMatchId.TryGetValue(waitingId, out escrowed);
            else if (StartingByCharId.TryGetValue(charId, out var startingId))
                StartingByMatchId.TryGetValue(startingId, out escrowed);

            if (escrowed != null)
            {
                bool refunded = await CancelEscrowedMatchAsync(escrowed, "Player disconnected before fight");
                RemoveWaiting(escrowed);
                RemoveStarting(escrowed);
                if (refunded)
                {
                    string message = PlayerLanguage.Get("PvpChallenge.CancelledOfflineRefunded");
                    await SendFinishAsync(FindOnlineCharacter(escrowed.ChallengerCharId), message);
                    await SendFinishAsync(FindOnlineCharacter(escrowed.OpponentCharId), message);
                }
                await TryStartWaitingMatchesLockedAsync();
                return;
            }
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "PvP Challenge disconnect handling failed. CharID={CharId} Reason={Reason}", charId, reason);
        }
        finally
        {
            Gate.Release();
        }

        await ForfeitActiveMatchAsync(charId, "Disconnect", "PvpChallenge.DisconnectForfeit");
    }

    private static async Task ForfeitActiveMatchAsync(int loserCharId, string reason, string playerMessageKey)
    {
        await Gate.WaitAsync();
        try
        {
            if (!ActiveByCharId.TryGetValue(loserCharId, out var matchId) ||
                !ActiveByMatchId.TryGetValue(matchId, out var match) ||
                !SettlementByMatchId.TryAdd(matchId, 0))
                return;

            int winnerCharId = match.OtherCharId(loserCharId);
            var outcome = new WinOutcome(
                winnerCharId,
                match.NameOf(winnerCharId),
                loserCharId,
                match.NameOf(loserCharId),
                checked(match.WagerGold * 2),
                reason,
                false,
                match.GameWorldId,
                match.RegionId,
                0,
                0,
                PlayerLanguage.Get(
                    playerMessageKey,
                    match.NameOf(winnerCharId),
                    match.NameOf(loserCharId),
                    checked(match.WagerGold * 2)));

            if (await SettleWinAsync(match, outcome))
                await CompleteWinRuntimeLockedAsync(match, outcome);
            else
            {
                await CleanupAlreadySettledRuntimeLockedAsync(match);
            }
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "PvP Challenge forfeit settlement failed. CharID={CharId} Reason={Reason}", loserCharId, reason);
            if (ActiveByCharId.TryGetValue(loserCharId, out var matchId) &&
                ActiveByMatchId.TryGetValue(matchId, out var match))
            {
                int winnerCharId = match.OtherCharId(loserCharId);
                _ = RetryWinSettlementAsync(match, new WinOutcome(
                    winnerCharId,
                    match.NameOf(winnerCharId),
                    loserCharId,
                    match.NameOf(loserCharId),
                    match.WagerGold * 2,
                    reason,
                    false,
                    match.GameWorldId,
                    match.RegionId,
                    0,
                    0,
                    PlayerLanguage.Get(
                        playerMessageKey,
                        match.NameOf(winnerCharId),
                        match.NameOf(loserCharId),
                        match.WagerGold * 2)));
            }
        }
        finally
        {
            Gate.Release();
        }
    }

    private static void ClearRuntimeState()
    {
        PendingByMatchId.Clear();
        PendingByCharId.Clear();
        WaitingByMatchId.Clear();
        WaitingByCharId.Clear();
        lock (WaitingQueue)
            WaitingQueue.Clear();
        StartingByMatchId.Clear();
        StartingByCharId.Clear();
        ActiveByMatchId.Clear();
        ActiveByCharId.Clear();
        SettlementByMatchId.Clear();
        Interlocked.Exchange(ref WaitingQueueRetryScheduled, 0);
    }

    private static bool CanParticipate(ISession session) =>
        session.CharacterGameReady &&
        !session.IsStopped &&
        !session.ClientDetached &&
        !session.SessionData.OfflineStall &&
        !session.SessionData.isSilkStall &&
        !session.SessionData.OnTransport &&
        !ActionManager.OpenStalls.ContainsKey(session.SessionData.UniqueCharId) &&
        session.SessionData.State.LifeState == LifeState.Alive &&
        session.SessionData.State.BodyState is BodyState.None or BodyState.Berserk &&
        session.SessionData.JobType == 4;

    private static async Task<bool> WaitForArenaArrivalAsync(ChallengeMatch match, int timeoutSeconds)
    {
        DateTime deadline = DateTime.UtcNow.AddSeconds(Math.Max(1, timeoutSeconds));
        while (DateTime.UtcNow < deadline && Volatile.Read(ref ShuttingDown) == 0)
        {
            if (!StartingByMatchId.ContainsKey(match.MatchId))
                return false;

            var challenger = FindOnlineCharacter(match.ChallengerCharId);
            var opponent = FindOnlineCharacter(match.OpponentCharId);
            if (challenger == null || opponent == null)
                return false;

            if (IsAtArena(challenger, match) && IsAtArena(opponent, match))
                return true;

            await Task.Delay(100);
        }

        return false;
    }

    private static bool IsAtArena(ISession session, ChallengeMatch match) =>
        session.SessionData.WorldID == match.GameWorldId &&
        session.SessionData.LatestRegion == match.RegionId;

    private static async Task RecoverInterruptedMatchesAsync(string reason)
    {
        var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync(IsolationLevel.Serializable);

        try
        {
            int requiredObjects = await connection.ExecuteScalarAsync<int>(
                """
                SELECT COUNT(1)
                FROM sys.tables
                WHERE schema_id = SCHEMA_ID(N'dbo')
                  AND name IN (N'PVP_Settings', N'PVP_Arenas', N'PVP_Matches', N'PVP_KillLog', N'Teleport_FreezeQueue')
                """,
                transaction: transaction);

            if (requiredObjects != 5)
                throw new InvalidOperationException("PvP Challenge database contract is not installed. Apply the v3.0.4 database update.");

            await connection.ExecuteAsync(
                """
                UPDATE [dbo].[PVP_Matches]
                SET Status = 4,
                    FinishedAt = COALESCE(FinishedAt, SYSUTCDATETIME()),
                    EndReason = @Reason
                WHERE Status = 0
                """,
                new { Reason = LimitReason(reason) },
                transaction);

            var interrupted = (await connection.QueryAsync<RecoveryMatch>(
                """
                SELECT MatchID, ChallengerCharID, OpponentCharID, WagerGold
                FROM [dbo].[PVP_Matches] WITH (UPDLOCK, HOLDLOCK)
                WHERE Status IN (1, @WaitingStatus)
                ORDER BY MatchID
                """,
                new { WaitingStatus = MatchStatusWaitingArena },
                transaction)).AsList();

            foreach (var row in interrupted)
            {
                int updated = await connection.ExecuteAsync(
                    """
                    UPDATE [dbo].[PVP_Matches]
                    SET Status = 6,
                        FinishedAt = COALESCE(FinishedAt, SYSUTCDATETIME()),
                        EndReason = @Reason
                    WHERE MatchID = @MatchID
                      AND Status IN (1, @WaitingStatus)
                    """,
                    new
                    {
                        row.MatchID,
                        WaitingStatus = MatchStatusWaitingArena,
                        Reason = LimitReason(reason)
                    },
                    transaction);

                if (updated != 1)
                    continue;

                await AddGoldSafelyAsync(connection, transaction, shardDb, row.ChallengerCharID, row.WagerGold);
                await AddGoldSafelyAsync(connection, transaction, shardDb, row.OpponentCharID, row.WagerGold);
            }

            await transaction.CommitAsync();
            if (interrupted.Count > 0)
                Log.Warning("Recovered and refunded {Count} interrupted PvP Challenge matches.", interrupted.Count);
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }
    }

    private static bool IsBusy(int charId) =>
        PendingByCharId.ContainsKey(charId) ||
        WaitingByCharId.ContainsKey(charId) ||
        StartingByCharId.ContainsKey(charId) ||
        ActiveByCharId.ContainsKey(charId);

    private static void RemovePending(ChallengeMatch match)
    {
        PendingByMatchId.TryRemove(match.MatchId, out _);
        PendingByCharId.TryRemove(match.ChallengerCharId, out _);
        PendingByCharId.TryRemove(match.OpponentCharId, out _);
    }

    private static void RemoveWaiting(ChallengeMatch match)
    {
        WaitingByMatchId.TryRemove(match.MatchId, out _);
        WaitingByCharId.TryRemove(match.ChallengerCharId, out _);
        WaitingByCharId.TryRemove(match.OpponentCharId, out _);
    }

    private static void AddStarting(ChallengeMatch match)
    {
        StartingByMatchId[match.MatchId] = match;
        StartingByCharId[match.ChallengerCharId] = match.MatchId;
        StartingByCharId[match.OpponentCharId] = match.MatchId;
    }

    private static void RemoveStarting(ChallengeMatch match)
    {
        StartingByMatchId.TryRemove(match.MatchId, out _);
        StartingByCharId.TryRemove(match.ChallengerCharId, out _);
        StartingByCharId.TryRemove(match.OpponentCharId, out _);
    }

    private static void AddActive(ChallengeMatch match)
    {
        ActiveByMatchId[match.MatchId] = match;
        ActiveByCharId[match.ChallengerCharId] = match.MatchId;
        ActiveByCharId[match.OpponentCharId] = match.MatchId;
    }

    private static void RemoveActive(ChallengeMatch match)
    {
        ActiveByMatchId.TryRemove(match.MatchId, out _);
        ActiveByCharId.TryRemove(match.ChallengerCharId, out _);
        ActiveByCharId.TryRemove(match.OpponentCharId, out _);
    }

    private static async Task ExpirePendingAsync(long matchId, int timeoutSeconds)
    {
        await Task.Delay(TimeSpan.FromSeconds(Math.Max(5, timeoutSeconds)));
        await Gate.WaitAsync();
        try
        {
            if (!PendingByMatchId.TryGetValue(matchId, out var match))
                return;

            await MarkMatchExpiredAsync(match.MatchId);
            RemovePending(match);

            await SendStatusAsync(FindOnlineCharacter(match.ChallengerCharId), false, PlayerLanguage.Get("PvpChallenge.RequestTimedOut"));
            await SendStatusAsync(FindOnlineCharacter(match.OpponentCharId), false, PlayerLanguage.Get("PvpChallenge.RequestTimedOut"));
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "PvP Challenge pending timeout failed. MatchID={MatchId}", matchId);
        }
        finally
        {
            Gate.Release();
        }
    }

    private static async Task ExpireActiveAsync(long matchId, int timeoutSeconds)
    {
        await Task.Delay(TimeSpan.FromSeconds(Math.Max(30, timeoutSeconds)));
        await Gate.WaitAsync();
        try
        {
            if (!ActiveByMatchId.TryGetValue(matchId, out var match))
                return;

            if (!SettlementByMatchId.TryAdd(matchId, 0))
                return;

            if (await RefundDrawAsync(match))
                await CompleteDrawRuntimeLockedAsync(match);
            else
            {
                await CleanupAlreadySettledRuntimeLockedAsync(match);
            }
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "PvP Challenge active timeout failed. MatchID={MatchId}", matchId);
            if (ActiveByMatchId.TryGetValue(matchId, out var match))
                _ = RetryDrawSettlementAsync(match);
        }
        finally
        {
            Gate.Release();
        }
    }

    private static async Task TryStartWaitingMatchesLockedAsync()
    {
        try
        {
            while (WaitingQueue.Count > 0)
            {
                long queuedMatchId = WaitingQueue.Peek();

                if (!WaitingByMatchId.TryGetValue(queuedMatchId, out var match))
                {
                    WaitingQueue.Dequeue();
                    continue;
                }

                var challenger = FindOnlineCharacter(match.ChallengerCharId);
                var opponent = FindOnlineCharacter(match.OpponentCharId);
                if (challenger == null || !CanParticipate(challenger) || opponent == null || !CanParticipate(opponent))
                {
                    await CancelEscrowedMatchAsync(match, "Player offline before arena start");
                    WaitingQueue.Dequeue();
                    RemoveWaiting(match);
                    string offlineMessage = PlayerLanguage.Get("PvpChallenge.CancelledOfflineRefunded");
                    await SendFinishAsync(challenger, offlineMessage);
                    await SendFinishAsync(opponent, offlineMessage);
                    continue;
                }

                var startingMatch = await TryClaimArenaAndStartAsync(match);
                if (startingMatch == null)
                {
                    ScheduleWaitingQueueRetry();
                    return;
                }

                WaitingQueue.Dequeue();
                RemoveWaiting(match);
                AddStarting(startingMatch);
                BeginMatchCountdown(startingMatch);
            }
        }
        catch
        {
            ScheduleWaitingQueueRetry();
            throw;
        }
    }

    private static void BeginMatchCountdown(ChallengeMatch match)
    {
        _ = StartMatchAfterCountdownAsync(match);
    }

    private static async Task StartMatchAfterCountdownAsync(ChallengeMatch match)
    {
        try
        {
            int delaySeconds = Math.Clamp(match.ArenaStartDelaySeconds, 0, 300);
            string arenaName = string.IsNullOrWhiteSpace(match.ArenaName)
                ? PlayerLanguage.Get("PvpChallenge.DefaultArenaName")
                : match.ArenaName;
            string countdownMessage = delaySeconds > 0
                ? PlayerLanguage.Get("PvpChallenge.RoundStartsIn", delaySeconds, arenaName)
                : PlayerLanguage.Get("PvpChallenge.RoundStartingNow", arenaName);

            await SendStatusAsync(FindOnlineCharacter(match.ChallengerCharId), true, countdownMessage);
            await SendStatusAsync(FindOnlineCharacter(match.OpponentCharId), true, countdownMessage);

            if (delaySeconds > 0)
                await Task.Delay(TimeSpan.FromSeconds(delaySeconds));

            await Gate.WaitAsync();
            try
            {
                if (!StartingByMatchId.TryGetValue(match.MatchId, out var startingMatch))
                    return;

                var challenger = FindOnlineCharacter(startingMatch.ChallengerCharId);
                var opponent = FindOnlineCharacter(startingMatch.OpponentCharId);
                if (challenger == null || !CanParticipate(challenger) || opponent == null || !CanParticipate(opponent))
                {
                    await CancelEscrowedMatchAsync(startingMatch, "Player offline before arena start");
                    RemoveStarting(startingMatch);
                    string offlineMessage = PlayerLanguage.Get("PvpChallenge.CancelledOfflineRefunded");
                    await SendFinishAsync(challenger, offlineMessage);
                    await SendFinishAsync(opponent, offlineMessage);
                    await TryStartWaitingMatchesLockedAsync();
                    return;
                }

                await TeleportToPositionAsync(challenger, startingMatch.GameWorldId, startingMatch.RegionId, startingMatch.PosX, startingMatch.PosY, startingMatch.PosZ, startingMatch.FreezeSeconds);
                await TeleportToPositionAsync(opponent, startingMatch.GameWorldId, startingMatch.RegionId, startingMatch.PosX, startingMatch.PosY, startingMatch.PosZ, startingMatch.FreezeSeconds);
            }
            finally
            {
                Gate.Release();
            }

            bool arrived = await WaitForArenaArrivalAsync(match, ArenaArrivalTimeoutSeconds);

            await Gate.WaitAsync();
            try
            {
                if (!StartingByMatchId.TryGetValue(match.MatchId, out var startingMatch))
                    return;

                var challenger = FindOnlineCharacter(startingMatch.ChallengerCharId);
                var opponent = FindOnlineCharacter(startingMatch.OpponentCharId);
                if (!arrived || challenger == null || !CanParticipate(challenger) || opponent == null || !CanParticipate(opponent))
                {
                    await CancelEscrowedMatchAsync(startingMatch, "Arena arrival failed");
                    RemoveStarting(startingMatch);
                    string message = PlayerLanguage.Get("PvpChallenge.ArenaArrivalFailedRefunded");
                    await SendFinishAsync(challenger, message);
                    await SendFinishAsync(opponent, message);
                    await TryStartWaitingMatchesLockedAsync();
                    return;
                }

                await MarkMatchStartedNowAsync(startingMatch.MatchId);
                RemoveStarting(startingMatch);
                AddActive(startingMatch);

                int freezeSeconds = Math.Clamp(startingMatch.FreezeSeconds, 0, 600);
                TeleportFreezeService.Freeze(startingMatch.ChallengerCharId, freezeSeconds);
                TeleportFreezeService.Freeze(startingMatch.OpponentCharId, freezeSeconds);
                await ApplyCapeAsync(challenger, startingMatch.CapeType);
                await ApplyCapeAsync(opponent, startingMatch.CapeType);
                _ = ReapplyChallengeCapeAsync(startingMatch.MatchId, startingMatch.CapeType, startingMatch.ChallengerCharId, startingMatch.OpponentCharId);
                _ = SendFreezeEndingNoticeAsync(startingMatch);

                string startMessage = freezeSeconds > 0
                    ? PlayerLanguage.Get("PvpChallenge.ArenaReachedFrozen", startingMatch.WagerGold, freezeSeconds)
                    : PlayerLanguage.Get("PvpChallenge.ArenaReachedStartNow", startingMatch.WagerGold);
                await SendStatusAsync(challenger, true, startMessage);
                await SendStatusAsync(opponent, true, startMessage);

                _ = ExpireActiveAsync(
                    startingMatch.MatchId,
                    checked(startingMatch.FightTimeoutSeconds + freezeSeconds));
            }
            finally
            {
                Gate.Release();
            }
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "PvP Challenge countdown failed. MatchID={MatchId}", match.MatchId);
            await Gate.WaitAsync();
            try
            {
                if (StartingByMatchId.TryGetValue(match.MatchId, out var startingMatch))
                {
                    await CancelEscrowedMatchAsync(startingMatch, "Countdown failed");
                    RemoveStarting(startingMatch);
                    string message = PlayerLanguage.Get("PvpChallenge.CancelledBeforeStartRefunded");
                    await SendFinishAsync(FindOnlineCharacter(startingMatch.ChallengerCharId), message);
                    await SendFinishAsync(FindOnlineCharacter(startingMatch.OpponentCharId), message);
                    await TryStartWaitingMatchesLockedAsync();
                }
                else if (ActiveByMatchId.TryGetValue(match.MatchId, out var activeMatch) &&
                         SettlementByMatchId.TryAdd(match.MatchId, 0))
                {
                    if (await RefundDrawAsync(activeMatch))
                        await CompleteDrawRuntimeLockedAsync(activeMatch);
                    else
                        await CleanupAlreadySettledRuntimeLockedAsync(activeMatch);
                }
            }
            finally
            {
                Gate.Release();
            }
        }
    }

    private static void ScheduleWaitingQueueRetry()
    {
        if (Interlocked.Exchange(ref WaitingQueueRetryScheduled, 1) == 1)
            return;

        _ = RetryWaitingQueueAsync();
    }

    private static async Task RetryWaitingQueueAsync()
    {
        try
        {
            while (true)
            {
                await Task.Delay(WaitingQueueRetryInterval);

                await Gate.WaitAsync();
                try
                {
                    if (WaitingByMatchId.IsEmpty)
                        return;

                    await TryStartWaitingMatchesLockedAsync();

                    if (WaitingByMatchId.IsEmpty)
                        return;
                }
                finally
                {
                    Gate.Release();
                }
            }
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "PvP Challenge waiting queue retry failed.");
        }
        finally
        {
            Interlocked.Exchange(ref WaitingQueueRetryScheduled, 0);

            if (!WaitingByMatchId.IsEmpty)
                ScheduleWaitingQueueRetry();
        }
    }

    private static async Task<ChallengeConfig?> GetConfigAsync()
    {
        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            return await connection.QueryFirstOrDefaultAsync<ChallengeConfig>(
                """
                SELECT TOP (1)
                    Enabled,
                    GameWorldID,
                    RegionID,
                    PosX,
                    PosY,
                    PosZ,
                    TownGameWorldID,
                    TownRegionID,
                    TownPosX,
                    TownPosY,
                    TownPosZ,
                    CapeType,
                    MinGold,
                    RequestTimeoutSeconds,
                    FightTimeoutSeconds,
                    ArenaStartDelaySeconds,
                    FreezeSeconds
                FROM [dbo].[PVP_Settings] WITH (NOLOCK)
                ORDER BY ID
                """);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "PvP Challenge config read failed.");
            return null;
        }
    }

    private static async Task<bool> HasEnabledArenaAsync()
    {
        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            int count = await connection.ExecuteScalarAsync<int>(
                """
                SELECT COUNT(1)
                FROM [dbo].[PVP_Arenas] WITH (NOLOCK)
                WHERE Enabled = 1
                  AND GameWorldID > 0
                  AND RegionID > 0
                """);

            return count > 0;
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "PvP Challenge arena list read failed.");
            return false;
        }
    }

    private static async Task<long> CreatePendingMatchAsync(ISession challenger, ISession opponent, long wagerGold, ChallengeConfig config)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        return await connection.QuerySingleAsync<long>(
            """
            INSERT INTO [dbo].[PVP_Matches]
                (ChallengerCharID, ChallengerCharName, OpponentCharID, OpponentCharName,
                 WagerGold, ArenaGameWorldID, ArenaRegionID, ArenaPosX, ArenaPosY, ArenaPosZ,
                 CapeType, Status, RequestedAt)
            OUTPUT INSERTED.MatchID
            VALUES
                (@ChallengerCharID, @ChallengerCharName, @OpponentCharID, @OpponentCharName,
                 @WagerGold, @ArenaGameWorldID, @ArenaRegionID, @ArenaPosX, @ArenaPosY, @ArenaPosZ,
                 @CapeType, 0, SYSUTCDATETIME())
            """,
            new
            {
                ChallengerCharID = challenger.SessionData.Charid,
                ChallengerCharName = challenger.SessionData.Charname,
                OpponentCharID = opponent.SessionData.Charid,
                OpponentCharName = opponent.SessionData.Charname,
                WagerGold = wagerGold,
                ArenaGameWorldID = config.GameWorldID,
                ArenaRegionID = config.RegionID,
                ArenaPosX = config.PosX,
                ArenaPosY = config.PosY,
                ArenaPosZ = config.PosZ,
                CapeType = config.CapeType
            });
    }

    private static async Task<ChallengeMatch?> TryClaimArenaAndStartAsync(ChallengeMatch match)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync(IsolationLevel.Serializable);

        try
        {
            var arena = await connection.QueryFirstOrDefaultAsync<ArenaSlot>(
                """
                SELECT TOP (1)
                    a.ArenaID,
                    a.ArenaName,
                    a.GameWorldID,
                    a.RegionID,
                    a.PosX,
                    a.PosY,
                    a.PosZ
                FROM [dbo].[PVP_Arenas] AS a WITH (UPDLOCK, HOLDLOCK, ROWLOCK)
                WHERE a.Enabled = 1
                  AND a.GameWorldID > 0
                  AND a.RegionID > 0
                  AND NOT EXISTS
                  (
                      SELECT 1
                      FROM [dbo].[PVP_Matches] AS m WITH (UPDLOCK, HOLDLOCK)
                      WHERE m.ArenaID = a.ArenaID
                        AND m.Status = 1
                  )
                ORDER BY a.SortOrder, a.ArenaID
                """,
                transaction: transaction);

            if (arena == null)
            {
                await transaction.RollbackAsync();
                return null;
            }

            int updated = await connection.ExecuteAsync(
                """
                UPDATE [dbo].[PVP_Matches]
                SET Status = 1,
                    ArenaID = @ArenaID,
                    ArenaGameWorldID = @GameWorldID,
                    ArenaRegionID = @RegionID,
                    ArenaPosX = @PosX,
                    ArenaPosY = @PosY,
                    ArenaPosZ = @PosZ,
                    CapeType = @CapeType
                WHERE MatchID = @MatchID
                  AND Status = @WaitingStatus
                """,
                new
                {
                    match.MatchID,
                    arena.ArenaID,
                    arena.GameWorldID,
                    arena.RegionID,
                    arena.PosX,
                    arena.PosY,
                    arena.PosZ,
                    match.CapeType,
                    WaitingStatus = MatchStatusWaitingArena
                },
                transaction);

            if (updated != 1)
            {
                await transaction.RollbackAsync();
                return null;
            }

            await transaction.CommitAsync();

            return match with
            {
                ArenaId = arena.ArenaID,
                ArenaName = arena.ArenaName,
                GameWorldId = arena.GameWorldID,
                RegionId = arena.RegionID,
                PosX = arena.PosX,
                PosY = arena.PosY,
                PosZ = arena.PosZ
            };
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }
    }

    private static async Task<EscrowResult> TryAcceptAndEscrowAsync(ChallengeMatch match)
    {
        var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync(IsolationLevel.Serializable);

        try
        {
            int matchUpdated = await connection.ExecuteAsync(
                """
                UPDATE [dbo].[PVP_Matches]
                SET Status = @WaitingStatus,
                    AcceptedAt = SYSUTCDATETIME(),
                    QueuedAt = SYSUTCDATETIME(),
                    CapeType = @CapeType
                WHERE MatchID = @MatchID AND Status = 0
                """,
                new
                {
                    match.MatchID,
                    match.CapeType,
                    WaitingStatus = MatchStatusWaitingArena
                },
                transaction);

            if (matchUpdated != 1)
            {
                await transaction.RollbackAsync();
                return new EscrowResult(false, PlayerLanguage.Get("PvpChallenge.RequestUnavailable"));
            }

            int challengerGold = await DeductGoldAsync(connection, transaction, shardDb, match.ChallengerCharId, match.WagerGold);
            int opponentGold = await DeductGoldAsync(connection, transaction, shardDb, match.OpponentCharId, match.WagerGold);

            if (challengerGold != 1 || opponentGold != 1)
            {
                await transaction.RollbackAsync();
                return new EscrowResult(false, PlayerLanguage.Get("PvpChallenge.PlayerInsufficientGold"));
            }

            await transaction.CommitAsync();
            await SyncGoldDeltaAsync(match.ChallengerCharId, -match.WagerGold);
            await SyncGoldDeltaAsync(match.OpponentCharId, -match.WagerGold);
            return new EscrowResult(true, PlayerLanguage.Get("PvpChallenge.Accepted"));
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }
    }

    private static Task<int> DeductGoldAsync(SqlConnection connection, SqlTransaction transaction, string shardDb, int charId, long amount)
    {
        return connection.ExecuteAsync(
            $"""
             UPDATE {shardDb}.._Char
             SET RemainGold = RemainGold - @Amount
             WHERE CharID = @CharID
               AND @Amount > 0
               AND @Amount <= @MaxWager
               AND RemainGold >= @Amount
               AND RemainGold <= @MaxGold - @Amount
             """,
            new { CharID = charId, Amount = amount, MaxWager = MaxWagerGold, MaxGold = long.MaxValue },
            transaction);
    }

    private static async Task<bool> SettleWinAsync(ChallengeMatch match, WinOutcome outcome)
    {
        var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync(IsolationLevel.Serializable);

        try
        {
            int matchUpdated = await connection.ExecuteAsync(
                """
                UPDATE [dbo].[PVP_Matches]
                SET Status = 2,
                    FinishedAt = SYSUTCDATETIME(),
                    WinnerCharID = @WinnerCharID,
                    WinnerCharName = @WinnerCharName,
                    LoserCharID = @LoserCharID,
                    LoserCharName = @LoserCharName,
                    EndReason = @EndReason
                WHERE MatchID = @MatchID
                  AND Status = 1
                """,
                new
                {
                    match.MatchID,
                    WinnerCharID = outcome.WinnerCharId,
                    WinnerCharName = outcome.WinnerName,
                    LoserCharID = outcome.LoserCharId,
                    LoserCharName = outcome.LoserName,
                    EndReason = LimitReason(outcome.EndReason)
                },
                transaction);

            if (matchUpdated != 1)
            {
                await transaction.RollbackAsync();
                return false;
            }

            await AddGoldSafelyAsync(connection, transaction, shardDb, outcome.WinnerCharId, outcome.Pot);

            if (outcome.WriteKillLog)
            {
                await connection.ExecuteAsync(
                    """
                    INSERT INTO [dbo].[PVP_KillLog]
                        (MatchID, KillerCharID, KillerCharName, DeadCharID, DeadCharName,
                         WorldID, RegionID, KillerPvpCape, DeadPvpCape, CreatedAt)
                    VALUES
                        (@MatchID, @KillerCharID, @KillerCharName, @DeadCharID, @DeadCharName,
                         @WorldID, @RegionID, @KillerPvpCape, @DeadPvpCape, SYSUTCDATETIME())
                    """,
                    new
                    {
                        match.MatchID,
                        KillerCharID = outcome.WinnerCharId,
                        KillerCharName = outcome.WinnerName,
                        DeadCharID = outcome.LoserCharId,
                        DeadCharName = outcome.LoserName,
                        WorldID = outcome.WorldId,
                        RegionID = outcome.RegionId,
                        outcome.KillerPvpCape,
                        outcome.DeadPvpCape
                    },
                    transaction);
            }

            await transaction.CommitAsync();
            await SyncGoldDeltaAsync(outcome.WinnerCharId, outcome.Pot);
            return true;
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }
    }

    private static async Task<bool> RefundDrawAsync(ChallengeMatch match)
    {
        var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync(IsolationLevel.Serializable);

        try
        {
            int matchUpdated = await connection.ExecuteAsync(
                """
                UPDATE [dbo].[PVP_Matches]
                SET Status = 5,
                    FinishedAt = SYSUTCDATETIME(),
                    EndReason = N'Timeout'
                WHERE MatchID = @MatchID
                  AND Status = 1
                """,
                new { match.MatchID },
                transaction);

            if (matchUpdated != 1)
            {
                await transaction.RollbackAsync();
                return false;
            }

            await AddGoldSafelyAsync(connection, transaction, shardDb, match.ChallengerCharId, match.WagerGold);
            await AddGoldSafelyAsync(connection, transaction, shardDb, match.OpponentCharId, match.WagerGold);

            await transaction.CommitAsync();
            await SyncGoldDeltaAsync(match.ChallengerCharId, match.WagerGold);
            await SyncGoldDeltaAsync(match.OpponentCharId, match.WagerGold);
            return true;
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }
    }

    private static async Task<bool> CancelEscrowedMatchAsync(ChallengeMatch match, string reason)
    {
        var shardDb = SqlIdentifier.Quote(_serverSettings.ShardDB);
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync();
        await using var transaction = (SqlTransaction)await connection.BeginTransactionAsync(IsolationLevel.Serializable);

        try
        {
            int matchUpdated = await connection.ExecuteAsync(
                """
                UPDATE [dbo].[PVP_Matches]
                SET Status = 6,
                    FinishedAt = SYSUTCDATETIME(),
                    EndReason = @Reason
                WHERE MatchID = @MatchID
                  AND Status IN (1, @WaitingStatus)
                """,
                new
                {
                    match.MatchID,
                    Reason = LimitReason(reason),
                    WaitingStatus = MatchStatusWaitingArena
                },
                transaction);

            if (matchUpdated != 1)
            {
                await transaction.RollbackAsync();
                return false;
            }

            await AddGoldSafelyAsync(connection, transaction, shardDb, match.ChallengerCharId, match.WagerGold);
            await AddGoldSafelyAsync(connection, transaction, shardDb, match.OpponentCharId, match.WagerGold);

            await transaction.CommitAsync();
            await SyncGoldDeltaAsync(match.ChallengerCharId, match.WagerGold);
            await SyncGoldDeltaAsync(match.OpponentCharId, match.WagerGold);
            return true;
        }
        catch
        {
            await transaction.RollbackAsync();
            throw;
        }
    }

    private static async Task AddGoldSafelyAsync(
        SqlConnection connection,
        SqlTransaction transaction,
        string shardDb,
        int charId,
        long amount)
    {
        int updated = await connection.ExecuteAsync(
            $"""
             UPDATE {shardDb}.._Char
             SET RemainGold = RemainGold + @Amount
             WHERE CharID = @CharID
               AND @Amount > 0
               AND RemainGold <= @MaxGold - @Amount
             """,
            new { CharID = charId, Amount = amount, MaxGold = long.MaxValue },
            transaction);

        if (updated != 1)
            throw new InvalidOperationException($"Unable to credit PvP Challenge gold safely for CharID={charId}.");
    }

    private static string LimitReason(string reason) =>
        string.IsNullOrWhiteSpace(reason)
            ? "PvP Challenge cancelled"
            : reason.Length <= 128 ? reason : reason[..128];

    private static async Task SyncGoldDeltaAsync(int charId, long delta)
    {
        var session = FindOnlineCharacter(charId);
        if (session == null || delta == 0)
            return;

        try
        {
            var packet = new Packet(0x3542, false, false);
            packet.WriteAscii(session.GameServerPacketKey);
            packet.WriteInt64(delta);
            await session.SendToServer(packet);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "PvP Challenge live Gold synchronization failed. CharID={CharId} Delta={Delta}", charId, delta);
        }
    }

    private static async Task MarkMatchStartedNowAsync(long matchId)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        int updated = await connection.ExecuteAsync(
            """
            UPDATE [dbo].[PVP_Matches]
            SET StartedAt = SYSUTCDATETIME()
            WHERE MatchID = @MatchID
              AND Status = 1
            """,
            new { MatchID = matchId });

        if (updated != 1)
            throw new InvalidOperationException($"PvP Challenge match {matchId} could not enter the active state.");
    }

    private static async Task MarkMatchRejectedAsync(long matchId)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.ExecuteAsync(
            """
            UPDATE [dbo].[PVP_Matches]
            SET Status = 3,
                FinishedAt = SYSUTCDATETIME(),
                EndReason = N'Rejected'
            WHERE MatchID = @MatchID
              AND Status = 0
            """,
            new { MatchID = matchId });
    }

    private static async Task MarkMatchExpiredAsync(long matchId)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.ExecuteAsync(
            """
            UPDATE [dbo].[PVP_Matches]
            SET Status = 4,
                FinishedAt = SYSUTCDATETIME(),
                EndReason = N'Request timeout'
            WHERE MatchID = @MatchID
              AND Status = 0
            """,
            new { MatchID = matchId });
    }

    private static async Task MarkMatchCancelledAsync(long matchId, string reason)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.ExecuteAsync(
            """
            UPDATE [dbo].[PVP_Matches]
            SET Status = 6,
                FinishedAt = SYSUTCDATETIME(),
                EndReason = @Reason
            WHERE MatchID = @MatchID
              AND Status = 0
            """,
            new { MatchID = matchId, Reason = reason });
    }

    private static async Task TeleportToTownAsync(ISession? session)
    {
        var config = await GetConfigAsync();
        if (config == null)
            return;

        await TeleportToPositionAsync(
            session,
            config.TownGameWorldID,
            config.TownRegionID,
            config.TownPosX,
            config.TownPosY,
            config.TownPosZ);
    }

    private static Task TeleportToPositionAsync(ISession? session, int gameWorldId, int regionId, int posX, int posY, int posZ, int freezeSeconds = 0)
    {
        return TeleportFreezeService.TeleportToPositionAsync(session, gameWorldId, regionId, posX, posY, posZ, freezeSeconds);
    }

    private static async Task ApplyCapeAsync(ISession? session, byte cape)
    {
        if (session == null || !session.CharacterGameReady)
            return;

        var packet = new Packet(0x3502);
        packet.WriteAscii(session.GameServerPacketKey);
        packet.WriteUInt8(cape);
        await session.SendToServer(packet);
    }

    private static async Task ReapplyChallengeCapeAsync(long matchId, byte cape, int challengerCharId, int opponentCharId)
    {
        try
        {
            for (int i = 0; i < 2; i++)
            {
                await Task.Delay(1500);
                if (!ActiveByMatchId.ContainsKey(matchId))
                    return;

                await ApplyCapeAsync(FindOnlineCharacter(challengerCharId), cape);
                await ApplyCapeAsync(FindOnlineCharacter(opponentCharId), cape);
            }
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "PvP Challenge cape reapply failed. MatchID={MatchId}", matchId);
        }
    }

    private static async Task SendFreezeEndingNoticeAsync(ChallengeMatch match)
    {
        int freezeSeconds = Math.Clamp(match.FreezeSeconds, 0, 600);
        if (freezeSeconds <= 0)
            return;

        try
        {
            int delaySeconds = Math.Max(0, freezeSeconds - 1);
            if (delaySeconds > 0)
                await Task.Delay(TimeSpan.FromSeconds(delaySeconds));

            if (!ActiveByMatchId.ContainsKey(match.MatchId))
                return;

            string message = PlayerLanguage.Get("PvpChallenge.FreezeEnding");
            await SendStatusAsync(FindOnlineCharacter(match.ChallengerCharId), true, message);
            await SendStatusAsync(FindOnlineCharacter(match.OpponentCharId), true, message);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "PvP Challenge freeze ending notice failed. MatchID={MatchId}", match.MatchId);
        }
    }

    private static async Task ReapplyCapeClearAsync(int challengerCharId, int opponentCharId)
    {
        try
        {
            await Task.Delay(1500);
            await ApplyCapeAsync(FindOnlineCharacter(challengerCharId), 0);
            await ApplyCapeAsync(FindOnlineCharacter(opponentCharId), 0);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "PvP Challenge cape clear reapply failed. Challenger={ChallengerCharId} Opponent={OpponentCharId}", challengerCharId, opponentCharId);
        }
    }

    private static async Task CompleteWinRuntimeLockedAsync(ChallengeMatch match, WinOutcome outcome)
    {
        RemoveActive(match);
        SettlementByMatchId.TryRemove(match.MatchId, out _);

        var winnerSession = FindOnlineCharacter(outcome.WinnerCharId);
        var loserSession = FindOnlineCharacter(outcome.LoserCharId);
        await TeleportToTownAsync(FindOnlineCharacter(match.ChallengerCharId));
        await TeleportToTownAsync(FindOnlineCharacter(match.OpponentCharId));
        await ApplyCapeAsync(winnerSession, 0);
        await ApplyCapeAsync(loserSession, 0);
        _ = ReapplyCapeClearAsync(match.ChallengerCharId, match.OpponentCharId);

        string message = outcome.WriteKillLog
            ? PlayerLanguage.Get("PvpChallenge.Winner", outcome.WinnerName, outcome.LoserName, outcome.Pot)
            : outcome.PlayerMessage;
        await SendFinishAsync(winnerSession, message);
        await SendFinishAsync(loserSession, message);
        await TryStartWaitingMatchesLockedAsync();
    }

    private static async Task CompleteDrawRuntimeLockedAsync(ChallengeMatch match)
    {
        RemoveActive(match);
        SettlementByMatchId.TryRemove(match.MatchId, out _);
        await TeleportToTownAsync(FindOnlineCharacter(match.ChallengerCharId));
        await TeleportToTownAsync(FindOnlineCharacter(match.OpponentCharId));
        await ApplyCapeAsync(FindOnlineCharacter(match.ChallengerCharId), 0);
        await ApplyCapeAsync(FindOnlineCharacter(match.OpponentCharId), 0);
        _ = ReapplyCapeClearAsync(match.ChallengerCharId, match.OpponentCharId);

        string message = PlayerLanguage.Get("PvpChallenge.DrawRefunded");
        await SendFinishAsync(FindOnlineCharacter(match.ChallengerCharId), message);
        await SendFinishAsync(FindOnlineCharacter(match.OpponentCharId), message);
        await TryStartWaitingMatchesLockedAsync();
    }

    private static async Task CleanupAlreadySettledRuntimeLockedAsync(ChallengeMatch match)
    {
        RemoveActive(match);
        SettlementByMatchId.TryRemove(match.MatchId, out _);
        await TeleportToTownAsync(FindOnlineCharacter(match.ChallengerCharId));
        await TeleportToTownAsync(FindOnlineCharacter(match.OpponentCharId));
        await ApplyCapeAsync(FindOnlineCharacter(match.ChallengerCharId), 0);
        await ApplyCapeAsync(FindOnlineCharacter(match.OpponentCharId), 0);
        _ = ReapplyCapeClearAsync(match.ChallengerCharId, match.OpponentCharId);
        await TryStartWaitingMatchesLockedAsync();
    }

    private static async Task RetryWinSettlementAsync(ChallengeMatch match, WinOutcome outcome)
    {
        int attempts = 0;
        while (Volatile.Read(ref ShuttingDown) == 0 && ActiveByMatchId.ContainsKey(match.MatchId))
        {
            attempts++;
            await Task.Delay(attempts <= SettlementRetryCount ? SettlementRetryInterval : TimeSpan.FromSeconds(15));
            await Gate.WaitAsync();
            try
            {
                if (!ActiveByMatchId.ContainsKey(match.MatchId))
                    return;

                SettlementByMatchId.TryAdd(match.MatchId, 0);
                if (await SettleWinAsync(match, outcome))
                {
                    await CompleteWinRuntimeLockedAsync(match, outcome);
                    return;
                }

                await CleanupAlreadySettledRuntimeLockedAsync(match);
                return;
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "PvP Challenge win settlement retry {Attempt} failed. MatchID={MatchId}", attempts, match.MatchId);
            }
            finally
            {
                Gate.Release();
            }
        }
    }

    private static async Task RetryDrawSettlementAsync(ChallengeMatch match)
    {
        int attempts = 0;
        while (Volatile.Read(ref ShuttingDown) == 0 && ActiveByMatchId.ContainsKey(match.MatchId))
        {
            attempts++;
            await Task.Delay(attempts <= SettlementRetryCount ? SettlementRetryInterval : TimeSpan.FromSeconds(15));
            await Gate.WaitAsync();
            try
            {
                if (!ActiveByMatchId.ContainsKey(match.MatchId))
                    return;

                SettlementByMatchId.TryAdd(match.MatchId, 0);
                if (await RefundDrawAsync(match))
                {
                    await CompleteDrawRuntimeLockedAsync(match);
                    return;
                }

                await CleanupAlreadySettledRuntimeLockedAsync(match);
                return;
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "PvP Challenge draw settlement retry {Attempt} failed. MatchID={MatchId}", attempts, match.MatchId);
            }
            finally
            {
                Gate.Release();
            }
        }
    }

    private static async Task SendWarningAsync(ISession? session, string message)
    {
        if (session == null)
            return;

        var notice = new Packet(0x168A);
        notice.WriteUInt8(NoticeType.WARNING);
        notice.WriteUnicode(message);
        await session.SendToClient(notice);
    }

    private static ISession? FindOnlineCharacter(string charName)
    {
        return ServerManager.AgentSessions.FindByCharName(
            charName,
            static session => session.CharacterGameReady &&
                              !session.IsStopped &&
                              !session.ClientDetached &&
                              session.SessionData.Charid > 0);
    }

    private static ISession? FindOnlineCharacter(int charId)
    {
        return ServerManager.AgentSessions.FindByCharId(
            charId,
            static session => session.CharacterGameReady && !session.IsStopped && !session.ClientDetached);
    }

    private static async Task SendIncomingRequestAsync(ISession target, ChallengeMatch match)
    {
        var packet = new Packet(ClientOpcode);
        packet.WriteUInt8(IncomingRequestAction);
        packet.WriteInt64(match.MatchID);
        packet.WriteUnicode(match.ChallengerName);
        packet.WriteInt64(match.WagerGold);
        packet.WriteInt32(match.RequestTimeoutSeconds);
        await target.SendToClient(packet);
    }

    private static async Task SendStatusAsync(ISession? session, bool success, string message)
    {
        if (session == null)
            return;

        var packet = new Packet(ClientOpcode);
        packet.WriteUInt8(StatusAction);
        packet.WriteUInt8(success ? 1 : 0);
        packet.WriteUnicode(message);
        await session.SendToClient(packet);
    }

    private static async Task SendFinishAsync(ISession? session, string message)
    {
        if (session == null)
            return;

        var packet = new Packet(ClientOpcode);
        packet.WriteUInt8(FinishAction);
        packet.WriteUnicode(message);
        await session.SendToClient(packet);
    }

    private sealed class ChallengeConfig
    {
        public bool Enabled { get; set; }
        public int GameWorldID { get; set; }
        public int RegionID { get; set; }
        public int PosX { get; set; }
        public int PosY { get; set; }
        public int PosZ { get; set; }
        public int TownGameWorldID { get; set; } = 1;
        public int TownRegionID { get; set; } = 25000;
        public int TownPosX { get; set; }
        public int TownPosY { get; set; }
        public int TownPosZ { get; set; }
        public byte CapeType { get; set; } = (byte)PVPCape.Yellow;
        public long MinGold { get; set; } = 1;
        public int RequestTimeoutSeconds { get; set; } = 60;
        public int FightTimeoutSeconds { get; set; } = 300;
        public int ArenaStartDelaySeconds { get; set; } = 10;
        public int FreezeSeconds { get; set; } = 3;

        public bool IsValid =>
            CapeType >= 1 &&
            CapeType <= 5 &&
            MinGold >= 1 &&
            MinGold <= MaxWagerGold &&
            TownGameWorldID > 0 &&
            TownRegionID > 0 &&
            RequestTimeoutSeconds >= 5 &&
            RequestTimeoutSeconds <= 3600 &&
            FightTimeoutSeconds >= 30 &&
            FightTimeoutSeconds <= 86400 &&
            ArenaStartDelaySeconds >= 0 &&
            ArenaStartDelaySeconds <= 300 &&
            FreezeSeconds >= 0 &&
            FreezeSeconds <= 600;
    }

    private sealed class ArenaSlot
    {
        public int ArenaID { get; set; }
        public string ArenaName { get; set; } = "Arena";
        public int GameWorldID { get; set; }
        public int RegionID { get; set; }
        public int PosX { get; set; }
        public int PosY { get; set; }
        public int PosZ { get; set; }
    }

    private sealed record EscrowResult(bool Success, string Message);

    private sealed class RecoveryMatch
    {
        public long MatchID { get; set; }
        public int ChallengerCharID { get; set; }
        public int OpponentCharID { get; set; }
        public long WagerGold { get; set; }
    }

    private sealed record WinOutcome(
        int WinnerCharId,
        string WinnerName,
        int LoserCharId,
        string LoserName,
        long Pot,
        string EndReason,
        bool WriteKillLog,
        int WorldId,
        int RegionId,
        byte KillerPvpCape,
        byte DeadPvpCape,
        string PlayerMessage = "");

    private sealed record ChallengeMatch(
        long MatchID,
        int ChallengerCharId,
        string ChallengerName,
        int OpponentCharId,
        string OpponentName,
        long WagerGold,
        int GameWorldId,
        int RegionId,
        int PosX,
        int PosY,
        int PosZ,
        byte CapeType,
        int RequestTimeoutSeconds,
        int FreezeSeconds,
        int FightTimeoutSeconds,
        int ArenaStartDelaySeconds,
        int? ArenaId,
        string ArenaName)
    {
        public long MatchId => MatchID;

        public static ChallengeMatch Create(long matchId, ISession challenger, ISession opponent, long wagerGold, ChallengeConfig config)
        {
            return new ChallengeMatch(
                matchId,
                challenger.SessionData.Charid,
                challenger.SessionData.Charname,
                opponent.SessionData.Charid,
                opponent.SessionData.Charname,
                wagerGold,
                config.GameWorldID,
                config.RegionID,
                config.PosX,
                config.PosY,
                config.PosZ,
                config.CapeType,
                config.RequestTimeoutSeconds,
                config.FreezeSeconds,
                config.FightTimeoutSeconds,
                config.ArenaStartDelaySeconds,
                null,
                string.Empty);
        }

        public bool Contains(int charId) => charId == ChallengerCharId || charId == OpponentCharId;

        public int OtherCharId(int charId) =>
            charId == ChallengerCharId ? OpponentCharId :
            charId == OpponentCharId ? ChallengerCharId : 0;

        public string NameOf(int charId)
        {
            if (charId == ChallengerCharId)
                return ChallengerName;
            if (charId == OpponentCharId)
                return OpponentName;
            return string.Empty;
        }
    }
}
