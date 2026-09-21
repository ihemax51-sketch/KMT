using KMTGuard.CommandManager;
using KMTGuard.ConsoleUi;
using KMTGuard.Helpers;
using KMTGuard.Localization;
using KMTGuard.Runtime;
using KMTGuard.RuntimeContract;
using KMTGuard.ServerManagers;
using KMTGuard.SettingManager;

using Serilog;
using Serilog.Core;
using Serilog.Events;

using System;
using System.Collections.Generic;
using System.Net;
using System.Security.Cryptography;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Data.SqlClient;

public static class Program
{
    public static string Connectionstring { get; set; } = string.Empty;
    public static string MainMachineIP { get; set; } = string.Empty;
    public static string ProxyDb { get; set; } = string.Empty;
    public static byte[] QuickLoginMasterKey { get; private set; } = Array.Empty<byte>();
    public static ISettings RuntimeSettings { get; private set; } = new Settings().Init();
    public static LoggingLevelSwitch LoggingLevelSwitch { get; } = new LoggingLevelSwitch();

    private static readonly SemaphoreSlim RuntimeLock = new(1, 1);
    private static readonly List<Mutex> InstanceMutexes = new();
    private static RuntimeControlServer? _runtimeControlServer;
    private static TaskCompletionSource? _workerStopSignal;
    private static bool _isRunning;
    private static DateTime? _startedAtUtc;
    private static FilterRole _currentRole = FilterRole.All;

    public static bool IsEmbeddedRunning => _isRunning;
    public static DateTime? EmbeddedStartedAtUtc => _startedAtUtc;
    public static FilterRole CurrentRole => _currentRole;
    public static string EmbeddedStatus => _isRunning ? $"{_currentRole} service is running" : "Stopped";

    private static void Main()
    {
        StartCoreAsync(FilterRole.All).GetAwaiter().GetResult();

        var commandHandler = new CommandHandler();
        try
        {
            while (true)
            {
                FilterConsole.Prompt();
                var command = Console.ReadLine();
                if (string.IsNullOrEmpty(command))
                    continue;

                if (command.Equals("/exit", StringComparison.OrdinalIgnoreCase))
                {
                    FilterConsole.WriteCommandSuccess("Stopping filter...");
                    break;
                }

                commandHandler.ExecuteCommand(command).GetAwaiter().GetResult();
            }
        }
        catch (Exception exception)
        {
            Log.Warning("Program.cs Main| {0}", exception.Message);
            Log.Warning("Program.cs Main| {0}", exception.StackTrace);
        }
        finally
        {
            StopEmbeddedAsync().GetAwaiter().GetResult();
        }
    }

    public static async Task StartEmbeddedAsync()
    {
        await StartRoleAsync(FilterRole.All);
    }

    public static async Task StartRoleAsync(FilterRole role)
    {
        await RuntimeLock.WaitAsync();
        try
        {
            if (_isRunning)
                return;

            try
            {
                await StartCoreAsync(role);
            }
            catch
            {
                CleanupRuntime();
                throw;
            }
        }
        finally
        {
            RuntimeLock.Release();
        }
    }

    public static async Task StopEmbeddedAsync()
    {
        await RuntimeLock.WaitAsync();
        try
        {
            if (!_isRunning && InstanceMutexes.Count == 0)
                return;

            CleanupRuntime();
        }
        finally
        {
            RuntimeLock.Release();
        }
    }

    public static async Task RestartEmbeddedAsync()
    {
        await StopEmbeddedAsync();
        await StartEmbeddedAsync();
    }

    public static async Task RunWorkerAsync(FilterRole role)
    {
        if (role == FilterRole.All)
            throw new ArgumentOutOfRangeException(nameof(role), "A worker must host one concrete server role.");

        _workerStopSignal = new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        ConsoleCancelEventHandler cancelHandler = (_, args) =>
        {
            args.Cancel = true;
            _workerStopSignal.TrySetResult();
        };
        Console.CancelKeyPress += cancelHandler;

        try
        {
            await StartRoleAsync(role);
            await _workerStopSignal.Task;
        }
        finally
        {
            Console.CancelKeyPress -= cancelHandler;
            await StopEmbeddedAsync();
            _workerStopSignal = null;
        }
    }

    private static Task RequestWorkerShutdownAsync()
    {
        if (_workerStopSignal != null)
        {
            _workerStopSignal.TrySetResult();
            return Task.CompletedTask;
        }

        return StopEmbeddedAsync();
    }

    private static void CleanupRuntime()
    {
        if (_runtimeControlServer != null)
        {
            try
            {
                _runtimeControlServer.DisposeAsync().AsTask().GetAwaiter().GetResult();
            }
            catch (Exception ex)
            {
                Log.Warning("Runtime control server dispose failed: {Message}", ex.Message);
            }

            _runtimeControlServer = null;
        }

        try
        {
            ServerManager.Dispose();
        }
        catch (Exception ex)
        {
            Log.Warning("ServerManager dispose failed: {Message}", ex.Message);
        }

        for (var index = InstanceMutexes.Count - 1; index >= 0; index--)
        {
            try
            {
                InstanceMutexes[index].ReleaseMutex();
            }
            catch
            {
            }

            InstanceMutexes[index].Dispose();
        }

        InstanceMutexes.Clear();
        if (QuickLoginMasterKey.Length > 0)
            CryptographicOperations.ZeroMemory(QuickLoginMasterKey);
        QuickLoginMasterKey = Array.Empty<byte>();
        _isRunning = false;
        _startedAtUtc = null;
        Log.CloseAndFlush();
    }

    private static async Task StartCoreAsync(FilterRole role)
    {
        _currentRole = role;
        FilterConsole.Configure(role);

        // ---- Settings ----
        var settings = new SettingsManager();
        ValidateSettings(settings.Settings);
        RuntimeSettings = settings.Settings;
        ProxyDb = settings.Settings.ProxyDb;

        var maximumPool = role switch
        {
            FilterRole.Download => Math.Min(Math.Max(settings.Settings.MaximumPool, 16), 32),
            FilterRole.Gateway => Math.Min(Math.Max(settings.Settings.MaximumPool, 32), 256),
            _ => settings.Settings.MaximumPool
        };
        var minimumPool = role is FilterRole.Agent or FilterRole.All
            ? settings.Settings.MinimumPool
            : 0;

        minimumPool = Math.Min(minimumPool, maximumPool);
        var sqlPassword = !string.IsNullOrEmpty(settings.Settings.Password)
            ? settings.Settings.Password
            : WindowsCredentialStore.ReadPassword(
                settings.Settings.CredentialTarget, settings.Settings.Username);
        var dataSource = settings.Settings.Port.HasValue
            ? $"{settings.Settings.Address},{settings.Settings.Port.Value}"
            : settings.Settings.Address;
        var connectionBuilder = new SqlConnectionStringBuilder
        {
            DataSource = dataSource,
            InitialCatalog = settings.Settings.ProxyDb,
            UserID = settings.Settings.Username,
            Password = sqlPassword,
            PersistSecurityInfo = false,
            MultipleActiveResultSets = true,
            ApplicationName = $"KMTGuard-{role}",
            Encrypt = false,
            Pooling = true,
            MaxPoolSize = maximumPool,
            MinPoolSize = minimumPool,
            LoadBalanceTimeout = settings.Settings.ConnectionLifetime,
            ConnectTimeout = 10
        };
        Connectionstring = connectionBuilder.ConnectionString;
        sqlPassword = string.Empty;

        if (role is FilterRole.Gateway or FilterRole.All)
            QuickLoginMasterKey = settings.GetOrCreateQuickLoginMasterKey();
        else
            QuickLoginMasterKey = Array.Empty<byte>();

        MainMachineIP = settings.Settings.ServerIP;
        DatabaseJobQueue.Start();

        // ---- Logging ----
        LoggingLevelSwitch.MinimumLevel = LogEventLevel.Debug;
        var loggerConfiguration = new LoggerConfiguration()
            .MinimumLevel.ControlledBy(LoggingLevelSwitch)
            .MinimumLevel.Override(
                "KMTGuard.Clientless.ClientlessHuntEngine",
                LogEventLevel.Error);

        if (FilterConsole.IsEnabled)
        {
            loggerConfiguration = loggerConfiguration.WriteTo.Console(
                formatter: new KmtServiceLogFormatter(role));
        }

        Log.Logger = loggerConfiguration
            .WriteTo.File(
                new KmtServiceLogFormatter(role),
                $"logs/kmtguard-{role.ToString().ToLowerInvariant()}-events-.log",
                rollingInterval: RollingInterval.Day,
                fileSizeLimitBytes: 32L * 1024L * 1024L,
                rollOnFileSizeLimit: true,
                retainedFileCountLimit: 14)
            .CreateLogger();

        AcquireInstanceMutexes(role);

        FilterConsole.Initialize();
        FilterConsole.WriteStartupStep("Configuration", $"DB={ProxyDb}, ServerIP={MainMachineIP}");
        var languageResult = PlayerLanguage.Initialize(settings.Settings.Language);
        FilterConsole.WriteStartupStep(
            "Player language",
            $"{languageResult.Language} ({languageResult.ActiveKeyCount} keys, {languageResult.MissingKeyCount} fallback)");

        // ---- Start services ----
        await GameServerPacketAuthenticator.InitializeAsync(Connectionstring);
        FilterConsole.WriteStartupStep("Packet authentication", "Internal channel authenticated.");
        FilterConsole.WriteStartupStep("Server startup", "Loading services and packet handlers...");
        await ServerManager.InitialServers(role);
        FilterConsole.SetConnectionTitle();
        FilterConsole.WriteReady(MainMachineIP, ProxyDb);
        _isRunning = true;
        _startedAtUtc = DateTime.UtcNow;
        _runtimeControlServer = new RuntimeControlServer(role, RequestWorkerShutdownAsync);
        _runtimeControlServer.Start();
        Log.Information("Runtime channel online :: role={Role} :: process={ProcessId} :: accepting traffic",
            role,
            Environment.ProcessId);
    }

    private static void ValidateSettings(ISettings settings)
    {
        if (string.IsNullOrWhiteSpace(settings.Address) ||
            string.Equals(settings.Address, "0.0.0.0", StringComparison.OrdinalIgnoreCase))
            throw new InvalidDataException("Settings.Address must identify the SQL Server host.");
        if (settings.Port is <= 0 or > 65535)
            throw new InvalidDataException("Settings.Port must be from 1 to 65535 when specified.");
        if (string.IsNullOrWhiteSpace(settings.ProxyDb) ||
            string.IsNullOrWhiteSpace(settings.Username))
            throw new InvalidDataException(
                "ProxyDb and Username are required.");
        if (string.IsNullOrEmpty(settings.Password) && string.IsNullOrWhiteSpace(settings.CredentialTarget))
            throw new InvalidDataException(
                "Set Password in Settings.json or configure CredentialTarget in Windows Credential Manager.");
        if (settings.MaximumPool is < 1 or > 2000 ||
            settings.MinimumPool < 0 || settings.MinimumPool > settings.MaximumPool)
            throw new InvalidDataException("SQL pool settings are outside the supported range.");
        if (settings.ConnectionLifetime is < 0 or > 86400)
            throw new InvalidDataException("ConnectionLifetime must be from 0 to 86400 seconds.");
        if (settings.UpstreamConnectTimeoutSeconds is < 1 or > 120 ||
            settings.HandshakeTimeoutSeconds is < 5 or > 120 ||
            settings.LoginTimeoutSeconds is < 15 or > 600 ||
            settings.UnauthenticatedIdleTimeoutSeconds is < 15 or > 900 ||
            settings.AuthenticatedHeartbeatTimeoutSeconds is < 30 or > 3600)
            throw new InvalidDataException("Transport timeout settings are outside the supported range.");
        if (settings.GatewaySessionCap is < 1 or > 100000 ||
            settings.AgentSessionCap is < 1 or > 100000 ||
            settings.DownloadSessionCap is < 1 or > 100000)
            throw new InvalidDataException("Per-service session caps must be from 1 to 100000.");
        if (!IPAddress.TryParse(settings.ServerIP, out _))
            throw new InvalidDataException("ServerIP must be a valid IP address.");
    }

    private static void AcquireInstanceMutexes(FilterRole role)
    {
        var roles = role == FilterRole.All
            ? new[] { FilterRole.Agent, FilterRole.Download, FilterRole.Gateway }
            : new[] { role };

        foreach (var ownedRole in roles)
        {
            var mutex = new Mutex(true, $@"Global\KMTGuard_Filter_{ownedRole}", out var isNewInstance);
            if (isNewInstance)
            {
                InstanceMutexes.Add(mutex);
                continue;
            }

            mutex.Dispose();
            for (var index = InstanceMutexes.Count - 1; index >= 0; index--)
            {
                try { InstanceMutexes[index].ReleaseMutex(); } catch { }
                InstanceMutexes[index].Dispose();
            }

            InstanceMutexes.Clear();
            Log.Error("Another KMTGuard {Role} service is already running.", ownedRole);
            throw new InvalidOperationException($"Another KMTGuard {ownedRole} service is already running.");
        }
    }

    public static void PrintInColor(string message, ConsoleColor color)
    {
        var originalColor = Console.ForegroundColor;
        Console.ForegroundColor = color;
        Log.Warning(message);
        Console.ForegroundColor = originalColor;
    }
}
