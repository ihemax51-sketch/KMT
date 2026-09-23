using System.Diagnostics;
using System.IO.Pipes;
using System.Text.Json;
using System.Collections.Concurrent;
using KMTGuard.Clientless;
using KMTGuard.Features.AutoEvents;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.RuntimeContract;
using KMTGuard.Server;
using KMTGuard.ServerManagers;
using KMTGuard.SessionManager;
using Serilog;

namespace KMTGuard.Runtime;

public sealed class RuntimeControlServer : IAsyncDisposable
{
    private static readonly JsonSerializerOptions JsonOptions = new(JsonSerializerDefaults.Web);
    private readonly FilterRole _role;
    private readonly Func<Task> _requestShutdown;
    private readonly CancellationTokenSource _shutdown = new();
    private readonly SemaphoreSlim _connections = new(16, 16);
    private readonly ConcurrentDictionary<int, Task> _clientTasks = new();
    private readonly ConcurrentDictionary<string, long> _usedNonces = new(StringComparer.Ordinal);
    private Task? _listenerTask;
    private int _nextClientTaskId;
    private const int MaximumRequestCharacters = 64 * 1024;

    public RuntimeControlServer(FilterRole role, Func<Task> requestShutdown)
    {
        _role = role;
        _requestShutdown = requestShutdown;
    }

    public void Start()
    {
        _listenerTask ??= Task.Run(ListenAsync);
    }

    private async Task ListenAsync()
    {
        while (!_shutdown.IsCancellationRequested)
        {
            NamedPipeServerStream? pipe = null;
            try
            {
                pipe = new NamedPipeServerStream(
                    RuntimeProtocol.GetPipeName(_role),
                    PipeDirection.InOut,
                    NamedPipeServerStream.MaxAllowedServerInstances,
                    PipeTransmissionMode.Byte,
                    PipeOptions.Asynchronous | PipeOptions.CurrentUserOnly);

                await pipe.WaitForConnectionAsync(_shutdown.Token);
                await _connections.WaitAsync(_shutdown.Token);
                int taskId = Interlocked.Increment(ref _nextClientTaskId);
                var clientTask = HandleClientAndDisposeAsync(pipe);
                _clientTasks[taskId] = clientTask;
                _ = clientTask.ContinueWith(
                    completedTask =>
                    {
                        _clientTasks.TryRemove(taskId, out _);
                        _connections.Release();
                    },
                    CancellationToken.None,
                    TaskContinuationOptions.ExecuteSynchronously,
                    TaskScheduler.Default);
                pipe = null;
            }
            catch (OperationCanceledException) when (_shutdown.IsCancellationRequested)
            {
                pipe?.Dispose();
                break;
            }
            catch (Exception ex)
            {
                pipe?.Dispose();
                Log.Warning(ex, "{Role} runtime control listener failed", _role);
                try
                {
                    await Task.Delay(250, _shutdown.Token);
                }
                catch (OperationCanceledException)
                {
                    break;
                }
            }
        }
    }

    private async Task HandleClientAndDisposeAsync(NamedPipeServerStream pipe)
    {
        await using (pipe)
        {
            try
            {
                using var reader = new StreamReader(pipe, leaveOpen: true);
                using var writer = new StreamWriter(pipe, leaveOpen: true) { AutoFlush = true };
                using var requestTimeout = CancellationTokenSource.CreateLinkedTokenSource(_shutdown.Token);
                requestTimeout.CancelAfter(TimeSpan.FromSeconds(5));
                var requestLine = await ReadBoundedLineAsync(reader, MaximumRequestCharacters, requestTimeout.Token);
                if (string.IsNullOrWhiteSpace(requestLine))
                    return;

                var request = JsonSerializer.Deserialize<RuntimeRequest>(requestLine, JsonOptions)
                              ?? throw new InvalidDataException("Invalid runtime control request.");
                if (!RuntimeRequestAuthentication.Verify(request, TimeSpan.FromSeconds(60)) ||
                    !_usedNonces.TryAdd(request.Nonce, request.TimestampUnixSeconds))
                    throw new UnauthorizedAccessException("Runtime control authentication failed.");
                PruneNonces();
                var response = await HandleRequestAsync(request);
                await writer.WriteLineAsync(JsonSerializer.Serialize(response, JsonOptions));
            }
            catch (OperationCanceledException) when (_shutdown.IsCancellationRequested)
            {
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "{Role} runtime control request failed", _role);
                try
                {
                    using var writer = new StreamWriter(pipe, leaveOpen: true) { AutoFlush = true };
                    await writer.WriteLineAsync(JsonSerializer.Serialize(Failure(ex.Message), JsonOptions));
                }
                catch
                {
                }
            }
        }
    }

    private static async Task<string?> ReadBoundedLineAsync(
        StreamReader reader,
        int maximumCharacters,
        CancellationToken cancellationToken)
    {
        var buffer = new char[1024];
        var value = new System.Text.StringBuilder();
        while (value.Length <= maximumCharacters)
        {
            int read = await reader.ReadAsync(buffer.AsMemory(0, buffer.Length), cancellationToken);
            if (read == 0)
                return value.Length == 0 ? null : value.ToString();

            for (int index = 0; index < read; index++)
            {
                if (buffer[index] == '\n')
                    return value.ToString().TrimEnd('\r');
                value.Append(buffer[index]);
                if (value.Length > maximumCharacters)
                    throw new InvalidDataException("Runtime control request is too large.");
            }
        }

        throw new InvalidDataException("Runtime control request is too large.");
    }

    private void PruneNonces()
    {
        long cutoff = DateTimeOffset.UtcNow.AddMinutes(-2).ToUnixTimeSeconds();
        foreach (var entry in _usedNonces)
        {
            if (entry.Value < cutoff)
                _usedNonces.TryRemove(entry.Key, out _);
        }
    }

    private async Task<RuntimeResponse> HandleRequestAsync(RuntimeRequest request)
    {
        switch (request.Command.Trim().ToLowerInvariant())
        {
            case "runtime.ping":
                return Success("Ready", await CreateRuntimeSnapshotAsync());

            case "runtime.shutdown":
                _ = Task.Run(async () =>
                {
                    await Task.Delay(100);
                    await _requestShutdown();
                });
                return Success("Shutdown requested.");

            case "runtime.language.set":
                var languageRequest = ReadPayload<PlayerLanguageCommandPayload>(request);
                var language = PlayerLanguage.Switch(languageRequest.Language);
                return Success(
                    $"Player language changed to {language.Language}.",
                    new PlayerLanguageStatusPayload
                    {
                        Language = language.Language,
                        ActiveKeyCount = language.ActiveKeyCount,
                        FallbackKeyCount = language.FallbackKeyCount,
                        MissingKeyCount = language.MissingKeyCount,
                        InvalidPlaceholderCount = language.InvalidPlaceholderCount
                    });

            case "agent.onlineplayers":
                EnsureAgentRole();
                return Success("Online player snapshot created.", CreateOnlinePlayerSnapshot());

            case "agent.clientless.status":
                EnsureAgentRole();
                return Success(await ClientlessManager.GetStatusAsync());

            case "agent.clientless.partyform.status":
                EnsureAgentRole();
                var partyFormPolicy = await ClientlessManager.GetPartyFormPolicyAsync();
                return Success(
                    "Clientless Party Form policy loaded.",
                    new ClientlessPartyFormPolicyPayload
                    {
                        Enabled = partyFormPolicy.Enabled,
                        Mode = partyFormPolicy.Mode,
                        Title = partyFormPolicy.Title,
                        MinLevel = partyFormPolicy.MinLevel,
                        MaxLevel = partyFormPolicy.MaxLevel,
                        Purpose = partyFormPolicy.Purpose,
                        SettingsFlag = partyFormPolicy.SettingsFlag
                    });

            case "agent.clientless.partyform.set":
                EnsureAgentRole();
                var partyFormPayload = ReadPayload<ClientlessPartyFormCommandPayload>(request);
                return Success(await ClientlessManager.SetPartyFormPolicyAsync(
                    partyFormPayload.Enabled,
                    partyFormPayload.Mode,
                    partyFormPayload.Title,
                    partyFormPayload.MinLevel,
                    partyFormPayload.MaxLevel,
                    partyFormPayload.Purpose,
                    partyFormPayload.SettingsFlag));

            case "agent.clientless.party.build":
                EnsureAgentRole();
                var partyBuildPayload = ReadPayload<ClientlessCommandPayload>(request);
                return Success(await ClientlessManager.BuildManagedPartiesAsync(partyBuildPayload.City));

            case "agent.clientless.start":
                EnsureAgentRole();
                var startPayload = ReadPayload<ClientlessCommandPayload>(request);
                return Success(await ClientlessManager.StartAsync(startPayload.Count, startPayload.City));

            case "agent.clientless.reload":
                EnsureAgentRole();
                var reloadPayload = ReadPayload<ClientlessCommandPayload>(request);
                return Success(await ClientlessManager.ReloadAsync(reloadPayload.Count, reloadPayload.City));

            case "agent.clientless.hunt.refresh":
                EnsureAgentRole();
                var huntRefreshPayload = ReadPayload<ClientlessCommandPayload>(request);
                return Success(await ClientlessManager.RefreshHuntingAsync(
                    huntRefreshPayload.City,
                    huntRefreshPayload.AccountId));

            case "agent.clientless.stop":
                EnsureAgentRole();
                var stopPayload = ReadPayload<ClientlessCommandPayload>(request);
                return Success(await ClientlessManager.StopAsync(stopPayload.City));

            case "agent.clientless.account.stop":
                EnsureAgentRole();
                var accountStopPayload = ReadPayload<ClientlessCommandPayload>(request);
                return accountStopPayload.AccountId is > 0
                    ? Success(await ClientlessManager.StopAccountAsync(accountStopPayload.AccountId.Value))
                    : Failure("A valid Clientless account row ID is required.");

            case "agent.clientless.system.start":
                EnsureAgentRole();
                var systemStart = ReadPayload<SystemClientlessCommandPayload>(request);
                return systemStart.Role.Equals("HNS", StringComparison.OrdinalIgnoreCase)
                    ? Success(await HideAndSeekEventService.LoginBotAsync())
                    : Failure($"Unsupported system clientless role: {systemStart.Role}");

            case "agent.clientless.system.stop":
                EnsureAgentRole();
                var systemStop = ReadPayload<SystemClientlessCommandPayload>(request);
                return systemStop.Role.Equals("HNS", StringComparison.OrdinalIgnoreCase)
                    ? Success(await HideAndSeekEventService.LogoutBotAsync())
                    : Failure($"Unsupported system clientless role: {systemStop.Role}");

            case "agent.quicklogin.publish":
                EnsureAgentRole();
                var auth = ReadPayload<QuickLoginAuthPayload>(request);
                return QuickLoginAgentAuthBridge.AcceptPublishedToken(auth)
                    ? Success("Quick-login token accepted.")
                    : Failure("Quick-login token was rejected.");

            case "agent.offlinestall.lookup":
                EnsureAgentRole();
                var lookup = ReadPayload<OfflineStallAccountPayload>(request);
                var active = OfflineStallService.TryGetActiveByUsername(lookup.Username, out var charName);
                return Success("Offline Stall lookup completed.", new OfflineStallLookupResponse
                {
                    IsActive = active,
                    CharName = charName
                });

            case "agent.offlinestall.terminate":
                EnsureAgentRole();
                var terminate = ReadPayload<OfflineStallAccountPayload>(request);
                var terminated = await OfflineStallService.TerminateByUsernameAsync(
                    terminate.Username,
                    "authenticated login started through Gateway service");
                return terminated
                    ? Success("Offline Stall terminated.")
                    : Failure("No active Offline Stall was found for this account.");

            default:
                return Failure($"Unknown runtime command: {request.Command}");
        }
    }

    private void EnsureAgentRole()
    {
        if (_role is not (FilterRole.Agent or FilterRole.All))
            throw new InvalidOperationException($"Command is not available in the {_role} service.");
    }

    private static T ReadPayload<T>(RuntimeRequest request)
    {
        if (string.IsNullOrWhiteSpace(request.Payload))
            throw new InvalidDataException("The runtime command payload is missing.");

        return JsonSerializer.Deserialize<T>(request.Payload, JsonOptions)
               ?? throw new InvalidDataException("The runtime command payload is invalid.");
    }

    private async Task<RuntimeSnapshot> CreateRuntimeSnapshotAsync()
    {
        using var process = Process.GetCurrentProcess();
        var databaseLatencyMs = -1d;
        var queueDepth = -1;
        long? oldestCommandAgeSeconds = null;
        try
        {
            var stopwatch = Stopwatch.StartNew();
            await using var connection = new Microsoft.Data.SqlClient.SqlConnection(
                global::Program.Connectionstring);
            await connection.OpenAsync();
            var queue = await Dapper.SqlMapper.QuerySingleAsync<CommandHealthRow>(connection, @"
SELECT COUNT_BIG(*) AS QueueDepth,
       MAX(AgeSeconds) AS OldestCommandAgeSeconds
FROM
(
    SELECT DATEDIFF_BIG(SECOND,CreatedUtc,SYSUTCDATETIME()) AS AgeSeconds
      FROM dbo.Command_FilterQueue WHERE Status IN (1,2,3)
    UNION ALL
    SELECT DATEDIFF_BIG(SECOND,CreatedUtc,SYSUTCDATETIME()) AS AgeSeconds
      FROM dbo.Command_PlannedQueue WHERE Status IN (1,2,3)
) Q;");
            stopwatch.Stop();
            databaseLatencyMs = stopwatch.Elapsed.TotalMilliseconds;
            queueDepth = checked((int)queue.QueueDepth);
            oldestCommandAgeSeconds = queue.OldestCommandAgeSeconds;
        }
        catch (Exception ex)
        {
            Log.Warning(ex, "{Role} health snapshot could not query database state", _role);
        }

        var listenerCount = ServerManager.Servers.Count(server => server.Started);
        var databaseWorker = DatabaseJobQueue.GetHealth();
        return new RuntimeSnapshot
        {
            Role = _role,
            ProcessId = Environment.ProcessId,
            StartedAtUtc = global::Program.EmbeddedStartedAtUtc ?? DateTime.UtcNow,
            WorkingSetBytes = process.WorkingSet64,
            PrivateMemoryBytes = process.PrivateMemorySize64,
            SessionCount = GetSessionCount(),
            ListenerCount = listenerCount,
            ListenersHealthy = listenerCount == ServerManager.Servers.Count,
            DatabaseLatencyMs = databaseLatencyMs,
            CommandQueueDepth = queueDepth,
            OldestCommandAgeSeconds = oldestCommandAgeSeconds,
            RejectedMassivePackets = SilkroadSecurityAPI.Security.RejectedMassivePackets,
            DatabaseWorkerQueueDepth = databaseWorker.Depth,
            DatabaseWorkerDroppedJobs = databaseWorker.DroppedJobs,
            DatabaseWorkerQueueAgeMs = databaseWorker.LastQueueAgeMs,
            DatabaseWorkerExecutionMs = databaseWorker.LastExecutionMs,
            CacheLastRefreshUtc = RefManager.CacheLastRefreshUtc
        };
    }

    private sealed class CommandHealthRow
    {
        public long QueueDepth { get; set; }
        public long? OldestCommandAgeSeconds { get; set; }
    }

    private int GetSessionCount()
    {
        return _role switch
        {
            FilterRole.Agent => ServerManager.AgentSessions.Count,
            FilterRole.Download => ServerManager.DownloadSessions.Count,
            FilterRole.Gateway => ServerManager.GatewaySessions.Count,
            _ => ServerManager.AgentSessions.Count +
                 ServerManager.DownloadSessions.Count +
                 ServerManager.GatewaySessions.Count
        };
    }

    private static OnlinePlayerSnapshot[] CreateOnlinePlayerSnapshot()
    {
        return ServerManager.AgentSessions
            .Where(ServerManager.IsOnlinePlayer)
            .Select(session => new OnlinePlayerSnapshot
            {
                CharName = session.SessionData.Charname,
                UserId = session.PlayerUserID,
                CharId = session.SessionData.Charid,
                UniqueId = session.SessionData.UniqueCharId,
                Ip = session.ClientIp,
                Hwid = session.SessionData.Hwid,
                Region = session.SessionData.LatestRegion,
                World = session.SessionData.WorldID,
                Level = session.SessionData.CurLevel,
                JobType = session.SessionData.JobType,
                Type = session.IsManagedClientless
                    ? "Clientless"
                    : session.IsExternalBot
                        ? "ExternalBot"
                        : "Client",
                CharacterReadyAtUtc = session.SessionData.CharacterReadyAtUtc,
                LastPing = session.LastPing == default ? null : session.LastPing
            })
            .OrderBy(player => player.CharName, StringComparer.OrdinalIgnoreCase)
            .ToArray();
    }

    private static RuntimeResponse Success(string message, object? payload = null) => new()
    {
        Success = true,
        Message = message,
        Payload = payload is null ? null : JsonSerializer.Serialize(payload, JsonOptions)
    };

    private static RuntimeResponse Failure(string message) => new()
    {
        Success = false,
        Message = message
    };

    public async ValueTask DisposeAsync()
    {
        if (!_shutdown.IsCancellationRequested)
            _shutdown.Cancel();

        if (_listenerTask != null)
        {
            try
            {
                await _listenerTask;
            }
            catch (OperationCanceledException)
            {
            }
        }

        await Task.WhenAll(_clientTasks.Values);

        _connections.Dispose();
        _shutdown.Dispose();
    }
}
