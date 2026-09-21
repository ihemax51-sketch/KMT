using System.Data;
using System.Diagnostics;
using System.IO;
using KMTGuard.AdminDesktop.Models;
using KMTGuard.RuntimeContract;

namespace KMTGuard.AdminDesktop.Services;

public sealed class FilterRuntimeService
{
    private sealed record ServiceDefinition(FilterRole Role, string ExecutableName);

    private static readonly ServiceDefinition[] StartOrder =
    {
        new(FilterRole.Agent, "KMTGuard.Agent.exe"),
        new(FilterRole.Download, "KMTGuard.Download.exe"),
        new(FilterRole.Gateway, "KMTGuard.Gateway.exe")
    };

    private static readonly ServiceDefinition[] StopOrder = StartOrder.Reverse().ToArray();
    private static readonly TimeSpan StartupTimeout = TimeSpan.FromSeconds(120);
    private static readonly SemaphoreSlim LifecycleGate = new(1, 1);

    public bool IsAgentRunning
    {
        get
        {
            return IsRoleRunning(FilterRole.Agent);
        }
    }

    public FilterProcessStatus GetStatus()
    {
        var services = StartOrder.Select(CreateServiceStatus).ToArray();
        var running = services.Count(service => service.IsRunning);
        return new FilterProcessStatus
        {
            IsRunning = running == services.Length,
            AnyRunning = running > 0,
            Services = services,
            Message = running switch
            {
                0 => "All services stopped",
                3 => "All services running",
                _ => $"{running}/3 services running"
            }
        };
    }

    public Task<FilterProcessStatus> GetStatusAsync() => GetStatusWithRuntimeMetricsAsync();

    public Task<FilterProcessStatus> StartAsync() =>
        RunLifecycleOperationAsync(StartAllCoreAsync);

    public Task<FilterProcessStatus> StopAsync() =>
        RunLifecycleOperationAsync(StopAllCoreAsync);

    public Task<FilterProcessStatus> RestartAsync() =>
        RunLifecycleOperationAsync(async () =>
        {
            await StopAllCoreAsync().ConfigureAwait(false);
            return await StartAllCoreAsync().ConfigureAwait(false);
        });

    public async Task<int> SetPlayerLanguageAsync(string language, string rollbackLanguage)
    {
        var runningRoles = GetStatus().Services
            .Where(service => service.IsRunning)
            .Select(service => service.Role)
            .ToArray();
        var updatedRoles = new List<FilterRole>();

        try
        {
            foreach (var role in runningRoles)
            {
                var response = await RuntimePipeClient.SendAsync(
                    role,
                    "runtime.language.set",
                    new PlayerLanguageCommandPayload { Language = language },
                    timeout: TimeSpan.FromSeconds(10)).ConfigureAwait(false);
                if (!response.Success)
                    throw new InvalidOperationException($"{role}: {response.Message}");
                updatedRoles.Add(role);
            }

            return updatedRoles.Count;
        }
        catch
        {
            foreach (var role in updatedRoles.AsEnumerable().Reverse())
            {
                try
                {
                    await RuntimePipeClient.SendAsync(
                        role,
                        "runtime.language.set",
                        new PlayerLanguageCommandPayload { Language = rollbackLanguage },
                        timeout: TimeSpan.FromSeconds(5)).ConfigureAwait(false);
                }
                catch
                {
                }
            }

            throw;
        }
    }

    public Task<FilterProcessStatus> StartRoleAsync(FilterRole role) =>
        RunLifecycleOperationAsync(() => StartRoleOperationCoreAsync(role));

    public Task<FilterProcessStatus> StopRoleAsync(FilterRole role) =>
        RunLifecycleOperationAsync(() => StopRoleOperationCoreAsync(role));

    public Task<FilterProcessStatus> RestartRoleAsync(FilterRole role) =>
        RunLifecycleOperationAsync(() => RestartRoleOperationCoreAsync(role));

    private async Task<FilterProcessStatus> StartAllCoreAsync()
    {
        var startedHere = new List<FilterRole>();
        try
        {
            foreach (var service in StartOrder)
            {
                if (IsRoleRunning(service.Role))
                    continue;

                await StartRoleCoreAsync(service);
                startedHere.Add(service.Role);
            }

            return await GetStatusWithRuntimeMetricsAsync();
        }
        catch
        {
            foreach (var role in startedHere.AsEnumerable().Reverse())
            {
                try { await StopRoleCoreAsync(role); } catch { }
            }

            throw;
        }
    }

    private async Task<FilterProcessStatus> StopAllCoreAsync()
    {
        var failures = new List<Exception>();
        foreach (var service in StopOrder)
        {
            try
            {
                await StopRoleCoreAsync(service.Role).ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                failures.Add(new InvalidOperationException(
                    $"KMTGuard {service.Role} could not be stopped: {ex.Message}",
                    ex));
            }
        }

        if (failures.Count > 0)
        {
            throw new AggregateException(
                $"One or more KMTGuard services could not be stopped. " +
                string.Join(" | ", failures.Select(failure => failure.Message)),
                failures);
        }

        return GetStatus();
    }

    private async Task<FilterProcessStatus> StartRoleOperationCoreAsync(FilterRole role)
    {
        var service = GetDefinition(role);
        if (!IsRoleRunning(role))
            await StartRoleCoreAsync(service).ConfigureAwait(false);
        return await GetStatusWithRuntimeMetricsAsync().ConfigureAwait(false);
    }

    private async Task<FilterProcessStatus> StopRoleOperationCoreAsync(FilterRole role)
    {
        await StopRoleCoreAsync(role).ConfigureAwait(false);
        return await GetStatusWithRuntimeMetricsAsync().ConfigureAwait(false);
    }

    private async Task<FilterProcessStatus> RestartRoleOperationCoreAsync(FilterRole role)
    {
        await StopRoleCoreAsync(role).ConfigureAwait(false);
        await StartRoleCoreAsync(GetDefinition(role)).ConfigureAwait(false);
        return await GetStatusWithRuntimeMetricsAsync().ConfigureAwait(false);
    }

    private static async Task<T> RunLifecycleOperationAsync<T>(Func<Task<T>> operation)
    {
        await LifecycleGate.WaitAsync().ConfigureAwait(false);
        try
        {
            return await operation().ConfigureAwait(false);
        }
        finally
        {
            LifecycleGate.Release();
        }
    }

    public async Task<DataTable> LoadOnlinePlayersSnapshotAsync()
    {
        var table = CreateOnlinePlayersTable();
        if (!IsAgentRunning)
            return table;

        var players = await RuntimePipeClient.SendForPayloadAsync<OnlinePlayerSnapshot[]>(
                          FilterRole.Agent,
                          "agent.onlinePlayers",
                          timeout: TimeSpan.FromSeconds(5))
                      ?? Array.Empty<OnlinePlayerSnapshot>();
        var now = DateTime.UtcNow;
        foreach (var player in players)
        {
            table.Rows.Add(
                player.CharName,
                player.UserId,
                player.CharId,
                player.UniqueId,
                player.Ip,
                player.Hwid,
                player.Region,
                player.World,
                player.Level,
                player.JobType,
                player.Type,
                true,
                FormatDuration(now - player.CharacterReadyAtUtc),
                player.LastPing?.ToLocalTime().ToString("HH:mm:ss") ?? string.Empty);
        }

        table.DefaultView.Sort = "Ready DESC, CharName ASC";
        return table;
    }

    public async Task<int> GetAgentConnectionCountAsync()
    {
        if (!IsAgentRunning)
            return 0;

        var snapshot = await RuntimePipeClient.SendForPayloadAsync<RuntimeSnapshot>(
            FilterRole.Agent,
            "runtime.ping",
            timeout: TimeSpan.FromSeconds(3));
        return snapshot?.SessionCount ?? 0;
    }

    public Task<string> GetClientlessStatusAsync() => SendAgentMessageAsync("agent.clientless.status");

    public Task<ClientlessPartyFormPolicyPayload?> GetClientlessPartyFormPolicyAsync() =>
        RuntimePipeClient.SendForPayloadAsync<ClientlessPartyFormPolicyPayload>(
            FilterRole.Agent,
            "agent.clientless.partyForm.status",
            timeout: TimeSpan.FromSeconds(5));

    public Task<string> SetClientlessPartyFormPolicyAsync(ClientlessPartyFormCommandPayload payload) =>
        SendAgentMessageAsync(
            "agent.clientless.partyForm.set",
            payload,
            TimeSpan.FromSeconds(15));

    public Task<string> BuildClientlessPartiesAsync(string? city = null) =>
        SendAgentMessageAsync(
            "agent.clientless.party.build",
            new ClientlessCommandPayload { City = city },
            TimeSpan.FromSeconds(90));

    public Task<string> StartClientlessAsync(int? count, string? city = null) =>
        SendAgentMessageAsync(
            "agent.clientless.start",
            new ClientlessCommandPayload { Count = count, City = city },
            TimeSpan.FromSeconds(15));

    public Task<string> ReloadClientlessAsync(int? count, string? city = null) =>
        SendAgentMessageAsync(
            "agent.clientless.reload",
            new ClientlessCommandPayload { Count = count, City = city },
            TimeSpan.FromSeconds(15));

    public Task<string> RefreshClientlessHuntingAsync(string? city = null, int? accountId = null) =>
        SendAgentMessageAsync(
            "agent.clientless.hunt.refresh",
            new ClientlessCommandPayload { City = city, AccountId = accountId },
            TimeSpan.FromSeconds(15));

    public Task<string> StopClientlessAsync(string? city = null) =>
        SendAgentMessageAsync(
            "agent.clientless.stop",
            new ClientlessCommandPayload { City = city },
            TimeSpan.FromSeconds(15));

    public Task<string> StopClientlessAccountAsync(int accountId) =>
        SendAgentMessageAsync(
            "agent.clientless.account.stop",
            new ClientlessCommandPayload { AccountId = accountId },
            TimeSpan.FromSeconds(15));

    public Task<string> StartSystemClientlessAsync(string role) =>
        SendAgentMessageAsync(
            "agent.clientless.system.start",
            new SystemClientlessCommandPayload { Role = role },
            TimeSpan.FromSeconds(15));

    public Task<string> StopSystemClientlessAsync(string role) =>
        SendAgentMessageAsync(
            "agent.clientless.system.stop",
            new SystemClientlessCommandPayload { Role = role },
            TimeSpan.FromSeconds(15));

    public string ReadRecentLogLines(int lineCount = 80)
    {
        var sections = new List<string>();
        foreach (var definition in StartOrder)
        {
            var content = ReadRecentLogLines(definition.Role, Math.Max(10, lineCount / 3));
            if (!string.IsNullOrWhiteSpace(content))
                sections.Add($"[{definition.Role}]\r\n{content}");
        }

        return sections.Count == 0
            ? "No KMTGuard service log files found yet."
            : string.Join(Environment.NewLine + Environment.NewLine, sections);
    }

    public string ReadRecentLogLines(FilterRole role, int lineCount = 80)
    {
        var logFile = FindLatestLogFile(role);
        if (logFile == null)
            return $"KMT/{role.ToString().ToUpperInvariant()} | Waiting for the first service event...";

        try
        {
            return LogService.ReadLastLines(
                logFile,
                Math.Max(10, lineCount),
                256 * 1024);
        }
        catch (Exception ex)
        {
            return $"KMT/{role.ToString().ToUpperInvariant()} | Log stream unavailable | {ex.Message}";
        }
    }

    private async Task StartRoleCoreAsync(ServiceDefinition definition)
    {
        var executable = FindExecutable(definition);
        if (executable == null)
            throw new FileNotFoundException(
                $"{definition.ExecutableName} was not found beside KMTGuard.exe. Publish the split service build first.");

        var settingsPath = ResolveSettingsPath(executable);

        var runtimeDirectory = Path.GetDirectoryName(settingsPath)!;
        var startInfo = new ProcessStartInfo
        {
            FileName = executable,
            WorkingDirectory = runtimeDirectory,
            UseShellExecute = false,
            CreateNoWindow = true,
            WindowStyle = ProcessWindowStyle.Hidden
        };
        startInfo.Environment["KMTGUARD_SETTINGS_PATH"] = settingsPath;

        using var process = Process.Start(startInfo)
                            ?? throw new InvalidOperationException($"Windows could not start {definition.ExecutableName}.");
        await WaitUntilReadyAsync(definition.Role, process);
    }

    private async Task StopRoleCoreAsync(FilterRole role)
    {
        using var process = FindRunningProcess(role);
        if (process == null)
            return;

        try
        {
            await RuntimePipeClient.SendAsync(role, "runtime.shutdown", timeout: TimeSpan.FromSeconds(3));
        }
        catch
        {
        }

        using var shutdownTimeout = new CancellationTokenSource(TimeSpan.FromSeconds(20));
        try
        {
            await process.WaitForExitAsync(shutdownTimeout.Token);
        }
        catch (OperationCanceledException)
        {
            process.Kill(entireProcessTree: true);
            await process.WaitForExitAsync();
        }
    }

    private async Task WaitUntilReadyAsync(FilterRole role, Process process)
    {
        var deadline = DateTime.UtcNow + StartupTimeout;
        Exception? lastError = null;
        while (DateTime.UtcNow < deadline)
        {
            if (process.HasExited)
                throw new InvalidOperationException(
                    $"KMTGuard {role} exited during startup with code {process.ExitCode}.\r\n" +
                    ReadRoleLogTail(role, 12, process.StartTime.ToUniversalTime().AddSeconds(-2)));

            try
            {
                var response = await RuntimePipeClient.SendAsync(
                    role,
                    "runtime.ping",
                    timeout: TimeSpan.FromMilliseconds(750));
                if (response.Success)
                    return;
            }
            catch (Exception ex)
            {
                lastError = ex;
            }

            await Task.Delay(200);
        }

        try
        {
            if (!process.HasExited)
                process.Kill(entireProcessTree: true);
        }
        catch
        {
        }

        throw new TimeoutException(
            $"KMTGuard {role} did not become ready within {StartupTimeout.TotalSeconds:N0} seconds. " +
            (lastError?.Message ?? string.Empty));
    }

    private async Task<FilterProcessStatus> GetStatusWithRuntimeMetricsAsync()
    {
        var status = GetStatus();
        foreach (var service in status.Services.Where(service => service.IsRunning))
        {
            try
            {
                var snapshot = await RuntimePipeClient.SendForPayloadAsync<RuntimeSnapshot>(
                    service.Role,
                    "runtime.ping",
                    timeout: TimeSpan.FromSeconds(2));
                if (snapshot == null)
                    continue;

                service.MemoryMb = Math.Round(snapshot.WorkingSetBytes / 1024d / 1024d, 1);
                service.Sessions = snapshot.SessionCount;
                service.Listeners = snapshot.ListenerCount;
            }
            catch
            {
                service.Status = "Starting";
            }
        }

        return status;
    }

    private static FilterServiceStatus CreateServiceStatus(ServiceDefinition definition)
    {
        using var process = FindRunningProcess(definition.Role);
        if (process == null)
        {
            return new FilterServiceStatus
            {
                Role = definition.Role,
                Status = "Stopped",
                Executable = definition.ExecutableName
            };
        }

        DateTime? startTime = null;
        long workingSet = 0;
        try
        {
            startTime = process.StartTime;
            workingSet = process.WorkingSet64;
        }
        catch
        {
        }

        return new FilterServiceStatus
        {
            Role = definition.Role,
            Status = "Running",
            IsRunning = true,
            ProcessId = process.Id,
            MemoryMb = Math.Round(workingSet / 1024d / 1024d, 1),
            Uptime = startTime.HasValue ? FormatDuration(DateTime.Now - startTime.Value) : "-",
            Executable = definition.ExecutableName
        };
    }

    private static Process? FindRunningProcess(FilterRole role)
    {
        var definition = GetDefinition(role);
        var executable = FindExecutable(definition);
        if (!TryGetFullPath(executable, out var targetExecutablePath))
            return null;

        var processName = Path.GetFileNameWithoutExtension(definition.ExecutableName);
        Process? selected = null;
        var selectedStartTime = DateTime.MaxValue;

        foreach (var process in Process.GetProcessesByName(processName))
        {
            try
            {
                if (process.HasExited)
                    continue;

                var runningExecutable = process.MainModule?.FileName;
                if (!TryGetFullPath(runningExecutable, out var runningExecutablePath) ||
                    !string.Equals(
                        runningExecutablePath,
                        targetExecutablePath,
                        StringComparison.OrdinalIgnoreCase))
                {
                    continue;
                }

                DateTime startTime;
                try
                {
                    startTime = process.StartTime;
                }
                catch
                {
                    startTime = DateTime.MaxValue;
                }

                if (selected != null && startTime >= selectedStartTime)
                    continue;

                selected?.Dispose();
                selected = process;
                selectedStartTime = startTime;
            }
            catch
            {
                // Fail closed when Windows does not allow reading MainModule.FileName.
            }
            finally
            {
                if (!ReferenceEquals(process, selected))
                    process.Dispose();
            }
        }

        return selected;
    }

    private static bool IsRoleRunning(FilterRole role)
    {
        using var process = FindRunningProcess(role);
        return process != null;
    }

    private static ServiceDefinition GetDefinition(FilterRole role) =>
        StartOrder.FirstOrDefault(service => service.Role == role)
        ?? throw new ArgumentOutOfRangeException(nameof(role), role, "Unsupported KMTGuard service role.");

    private static string? FindExecutable(ServiceDefinition definition)
    {
        var candidates = new[]
        {
            Path.Combine(AppContext.BaseDirectory, definition.ExecutableName),
            Path.Combine(Environment.CurrentDirectory, definition.ExecutableName),
            Path.Combine(@"D:\KMTGuard-build\Filter", definition.ExecutableName),
            Path.Combine(
                AppContext.BaseDirectory,
                $@"..\..\..\..\..\filter\KMTGuardnew\KMTGuard.{definition.Role}Host\bin\Release\net8.0\win-x64\{definition.ExecutableName}")
        };

        foreach (var candidate in candidates)
        {
            if (TryGetFullPath(candidate, out var fullPath) && File.Exists(fullPath))
                return fullPath;
        }

        return null;
    }

    private static string ResolveSettingsPath(string executable)
    {
        var configuredPath = Environment.GetEnvironmentVariable("KMTGUARD_SETTINGS_PATH");
        if (TryGetExistingFileFullPath(configuredPath, out var settingsPath))
            return settingsPath;

        var fallbackCandidates = new[]
        {
            Path.Combine(AppContext.BaseDirectory, "Settings.json"),
            Path.Combine(Path.GetDirectoryName(executable)!, "Settings.json")
        };

        foreach (var candidate in fallbackCandidates)
        {
            if (TryGetExistingFileFullPath(candidate, out settingsPath))
                return settingsPath;
        }

        throw new FileNotFoundException(
            "Settings.json was not found. Select an existing settings file in the dashboard " +
            "or place it beside the KMTGuard runtime.",
            fallbackCandidates[^1]);
    }

    private static bool TryGetExistingFileFullPath(string? path, out string fullPath)
    {
        if (TryGetFullPath(path, out fullPath) && File.Exists(fullPath))
            return true;

        fullPath = string.Empty;
        return false;
    }

    private static bool TryGetFullPath(string? path, out string fullPath)
    {
        fullPath = string.Empty;
        if (string.IsNullOrWhiteSpace(path))
            return false;

        try
        {
            fullPath = Path.GetFullPath(path.Trim());
            return true;
        }
        catch
        {
            return false;
        }
    }

    private async Task<string> SendAgentMessageAsync(
        string command,
        object? payload = null,
        TimeSpan? timeout = null)
    {
        if (!IsAgentRunning)
            throw new InvalidOperationException("KMTGuard Agent service is stopped.");

        var response = await RuntimePipeClient.SendAsync(
            FilterRole.Agent,
            command,
            payload,
            timeout ?? TimeSpan.FromSeconds(5));
        if (!response.Success)
            throw new InvalidOperationException(response.Message);
        return response.Message;
    }

    private static DataTable CreateOnlinePlayersTable()
    {
        var table = new DataTable();
        table.Columns.Add("CharName", typeof(string));
        table.Columns.Add("UserID", typeof(string));
        table.Columns.Add("CharID", typeof(int));
        table.Columns.Add("UniqueID", typeof(uint));
        table.Columns.Add("IP", typeof(string));
        table.Columns.Add("HWID", typeof(string));
        table.Columns.Add("Region", typeof(int));
        table.Columns.Add("World", typeof(int));
        table.Columns.Add("Level", typeof(byte));
        table.Columns.Add("JobType", typeof(byte));
        table.Columns.Add("Type", typeof(string));
        table.Columns.Add("Ready", typeof(bool));
        table.Columns.Add("OnlineFor", typeof(string));
        table.Columns.Add("LastPing", typeof(string));
        return table;
    }

    private static string? FindLatestLogFile(FilterRole role)
    {
        var directories = new List<string>
        {
            Path.Combine(AppContext.BaseDirectory, "logs"),
            Path.Combine(Environment.CurrentDirectory, "logs")
        };
        var executable = FindExecutable(GetDefinition(role));
        if (!string.IsNullOrWhiteSpace(executable))
            directories.Add(Path.Combine(Path.GetDirectoryName(executable)!, "logs"));

        return directories
            .Distinct(StringComparer.OrdinalIgnoreCase)
            .Where(Directory.Exists)
            .SelectMany(directory => Directory.EnumerateFiles(
                directory,
                $"kmtguard-{role.ToString().ToLowerInvariant()}-*.log"))
            .OrderByDescending(File.GetLastWriteTimeUtc)
            .FirstOrDefault();
    }

    private static string ReadRoleLogTail(FilterRole role, int lineCount, DateTime? notBeforeUtc = null)
    {
        var path = FindLatestLogFile(role);
        if (path == null)
            return "No service log was created.";
        if (notBeforeUtc.HasValue && File.GetLastWriteTimeUtc(path) < notBeforeUtc.Value)
            return "No service log was created for the current startup attempt. Check Settings.json and startup prerequisites.";

        try
        {
            return LogService.ReadLastLines(path, lineCount, 128 * 1024);
        }
        catch (Exception ex)
        {
            return ex.Message;
        }
    }

    private static string FormatDuration(TimeSpan duration)
    {
        if (duration < TimeSpan.Zero)
            duration = TimeSpan.Zero;

        return duration.TotalDays >= 1
            ? $"{(int)duration.TotalDays}d {duration.Hours:00}:{duration.Minutes:00}:{duration.Seconds:00}"
            : duration.TotalHours >= 1
                ? $"{(int)duration.TotalHours:00}:{duration.Minutes:00}:{duration.Seconds:00}"
                : $"{duration.Minutes:00}:{duration.Seconds:00}";
    }
}
