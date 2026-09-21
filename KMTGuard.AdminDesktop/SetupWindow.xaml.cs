using System.Collections.ObjectModel;
using System.Data;
using System.IO;
using System.Net;
using System.Reflection;
using System.Text;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using KMTGuard.AdminDesktop.Models;
using KMTGuard.AdminDesktop.Services;

namespace KMTGuard.AdminDesktop;

public partial class SetupWindow : Window
{
    private readonly string _settingsPath;
    private readonly SettingsFileService _settingsService = new();
    private readonly SetupStateService _setupStateService = new();
    private readonly FilterRuntimeService _runtimeService = new();
    private AdminSettings _settings;
    private int _currentStep;
    private bool _databaseVerified;
    private bool _preflightVerified;
    private bool _readinessVerified;
    private bool _updatesNeedReview;
    private string _lastOperation = "No setup checks have been run.";

    public ObservableCollection<SetupStepItem> Steps { get; } = new();
    public ObservableCollection<SetupCheckItem> PreflightChecks { get; } = new();
    public ObservableCollection<SetupCheckItem> ReadinessChecks { get; } = new();
    public string? RequestedPage { get; private set; }

    public SetupWindow(string settingsPath)
    {
        _settingsPath = settingsPath;
        _settings = LoadInitialSettings();
        InitializeComponent();
        DataContext = this;
        InitializeSteps();
        ApplySettings();
        ShowStep(0);
    }

    private AdminSettings LoadInitialSettings()
    {
        if (File.Exists(_settingsPath))
        {
            try
            {
                return _settingsService.Load(_settingsPath);
            }
            catch
            {
                // The guided form provides a safe recovery path for an invalid file.
            }
        }

        return new AdminSettings
        {
            Username = "sa",
            ProxyDb = "KMTGuard",
            ServerIP = string.Empty,
            MaximumPool = 1500,
            MinimumPool = 50
        };
    }

    private void InitializeSteps()
    {
        Steps.Add(new SetupStepItem(1, "Welcome", "Unified KMTGuard setup"));
        Steps.Add(new SetupStepItem(2, "Database", "SQL access and configuration"));
        Steps.Add(new SetupStepItem(3, "Preflight", "Files and server identity"));
        Steps.Add(new SetupStepItem(4, "Readiness", "Required database components"));
        Steps.Add(new SetupStepItem(5, "Launch", "Save settings and start services"));
        Steps[0].MarkComplete();
    }

    private void ApplySettings()
    {
        SqlHostBox.Text = _settings.Address;
        SqlPortBox.Text = _settings.Port?.ToString() ?? string.Empty;
        SqlUsernameBox.Text = _settings.Username;
        SqlPasswordBox.Password = _settings.Password;
        ProxyDatabaseBox.Text = _settings.ProxyDb;
        ServerIpBox.Text = _settings.ServerIP;
    }

    private async void PrimaryButton_Click(object sender, RoutedEventArgs e)
    {
        SetBusy(true);
        try
        {
            switch (_currentStep)
            {
                case 0:
                    ShowStep(1);
                    break;
                case 1:
                    if (await VerifyDatabaseAsync())
                        ShowStep(2);
                    break;
                case 2:
                    if (RunPreflightChecks())
                        ShowStep(3);
                    break;
                case 3:
                    if (await VerifyReadinessAsync())
                        ShowStep(4);
                    break;
                case 4:
                    await SaveAndStartAsync();
                    break;
            }
        }
        finally
        {
            SetBusy(false);
        }
    }

    private async Task<bool> VerifyDatabaseAsync()
    {
        try
        {
            _settings = ReadSettings();
            DatabaseResultText.Text = "Connecting to SQL Server and verifying the selected database...";
            DatabaseResultIcon.Text = "\uE72C";
            DatabaseResultIcon.Foreground = ThemeBrush("AccentBrush", Brushes.SteelBlue);

            var health = await new SqlAdminService(_settings.BuildConnectionString()).TestAsync();
            if (!health.IsConnected)
                throw new InvalidOperationException(ToFriendlyConnectionError(health.Message));

            _settingsService.Save(_settingsPath, _settings);
            Environment.SetEnvironmentVariable("KMTGUARD_SETTINGS_PATH", _settingsPath);
            _databaseVerified = true;
            Steps[1].MarkComplete();
            DatabaseResultPanel.Background = ThemeBrush("SuccessSoftBrush", Brushes.Honeydew);
            DatabaseResultPanel.BorderBrush = ThemeBrush("SuccessBrush", Brushes.ForestGreen);
            DatabaseResultIcon.Text = "\uE73E";
            DatabaseResultIcon.Foreground = ThemeBrush("SuccessBrush", Brushes.ForestGreen);
            DatabaseResultText.Foreground = ThemeBrush("SuccessBrush", Brushes.ForestGreen);
            DatabaseResultText.Text =
                $"Connection verified. SQL Server {health.ServerVersion}; {health.TableCount:N0} tables are available in { _settings.ProxyDb}.";
            _lastOperation = $"Database connection verified for {_settings.Address} / {_settings.ProxyDb}.";
            return true;
        }
        catch (Exception ex)
        {
            _databaseVerified = false;
            Steps[1].MarkError();
            DatabaseResultPanel.Background = ThemeBrush("RoseSoftBrush", Brushes.MistyRose);
            DatabaseResultPanel.BorderBrush = ThemeBrush("RoseBrush", Brushes.IndianRed);
            DatabaseResultIcon.Text = "\uEA39";
            DatabaseResultIcon.Foreground = ThemeBrush("RoseBrush", Brushes.IndianRed);
            DatabaseResultText.Foreground = ThemeBrush("RoseBrush", Brushes.IndianRed);
            DatabaseResultText.Text = ex.Message;
            _lastOperation = $"Database verification failed: {ex.Message}";
            return false;
        }
    }

    private bool RunPreflightChecks()
    {
        PreflightChecks.Clear();
        var status = _runtimeService.GetStatus();
        AddPreflightCheck(
            "Settings file",
            File.Exists(_settingsPath),
            File.Exists(_settingsPath) ? $"Configuration is saved at {_settingsPath}." : "Settings.json has not been saved.");
        AddPreflightCheck(
            "Server identity",
            IPAddress.TryParse(_settings.ServerIP, out _),
            IPAddress.TryParse(_settings.ServerIP, out _) ? $"{_settings.ServerIP} is a valid server address." : "Enter this machine's server IP address.");

        foreach (var service in status.Services)
        {
            var exists = File.Exists(service.Executable);
            AddPreflightCheck(
                $"{service.Role} service",
                exists,
                exists ? $"{Path.GetFileName(service.Executable)} is available." : $"{Path.GetFileName(service.Executable)} was not found in the Filter package.");
        }

        _preflightVerified = PreflightChecks.All(item => item.Passed);
        if (_preflightVerified)
        {
            Steps[2].MarkComplete();
            _lastOperation = "Preflight completed successfully. All runtime files and the server identity are ready.";
        }
        else
        {
            Steps[2].MarkError();
            _lastOperation = "Preflight found one or more items that require attention.";
        }

        return _preflightVerified;
    }

    private void AddPreflightCheck(string title, bool passed, string detail)
    {
        PreflightChecks.Add(new SetupCheckItem(title, detail, passed ? "READY" : "ACTION REQUIRED", passed));
    }

    private async Task<bool> VerifyReadinessAsync()
    {
        ReadinessChecks.Clear();
        try
        {
            var sql = new SqlAdminService(_settings.BuildConnectionString());
            var diagnostics = await sql.LoadDiagnosticsAsync();
            var requiredMissing = diagnostics.AsEnumerable()
                .Where(row => string.Equals(row.Field<string>("Status"), "Missing", StringComparison.OrdinalIgnoreCase))
                .ToList();
            var ready = diagnostics.AsEnumerable()
                .Count(row => string.Equals(row.Field<string>("Status"), "Ready", StringComparison.OrdinalIgnoreCase));

            foreach (var row in requiredMissing)
            {
                ReadinessChecks.Add(new SetupCheckItem(
                    row.Field<string>("Module") ?? "Required component",
                    $"{row.Field<string>("ObjectName")} · {row.Field<string>("Notes")}",
                    "MISSING",
                    false));
            }

            var migrations = await sql.CheckMigrationsAsync(GetMigrationsPath());
            var updatesToReview = migrations.Count(item =>
                item.Status is "Not found" or "Partial" or "Available");
            _updatesNeedReview = updatesToReview > 0 || requiredMissing.Count > 0;

            if (requiredMissing.Count == 0)
            {
                ReadinessChecks.Add(new SetupCheckItem(
                    "Required components",
                    $"{ready:N0} database components were detected; no required component is missing.",
                    "READY",
                    true));
            }

            ReadinessChecks.Add(new SetupCheckItem(
                "Packaged database updates",
                updatesToReview == 0
                    ? "Every detectable packaged update looks applied."
                    : $"{updatesToReview:N0} packaged update(s) remain available for review. Optional feature updates do not block startup.",
                updatesToReview == 0 ? "CURRENT" : "REVIEW",
                true,
                updatesToReview > 0));

            _readinessVerified = requiredMissing.Count == 0;
            ReviewUpdatesButton.Visibility = _updatesNeedReview
                ? Visibility.Visible
                : Visibility.Collapsed;

            if (_readinessVerified)
            {
                Steps[3].MarkComplete();
                ReadinessSummaryPanel.Background = ThemeBrush("SuccessSoftBrush", Brushes.Honeydew);
                ReadinessSummaryPanel.BorderBrush = ThemeBrush("SuccessBrush", Brushes.ForestGreen);
                ReadinessSummaryIcon.Text = "\uE73E";
                ReadinessSummaryIcon.Foreground = ThemeBrush("SuccessBrush", Brushes.ForestGreen);
                ReadinessSummaryTitle.Text = "Core database readiness verified";
                ReadinessSummaryText.Text = "KMTGuard can start safely. Optional updates can be reviewed at any time.";
                _lastOperation = "Database readiness verified. No required component is missing.";
                return true;
            }

            Steps[3].MarkError();
            ReadinessSummaryPanel.Background = ThemeBrush("RoseSoftBrush", Brushes.MistyRose);
            ReadinessSummaryPanel.BorderBrush = ThemeBrush("RoseBrush", Brushes.IndianRed);
            ReadinessSummaryIcon.Text = "\uEA39";
            ReadinessSummaryIcon.Foreground = ThemeBrush("RoseBrush", Brushes.IndianRed);
            ReadinessSummaryTitle.Text = $"{requiredMissing.Count:N0} required component(s) need attention";
            ReadinessSummaryText.Text = "Review the packaged database updates, apply the relevant updates, then return and run this check again.";
            _lastOperation = $"Database readiness failed with {requiredMissing.Count:N0} required component(s) missing.";
            return false;
        }
        catch (Exception ex)
        {
            Steps[3].MarkError();
            ReadinessSummaryTitle.Text = "Database readiness could not be checked";
            ReadinessSummaryText.Text = ex.Message;
            _lastOperation = $"Readiness check failed: {ex.Message}";
            return false;
        }
    }

    private async Task SaveAndStartAsync()
    {
        try
        {
            _settingsService.Save(_settingsPath, _settings);
            Environment.SetEnvironmentVariable("KMTGUARD_SETTINGS_PATH", _settingsPath);
            LaunchStatusText.Text = "Starting Agent, Download, and Gateway services...";
            PrimaryButtonText.Text = "Starting services";

            var status = await _runtimeService.StartAsync();
            if (!status.IsRunning)
                throw new InvalidOperationException(status.Message);

            _setupStateService.MarkComplete();
            Steps[4].MarkComplete();
            OverallStatusText.Text = "READY";
            LaunchHeroPanel.Background = ThemeBrush("SuccessSoftBrush", Brushes.Honeydew);
            LaunchHeroPanel.BorderBrush = ThemeBrush("SuccessBrush", Brushes.ForestGreen);
            LaunchIconPanel.Background = ThemeBrush("SuccessSoftBrush", Brushes.Honeydew);
            LaunchIconText.Text = "\uE73E";
            LaunchIconText.Foreground = ThemeBrush("SuccessBrush", Brushes.ForestGreen);
            LaunchTitleText.Text = "KMTGuard is running successfully";
            LaunchDetailText.Text = "All three services are online. The complete operations workspace is now available.";
            LaunchStatusText.Text = string.Join(" · ", status.Services.Select(service => $"{service.Role}: {service.Status}"));
            LaunchStatusText.Foreground = ThemeBrush("SuccessBrush", Brushes.ForestGreen);
            PrimaryButtonText.Text = "Open control center";
            PrimaryButtonIcon.Text = "\uE8A7";
            _lastOperation = "Guided setup completed successfully. All three KMTGuard services are running.";
            PrimaryButton.Click -= PrimaryButton_Click;
            PrimaryButton.Click += FinishButton_Click;
        }
        catch (Exception ex)
        {
            Steps[4].MarkError();
            LaunchHeroPanel.Background = ThemeBrush("RoseSoftBrush", Brushes.MistyRose);
            LaunchHeroPanel.BorderBrush = ThemeBrush("RoseBrush", Brushes.IndianRed);
            LaunchIconText.Text = "\uEA39";
            LaunchIconText.Foreground = ThemeBrush("RoseBrush", Brushes.IndianRed);
            LaunchTitleText.Text = "KMTGuard could not start";
            LaunchDetailText.Text = "The verified settings were saved, but one or more services did not reach a healthy running state.";
            LaunchStatusText.Text = ex.Message;
            LaunchStatusText.Foreground = ThemeBrush("RoseBrush", Brushes.IndianRed);
            PrimaryButtonText.Text = "Try again";
            _lastOperation = $"Service startup failed: {ex.Message}";
        }
    }

    private void FinishButton_Click(object sender, RoutedEventArgs e)
    {
        DialogResult = true;
    }

    private AdminSettings ReadSettings()
    {
        var host = SqlHostBox.Text.Trim();
        var database = ProxyDatabaseBox.Text.Trim();
        var serverIp = ServerIpBox.Text.Trim();
        if (host.Length == 0)
            throw new InvalidOperationException("Enter the SQL Server host name or IP address.");
        if (database.Length == 0)
            throw new InvalidOperationException("Enter the KMTGuard database name.");
        if (!IPAddress.TryParse(serverIp, out _))
            throw new InvalidOperationException("Enter this machine's valid server IP address.");

        int? port = null;
        if (SqlPortBox.Text.Trim().Length > 0)
        {
            if (!int.TryParse(SqlPortBox.Text.Trim(), out var parsedPort) || parsedPort is < 1 or > 65535)
                throw new InvalidOperationException("SQL port must be a number between 1 and 65,535.");
            port = parsedPort;
        }

        return new AdminSettings
        {
            Address = host,
            Port = port,
            Username = SqlUsernameBox.Text.Trim(),
            Password = SqlPasswordBox.Password,
            CredentialTarget = _settings.CredentialTarget,
            QuickLoginCredentialTarget = _settings.QuickLoginCredentialTarget,
            QuickLoginMasterKey = _settings.QuickLoginMasterKey,
            ProxyDb = database,
            ServerIP = serverIp,
            MaximumPool = _settings.MaximumPool <= 0 ? 1500 : _settings.MaximumPool,
            MinimumPool = Math.Max(0, _settings.MinimumPool),
            ConnectionLifetime = Math.Max(0, _settings.ConnectionLifetime),
            Language = _settings.Language,
            UpstreamConnectTimeoutSeconds = _settings.UpstreamConnectTimeoutSeconds,
            HandshakeTimeoutSeconds = _settings.HandshakeTimeoutSeconds,
            LoginTimeoutSeconds = _settings.LoginTimeoutSeconds,
            UnauthenticatedIdleTimeoutSeconds = _settings.UnauthenticatedIdleTimeoutSeconds,
            AuthenticatedHeartbeatTimeoutSeconds = _settings.AuthenticatedHeartbeatTimeoutSeconds,
            GatewaySessionCap = _settings.GatewaySessionCap,
            AgentSessionCap = _settings.AgentSessionCap,
            DownloadSessionCap = _settings.DownloadSessionCap
        };
    }

    private void ShowStep(int step)
    {
        _currentStep = Math.Clamp(step, 0, 4);
        WelcomePage.Visibility = _currentStep == 0 ? Visibility.Visible : Visibility.Collapsed;
        DatabasePage.Visibility = _currentStep == 1 ? Visibility.Visible : Visibility.Collapsed;
        PreflightPage.Visibility = _currentStep == 2 ? Visibility.Visible : Visibility.Collapsed;
        ReadinessPage.Visibility = _currentStep == 3 ? Visibility.Visible : Visibility.Collapsed;
        LaunchPage.Visibility = _currentStep == 4 ? Visibility.Visible : Visibility.Collapsed;

        for (var index = 0; index < Steps.Count; index++)
            Steps[index].IsCurrent = index == _currentStep;

        PageEyebrowText.Text = $"STEP {_currentStep + 1} OF 5";
        (PageTitleText.Text, PageSubtitleText.Text, PrimaryButtonText.Text, PrimaryButtonIcon.Text) = _currentStep switch
        {
            0 => ("Welcome to KMTGuard", "We will verify the essentials before any service is started.", "Begin setup", "\uE76C"),
            1 => ("Connect the database", "Verify the SQL credentials and save a clean Settings.json file.", _databaseVerified ? "Continue" : "Test and continue", "\uE968"),
            2 => ("Run the server preflight", "Confirm the runtime files and this machine's server identity.", _preflightVerified ? "Continue" : "Run preflight", "\uE9D9"),
            3 => ("Verify database readiness", "Check required components and identify packaged updates that need review.", _readinessVerified ? "Continue" : "Check readiness", "\uE895"),
            _ => ("Launch KMTGuard", "Save the verified configuration and start all services in the correct order.", "Save and start", "\uE768")
        };

        BackButton.Visibility = _currentStep > 0 && Steps[4].Status != SetupStepStatus.Complete
            ? Visibility.Visible
            : Visibility.Collapsed;
        ReviewUpdatesButton.Visibility = _currentStep == 3 && _updatesNeedReview
            ? Visibility.Visible
            : Visibility.Collapsed;
    }

    private void BackButton_Click(object sender, RoutedEventArgs e)
    {
        if (_currentStep > 0)
            ShowStep(_currentStep - 1);
    }

    private void ReviewUpdatesButton_Click(object sender, RoutedEventArgs e)
    {
        RequestedPage = "Migrations";
        DialogResult = false;
    }

    private void CopySupportReportButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            Clipboard.SetText(BuildSupportReport());
            _lastOperation = "A password-free support report was copied to the clipboard.";
        }
        catch (Exception ex)
        {
            MessageBox.Show(this, ex.Message, "Copy Support Report", MessageBoxButton.OK, MessageBoxImage.Warning);
        }
    }

    private string BuildSupportReport()
    {
        var runtime = _runtimeService.GetStatus();
        var builder = new StringBuilder()
            .AppendLine("KMTGuard Support Report")
            .AppendLine($"Generated: {DateTimeOffset.Now:yyyy-MM-dd HH:mm:ss zzz}")
            .AppendLine($"Admin Desktop: {Assembly.GetExecutingAssembly().GetName().Version}")
            .AppendLine($"Setup step: {_currentStep + 1} of 5")
            .AppendLine($"Last result: {_lastOperation}")
            .AppendLine()
            .AppendLine("Configuration")
            .AppendLine($"SQL host: {SqlHostBox.Text.Trim()}")
            .AppendLine($"SQL port: {(SqlPortBox.Text.Trim().Length == 0 ? "default" : SqlPortBox.Text.Trim())}")
            .AppendLine($"Database: {ProxyDatabaseBox.Text.Trim()}")
            .AppendLine($"Server IP: {ServerIpBox.Text.Trim()}")
            .AppendLine($"Settings file: {_settingsPath}")
            .AppendLine()
            .AppendLine("Checks");

        foreach (var step in Steps)
            builder.AppendLine($"{step.Number}. {step.Title}: {step.Status}");

        builder.AppendLine()
            .AppendLine("Runtime");
        foreach (var service in runtime.Services)
            builder.AppendLine($"{service.Role}: {service.Status}; executable={(File.Exists(service.Executable) ? "present" : "missing")}");

        return builder.ToString();
    }

    private void SetBusy(bool busy)
    {
        PrimaryButton.IsEnabled = !busy;
        BackButton.IsEnabled = !busy;
        ReviewUpdatesButton.IsEnabled = !busy;
        if (busy)
        {
            PrimaryButtonText.Text = "Working...";
            return;
        }

        if (Steps[4].Status == SetupStepStatus.Complete)
            return;

        PrimaryButtonText.Text = _currentStep switch
        {
            0 => "Begin setup",
            1 => _databaseVerified ? "Continue" : "Test and continue",
            2 => _preflightVerified ? "Continue" : "Run preflight",
            3 => _readinessVerified ? "Continue" : "Check readiness",
            _ => "Save and start"
        };
    }

    private static string ToFriendlyConnectionError(string error)
    {
        if (error.Contains("login failed", StringComparison.OrdinalIgnoreCase))
            return "SQL Server rejected the username or password. Verify the credentials and try again.";
        if (error.Contains("network-related", StringComparison.OrdinalIgnoreCase) ||
            error.Contains("server was not found", StringComparison.OrdinalIgnoreCase))
            return "KMTGuard could not reach SQL Server. Verify the host, port, SQL service, and firewall access.";
        if (error.Contains("cannot open database", StringComparison.OrdinalIgnoreCase))
            return "SQL Server was reached, but the selected KMTGuard database is unavailable to this account.";
        return $"Database verification failed: {error}";
    }

    private static string GetMigrationsPath()
    {
        var candidates = new[]
        {
            Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "database")),
            Path.GetFullPath(Path.Combine(Environment.CurrentDirectory, "database")),
            Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\database\migrations")),
            Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, @"..\..\..\..\..\database\migrations"))
        };
        return candidates.FirstOrDefault(Directory.Exists) ?? candidates[0];
    }

    private static Brush ThemeBrush(string key, Brush fallback) =>
        Application.Current.TryFindResource(key) as Brush ?? fallback;

    private void TitleBar_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
    {
        if (e.ButtonState == MouseButtonState.Pressed)
            DragMove();
    }

    private void MinimizeButton_Click(object sender, RoutedEventArgs e) => WindowState = WindowState.Minimized;
    private void CloseButton_Click(object sender, RoutedEventArgs e) => Close();
}

public enum SetupStepStatus
{
    Pending,
    Complete,
    Error
}

public sealed class SetupStepItem : System.ComponentModel.INotifyPropertyChanged
{
    private bool _isCurrent;
    private SetupStepStatus _status;

    public SetupStepItem(int number, string title, string subtitle)
    {
        Number = number;
        Title = title;
        Subtitle = subtitle;
    }

    public int Number { get; }
    public string Title { get; }
    public string Subtitle { get; }
    public SetupStepStatus Status
    {
        get => _status;
        private set
        {
            _status = value;
            NotifyAll();
        }
    }

    public bool IsCurrent
    {
        get => _isCurrent;
        set
        {
            _isCurrent = value;
            NotifyAll();
        }
    }

    public string StatusGlyph => Status switch { SetupStepStatus.Complete => "\uE73E", SetupStepStatus.Error => "\uEA39", _ => string.Empty };
    public Brush StatusBrush => Status switch
    {
        SetupStepStatus.Complete => SetupWindowBrush("SuccessBrush", Brushes.ForestGreen),
        SetupStepStatus.Error => SetupWindowBrush("RoseBrush", Brushes.IndianRed),
        _ => SetupWindowBrush("SubtleTextBrush", Brushes.Gray)
    };
    public Brush NumberBackground => IsCurrent
        ? SetupWindowBrush("AccentBrush", Brushes.IndianRed)
        : Status == SetupStepStatus.Complete
            ? SetupWindowBrush("SuccessSoftBrush", Brushes.Honeydew)
            : Brushes.White;
    public Brush NumberBorder => IsCurrent
        ? SetupWindowBrush("AccentBrush", Brushes.IndianRed)
        : SetupWindowBrush("LineBrush", Brushes.LightGray);
    public Brush NumberForeground => IsCurrent
        ? Brushes.White
        : Status == SetupStepStatus.Complete
            ? SetupWindowBrush("SuccessBrush", Brushes.ForestGreen)
            : SetupWindowBrush("MutedBrush", Brushes.Gray);

    public void MarkComplete() => Status = SetupStepStatus.Complete;
    public void MarkError() => Status = SetupStepStatus.Error;

    public event System.ComponentModel.PropertyChangedEventHandler? PropertyChanged;

    private void NotifyAll()
    {
        PropertyChanged?.Invoke(this, new System.ComponentModel.PropertyChangedEventArgs(string.Empty));
    }

    private static Brush SetupWindowBrush(string key, Brush fallback) =>
        Application.Current.TryFindResource(key) as Brush ?? fallback;
}

public sealed class SetupCheckItem
{
    public SetupCheckItem(string title, string detail, string status, bool passed, bool warning = false)
    {
        Title = title;
        Detail = detail;
        Status = status;
        Passed = passed;
        Warning = warning;
    }

    public string Title { get; }
    public string Detail { get; }
    public string Status { get; }
    public bool Passed { get; }
    public bool Warning { get; }
    public string Icon => Passed ? Warning ? "\uE7BA" : "\uE73E" : "\uEA39";
    public Brush IconBrush => Passed
        ? Warning
            ? Application.Current.TryFindResource("AmberBrush") as Brush ?? Brushes.DarkGoldenrod
            : Application.Current.TryFindResource("SuccessBrush") as Brush ?? Brushes.ForestGreen
        : Application.Current.TryFindResource("RoseBrush") as Brush ?? Brushes.IndianRed;
    public Brush IconBackground => Passed
        ? Warning
            ? Application.Current.TryFindResource("AmberSoftBrush") as Brush ?? Brushes.LemonChiffon
            : Application.Current.TryFindResource("SuccessSoftBrush") as Brush ?? Brushes.Honeydew
        : Application.Current.TryFindResource("RoseSoftBrush") as Brush ?? Brushes.MistyRose;
}
