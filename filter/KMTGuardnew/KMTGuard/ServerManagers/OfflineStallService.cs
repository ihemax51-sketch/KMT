using System.Collections.Concurrent;
using Dapper;
using KMTGuard.Database;
using KMTGuard.Database.Models;
using KMTGuard.Localization;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.ServerManagers;

public readonly record struct OfflineStallActivationResult(bool Success, byte Code, string Message);

internal sealed class OfflineStallEntry
{
    public required ISession Session { get; init; }
    public required string Username { get; init; }
    public required string CharName { get; init; }
    public required int CharId { get; init; }
    public required uint UniqueId { get; init; }
    public DateTime ActivatedAtUtc { get; init; }
    public DateTime? DetachedAtUtc { get; set; }
    public long DatabaseId { get; set; }
    public int TerminationStarted;
}

internal readonly record struct PendingStallPurchase(uint SellerUniqueId, byte StallSlot);

public static class OfflineStallService
{
    public const ushort ActivateRequestOpcode = 0x18D0;
    public const ushort ActivateResultOpcode = 0x18D1;

    private static readonly TimeSpan HeartbeatInterval = TimeSpan.FromSeconds(5);
    private static readonly TimeSpan DatabaseHeartbeatInterval = TimeSpan.FromMinutes(1);
    private static readonly TimeSpan DatabaseCommandTimeout = TimeSpan.FromSeconds(5);
    private const int DatabaseHeartbeatBatchSize = 500;
    private const int MaximumConfiguredHours = 8760;
    private static readonly object RegistryLock = new();
    private static readonly object HeartbeatLifecycleLock = new();
    private static readonly ConcurrentDictionary<string, OfflineStallEntry> ByUsername =
        new(StringComparer.OrdinalIgnoreCase);
    private static readonly ConcurrentDictionary<string, OfflineStallEntry> ByCharName =
        new(StringComparer.OrdinalIgnoreCase);
    private static readonly ConcurrentDictionary<int, OfflineStallEntry> ByCharId = new();
    private static readonly ConcurrentDictionary<uint, OfflineStallEntry> ByUniqueId = new();
    private static readonly ConcurrentDictionary<string, Guid> AutomaticLoginAccounts =
        new(StringComparer.OrdinalIgnoreCase);
    private static readonly ConcurrentDictionary<Guid, ConcurrentQueue<PendingStallPurchase>> PendingPurchases = new();
    private static readonly ConcurrentDictionary<long, Task> DatabaseOperations = new();
    private static CancellationTokenSource? _heartbeatShutdown;
    private static Task? _heartbeatWorker;
    private static long _databaseOperationSequence;
    private static volatile bool _databaseReady;
    private static volatile bool _shuttingDown;

    public static async Task InitializeAsync()
    {
        _shuttingDown = false;
        _databaseReady = false;
        if (_serverSettings.OfflineStallMaxHours < 0 ||
            _serverSettings.OfflineStallMaxHours > MaximumConfiguredHours)
        {
            Log.Error(
                "OfflineStallMaxHours value {Value} is outside the supported range 0-{Maximum}; using 24 hours",
                _serverSettings.OfflineStallMaxHours,
                MaximumConfiguredHours);
            _serverSettings.OfflineStallMaxHours = 24;
        }

        try
        {
            using var timeout = new CancellationTokenSource(DatabaseCommandTimeout);
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync(timeout.Token);
            var schemaReady = await connection.ExecuteScalarAsync<int>(new CommandDefinition(@"
SELECT CASE WHEN OBJECT_ID(N'[dbo].[Stall_Offline]', N'U') IS NOT NULL THEN 1 ELSE 0 END;",
                commandTimeout: (int)DatabaseCommandTimeout.TotalSeconds,
                cancellationToken: timeout.Token));
            if (schemaReady != 1)
            {
                Log.Error("Offline Stall is unavailable because dbo.Stall_Offline is missing. Apply the database update and restart the Filter.");
                _serverSettings.EnableOfflineStall = false;
                StartHeartbeatWorker();
                return;
            }

            await connection.ExecuteAsync(new CommandDefinition(@"
UPDATE [dbo].[Stall_Offline]
   SET [Status] = 2,
       ClosedAtUtc = SYSUTCDATETIME(),
       CloseReason = N'filter startup cleanup'
 WHERE [Status] IN (0, 1);",
                commandTimeout: (int)DatabaseCommandTimeout.TotalSeconds,
                cancellationToken: timeout.Token));
            _databaseReady = true;
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Offline Stall database readiness check failed; the feature is disabled until restart");
            _serverSettings.EnableOfflineStall = false;
        }

        StartHeartbeatWorker();
    }

    public static bool IsOfflineStallSession(ISession session) =>
        session.SessionData.OfflineStall && TryGetBySession(session, out _);

    public static bool TryGetActiveByUsername(string username, out string charName)
    {
        charName = string.Empty;
        if (string.IsNullOrWhiteSpace(username) || !ByUsername.TryGetValue(username, out var entry) ||
            Volatile.Read(ref entry.TerminationStarted) != 0)
            return false;

        charName = entry.CharName;
        return true;
    }

    public static async Task<bool> TerminateByUsernameAsync(string username, string reason)
    {
        if (string.IsNullOrWhiteSpace(username) || !ByUsername.TryGetValue(username, out var entry))
            return false;

        await TerminateAsync(entry, reason);
        return true;
    }

    public static Task<OfflineStallActivationResult> ActivateAsync(ISession session)
    {
        if (!_serverSettings.EnableOfflineStall)
            return Task.FromResult(new OfflineStallActivationResult(false, 1, PlayerLanguage.Get("OfflineStall.Disabled")));
        if (!_databaseReady)
            return Task.FromResult(new OfflineStallActivationResult(false, 8, PlayerLanguage.Get("OfflineStall.ServiceUnavailable")));
        if (_shuttingDown || session.IsStopped || !session.CharacterGameReady)
            return Task.FromResult(new OfflineStallActivationResult(false, 2, PlayerLanguage.Get("OfflineStall.CharacterNotReady")));
        if (session.SessionData.OnTransport)
            return Task.FromResult(new OfflineStallActivationResult(false, 3, PlayerLanguage.Get("OfflineStall.TransportNotAllowed")));
        if (session.SessionData.UniqueCharId == 0 || session.SessionData.Charid <= 0 ||
            string.IsNullOrWhiteSpace(session.PlayerUserID) || string.IsNullOrWhiteSpace(session.SessionData.Charname))
            return Task.FromResult(new OfflineStallActivationResult(false, 4, PlayerLanguage.Get("OfflineStall.IdentityNotReady")));
        if (!ActionManager.OpenStalls.TryGetValue(session.SessionData.UniqueCharId, out var openStall))
            return Task.FromResult(new OfflineStallActivationResult(false, 5, PlayerLanguage.Get("OfflineStall.OpenStallFirst")));
        if (!openStall.IsOperating)
            return Task.FromResult(new OfflineStallActivationResult(false, 5, PlayerLanguage.Get("OfflineStall.MustBeOperating")));
        if (openStall.Slots.IsEmpty)
            return Task.FromResult(new OfflineStallActivationResult(false, 6, PlayerLanguage.Get("OfflineStall.AddItemFirst")));

        var entry = new OfflineStallEntry
        {
            Session = session,
            Username = session.PlayerUserID.Trim(),
            CharName = session.SessionData.Charname.Trim(),
            CharId = session.SessionData.Charid,
            UniqueId = session.SessionData.UniqueCharId,
            ActivatedAtUtc = DateTime.UtcNow
        };

        lock (RegistryLock)
        {
            if (ByUsername.ContainsKey(entry.Username) || ByCharName.ContainsKey(entry.CharName) ||
                ByCharId.ContainsKey(entry.CharId) || ByUniqueId.ContainsKey(entry.UniqueId))
                return Task.FromResult(new OfflineStallActivationResult(false, 7, PlayerLanguage.Get("OfflineStall.AlreadyActive")));

            ByUsername[entry.Username] = entry;
            ByCharName[entry.CharName] = entry;
            ByCharId[entry.CharId] = entry;
            ByUniqueId[entry.UniqueId] = entry;
            session.SessionData.OfflineStall = true;
            session.SessionData.OfflineStallActivatedAtUtc = entry.ActivatedAtUtc;
        }

        QueueDatabaseWork(() => PersistActivationAsync(entry));
        Log.Information("Offline stall armed for {CharName} ({Username})", entry.CharName, entry.Username);
        return Task.FromResult(new OfflineStallActivationResult(true, 0, PlayerLanguage.Get("OfflineStall.Activated")));
    }

    public static async Task DetachAfterActivationAsync(ISession session)
    {
        try
        {
            await Task.Delay(750);
            session.TryDetachClientTransport("offline stall activated");
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to detach Offline Stall client {CharName}", session.SessionData.Charname);
            session.Stop("offline stall detach failed");
        }
    }

    public static void NotifyClientDetached(ISession session)
    {
        if (!TryGetBySession(session, out var entry) || entry.DetachedAtUtc.HasValue)
            return;

        entry.DetachedAtUtc = DateTime.UtcNow;
        session.SessionData.OfflineStallDetachedAtUtc = entry.DetachedAtUtc;
        QueueDatabaseWork(() => MarkDetachedAsync(entry));
        Log.Information("Offline stall detached for {CharName}; upstream Agent session remains alive", entry.CharName);
    }

    public static Task<bool> TryReplaceCharacterSelectionAsync(
        ISession newSession,
        string charName,
        Func<Task> replayAsync)
    {
        if (!_serverSettings.EnableOfflineStall)
            return Task.FromResult(false);

        if (!string.IsNullOrWhiteSpace(newSession.PlayerUserID) &&
            AutomaticLoginAccounts.ContainsKey(newSession.PlayerUserID))
        {
            newSession.Stop("another Offline Stall automatic login is in progress");
            return Task.FromResult(true);
        }

        if (!ByCharName.TryGetValue(charName, out var entry) ||
            ReferenceEquals(entry.Session, newSession) ||
            !string.Equals(newSession.PlayerUserID, entry.Username, StringComparison.OrdinalIgnoreCase))
            return Task.FromResult(false);

        return ReplaceForLoginAsync(newSession, entry, replayAsync);
    }

    private static async Task<bool> ReplaceForLoginAsync(
        ISession newSession,
        OfflineStallEntry entry,
        Func<Task> replayAsync)
    {
        if (newSession.IsStopped)
            return true;

        if (!AutomaticLoginAccounts.TryAdd(entry.Username, newSession.ClientGuid))
        {
            newSession.Stop("another Offline Stall automatic login is in progress");
            return true;
        }

        try
        {
            Log.Information(
                "Automatically replacing Offline Stall {CharName} during character selection",
                entry.CharName);
            await TerminateAsync(entry, "authenticated login started");
            if (!newSession.IsStopped)
                await replayAsync();
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to resume login after automatically replacing Offline Stall {CharName}", entry.CharName);
            newSession.Stop("Offline Stall automatic login replacement failed");
        }
        finally
        {
            AutomaticLoginAccounts.TryRemove(new KeyValuePair<string, Guid>(
                entry.Username,
                newSession.ClientGuid));
        }

        return true;
    }

    public static void CancelPendingForSession(ISession session)
    {
        PendingPurchases.TryRemove(session.ClientGuid, out _);
        ActionManager.CancelPendingStallActions(session.ClientGuid);
    }

    public static void TrackPendingPurchase(ISession buyer, uint sellerUniqueId, byte stallSlot)
    {
        if (sellerUniqueId != 0)
            PendingPurchases
                .GetOrAdd(buyer.ClientGuid, static _ => new ConcurrentQueue<PendingStallPurchase>())
                .Enqueue(new PendingStallPurchase(sellerUniqueId, stallSlot));
    }

    public static async Task CompletePendingPurchaseAsync(ISession buyer, bool success, byte? confirmedSlot)
    {
        if (!PendingPurchases.TryGetValue(buyer.ClientGuid, out var queue) ||
            !queue.TryDequeue(out var pending))
            return;

        if (queue.IsEmpty)
            PendingPurchases.TryRemove(
                new KeyValuePair<Guid, ConcurrentQueue<PendingStallPurchase>>(buyer.ClientGuid, queue));

        if (!success)
            return;

        var soldSlot = confirmedSlot ?? pending.StallSlot;
        if (confirmedSlot.HasValue && confirmedSlot.Value != pending.StallSlot)
        {
            Log.Warning(
                "Stall buy response slot {ConfirmedSlot} did not match requested slot {RequestedSlot} for seller {SellerUniqueId}",
                confirmedSlot.Value,
                pending.StallSlot,
                pending.SellerUniqueId);
        }

        await ReconcileSoldSlotAsync(pending.SellerUniqueId, soldSlot);
    }

    public static async Task ReconcileSoldSlotAsync(uint sellerUniqueId, byte stallSlot)
    {
        if (sellerUniqueId == 0 || stallSlot > 9)
            return;

        var remaining = ActionManager.RemoveStallSlot(sellerUniqueId, stallSlot);
        if (ActionManager.SilkStalls.TryGetValue(sellerUniqueId, out var silkStall))
            silkStall.StallPrices.TryRemove(stallSlot, out _);

        if (remaining != 0 || !ByUniqueId.ContainsKey(sellerUniqueId))
            return;

        await Task.Delay(100);
        if (ActionManager.OpenStalls.TryGetValue(sellerUniqueId, out var stall) && !stall.Slots.IsEmpty)
            return;

        await TerminateByUniqueIdAsync(sellerUniqueId, "all stall items sold");
    }

    public static Task TerminateByUniqueIdAsync(uint uniqueId, string reason)
    {
        return ByUniqueId.TryGetValue(uniqueId, out var entry)
            ? TerminateAsync(entry, reason)
            : Task.CompletedTask;
    }

    public static async Task ShutdownAsync(string reason)
    {
        _shuttingDown = true;
        await StopHeartbeatWorkerAsync();
        var entries = ByUniqueId.Values.Distinct().ToArray();
        foreach (var entry in entries)
            await TerminateAsync(entry, reason);
        await WaitForDatabaseOperationsAsync();
    }

    public static void OnSessionStopping(ISession session, string reason)
    {
        CancelPendingForSession(session);
        if (!TryGetBySession(session, out var entry) ||
            Interlocked.Exchange(ref entry.TerminationStarted, 1) != 0)
            return;

        Unregister(entry);
        session.SessionData.OfflineStall = false;
        ActionManager.CloseStall(entry.UniqueId);
        QueueDatabaseWork(() => CloseDatabaseRowAsync(entry, reason));
    }

    private static Task TerminateAsync(OfflineStallEntry entry, string reason)
    {
        if (Interlocked.Exchange(ref entry.TerminationStarted, 1) != 0)
            return Task.CompletedTask;

        Unregister(entry);
        entry.Session.SessionData.OfflineStall = false;
        ActionManager.CloseStall(entry.UniqueId);
        entry.Session.Stop("Offline Stall ended: " + reason);
        QueueDatabaseWork(() => CloseDatabaseRowAsync(entry, reason));
        Log.Information("Offline stall ended for {CharName}: {Reason}", entry.CharName, reason);
        return Task.CompletedTask;
    }

    private static void Unregister(OfflineStallEntry entry)
    {
        lock (RegistryLock)
        {
            ByUsername.TryRemove(new KeyValuePair<string, OfflineStallEntry>(entry.Username, entry));
            ByCharName.TryRemove(new KeyValuePair<string, OfflineStallEntry>(entry.CharName, entry));
            ByCharId.TryRemove(new KeyValuePair<int, OfflineStallEntry>(entry.CharId, entry));
            ByUniqueId.TryRemove(new KeyValuePair<uint, OfflineStallEntry>(entry.UniqueId, entry));
        }
    }

    private static bool TryGetBySession(ISession session, out OfflineStallEntry entry)
    {
        if (session.SessionData.UniqueCharId != 0 &&
            ByUniqueId.TryGetValue(session.SessionData.UniqueCharId, out entry!) &&
            ReferenceEquals(entry.Session, session))
            return true;

        entry = null!;
        return false;
    }

    private static void StartHeartbeatWorker()
    {
        lock (HeartbeatLifecycleLock)
        {
            if (_heartbeatWorker is { IsCompleted: false })
                return;

            _heartbeatShutdown?.Dispose();
            _heartbeatShutdown = new CancellationTokenSource();
            var cancellationToken = _heartbeatShutdown.Token;
            _heartbeatWorker = Task.Run(() => RunHeartbeatWorkerAsync(cancellationToken));
        }
    }

    private static async Task StopHeartbeatWorkerAsync()
    {
        Task? worker;
        CancellationTokenSource? shutdown;
        lock (HeartbeatLifecycleLock)
        {
            worker = _heartbeatWorker;
            shutdown = _heartbeatShutdown;
            _heartbeatWorker = null;
            _heartbeatShutdown = null;
        }

        if (shutdown == null)
            return;

        try { shutdown.Cancel(); } catch { }
        if (worker != null)
        {
            try { await worker.WaitAsync(TimeSpan.FromSeconds(5)); }
            catch (OperationCanceledException) { }
            catch (TimeoutException)
            {
                Log.Warning("Offline stall heartbeat worker did not stop within the shutdown timeout.");
            }
        }

        shutdown.Dispose();
    }

    private static async Task RunHeartbeatWorkerAsync(CancellationToken cancellationToken)
    {
        var lastDatabaseHeartbeatUtc = DateTime.MinValue;
        using var timer = new PeriodicTimer(HeartbeatInterval);
        try
        {
            while (await timer.WaitForNextTickAsync(cancellationToken))
            {
                var nowUtc = DateTime.UtcNow;
                var updateDatabase = nowUtc - lastDatabaseHeartbeatUtc >= DatabaseHeartbeatInterval;
                var databaseIds = updateDatabase ? new List<long>() : null;
                var entries = ByUniqueId.Values
                    .Where(entry => entry.DetachedAtUtc.HasValue)
                    .Distinct()
                    .ToArray();

                foreach (var entry in entries)
                {
                    if (Volatile.Read(ref entry.TerminationStarted) != 0 || entry.Session.IsStopped)
                        continue;

                    try
                    {
                        if (_serverSettings.OfflineStallMaxHours > 0 &&
                            nowUtc >= entry.ActivatedAtUtc.AddHours(_serverSettings.OfflineStallMaxHours))
                        {
                            await TerminateAsync(entry, "maximum duration reached");
                            continue;
                        }

                        await entry.Session.SendToServer(new Packet(0x2002, false, false));
                        entry.Session.LastPing = DateTime.Now;
                        if (entry.DatabaseId > 0)
                            databaseIds?.Add(entry.DatabaseId);
                    }
                    catch (Exception ex)
                    {
                        Log.Warning(ex, "Offline stall heartbeat failed for {CharName}", entry.CharName);
                        entry.Session.Stop("Offline Stall heartbeat failed");
                    }
                }

                if (updateDatabase && databaseIds is { Count: > 0 })
                {
                    lastDatabaseHeartbeatUtc = nowUtc;
                    await UpdateHeartbeatsAsync(databaseIds, cancellationToken);
                }
            }
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested) { }
    }

    private static async Task<long> InsertDatabaseRowAsync(OfflineStallEntry entry)
    {
        try
        {
            using var timeout = new CancellationTokenSource(DatabaseCommandTimeout);
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync(timeout.Token);
            return await connection.ExecuteScalarAsync<long>(new CommandDefinition(@"
IF OBJECT_ID(N'[dbo].[Stall_Offline]', N'U') IS NULL
    SELECT CONVERT(BIGINT, 0);
ELSE
BEGIN
    INSERT INTO [dbo].[Stall_Offline]
        (JID, AccountName, CharID, CharName, UniqueID, FilterSessionGuid, [Status], ActivatedAtUtc, ExpiresAtUtc)
    OUTPUT INSERTED.ID
    VALUES
        (@JID, @AccountName, @CharID, @CharName, @UniqueID, @SessionGuid, 0, @ActivatedAtUtc, @ExpiresAtUtc);
END",
                new
                {
                    JID = entry.Session.SessionData.JID,
                    AccountName = entry.Username,
                    CharID = entry.CharId,
                    CharName = entry.CharName,
                    UniqueID = (long)entry.UniqueId,
                    SessionGuid = entry.Session.ClientGuid,
                    entry.ActivatedAtUtc,
                    ExpiresAtUtc = _serverSettings.OfflineStallMaxHours > 0
                        ? entry.ActivatedAtUtc.AddHours(_serverSettings.OfflineStallMaxHours)
                        : (DateTime?)null
                },
                commandTimeout: (int)DatabaseCommandTimeout.TotalSeconds,
                cancellationToken: timeout.Token));
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Failed to persist Offline Stall activation for {CharName}", entry.CharName);
            return 0;
        }
    }

    private static async Task MarkDetachedAsync(OfflineStallEntry entry)
    {
        if (entry.DatabaseId <= 0) return;
        try
        {
            using var timeout = new CancellationTokenSource(DatabaseCommandTimeout);
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync(timeout.Token);
            await connection.ExecuteAsync(new CommandDefinition(
                "UPDATE [dbo].[Stall_Offline] SET [Status]=1, DetachedAtUtc=@DetachedAtUtc, LastHeartbeatAtUtc=SYSUTCDATETIME() WHERE ID=@ID AND [Status]=0",
                new { entry.DetachedAtUtc, ID = entry.DatabaseId },
                commandTimeout: (int)DatabaseCommandTimeout.TotalSeconds,
                cancellationToken: timeout.Token));
        }
        catch (Exception ex) { Log.Warning(ex, "Failed to mark Offline Stall detached"); }
    }

    private static async Task UpdateHeartbeatsAsync(
        IReadOnlyCollection<long> databaseIds,
        CancellationToken cancellationToken)
    {
        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync(cancellationToken);
            foreach (var batch in databaseIds.Distinct().Chunk(DatabaseHeartbeatBatchSize))
            {
                await connection.ExecuteAsync(new CommandDefinition(
                    "UPDATE [dbo].[Stall_Offline] SET LastHeartbeatAtUtc=SYSUTCDATETIME() WHERE ID IN @IDs AND [Status] IN (0,1)",
                    new { IDs = batch },
                    commandTimeout: (int)DatabaseCommandTimeout.TotalSeconds,
                    cancellationToken: cancellationToken));
            }
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested) { }
        catch (Exception ex) { Log.Debug(ex, "Offline Stall database heartbeat batch failed"); }
    }

    private static async Task CloseDatabaseRowAsync(OfflineStallEntry entry, string reason)
    {
        if (entry.DatabaseId <= 0) return;
        try
        {
            using var timeout = new CancellationTokenSource(DatabaseCommandTimeout);
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync(timeout.Token);
            await connection.ExecuteAsync(new CommandDefinition(@"
UPDATE [dbo].[Stall_Offline]
   SET [Status]=2, ClosedAtUtc=SYSUTCDATETIME(), CloseReason=@Reason
 WHERE ID=@ID AND [Status] IN (0,1)",
                new { ID = entry.DatabaseId, Reason = reason.Length > 256 ? reason[..256] : reason },
                commandTimeout: (int)DatabaseCommandTimeout.TotalSeconds,
                cancellationToken: timeout.Token));
        }
        catch (Exception ex) { Log.Warning(ex, "Failed to close Offline Stall database row"); }
    }

    private static async Task PersistActivationAsync(OfflineStallEntry entry)
    {
        entry.DatabaseId = await InsertDatabaseRowAsync(entry);
        if (entry.DatabaseId <= 0)
            return;

        if (Volatile.Read(ref entry.TerminationStarted) != 0)
            await CloseDatabaseRowAsync(entry, "Offline Stall ended before activation persistence completed");
        else if (entry.DetachedAtUtc.HasValue)
            await MarkDetachedAsync(entry);
    }

    private static void QueueDatabaseWork(Func<Task> work)
    {
        var operationId = Interlocked.Increment(ref _databaseOperationSequence);
        var task = Task.Run(work);
        DatabaseOperations[operationId] = task;
        _ = task.ContinueWith(
            completedTask => DatabaseOperations.TryRemove(operationId, out _),
            CancellationToken.None,
            TaskContinuationOptions.ExecuteSynchronously,
            TaskScheduler.Default);
    }

    private static async Task WaitForDatabaseOperationsAsync()
    {
        var operations = DatabaseOperations.Values.ToArray();
        if (operations.Length == 0)
            return;

        try
        {
            await Task.WhenAll(operations).WaitAsync(DatabaseCommandTimeout);
        }
        catch (TimeoutException)
        {
            Log.Warning(
                "Offline Stall shutdown reached the database drain timeout with {Count} operation(s) still pending",
                DatabaseOperations.Count);
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "Offline Stall database drain failed during shutdown");
        }
    }
}
