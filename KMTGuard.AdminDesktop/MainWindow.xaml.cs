using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Data;
using System.IO;
using System.Net;
using System.Reflection;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Animation;
using System.Windows.Threading;
using KMTGuard.AdminDesktop.Models;
using KMTGuard.AdminDesktop.Services;
using KMTGuard.RuntimeContract;
using Microsoft.Win32;

namespace KMTGuard.AdminDesktop;

public partial class MainWindow : Window
{
    private readonly SettingsFileService _settingsFileService = new();
    private readonly SetupStateService _setupStateService = new();
    private readonly LogService _logService = new();
    private readonly FilterRuntimeService _filterRuntimeService = new();
    private IReadOnlyList<SettingEntry> _allSettings = Array.Empty<SettingEntry>();
    private IReadOnlyList<ProxyServiceEntry> _proxyServices = Array.Empty<ProxyServiceEntry>();
    private readonly ObservableCollection<VipTierEntry> _vipTiers = new();
    private bool _vipSystemEnabled = true;
    private readonly ObservableCollection<DiscordNotificationChannel> _discordChannels = new();
    private readonly ObservableCollection<DiscordDeliveryActivity> _discordActivity = new();
    private readonly ObservableCollection<ClientlessCityHunterPlan> _clientlessCityHunterPlans = new();
    private AdminSettings? _loadedSettings;
    private bool _uiReady;
    private string _selectedSettingsSection = string.Empty;
    private SettingStore _settingsStoreView = SettingStore.Filter;
    private string _securityGridMode = "BlockedWords";
    private string _pvpGridMode = "Arenas";
    private string _selectedManagedTableKey = string.Empty;
    private IReadOnlyList<ManagedColumnInfo> _managedColumns = Array.Empty<ManagedColumnInfo>();
    private readonly Dictionary<string, FrameworkElement> _settingInputs = new(StringComparer.OrdinalIgnoreCase);
    private readonly Dictionary<string, FrameworkElement> _managedInputs = new(StringComparer.OrdinalIgnoreCase);
    private readonly DispatcherTimer _onlinePlayersTimer;
    private bool _onlinePlayersRefreshBusy;
    private int _survivalWinnerRewardId;
    private int _survivalLoserRewardId;
    private int _survivalSoloWinnerRewardId;
    private int _survivalSoloLoserRewardId;
    private string _competitiveEventCode = "LMS";
    private static readonly IReadOnlyDictionary<string, SettingDefinition> SettingDefinitions = BuildSettingDefinitions();
    private static readonly IReadOnlyDictionary<string, SettingDefinition> GameServerSettingDefinitions = BuildGameServerSettingDefinitions();
    private static readonly IReadOnlyDictionary<string, SectionDefinition> SectionDefinitions = BuildSectionDefinitions();
    private readonly Dictionary<int, ProxyServiceInputs> _proxyServiceInputs = new();
    private PlayerCommandTarget? _playerCommandTarget;
    private bool _closeCommitted;
    private bool _windowPresented;
    private bool _telegramHasStoredBotToken;
    private bool _telegramUniqueSpawnEnabled = true;
    private bool _telegramUniqueKillEnabled = true;
    private bool _telegramShowKillerName = true;
    private bool _discordHasStoredBotToken;
    private string _selectedNavigationGroup = "Workspace";
    private bool _suppressNavigationGroupSelection;
    private string _schedulerDefaultDatabaseName = string.Empty;
    private bool _schedulerSelectionLoading;
    private bool _autoEventSelectionLoading;
    private bool _setupComplete;
    private bool _setupWindowOpen;
    private DataTable? _clientlessHuntAreas;
    private int _clientlessHuntLoadGeneration;

    public MainWindow()
    {
        InitializeComponent();
        RefreshClientlessWeaponChoices();
        InitializeSchedulerEditor();
        VipTiersGrid.ItemsSource = _vipTiers;
        ClientlessCityPlansEditor.ItemsSource = _clientlessCityHunterPlans;
        DiscordChannelCardsControl.ItemsSource = _discordChannels;
        DiscordManualChannelBox.ItemsSource = _discordChannels;
        DiscordActivityList.ItemsSource = _discordActivity;
        _onlinePlayersTimer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(2) };
        _onlinePlayersTimer.Tick += async (_, _) => await RefreshOnlinePlayersAsync();
        _uiReady = true;
        ApplyNavigationFilter();
        ShowPage("Dashboard");
        Loaded += MainWindow_Loaded;
        SourceInitialized += (_, _) => FitWindowToWorkArea();
    }

    private void MainWindow_ContentRendered(object? sender, EventArgs e)
    {
        if (_windowPresented)
            return;

        _windowPresented = true;
        var easing = new CubicEase { EasingMode = EasingMode.EaseOut };
        BeginAnimation(OpacityProperty, new DoubleAnimation(0, 1, TimeSpan.FromMilliseconds(190))
        {
            EasingFunction = easing
        });
        WindowScaleTransform.BeginAnimation(ScaleTransform.ScaleXProperty,
            new DoubleAnimation(0.985, 1, TimeSpan.FromMilliseconds(190)) { EasingFunction = easing });
        WindowScaleTransform.BeginAnimation(ScaleTransform.ScaleYProperty,
            new DoubleAnimation(0.985, 1, TimeSpan.FromMilliseconds(190)) { EasingFunction = easing });
    }

    private void MainWindow_Closing(object? sender, CancelEventArgs e)
    {
        if (_closeCommitted)
            return;

        e.Cancel = true;
        IsEnabled = false;
        var duration = TimeSpan.FromMilliseconds(125);
        var fade = new DoubleAnimation(Opacity, 0, duration)
        {
            EasingFunction = new QuadraticEase { EasingMode = EasingMode.EaseIn }
        };
        fade.Completed += (_, _) =>
        {
            _closeCommitted = true;
            Dispatcher.BeginInvoke(Close);
        };
        BeginAnimation(OpacityProperty, fade);
        WindowScaleTransform.BeginAnimation(ScaleTransform.ScaleXProperty, new DoubleAnimation(1, 0.99, duration));
        WindowScaleTransform.BeginAnimation(ScaleTransform.ScaleYProperty, new DoubleAnimation(1, 0.99, duration));
    }

    private void FitWindowToWorkArea()
    {
        var workArea = SystemParameters.WorkArea;
        const double outerMargin = 24;
        MinWidth = Math.Min(MinWidth, Math.Max(960, workArea.Width - outerMargin));
        MinHeight = Math.Min(MinHeight, Math.Max(600, workArea.Height - outerMargin));
        MaxWidth = workArea.Width;
        MaxHeight = workArea.Height;
        Width = Math.Min(Width, Math.Max(MinWidth, workArea.Width - outerMargin));
        Height = Math.Min(Height, Math.Max(MinHeight, workArea.Height - outerMargin));
        Left = workArea.Left + ((workArea.Width - Width) / 2);
        Top = workArea.Top + ((workArea.Height - Height) / 2);
    }

    private async void MainWindow_Loaded(object sender, RoutedEventArgs e)
    {
        AdminNameText.Text = AppSession.CurrentAdmin;
        SettingsPathBox.Text = _settingsFileService.FindDefaultSettingsPath() ?? _settingsFileService.GetRuntimeSettingsPath();

        if (File.Exists(SettingsPathBox.Text))
        {
            LoadSettingsFromFile(SettingsPathBox.Text);
        }
        else
        {
            ApplySettingsToFields(CreateDefaultConnectionSettings());
            SetStatus("Complete Guided Setup to verify SQL access and start KMTGuard safely.", true);
        }

        _setupComplete = _setupStateService.IsComplete(SettingsPathBox.Text);
        ApplySetupExperienceState();
        if (!_setupComplete)
            await OpenSetupExperienceAsync();
        else
            await RefreshDashboardAsync();

        RefreshFilterProcessStatus();
    }

    private static bool IsGuidedSetupPage(string page) =>
        page is "Dashboard" or "Setup" or "Connection" or "Diagnostics" or "Migrations" or "Logs";

    private void ApplySetupExperienceState()
    {
        var advancedVisibility = _setupComplete ? Visibility.Visible : Visibility.Collapsed;
        OperationsNavigationGroupItem.Visibility = advancedVisibility;
        EventsNavigationGroupItem.Visibility = advancedVisibility;
        ContentNavigationGroupItem.Visibility = advancedVisibility;
        BotOperationsNavigationGroupItem.Visibility = advancedVisibility;
        SystemGameNavigationGroupItem.Visibility = advancedVisibility;
        NotificationsNavigationGroupItem.Visibility = advancedVisibility;
        SystemNavigationGroupItem.Visibility = Visibility.Visible;

        DashboardSetupBanner.Visibility = _setupComplete ? Visibility.Collapsed : Visibility.Visible;
        DashboardPosturePanel.Visibility = advancedVisibility;
        DashboardServicesPanel.Visibility = advancedVisibility;
        DashboardReadinessPanel.Visibility = advancedVisibility;
        DashboardActivityPanel.Visibility = advancedVisibility;
        DashboardWorkspaceMapPanel.Visibility = advancedVisibility;

        if (!_setupComplete)
        {
            SidebarStatusDot.Fill = GetThemeBrush("AmberBrush", Brushes.DarkGoldenrod);
            SidebarStatusText.Text = "Guided setup is waiting for completion";
            OverallStatusForSetupMode();
        }

        ApplyNavigationFilter();
    }

    private void OverallStatusForSetupMode()
    {
        DashboardUpdatedText.Text = "Setup required";
        TopRuntimeStatusText.Text = "Setup required";
        TopRuntimeStatusDot.Fill = GetThemeBrush("AmberBrush", Brushes.DarkGoldenrod);
    }

    private async Task OpenSetupExperienceAsync()
    {
        if (_setupWindowOpen)
            return;

        _setupWindowOpen = true;
        try
        {
            var wizard = new SetupWindow(SettingsPathBox.Text.Trim())
            {
                Owner = this
            };
            var completed = wizard.ShowDialog() == true;

            if (completed)
            {
                _setupComplete = true;
                LoadSettingsFromFile(SettingsPathBox.Text);
                ApplySetupExperienceState();
                SelectNavigationPage("Dashboard");
                await RefreshDashboardAsync();
                SetStatus("Guided Setup completed. All KMTGuard services are ready.", true);
                return;
            }

            ApplySetupExperienceState();
            if (!string.IsNullOrWhiteSpace(wizard.RequestedPage))
                SelectNavigationPage(wizard.RequestedPage);
            else
                SelectNavigationPage("Dashboard");
        }
        finally
        {
            _setupWindowOpen = false;
        }
    }

    private async void OpenSetupButton_Click(object sender, RoutedEventArgs e)
    {
        await OpenSetupExperienceAsync();
    }

    private void CopySupportReportButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var runtime = _filterRuntimeService.GetStatus();
            var report = new StringBuilder()
                .AppendLine("KMTGuard Support Report")
                .AppendLine($"Generated: {DateTimeOffset.Now:yyyy-MM-dd HH:mm:ss zzz}")
                .AppendLine($"Admin Desktop: {Assembly.GetExecutingAssembly().GetName().Version}")
                .AppendLine($"Guided setup: {(_setupComplete ? "Complete" : "Required")}")
                .AppendLine()
                .AppendLine("Configuration")
                .AppendLine($"SQL host: {AddressBox.Text.Trim()}")
                .AppendLine($"SQL port: {(string.IsNullOrWhiteSpace(PortBox.Text) ? "default" : PortBox.Text.Trim())}")
                .AppendLine($"Database: {ProxyDbBox.Text.Trim()}")
                .AppendLine($"Server IP: {ServerIpBox.Text.Trim()}")
                .AppendLine($"Settings file: {SettingsPathBox.Text.Trim()}")
                .AppendLine()
                .AppendLine("Runtime");

            foreach (var service in runtime.Services)
                report.AppendLine($"{service.Role}: {service.Status}; executable={(File.Exists(service.Executable) ? "present" : "missing")}");

            Clipboard.SetText(report.ToString());
            SetStatus("A password-free support report was copied to the clipboard.", true);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private void NavigationList_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (!_uiReady)
            return;

        if (NavigationList.SelectedItem is not ListBoxItem item || item.Tag is not string page)
            return;

        if (page == "Setup")
        {
            _ = OpenSetupExperienceAsync();
            return;
        }

        ShowPage(page);
    }

    private void NavigationGroupList_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (!_uiReady || _suppressNavigationGroupSelection)
            return;

        if (NavigationGroupList.SelectedItem is not ListBoxItem { Tag: string group })
            return;

        _selectedNavigationGroup = group;
        ClearNavigationSearch();
        ApplyNavigationFilter();

        var firstPage = NavigationList.Items
            .OfType<ListBoxItem>()
            .FirstOrDefault(item => item.Tag is string && item.Visibility == Visibility.Visible);
        if (firstPage is not null)
            NavigationList.SelectedItem = firstPage;
    }

    private void MainWindow_PreviewKeyDown(object sender, KeyEventArgs e)
    {
        if (e.Key == Key.K && Keyboard.Modifiers.HasFlag(ModifierKeys.Control))
        {
            NavigationSearchBox.Focus();
            NavigationSearchBox.SelectAll();
            e.Handled = true;
        }
    }

    private void NavigationSearchBox_TextChanged(object sender, TextChangedEventArgs e)
    {
        if (NavigationSearchHint is null || ClearNavigationSearchButton is null || NavigationList is null)
            return;

        var query = NavigationSearchBox.Text.Trim();
        var hasQuery = query.Length > 0;
        NavigationSearchHint.Visibility = hasQuery ? Visibility.Collapsed : Visibility.Visible;
        ClearNavigationSearchButton.Visibility = hasQuery ? Visibility.Visible : Visibility.Collapsed;

        ApplyNavigationFilter(query);
    }

    private void NavigationSearchBox_KeyDown(object sender, KeyEventArgs e)
    {
        if (e.Key == Key.Escape)
        {
            ClearNavigationSearch();
            e.Handled = true;
            return;
        }

        if (e.Key != Key.Enter)
            return;

        var target = NavigationList.Items
            .OfType<ListBoxItem>()
            .FirstOrDefault(item => item.Tag is string && item.Visibility == Visibility.Visible);

        if (target?.Tag is string page)
            SelectNavigationPage(page);

        e.Handled = true;
    }

    private void ClearNavigationSearchButton_Click(object sender, RoutedEventArgs e)
    {
        ClearNavigationSearch();
        NavigationSearchBox.Focus();
    }

    private void NavigateToPageButton_Click(object sender, RoutedEventArgs e)
    {
        if (sender is Button { Tag: string page })
            SelectNavigationPage(page);
    }

    private void SelectNavigationPage(string page)
    {
        ClearNavigationSearch();
        var group = GetNavigationGroup(page);
        if (!string.Equals(group, _selectedNavigationGroup, StringComparison.OrdinalIgnoreCase))
        {
            _selectedNavigationGroup = group;
            var groupItem = NavigationGroupList.Items
                .OfType<ListBoxItem>()
                .FirstOrDefault(candidate => string.Equals(candidate.Tag as string, group, StringComparison.OrdinalIgnoreCase));
            if (groupItem is not null)
            {
                _suppressNavigationGroupSelection = true;
                NavigationGroupList.SelectedItem = groupItem;
                _suppressNavigationGroupSelection = false;
            }
            ApplyNavigationFilter();
        }

        var item = NavigationList.Items
            .OfType<ListBoxItem>()
            .FirstOrDefault(candidate => string.Equals(candidate.Tag as string, page, StringComparison.OrdinalIgnoreCase));

        if (item is null)
            return;

        if (!ReferenceEquals(NavigationList.SelectedItem, item))
            NavigationList.SelectedItem = item;
        else
            ShowPage(page);
        item.BringIntoView();
    }

    private void ApplyNavigationFilter(string? query = null)
    {
        if (NavigationList is null)
            return;

        query = string.IsNullOrWhiteSpace(query) ? null : query.Trim();
        var visibleCount = 0;
        foreach (var item in NavigationList.Items.OfType<ListBoxItem>())
        {
            if (item.Tag is not string page)
            {
                item.Visibility = Visibility.Collapsed;
                continue;
            }

            var allowedBySetup = _setupComplete || IsGuidedSetupPage(page);
            var visible = allowedBySetup && (query is not null
                ? GetNavigationItemText(item).Contains(query, StringComparison.OrdinalIgnoreCase)
                : string.Equals(GetNavigationGroup(page), _selectedNavigationGroup, StringComparison.OrdinalIgnoreCase));
            item.Visibility = visible ? Visibility.Visible : Visibility.Collapsed;
            if (visible)
                visibleCount++;
        }

        if (NavigationGroupTitleText is null || NavigationGroupSubtitleText is null || NavigationGroupCountText is null)
            return;

        if (query is not null)
        {
            NavigationGroupTitleText.Text = "Search results";
            NavigationGroupSubtitleText.Text = "Across every workspace";
        }
        else
        {
            NavigationGroupTitleText.Text = _selectedNavigationGroup switch
            {
                "Events" => "Event automation",
                "Content" => "Content & economy",
                "BotOperations" => "Bot Operations",
                "SystemGame" => "System Game",
                "Notifications" => "Notifications",
                "System" => "Data & system",
                _ => _selectedNavigationGroup
            };
            NavigationGroupSubtitleText.Text = _selectedNavigationGroup switch
            {
                "Workspace" => "Daily server control",
                "Operations" => "Runtime, player commands and protection",
                "Events" => "Schedules and competitions",
                "Content" => "Rewards, offers and activity",
                "BotOperations" => "Clientless accounts and automation",
                "SystemGame" => "GameServer rules and controls",
                "Notifications" => "Discord and Telegram delivery",
                _ => "Client tools and audit history"
            };
        }

        NavigationGroupCountText.Text = visibleCount.ToString();
        if (NavigationNoResultsPanel is not null)
            NavigationNoResultsPanel.Visibility = visibleCount == 0 ? Visibility.Visible : Visibility.Collapsed;
    }

    private static bool IsClientlessPage(string page) => page is
        "ClientlessOverview" or
        "ClientlessRuntime" or
        "ClientlessProvisioning" or
        "ClientlessAccounts" or
        "ClientlessHunting" or
        "ClientlessParty";

    private static string GetNavigationGroup(string page) => page switch
    {
        "Dashboard" or "Setup" or "Connection" or "Players" => "Workspace",
        "Settings" or "GameServerPatch" or "Runtime" or "PlayerCommands" or "Security" or "PacketRules" => "Operations",
        "AutoEvents" or "SurvivalParty" or "SurvivalSolo" or "HideAndSeek" or "LastManStanding" or "MadnessSolo" or "DefendTower" => "Events",
        "Rewards" or "LuckySpin" or "SpecialOffers" or "KillerAnimations" or "VipSystem" or "EconomyLogs" or "PvpChallenge" => "Content",
        "ClientlessOverview" or "ClientlessRuntime" or "ClientlessProvisioning" or "ClientlessAccounts" or "ClientlessHunting" or "ClientlessParty" => "BotOperations",
        "UniqueRules" or "RegionControl" or "Scheduler" or "WebViewer" => "SystemGame",
        "Discord" or "Telegram" => "Notifications",
        _ => "System"
    };

    private void ClearNavigationSearch()
    {
        if (NavigationSearchBox.Text.Length > 0)
            NavigationSearchBox.Clear();
    }

    private static string GetNavigationItemText(ListBoxItem item)
    {
        if (item.Content is StackPanel panel)
        {
            return string.Join(" ", panel.Children
                .OfType<TextBlock>()
                .Select(text => text.Text));
        }

        return item.Content?.ToString() ?? item.Tag?.ToString() ?? string.Empty;
    }

    private void MinimizeWindowButton_Click(object sender, RoutedEventArgs e)
    {
        WindowState = WindowState.Minimized;
    }

    private void MaximizeWindowButton_Click(object sender, RoutedEventArgs e)
    {
        WindowState = WindowState == WindowState.Maximized
            ? WindowState.Normal
            : WindowState.Maximized;
    }

    private void CloseWindowButton_Click(object sender, RoutedEventArgs e)
    {
        Close();
    }

    private void MainWindow_StateChanged(object? sender, EventArgs e)
    {
        MaximizeGlyphText.Text = WindowState == WindowState.Maximized ? "\uE923" : "\uE922";
        WindowFrame.BorderThickness = WindowState == WindowState.Maximized
            ? new Thickness(0)
            : new Thickness(1);
    }

    private async void RefreshButton_Click(object sender, RoutedEventArgs e)
    {
        await RefreshCurrentPageAsync();
    }

    private async void TestSqlButton_Click(object sender, RoutedEventArgs e)
    {
        await RefreshDashboardAsync(showDialog: true);
    }

    private void BrowseSettingsButton_Click(object sender, RoutedEventArgs e)
    {
        var dialog = new OpenFileDialog
        {
            Title = "Select KMTGuard Settings.json",
            Filter = "Settings.json|Settings.json|JSON files|*.json|All files|*.*",
            CheckFileExists = true
        };

        if (dialog.ShowDialog(this) == true)
            SettingsPathBox.Text = dialog.FileName;
    }

    private void LoadSettingsButton_Click(object sender, RoutedEventArgs e)
    {
        LoadSettingsFromFile(SettingsPathBox.Text);
    }

    private void SaveSettingsButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var path = SettingsPathBox.Text.Trim();
            var settings = ReadSettingsFromFields();
            _settingsFileService.Save(path, settings);
            _loadedSettings = settings.Clone();
            Environment.SetEnvironmentVariable("KMTGUARD_SETTINGS_PATH", path);
            SetStatus($"Settings.json saved: {path}", true);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void LoadDbSettingsButton_Click(object sender, RoutedEventArgs e)
    {
        await LoadDbSettingsAsync();
    }

    private async void ApplyPlayerLanguageButton_Click(object sender, RoutedEventArgs e)
    {
        var previousLanguage = NormalizePlayerLanguage(_loadedSettings?.Language);
        var requestedLanguage = ReadSelectedPlayerLanguage();
        ApplyPlayerLanguageButton.IsEnabled = false;
        PlayerLanguageStatusText.Text = "Applying language to Filter services...";

        try
        {
            var updatedServices = await _filterRuntimeService.SetPlayerLanguageAsync(
                requestedLanguage,
                previousLanguage);

            var path = SettingsPathBox.Text.Trim();
            if (string.IsNullOrWhiteSpace(path))
            {
                path = _settingsFileService.GetRuntimeSettingsPath();
                SettingsPathBox.Text = path;
            }

            try
            {
                var settings = ReadSettingsFromFields();
                _settingsFileService.Save(path, settings);
                _loadedSettings = settings.Clone();
                Environment.SetEnvironmentVariable("KMTGUARD_SETTINGS_PATH", path);
            }
            catch
            {
                if (updatedServices > 0)
                    await _filterRuntimeService.SetPlayerLanguageAsync(previousLanguage, requestedLanguage);
                throw;
            }

            PlayerLanguageStatusText.Text = updatedServices > 0
                ? $"{requestedLanguage} is active in {updatedServices} running service(s)."
                : $"{requestedLanguage} saved and will be active when Filter starts.";
            SetStatus($"Player notice language changed to {requestedLanguage}.", true);
        }
        catch (Exception ex)
        {
            SelectPlayerLanguage(previousLanguage);
            PlayerLanguageStatusText.Text = $"Language was not changed: {ex.Message}";
            ShowError(ex.Message);
        }
        finally
        {
            ApplyPlayerLanguageButton.IsEnabled = true;
        }
    }

    private async void LoadProxyServicesButton_Click(object sender, RoutedEventArgs e)
    {
        await LoadProxyServicesConnectionAsync();
    }

    private async void SaveProxyServicesButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await SaveProxyServicesAsync();
        }
        catch (Exception ex)
        {
            ProxyServicesStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void SettingsSearchBox_TextChanged(object sender, TextChangedEventArgs e)
    {
        BuildSettingsSections();
    }

    private void SettingsCategoryList_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (SettingsCategoryList.SelectedItem is not SectionDefinition section)
            return;

        _selectedSettingsSection = section.Key;
        RenderSettingsSection(section.Key);
    }

    private async void SaveSettingsSectionButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (string.IsNullOrWhiteSpace(_selectedSettingsSection))
                throw new InvalidOperationException("Choose a settings section first.");

            var service = CreateSqlService();
            var changed = 0;

            foreach (var entry in GetVisibleSettingsForSection(_selectedSettingsSection))
            {
                if (!_settingInputs.TryGetValue(entry.SettingName, out var input))
                    continue;

                var value = ReadSettingInputValue(entry, input);
                if (value == entry.Value)
                    continue;

                ValidateSettingValue(entry, value);
                await service.UpdateSettingAsync(entry.Store, entry.SettingName, value);
                changed++;
            }

            await LoadDbSettingsAsync();
            SettingsFooterText.Text = changed == 0
                ? "No changes to save."
                : _selectedSettingsSection.StartsWith("GameServer.", StringComparison.OrdinalIgnoreCase)
                    ? $"{changed:N0} GameServer setting(s) saved. Restart every GameServer to apply the changes."
                    : $"{changed:N0} setting(s) saved. Restart the relevant service for cached values; GameServer settings require a GameServer restart.";
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void QueueNoticeButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var noticeType = NoticeTypeBox.Text.Trim();
            var message = NoticeMessageBox.Text.Trim();
            if (string.IsNullOrWhiteSpace(message))
                throw new InvalidOperationException("Notice message is required.");

            if (!byte.TryParse(noticeType, out _))
                throw new InvalidOperationException("Notice type must be a byte value.");

            await CreateSqlService().QueueRuntimeCommandAsync(3, "sendall", noticeType, message);
            RuntimeResultText.Text = "Notice command queued.";
            await RefreshDashboardAsync();
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void DisconnectCharButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var charName = DisconnectCharBox.Text.Trim();
            if (string.IsNullOrWhiteSpace(charName))
                throw new InvalidOperationException("Character name is required.");

            await CreateSqlService().QueueRuntimeCommandAsync(1, charName);
            RuntimeResultText.Text = $"Disconnect command queued for {charName}.";
            await RefreshDashboardAsync();
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void DisconnectAllButton_Click(object sender, RoutedEventArgs e)
    {
        if (MessageBox.Show(this,
                "This queues a disconnect command for all online sessions. Continue?",
                "Confirm disconnect all",
                MessageBoxButton.YesNo,
                MessageBoxImage.Warning) != MessageBoxResult.Yes)
            return;

        try
        {
            await CreateSqlService().QueueRuntimeCommandAsync(2);
            RuntimeResultText.Text = "Disconnect-all command queued.";
            await RefreshDashboardAsync();
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void QueueRankReloadButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().QueueRuntimeCommandAsync(40);
            RuntimeResultText.Text = "Rank and achievement reload command queued.";
            await RefreshDashboardAsync();
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void LoadClientlessButton_Click(object sender, RoutedEventArgs e) => await LoadClientlessAsync();

    private void BrowseClientlessImportButton_Click(object sender, RoutedEventArgs e)
    {
        var dialog = new OpenFileDialog
        {
            Title = "Select clientless account list",
            Filter = "Text files|*.txt;*.csv|All files|*.*",
            CheckFileExists = true
        };

        if (dialog.ShowDialog(this) == true)
            ClientlessImportPathBox.Text = dialog.FileName;
    }

    private async void ImportClientlessButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var result = await CreateSqlService().ImportClientlessAccountsAsync(
                ClientlessImportPathBox.Text.Trim(),
                ParseRequiredInt(ClientlessShardBox.Text, "Shard"),
                ParseRequiredByte(ClientlessLocaleBox.Text, "Locale"),
                ParseRequiredInt(ClientlessLaunchDelayBox.Text, "Launch delay"),
                ParseRequiredInt(ClientlessReconnectBox.Text, "Reconnect delay"));

            ClientlessImportResultText.Text = result;
            SetStatus(result, true);
            await LoadClientlessAsync();
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void BulkCreateClientlessButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var race = ReadComboTag(ClientlessBulkRaceBox, "Random");
            var weapon = ReadComboTag(ClientlessBulkWeaponBox, "Random");
            var gender = ReadComboTag(ClientlessBulkGenderBox, "Random");
            var equipment = ReadComboTag(ClientlessBulkEquipmentBox, "Rare");
            var skills = ReadComboTag(ClientlessBulkSkillsBox, "MasteryAndSkills");
            var build = ReadComboTag(ClientlessBulkBuildBox, "Random");
            var includeAvatars = ReadComboTag(ClientlessBulkAvatarsBox, "Enabled") == "Enabled";
            var includeAttackPet = ReadComboTag(ClientlessBulkAttackPetBox, "Enabled") == "Enabled";
            var includeGrabPet = ReadComboTag(ClientlessBulkGrabPetBox, "Enabled") == "Enabled";
            var cityCounts = new Dictionary<string, int>(StringComparer.OrdinalIgnoreCase)
            {
                ["Jangan"] = ParseRequiredInt(ClientlessJanganCountBox.Text, "Jangan count"),
                ["Donwhang"] = ParseRequiredInt(ClientlessDonwhangCountBox.Text, "Donwhang count"),
                ["Hotan"] = ParseRequiredInt(ClientlessHotanCountBox.Text, "Hotan count"),
                ["SamarKand"] = ParseRequiredInt(ClientlessSamarkandCountBox.Text, "SamarKand count"),
                ["Constantinople"] = ParseRequiredInt(ClientlessConstantinopleCountBox.Text, "Constantinople count"),
                ["Alexandria North (SD)"] = ParseRequiredInt(ClientlessAlexandriaNorthCountBox.Text, "Alexandria North count")
            };
            if (cityCounts.Any(item => item.Value < 0))
                throw new InvalidOperationException("City account counts cannot be negative.");

            var count = cityCounts.Values.Sum();
            if (count is < 1 or > 5000)
                throw new InvalidOperationException("The combined city account count must be between 1 and 5,000.");

            var allocationSummary = string.Join(", ",
                cityCounts.Where(item => item.Value > 0).Select(item => $"{item.Value:N0} in {item.Key}"));
            if (MessageBox.Show(this,
                    $"Create {count:N0} new clientless account(s)?\n\n{allocationSummary}\n\nRace: {race}\nWeapon: {weapon}\nGender: {gender}\nEquipment: {equipment}\nSkills: {skills}\nBuild: {build}\nAvatars: {(includeAvatars ? "Enabled" : "Disabled")}\nAttack pet: {(includeAttackPet ? "Enabled" : "Disabled")}\nGrab pet: {(includeGrabPet ? "Enabled" : "Disabled")}\n\nExisting accounts will be kept.",
                    "Create clientless accounts",
                    MessageBoxButton.YesNo,
                    MessageBoxImage.Question) != MessageBoxResult.Yes)
                return;

            var sqlService = CreateSqlService();
            ClientlessBulkResultText.Text = "Creating and saving the new city groups...";
            var result = await sqlService.BulkCreateEquippedClientlessAccountsAsync(
                cityCounts,
                ClientlessBulkPasswordBox.Text,
                ParseRequiredInt(ClientlessBulkMinLevelBox.Text, "Minimum level"),
                ParseRequiredInt(ClientlessBulkMaxLevelBox.Text, "Maximum level"),
                ParseRequiredInt(ClientlessShardBox.Text, "Shard"),
                ParseRequiredByte(ClientlessLocaleBox.Text, "Locale"),
                ParseRequiredInt(ClientlessLaunchDelayBox.Text, "Launch delay"),
                ParseRequiredInt(ClientlessReconnectBox.Text, "Reconnect delay"),
                race,
                weapon,
                gender,
                equipment,
                skills,
                build,
                includeAvatars,
                includeAttackPet,
                includeGrabPet);

            ClientlessBulkResultText.Text = result;
            ClientlessImportResultText.Text = result;
            SetStatus(result, true);
            await LoadClientlessAsync();
        }
        catch (Exception ex)
        {
            ClientlessBulkResultText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void PrepareExistingClientlessSkillsButton_Click(object sender, RoutedEventArgs e)
    {
        if (MessageBox.Show(this,
                "Prepare every existing Clientless character for real combat?\n\nThis safely stops Clientless sessions, corrects STR/INT allocation, equips arrows or bolts, opens the weapon and Chinese support masteries within the server cap, installs only the highest valid skill rank per branch, then starts the accounts again.",
                "Prepare existing Clientless combat",
                MessageBoxButton.YesNo,
                MessageBoxImage.Question) != MessageBoxResult.Yes)
            return;

        var clientlessStopped = false;
        try
        {
            ClientlessBulkResultText.Text = "Stopping Clientless sessions before changing live character data...";
            var stopResult = await _filterRuntimeService.StopClientlessAsync(null);
            clientlessStopped = true;
            await Task.Delay(TimeSpan.FromSeconds(5));
            ClientlessBulkResultText.Text = "Repairing stats, ammunition, masteries, attack skills, and self buffs...";
            var prepareResult = await CreateSqlService().PrepareExistingClientlessWeaponSkillsAsync();
            var startResult = await _filterRuntimeService.StartClientlessAsync(null, null);
            clientlessStopped = false;
            var result = stopResult + Environment.NewLine + prepareResult + Environment.NewLine + startResult;
            ClientlessBulkResultText.Text = result;
            SetStatus(result, true);
            MessageBox.Show(this,
                result,
                "Clientless combat prepared",
                MessageBoxButton.OK,
                MessageBoxImage.Information);
            await LoadClientlessAsync();
        }
        catch (Exception ex)
        {
            var message = ex.Message;
            if (clientlessStopped)
            {
                try
                {
                    var recovery = await _filterRuntimeService.StartClientlessAsync(null, null);
                    message += Environment.NewLine + "Clientless restart recovery: " + recovery;
                }
                catch (Exception recoveryException)
                {
                    message += Environment.NewLine + "Clientless restart recovery failed: " + recoveryException.Message;
                }
            }
            ClientlessBulkResultText.Text = message;
            ShowError(message);
        }
    }

    private void ClientlessBulkRaceBox_SelectionChanged(object sender, SelectionChangedEventArgs e) =>
        RefreshClientlessWeaponChoices();

    private void RefreshClientlessWeaponChoices()
    {
        if (ClientlessBulkRaceBox is null || ClientlessBulkWeaponBox is null)
            return;

        var race = ReadComboTag(ClientlessBulkRaceBox, "Random");
        var previousWeapon = ReadComboTag(ClientlessBulkWeaponBox, "Random");
        var choices = SqlAdminService.GetClientlessWeaponChoices(race);
        ClientlessBulkWeaponBox.Items.Clear();
        foreach (var weapon in choices)
            ClientlessBulkWeaponBox.Items.Add(new ComboBoxItem { Content = weapon, Tag = weapon });

        ClientlessBulkWeaponBox.SelectedItem = ClientlessBulkWeaponBox.Items
            .OfType<ComboBoxItem>()
            .FirstOrDefault(item => string.Equals(item.Tag?.ToString(), previousWeapon, StringComparison.OrdinalIgnoreCase));
        if (ClientlessBulkWeaponBox.SelectedItem is null)
            ClientlessBulkWeaponBox.SelectedIndex = 0;
    }

    private async void SaveClientlessAccountButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().SaveClientlessAccountAsync(
                ClientlessAccountBox.Text,
                ClientlessPasswordBox.Text,
                ClientlessCharacterBox.Text,
                GetSelectedClientlessAccountCity(),
                ParseRequiredInt(ClientlessShardBox.Text, "Shard"),
                ParseRequiredByte(ClientlessLocaleBox.Text, "Locale"),
                ParseRequiredInt(ClientlessLaunchDelayBox.Text, "Launch delay"),
                ParseRequiredInt(ClientlessReconnectBox.Text, "Reconnect delay"));

            SetStatus("Clientless account saved.", true);
            await LoadClientlessAsync();
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void ToggleClientlessAccountButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (GetSelectedRow(ClientlessGrid) is not { } row || !row.Row.Table.Columns.Contains("ID"))
                throw new InvalidOperationException("Choose a clientless account row first.");

            var id = Convert.ToInt32(row["ID"]);
            var enabled = row.Row.Table.Columns.Contains("Enabled") && Convert.ToBoolean(row["Enabled"]);
            if (enabled && _filterRuntimeService.IsAgentRunning)
                await _filterRuntimeService.StopClientlessAccountAsync(id);
            await CreateSqlService().SetClientlessAccountEnabledAsync(id, !enabled);
            SetStatus($"Clientless account #{id} {(enabled ? "disabled" : "enabled")}.", true);
            await LoadClientlessAsync();
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void DeleteClientlessAccountButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (GetSelectedRow(ClientlessGrid) is not { } row || !row.Row.Table.Columns.Contains("ID"))
                throw new InvalidOperationException("Choose a clientless account row first.");

            var id = Convert.ToInt32(row["ID"]);
            if (MessageBox.Show(this, $"Delete clientless account row #{id}?", "Clientless", MessageBoxButton.YesNo, MessageBoxImage.Warning) != MessageBoxResult.Yes)
                return;

            if (_filterRuntimeService.IsAgentRunning)
                await _filterRuntimeService.StopClientlessAccountAsync(id);
            await CreateSqlService().DeleteClientlessAccountAsync(id);
            SetStatus($"Clientless account #{id} deleted.", true);
            await LoadClientlessAsync();
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void DeleteAllClientlessAccountsButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (MessageBox.Show(this,
                    "Clear every row from [dbo].[Clientless_Accounts]? Game accounts and characters will not be deleted.",
                    "Clear clientless list",
                    MessageBoxButton.YesNo,
                    MessageBoxImage.Warning) != MessageBoxResult.Yes)
                return;

            if (_filterRuntimeService.IsAgentRunning)
                await _filterRuntimeService.StopClientlessAsync();

            var deleted = await CreateSqlService().DeleteAllClientlessAccountsAsync();
            var result = $"Cleared {deleted:N0} clientless account row(s).";
            ClientlessGridStatusText.Text = result;
            ClientlessRuntimeText.Text = _filterRuntimeService.IsAgentRunning
                ? await _filterRuntimeService.GetClientlessStatusAsync()
                : "Agent service is stopped.";
            SetStatus(result, true);
            await LoadClientlessAsync();
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private void ClientlessGrid_MouseDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
    {
        if (GetSelectedRow(ClientlessGrid) is not { } row)
            return;

        ClientlessAccountBox.Text = ReadColumnText(row, "AccountName", string.Empty);
        ClientlessCharacterBox.Text = ReadColumnText(row, "CharacterName", string.Empty);
        SetClientlessCitySelection(ClientlessAccountCityBox, ReadColumnText(row, "City", "Unassigned"));
        ClientlessShardBox.Text = ReadColumnText(row, "ShardID", "64");
        ClientlessLocaleBox.Text = ReadColumnText(row, "Locale", "22");
        ClientlessLaunchDelayBox.Text = ReadColumnText(row, "LaunchDelayMs", "1000");
        ClientlessReconnectBox.Text = ReadColumnText(row, "ReconnectDelaySeconds", "30");
        ClientlessPasswordBox.Text = string.Empty;
        ClientlessGridStatusText.Text = "Row loaded. Enter a password only if you want to update it.";
    }

    private async void StartClientlessButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var allocationResult = await ApplyClientlessCityPlansAsync(refreshRuntime: false);
            await EnsureFilterServicesRunningForClientlessAsync();
            var startResult = await RunClientlessCityActionAsync(city =>
                _filterRuntimeService.ReloadClientlessAsync(null, city));
            var refreshResult = await RunClientlessCityActionAsync(city =>
                _filterRuntimeService.RefreshClientlessHuntingAsync(city));
            var result = allocationResult + Environment.NewLine + startResult + Environment.NewLine + refreshResult;
            await LoadClientlessAsync();
            ClientlessRuntimeText.Text = result;
            SetStatus(result, true);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void ReloadClientlessButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var allocationResult = await ApplyClientlessCityPlansAsync(refreshRuntime: false);
            await EnsureFilterServicesRunningForClientlessAsync();
            var reloadResult = await RunClientlessCityActionAsync(city =>
                _filterRuntimeService.ReloadClientlessAsync(null, city));
            var refreshResult = await RunClientlessCityActionAsync(city =>
                _filterRuntimeService.RefreshClientlessHuntingAsync(city));
            var result = allocationResult + Environment.NewLine + reloadResult + Environment.NewLine + refreshResult;
            await LoadClientlessAsync();
            ClientlessRuntimeText.Text = result;
            SetStatus(result, true);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void StopClientlessButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var result = await RunClientlessCityActionAsync(city =>
                _filterRuntimeService.StopClientlessAsync(city));
            await LoadClientlessAsync();
            ClientlessRuntimeText.Text = result;
            SetStatus(result, true);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void ApplyClientlessHunterCountButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var result = await ApplyClientlessCityPlansAsync(
                refreshRuntime: _filterRuntimeService.IsAgentRunning);
            await LoadClientlessAsync();
            ClientlessRuntimeText.Text = result;
            SetStatus(result, true);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void ParkSelectedClientlessCitiesButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            foreach (var plan in GetSelectedClientlessCityPlans())
                plan.DesiredHuntersText = "0";
            var result = await ApplyClientlessCityPlansAsync(
                refreshRuntime: _filterRuntimeService.IsAgentRunning);
            await LoadClientlessAsync();
            ClientlessRuntimeText.Text = result;
            SetStatus(result + Environment.NewLine + "Selected accounts remain online and are returning to their city standby points.", true);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private void SelectAllClientlessCityPlansButton_Click(object sender, RoutedEventArgs e)
    {
        foreach (var plan in _clientlessCityHunterPlans)
            plan.IsSelected = true;
    }

    private void ClearClientlessCityPlansButton_Click(object sender, RoutedEventArgs e)
    {
        foreach (var plan in _clientlessCityHunterPlans)
            plan.IsSelected = false;
    }

    private IReadOnlyList<ClientlessCityHunterPlan> GetSelectedClientlessCityPlans()
    {
        var selected = _clientlessCityHunterPlans.Where(plan => plan.IsSelected).ToArray();
        if (selected.Length == 0)
            throw new InvalidOperationException("Select at least one city in the runtime plan.");
        return selected;
    }

    private static (int Online, int Hunters) ParseClientlessCityPlan(ClientlessCityHunterPlan plan)
    {
        if (!int.TryParse(plan.DesiredOnlineText?.Trim(), out var online) || online is < 0 or > 5000)
            throw new InvalidOperationException($"{plan.City}: Online wanted must be between 0 and 5,000.");
        if (!int.TryParse(plan.DesiredHuntersText?.Trim(), out var hunters) || hunters is < 0 or > 5000)
            throw new InvalidOperationException($"{plan.City}: Hunters wanted must be between 0 and 5,000.");
        if (hunters > online)
            throw new InvalidOperationException($"{plan.City}: Hunters wanted cannot be greater than Online wanted.");
        return (online, hunters);
    }

    private async Task<string> ApplyClientlessCityPlansAsync(bool refreshRuntime)
    {
        var sqlService = CreateSqlService();
        var results = new List<string>();
        foreach (var plan in GetSelectedClientlessCityPlans())
        {
            var requested = ParseClientlessCityPlan(plan);
            var allocation = await sqlService.SetClientlessCityRuntimePlanAsync(
                plan.City,
                requested.Online,
                requested.Hunters);
            results.Add(FormatClientlessHunterAllocation(allocation));
            if (refreshRuntime)
            {
                results.Add(await _filterRuntimeService.ReloadClientlessAsync(null, plan.City));
                results.Add(await _filterRuntimeService.RefreshClientlessHuntingAsync(plan.City));
            }
        }

        return string.Join(Environment.NewLine, results.Where(result => !string.IsNullOrWhiteSpace(result)));
    }

    private static string FormatClientlessHunterAllocation(ClientlessHunterAllocationResult result)
    {
        if (result.Cities.Count == 0)
            return "No Clientless accounts were found in the selected cities.";

        return string.Join(Environment.NewLine, result.Cities.Select(city =>
            $"{city.City}: {city.ReadyEnabledAccounts:N0} online; " +
            $"{city.ActiveHunters:N0} hunting; " +
            $"{city.ParkedReadyAccounts:N0} parked in the city; " +
            $"{city.OfflineReadyAccounts:N0} ready account(s) kept offline; " +
            $"{city.UnavailableAccounts:N0} account(s) not game-ready."));
    }

    private async Task<string> RunClientlessCityActionAsync(Func<string?, Task<string>> action)
    {
        var results = new List<string>();
        foreach (var city in GetSelectedClientlessRuntimeCities())
        {
            var response = await action(city);
            results.Add(city is null ? response : $"{city}: {response}");
        }

        return string.Join(Environment.NewLine, results);
    }

    private IReadOnlyList<string?> GetSelectedClientlessRuntimeCities()
    {
        return GetSelectedClientlessCityPlans()
            .Select(plan => (string?)plan.City)
            .ToArray();
    }

    private string GetSelectedClientlessAccountCity()
    {
        var city = (ClientlessAccountCityBox.SelectedItem as ComboBoxItem)?.Tag?.ToString();
        return SqlAdminService.NormalizeClientlessTownOrUnassigned(city);
    }

    private static void SetClientlessCitySelection(ComboBox comboBox, string city)
    {
        foreach (var item in comboBox.Items.OfType<ComboBoxItem>())
        {
            if (!string.Equals(item.Tag?.ToString(), city, StringComparison.OrdinalIgnoreCase))
                continue;

            comboBox.SelectedItem = item;
            return;
        }

        comboBox.SelectedIndex = 0;
    }

    private async void EnableClientlessPartyFormButton_Click(object sender, RoutedEventArgs e)
    {
        await SetClientlessPartyFormPolicyAsync(true);
    }

    private async void DisableClientlessPartyFormButton_Click(object sender, RoutedEventArgs e)
    {
        await SetClientlessPartyFormPolicyAsync(false);
    }

    private void ClientlessPartyModeBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        UpdateClientlessPartyModeUi();
    }

    private void UpdateClientlessPartyModeUi()
    {
        if (ClientlessPartyModeBox is null ||
            ClientlessPartyModeHelpText is null ||
            BuildClientlessPartiesButton is null)
            return;

        var grouped = string.Equals(
            ReadComboTag(ClientlessPartyModeBox, "SoloForms"),
            "GroupsOf8",
            StringComparison.OrdinalIgnoreCase);
        ClientlessPartyModeHelpText.Text = grouped
            ? "Characters are grouped by city into real parties of up to 8. Only each confirmed leader publishes a Party Form."
            : "Every character stays outside managed parties and publishes its own Party Form in Party Matching.";
        BuildClientlessPartiesButton.IsEnabled = grouped;
        if (!grouped && ClientlessPartyGroupingStatusText is not null)
            ClientlessPartyGroupingStatusText.Text = "Individual Forms mode does not join Clientless characters together.";
    }

    private async void BuildClientlessPartiesButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (!string.Equals(
                    ReadComboTag(ClientlessPartyModeBox, "SoloForms"),
                    "GroupsOf8",
                    StringComparison.OrdinalIgnoreCase))
                throw new InvalidOperationException("Select Groups of 8 and press Save & Apply Selected Mode first.");

            await EnsureFilterServicesRunningForClientlessAsync();
            ClientlessPartyGroupingStatusText.Text = "Creating real parties and waiting for GameServer confirmations...";
            var result = await _filterRuntimeService.BuildClientlessPartiesAsync();
            ClientlessPartyGroupingStatusText.Text = result;
            SetStatus(result, true);
        }
        catch (Exception ex)
        {
            ClientlessPartyGroupingStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async Task SetClientlessPartyFormPolicyAsync(bool enabled)
    {
        try
        {
            await EnsureFilterServicesRunningForClientlessAsync();

            var title = ClientlessPartyTitleBox.Text.Trim();
            if (title.Length is < 1 or > 64)
                throw new InvalidOperationException("Party title must contain 1 to 64 characters.");

            var minLevel = ParseRequiredInt(ClientlessPartyMinLevelBox.Text, "Party minimum level");
            var maxLevel = ParseRequiredInt(ClientlessPartyMaxLevelBox.Text, "Party maximum level");
            if (minLevel is < 1 or > 255 || maxLevel is < 1 or > 255)
                throw new InvalidOperationException("Party levels must be between 1 and 255.");
            if (minLevel > maxLevel)
                throw new InvalidOperationException("Party minimum level cannot exceed maximum level.");

            var result = await _filterRuntimeService.SetClientlessPartyFormPolicyAsync(
                new ClientlessPartyFormCommandPayload
                {
                    Enabled = enabled,
                    Mode = ReadComboTag(ClientlessPartyModeBox, "SoloForms"),
                    Title = title,
                    MinLevel = checked((byte)minLevel),
                    MaxLevel = checked((byte)maxLevel),
                    Purpose = checked((byte)ReadTaggedValue(ClientlessPartyPurposeBox, 0)),
                    SettingsFlag = checked((byte)ReadTaggedValue(ClientlessPartySettingsBox, 7))
                });

            ClientlessPartyFormStatusText.Text = result;
            SetStatus(result, true);
            await LoadClientlessPartyFormPolicyAsync();
        }
        catch (Exception ex)
        {
            ClientlessPartyFormStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void ClientlessHuntCityBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_uiReady && ClientlessHuntAreasEditor is not null)
            await LoadClientlessHuntingAsync();
    }

    private async void SaveEnableClientlessHuntingButton_Click(object sender, RoutedEventArgs e) =>
        await SaveClientlessHuntingAsync(true);

    private async void SavePauseClientlessHuntingButton_Click(object sender, RoutedEventArgs e) =>
        await SaveClientlessHuntingAsync(false);

    private async Task SaveClientlessHuntingAsync(bool enabled)
    {
        try
        {
            if (_clientlessHuntAreas == null)
                throw new InvalidOperationException("Load a city before saving its hunting areas.");

            var city = ReadComboTag(ClientlessHuntCityBox, "Jangan");
            var policy = new ClientlessHuntPolicy
            {
                Enabled = enabled,
                AttackNormal = ClientlessHuntNormalCheckBox.IsChecked == true,
                AttackUnique = ClientlessHuntUniqueCheckBox.IsChecked == true,
                UniquePriority = ClientlessHuntUniquePriorityCheckBox.IsChecked == true,
                UseSkills = ClientlessHuntSkillsCheckBox.IsChecked == true,
                UseBasicAttack = ClientlessHuntBasicAttackCheckBox.IsChecked == true,
                HpPotionPercent = ParseRequiredByte(ClientlessHuntHpBox.Text, "HP potion percent"),
                MpPotionPercent = ParseRequiredByte(ClientlessHuntMpBox.Text, "MP potion percent"),
                StuckSeconds = checked((short)ParseRequiredInt(ClientlessHuntStuckBox.Text, "Stuck seconds")),
                TargetTimeoutSeconds = checked((short)ParseRequiredInt(ClientlessHuntTargetTimeoutBox.Text, "Target timeout"))
            };

            ClientlessHuntStatusText.Text = "Saving validated hunting settings...";
            var result = await CreateSqlService().SaveClientlessHuntingAsync(city, _clientlessHuntAreas, policy);
            if (_filterRuntimeService.IsAgentRunning)
            {
                result += Environment.NewLine + await _filterRuntimeService.RefreshClientlessHuntingAsync(city);
                if (enabled)
                    result += Environment.NewLine + await _filterRuntimeService.StartClientlessAsync(null, city);
            }

            ClientlessHuntStatusText.Text = result;
            SetStatus(result, true);
            await LoadClientlessHuntingAsync();
        }
        catch (Exception ex)
        {
            ClientlessHuntStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void EnableSelectedClientlessHuntButton_Click(object sender, RoutedEventArgs e) =>
        await SetSelectedClientlessAccountHuntingAsync(true);

    private async void PauseSelectedClientlessHuntButton_Click(object sender, RoutedEventArgs e) =>
        await SetSelectedClientlessAccountHuntingAsync(false);

    private async Task SetSelectedClientlessAccountHuntingAsync(bool enabled)
    {
        try
        {
            if (GetSelectedRow(ClientlessHuntAccountsGrid) is not { } row)
                throw new InvalidOperationException("Select one account from the hunting status table first.");
            var id = Convert.ToInt32(row["ID"]);
            var city = ReadComboTag(ClientlessHuntCityBox, "Jangan");
            var result = await CreateSqlService().SetClientlessAccountHuntingAsync(id, enabled);
            if (_filterRuntimeService.IsAgentRunning)
                result += Environment.NewLine + await _filterRuntimeService.RefreshClientlessHuntingAsync(city, id);
            ClientlessHuntStatusText.Text = result;
            SetStatus(result, true);
            await LoadClientlessHuntingAsync();
        }
        catch (Exception ex)
        {
            ClientlessHuntStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void RefreshFilterStatusButton_Click(object sender, RoutedEventArgs e)
    {
        RefreshFilterProcessStatus();
    }

    private async void StartFilterButton_Click(object sender, RoutedEventArgs e)
    {
        SaveSettingsForRuntime();
        await RunFilterProcessActionAsync(() => _filterRuntimeService.StartAsync(), "All KMTGuard services started.");
    }

    private async void StopFilterButton_Click(object sender, RoutedEventArgs e)
    {
        if (MessageBox.Show(this, "Stop Gateway, Download and Agent services?", "KMTGuard Services", MessageBoxButton.YesNo, MessageBoxImage.Warning) != MessageBoxResult.Yes)
            return;

        await RunFilterProcessActionAsync(() => _filterRuntimeService.StopAsync(), "All KMTGuard services stopped.");
    }

    private async void RestartFilterButton_Click(object sender, RoutedEventArgs e)
    {
        if (MessageBox.Show(this, "Restart Gateway, Download and Agent services?", "KMTGuard Services", MessageBoxButton.YesNo, MessageBoxImage.Question) != MessageBoxResult.Yes)
            return;

        SaveSettingsForRuntime();
        await RunFilterProcessActionAsync(() => _filterRuntimeService.RestartAsync(), "All KMTGuard services restarted.");
    }

    private async void StartSelectedRuntimeButton_Click(object sender, RoutedEventArgs e)
    {
        if (GetSelectedRuntimeRole() is not { } role)
        {
            ShowError("Select Agent, Download or Gateway first.");
            return;
        }

        SaveSettingsForRuntime();
        await RunFilterProcessActionAsync(
            () => _filterRuntimeService.StartRoleAsync(role),
            $"KMTGuard {role} service started.");
    }

    private async void StopSelectedRuntimeButton_Click(object sender, RoutedEventArgs e)
    {
        if (GetSelectedRuntimeRole() is not { } role)
        {
            ShowError("Select Agent, Download or Gateway first.");
            return;
        }

        if (MessageBox.Show(this, $"Stop the {role} service?", "KMTGuard Services", MessageBoxButton.YesNo, MessageBoxImage.Warning) != MessageBoxResult.Yes)
            return;

        await RunFilterProcessActionAsync(
            () => _filterRuntimeService.StopRoleAsync(role),
            $"KMTGuard {role} service stopped.");
    }

    private async void RestartSelectedRuntimeButton_Click(object sender, RoutedEventArgs e)
    {
        if (GetSelectedRuntimeRole() is not { } role)
        {
            ShowError("Select Agent, Download or Gateway first.");
            return;
        }

        SaveSettingsForRuntime();
        await RunFilterProcessActionAsync(
            () => _filterRuntimeService.RestartRoleAsync(role),
            $"KMTGuard {role} service restarted.");
    }

    private FilterRole? GetSelectedRuntimeRole()
    {
        return RuntimeServicesGrid.SelectedItem is FilterServiceStatus status
            ? status.Role
            : null;
    }

    private async void CheckMigrationsButton_Click(object sender, RoutedEventArgs e)
    {
        await CheckMigrationsAsync();
    }

    private void FindLogsButton_Click(object sender, RoutedEventArgs e)
    {
        LoadLogs();
    }

    private void LogFilesList_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (LogFilesList.SelectedItem is not LogFileItem item)
            return;

        try
        {
            LogTailBox.Text = _logService.ReadTail(item.Path);
            LogTailBox.ScrollToEnd();
        }
        catch (Exception ex)
        {
            LogTailBox.Text = ex.Message;
        }
    }

    private async Task RefreshCurrentPageAsync()
    {
        if (NavigationList.SelectedItem is not ListBoxItem item || item.Tag is not string page)
        {
            await RefreshDashboardAsync();
            return;
        }

        switch (page)
        {
            case "Settings":
                await LoadDbSettingsAsync();
                break;
            case "Players":
                await RefreshOnlinePlayersAsync();
                break;
            case "PlayerCommands":
                if (_playerCommandTarget is not null)
                    ApplyPlayerCommandTarget(await CreateSqlService().GetPlayerCommandTargetAsync(_playerCommandTarget.CharacterName));
                break;
            case "Rewards":
                await SearchItemsAsync();
                break;
            case "LuckySpin":
                await LoadLuckySpinAsync();
                break;
            case "SpecialOffers":
                await LoadSpecialOffersAsync();
                break;
            case "KillerAnimations":
                await LoadKillerAnimationsAsync();
                break;
            case "VipSystem":
                await LoadVipTiersAsync();
                break;
            case "EconomyLogs":
                await LoadEconomyLogAsync("LuckySpin");
                break;
            case "AutoEvents":
                await LoadAutoEventConfigAsync();
                break;
            case "SurvivalParty":
                await LoadSurvivalPartyAsync();
                break;
            case "SurvivalSolo":
                await LoadSurvivalSoloAsync();
                break;
            case "HideAndSeek":
                await LoadHideAndSeekAsync();
                break;
            case "PvpChallenge":
                await LoadPvpArenasAsync();
                break;
            case "RegionControl":
                await LoadRegionControlAsync();
                break;
            case "Security":
                await LoadBlockedWordsAsync();
                break;
            case "UniqueRules":
                await LoadUniqueRulesAsync();
                break;
            case "Scheduler":
                await LoadSchedulerAsync();
                break;
            case "Discord":
                await LoadDiscordAsync();
                break;
            case "Telegram":
                await LoadTelegramAsync();
                break;
            case "WebViewer":
                await LoadWebViewerAsync();
                break;
            case "ChatLogs":
                await SearchChatLogsAsync();
                break;
            case "PacketRules":
                await LoadPacketRulesAsync(PacketTableBox.Text);
                break;
            case "DataStudio":
                await LoadDataStudioAsync();
                break;
            case "Audit":
                await LoadAuditAsync();
                break;
            case "Diagnostics":
                await LoadDiagnosticsAsync();
                break;
            case "Migrations":
                await CheckMigrationsAsync();
                break;
            case "Logs":
                LoadLogs();
                break;
            case "Runtime":
                RefreshFilterProcessStatus();
                break;
            case "ClientlessRuntime":
            case "ClientlessProvisioning":
            case "ClientlessAccounts":
            case "ClientlessParty":
                await LoadClientlessAsync();
                break;
            case "ClientlessHunting":
                await LoadClientlessHuntingAsync();
                break;
            default:
                await RefreshDashboardAsync();
                break;
        }
    }

    private async void SearchPlayersButton_Click(object sender, RoutedEventArgs e) => await SearchPlayersAsync();

    private void PlayerCommandNameBox_TextChanged(object sender, TextChangedEventArgs e)
    {
        if (PlayerCommandActionsPanel is null || PlayerCommandTargetStatusText is null)
            return;

        _playerCommandTarget = null;
        PlayerCommandActionsPanel.IsEnabled = false;
        PlayerCommandTargetStatusText.Text = "Load an exact character name to unlock commands.";
        PlayerCommandTargetNameText.Text = "No character loaded";
        PlayerCommandTargetIdText.Text = "—";
        PlayerCommandTargetLevelText.Text = "—";
        PlayerCommandTargetRegionText.Text = "—";
        PlayerCommandTargetGoldText.Text = "—";
        PlayerCommandTargetSilkText.Text = "—";
    }

    private async void LoadPlayerCommandTargetButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            PlayerCommandTargetStatusText.Text = "Resolving character and account data...";
            var target = await CreateSqlService().GetPlayerCommandTargetAsync(PlayerCommandNameBox.Text);
            ApplyPlayerCommandTarget(target);
            PlayerCommandResultText.Text = $"Ready to manage {target.CharacterName}.";
            PlayerCommandResultText.Foreground = GetThemeBrush("SuccessBrush", Brushes.ForestGreen);
            SetStatus($"Player command target loaded: {target.CharacterName}", true);
        }
        catch (Exception ex)
        {
            PlayerCommandActionsPanel.IsEnabled = false;
            PlayerCommandTargetStatusText.Text = ex.Message;
            PlayerCommandResultText.Text = ex.Message;
            PlayerCommandResultText.Foreground = GetThemeBrush("RoseBrush", Brushes.IndianRed);
            ShowError(ex.Message);
        }
    }

    private async void PlayerGoldCommandButton_Click(object sender, RoutedEventArgs e)
    {
        var add = ReadComboText(PlayerGoldModeBox).Equals("Add", StringComparison.OrdinalIgnoreCase);
        if (!add && !ConfirmPlayerCommand("Remove gold from this character?", "Confirm gold removal"))
            return;

        await ExecutePlayerCommandAsync(
            (service, name) => service.AdjustPlayerGoldAsync(name, ParseLong(PlayerGoldAmountBox.Text, 0), add),
            add ? "Gold addition queued for GameServer." : "Gold removal queued for GameServer.");
    }

    private async void PlayerSilkCommandButton_Click(object sender, RoutedEventArgs e)
    {
        var add = ReadComboText(PlayerSilkModeBox).Equals("Add", StringComparison.OrdinalIgnoreCase);
        if (!add && !ConfirmPlayerCommand("Remove silk from this account?", "Confirm silk removal"))
            return;

        await ExecutePlayerCommandAsync(
            (service, name) => service.AdjustPlayerSilkAsync(
                name,
                ParseInt(PlayerSilkOwnBox.Text, 0),
                ParseInt(PlayerSilkGiftBox.Text, 0),
                ParseInt(PlayerSilkPointBox.Text, 0),
                add),
            add ? "Silk balance added and synchronized." : "Silk balance removed and synchronized.",
            refreshTarget: true);
    }

    private async void PlayerGrantItemButton_Click(object sender, RoutedEventArgs e)
    {
        await ExecutePlayerCommandAsync(
            (service, name) => service.GrantRewardAsync(
                name,
                ParseRequiredInt(PlayerItemIdBox.Text, "Item ID"),
                ParseRequiredInt(PlayerItemQuantityBox.Text, "Quantity"),
                ParseInt(PlayerItemPlusBox.Text, 0),
                "PlayerCommands"),
            "Item added to the character item chest.");
    }

    private async void PlayerTeleportTownButton_Click(object sender, RoutedEventArgs e)
    {
        if (!ConfirmPlayerCommand("Teleport this character to the configured town?", "Confirm town teleport"))
            return;

        await ExecutePlayerCommandAsync(
            (service, name) => service.TeleportPlayerToTownAsync(name),
            "Town teleport command queued.");
    }

    private async void PlayerTeleportPositionButton_Click(object sender, RoutedEventArgs e)
    {
        if (!ConfirmPlayerCommand("Teleport this character to the entered coordinates?", "Confirm position teleport"))
            return;

        await ExecutePlayerCommandAsync(
            (service, name) => service.TeleportPlayerToPositionAsync(
                name,
                ParseRequiredInt(PlayerTeleportWorldBox.Text, "World ID"),
                ParseRequiredInt(PlayerTeleportRegionBox.Text, "Region ID"),
                ParseInt(PlayerTeleportXBox.Text, 0),
                ParseInt(PlayerTeleportYBox.Text, 0),
                ParseInt(PlayerTeleportZBox.Text, 0)),
            "Position teleport command queued.");
    }

    private async void PlayerSelfTeleportButton_Click(object sender, RoutedEventArgs e)
    {
        await ExecutePlayerCommandAsync(
            (service, name) => service.QueuePlayerSelfTeleportAsync(name),
            "Character refresh teleport queued.");
    }

    private async void PlayerNoticeButton_Click(object sender, RoutedEventArgs e)
    {
        await ExecutePlayerCommandAsync(
            (service, name) => service.QueuePlayerNoticeAsync(
                name,
                checked((byte)ParseRequiredInt(PlayerNoticeTypeBox.Text, "Notice type")),
                PlayerNoticeMessageBox.Text),
            "Personal notice queued.");
    }

    private async void PlayerDisconnectButton_Click(object sender, RoutedEventArgs e)
    {
        if (!ConfirmPlayerCommand("Disconnect this character from the server?", "Confirm player disconnect"))
            return;

        await ExecutePlayerCommandAsync(
            (service, name) => service.QueuePlayerDisconnectAsync(name),
            "Disconnect command queued.");
    }

    private async void PlayerSetNameColorButton_Click(object sender, RoutedEventArgs e)
    {
        await ExecutePlayerCommandAsync(
            (service, name) => service.QueuePlayerNameColorAsync(name, PlayerNameColorBox.Text),
            "Name color command queued.");
    }

    private async void PlayerClearNameColorButton_Click(object sender, RoutedEventArgs e)
    {
        await ExecutePlayerCommandAsync(
            (service, name) => service.QueuePlayerNameColorAsync(name, null),
            "Name color removal queued.");
    }

    private async void PlayerActivateTagButton_Click(object sender, RoutedEventArgs e)
    {
        await ExecutePlayerCommandAsync(
            (service, name) => service.QueuePlayerTagAsync(
                name,
                checked((byte)ParseRequiredInt(PlayerTagIdBox.Text, "Tag ID"))),
            "Tag add and activate command queued.");
    }

    private async void PlayerDeactivateTagButton_Click(object sender, RoutedEventArgs e)
    {
        await ExecutePlayerCommandAsync(
            (service, name) => service.QueuePlayerTagAsync(name, null),
            "Tag deactivate command queued.");
    }

    private async void PlayerSetIconButton_Click(object sender, RoutedEventArgs e)
    {
        var leftSide = ReadComboText(PlayerIconSideBox).Equals("Left", StringComparison.OrdinalIgnoreCase);
        await ExecutePlayerCommandAsync(
            (service, name) => service.QueuePlayerIconAsync(
                name,
                leftSide,
                ParseRequiredInt(PlayerIconIdBox.Text, "Icon ID")),
            $"{(leftSide ? "Left" : "Right")} icon command queued.");
    }

    private async void PlayerClearIconButton_Click(object sender, RoutedEventArgs e)
    {
        var leftSide = ReadComboText(PlayerIconSideBox).Equals("Left", StringComparison.OrdinalIgnoreCase);
        await ExecutePlayerCommandAsync(
            (service, name) => service.QueuePlayerIconAsync(name, leftSide, null),
            $"{(leftSide ? "Left" : "Right")} icon removal queued.");
    }

    private void ApplyPlayerCommandTarget(PlayerCommandTarget target)
    {
        PlayerCommandNameBox.Text = target.CharacterName;
        _playerCommandTarget = target;
        PlayerCommandActionsPanel.IsEnabled = true;
        PlayerCommandTargetStatusText.Text = "Exact character match loaded. Commands below target this character only.";
        PlayerCommandTargetNameText.Text = target.CharacterName;
        PlayerCommandTargetIdText.Text = $"#{target.CharId:N0}  /  JID {target.UserJid:N0}";
        PlayerCommandTargetLevelText.Text = target.Level.ToString("N0");
        PlayerCommandTargetRegionText.Text = target.RegionId.ToString("N0");
        PlayerCommandTargetGoldText.Text = target.Gold.ToString("N0");
        PlayerCommandTargetSilkText.Text =
            $"{target.SilkOwn:N0} own  /  {target.SilkGift:N0} gift  /  {target.SilkPoint:N0} point";
    }

    private string RequireLoadedPlayerCommandName()
    {
        var enteredName = PlayerCommandNameBox.Text.Trim();
        if (_playerCommandTarget is null ||
            !string.Equals(_playerCommandTarget.CharacterName, enteredName, StringComparison.OrdinalIgnoreCase))
        {
            throw new InvalidOperationException("Load the exact character name before running a command.");
        }

        return _playerCommandTarget.CharacterName;
    }

    private async Task ExecutePlayerCommandAsync(
        Func<SqlAdminService, string, Task> action,
        string successMessage,
        bool refreshTarget = false)
    {
        try
        {
            var characterName = RequireLoadedPlayerCommandName();
            PlayerCommandActionsPanel.IsEnabled = false;
            PlayerCommandResultText.Text = $"Applying command to {characterName}...";
            PlayerCommandResultText.Foreground = GetThemeBrush("MutedBrush", Brushes.SlateGray);
            await action(CreateSqlService(), characterName);

            if (refreshTarget)
                ApplyPlayerCommandTarget(await CreateSqlService().GetPlayerCommandTargetAsync(characterName));

            PlayerCommandResultText.Text = $"{successMessage}  Target: {characterName}.";
            PlayerCommandResultText.Foreground = GetThemeBrush("SuccessBrush", Brushes.ForestGreen);
            SetStatus(successMessage, true);
        }
        catch (Exception ex)
        {
            PlayerCommandResultText.Text = ex.Message;
            PlayerCommandResultText.Foreground = GetThemeBrush("RoseBrush", Brushes.IndianRed);
            ShowError(ex.Message);
        }
        finally
        {
            PlayerCommandActionsPanel.IsEnabled = _playerCommandTarget is not null;
        }
    }

    private bool ConfirmPlayerCommand(string message, string title)
    {
        var target = _playerCommandTarget?.CharacterName ?? PlayerCommandNameBox.Text.Trim();
        return MessageBox.Show(
                   this,
                   $"{message}\n\nTarget: {target}",
                   title,
                   MessageBoxButton.YesNo,
                   MessageBoxImage.Warning) == MessageBoxResult.Yes;
    }

    private async void RefreshOnlinePlayersButton_Click(object sender, RoutedEventArgs e) => await RefreshOnlinePlayersAsync();

    private void OnlinePlayersAutoRefreshBox_Changed(object sender, RoutedEventArgs e) => UpdateOnlinePlayersTimer();

    private async Task RefreshOnlinePlayersAsync()
    {
        if (_onlinePlayersRefreshBusy)
            return;

        try
        {
            _onlinePlayersRefreshBusy = true;
            var table = await _filterRuntimeService.LoadOnlinePlayersSnapshotAsync();
            BindTable(OnlinePlayersGrid, table);
            var agentConnections = await _filterRuntimeService.GetAgentConnectionCountAsync();

            OnlinePlayersStatusText.Text = _filterRuntimeService.IsAgentRunning
                ? $"Live online: {table.Rows.Count:N0} player(s) | Agent connections: {agentConnections:N0} | Last refresh: {DateTime.Now:HH:mm:ss}"
                : "KMTGuard Agent service is stopped. Start Agent to see live players.";
        }
        catch (Exception ex)
        {
            OnlinePlayersStatusText.Text = ex.Message;
            SetStatus(ex.Message, false);
        }
        finally
        {
            _onlinePlayersRefreshBusy = false;
        }

    }

    private void PlayersGrid_MouseDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
    {
        if (GetSelectedRow(PlayersGrid) is not { } row)
            return;

        var charName = row.Row.Table.Columns.Contains("CharName")
            ? Convert.ToString(row["CharName"])
            : Convert.ToString(row["CharID"]);

        if (string.IsNullOrWhiteSpace(charName))
            return;

        RewardCharBox.Text = charName;
        DisconnectCharBox.Text = charName;
        PlayerCommandNameBox.Text = charName;
        SetStatus($"Selected character: {charName}", true);
    }

    private async void SearchItemsButton_Click(object sender, RoutedEventArgs e) => await SearchItemsAsync();

    private async void LoadChestButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            BindTable(RewardsGrid, await CreateSqlService().LoadItemChestAsync(RewardCharBox.Text));
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void GrantRewardButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().GrantRewardAsync(
                RewardCharBox.Text,
                ParseRequiredInt(RewardItemIdBox.Text, "Item ID"),
                ParseRequiredInt(RewardQuantityBox.Text, "Quantity"),
                ParseInt(RewardPlusBox.Text, 0),
                "AdminDesktop");
            await LoadChestForRewardTargetAsync();
            SetStatus("Reward granted to item chest.", true);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private void RewardsGrid_MouseDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
    {
        if (GetSelectedRow(RewardsGrid) is not { } row)
            return;

        if (row.Row.Table.Columns.Contains("ID") && row.Row.Table.Columns.Contains("CodeName128"))
        {
            RewardItemIdBox.Text = Convert.ToString(row["ID"]) ?? string.Empty;
            LuckyItemIdBox.Text = RewardItemIdBox.Text;
            OfferItemIdBox.Text = RewardItemIdBox.Text;
        }
    }

    private async void LoadLuckySpinButton_Click(object sender, RoutedEventArgs e) => await LoadLuckySpinAsync();

    private async void AddLuckyRewardButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().AddLuckySpinRewardAsync(
                ParseRequiredInt(LuckyItemIdBox.Text, "Item ID"),
                ParseRequiredInt(LuckyAmountBox.Text, "Amount"),
                ParseRequiredInt(LuckyRateBox.Text, "Rate"),
                LuckyActiveBox.IsChecked == true);
            await LoadLuckySpinAsync();
            SetStatus("Lucky Spin reward added.", true);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void LuckyRewardsGrid_MouseDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
    {
        if (GetSelectedRow(LuckyRewardsGrid) is not { } row || !row.Row.Table.Columns.Contains("ID"))
            return;

        var id = Convert.ToInt32(row["ID"]);
        var active = row.Row.Table.Columns.Contains("IsActive") && Convert.ToBoolean(row["IsActive"]);
        if (MessageBox.Show(this, $"Set reward #{id} {(active ? "inactive" : "active")}?", "Lucky Spin", MessageBoxButton.YesNo, MessageBoxImage.Question) != MessageBoxResult.Yes)
            return;

        await CreateSqlService().SetLuckySpinRewardActiveAsync(id, !active);
        await LoadLuckySpinAsync();
    }

    private async void DeleteLuckyRewardButton_Click(object sender, RoutedEventArgs e)
    {
        if (GetSelectedRow(LuckyRewardsGrid) is not { } row || !row.Row.Table.Columns.Contains("ID"))
        {
            ShowError("Select a Lucky Spin reward first.");
            return;
        }

        try
        {
            var id = Convert.ToInt32(row["ID"]);
            var itemId = ReadColumnText(row, "ItemID", "-");
            if (MessageBox.Show(this,
                    $"Delete Lucky Spin reward #{id} (Item {itemId})?\n\nExisting spin history will be preserved.",
                    "Lucky Spin", MessageBoxButton.YesNo, MessageBoxImage.Warning) != MessageBoxResult.Yes)
                return;

            await CreateSqlService().DeleteLuckySpinRewardAsync(id);
            await LoadLuckySpinAsync();
            SetStatus("Lucky Spin reward deleted. Restart KMTGuard to refresh the live reward cache.", true);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void LoadSpecialOffersButton_Click(object sender, RoutedEventArgs e) => await LoadSpecialOffersAsync();

    private async void LoadLuckyLogButton_Click(object sender, RoutedEventArgs e) => await LoadEconomyLogAsync("LuckySpin");

    private async void LoadOfferLogButton_Click(object sender, RoutedEventArgs e) => await LoadEconomyLogAsync("SpecialOffers");

    private async void LoadSilkStallLogButton_Click(object sender, RoutedEventArgs e) => await LoadEconomyLogAsync("SilkStall");

    private async void LoadAutoEventConfigButton_Click(object sender, RoutedEventArgs e) => await LoadAutoEventConfigAsync();

    private async void StartAutoEventButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().QueueAutoEventCommandAsync("START", AutoEventCodeBox.Text);
            SetStatus("Auto event START command queued.", true);
            await LoadAutoEventConfigAsync();
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void StopAutoEventButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().QueueAutoEventCommandAsync("STOP", string.Empty);
            SetStatus("Auto event STOP command queued.", true);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void ReloadAutoEventButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().QueueAutoEventCommandAsync("RELOAD", string.Empty);
            SetStatus("Auto event RELOAD command queued.", true);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void AutoEventsGrid_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_autoEventSelectionLoading)
            return;

        if (GetSelectedRow(AutoEventsGrid) is not { } row || !row.Row.Table.Columns.Contains("EventCode"))
            return;

        _autoEventSelectionLoading = true;
        try
        {
            AutoEventPickerBox.SelectedItem = row;
        }
        finally
        {
            _autoEventSelectionLoading = false;
        }

        await LoadAutoEventEditorAsync(row);
    }

    private async void AutoEventPickerBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_autoEventSelectionLoading || AutoEventPickerBox.SelectedItem is not DataRowView row)
            return;

        _autoEventSelectionLoading = true;
        try
        {
            AutoEventsGrid.SelectedItem = row;
            AutoEventsGrid.ScrollIntoView(row);
        }
        finally
        {
            _autoEventSelectionLoading = false;
        }

        await LoadAutoEventEditorAsync(row);
    }

    private async Task LoadAutoEventEditorAsync(DataRowView row)
    {
        var eventCode = Convert.ToString(row["EventCode"]) ?? string.Empty;
        AutoEventCodeBox.Text = eventCode;
        AutoEventDisplayNameBox.Text = Convert.ToString(row["DisplayName"]) ?? eventCode;
        AutoEventEnabledBox.IsChecked = row.Row.Table.Columns.Contains("Enabled") && Convert.ToBoolean(row["Enabled"]);
        AutoEventStartDelayBox.Text = ReadColumnText(row, "StartDelaySeconds", "60");
        AutoEventHwidLimitBox.Text = ReadColumnText(row, "HwidLimit", "1");
        AutoEventRoundCountBox.Text = Convert.ToString(row["RoundCount"]) ?? "3";
        AutoEventRoundDurationBox.Text = Convert.ToString(row["RoundDurationSeconds"]) ?? "45";
        AutoEventDelayBox.Text = Convert.ToString(row["InterRoundDelaySeconds"]) ?? "10";
        AutoEventMinLevelBox.Text = Convert.ToString(row["MinLevel"]) ?? "1";
        AutoEventUniqueWinnerBox.IsChecked = row.Row.Table.Columns.Contains("UniqueWinnerPerRun") && Convert.ToBoolean(row["UniqueWinnerPerRun"]);
        AutoEventRequireHwidBox.IsChecked = row.Row.Table.Columns.Contains("RequireHwid") && Convert.ToBoolean(row["RequireHwid"]);
        AutoEventCooldownBox.Text = Convert.ToString(row["AnswerCooldownMs"]) ?? "750";
        AutoEventAlchemyPlusBox.Text = Convert.ToString(row["AlchemyTargetPlus"]) ?? "7";
        AutoEventAlchemyPanel.Visibility = eventCode.Equals("Alchemy", StringComparison.OrdinalIgnoreCase)
            ? Visibility.Visible
            : Visibility.Collapsed;
        AutoEventStatusText.Text = $"Editing {AutoEventDisplayNameBox.Text} ({eventCode}).";
        ResetAutoEventScheduleEditor();
        await LoadSelectedAutoEventDetailsAsync();
    }

    private async void SaveAutoEventScheduleButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (!TimeSpan.TryParse(AutoEventScheduleTimeBox.Text.Trim(), out var startTime) ||
                startTime < TimeSpan.Zero ||
                startTime >= TimeSpan.FromDays(1))
                throw new InvalidOperationException("Enter a valid time from 00:00 to 23:59.");

            var daysMask = GetAutoEventScheduleDaysMask();
            if (daysMask == 0)
                throw new InvalidOperationException("Choose at least one day.");

            var isActive = AutoEventScheduleActiveBox.IsChecked == true;
            await CreateSqlService().SaveAutoEventScheduleAsync(
                AutoEventCodeBox.Text,
                ParseInt(AutoEventScheduleIdBox.Text, 0),
                startTime,
                daysMask,
                GetAutoEventScheduleRepeatMinutes(),
                isActive);

            var repeatMinutes = GetAutoEventScheduleRepeatMinutes();
            var repeatText = repeatMinutes == 0 ? "once per selected day" : FormatAutoEventRepeat(repeatMinutes);
            var scheduleState = isActive ? "active" : "paused";
            AutoEventStatusText.Text = $"Schedule saved for {AutoEventCodeBox.Text}: {repeatText}, {scheduleState}.";
            SetStatus($"Auto Event schedule from {startTime:hh\\:mm} saved as {scheduleState}.", true);
            ResetAutoEventScheduleEditor();
            await LoadAutoEventSchedulesAsync();
        }
        catch (Exception ex)
        {
            AutoEventStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void NewAutoEventScheduleButton_Click(object sender, RoutedEventArgs e)
    {
        ResetAutoEventScheduleEditor();
    }

    private void ResetAutoEventScheduleEditor()
    {
        AutoEventScheduleIdBox.Text = string.Empty;
        AutoEventScheduleTimeBox.Text = "20:00";
        AutoEventScheduleRepeatBox.SelectedValue = "0";
        ApplyAutoEventScheduleDaysMask(127);
        AutoEventScheduleActiveBox.IsChecked = true;
        AutoEventSchedulesGrid.SelectedItem = null;
        AutoEventScheduleSaveText.Text = "Add Time";
    }

    private async void DeleteAutoEventScheduleButton_Click(object sender, RoutedEventArgs e)
    {
        var scheduleId = ParseInt(AutoEventScheduleIdBox.Text, 0);
        if (scheduleId <= 0)
        {
            ShowError("Select a schedule row first.");
            return;
        }

        if (MessageBox.Show(
                this,
                $"Delete the selected {AutoEventCodeBox.Text} run time?",
                "Delete schedule",
                MessageBoxButton.YesNo,
                MessageBoxImage.Question) != MessageBoxResult.Yes)
            return;

        try
        {
            await CreateSqlService().DeleteAutoEventScheduleAsync(AutoEventCodeBox.Text, scheduleId);
            NewAutoEventScheduleButton_Click(sender, e);
            await LoadAutoEventSchedulesAsync();
            AutoEventStatusText.Text = "Schedule deleted.";
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private void AutoEventSchedulesGrid_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (GetSelectedRow(AutoEventSchedulesGrid) is not { } row)
            return;

        AutoEventScheduleIdBox.Text = ReadColumnText(row, "ScheduleID", string.Empty);
        var startTime = ReadColumnText(row, "StartTime", "20:00");
        AutoEventScheduleTimeBox.Text = startTime.Length > 5 ? startTime[..5] : startTime;
        SetAutoEventScheduleRepeatMinutes(ParseInt(ReadColumnText(row, "RepeatMinutes", "0"), 0));
        ApplyAutoEventScheduleDaysMask(ParseInt(ReadColumnText(row, "DaysMask", "127"), 127));
        AutoEventScheduleActiveBox.IsChecked = ReadBool(row, "IsActive", true);
        AutoEventScheduleSaveText.Text = "Save Changes";
    }

    private int GetAutoEventScheduleRepeatMinutes()
    {
        return ParseInt(Convert.ToString(AutoEventScheduleRepeatBox.SelectedValue) ?? "0", 0);
    }

    private void SetAutoEventScheduleRepeatMinutes(int repeatMinutes)
    {
        var value = repeatMinutes.ToString();
        AutoEventScheduleRepeatBox.SelectedValue = value;
        if (AutoEventScheduleRepeatBox.SelectedIndex < 0)
            AutoEventScheduleRepeatBox.SelectedValue = "0";
    }

    private static string FormatAutoEventRepeat(int repeatMinutes)
    {
        if (repeatMinutes % 60 == 0)
        {
            var hours = repeatMinutes / 60;
            return hours == 1 ? "every hour" : $"every {hours} hours";
        }

        return $"every {repeatMinutes} minutes";
    }

    private int GetAutoEventScheduleDaysMask()
    {
        var mask = 0;
        if (AutoEventScheduleSunBox.IsChecked == true) mask |= 1 << 0;
        if (AutoEventScheduleMonBox.IsChecked == true) mask |= 1 << 1;
        if (AutoEventScheduleTueBox.IsChecked == true) mask |= 1 << 2;
        if (AutoEventScheduleWedBox.IsChecked == true) mask |= 1 << 3;
        if (AutoEventScheduleThuBox.IsChecked == true) mask |= 1 << 4;
        if (AutoEventScheduleFriBox.IsChecked == true) mask |= 1 << 5;
        if (AutoEventScheduleSatBox.IsChecked == true) mask |= 1 << 6;
        return mask;
    }

    private void ApplyAutoEventScheduleDaysMask(int mask)
    {
        AutoEventScheduleSunBox.IsChecked = (mask & (1 << 0)) != 0;
        AutoEventScheduleMonBox.IsChecked = (mask & (1 << 1)) != 0;
        AutoEventScheduleTueBox.IsChecked = (mask & (1 << 2)) != 0;
        AutoEventScheduleWedBox.IsChecked = (mask & (1 << 3)) != 0;
        AutoEventScheduleThuBox.IsChecked = (mask & (1 << 4)) != 0;
        AutoEventScheduleFriBox.IsChecked = (mask & (1 << 5)) != 0;
        AutoEventScheduleSatBox.IsChecked = (mask & (1 << 6)) != 0;
    }

    private void ScheduleDaysPresetButton_Click(object sender, RoutedEventArgs e)
    {
        var tag = (sender as FrameworkElement)?.Tag as string ?? string.Empty;
        var parts = tag.Split(':', 2);
        if (parts.Length != 2)
            return;

        var mask = parts[1] switch
        {
            "EveryDay" => 127,
            "Weekends" => (1 << 5) | (1 << 6),
            "Clear" => 0,
            _ => 127
        };

        switch (parts[0])
        {
            case "Auto":
                ApplyAutoEventScheduleDaysMask(mask);
                break;
            case "SurvivalParty":
                ApplySurvivalScheduleDaysMask(mask);
                break;
            case "SurvivalSolo":
                ApplySurvivalSoloScheduleDaysMask(mask);
                break;
            case "HideAndSeek":
                ApplyHideAndSeekScheduleDaysMask(mask);
                break;
            case "Competitive":
                ApplyCompetitiveScheduleDaysMask(mask);
                break;
        }
    }

    private async void SaveAutoEventConfigButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().SaveAutoEventConfigAsync(
                AutoEventCodeBox.Text,
                AutoEventDisplayNameBox.Text,
                AutoEventEnabledBox.IsChecked == true,
                ParseInt(AutoEventStartDelayBox.Text, 60),
                ParseRequiredInt(AutoEventRoundCountBox.Text, "Round count"),
                ParseRequiredInt(AutoEventRoundDurationBox.Text, "Round duration"),
                ParseInt(AutoEventDelayBox.Text, 0),
                ParseInt(AutoEventMinLevelBox.Text, 1),
                ParseInt(AutoEventHwidLimitBox.Text, 1),
                AutoEventUniqueWinnerBox.IsChecked == true,
                AutoEventRequireHwidBox.IsChecked == true,
                ParseInt(AutoEventCooldownBox.Text, 750),
                ParseInt(AutoEventAlchemyPlusBox.Text, 7));
            AutoEventStatusText.Text = "Event config saved.";
            await LoadAutoEventConfigAsync();
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void SaveAutoContentButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().SaveAutoEventContentAsync(
                ParseInt(AutoContentIdBox.Text, 0),
                AutoEventCodeBox.Text,
                AutoContentPromptBox.Text,
                AutoContentAnswerBox.Text,
                ParseRequiredInt(AutoContentWeightBox.Text, "Question weight"),
                AutoContentActiveBox.IsChecked == true);
            AutoEventStatusText.Text = "Question saved.";
            await LoadAutoEventContentAsync();
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private void NewAutoContentButton_Click(object sender, RoutedEventArgs e)
    {
        AutoContentIdBox.Text = string.Empty;
        AutoContentPromptBox.Text = string.Empty;
        AutoContentAnswerBox.Text = string.Empty;
        AutoContentWeightBox.Text = "10";
        AutoContentActiveBox.IsChecked = true;
    }

    private void AutoEventContentGrid_MouseDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
    {
        if (GetSelectedRow(AutoEventContentGrid) is not { } row)
            return;

        AutoContentIdBox.Text = Convert.ToString(row["ContentID"]) ?? string.Empty;
        AutoEventCodeBox.Text = Convert.ToString(row["EventCode"]) ?? AutoEventCodeBox.Text;
        AutoContentPromptBox.Text = Convert.ToString(row["Prompt"]) ?? string.Empty;
        AutoContentAnswerBox.Text = Convert.ToString(row["Answer"]) ?? string.Empty;
        AutoContentWeightBox.Text = Convert.ToString(row["Weight"]) ?? "10";
        AutoContentActiveBox.IsChecked = row.Row.Table.Columns.Contains("IsActive") && Convert.ToBoolean(row["IsActive"]);
    }

    private async void SaveAutoRewardButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().SaveAutoEventRewardAsync(
                ParseInt(AutoRewardIdBox.Text, 0),
                AutoEventCodeBox.Text,
                ParseRequiredInt(AutoRewardPlacementBox.Text, "Placement"),
                ReadComboText(AutoRewardTypeBox),
                ParseLong(AutoRewardAmountBox.Text, 0),
                AutoRewardCodeNameBox.Text,
                ParseOptionalInt(AutoRewardItemIdBox.Text),
                ParseRequiredInt(AutoRewardItemCountBox.Text, "Item count"),
                ParseInt(AutoRewardPlusBox.Text, 0),
                AutoRewardActiveBox.IsChecked == true);
            AutoEventStatusText.Text = "Reward saved.";
            await LoadAutoEventRewardsAsync();
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private void NewAutoRewardButton_Click(object sender, RoutedEventArgs e)
    {
        AutoRewardIdBox.Text = string.Empty;
        AutoRewardPlacementBox.Text = "1";
        AutoRewardTypeBox.SelectedIndex = 0;
        AutoRewardAmountBox.Text = "10";
        AutoRewardItemIdBox.Text = "0";
        AutoRewardItemCountBox.Text = "1";
        AutoRewardPlusBox.Text = "0";
        AutoRewardCodeNameBox.Text = string.Empty;
        AutoRewardActiveBox.IsChecked = true;
    }

    private void AutoEventRewardsGrid_MouseDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
    {
        if (GetSelectedRow(AutoEventRewardsGrid) is not { } row)
            return;

        AutoRewardIdBox.Text = Convert.ToString(row["RewardID"]) ?? string.Empty;
        AutoEventCodeBox.Text = Convert.ToString(row["EventCode"]) ?? AutoEventCodeBox.Text;
        AutoRewardPlacementBox.Text = Convert.ToString(row["Placement"]) ?? "1";
        SetComboText(AutoRewardTypeBox, Convert.ToString(row["RewardType"]) ?? "SilkOwn");
        AutoRewardAmountBox.Text = Convert.ToString(row["Amount"]) ?? "0";
        AutoRewardCodeNameBox.Text = Convert.ToString(row["ItemCodeName128"]) ?? string.Empty;
        AutoRewardItemIdBox.Text = Convert.ToString(row["ItemID"]) ?? "0";
        AutoRewardItemCountBox.Text = Convert.ToString(row["ItemCount"]) ?? "1";
        AutoRewardPlusBox.Text = Convert.ToString(row["Plus"]) ?? "0";
        AutoRewardActiveBox.IsChecked = row.Row.Table.Columns.Contains("IsActive") && Convert.ToBoolean(row["IsActive"]);
    }

    private async void LoadSurvivalPartyButton_Click(object sender, RoutedEventArgs e) => await LoadSurvivalPartyAsync();

    private async void SaveSurvivalPartyButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().SaveSurvivalPartyConfigAsync(
                SurvivalDisplayNameBox.Text,
                SurvivalEnabledBox.IsChecked == true,
                ParseRequiredInt(SurvivalEventIdBox.Text, "Event ID"),
                ParseInt(SurvivalStartDelayBox.Text, 60),
                ParseInt(SurvivalRegistrationBox.Text, 60),
                ParseRequiredInt(SurvivalFightBox.Text, "Fight duration"),
                ParseInt(SurvivalMinLevelBox.Text, 1),
                ParseInt(SurvivalHwidLimitBox.Text, 1),
                SurvivalRequireHwidBox.IsChecked == true,
                ParseRequiredInt(SurvivalMaxPlayersBox.Text, "Max players"),
                ParseRequiredInt(SurvivalWorldBox.Text, "WorldID"),
                ParseRequiredInt(SurvivalRegionBox.Text, "RegionID"),
                ParseInt(SurvivalXBox.Text, 0),
                ParseInt(SurvivalYBox.Text, 0),
                ParseInt(SurvivalZBox.Text, 0));

            SurvivalStatusText.Text = "Survival Party config saved.";
            SetStatus("Survival Party config saved.", true);
            await LoadSurvivalPartyAsync();
        }
        catch (Exception ex)
        {
            SurvivalStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void StartSurvivalPartyButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            SurvivalStatusText.Text = "Starting Survival Party...";
            var result = await CreateSqlService().QueueAutoEventCommandAndWaitAsync("START", "SPARTY", TimeSpan.FromSeconds(10));
            SurvivalStatusText.Text = result;
            SetStatus(result, true);
        }
        catch (Exception ex)
        {
            SurvivalStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void StopSurvivalPartyButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            SurvivalStatusText.Text = "Stopping Survival Party...";
            var result = await CreateSqlService().QueueAutoEventCommandAndWaitAsync("STOP", "SPARTY", TimeSpan.FromSeconds(10));
            SurvivalStatusText.Text = result;
            SetStatus(result, true);
        }
        catch (Exception ex)
        {
            SurvivalStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void SaveSurvivalWinnerRewardButton_Click(object sender, RoutedEventArgs e)
    {
        await SaveSurvivalTeamRewardAsync(true);
    }

    private async void SaveSurvivalLoserRewardButton_Click(object sender, RoutedEventArgs e)
    {
        await SaveSurvivalTeamRewardAsync(false);
    }

    private async Task SaveSurvivalTeamRewardAsync(bool winner)
    {
        try
        {
            var placement = winner ? 1 : 2;
            var rewardId = winner ? _survivalWinnerRewardId : _survivalLoserRewardId;
            var rewardType = winner ? ReadComboText(SurvivalWinnerRewardTypeBox) : ReadComboText(SurvivalLoserRewardTypeBox);
            var amountText = winner ? SurvivalWinnerRewardAmountBox.Text : SurvivalLoserRewardAmountBox.Text;
            var codeName = winner ? SurvivalWinnerRewardCodeNameBox.Text : SurvivalLoserRewardCodeNameBox.Text;
            var itemIdText = winner ? SurvivalWinnerRewardItemIdBox.Text : SurvivalLoserRewardItemIdBox.Text;
            var itemCountText = winner ? SurvivalWinnerRewardItemCountBox.Text : SurvivalLoserRewardItemCountBox.Text;
            var plusText = winner ? SurvivalWinnerRewardPlusBox.Text : SurvivalLoserRewardPlusBox.Text;
            var isActive = winner ? SurvivalWinnerRewardActiveBox.IsChecked == true : SurvivalLoserRewardActiveBox.IsChecked == true;

            await CreateSqlService().SaveSurvivalPartyRewardAsync(
                rewardId,
                placement,
                rewardType,
                ParseLong(amountText, 0),
                codeName,
                ParseOptionalInt(itemIdText),
                ParseRequiredInt(itemCountText, "Item count"),
                ParseInt(plusText, 0),
                isActive);

            var label = winner ? "Winner team" : "Loser teams";
            SurvivalStatusText.Text = $"{label} reward saved.";
            SetStatus($"{label} reward saved.", true);
            await LoadSurvivalPartyAsync();
        }
        catch (Exception ex)
        {
            SurvivalStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void SaveSurvivalScheduleButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (!TimeSpan.TryParse(SurvivalScheduleTimeBox.Text.Trim(), out var startTime) || startTime < TimeSpan.Zero || startTime >= TimeSpan.FromDays(1))
                throw new InvalidOperationException("Start time must use HH:mm, for example 20:30.");

            var daysMask = GetSurvivalScheduleDaysMask();
            if (daysMask == 0)
                throw new InvalidOperationException("Select at least one day.");

            await CreateSqlService().SaveSurvivalPartyScheduleAsync(
                ParseInt(SurvivalScheduleIdBox.Text, 0),
                startTime,
                daysMask,
                SurvivalScheduleActiveBox.IsChecked == true);

            SurvivalStatusText.Text = "Automatic start time saved.";
            SetStatus("Survival Party schedule saved.", true);
            await LoadSurvivalPartyAsync();
        }
        catch (Exception ex)
        {
            SurvivalStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void NewSurvivalScheduleButton_Click(object sender, RoutedEventArgs e)
    {
        SurvivalScheduleIdBox.Text = string.Empty;
        SurvivalScheduleTimeBox.Text = "20:00";
        ApplySurvivalScheduleDaysMask(127);
        SurvivalScheduleActiveBox.IsChecked = true;
        SurvivalSchedulesGrid.SelectedItem = null;
    }

    private async void DeleteSurvivalScheduleButton_Click(object sender, RoutedEventArgs e)
    {
        var scheduleId = ParseInt(SurvivalScheduleIdBox.Text, 0);
        if (scheduleId <= 0)
        {
            ShowError("Select a schedule first.");
            return;
        }

        try
        {
            await CreateSqlService().DeleteSurvivalPartyScheduleAsync(scheduleId);
            NewSurvivalScheduleButton_Click(sender, e);
            SurvivalStatusText.Text = "Schedule deleted.";
            await LoadSurvivalPartyAsync();
        }
        catch (Exception ex)
        {
            SurvivalStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void SurvivalSchedulesGrid_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (GetSelectedRow(SurvivalSchedulesGrid) is not { } row)
            return;

        SurvivalScheduleIdBox.Text = ReadColumnText(row, "ScheduleID", string.Empty);
        var startTime = ReadColumnText(row, "StartTime", "20:00");
        SurvivalScheduleTimeBox.Text = startTime.Length > 5 ? startTime[..5] : startTime;
        ApplySurvivalScheduleDaysMask(ParseInt(ReadColumnText(row, "DaysMask", "127"), 127));
        SurvivalScheduleActiveBox.IsChecked = ReadBool(row, "IsActive", true);
    }

    private int GetSurvivalScheduleDaysMask()
    {
        var mask = 0;
        if (SurvivalScheduleSunBox.IsChecked == true) mask |= 1 << 0;
        if (SurvivalScheduleMonBox.IsChecked == true) mask |= 1 << 1;
        if (SurvivalScheduleTueBox.IsChecked == true) mask |= 1 << 2;
        if (SurvivalScheduleWedBox.IsChecked == true) mask |= 1 << 3;
        if (SurvivalScheduleThuBox.IsChecked == true) mask |= 1 << 4;
        if (SurvivalScheduleFriBox.IsChecked == true) mask |= 1 << 5;
        if (SurvivalScheduleSatBox.IsChecked == true) mask |= 1 << 6;
        return mask;
    }

    private void ApplySurvivalScheduleDaysMask(int mask)
    {
        SurvivalScheduleSunBox.IsChecked = (mask & (1 << 0)) != 0;
        SurvivalScheduleMonBox.IsChecked = (mask & (1 << 1)) != 0;
        SurvivalScheduleTueBox.IsChecked = (mask & (1 << 2)) != 0;
        SurvivalScheduleWedBox.IsChecked = (mask & (1 << 3)) != 0;
        SurvivalScheduleThuBox.IsChecked = (mask & (1 << 4)) != 0;
        SurvivalScheduleFriBox.IsChecked = (mask & (1 << 5)) != 0;
        SurvivalScheduleSatBox.IsChecked = (mask & (1 << 6)) != 0;
    }

    private async void LoadSurvivalSoloButton_Click(object sender, RoutedEventArgs e) => await LoadSurvivalSoloAsync();

    private async void SaveSurvivalSoloButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().SaveSurvivalSoloConfigAsync(
                SurvivalSoloDisplayNameBox.Text,
                SurvivalSoloEnabledBox.IsChecked == true,
                ParseRequiredInt(SurvivalSoloEventIdBox.Text, "Event ID"),
                ParseInt(SurvivalSoloStartDelayBox.Text, 60),
                ParseInt(SurvivalSoloRegistrationBox.Text, 60),
                ParseRequiredInt(SurvivalSoloFightBox.Text, "Fight duration"),
                ParseInt(SurvivalSoloMinLevelBox.Text, 1),
                ParseInt(SurvivalSoloHwidLimitBox.Text, 1),
                SurvivalSoloRequireHwidBox.IsChecked == true,
                ParseRequiredInt(SurvivalSoloMaxPlayersBox.Text, "Max players"),
                ParseRequiredInt(SurvivalSoloWorldBox.Text, "WorldID"),
                ParseRequiredInt(SurvivalSoloRegionBox.Text, "RegionID"),
                ParseInt(SurvivalSoloXBox.Text, 0),
                ParseInt(SurvivalSoloYBox.Text, 0),
                ParseInt(SurvivalSoloZBox.Text, 0));

            SurvivalSoloStatusText.Text = "Survival Solo config saved.";
            SetStatus("Survival Solo config saved.", true);
            await LoadSurvivalSoloAsync();
        }
        catch (Exception ex)
        {
            SurvivalSoloStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void StartSurvivalSoloButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            SurvivalSoloStatusText.Text = "Starting Survival Solo...";
            var result = await CreateSqlService().QueueAutoEventCommandAndWaitAsync("START", "SSOLO", TimeSpan.FromSeconds(10));
            SurvivalSoloStatusText.Text = result;
            SetStatus(result, true);
        }
        catch (Exception ex)
        {
            SurvivalSoloStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void StopSurvivalSoloButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            SurvivalSoloStatusText.Text = "Stopping Survival Solo...";
            var result = await CreateSqlService().QueueAutoEventCommandAndWaitAsync("STOP", "SSOLO", TimeSpan.FromSeconds(10));
            SurvivalSoloStatusText.Text = result;
            SetStatus(result, true);
        }
        catch (Exception ex)
        {
            SurvivalSoloStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void SaveSurvivalSoloWinnerRewardButton_Click(object sender, RoutedEventArgs e)
    {
        await SaveSurvivalSoloTeamRewardAsync(true);
    }

    private async void SaveSurvivalSoloLoserRewardButton_Click(object sender, RoutedEventArgs e)
    {
        await SaveSurvivalSoloTeamRewardAsync(false);
    }

    private async Task SaveSurvivalSoloTeamRewardAsync(bool winner)
    {
        try
        {
            var placement = winner ? 1 : 2;
            var rewardId = winner ? _survivalSoloWinnerRewardId : _survivalSoloLoserRewardId;
            var rewardType = winner ? ReadComboText(SurvivalSoloWinnerRewardTypeBox) : ReadComboText(SurvivalSoloLoserRewardTypeBox);
            var amountText = winner ? SurvivalSoloWinnerRewardAmountBox.Text : SurvivalSoloLoserRewardAmountBox.Text;
            var codeName = winner ? SurvivalSoloWinnerRewardCodeNameBox.Text : SurvivalSoloLoserRewardCodeNameBox.Text;
            var itemIdText = winner ? SurvivalSoloWinnerRewardItemIdBox.Text : SurvivalSoloLoserRewardItemIdBox.Text;
            var itemCountText = winner ? SurvivalSoloWinnerRewardItemCountBox.Text : SurvivalSoloLoserRewardItemCountBox.Text;
            var plusText = winner ? SurvivalSoloWinnerRewardPlusBox.Text : SurvivalSoloLoserRewardPlusBox.Text;
            var isActive = winner ? SurvivalSoloWinnerRewardActiveBox.IsChecked == true : SurvivalSoloLoserRewardActiveBox.IsChecked == true;

            await CreateSqlService().SaveSurvivalSoloRewardAsync(
                rewardId,
                placement,
                rewardType,
                ParseLong(amountText, 0),
                codeName,
                ParseOptionalInt(itemIdText),
                ParseRequiredInt(itemCountText, "Item count"),
                ParseInt(plusText, 0),
                isActive);

            var label = winner ? "Winner player" : "Other players";
            SurvivalSoloStatusText.Text = $"{label} reward saved.";
            SetStatus($"{label} reward saved.", true);
            await LoadSurvivalSoloAsync();
        }
        catch (Exception ex)
        {
            SurvivalSoloStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void SaveSurvivalSoloScheduleButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (!TimeSpan.TryParse(SurvivalSoloScheduleTimeBox.Text.Trim(), out var startTime) || startTime < TimeSpan.Zero || startTime >= TimeSpan.FromDays(1))
                throw new InvalidOperationException("Start time must use HH:mm, for example 20:30.");

            var daysMask = GetSurvivalSoloScheduleDaysMask();
            if (daysMask == 0)
                throw new InvalidOperationException("Select at least one day.");

            await CreateSqlService().SaveSurvivalSoloScheduleAsync(
                ParseInt(SurvivalSoloScheduleIdBox.Text, 0),
                startTime,
                daysMask,
                SurvivalSoloScheduleActiveBox.IsChecked == true);

            SurvivalSoloStatusText.Text = "Automatic start time saved.";
            SetStatus("Survival Solo schedule saved.", true);
            await LoadSurvivalSoloAsync();
        }
        catch (Exception ex)
        {
            SurvivalSoloStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void NewSurvivalSoloScheduleButton_Click(object sender, RoutedEventArgs e)
    {
        SurvivalSoloScheduleIdBox.Text = string.Empty;
        SurvivalSoloScheduleTimeBox.Text = "20:00";
        ApplySurvivalSoloScheduleDaysMask(127);
        SurvivalSoloScheduleActiveBox.IsChecked = true;
        SurvivalSoloSchedulesGrid.SelectedItem = null;
    }

    private async void DeleteSurvivalSoloScheduleButton_Click(object sender, RoutedEventArgs e)
    {
        var scheduleId = ParseInt(SurvivalSoloScheduleIdBox.Text, 0);
        if (scheduleId <= 0)
        {
            ShowError("Select a schedule first.");
            return;
        }

        try
        {
            await CreateSqlService().DeleteSurvivalSoloScheduleAsync(scheduleId);
            NewSurvivalSoloScheduleButton_Click(sender, e);
            SurvivalSoloStatusText.Text = "Schedule deleted.";
            await LoadSurvivalSoloAsync();
        }
        catch (Exception ex)
        {
            SurvivalSoloStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void SurvivalSoloSchedulesGrid_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (GetSelectedRow(SurvivalSoloSchedulesGrid) is not { } row)
            return;

        SurvivalSoloScheduleIdBox.Text = ReadColumnText(row, "ScheduleID", string.Empty);
        var startTime = ReadColumnText(row, "StartTime", "20:00");
        SurvivalSoloScheduleTimeBox.Text = startTime.Length > 5 ? startTime[..5] : startTime;
        ApplySurvivalSoloScheduleDaysMask(ParseInt(ReadColumnText(row, "DaysMask", "127"), 127));
        SurvivalSoloScheduleActiveBox.IsChecked = ReadBool(row, "IsActive", true);
    }

    private int GetSurvivalSoloScheduleDaysMask()
    {
        var mask = 0;
        if (SurvivalSoloScheduleSunBox.IsChecked == true) mask |= 1 << 0;
        if (SurvivalSoloScheduleMonBox.IsChecked == true) mask |= 1 << 1;
        if (SurvivalSoloScheduleTueBox.IsChecked == true) mask |= 1 << 2;
        if (SurvivalSoloScheduleWedBox.IsChecked == true) mask |= 1 << 3;
        if (SurvivalSoloScheduleThuBox.IsChecked == true) mask |= 1 << 4;
        if (SurvivalSoloScheduleFriBox.IsChecked == true) mask |= 1 << 5;
        if (SurvivalSoloScheduleSatBox.IsChecked == true) mask |= 1 << 6;
        return mask;
    }

    private void ApplySurvivalSoloScheduleDaysMask(int mask)
    {
        SurvivalSoloScheduleSunBox.IsChecked = (mask & (1 << 0)) != 0;
        SurvivalSoloScheduleMonBox.IsChecked = (mask & (1 << 1)) != 0;
        SurvivalSoloScheduleTueBox.IsChecked = (mask & (1 << 2)) != 0;
        SurvivalSoloScheduleWedBox.IsChecked = (mask & (1 << 3)) != 0;
        SurvivalSoloScheduleThuBox.IsChecked = (mask & (1 << 4)) != 0;
        SurvivalSoloScheduleFriBox.IsChecked = (mask & (1 << 5)) != 0;
        SurvivalSoloScheduleSatBox.IsChecked = (mask & (1 << 6)) != 0;
    }

    private async void LoadHideAndSeekButton_Click(object sender, RoutedEventArgs e) =>
        await LoadHideAndSeekAsync();

    private async Task LoadHideAndSeekAsync()
    {
        try
        {
            var service = CreateSqlService();
            var config = await service.LoadHideAndSeekConfigAsync();
            if (config.DefaultView.Count == 0)
                throw new InvalidOperationException("Hide and Seek configuration is missing.");

            ApplyHideAndSeekConfig(config.DefaultView[0]);
            BindTable(HideAndSeekLocationsGrid, await service.LoadHideAndSeekLocationsAsync());
            BindTable(HideAndSeekRewardsGrid, await service.LoadHideAndSeekRewardsAsync());
            BindTable(HideAndSeekSchedulesGrid, await service.LoadHideAndSeekSchedulesAsync());
            BindTable(HideAndSeekRunsGrid, await service.LoadHideAndSeekRunsAsync());
            HideAndSeekStatusText.Text = "Hide and Seek loaded.";
        }
        catch (Exception ex)
        {
            HideAndSeekStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void ApplyHideAndSeekConfig(DataRowView row)
    {
        HideAndSeekDisplayNameBox.Text = ReadColumnText(row, "DisplayName", "Hide and Seek");
        HideAndSeekEnabledBox.IsChecked = ReadBool(row, "Enabled", true);
        HideAndSeekStartDelayBox.Text = ReadColumnText(row, "StartDelaySeconds", "60");
        HideAndSeekSearchSecondsBox.Text = ReadColumnText(row, "SearchSeconds", "600");
        HideAndSeekReminderBox.Text = ReadColumnText(row, "ReminderIntervalSeconds", "120");
        HideAndSeekMinLevelBox.Text = ReadColumnText(row, "MinLevel", "1");
        HideAndSeekHwidLimitBox.Text = ReadColumnText(row, "HwidLimit", "1");
        HideAndSeekRequireHwidBox.IsChecked = ReadBool(row, "RequireHwid", false);
        HideAndSeekBotAccountBox.Text = ReadColumnText(row, "BotAccountName", string.Empty);
        HideAndSeekBotCharacterBox.Text = ReadColumnText(row, "BotCharacterName", string.Empty);
        HideAndSeekBotPasswordBox.Password = ReadColumnText(row, "BotPassword", string.Empty);
        var botStatus = ReadColumnText(row, "BotStatus", "Not provisioned");
        var botMessage = ReadColumnText(row, "BotMessage", string.Empty);
        HideAndSeekBotStatusText.Text = string.IsNullOrWhiteSpace(botMessage)
            ? botStatus
            : $"{botStatus}: {botMessage}";
        HideAndSeekReturnWorldBox.Text = ReadColumnText(row, "ReturnWorldID", "1");
        HideAndSeekReturnRegionBox.Text = ReadColumnText(row, "ReturnRegionID", "25000");
        HideAndSeekReturnXBox.Text = ReadColumnText(row, "ReturnX", "982");
        HideAndSeekReturnYBox.Text = ReadColumnText(row, "ReturnY", "0");
        HideAndSeekReturnZBox.Text = ReadColumnText(row, "ReturnZ", "140");
    }

    private async void SaveHideAndSeekButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().SaveHideAndSeekConfigAsync(
                HideAndSeekDisplayNameBox.Text,
                HideAndSeekEnabledBox.IsChecked == true,
                ParseInt(HideAndSeekStartDelayBox.Text, 60),
                ParseRequiredInt(HideAndSeekSearchSecondsBox.Text, "Search duration"),
                ParseInt(HideAndSeekReminderBox.Text, 120),
                ParseInt(HideAndSeekMinLevelBox.Text, 1),
                ParseInt(HideAndSeekHwidLimitBox.Text, 1),
                HideAndSeekRequireHwidBox.IsChecked == true,
                HideAndSeekBotAccountBox.Text,
                HideAndSeekBotPasswordBox.Password,
                HideAndSeekBotCharacterBox.Text,
                ParseRequiredInt(HideAndSeekReturnWorldBox.Text, "Return WorldID"),
                ParseRequiredInt(HideAndSeekReturnRegionBox.Text, "Return RegionID"),
                ParseInt(HideAndSeekReturnXBox.Text, 982),
                ParseInt(HideAndSeekReturnYBox.Text, 0),
                ParseInt(HideAndSeekReturnZBox.Text, 140));
            HideAndSeekStatusText.Text = "Hide and Seek setup saved.";
            SetStatus("Hide and Seek setup saved.", true);
            await LoadHideAndSeekAsync();
        }
        catch (Exception ex)
        {
            HideAndSeekStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void StartHideAndSeekButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().SaveHideAndSeekConfigAsync(
                HideAndSeekDisplayNameBox.Text,
                HideAndSeekEnabledBox.IsChecked == true,
                ParseInt(HideAndSeekStartDelayBox.Text, 60),
                ParseRequiredInt(HideAndSeekSearchSecondsBox.Text, "Search duration"),
                ParseInt(HideAndSeekReminderBox.Text, 120),
                ParseInt(HideAndSeekMinLevelBox.Text, 1),
                ParseInt(HideAndSeekHwidLimitBox.Text, 1),
                HideAndSeekRequireHwidBox.IsChecked == true,
                HideAndSeekBotAccountBox.Text,
                HideAndSeekBotPasswordBox.Password,
                HideAndSeekBotCharacterBox.Text,
                ParseRequiredInt(HideAndSeekReturnWorldBox.Text, "Return WorldID"),
                ParseRequiredInt(HideAndSeekReturnRegionBox.Text, "Return RegionID"),
                ParseInt(HideAndSeekReturnXBox.Text, 982),
                ParseInt(HideAndSeekReturnYBox.Text, 0),
                ParseInt(HideAndSeekReturnZBox.Text, 140));
            HideAndSeekStatusText.Text = "Starting Hide and Seek...";
            var result = await CreateSqlService().QueueAutoEventCommandAndWaitAsync(
                "START",
                "HNS",
                TimeSpan.FromSeconds(15));
            HideAndSeekStatusText.Text = result;
            SetStatus(result, true);
        }
        catch (Exception ex)
        {
            HideAndSeekStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void LoginHideAndSeekAccountButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await SaveHideAndSeekSetupForManualLoginAsync();
            await EnsureFilterServicesRunningForClientlessAsync();
            HideAndSeekBotStatusText.Text = "Manual login requested...";
            var result = await _filterRuntimeService.StartSystemClientlessAsync("HNS");
            HideAndSeekBotStatusText.Text = result;
            HideAndSeekStatusText.Text = result;
            SetStatus(result, true);
            await Task.Delay(1000);
            await LoadHideAndSeekAsync();
        }
        catch (Exception ex)
        {
            HideAndSeekBotStatusText.Text = ex.Message;
            HideAndSeekStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void LogoutHideAndSeekAccountButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (!_filterRuntimeService.IsAgentRunning)
            {
                HideAndSeekBotStatusText.Text = "Agent service is stopped; the event account is offline.";
                return;
            }

            HideAndSeekBotStatusText.Text = "Logging out the event account...";
            var result = await _filterRuntimeService.StopSystemClientlessAsync("HNS");
            HideAndSeekBotStatusText.Text = result;
            HideAndSeekStatusText.Text = result;
            SetStatus(result, true);
            await LoadHideAndSeekAsync();
        }
        catch (Exception ex)
        {
            HideAndSeekBotStatusText.Text = ex.Message;
            HideAndSeekStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private Task SaveHideAndSeekSetupForManualLoginAsync()
    {
        return CreateSqlService().SaveHideAndSeekConfigAsync(
            HideAndSeekDisplayNameBox.Text,
            HideAndSeekEnabledBox.IsChecked == true,
            ParseInt(HideAndSeekStartDelayBox.Text, 60),
            ParseRequiredInt(HideAndSeekSearchSecondsBox.Text, "Search duration"),
            ParseInt(HideAndSeekReminderBox.Text, 120),
            ParseInt(HideAndSeekMinLevelBox.Text, 1),
            ParseInt(HideAndSeekHwidLimitBox.Text, 1),
            HideAndSeekRequireHwidBox.IsChecked == true,
            HideAndSeekBotAccountBox.Text,
            HideAndSeekBotPasswordBox.Password,
            HideAndSeekBotCharacterBox.Text,
            ParseRequiredInt(HideAndSeekReturnWorldBox.Text, "Return WorldID"),
            ParseRequiredInt(HideAndSeekReturnRegionBox.Text, "Return RegionID"),
            ParseInt(HideAndSeekReturnXBox.Text, 982),
            ParseInt(HideAndSeekReturnYBox.Text, 0),
            ParseInt(HideAndSeekReturnZBox.Text, 140));
    }

    private async void StopHideAndSeekButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            HideAndSeekStatusText.Text = "Stopping Hide and Seek...";
            var result = await CreateSqlService().QueueAutoEventCommandAndWaitAsync(
                "STOP",
                "HNS",
                TimeSpan.FromSeconds(15));
            HideAndSeekStatusText.Text = result;
            SetStatus(result, true);
            await LoadHideAndSeekAsync();
        }
        catch (Exception ex)
        {
            HideAndSeekStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void SaveHideAndSeekLocationButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().SaveHideAndSeekLocationAsync(
                ParseInt(HideAndSeekLocationIdBox.Text, 0),
                HideAndSeekLocationNameBox.Text,
                ParseRequiredInt(HideAndSeekLocationWorldBox.Text, "WorldID"),
                ParseRequiredInt(HideAndSeekLocationRegionBox.Text, "RegionID"),
                ParseInt(HideAndSeekLocationXBox.Text, 0),
                ParseInt(HideAndSeekLocationYBox.Text, 0),
                ParseInt(HideAndSeekLocationZBox.Text, 0),
                ParseInt(HideAndSeekLocationWeightBox.Text, 1),
                HideAndSeekLocationActiveBox.IsChecked == true);
            HideAndSeekStatusText.Text = "Hiding location saved.";
            NewHideAndSeekLocationButton_Click(sender, e);
            await LoadHideAndSeekAsync();
        }
        catch (Exception ex)
        {
            HideAndSeekStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void NewHideAndSeekLocationButton_Click(object sender, RoutedEventArgs e)
    {
        HideAndSeekLocationIdBox.Text = string.Empty;
        HideAndSeekLocationNameBox.Text = string.Empty;
        HideAndSeekLocationWorldBox.Text = "1";
        HideAndSeekLocationRegionBox.Text = string.Empty;
        HideAndSeekLocationXBox.Text = "0";
        HideAndSeekLocationYBox.Text = "0";
        HideAndSeekLocationZBox.Text = "0";
        HideAndSeekLocationWeightBox.Text = "1";
        HideAndSeekLocationActiveBox.IsChecked = true;
        HideAndSeekLocationsGrid.SelectedItem = null;
    }

    private async void DeleteHideAndSeekLocationButton_Click(object sender, RoutedEventArgs e)
    {
        var locationId = ParseInt(HideAndSeekLocationIdBox.Text, 0);
        if (locationId <= 0)
        {
            ShowError("Select a hiding location first.");
            return;
        }

        try
        {
            await CreateSqlService().DeleteHideAndSeekLocationAsync(locationId);
            NewHideAndSeekLocationButton_Click(sender, e);
            HideAndSeekStatusText.Text = "Hiding location deleted.";
            await LoadHideAndSeekAsync();
        }
        catch (Exception ex)
        {
            HideAndSeekStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void HideAndSeekLocationsGrid_MouseDoubleClick(object sender, MouseButtonEventArgs e)
    {
        if (GetSelectedRow(HideAndSeekLocationsGrid) is not { } row)
            return;

        HideAndSeekLocationIdBox.Text = ReadColumnText(row, "LocationID", string.Empty);
        HideAndSeekLocationNameBox.Text = ReadColumnText(row, "LocationName", string.Empty);
        HideAndSeekLocationWorldBox.Text = ReadColumnText(row, "WorldID", "1");
        HideAndSeekLocationRegionBox.Text = ReadColumnText(row, "RegionID", string.Empty);
        HideAndSeekLocationXBox.Text = ReadColumnText(row, "PosX", "0");
        HideAndSeekLocationYBox.Text = ReadColumnText(row, "PosY", "0");
        HideAndSeekLocationZBox.Text = ReadColumnText(row, "PosZ", "0");
        HideAndSeekLocationWeightBox.Text = ReadColumnText(row, "Weight", "1");
        HideAndSeekLocationActiveBox.IsChecked = ReadBool(row, "IsActive", true);
    }

    private async void SaveHideAndSeekRewardButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().SaveHideAndSeekRewardAsync(
                ParseInt(HideAndSeekRewardIdBox.Text, 0),
                ReadComboText(HideAndSeekRewardTypeBox),
                ParseLong(HideAndSeekRewardAmountBox.Text, 0),
                HideAndSeekRewardCodeBox.Text,
                ParseOptionalInt(HideAndSeekRewardItemIdBox.Text),
                ParseRequiredInt(HideAndSeekRewardCountBox.Text, "Item count"),
                ParseInt(HideAndSeekRewardPlusBox.Text, 0),
                HideAndSeekRewardActiveBox.IsChecked == true);
            HideAndSeekStatusText.Text = "Winner reward saved.";
            NewHideAndSeekRewardButton_Click(sender, e);
            await LoadHideAndSeekAsync();
        }
        catch (Exception ex)
        {
            HideAndSeekStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void NewHideAndSeekRewardButton_Click(object sender, RoutedEventArgs e)
    {
        HideAndSeekRewardIdBox.Text = string.Empty;
        HideAndSeekRewardTypeBox.SelectedIndex = 0;
        HideAndSeekRewardAmountBox.Text = "100";
        HideAndSeekRewardCodeBox.Text = string.Empty;
        HideAndSeekRewardItemIdBox.Text = "0";
        HideAndSeekRewardCountBox.Text = "1";
        HideAndSeekRewardPlusBox.Text = "0";
        HideAndSeekRewardActiveBox.IsChecked = true;
        HideAndSeekRewardsGrid.SelectedItem = null;
    }

    private async void DeleteHideAndSeekRewardButton_Click(object sender, RoutedEventArgs e)
    {
        var rewardId = ParseInt(HideAndSeekRewardIdBox.Text, 0);
        if (rewardId <= 0)
        {
            ShowError("Select a reward first.");
            return;
        }

        try
        {
            await CreateSqlService().DeleteHideAndSeekRewardAsync(rewardId);
            NewHideAndSeekRewardButton_Click(sender, e);
            HideAndSeekStatusText.Text = "Reward deleted.";
            await LoadHideAndSeekAsync();
        }
        catch (Exception ex)
        {
            HideAndSeekStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void HideAndSeekRewardsGrid_MouseDoubleClick(object sender, MouseButtonEventArgs e)
    {
        if (GetSelectedRow(HideAndSeekRewardsGrid) is not { } row)
            return;

        HideAndSeekRewardIdBox.Text = ReadColumnText(row, "RewardID", string.Empty);
        SetComboText(HideAndSeekRewardTypeBox, ReadColumnText(row, "RewardType", "SilkOwn"));
        HideAndSeekRewardAmountBox.Text = ReadColumnText(row, "Amount", "0");
        HideAndSeekRewardCodeBox.Text = ReadColumnText(row, "ItemCodeName128", string.Empty);
        HideAndSeekRewardItemIdBox.Text = ReadColumnText(row, "ItemID", "0");
        HideAndSeekRewardCountBox.Text = ReadColumnText(row, "ItemCount", "1");
        HideAndSeekRewardPlusBox.Text = ReadColumnText(row, "Plus", "0");
        HideAndSeekRewardActiveBox.IsChecked = ReadBool(row, "IsActive", true);
    }

    private async void SaveHideAndSeekScheduleButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (!TimeSpan.TryParse(HideAndSeekScheduleTimeBox.Text.Trim(), out var startTime) ||
                startTime < TimeSpan.Zero ||
                startTime >= TimeSpan.FromDays(1))
                throw new InvalidOperationException("Start time must use HH:mm, for example 20:30.");

            var daysMask = GetHideAndSeekScheduleDaysMask();
            if (daysMask == 0)
                throw new InvalidOperationException("Select at least one day.");

            await CreateSqlService().SaveHideAndSeekScheduleAsync(
                ParseInt(HideAndSeekScheduleIdBox.Text, 0),
                startTime,
                daysMask,
                HideAndSeekScheduleActiveBox.IsChecked == true);
            HideAndSeekStatusText.Text = "Automatic start time saved.";
            NewHideAndSeekScheduleButton_Click(sender, e);
            await LoadHideAndSeekAsync();
        }
        catch (Exception ex)
        {
            HideAndSeekStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void NewHideAndSeekScheduleButton_Click(object sender, RoutedEventArgs e)
    {
        HideAndSeekScheduleIdBox.Text = string.Empty;
        HideAndSeekScheduleTimeBox.Text = "20:00";
        ApplyHideAndSeekScheduleDaysMask(127);
        HideAndSeekScheduleActiveBox.IsChecked = true;
        HideAndSeekSchedulesGrid.SelectedItem = null;
    }

    private async void DeleteHideAndSeekScheduleButton_Click(object sender, RoutedEventArgs e)
    {
        var scheduleId = ParseInt(HideAndSeekScheduleIdBox.Text, 0);
        if (scheduleId <= 0)
        {
            ShowError("Select a schedule first.");
            return;
        }

        try
        {
            await CreateSqlService().DeleteHideAndSeekScheduleAsync(scheduleId);
            NewHideAndSeekScheduleButton_Click(sender, e);
            HideAndSeekStatusText.Text = "Schedule deleted.";
            await LoadHideAndSeekAsync();
        }
        catch (Exception ex)
        {
            HideAndSeekStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void HideAndSeekSchedulesGrid_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (GetSelectedRow(HideAndSeekSchedulesGrid) is not { } row)
            return;

        HideAndSeekScheduleIdBox.Text = ReadColumnText(row, "ScheduleID", string.Empty);
        var startTime = ReadColumnText(row, "StartTime", "20:00");
        HideAndSeekScheduleTimeBox.Text = startTime.Length > 5 ? startTime[..5] : startTime;
        ApplyHideAndSeekScheduleDaysMask(ParseInt(ReadColumnText(row, "DaysMask", "127"), 127));
        HideAndSeekScheduleActiveBox.IsChecked = ReadBool(row, "IsActive", true);
    }

    private int GetHideAndSeekScheduleDaysMask()
    {
        var mask = 0;
        if (HideAndSeekScheduleSunBox.IsChecked == true) mask |= 1 << 0;
        if (HideAndSeekScheduleMonBox.IsChecked == true) mask |= 1 << 1;
        if (HideAndSeekScheduleTueBox.IsChecked == true) mask |= 1 << 2;
        if (HideAndSeekScheduleWedBox.IsChecked == true) mask |= 1 << 3;
        if (HideAndSeekScheduleThuBox.IsChecked == true) mask |= 1 << 4;
        if (HideAndSeekScheduleFriBox.IsChecked == true) mask |= 1 << 5;
        if (HideAndSeekScheduleSatBox.IsChecked == true) mask |= 1 << 6;
        return mask;
    }

    private void ApplyHideAndSeekScheduleDaysMask(int mask)
    {
        HideAndSeekScheduleSunBox.IsChecked = (mask & (1 << 0)) != 0;
        HideAndSeekScheduleMonBox.IsChecked = (mask & (1 << 1)) != 0;
        HideAndSeekScheduleTueBox.IsChecked = (mask & (1 << 2)) != 0;
        HideAndSeekScheduleWedBox.IsChecked = (mask & (1 << 3)) != 0;
        HideAndSeekScheduleThuBox.IsChecked = (mask & (1 << 4)) != 0;
        HideAndSeekScheduleFriBox.IsChecked = (mask & (1 << 5)) != 0;
        HideAndSeekScheduleSatBox.IsChecked = (mask & (1 << 6)) != 0;
    }

    private async void LoadCompetitiveEventButton_Click(object sender, RoutedEventArgs e) => await LoadCompetitiveEventAsync();

    private async Task LoadCompetitiveEventAsync()
    {
        try
        {
            var service = CreateSqlService();
            var config = await service.LoadCompetitiveEventConfigAsync(_competitiveEventCode);
            if (config.DefaultView.Count == 0)
                throw new InvalidOperationException("Competitive event configuration is missing.");
            ApplyCompetitiveEventConfig(config.DefaultView[0]);
            BindTable(CompetitiveRewardsGrid, await service.LoadCompetitiveEventRewardsAsync(_competitiveEventCode));
            BindTable(CompetitiveSchedulesGrid, await service.LoadCompetitiveEventSchedulesAsync(_competitiveEventCode));
            CompetitiveStatusText.Text = $"{CompetitivePageTitleText.Text} loaded.";
        }
        catch (Exception ex)
        {
            CompetitiveStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void ApplyCompetitiveEventConfig(DataRowView row)
    {
        CompetitivePageTitleText.Text = ReadColumnText(row, "DisplayName", _competitiveEventCode);
        CompetitiveDisplayNameBox.Text = CompetitivePageTitleText.Text;
        CompetitiveEnabledBox.IsChecked = ReadBool(row, "Enabled", true);
        CompetitiveRequireHwidBox.IsChecked = ReadBool(row, "RequireHwid", true);
        CompetitiveRequireNoPartyBox.IsChecked = ReadBool(row, "RequireNoParty", true);
        CompetitiveEventIdBox.Text = ReadColumnText(row, "EventID", "14");
        CompetitiveStartDelayBox.Text = ReadColumnText(row, "StartDelaySeconds", "60");
        CompetitiveRegistrationBox.Text = ReadColumnText(row, "RegistrationSeconds", "600");
        CompetitivePrepareBox.Text = ReadColumnText(row, "PrepareSeconds", "20");
        CompetitiveFightBox.Text = ReadColumnText(row, "FightSeconds", "600");
        CompetitiveMinPlayersBox.Text = ReadColumnText(row, "MinPlayers", "2");
        CompetitiveMaxPlayersBox.Text = ReadColumnText(row, "MaxPlayers", "100");
        CompetitiveMinLevelBox.Text = ReadColumnText(row, "MinLevel", "1");
        CompetitiveHwidLimitBox.Text = ReadColumnText(row, "HwidLimit", "1");
        CompetitiveWorldBox.Text = ReadColumnText(row, "ArenaWorldID", "1");
        CompetitiveRegionBox.Text = ReadColumnText(row, "ArenaRegionID", "1");
        CompetitiveXBox.Text = ReadColumnText(row, "ArenaX", "0");
        CompetitiveYBox.Text = ReadColumnText(row, "ArenaY", "0");
        CompetitiveZBox.Text = ReadColumnText(row, "ArenaZ", "0");
        CompetitiveMobSpawnDelayBox.Text = ReadColumnText(row, "MobSpawnDelaySeconds", "60");
        CompetitiveTeam1XBox.Text = ReadColumnText(row, "Team1X", "0");
        CompetitiveTeam1YBox.Text = ReadColumnText(row, "Team1Y", "0");
        CompetitiveTeam1ZBox.Text = ReadColumnText(row, "Team1Z", "0");
        CompetitiveTeam2XBox.Text = ReadColumnText(row, "Team2X", "0");
        CompetitiveTeam2YBox.Text = ReadColumnText(row, "Team2Y", "0");
        CompetitiveTeam2ZBox.Text = ReadColumnText(row, "Team2Z", "0");
        CompetitiveMadnessMobIdBox.Text = ReadColumnText(row, "MadnessMobID", "0");
        CompetitiveMadnessMobCountBox.Text = ReadColumnText(row, "MadnessMobCount", "3");
        CompetitiveMadnessMobXBox.Text = ReadColumnText(row, "MadnessMobX", "0");
        CompetitiveMadnessMobYBox.Text = ReadColumnText(row, "MadnessMobY", "0");
        CompetitiveMadnessMobZBox.Text = ReadColumnText(row, "MadnessMobZ", "0");
        CompetitivePairKillLimitBox.Text = ReadColumnText(row, "PairKillLimit", "3");
        CompetitiveTotalKillLimitBox.Text = ReadColumnText(row, "TotalKillLimit", "200");
        CompetitiveKillRewardCodeBox.Text = ReadColumnText(row, "KillRewardItemCode", string.Empty);
        CompetitiveKillRewardCountBox.Text = ReadColumnText(row, "KillRewardItemCount", "1");
        CompetitiveKillRewardLimitBox.Text = ReadColumnText(row, "KillRewardLimit", "10");
        CompetitiveTower1MobIdBox.Text = ReadColumnText(row, "Team1TowerMobID", "0");
        CompetitiveTower1XBox.Text = ReadColumnText(row, "Team1TowerX", "0");
        CompetitiveTower1YBox.Text = ReadColumnText(row, "Team1TowerY", "0");
        CompetitiveTower1ZBox.Text = ReadColumnText(row, "Team1TowerZ", "0");
        CompetitiveTower2MobIdBox.Text = ReadColumnText(row, "Team2TowerMobID", "0");
        CompetitiveTower2XBox.Text = ReadColumnText(row, "Team2TowerX", "0");
        CompetitiveTower2YBox.Text = ReadColumnText(row, "Team2TowerY", "0");
        CompetitiveTower2ZBox.Text = ReadColumnText(row, "Team2TowerZ", "0");
        CompetitiveMadnessPanel.Visibility = _competitiveEventCode == "MADNESS" ? Visibility.Visible : Visibility.Collapsed;
        CompetitiveTowerPanel.Visibility = _competitiveEventCode == "DTT" ? Visibility.Visible : Visibility.Collapsed;
        CompetitiveSpawnDelayPanel.Visibility = _competitiveEventCode == "LMS" ? Visibility.Collapsed : Visibility.Visible;
    }

    private async void SaveCompetitiveEventButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var config = new CompetitiveEventAdminConfig(
                _competitiveEventCode, CompetitiveDisplayNameBox.Text, CompetitiveEnabledBox.IsChecked == true,
                ParseRequiredInt(CompetitiveEventIdBox.Text, "Event ID"), ParseInt(CompetitiveStartDelayBox.Text, 60),
                ParseRequiredInt(CompetitiveRegistrationBox.Text, "Registration duration"), ParseInt(CompetitivePrepareBox.Text, 20),
                ParseRequiredInt(CompetitiveFightBox.Text, "Fight duration"), ParseRequiredInt(CompetitiveMinPlayersBox.Text, "Min players"),
                ParseRequiredInt(CompetitiveMaxPlayersBox.Text, "Max players"), ParseInt(CompetitiveMinLevelBox.Text, 1),
                ParseInt(CompetitiveHwidLimitBox.Text, 1), CompetitiveRequireHwidBox.IsChecked == true,
                CompetitiveRequireNoPartyBox.IsChecked == true, ParseRequiredInt(CompetitiveWorldBox.Text, "WorldID"),
                ParseRequiredInt(CompetitiveRegionBox.Text, "RegionID"), ParseInt(CompetitiveXBox.Text, 0), ParseInt(CompetitiveYBox.Text, 0), ParseInt(CompetitiveZBox.Text, 0),
                ParseInt(CompetitiveTeam1XBox.Text, 0), ParseInt(CompetitiveTeam1YBox.Text, 0), ParseInt(CompetitiveTeam1ZBox.Text, 0),
                ParseInt(CompetitiveTeam2XBox.Text, 0), ParseInt(CompetitiveTeam2YBox.Text, 0), ParseInt(CompetitiveTeam2ZBox.Text, 0),
                ParseInt(CompetitiveMadnessMobIdBox.Text, 0), ParseInt(CompetitiveMadnessMobCountBox.Text, 0),
                ParseInt(CompetitiveMadnessMobXBox.Text, 0), ParseInt(CompetitiveMadnessMobYBox.Text, 0), ParseInt(CompetitiveMadnessMobZBox.Text, 0),
                ParseInt(CompetitiveMobSpawnDelayBox.Text, 60), ParseInt(CompetitivePairKillLimitBox.Text, 3), ParseInt(CompetitiveTotalKillLimitBox.Text, 200),
                CompetitiveKillRewardCodeBox.Text, ParseInt(CompetitiveKillRewardCountBox.Text, 1), ParseInt(CompetitiveKillRewardLimitBox.Text, 10),
                ParseInt(CompetitiveTower1MobIdBox.Text, 0), ParseInt(CompetitiveTower1XBox.Text, 0), ParseInt(CompetitiveTower1YBox.Text, 0), ParseInt(CompetitiveTower1ZBox.Text, 0),
                ParseInt(CompetitiveTower2MobIdBox.Text, 0), ParseInt(CompetitiveTower2XBox.Text, 0), ParseInt(CompetitiveTower2YBox.Text, 0), ParseInt(CompetitiveTower2ZBox.Text, 0));
            await CreateSqlService().SaveCompetitiveEventConfigAsync(config);
            var result = $"{config.DisplayName} setup saved.";
            CompetitiveStatusText.Text = result;
            SetStatus(result, true);
            await LoadCompetitiveEventAsync();
        }
        catch (Exception ex) { CompetitiveStatusText.Text = ex.Message; ShowError(ex.Message); }
    }

    private async void StartCompetitiveEventButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            CompetitiveStatusText.Text = $"Starting {CompetitivePageTitleText.Text}...";
            var result = await CreateSqlService().QueueAutoEventCommandAndWaitAsync("START", _competitiveEventCode, TimeSpan.FromSeconds(10));
            CompetitiveStatusText.Text = result; SetStatus(result, true);
        }
        catch (Exception ex) { CompetitiveStatusText.Text = ex.Message; ShowError(ex.Message); }
    }

    private async void StopCompetitiveEventButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            CompetitiveStatusText.Text = $"Stopping {CompetitivePageTitleText.Text}...";
            var result = await CreateSqlService().QueueAutoEventCommandAndWaitAsync("STOP", _competitiveEventCode, TimeSpan.FromSeconds(10));
            CompetitiveStatusText.Text = result; SetStatus(result, true);
        }
        catch (Exception ex) { CompetitiveStatusText.Text = ex.Message; ShowError(ex.Message); }
    }

    private async void SaveCompetitiveRewardButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().SaveCompetitiveEventRewardAsync(_competitiveEventCode,
                ParseInt(CompetitiveRewardIdBox.Text, 0), ParseRequiredInt(CompetitiveRewardPlacementBox.Text, "Placement"),
                ReadComboText(CompetitiveRewardTypeBox), ParseLong(CompetitiveRewardAmountBox.Text, 0), CompetitiveRewardCodeBox.Text,
                ParseOptionalInt(CompetitiveRewardItemIdBox.Text), ParseRequiredInt(CompetitiveRewardItemCountBox.Text, "Item count"),
                ParseInt(CompetitiveRewardPlusBox.Text, 0), CompetitiveRewardActiveBox.IsChecked == true);
            CompetitiveStatusText.Text = "Reward saved."; await LoadCompetitiveEventAsync();
        }
        catch (Exception ex) { CompetitiveStatusText.Text = ex.Message; ShowError(ex.Message); }
    }

    private void NewCompetitiveRewardButton_Click(object sender, RoutedEventArgs e)
    {
        CompetitiveRewardIdBox.Text = string.Empty; CompetitiveRewardPlacementBox.Text = "1"; CompetitiveRewardTypeBox.SelectedIndex = 0;
        CompetitiveRewardAmountBox.Text = "0"; CompetitiveRewardCodeBox.Text = string.Empty; CompetitiveRewardItemIdBox.Text = "0";
        CompetitiveRewardItemCountBox.Text = "1"; CompetitiveRewardPlusBox.Text = "0"; CompetitiveRewardActiveBox.IsChecked = true;
        CompetitiveRewardsGrid.SelectedItem = null;
    }

    private async void DeleteCompetitiveRewardButton_Click(object sender, RoutedEventArgs e)
    {
        var id = ParseInt(CompetitiveRewardIdBox.Text, 0);
        if (id <= 0) { ShowError("Select a reward first."); return; }
        try { await CreateSqlService().DeleteCompetitiveEventRewardAsync(_competitiveEventCode, id); NewCompetitiveRewardButton_Click(sender, e); await LoadCompetitiveEventAsync(); }
        catch (Exception ex) { CompetitiveStatusText.Text = ex.Message; ShowError(ex.Message); }
    }

    private void CompetitiveRewardsGrid_MouseDoubleClick(object sender, MouseButtonEventArgs e)
    {
        if (GetSelectedRow(CompetitiveRewardsGrid) is not { } row) return;
        CompetitiveRewardIdBox.Text = ReadColumnText(row, "RewardID", string.Empty);
        CompetitiveRewardPlacementBox.Text = ReadColumnText(row, "Placement", "1");
        SetComboText(CompetitiveRewardTypeBox, ReadColumnText(row, "RewardType", "SilkOwn"));
        CompetitiveRewardAmountBox.Text = ReadColumnText(row, "Amount", "0");
        CompetitiveRewardCodeBox.Text = ReadColumnText(row, "ItemCodeName128", string.Empty);
        CompetitiveRewardItemIdBox.Text = ReadColumnText(row, "ItemID", "0");
        CompetitiveRewardItemCountBox.Text = ReadColumnText(row, "ItemCount", "1");
        CompetitiveRewardPlusBox.Text = ReadColumnText(row, "Plus", "0");
        CompetitiveRewardActiveBox.IsChecked = ReadBool(row, "IsActive", true);
    }

    private async void SaveCompetitiveScheduleButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (!TimeSpan.TryParse(CompetitiveScheduleTimeBox.Text.Trim(), out var time) || time < TimeSpan.Zero || time >= TimeSpan.FromDays(1))
                throw new InvalidOperationException("Start time must use HH:mm, for example 20:30.");
            var mask = GetCompetitiveScheduleDaysMask();
            if (mask == 0) throw new InvalidOperationException("Select at least one day.");
            await CreateSqlService().SaveCompetitiveEventScheduleAsync(_competitiveEventCode, ParseInt(CompetitiveScheduleIdBox.Text, 0), time, mask, CompetitiveScheduleActiveBox.IsChecked == true);
            CompetitiveStatusText.Text = "Automatic start time saved."; await LoadCompetitiveEventAsync();
        }
        catch (Exception ex) { CompetitiveStatusText.Text = ex.Message; ShowError(ex.Message); }
    }

    private void NewCompetitiveScheduleButton_Click(object sender, RoutedEventArgs e)
    {
        CompetitiveScheduleIdBox.Text = string.Empty; CompetitiveScheduleTimeBox.Text = "20:00"; ApplyCompetitiveScheduleDaysMask(127);
        CompetitiveScheduleActiveBox.IsChecked = true; CompetitiveSchedulesGrid.SelectedItem = null;
    }

    private async void DeleteCompetitiveScheduleButton_Click(object sender, RoutedEventArgs e)
    {
        var id = ParseInt(CompetitiveScheduleIdBox.Text, 0);
        if (id <= 0) { ShowError("Select a schedule first."); return; }
        try { await CreateSqlService().DeleteCompetitiveEventScheduleAsync(_competitiveEventCode, id); NewCompetitiveScheduleButton_Click(sender, e); await LoadCompetitiveEventAsync(); }
        catch (Exception ex) { CompetitiveStatusText.Text = ex.Message; ShowError(ex.Message); }
    }

    private void CompetitiveSchedulesGrid_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (GetSelectedRow(CompetitiveSchedulesGrid) is not { } row) return;
        CompetitiveScheduleIdBox.Text = ReadColumnText(row, "ScheduleID", string.Empty);
        var time = ReadColumnText(row, "StartTime", "20:00"); CompetitiveScheduleTimeBox.Text = time.Length > 5 ? time[..5] : time;
        ApplyCompetitiveScheduleDaysMask(ParseInt(ReadColumnText(row, "DaysMask", "127"), 127));
        CompetitiveScheduleActiveBox.IsChecked = ReadBool(row, "IsActive", true);
    }

    private int GetCompetitiveScheduleDaysMask()
    {
        var mask = 0;
        if (CompetitiveScheduleSunBox.IsChecked == true) mask |= 1 << 0; if (CompetitiveScheduleMonBox.IsChecked == true) mask |= 1 << 1;
        if (CompetitiveScheduleTueBox.IsChecked == true) mask |= 1 << 2; if (CompetitiveScheduleWedBox.IsChecked == true) mask |= 1 << 3;
        if (CompetitiveScheduleThuBox.IsChecked == true) mask |= 1 << 4; if (CompetitiveScheduleFriBox.IsChecked == true) mask |= 1 << 5;
        if (CompetitiveScheduleSatBox.IsChecked == true) mask |= 1 << 6; return mask;
    }

    private void ApplyCompetitiveScheduleDaysMask(int mask)
    {
        CompetitiveScheduleSunBox.IsChecked = (mask & 1) != 0; CompetitiveScheduleMonBox.IsChecked = (mask & 2) != 0;
        CompetitiveScheduleTueBox.IsChecked = (mask & 4) != 0; CompetitiveScheduleWedBox.IsChecked = (mask & 8) != 0;
        CompetitiveScheduleThuBox.IsChecked = (mask & 16) != 0; CompetitiveScheduleFriBox.IsChecked = (mask & 32) != 0;
        CompetitiveScheduleSatBox.IsChecked = (mask & 64) != 0;
    }

    private async void LoadUniqueRewardsButton_Click(object sender, RoutedEventArgs e) => await LoadAutoEventUniqueRewardsAsync();

    private async void SaveUniqueRewardButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().SaveAutoEventUniqueRewardAsync(
                ParseInt(UniqueRewardIdBox.Text, 0),
                ParseRequiredInt(UniqueMobIdBox.Text, "Mob ID"),
                UniqueRewardEnabledBox.IsChecked == true,
                ReadComboText(UniqueRewardTypeBox),
                ParseLong(UniqueRewardAmountBox.Text, 0),
                UniqueRewardCodeNameBox.Text,
                ParseOptionalInt(UniqueRewardItemIdBox.Text),
                ParseRequiredInt(UniqueRewardItemCountBox.Text, "Item count"),
                ParseInt(UniqueRewardPlusBox.Text, 0));
            AutoEventStatusText.Text = "Unique reward saved.";
            await LoadAutoEventUniqueRewardsAsync();
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private void NewUniqueRewardButton_Click(object sender, RoutedEventArgs e)
    {
        UniqueRewardIdBox.Text = string.Empty;
        UniqueMobIdBox.Text = string.Empty;
        UniqueRewardTypeBox.SelectedIndex = 0;
        UniqueRewardAmountBox.Text = "10";
        UniqueRewardItemIdBox.Text = "0";
        UniqueRewardItemCountBox.Text = "1";
        UniqueRewardPlusBox.Text = "0";
        UniqueRewardCodeNameBox.Text = string.Empty;
        UniqueRewardEnabledBox.IsChecked = true;
    }

    private void AutoEventUniqueRewardsGrid_MouseDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
    {
        if (GetSelectedRow(AutoEventUniqueRewardsGrid) is not { } row)
            return;

        UniqueRewardIdBox.Text = Convert.ToString(row["RewardID"]) ?? string.Empty;
        UniqueMobIdBox.Text = Convert.ToString(row["MobID"]) ?? string.Empty;
        UniqueRewardEnabledBox.IsChecked = row.Row.Table.Columns.Contains("Enabled") && Convert.ToBoolean(row["Enabled"]);
        SetComboText(UniqueRewardTypeBox, Convert.ToString(row["RewardType"]) ?? "SilkOwn");
        UniqueRewardAmountBox.Text = Convert.ToString(row["Amount"]) ?? "0";
        UniqueRewardCodeNameBox.Text = Convert.ToString(row["ItemCodeName128"]) ?? string.Empty;
        UniqueRewardItemIdBox.Text = Convert.ToString(row["ItemID"]) ?? "0";
        UniqueRewardItemCountBox.Text = Convert.ToString(row["ItemCount"]) ?? "1";
        UniqueRewardPlusBox.Text = Convert.ToString(row["Plus"]) ?? "0";
    }

    private async void LoadPvpConfigButton_Click(object sender, RoutedEventArgs e)
    {
        _pvpGridMode = "Config";
        BindTable(PvpGrid, await CreateSqlService().LoadPvpConfigAsync());
    }

    private async void LoadPvpArenasButton_Click(object sender, RoutedEventArgs e) => await LoadPvpArenasAsync();

    private async void LoadPvpMatchesButton_Click(object sender, RoutedEventArgs e)
    {
        _pvpGridMode = "Matches";
        BindTable(PvpGrid, await CreateSqlService().LoadPvpMatchesAsync(PvpSearchBox.Text));
    }

    private async void AddPvpArenaButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().AddPvpArenaAsync(
                PvpArenaNameBox.Text,
                ParseRequiredInt(PvpWorldBox.Text, "GameWorldID"),
                ParseRequiredInt(PvpRegionBox.Text, "RegionID"),
                ParseInt(PvpXBox.Text, 0),
                ParseInt(PvpYBox.Text, 0),
                ParseInt(PvpZBox.Text, 0),
                ParseInt(PvpSortBox.Text, 0),
                PvpArenaEnabledBox.IsChecked == true);
            await LoadPvpArenasAsync();
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void PvpGrid_MouseDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
    {
        if (_pvpGridMode != "Arenas" || GetSelectedRow(PvpGrid) is not { } row || !row.Row.Table.Columns.Contains("ArenaID"))
            return;

        var arenaId = Convert.ToInt32(row["ArenaID"]);
        var enabled = row.Row.Table.Columns.Contains("Enabled") && Convert.ToBoolean(row["Enabled"]);
        await CreateSqlService().SetPvpArenaEnabledAsync(arenaId, !enabled);
        await LoadPvpArenasAsync();
    }

    private async void LoadRegionControlButton_Click(object sender, RoutedEventArgs e) => await LoadRegionControlAsync();

    private async void SearchRegionControlButton_Click(object sender, RoutedEventArgs e) => await LoadRegionControlAsync();

    private async void SaveRegionControlButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().SaveRegionControlAsync(
                ParseInt(RegionWorldIdBox.Text, 0),
                ParseRequiredInt(RegionIdBox.Text, "Region ID"),
                RegionRuleNameBox.Text,
                RegionEnabledBox.IsChecked == true,
                ReadTaggedValue(RegionBuildModeBox, 0),
                ReadTaggedValue(RegionJobModeBox, 0),
                ReadTaggedValue(RegionRaceModeBox, 0),
                ReadTaggedValue(RegionPartyModeBox, 0),
                ParseInt(RegionMinLevelBox.Text, 0),
                ParseInt(RegionMaxLevelBox.Text, 0),
                RegionTeleportBox.IsChecked == true,
                RegionReverseBox.IsChecked == true,
                RegionTraceBox.IsChecked == true,
                RegionMoveBox.IsChecked == true,
                RegionChatBox.IsChecked == true,
                RegionGlobalBox.IsChecked == true,
                RegionPartyBox.IsChecked == true,
                RegionExchangeBox.IsChecked == true,
                RegionStallBox.IsChecked == true,
                RegionPvpBox.IsChecked == true,
                RegionAlchemyBox.IsChecked == true,
                RegionSpecialItemsBox.IsChecked == true,
                RegionZerkBox.IsChecked == true,
                ParseInt(RegionAutoPvpBox.Text, 0),
                ParseInt(RegionInactivitySecondsBox.Text, 0),
                ReadTaggedValue(RegionEventSuitModeBox, 0));
            RegionControlStatusText.Text = "Region rule saved. The filter cache refreshes automatically within a few seconds.";
            SetStatus("Region control rule saved.", true);
            await LoadRegionControlAsync();
        }
        catch (Exception ex)
        {
            RegionControlStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void DeleteRegionControlButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var worldId = ParseInt(RegionWorldIdBox.Text, 0);
            var regionId = ParseRequiredInt(RegionIdBox.Text, "Region ID");
            if (MessageBox.Show(this, $"Delete region rule World {worldId}, Region {regionId}?", "Region Control", MessageBoxButton.YesNo, MessageBoxImage.Warning) != MessageBoxResult.Yes)
                return;

            await CreateSqlService().DeleteRegionControlAsync(worldId, regionId);
            RegionControlStatusText.Text = "Region rule deleted.";
            NewRegionControlButton_Click(sender, e);
            await LoadRegionControlAsync();
        }
        catch (Exception ex)
        {
            RegionControlStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void NewRegionControlButton_Click(object sender, RoutedEventArgs e)
    {
        RegionWorldIdBox.Text = "0";
        RegionIdBox.Text = string.Empty;
        RegionRuleNameBox.Text = string.Empty;
        RegionPresetAllowAll_Click(sender, e);
        RegionControlStatusText.Text = "Ready for a new region rule.";
    }

    private void RegionPresetAllowAll_Click(object sender, RoutedEventArgs e)
    {
        SetRegionChecks(true);
        RegionEnabledBox.IsChecked = true;
        SetTaggedValue(RegionBuildModeBox, 0);
        SetTaggedValue(RegionJobModeBox, 0);
        SetTaggedValue(RegionRaceModeBox, 0);
        SetTaggedValue(RegionPartyModeBox, 0);
        RegionMinLevelBox.Text = "0";
        RegionMaxLevelBox.Text = "0";
        RegionAutoPvpBox.Text = "0";
        RegionInactivitySecondsBox.Text = "0";
        SetTaggedValue(RegionEventSuitModeBox, 0);
    }

    private void RegionPresetLockdown_Click(object sender, RoutedEventArgs e)
    {
        SetRegionChecks(false);
        RegionEnabledBox.IsChecked = true;
        RegionMoveBox.IsChecked = true;
        RegionChatBox.IsChecked = false;
        RegionPvpBox.IsChecked = false;
        RegionAutoPvpBox.Text = "0";
        RegionInactivitySecondsBox.Text = "10";
        SetTaggedValue(RegionEventSuitModeBox, 0);
    }

    private void RegionPresetPvpArena_Click(object sender, RoutedEventArgs e)
    {
        SetRegionChecks(true);
        RegionEnabledBox.IsChecked = true;
        RegionExchangeBox.IsChecked = false;
        RegionStallBox.IsChecked = false;
        RegionReverseBox.IsChecked = false;
        RegionTraceBox.IsChecked = false;
        SetTaggedValue(RegionJobModeBox, 1);
        RegionPvpBox.IsChecked = true;
        RegionAutoPvpBox.Text = "5";
        RegionInactivitySecondsBox.Text = "0";
        SetTaggedValue(RegionEventSuitModeBox, 1);
    }

    private void RegionControlGrid_MouseDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
    {
        if (GetSelectedRow(RegionControlGrid) is not { } row)
            return;

        RegionWorldIdBox.Text = Convert.ToString(row["WorldID"]) ?? "0";
        RegionIdBox.Text = Convert.ToString(row["RegionID"]) ?? string.Empty;
        RegionRuleNameBox.Text = Convert.ToString(row["RuleName"]) ?? string.Empty;
        RegionEnabledBox.IsChecked = ReadBool(row, "Enabled", true);
        SetTaggedValue(RegionBuildModeBox, ReadInt(row, "BuildMode", 0));
        SetTaggedValue(RegionJobModeBox, ReadInt(row, "JobMode", 0));
        SetTaggedValue(RegionRaceModeBox, ReadInt(row, "RaceMode", 0));
        SetTaggedValue(RegionPartyModeBox, ReadInt(row, "PartyMode", 0));
        RegionMinLevelBox.Text = Convert.ToString(row["MinLevel"]) ?? "0";
        RegionMaxLevelBox.Text = Convert.ToString(row["MaxLevel"]) ?? "0";
        RegionTeleportBox.IsChecked = ReadBool(row, "AllowTeleport", true);
        RegionReverseBox.IsChecked = ReadBool(row, "AllowReverse", true);
        RegionTraceBox.IsChecked = ReadBool(row, "AllowTrace", true);
        RegionMoveBox.IsChecked = ReadBool(row, "AllowMovement", true);
        RegionChatBox.IsChecked = ReadBool(row, "AllowChat", true);
        RegionGlobalBox.IsChecked = ReadBool(row, "AllowGlobalChat", true);
        RegionPartyBox.IsChecked = ReadBool(row, "AllowParty", true);
        RegionExchangeBox.IsChecked = ReadBool(row, "AllowExchange", true);
        RegionStallBox.IsChecked = ReadBool(row, "AllowStall", true);
        RegionPvpBox.IsChecked = ReadBool(row, "AllowPvP", true);
        RegionAlchemyBox.IsChecked = ReadBool(row, "AllowAlchemy", true);
        RegionSpecialItemsBox.IsChecked = ReadBool(row, "AllowSpecialItems", true);
        RegionZerkBox.IsChecked = ReadBool(row, "AllowBerserk", true);
        RegionAutoPvpBox.Text = Convert.ToString(row["AutoPvpCape"]) ?? "0";
        RegionInactivitySecondsBox.Text = Convert.ToString(row["InactivityReturnSeconds"]) ?? "0";
        SetTaggedValue(RegionEventSuitModeBox, ReadInt(row, "EventSuitMode", 0));
        RegionControlStatusText.Text = $"Loaded World {RegionWorldIdBox.Text}, Region {RegionIdBox.Text}.";
    }

    private async void AddSpecialOfferButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (string.IsNullOrWhiteSpace(OfferTitleBox.Text))
                throw new InvalidOperationException("Offer title is required.");

            await CreateSqlService().AddSpecialOfferAsync(
                OfferTitleBox.Text,
                ParseRequiredInt(OfferItemIdBox.Text, "Item ID"),
                ParseRequiredInt(OfferItemCountBox.Text, "Item count"),
                ParseInt(OfferMainPriceBox.Text, 0),
                ParseRequiredInt(OfferSalePriceBox.Text, "Sale price"),
                ParseInt(OfferPaymentTypeBox.Text, 0),
                ParseInt(OfferSortOrderBox.Text, 0),
                OfferActiveBox.IsChecked == true);
            await LoadSpecialOffersAsync();
            SetStatus("Special offer created.", true);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void SpecialOffersGrid_MouseDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
    {
        if (GetSelectedRow(SpecialOffersGrid) is not { } row || !row.Row.Table.Columns.Contains("ID"))
            return;

        var id = Convert.ToInt32(row["ID"]);
        var active = row.Row.Table.Columns.Contains("Service") && Convert.ToBoolean(row["Service"]);
        if (MessageBox.Show(this, $"Set offer #{id} {(active ? "inactive" : "active")}?", "Special Offers", MessageBoxButton.YesNo, MessageBoxImage.Question) != MessageBoxResult.Yes)
            return;

        await CreateSqlService().SetSpecialOfferActiveAsync(id, !active);
        await LoadSpecialOffersAsync();
    }

    private async void LoadKillerAnimationsButton_Click(object sender, RoutedEventArgs e) => await LoadKillerAnimationsAsync();

    private async void LoadVipTiersButton_Click(object sender, RoutedEventArgs e) => await LoadVipTiersAsync();

    private async void VipSystemToggleButton_Click(object sender, RoutedEventArgs e)
    {
        var nextState = !_vipSystemEnabled;
        var action = nextState ? "enable" : "disable";
        var message = nextState
            ? "Enable the VIP System? The change will take effect after the Filter is restarted."
            : "Disable the VIP System? VIP spend processing and VIP rank handling will stop after the Filter is restarted. Existing tier and ranking data will remain saved.";

        if (MessageBox.Show(this, message, $"{action} VIP System", MessageBoxButton.YesNo, MessageBoxImage.Question) != MessageBoxResult.Yes)
            return;

        try
        {
            VipSystemToggleButton.IsEnabled = false;
            await CreateSqlService().SetVipSystemEnabledAsync(nextState);
            _vipSystemEnabled = nextState;
            UpdateVipSystemToggleState();
            VipSystemStatusText.Text = nextState
                ? "VIP System enabled in the database. Restart the Filter to apply it."
                : "VIP System disabled in the database. Restart the Filter to stop VIP processing.";
        }
        catch (Exception ex)
        {
            VipSystemStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
        finally
        {
            VipSystemToggleButton.IsEnabled = true;
        }
    }

    private async void SaveVipTiersButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (!VipTiersGrid.CommitEdit(DataGridEditingUnit.Cell, true) ||
                !VipTiersGrid.CommitEdit(DataGridEditingUnit.Row, true))
            {
                throw new InvalidOperationException("Fix the highlighted Required Silk value before saving.");
            }
            SaveVipTiersButton.IsEnabled = false;
            VipSystemStatusText.Text = "Validating skills and applying VIP tiers...";

            await CreateSqlService().SaveVipTiersAsync(_vipTiers.ToArray());
            await LoadVipTiersAsync();
            VipSystemStatusText.Text =
                _vipSystemEnabled
                    ? "VIP tiers saved. Player ranks and name icons were recalculated; optional buffs refresh on the next login or tier change."
                    : "VIP tiers saved. VIP System is disabled, so existing player ranks and icons were not recalculated.";
        }
        catch (Exception ex)
        {
            VipSystemStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
        finally
        {
            SaveVipTiersButton.IsEnabled = true;
        }
    }

    private void NewKillerAnimationButton_Click(object sender, RoutedEventArgs e)
    {
        KillerAnimationIdBox.Text = string.Empty;
        KillerCodeNameBox.Text = "KILLER_CUSTOM";
        KillerDisplayNameBox.Text = "Custom Animation";
        KillerAnimationValueBox.Text = "31";
        KillerPriceBox.Text = "100";
        KillerSortOrderBox.Text = "0";
        KillerPaymentTypeBox.SelectedIndex = 0;
        KillerAnimationActiveBox.IsChecked = true;
        KillerAnimationsStatusText.Text = "Ready for a new killer animation row.";
    }

    private async void SaveKillerAnimationButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().SaveKillerAnimationAsync(
                TryParseNullableInt(KillerAnimationIdBox.Text),
                KillerAnimationActiveBox.IsChecked == true,
                ParseInt(KillerSortOrderBox.Text, 0),
                KillerCodeNameBox.Text,
                KillerDisplayNameBox.Text,
                ParseRequiredInt(KillerAnimationValueBox.Text, "Animation ID"),
                ParseRequiredInt(KillerPriceBox.Text, "Price"),
                ReadPaymentType(KillerPaymentTypeBox));

            KillerAnimationsStatusText.Text = "Killer animation saved. Restart the engine to refresh cached shop rows.";
            SetStatus("Killer animation saved.", true);
            await LoadKillerAnimationsAsync();
        }
        catch (Exception ex)
        {
            KillerAnimationsStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void DisableKillerAnimationButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (!int.TryParse(KillerAnimationIdBox.Text.Trim(), out var id) || id <= 0)
                throw new InvalidOperationException("Double click a killer animation row first.");

            var newState = KillerAnimationActiveBox.IsChecked != true;
            await CreateSqlService().SetKillerAnimationActiveAsync(id, newState);
            KillerAnimationActiveBox.IsChecked = newState;
            KillerAnimationsStatusText.Text = $"Killer animation #{id} set {(newState ? "visible" : "hidden")}. Restart the engine to refresh cached shop rows.";
            SetStatus("Killer animation visibility changed.", true);
            await LoadKillerAnimationsAsync();
        }
        catch (Exception ex)
        {
            KillerAnimationsStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void KillerAnimationsGrid_MouseDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
    {
        if (GetSelectedRow(KillerAnimationsGrid) is not { } row || !row.Row.Table.Columns.Contains("ID"))
            return;

        KillerAnimationIdBox.Text = Convert.ToString(row["ID"]) ?? string.Empty;
        KillerAnimationActiveBox.IsChecked = ReadBool(row, "Service", true);
        KillerSortOrderBox.Text = ReadColumnText(row, "SortOrder", "0");
        KillerCodeNameBox.Text = ReadColumnText(row, "CodeName", string.Empty);
        KillerDisplayNameBox.Text = ReadColumnText(row, "DisplayName", string.Empty);
        KillerAnimationValueBox.Text = ReadColumnText(row, "AnimationID", "31");
        KillerPriceBox.Text = ReadColumnText(row, "Price", "0");
        SetPaymentType(KillerPaymentTypeBox, ParseInt(ReadColumnText(row, "PaymentType", "0"), 0));
        KillerAnimationsStatusText.Text = $"Loaded killer animation #{KillerAnimationIdBox.Text} for editing.";
    }

    private async void LoadBlockedWordsButton_Click(object sender, RoutedEventArgs e) => await LoadBlockedWordsAsync();

    private async void LoadAuditButton_Click(object sender, RoutedEventArgs e) => await LoadAuditAsync();

    private async void SearchChatButton_Click(object sender, RoutedEventArgs e) => await SearchChatLogsAsync();

    private async void LoadSchedulerButton_Click(object sender, RoutedEventArgs e) => await LoadSchedulerAsync();

    private async void RefreshDiscordButton_Click(object sender, RoutedEventArgs e) => await LoadDiscordAsync();

    private async void SaveDiscordSettingsButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            DiscordStatusText.Text = "Saving Discord bot settings...";
            await SaveDiscordSettingsFromFieldsAsync();
            UpdateDiscordConnectionBadge();
            DiscordStatusText.Text =
                "Discord settings saved. The Agent service will apply them automatically within 15 seconds.";
        }
        catch (Exception ex)
        {
            DiscordStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void TestDiscordBotButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            DiscordStatusText.Text = "Validating the Discord Bot Token...";
            await SaveDiscordSettingsFromFieldsAsync();
            DiscordStatusText.Text = await CreateSqlService().TestDiscordBotAsync();
            DiscordConnectionDot.Fill = (Brush)FindResource("SuccessBrush");
            DiscordConnectionBadgeText.Foreground = (Brush)FindResource("SuccessBrush");
            DiscordConnectionBadgeText.Text = "CONNECTED";
        }
        catch (Exception ex)
        {
            DiscordConnectionDot.Fill = (Brush)FindResource("RoseBrush");
            DiscordConnectionBadgeText.Foreground = (Brush)FindResource("RoseBrush");
            DiscordConnectionBadgeText.Text = "CONNECTION FAILED";
            DiscordStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void AddDiscordChannelButton_Click(object sender, RoutedEventArgs e)
    {
        var channel = new DiscordNotificationChannel
        {
            Enabled = true,
            SortOrder = _discordChannels.Count == 0
                ? 10
                : _discordChannels.Max(item => item.SortOrder) + 10,
            StatusMessage = "Enter a name and Channel ID."
        };
        _discordChannels.Add(channel);
        DiscordChannelsEmptyPanel.Visibility = Visibility.Collapsed;
        DiscordStatusText.Text = "New channel card added. Complete both fields and choose Save.";
    }

    private async void SaveDiscordChannelButton_Click(object sender, RoutedEventArgs e)
    {
        if ((sender as FrameworkElement)?.DataContext is not DiscordNotificationChannel channel)
            return;

        try
        {
            channel.StatusMessage = "Saving...";
            var isNew = channel.ChannelRecordID <= 0;
            await CreateSqlService().SaveDiscordChannelAsync(channel);
            channel.StatusMessage = isNew ? "Channel added successfully." : "Changes saved.";
            DiscordStatusText.Text =
                $"'{channel.ChannelName}' is ready for SQL and dashboard notifications.";
            RefreshDiscordChannelPicker(channel);
        }
        catch (Exception ex)
        {
            channel.StatusMessage = ex.Message;
            DiscordStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void TestDiscordChannelButton_Click(object sender, RoutedEventArgs e)
    {
        if ((sender as FrameworkElement)?.DataContext is not DiscordNotificationChannel channel)
            return;

        try
        {
            channel.StatusMessage = "Saving and sending a test...";
            await CreateSqlService().SaveDiscordChannelAsync(channel);
            channel.StatusMessage = await CreateSqlService().SendDiscordChannelTestAsync(channel);
            DiscordStatusText.Text =
                $"Discord accepted the test message for '{channel.ChannelName}'.";
            RefreshDiscordChannelPicker(channel);
        }
        catch (Exception ex)
        {
            channel.StatusMessage = ex.Message;
            DiscordStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void RemoveDiscordChannelButton_Click(object sender, RoutedEventArgs e)
    {
        if ((sender as FrameworkElement)?.DataContext is not DiscordNotificationChannel channel)
            return;

        if (channel.ChannelRecordID > 0 &&
            MessageBox.Show(
                this,
                $"Remove the Discord destination '{channel.ChannelName}'?\n\n" +
                "Pending notifications for this destination will be marked as failed.",
                "Remove Discord channel",
                MessageBoxButton.YesNo,
                MessageBoxImage.Warning) != MessageBoxResult.Yes)
        {
            return;
        }

        try
        {
            if (channel.ChannelRecordID > 0)
                await CreateSqlService().DeleteDiscordChannelAsync(channel.ChannelRecordID);
            _discordChannels.Remove(channel);
            DiscordChannelsEmptyPanel.Visibility =
                _discordChannels.Count == 0 ? Visibility.Visible : Visibility.Collapsed;
            RefreshDiscordChannelPicker();
            DiscordStatusText.Text = "Discord channel removed.";
            await LoadDiscordActivityAsync();
        }
        catch (Exception ex)
        {
            channel.StatusMessage = ex.Message;
            DiscordStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void QueueDiscordNotificationButton_Click(object sender, RoutedEventArgs e)
    {
        if (DiscordManualChannelBox.SelectedItem is not DiscordNotificationChannel channel ||
            channel.ChannelRecordID <= 0)
        {
            ShowError("Choose a saved Discord destination first.");
            return;
        }

        try
        {
            DiscordStatusText.Text = "Queueing Discord notification...";
            await CreateSqlService().QueueDiscordNotificationAsync(
                channel.ChannelName,
                DiscordManualMessageBox.Text);
            DiscordManualMessageBox.Clear();
            DiscordStatusText.Text =
                $"Notification queued for '{channel.ChannelName}'. The Agent will deliver it in the background.";
            await LoadDiscordActivityAsync();
        }
        catch (Exception ex)
        {
            DiscordStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void RetryDiscordNotificationsButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().RetryFailedDiscordNotificationsAsync();
            DiscordStatusText.Text = "Failed Discord notifications were returned to the delivery queue.";
            await LoadDiscordActivityAsync();
        }
        catch (Exception ex)
        {
            DiscordStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void RefreshTelegramButton_Click(object sender, RoutedEventArgs e) => await LoadTelegramAsync();

    private async void SaveTelegramButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            TelegramStatusText.Text = "Saving notification settings...";
            var settings = ReadTelegramSettingsFromFields();
            await CreateSqlService().SaveTelegramSettingsAsync(settings);
            _telegramHasStoredBotToken = _telegramHasStoredBotToken ||
                                         !string.IsNullOrWhiteSpace(TelegramBotTokenBox.Password);
            TelegramTokenHintText.Text = "A Bot Token is securely stored for this server.";
            TelegramStatusText.Text = "Settings saved. The Agent service will apply them automatically within 15 seconds.";
        }
        catch (Exception ex)
        {
            TelegramStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void TestTelegramButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            TelegramStatusText.Text = "Validating the Telegram connection...";
            var settings = ReadTelegramSettingsFromFields();
            await CreateSqlService().SaveTelegramSettingsAsync(settings);
            _telegramHasStoredBotToken = _telegramHasStoredBotToken ||
                                         !string.IsNullOrWhiteSpace(TelegramBotTokenBox.Password);
            TelegramStatusText.Text = await CreateSqlService().SendTelegramTestAsync();
        }
        catch (Exception ex)
        {
            TelegramStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void QueueTelegramAnnouncementButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            TelegramStatusText.Text = "Queueing announcement...";
            await CreateSqlService().QueueManualTelegramAnnouncementAsync(
                TelegramAnnouncementTitleBox.Text,
                TelegramAnnouncementBodyBox.Text);
            TelegramAnnouncementTitleBox.Clear();
            TelegramAnnouncementBodyBox.Clear();
            TelegramStatusText.Text = "Announcement queued for secure delivery by the Agent service.";
            BindTable(TelegramDeliveryLogGrid, await CreateSqlService().LoadTelegramDeliveryLogAsync());
        }
        catch (Exception ex)
        {
            TelegramStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void RetryTelegramButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().RetryFailedTelegramNotificationsAsync();
            BindTable(TelegramDeliveryLogGrid, await CreateSqlService().LoadTelegramDeliveryLogAsync());
            TelegramStatusText.Text = "Failed notifications were returned to the delivery queue.";
        }
        catch (Exception ex)
        {
            TelegramStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void SaveSchedulerButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            SchedulerStatusText.Text = "Saving schedule...";
            await CreateSqlService().SaveSchedulerJobAsync(ReadSchedulerEditor());
            SchedulerStatusText.Text = "Schedule saved. The engine will refresh its clockwork within 30 seconds.";
            SetStatus("Procedure schedule saved.", true);
            await LoadSchedulerAsync();
        }
        catch (Exception ex)
        {
            SchedulerStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void NewSchedulerButton_Click(object sender, RoutedEventArgs e) => ResetSchedulerEditor();

    private async void RunSchedulerNowButton_Click(object sender, RoutedEventArgs e)
    {
        if (!TryGetSelectedSchedulerId(out var idx))
        {
            ShowError("Select a saved schedule first. Save a new schedule before running it manually.");
            return;
        }

        try
        {
            SchedulerStatusText.Text = "Running the selected procedure now...";
            SchedulerStatusText.Text = await CreateSqlService().RunSchedulerJobNowAsync(idx);
            await LoadSchedulerAsync(idx);
        }
        catch (Exception ex)
        {
            SchedulerStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void DeleteSchedulerButton_Click(object sender, RoutedEventArgs e)
    {
        if (!TryGetSelectedSchedulerId(out var idx))
        {
            ShowError("Select a saved schedule to delete.");
            return;
        }

        if (MessageBox.Show(
                "Delete the selected schedule? Its execution history will remain available for auditing.",
                "Delete schedule",
                MessageBoxButton.YesNo,
                MessageBoxImage.Warning) != MessageBoxResult.Yes)
        {
            return;
        }

        try
        {
            await CreateSqlService().DeleteSchedulerJobAsync(idx);
            ResetSchedulerEditor();
            SchedulerStatusText.Text = "Schedule deleted.";
            await LoadSchedulerAsync();
        }
        catch (Exception ex)
        {
            SchedulerStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void SchedulerGrid_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_schedulerSelectionLoading ||
            GetSelectedRow(SchedulerGrid) is not { } row ||
            !row.Row.Table.Columns.Contains("Idx"))
        {
            return;
        }

        LoadSchedulerEditor(row);
        await LoadSchedulerHistoryAsync(Convert.ToInt32(row["Idx"]));
    }

    private void SchedulerFrequencyBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        UpdateSchedulerFrequencyPanels();
        UpdateSchedulerPreview();
    }

    private void SchedulerStartImmediatelyBox_Changed(object sender, RoutedEventArgs e)
    {
        if (SchedulerIntervalStartPanel is not null)
            SchedulerIntervalStartPanel.IsEnabled = SchedulerStartImmediatelyBox.IsChecked != true;
        UpdateSchedulerPreview();
    }

    private void SchedulerEditorField_Changed(object sender, RoutedEventArgs e) => UpdateSchedulerPreview();

    private void SchedulerDaysPresetButton_Click(object sender, RoutedEventArgs e)
    {
        if (sender is not Button { Tag: string preset })
            return;

        var weekdays = preset is "EveryDay" or "Weekdays";
        var weekend = preset is "EveryDay" or "Weekend";
        SchedulerMondayBox.IsChecked = weekdays;
        SchedulerTuesdayBox.IsChecked = weekdays;
        SchedulerWednesdayBox.IsChecked = weekdays;
        SchedulerThursdayBox.IsChecked = weekdays;
        SchedulerFridayBox.IsChecked = weekdays;
        SchedulerSaturdayBox.IsChecked = weekend;
        SchedulerSundayBox.IsChecked = weekend;
        UpdateSchedulerPreview();
    }

    private async void LoadWebViewerButton_Click(object sender, RoutedEventArgs e) => await LoadWebViewerAsync();

    private async void AddWebViewerButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().AddWebViewerButtonAsync(
                WebNameBox.Text,
                WebIconBox.Text,
                WebUrlBox.Text,
                ParseRequiredInt(WebWidthBox.Text, "Frame width"),
                ParseRequiredInt(WebHeightBox.Text, "Frame height"),
                ParseInt(WebOrderBox.Text, 0),
                WebEnabledBox.IsChecked == true);
            await LoadWebViewerAsync();
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void WebViewerGrid_MouseDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
    {
        if (GetSelectedRow(WebViewerGrid) is not { } row || !row.Row.Table.Columns.Contains("ID"))
            return;

        var id = Convert.ToInt32(row["ID"]);
        var enabled = row.Row.Table.Columns.Contains("IsEnabled") && Convert.ToBoolean(row["IsEnabled"]);
        await CreateSqlService().SetWebViewerButtonEnabledAsync(id, !enabled);
        await LoadWebViewerAsync();
    }

    private async void LoadWhitelistButton_Click(object sender, RoutedEventArgs e)
    {
        PacketTableBox.Text = "[dbo].[Security_Whitelist]";
        await LoadPacketRulesAsync("[dbo].[Security_Whitelist]");
    }

    private async void LoadBlacklistButton_Click(object sender, RoutedEventArgs e)
    {
        PacketTableBox.Text = "[dbo].[Security_Blacklist]";
        await LoadPacketRulesAsync("[dbo].[Security_Blacklist]");
    }

    private async void AddPacketRuleButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().AddPacketRuleAsync(
                PacketTableBox.Text.Trim(),
                ParseRequiredInt(PacketServerTypeBox.Text, "Server type"),
                ParseMsgId(PacketMsgIdBox.Text));
            await LoadPacketRulesAsync(PacketTableBox.Text);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void LoadDiagnosticsButton_Click(object sender, RoutedEventArgs e)
    {
        ShowPage("Diagnostics");
        await LoadDiagnosticsAsync();
    }

    private async void LoadDataStudioButton_Click(object sender, RoutedEventArgs e) => await LoadDataStudioAsync();

    private async void AdvancedTablesList_SelectionChanged(object sender, SelectionChangedEventArgs e)
    {
        if (AdvancedTablesList.SelectedItem is not ManagedTableInfo table)
            return;

        _selectedManagedTableKey = table.Key;
        await LoadManagedTableAsync(table);
    }

    private async void SearchManagedRowsButton_Click(object sender, RoutedEventArgs e) => await LoadManagedRowsAsync();

    private void NewManagedRowButton_Click(object sender, RoutedEventArgs e)
    {
        foreach (var input in _managedInputs.Values)
        {
            switch (input)
            {
                case CheckBox checkBox:
                    checkBox.IsChecked = false;
                    break;
                case TextBox textBox:
                    textBox.Text = string.Empty;
                    break;
            }
        }

        ManagedStatusText.Text = "Ready for a new row.";
    }

    private async void SaveManagedRowButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (string.IsNullOrWhiteSpace(_selectedManagedTableKey))
                throw new InvalidOperationException("Choose a managed table first.");

            await CreateSqlService().SaveManagedTableRowAsync(_selectedManagedTableKey, ReadManagedFormValues());
            ManagedStatusText.Text = "Row saved.";
            SetStatus("Data Studio row saved.", true);
            await LoadManagedRowsAsync();
        }
        catch (Exception ex)
        {
            ManagedStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void DeleteManagedRowButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (string.IsNullOrWhiteSpace(_selectedManagedTableKey))
                throw new InvalidOperationException("Choose a managed table first.");

            if (MessageBox.Show(this, "Delete selected row from this managed table?", "Confirm delete", MessageBoxButton.YesNo, MessageBoxImage.Warning) != MessageBoxResult.Yes)
                return;

            await CreateSqlService().DeleteManagedTableRowAsync(_selectedManagedTableKey, ReadManagedFormValues());
            ManagedStatusText.Text = "Row deleted.";
            SetStatus("Data Studio row deleted.", true);
            NewManagedRowButton_Click(sender, e);
            await LoadManagedRowsAsync();
        }
        catch (Exception ex)
        {
            ManagedStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void ManagedRowsGrid_MouseDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
    {
        if (GetSelectedRow(ManagedRowsGrid) is not { } row)
            return;

        FillManagedForm(row);
        ManagedStatusText.Text = "Row loaded into the editor.";
    }

    private async void RunSelectedMigrationButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (MigrationsGrid.SelectedItem is not MigrationStatus migration)
                throw new InvalidOperationException("Select a migration first.");

            if (MessageBox.Show(this,
                    $"Run {migration.Name} on database '{ProxyDbBox.Text.Trim()}'?",
                    "Confirm migration",
                    MessageBoxButton.YesNo,
                    MessageBoxImage.Warning) != MessageBoxResult.Yes)
                return;

            var migrationsPath = GetMigrationsPath();
            var batchCount = await CreateSqlService().RunMigrationAsync(migration.FullPath, migrationsPath);
            MigrationsResultText.Text = $"{migration.Name} executed successfully ({batchCount:N0} batch(es)).";
            SetStatus("Migration executed successfully.", true);
            await CheckMigrationsAsync();
        }
        catch (Exception ex)
        {
            MigrationsResultText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async void AddBlockedWordButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            if (string.IsNullOrWhiteSpace(BlockedWordBox.Text))
                throw new InvalidOperationException("Blocked word is required.");

            await CreateSqlService().AddBlockedWordAsync(
                BlockedWordBox.Text,
                ParseInt(BlockedMatchModeBox.Text, 0),
                BlockedActiveBox.IsChecked == true);
            await LoadBlockedWordsAsync();
            SetStatus("Blocked word added.", true);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void LoadUniqueRulesButton_Click(object sender, RoutedEventArgs e) => await LoadUniqueRulesAsync();

    private void NewUniqueRuleButton_Click(object sender, RoutedEventArgs e)
    {
        UniqueRuleMobIdBox.Clear();
        UniqueRuleOffJobBox.IsChecked = false; UniqueRuleOnJobBox.IsChecked = false;
        UniqueRuleThiefBox.IsChecked = false; UniqueRuleTraderBox.IsChecked = false;
        UniqueRuleStrBox.IsChecked = false; UniqueRuleIntBox.IsChecked = false;
        UniqueRuleJobMaskBox.Text = "0"; UniqueRuleCapeMaskBox.Text = "0"; UniqueRuleRaceMaskBox.Text = "0";
        UniqueRulePartyBox.Text = "-1"; UniqueRuleGuildBox.Text = "-1";
        UniqueRulesStatusText.Text = "Ready for a new unique rule.";
    }

    private async void SaveUniqueRuleButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            await CreateSqlService().SaveUniqueRuleAsync(
                ParseRequiredInt(UniqueRuleMobIdBox.Text, "Unique ID"),
                UniqueRuleOffJobBox.IsChecked == true, UniqueRuleOnJobBox.IsChecked == true,
                UniqueRuleThiefBox.IsChecked == true, UniqueRuleTraderBox.IsChecked == true,
                UniqueRuleStrBox.IsChecked == true, UniqueRuleIntBox.IsChecked == true,
                ParseInt(UniqueRuleJobMaskBox.Text, 0), ParseInt(UniqueRuleCapeMaskBox.Text, 0), ParseInt(UniqueRuleRaceMaskBox.Text, 0),
                ParseInt(UniqueRulePartyBox.Text, -1), ParseInt(UniqueRuleGuildBox.Text, -1));
            await LoadUniqueRulesAsync();
            UniqueRulesStatusText.Text = "Unique attack rule saved. Restart the GameServer to reload cached rules.";
            SetStatus("Unique rule saved.", true);
        }
        catch (Exception ex) { UniqueRulesStatusText.Text = ex.Message; ShowError(ex.Message); }
    }

    private async void DeleteUniqueRuleButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            var id = ParseRequiredInt(UniqueRuleMobIdBox.Text, "Unique ID");
            if (MessageBox.Show(this, $"Delete the attack rule for unique ID {id}?", "Unique Rules", MessageBoxButton.YesNo, MessageBoxImage.Warning) != MessageBoxResult.Yes)
                return;
            await CreateSqlService().DeleteUniqueRuleAsync(id);
            await LoadUniqueRulesAsync();
            NewUniqueRuleButton_Click(sender, e);
            SetStatus("Unique rule deleted.", true);
        }
        catch (Exception ex) { UniqueRulesStatusText.Text = ex.Message; ShowError(ex.Message); }
    }

    private void UniqueRulesGrid_MouseDoubleClick(object sender, MouseButtonEventArgs e)
    {
        if (GetSelectedRow(UniqueRulesGrid) is not { } row) return;
        UniqueRuleMobIdBox.Text = ReadColumnText(row, "MobRefObjID", string.Empty);
        UniqueRuleOffJobBox.IsChecked = ReadBool(row, "OnlyOffJob", false); UniqueRuleOnJobBox.IsChecked = ReadBool(row, "OnlyOnJob", false);
        UniqueRuleThiefBox.IsChecked = ReadBool(row, "OnlyByThief", false); UniqueRuleTraderBox.IsChecked = ReadBool(row, "OnlyByTrader", false);
        UniqueRuleStrBox.IsChecked = ReadBool(row, "OnlyStrPlayer", false); UniqueRuleIntBox.IsChecked = ReadBool(row, "OnlyIntPlayer", false);
        UniqueRuleJobMaskBox.Text = ReadColumnText(row, "AllowedJobMask", "0"); UniqueRuleCapeMaskBox.Text = ReadColumnText(row, "AllowedCapeMask", "0"); UniqueRuleRaceMaskBox.Text = ReadColumnText(row, "AllowedRaceMask", "0");
        UniqueRulePartyBox.Text = ReadColumnText(row, "RequireParty", "-1"); UniqueRuleGuildBox.Text = ReadColumnText(row, "RequireGuild", "-1");
        UniqueRulesStatusText.Text = $"Loaded unique rule #{UniqueRuleMobIdBox.Text} for editing.";
    }

    private async void SearchHwidButton_Click(object sender, RoutedEventArgs e)
    {
        try
        {
            _securityGridMode = "Hwid";
            BindTable(SecurityGrid, await CreateSqlService().SearchHwidAsync(HwidSearchBox.Text));
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async void SecurityGrid_MouseDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
    {
        if (GetSelectedRow(SecurityGrid) is not { } row)
            return;

        try
        {
            if (_securityGridMode == "BlockedWords" && row.Row.Table.Columns.Contains("ID"))
            {
                var id = Convert.ToInt32(row["ID"]);
                var active = row.Row.Table.Columns.Contains("IsActive") && Convert.ToBoolean(row["IsActive"]);
                if (MessageBox.Show(this, $"Set blocked word #{id} {(active ? "inactive" : "active")}?", "Chat filter", MessageBoxButton.YesNo, MessageBoxImage.Question) != MessageBoxResult.Yes)
                    return;

                await CreateSqlService().SetBlockedWordActiveAsync(id, !active);
                await LoadBlockedWordsAsync();
            }
            else if (_securityGridMode == "Hwid" && row.Row.Table.Columns.Contains("CharID"))
            {
                var charId = Convert.ToInt32(row["CharID"]);
                if (MessageBox.Show(this, $"Deactivate HWID rows for CharID {charId}?", "HWID", MessageBoxButton.YesNo, MessageBoxImage.Warning) != MessageBoxResult.Yes)
                    return;

                await CreateSqlService().DeactivateHwidByCharIdAsync(charId);
                BindTable(SecurityGrid, await CreateSqlService().SearchHwidAsync(HwidSearchBox.Text));
            }
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task RefreshDashboardAsync(bool showDialog = false)
    {
        try
        {
            var service = CreateSqlService();
            var health = await service.TestAsync();

            MetricSqlStatus.Text = health.IsConnected ? "Online" : "Offline";
            MetricSettings.Text = health.SettingCount.ToString("N0");
            MetricCommands.Text = health.PendingCommands.ToString("N0");
            MetricInvalid.Text = health.InvalidSettings.ToString("N0");
            SidebarStatusText.Text = health.IsConnected ? $"Connected to {_loadedSettings?.ProxyDb}" : health.Message;
            DashboardDbText.Text = health.IsConnected
                ? $"SQL Server {health.ServerVersion}. Tables: {health.TableCount:N0}. Proxy DB: {_loadedSettings?.ProxyDb}."
                : health.Message;
            var sqlBrush = health.IsConnected
                ? GetThemeBrush("SuccessBrush", Brushes.ForestGreen)
                : GetThemeBrush("RoseBrush", Brushes.IndianRed);
            TopSqlStatusDot.Fill = sqlBrush;
            DashboardSqlStatusDot.Fill = sqlBrush;
            DashboardSystemHeadlineText.Text = health.IsConnected
                ? "Database and administration services are reachable"
                : "Database connection needs attention";
            DashboardUpdatedText.Text = $"Updated {DateTime.Now:HH:mm:ss}";

            SetStatus(health.Message, health.IsConnected);
            await RefreshDashboardOperationsAsync(service, health);

            if (showDialog)
            {
                MessageBox.Show(this,
                    health.IsConnected ? "SQL connection succeeded." : health.Message,
                    "SQL Test",
                    MessageBoxButton.OK,
                    health.IsConnected ? MessageBoxImage.Information : MessageBoxImage.Warning);
            }
        }
        catch (Exception ex)
        {
            MetricSqlStatus.Text = "Offline";
            SidebarStatusText.Text = ex.Message;
            DashboardDbText.Text = ex.Message;
            var errorBrush = GetThemeBrush("RoseBrush", Brushes.IndianRed);
            TopSqlStatusDot.Fill = errorBrush;
            DashboardSqlStatusDot.Fill = errorBrush;
            DashboardSystemHeadlineText.Text = "Database connection needs attention";
            DashboardUpdatedText.Text = $"Check failed {DateTime.Now:HH:mm:ss}";
            SetStatus(ex.Message, false);
            await RefreshDashboardOperationsAsync(null, null, ex.Message);

            if (showDialog)
                ShowError(ex.Message);
        }
    }

    private async Task SearchPlayersAsync()
    {
        try
        {
            BindTable(PlayersGrid, await CreateSqlService().SearchPlayersAsync(PlayerSearchBox.Text));
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task SearchItemsAsync()
    {
        try
        {
            BindTable(RewardsGrid, await CreateSqlService().SearchItemsAsync(ItemSearchBox.Text));
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task LoadChestForRewardTargetAsync()
    {
        if (!string.IsNullOrWhiteSpace(RewardCharBox.Text))
            BindTable(RewardsGrid, await CreateSqlService().LoadItemChestAsync(RewardCharBox.Text));
    }

    private async Task LoadLuckySpinAsync()
    {
        try
        {
            BindTable(LuckyRewardsGrid, await CreateSqlService().LoadLuckySpinRewardsAsync());
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task LoadSpecialOffersAsync()
    {
        try
        {
            BindTable(SpecialOffersGrid, await CreateSqlService().LoadSpecialOffersAsync());
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task LoadBlockedWordsAsync()
    {
        try
        {
            _securityGridMode = "BlockedWords";
            BindTable(SecurityGrid, await CreateSqlService().LoadBlockedWordsAsync());
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task LoadUniqueRulesAsync()
    {
        try
        {
            BindTable(UniqueRulesGrid, await CreateSqlService().LoadUniqueRulesAsync());
            UniqueRulesStatusText.Text = "Rules loaded. Double click a row to edit it.";
        }
        catch (Exception ex)
        {
            UniqueRulesStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async Task LoadKillerAnimationsAsync()
    {
        try
        {
            BindTable(KillerAnimationsGrid, await CreateSqlService().LoadKillerAnimationsAsync());
            KillerAnimationsStatusText.Text = "Loaded killer animation controls. Shop changes require engine restart because the filter caches these rows.";
        }
        catch (Exception ex)
        {
            KillerAnimationsStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async Task LoadVipTiersAsync()
    {
        try
        {
            VipSystemStatusText.Text = "Loading VIP tier configuration...";
            var service = CreateSqlService();
            _vipSystemEnabled = await service.LoadVipSystemEnabledAsync();
            UpdateVipSystemToggleState();

            var tiers = await service.LoadVipTiersAsync();
            _vipTiers.Clear();
            foreach (var tier in tiers)
                _vipTiers.Add(tier);

            BindTable(VipPlayersGrid, await service.LoadVipPlayerRankingsAsync());

            var assignedPlayers = tiers.Sum(tier => tier.AssignedPlayers);
            VipSystemStatusText.Text =
                _vipSystemEnabled
                    ? $"Loaded {tiers.Count} VIP tiers and {assignedPlayers:N0} ranked player(s). Empty buff cells give the icon only."
                    : $"VIP System is disabled. Loaded {tiers.Count} tiers and {assignedPlayers:N0} saved ranked player(s); restart the Filter after enabling it.";
        }
        catch (Exception ex)
        {
            _vipTiers.Clear();
            VipPlayersGrid.ItemsSource = null;
            VipSystemStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void UpdateVipSystemToggleState()
    {
        VipSystemToggleText.Text = _vipSystemEnabled ? "Disable VIP System" : "Enable VIP System";
        VipSystemToggleButton.ToolTip = _vipSystemEnabled
            ? "Disable VIP processing after the next Filter restart"
            : "Enable VIP processing after the next Filter restart";
    }

    private async Task LoadEconomyLogAsync(string logType)
    {
        try
        {
            BindTable(EconomyLogsGrid, await CreateSqlService().LoadEconomyLogAsync(logType, EconomySearchBox.Text));
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task LoadAutoEventConfigAsync()
    {
        try
        {
            var currentEventCode = AutoEventCodeBox.Text.Trim();
            var table = await CreateSqlService().LoadAutoEventConfigAsync();
            BindTable(AutoEventsGrid, table);
            AutoEventPickerBox.ItemsSource = table.DefaultView;

            var selectedRow = table.DefaultView
                                  .Cast<DataRowView>()
                                  .FirstOrDefault(row =>
                                      ReadColumnText(row, "EventCode", string.Empty)
                                          .Equals(currentEventCode, StringComparison.OrdinalIgnoreCase))
                              ?? table.DefaultView.Cast<DataRowView>().FirstOrDefault();

            if (selectedRow == null)
            {
                AutoEventCodeBox.Text = string.Empty;
                AutoEventDisplayNameBox.Text = string.Empty;
                AutoEventAlchemyPanel.Visibility = Visibility.Collapsed;
                AutoEventStatusText.Text = "No Auto Events are configured.";
                return;
            }

            _autoEventSelectionLoading = true;
            try
            {
                AutoEventPickerBox.SelectedItem = selectedRow;
                AutoEventsGrid.SelectedItem = selectedRow;
                AutoEventsGrid.ScrollIntoView(selectedRow);
            }
            finally
            {
                _autoEventSelectionLoading = false;
            }

            await LoadAutoEventEditorAsync(selectedRow);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task LoadSelectedAutoEventDetailsAsync()
    {
        var eventCode = AutoEventCodeBox.Text.Trim();
        if (eventCode.Length == 0)
            return;

        try
        {
            var service = CreateSqlService();
            var contentTask = service.LoadAutoEventContentAsync(eventCode);
            var rewardsTask = service.LoadAutoEventRewardsAsync(eventCode);
            var schedulesTask = service.LoadAutoEventSchedulesAsync(eventCode);
            await Task.WhenAll(contentTask, rewardsTask, schedulesTask);

            if (!AutoEventCodeBox.Text.Trim().Equals(eventCode, StringComparison.OrdinalIgnoreCase))
                return;

            BindTable(AutoEventContentGrid, await contentTask);
            BindTable(AutoEventRewardsGrid, await rewardsTask);
            BindTable(AutoEventSchedulesGrid, await schedulesTask);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task LoadAutoEventSchedulesAsync()
    {
        try
        {
            BindTable(
                AutoEventSchedulesGrid,
                await CreateSqlService().LoadAutoEventSchedulesAsync(AutoEventCodeBox.Text));
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task LoadAutoEventContentAsync()
    {
        try
        {
            BindTable(AutoEventContentGrid, await CreateSqlService().LoadAutoEventContentAsync(AutoEventCodeBox.Text));
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task LoadAutoEventRewardsAsync()
    {
        try
        {
            BindTable(AutoEventRewardsGrid, await CreateSqlService().LoadAutoEventRewardsAsync(AutoEventCodeBox.Text));
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task LoadSurvivalPartyAsync()
    {
        try
        {
            var service = CreateSqlService();
            var config = await service.LoadSurvivalPartyConfigAsync();
            if (config.DefaultView.Count > 0)
                ApplySurvivalPartyConfig(config.DefaultView[0]);

            var rewards = await service.LoadSurvivalPartyRewardsAsync();
            ApplySurvivalPartyRewards(rewards);
            BindTable(SurvivalSchedulesGrid, await service.LoadSurvivalPartySchedulesAsync());
            SurvivalStatusText.Text = "Survival Party loaded.";
        }
        catch (Exception ex)
        {
            SurvivalStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void ApplySurvivalPartyConfig(DataRowView row)
    {
        SurvivalDisplayNameBox.Text = ReadColumnText(row, "DisplayName", "Survival Party");
        SurvivalEnabledBox.IsChecked = ReadBool(row, "Enabled", true);
        SurvivalEventIdBox.Text = ReadColumnText(row, "EventID", "12");
        SurvivalStartDelayBox.Text = ReadColumnText(row, "StartDelaySeconds", "60");
        SurvivalRegistrationBox.Text = ReadColumnText(row, "RegistrationSeconds", "60");
        SurvivalFightBox.Text = ReadColumnText(row, "FightSeconds", "600");
        SurvivalMinLevelBox.Text = ReadColumnText(row, "MinLevel", "1");
        SurvivalHwidLimitBox.Text = ReadColumnText(row, "HwidLimit", "1");
        SurvivalRequireHwidBox.IsChecked = ReadBool(row, "RequireHwid", true);
        SurvivalMaxPlayersBox.Text = ReadColumnText(row, "MaxPlayers", "100");
        SurvivalWorldBox.Text = ReadColumnText(row, "ArenaWorldID", "107");
        SurvivalRegionBox.Text = ReadColumnText(row, "ArenaRegionID", "25580");
        SurvivalXBox.Text = ReadColumnText(row, "ArenaX", "500");
        SurvivalYBox.Text = ReadColumnText(row, "ArenaY", "0");
        SurvivalZBox.Text = ReadColumnText(row, "ArenaZ", "500");
    }

    private void ApplySurvivalPartyRewards(DataTable rewards)
    {
        DataRow? winner = null;
        DataRow? loser = null;
        foreach (DataRow row in rewards.Rows)
        {
            var placement = ParseInt(Convert.ToString(row["Placement"]) ?? "0", 0);
            if (placement == 1 && winner == null)
                winner = row;
            else if (placement == 2 && loser == null)
                loser = row;
        }

        ApplySurvivalTeamReward(winner, true);
        ApplySurvivalTeamReward(loser, false);
    }

    private void ApplySurvivalTeamReward(DataRow? row, bool winner)
    {
        var rewardId = row == null ? 0 : ParseInt(Convert.ToString(row["RewardID"]) ?? "0", 0);
        var rewardType = row == null ? "SilkOwn" : Convert.ToString(row["RewardType"]) ?? "SilkOwn";
        var amount = row == null ? (winner ? "500" : "50") : Convert.ToString(row["Amount"]) ?? "0";
        var codeName = row == null || row.IsNull("ItemCodeName128") ? string.Empty : Convert.ToString(row["ItemCodeName128"]) ?? string.Empty;
        var itemId = row == null || row.IsNull("ItemID") ? "0" : Convert.ToString(row["ItemID"]) ?? "0";
        var itemCount = row == null ? "1" : Convert.ToString(row["ItemCount"]) ?? "1";
        var plus = row == null ? "0" : Convert.ToString(row["Plus"]) ?? "0";
        var active = row == null || !row.Table.Columns.Contains("IsActive") || Convert.ToBoolean(row["IsActive"]);

        if (winner)
        {
            _survivalWinnerRewardId = rewardId;
            SetComboText(SurvivalWinnerRewardTypeBox, rewardType);
            SurvivalWinnerRewardAmountBox.Text = amount;
            SurvivalWinnerRewardCodeNameBox.Text = codeName;
            SurvivalWinnerRewardItemIdBox.Text = itemId;
            SurvivalWinnerRewardItemCountBox.Text = itemCount;
            SurvivalWinnerRewardPlusBox.Text = plus;
            SurvivalWinnerRewardActiveBox.IsChecked = active;
        }
        else
        {
            _survivalLoserRewardId = rewardId;
            SetComboText(SurvivalLoserRewardTypeBox, rewardType);
            SurvivalLoserRewardAmountBox.Text = amount;
            SurvivalLoserRewardCodeNameBox.Text = codeName;
            SurvivalLoserRewardItemIdBox.Text = itemId;
            SurvivalLoserRewardItemCountBox.Text = itemCount;
            SurvivalLoserRewardPlusBox.Text = plus;
            SurvivalLoserRewardActiveBox.IsChecked = active;
        }
    }

    private async Task LoadSurvivalSoloAsync()
    {
        try
        {
            var service = CreateSqlService();
            var config = await service.LoadSurvivalSoloConfigAsync();
            if (config.DefaultView.Count > 0)
                ApplySurvivalSoloConfig(config.DefaultView[0]);

            var rewards = await service.LoadSurvivalSoloRewardsAsync();
            ApplySurvivalSoloRewards(rewards);
            BindTable(SurvivalSoloSchedulesGrid, await service.LoadSurvivalSoloSchedulesAsync());
            SurvivalSoloStatusText.Text = "Survival Solo loaded.";
        }
        catch (Exception ex)
        {
            SurvivalSoloStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void ApplySurvivalSoloConfig(DataRowView row)
    {
        SurvivalSoloDisplayNameBox.Text = ReadColumnText(row, "DisplayName", "Survival Solo");
        SurvivalSoloEnabledBox.IsChecked = ReadBool(row, "Enabled", true);
        SurvivalSoloEventIdBox.Text = ReadColumnText(row, "EventID", "13");
        SurvivalSoloStartDelayBox.Text = ReadColumnText(row, "StartDelaySeconds", "60");
        SurvivalSoloRegistrationBox.Text = ReadColumnText(row, "RegistrationSeconds", "60");
        SurvivalSoloFightBox.Text = ReadColumnText(row, "FightSeconds", "600");
        SurvivalSoloMinLevelBox.Text = ReadColumnText(row, "MinLevel", "1");
        SurvivalSoloHwidLimitBox.Text = ReadColumnText(row, "HwidLimit", "1");
        SurvivalSoloRequireHwidBox.IsChecked = ReadBool(row, "RequireHwid", true);
        SurvivalSoloMaxPlayersBox.Text = ReadColumnText(row, "MaxPlayers", "100");
        SurvivalSoloWorldBox.Text = ReadColumnText(row, "ArenaWorldID", "107");
        SurvivalSoloRegionBox.Text = ReadColumnText(row, "ArenaRegionID", "25580");
        SurvivalSoloXBox.Text = ReadColumnText(row, "ArenaX", "500");
        SurvivalSoloYBox.Text = ReadColumnText(row, "ArenaY", "0");
        SurvivalSoloZBox.Text = ReadColumnText(row, "ArenaZ", "500");
    }

    private void ApplySurvivalSoloRewards(DataTable rewards)
    {
        DataRow? winner = null;
        DataRow? loser = null;
        foreach (DataRow row in rewards.Rows)
        {
            var placement = ParseInt(Convert.ToString(row["Placement"]) ?? "0", 0);
            if (placement == 1 && winner == null)
                winner = row;
            else if (placement == 2 && loser == null)
                loser = row;
        }

        ApplySurvivalSoloTeamReward(winner, true);
        ApplySurvivalSoloTeamReward(loser, false);
    }

    private void ApplySurvivalSoloTeamReward(DataRow? row, bool winner)
    {
        var rewardId = row == null ? 0 : ParseInt(Convert.ToString(row["RewardID"]) ?? "0", 0);
        var rewardType = row == null ? "SilkOwn" : Convert.ToString(row["RewardType"]) ?? "SilkOwn";
        var amount = row == null ? (winner ? "500" : "50") : Convert.ToString(row["Amount"]) ?? "0";
        var codeName = row == null || row.IsNull("ItemCodeName128") ? string.Empty : Convert.ToString(row["ItemCodeName128"]) ?? string.Empty;
        var itemId = row == null || row.IsNull("ItemID") ? "0" : Convert.ToString(row["ItemID"]) ?? "0";
        var itemCount = row == null ? "1" : Convert.ToString(row["ItemCount"]) ?? "1";
        var plus = row == null ? "0" : Convert.ToString(row["Plus"]) ?? "0";
        var active = row == null || !row.Table.Columns.Contains("IsActive") || Convert.ToBoolean(row["IsActive"]);

        if (winner)
        {
            _survivalSoloWinnerRewardId = rewardId;
            SetComboText(SurvivalSoloWinnerRewardTypeBox, rewardType);
            SurvivalSoloWinnerRewardAmountBox.Text = amount;
            SurvivalSoloWinnerRewardCodeNameBox.Text = codeName;
            SurvivalSoloWinnerRewardItemIdBox.Text = itemId;
            SurvivalSoloWinnerRewardItemCountBox.Text = itemCount;
            SurvivalSoloWinnerRewardPlusBox.Text = plus;
            SurvivalSoloWinnerRewardActiveBox.IsChecked = active;
        }
        else
        {
            _survivalSoloLoserRewardId = rewardId;
            SetComboText(SurvivalSoloLoserRewardTypeBox, rewardType);
            SurvivalSoloLoserRewardAmountBox.Text = amount;
            SurvivalSoloLoserRewardCodeNameBox.Text = codeName;
            SurvivalSoloLoserRewardItemIdBox.Text = itemId;
            SurvivalSoloLoserRewardItemCountBox.Text = itemCount;
            SurvivalSoloLoserRewardPlusBox.Text = plus;
            SurvivalSoloLoserRewardActiveBox.IsChecked = active;
        }
    }

    private async Task LoadAutoEventUniqueRewardsAsync()
    {
        try
        {
            BindTable(AutoEventUniqueRewardsGrid, await CreateSqlService().LoadAutoEventUniqueRewardsAsync(UniqueRewardSearchBox.Text));
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task LoadPvpArenasAsync()
    {
        try
        {
            _pvpGridMode = "Arenas";
            BindTable(PvpGrid, await CreateSqlService().LoadPvpArenasAsync());
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task LoadRegionControlAsync()
    {
        try
        {
            BindTable(RegionControlGrid, await CreateSqlService().LoadRegionControlAsync(RegionSearchBox.Text));
            RegionControlStatusText.Text = "Double click a region rule to edit it.";
        }
        catch (Exception ex)
        {
            RegionControlStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async Task LoadAuditAsync()
    {
        try
        {
            BindTable(AuditGrid, await CreateSqlService().LoadAuditLogAsync());
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task LoadClientlessAsync()
    {
        try
        {
            var sqlService = CreateSqlService();
            var overview = await sqlService.LoadClientlessOperationsOverviewAsync();
            ClientlessTotalAccountsText.Text = overview.TotalAccounts.ToString("N0");
            ClientlessEnabledAccountsText.Text = overview.EnabledAccounts.ToString("N0");
            ClientlessReadyAccountsText.Text = overview.ReadyAccounts.ToString("N0");
            ClientlessOnlineAccountsText.Text = overview.OnlineAccounts.ToString("N0");
            ClientlessAttentionAccountsText.Text = overview.NeedsAttentionAccounts.ToString("N0");
            BindTable(ClientlessCitySummaryGrid, overview.Cities);
            UpdateClientlessCityHunterPlans(overview.Cities);

            var accounts = await sqlService.LoadClientlessAccountsAsync(
                ClientlessSearchBox.Text,
                ReadComboTag(ClientlessAccountCityFilterBox, "All"),
                ReadComboTag(ClientlessAccountStateFilterBox, "All"));
            BindTable(ClientlessGrid, accounts);
            ClientlessRuntimeText.Text = _filterRuntimeService.IsAgentRunning
                ? await _filterRuntimeService.GetClientlessStatusAsync()
                : "Agent service is stopped.";
            await LoadClientlessPartyFormPolicyAsync();
            ClientlessGridStatusText.Text = $"Showing {accounts.Rows.Count:N0} account(s). Double-click a row to edit it.";
        }
        catch (Exception ex)
        {
            ClientlessGridStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void UpdateClientlessCityHunterPlans(DataTable cities)
    {
        var previous = _clientlessCityHunterPlans.ToDictionary(
            plan => plan.City,
            plan => (plan.IsSelected, plan.DesiredOnlineText, plan.DesiredHuntersText),
            StringComparer.OrdinalIgnoreCase);

        _clientlessCityHunterPlans.Clear();
        foreach (DataRow row in cities.Rows)
        {
            var city = Convert.ToString(row["City"]) ?? "Unassigned";
            var total = Convert.ToInt32(row["Total"]);
            var enabled = Convert.ToInt32(row["Enabled"]);
            var ready = Convert.ToInt32(row["Ready"]);
            var hunters = Convert.ToInt32(row["Assigned hunters"]);
            var parked = Convert.ToInt32(row["Ready parked"]);
            var online = Convert.ToInt32(row["Online"]);
            var hasPrevious = previous.TryGetValue(city, out var old);
            _clientlessCityHunterPlans.Add(new ClientlessCityHunterPlan
            {
                City = city,
                TotalAccounts = total,
                ReadyAccounts = ready,
                CurrentEnabled = enabled,
                CurrentHunters = hunters,
                CurrentParked = parked,
                OnlineAccounts = online,
                IsSelected = hasPrevious ? old.IsSelected : total > 0,
                DesiredOnlineText = hasPrevious ? old.DesiredOnlineText : enabled.ToString(),
                DesiredHuntersText = hasPrevious ? old.DesiredHuntersText : hunters.ToString()
            });
        }
    }

    private async Task LoadClientlessHuntingAsync()
    {
        var loadGeneration = ++_clientlessHuntLoadGeneration;
        var city = ReadComboTag(ClientlessHuntCityBox, "Jangan");
        _clientlessHuntAreas = null;
        ClientlessHuntAreasEditor.ItemsSource = null;
        ClientlessHuntAreasEditor.IsEnabled = false;
        ClientlessHuntStatusText.Text = $"Loading hunting areas for {city}...";
        try
        {
            var workspace = await CreateSqlService().LoadClientlessHuntingWorkspaceAsync(city);
            // City selection and manual refreshes can overlap. Never let an older SQL
            // response replace the editor for the city that is currently selected.
            if (loadGeneration != _clientlessHuntLoadGeneration ||
                !string.Equals(city, ReadComboTag(ClientlessHuntCityBox, "Jangan"), StringComparison.OrdinalIgnoreCase))
            {
                return;
            }

            _clientlessHuntAreas = workspace.Areas;
            ClientlessHuntAreasEditor.ItemsSource = workspace.Areas.DefaultView;
            BindTable(ClientlessHuntAccountsGrid, workspace.Accounts);
            ClientlessHuntNormalCheckBox.IsChecked = workspace.Policy.AttackNormal;
            ClientlessHuntUniqueCheckBox.IsChecked = workspace.Policy.AttackUnique;
            ClientlessHuntUniquePriorityCheckBox.IsChecked = workspace.Policy.UniquePriority;
            ClientlessHuntSkillsCheckBox.IsChecked = workspace.Policy.UseSkills;
            ClientlessHuntBasicAttackCheckBox.IsChecked = workspace.Policy.UseBasicAttack;
            ClientlessHuntHpBox.Text = workspace.Policy.HpPotionPercent.ToString();
            ClientlessHuntMpBox.Text = workspace.Policy.MpPotionPercent.ToString();
            ClientlessHuntStuckBox.Text = workspace.Policy.StuckSeconds.ToString();
            ClientlessHuntTargetTimeoutBox.Text = workspace.Policy.TargetTimeoutSeconds.ToString();
            var activeAreas = workspace.Areas.Rows.Cast<DataRow>().Count(row => Convert.ToBoolean(row["Enabled"]));
            ClientlessHuntStatusText.Text =
                $"{city}: {activeAreas}/5 area(s) enabled, {workspace.Accounts.Rows.Count:N0} account(s). " +
                $"Hunting is {(workspace.Policy.Enabled ? "running" : "paused")} globally.";
        }
        catch (Exception ex)
        {
            if (loadGeneration != _clientlessHuntLoadGeneration)
                return;

            ClientlessHuntStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
        finally
        {
            if (loadGeneration == _clientlessHuntLoadGeneration)
                ClientlessHuntAreasEditor.IsEnabled = true;
        }
    }

    private async Task LoadClientlessPartyFormPolicyAsync()
    {
        if (!_filterRuntimeService.IsAgentRunning)
        {
            ClientlessPartyFormStatusText.Text = "Agent service is stopped. Start it to manage Party Forms.";
            return;
        }

        var policy = await _filterRuntimeService.GetClientlessPartyFormPolicyAsync();
        if (policy == null)
        {
            ClientlessPartyFormStatusText.Text = "Party Form policy is unavailable.";
            return;
        }

        ClientlessPartyTitleBox.Text = policy.Title;
        ClientlessPartyMinLevelBox.Text = policy.MinLevel.ToString();
        ClientlessPartyMaxLevelBox.Text = policy.MaxLevel.ToString();
        SetComboTag(ClientlessPartyModeBox, policy.Mode, "SoloForms");
        SetTaggedValue(ClientlessPartyPurposeBox, policy.Purpose);
        SetTaggedValue(ClientlessPartySettingsBox, policy.SettingsFlag);
        UpdateClientlessPartyModeUi();
        ClientlessPartyFormStatusText.Text = !policy.Enabled
            ? "Disabled - no managed parties or Party Forms are created."
            : string.Equals(policy.Mode, "SoloForms", StringComparison.OrdinalIgnoreCase)
                ? $"Enabled - every online character publishes its own \"{policy.Title}\" Party Form and remains outside managed parties."
                : $"Enabled - characters are grouped by city into parties of up to 8, and only each confirmed leader publishes \"{policy.Title}\".";
    }

    private async Task EnsureFilterServicesRunningForClientlessAsync()
    {
        SaveSettingsForRuntime();
        var status = _filterRuntimeService.GetStatus();
        if (status.IsRunning)
            return;

        ClientlessRuntimeText.Text = "Starting KMTGuard services...";
        status = await _filterRuntimeService.StartAsync();
        ApplyFilterProcessStatus(status);
        if (!status.IsRunning)
            throw new InvalidOperationException(status.Message);
    }

    private async Task SearchChatLogsAsync()
    {
        try
        {
            BindTable(ChatLogsGrid, await CreateSqlService().SearchChatLogAsync(ChatSearchBox.Text));
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task LoadSchedulerAsync(int? selectIdx = null)
    {
        try
        {
            SchedulerStatusText.Text = "Loading schedules...";
            var service = CreateSqlService();
            if (string.IsNullOrWhiteSpace(_schedulerDefaultDatabaseName))
            {
                _schedulerDefaultDatabaseName = await service.LoadCurrentDatabaseNameAsync();
                if (string.IsNullOrWhiteSpace(SchedulerDatabaseBox.Text) &&
                    string.IsNullOrWhiteSpace(SchedulerIdBox.Text))
                {
                    SchedulerDatabaseBox.Text = _schedulerDefaultDatabaseName;
                }
            }

            var table = await service.LoadSchedulerAsync();
            _schedulerSelectionLoading = true;
            BindTable(SchedulerGrid, table);
            SchedulerTotalJobsText.Text = table.Rows.Count.ToString("N0");
            SchedulerActiveJobsText.Text = table.AsEnumerable()
                .Count(row => row.Field<bool>("IsEnabled"))
                .ToString("N0");
            var upcoming = table.AsEnumerable()
                .Where(row => row["NextRunLocal"] != DBNull.Value)
                .Select(row => new
                {
                    Name = Convert.ToString(row["Name"]) ?? "Job",
                    Next = Convert.ToDateTime(row["NextRunLocal"])
                })
                .OrderBy(item => item.Next)
                .FirstOrDefault();
            SchedulerNextRunText.Text = upcoming is null
                ? "No upcoming run."
                : $"Next: {upcoming.Name} · {upcoming.Next:yyyy-MM-dd HH:mm:ss}";

            if (selectIdx.HasValue)
            {
                var selected = table.DefaultView.Cast<DataRowView>()
                    .FirstOrDefault(row => Convert.ToInt32(row["Idx"]) == selectIdx.Value);
                SchedulerGrid.SelectedItem = selected;
                if (selected is not null)
                {
                    SchedulerGrid.ScrollIntoView(selected);
                    LoadSchedulerEditor(selected);
                }
            }
            _schedulerSelectionLoading = false;
            await LoadSchedulerHistoryAsync(selectIdx);
            SchedulerStatusText.Text =
                "Ready. Changes are applied automatically by the running engine within 30 seconds.";
        }
        catch (Exception ex)
        {
            _schedulerSelectionLoading = false;
            SchedulerStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async Task LoadSchedulerHistoryAsync(int? idx)
    {
        BindTable(SchedulerHistoryGrid, await CreateSqlService().LoadSchedulerHistoryAsync(idx));
    }

    private void InitializeSchedulerEditor()
    {
        var rounded = DateTime.Now.AddMinutes(1);
        rounded = new DateTime(rounded.Year, rounded.Month, rounded.Day, rounded.Hour, rounded.Minute, 0);
        SchedulerOnceDatePicker.SelectedDate = rounded.Date;
        SchedulerStartDatePicker.SelectedDate = rounded.Date;
        SchedulerOnceTimeBox.Text = rounded.ToString("HH:mm");
        SchedulerStartTimeBox.Text = rounded.ToString("HH:mm");
        SchedulerFrequencyBox.SelectedIndex = 1;
        UpdateSchedulerFrequencyPanels();
        UpdateSchedulerPreview();
    }

    private void ResetSchedulerEditor()
    {
        _schedulerSelectionLoading = true;
        SchedulerGrid.SelectedItem = null;
        SchedulerIdBox.Clear();
        SchedulerEditorTitleText.Text = "New schedule";
        SchedulerNameBox.Clear();
        SchedulerDatabaseBox.Text = _schedulerDefaultDatabaseName;
        SchedulerProcedureBox.Text = string.Empty;
        SchedulerArgumentsBox.Clear();
        SchedulerFrequencyBox.SelectedIndex = 1;
        SchedulerIntervalValueBox.Text = "10";
        SchedulerIntervalUnitBox.SelectedIndex = 0;
        SchedulerStartImmediatelyBox.IsChecked = true;
        SchedulerDailyTimeBox.Text = "00:00";
        SchedulerWeeklyTimeBox.Text = "20:00";
        SchedulerMondayBox.IsChecked = true;
        SchedulerTuesdayBox.IsChecked = true;
        SchedulerWednesdayBox.IsChecked = true;
        SchedulerThursdayBox.IsChecked = true;
        SchedulerFridayBox.IsChecked = true;
        SchedulerSaturdayBox.IsChecked = true;
        SchedulerSundayBox.IsChecked = true;
        SchedulerTimeoutBox.Text = "7200";
        SchedulerCatchUpMinutesBox.Text = "5";
        SchedulerEnabledBox.IsChecked = true;
        var rounded = DateTime.Now.AddMinutes(1);
        rounded = new DateTime(rounded.Year, rounded.Month, rounded.Day, rounded.Hour, rounded.Minute, 0);
        SchedulerOnceDatePicker.SelectedDate = rounded.Date;
        SchedulerStartDatePicker.SelectedDate = rounded.Date;
        SchedulerOnceTimeBox.Text = rounded.ToString("HH:mm");
        SchedulerStartTimeBox.Text = rounded.ToString("HH:mm");
        _schedulerSelectionLoading = false;
        UpdateSchedulerFrequencyPanels();
        UpdateSchedulerPreview();
        SchedulerStatusText.Text = "New schedule ready.";
    }

    private SchedulerJobInput ReadSchedulerEditor()
    {
        var repeatType = GetSelectedComboTag(SchedulerFrequencyBox, "Interval");
        var database = SchedulerDatabaseBox.Text.Trim();
        if (string.IsNullOrWhiteSpace(database))
            throw new InvalidOperationException("Type the database name.");

        var procedure = SchedulerProcedureBox.Text.Trim();
        if (string.IsNullOrWhiteSpace(procedure))
            throw new InvalidOperationException("Type the stored procedure name.");

        var arguments = SchedulerArgumentsBox.Text.Trim();

        var timeoutSeconds = ParseRequiredInt(SchedulerTimeoutBox.Text, "Execution timeout");
        var catchUpMinutes = ParseRequiredInt(SchedulerCatchUpMinutesBox.Text, "Missed-run tolerance");
        if (catchUpMinutes is < 1 or > 10080)
            throw new InvalidOperationException("Missed-run tolerance must be between 1 minute and 7 days.");

        DateTime? scheduledDate = null;
        DateTime? startDateTime = null;
        var time = TimeSpan.Zero;
        int? intervalSeconds = null;
        byte? daysMask = null;

        switch (repeatType)
        {
            case "None":
                scheduledDate = SchedulerOnceDatePicker.SelectedDate ??
                    throw new InvalidOperationException("Choose the one-time run date.");
                time = ParseSchedulerTime(SchedulerOnceTimeBox.Text, "One-time run");
                break;

            case "Interval":
                var intervalValue = ParseRequiredInt(SchedulerIntervalValueBox.Text, "Repeat interval");
                if (intervalValue < 1)
                    throw new InvalidOperationException("Repeat interval must be at least 1.");
                intervalSeconds = GetSelectedComboTag(SchedulerIntervalUnitBox, "Minutes") switch
                {
                    "Hours" => checked(intervalValue * 3600),
                    "Days" => checked(intervalValue * 86400),
                    _ => checked(intervalValue * 60)
                };
                if (intervalSeconds > 604800)
                    throw new InvalidOperationException("The longest supported interval is 7 days.");
                startDateTime = SchedulerStartImmediatelyBox.IsChecked == true
                    ? DateTime.Now
                    : CombineSchedulerDateTime(
                        SchedulerStartDatePicker.SelectedDate,
                        SchedulerStartTimeBox.Text,
                        "First run");
                time = startDateTime.Value.TimeOfDay;
                catchUpMinutes = Math.Max(catchUpMinutes, (int)Math.Ceiling(intervalSeconds.Value / 60d));
                break;

            case "Daily":
                time = ParseSchedulerTime(SchedulerDailyTimeBox.Text, "Daily run");
                break;

            case "Weekly":
                time = ParseSchedulerTime(SchedulerWeeklyTimeBox.Text, "Weekly run");
                daysMask = GetSchedulerDaysMask();
                if (daysMask == 0)
                    throw new InvalidOperationException("Choose at least one weekday.");
                break;
        }

        return new SchedulerJobInput
        {
            Idx = int.TryParse(SchedulerIdBox.Text, out var idx) ? idx : null,
            Name = SchedulerNameBox.Text,
            DatabaseName = database,
            ProcedureName = procedure,
            Arguments = arguments,
            RepeatType = repeatType,
            StartDateTime = startDateTime,
            ScheduledDate = scheduledDate,
            Time = time,
            IntervalSeconds = intervalSeconds,
            DaysOfWeekMask = daysMask,
            Enabled = SchedulerEnabledBox.IsChecked == true,
            ExecutionTimeoutSeconds = timeoutSeconds,
            CatchUpWindowSeconds = checked(catchUpMinutes * 60)
        };
    }

    private void LoadSchedulerEditor(DataRowView row)
    {
        _schedulerSelectionLoading = true;
        SchedulerIdBox.Text = ReadColumnText(row, "Idx", string.Empty);
        SchedulerEditorTitleText.Text = $"Edit schedule #{SchedulerIdBox.Text}";
        SchedulerNameBox.Text = ReadColumnText(row, "Name", string.Empty);
        var target = SqlAdminService.ParseSchedulerExecQuery(
            ReadColumnText(row, "Query", string.Empty),
            _schedulerDefaultDatabaseName);
        SchedulerDatabaseBox.Text = target.DatabaseName;
        SchedulerProcedureBox.Text = target.ProcedureName;
        SchedulerArgumentsBox.Text = target.Arguments;

        var repeatType = ReadColumnText(row, "RepeatType", "None");
        SelectComboByTag(SchedulerFrequencyBox, repeatType);
        SchedulerEnabledBox.IsChecked = ReadBool(row, "IsEnabled", true);
        SchedulerTimeoutBox.Text = ReadColumnText(row, "ExecutionTimeoutSeconds", "7200");
        var catchUpSeconds = ParseInt(ReadColumnText(row, "CatchUpWindowSeconds", "300"), 300);
        SchedulerCatchUpMinutesBox.Text = Math.Max(1, (int)Math.Ceiling(catchUpSeconds / 60d)).ToString();

        var timeText = ReadColumnText(row, "Time", "00:00");
        if (TimeSpan.TryParse(timeText, out var savedTime))
            timeText = savedTime.ToString("hh\\:mm");

        if (repeatType.Equals("None", StringComparison.OrdinalIgnoreCase))
        {
            SchedulerOnceDatePicker.SelectedDate = TryReadSchedulerDate(row, "ScheduledDate") ?? DateTime.Today;
            SchedulerOnceTimeBox.Text = timeText;
        }
        else if (repeatType.Equals("Interval", StringComparison.OrdinalIgnoreCase))
        {
            var intervalSeconds = ParseInt(ReadColumnText(row, "IntervalSeconds", "600"), 600);
            if (intervalSeconds % 86400 == 0)
            {
                SchedulerIntervalValueBox.Text = (intervalSeconds / 86400).ToString();
                SelectComboByTag(SchedulerIntervalUnitBox, "Days");
            }
            else if (intervalSeconds % 3600 == 0)
            {
                SchedulerIntervalValueBox.Text = (intervalSeconds / 3600).ToString();
                SelectComboByTag(SchedulerIntervalUnitBox, "Hours");
            }
            else
            {
                SchedulerIntervalValueBox.Text = Math.Max(1, intervalSeconds / 60).ToString();
                SelectComboByTag(SchedulerIntervalUnitBox, "Minutes");
            }

            var start = TryReadSchedulerDate(row, "StartDateTime") ?? DateTime.Now;
            SchedulerStartImmediatelyBox.IsChecked = false;
            SchedulerStartDatePicker.SelectedDate = start.Date;
            SchedulerStartTimeBox.Text = start.ToString("HH:mm");
        }
        else if (repeatType.Equals("Daily", StringComparison.OrdinalIgnoreCase))
        {
            SchedulerDailyTimeBox.Text = timeText;
        }
        else if (repeatType.Equals("Weekly", StringComparison.OrdinalIgnoreCase))
        {
            SchedulerWeeklyTimeBox.Text = timeText;
            var mask = ParseInt(ReadColumnText(row, "DaysOfWeekMask", "0"), 0);
            if (mask == 0)
            {
                var legacyDay = ParseInt(ReadColumnText(row, "RepeatDayOfWeek", "1"), 1);
                mask = 1 << (Math.Clamp(legacyDay, 1, 7) - 1);
            }
            ApplySchedulerDaysMask(mask);
        }

        _schedulerSelectionLoading = false;
        UpdateSchedulerFrequencyPanels();
        UpdateSchedulerPreview();
        SchedulerStatusText.Text = $"Loaded “{SchedulerNameBox.Text}” for editing.";
    }

    private void UpdateSchedulerFrequencyPanels()
    {
        if (SchedulerOncePanel is null)
            return;
        var repeatType = GetSelectedComboTag(SchedulerFrequencyBox, "Interval");
        SchedulerOncePanel.Visibility = repeatType == "None" ? Visibility.Visible : Visibility.Collapsed;
        SchedulerIntervalPanel.Visibility = repeatType == "Interval" ? Visibility.Visible : Visibility.Collapsed;
        SchedulerDailyPanel.Visibility = repeatType == "Daily" ? Visibility.Visible : Visibility.Collapsed;
        SchedulerWeeklyPanel.Visibility = repeatType == "Weekly" ? Visibility.Visible : Visibility.Collapsed;
        SchedulerIntervalStartPanel.IsEnabled = SchedulerStartImmediatelyBox.IsChecked != true;
    }

    private void UpdateSchedulerPreview()
    {
        if (SchedulerPreviewText is null)
            return;
        var repeatType = GetSelectedComboTag(SchedulerFrequencyBox, "Interval");
        SchedulerPreviewText.Text = repeatType switch
        {
            "None" => $"Run once on {SchedulerOnceDatePicker.SelectedDate:yyyy-MM-dd} at {SchedulerOnceTimeBox.Text}.",
            "Daily" => $"Run every day at {SchedulerDailyTimeBox.Text}.",
            "Weekly" => $"Run on {DescribeSchedulerDays(GetSchedulerDaysMask())} at {SchedulerWeeklyTimeBox.Text}.",
            _ => $"Run every {SchedulerIntervalValueBox.Text} {GetSelectedComboTag(SchedulerIntervalUnitBox, "Minutes").ToLowerInvariant()}" +
                 (SchedulerStartImmediatelyBox.IsChecked == true
                     ? ", starting immediately."
                     : $" from {SchedulerStartDatePicker.SelectedDate:yyyy-MM-dd} {SchedulerStartTimeBox.Text}.")
        };
    }

    private static TimeSpan ParseSchedulerTime(string text, string fieldName)
    {
        if (!TimeSpan.TryParse(text.Trim(), out var value) ||
            value < TimeSpan.Zero ||
            value >= TimeSpan.FromDays(1))
        {
            throw new InvalidOperationException($"{fieldName} time must use HH:mm, for example 20:30.");
        }
        return value;
    }

    private static DateTime CombineSchedulerDateTime(DateTime? date, string timeText, string fieldName)
    {
        if (!date.HasValue)
            throw new InvalidOperationException($"Choose the {fieldName.ToLowerInvariant()} date.");
        return date.Value.Date.Add(ParseSchedulerTime(timeText, fieldName));
    }

    private static string GetSelectedComboTag(ComboBox comboBox, string fallback) =>
        comboBox.SelectedItem is ComboBoxItem { Tag: string tag } ? tag : fallback;

    private static void SelectComboByTag(ComboBox comboBox, string tag)
    {
        foreach (var item in comboBox.Items.OfType<ComboBoxItem>())
        {
            if (string.Equals(Convert.ToString(item.Tag), tag, StringComparison.OrdinalIgnoreCase))
            {
                comboBox.SelectedItem = item;
                return;
            }
        }
    }

    private byte GetSchedulerDaysMask()
    {
        byte mask = 0;
        if (SchedulerMondayBox.IsChecked == true) mask |= 1 << 0;
        if (SchedulerTuesdayBox.IsChecked == true) mask |= 1 << 1;
        if (SchedulerWednesdayBox.IsChecked == true) mask |= 1 << 2;
        if (SchedulerThursdayBox.IsChecked == true) mask |= 1 << 3;
        if (SchedulerFridayBox.IsChecked == true) mask |= 1 << 4;
        if (SchedulerSaturdayBox.IsChecked == true) mask |= 1 << 5;
        if (SchedulerSundayBox.IsChecked == true) mask |= 1 << 6;
        return mask;
    }

    private void ApplySchedulerDaysMask(int mask)
    {
        SchedulerMondayBox.IsChecked = (mask & (1 << 0)) != 0;
        SchedulerTuesdayBox.IsChecked = (mask & (1 << 1)) != 0;
        SchedulerWednesdayBox.IsChecked = (mask & (1 << 2)) != 0;
        SchedulerThursdayBox.IsChecked = (mask & (1 << 3)) != 0;
        SchedulerFridayBox.IsChecked = (mask & (1 << 4)) != 0;
        SchedulerSaturdayBox.IsChecked = (mask & (1 << 5)) != 0;
        SchedulerSundayBox.IsChecked = (mask & (1 << 6)) != 0;
    }

    private static string DescribeSchedulerDays(byte mask)
    {
        if (mask == 127) return "every day";
        if (mask == 31) return "weekdays";
        if (mask == 96) return "the weekend";
        var names = new[] { "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };
        return string.Join(", ", Enumerable.Range(0, 7)
            .Where(index => (mask & (1 << index)) != 0)
            .Select(index => names[index]));
    }

    private bool TryGetSelectedSchedulerId(out int idx)
    {
        if (GetSelectedRow(SchedulerGrid) is { } row &&
            row.Row.Table.Columns.Contains("Idx") &&
            int.TryParse(Convert.ToString(row["Idx"]), out idx))
        {
            return true;
        }
        return int.TryParse(SchedulerIdBox.Text, out idx);
    }

    private static DateTime? TryReadSchedulerDate(DataRowView row, string column)
    {
        if (!row.Row.Table.Columns.Contains(column) || row[column] == DBNull.Value)
            return null;
        return Convert.ToDateTime(row[column]);
    }

    private async Task LoadDiscordAsync()
    {
        try
        {
            DiscordStatusText.Text = "Loading Discord configuration...";
            var service = CreateSqlService();
            var settings = await service.LoadDiscordSettingsAsync();
            _discordHasStoredBotToken = settings.HasStoredBotToken;
            DiscordEnabledBox.IsChecked = settings.Enabled;
            DiscordBotTokenBox.Clear();
            DiscordTokenHintText.Text = settings.HasStoredBotToken
                ? "A Bot Token is securely stored. Leave this field empty to keep it unchanged."
                : "Paste the token from the Discord Developer Portal.";

            _discordChannels.Clear();
            foreach (var channel in await service.LoadDiscordChannelsAsync())
                _discordChannels.Add(channel);
            DiscordChannelsEmptyPanel.Visibility =
                _discordChannels.Count == 0 ? Visibility.Visible : Visibility.Collapsed;
            RefreshDiscordChannelPicker();
            await LoadDiscordActivityAsync(service);
            UpdateDiscordConnectionBadge();
            DiscordStatusText.Text = settings.Enabled
                ? "Discord delivery is enabled and ready for queued notifications."
                : "Discord delivery is currently disabled.";
        }
        catch (Exception ex)
        {
            DiscordStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async Task SaveDiscordSettingsFromFieldsAsync()
    {
        var hasNewToken = !string.IsNullOrWhiteSpace(DiscordBotTokenBox.Password);
        await CreateSqlService().SaveDiscordSettingsAsync(new DiscordNotificationSettings
        {
            Enabled = DiscordEnabledBox.IsChecked == true,
            BotToken = DiscordBotTokenBox.Password,
            HasStoredBotToken = _discordHasStoredBotToken
        });
        _discordHasStoredBotToken = _discordHasStoredBotToken || hasNewToken;
        DiscordBotTokenBox.Clear();
        DiscordTokenHintText.Text =
            "A Bot Token is securely stored. Leave this field empty to keep it unchanged.";
    }

    private async Task LoadDiscordActivityAsync(SqlAdminService? service = null)
    {
        service ??= CreateSqlService();
        _discordActivity.Clear();
        foreach (var activity in await service.LoadDiscordDeliveryActivityAsync())
            _discordActivity.Add(activity);
    }

    private void RefreshDiscordChannelPicker(DiscordNotificationChannel? preferred = null)
    {
        DiscordManualChannelBox.Items.Refresh();
        var selection = preferred is { Enabled: true, ChannelRecordID: > 0 }
            ? preferred
            : _discordChannels.FirstOrDefault(item => item.Enabled && item.ChannelRecordID > 0);
        DiscordManualChannelBox.SelectedItem = selection;
    }

    private void UpdateDiscordConnectionBadge()
    {
        if (!_discordHasStoredBotToken)
        {
            DiscordConnectionDot.Fill = (Brush)FindResource("SubtleTextBrush");
            DiscordConnectionBadgeText.Foreground = (Brush)FindResource("MutedBrush");
            DiscordConnectionBadgeText.Text = "NOT CONFIGURED";
            return;
        }

        if (DiscordEnabledBox.IsChecked == true)
        {
            DiscordConnectionDot.Fill = (Brush)FindResource("SuccessBrush");
            DiscordConnectionBadgeText.Foreground = (Brush)FindResource("SuccessBrush");
            DiscordConnectionBadgeText.Text = "ENABLED";
            return;
        }

        DiscordConnectionDot.Fill = (Brush)FindResource("AmberBrush");
        DiscordConnectionBadgeText.Foreground = (Brush)FindResource("AmberBrush");
        DiscordConnectionBadgeText.Text = "READY · DISABLED";
    }

    private async Task LoadTelegramAsync()
    {
        try
        {
            TelegramStatusText.Text = "Loading Telegram configuration...";
            var service = CreateSqlService();
            var settings = await service.LoadTelegramSettingsAsync();
            _telegramHasStoredBotToken = settings.HasStoredBotToken;
            _telegramUniqueSpawnEnabled = settings.UniqueSpawnEnabled;
            _telegramUniqueKillEnabled = settings.UniqueKillEnabled;
            _telegramShowKillerName = settings.ShowKillerName;
            TelegramEnabledBox.IsChecked = settings.Enabled;
            TelegramBotTokenBox.Password = settings.BotToken;
            TelegramChannelIdBox.Text = settings.ChannelId;
            TelegramEventReminderBox.IsChecked = settings.EventReminderEnabled;
            TelegramReminderMinutesBox.Text = settings.ReminderMinutes.Replace(",", ", ");
            TelegramEventStartedBox.IsChecked = settings.EventStartedEnabled;
            TelegramEventFinishedBox.IsChecked = settings.EventFinishedEnabled;
            TelegramServerOnlineBox.IsChecked = settings.ServerOnlineEnabled;
            TelegramFortressWarBox.IsChecked = settings.FortressWarEnabled;
            TelegramTokenHintText.Text = settings.HasStoredBotToken
                ? "A Bot Token is securely stored for this server."
                : "Paste the token issued by BotFather.";

            BindTable(TelegramDeliveryLogGrid, await service.LoadTelegramDeliveryLogAsync());
            TelegramStatusText.Text = settings.Enabled
                ? "Telegram notifications are enabled."
                : "Telegram notifications are currently disabled.";
        }
        catch (Exception ex)
        {
            TelegramStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private TelegramNotificationSettings ReadTelegramSettingsFromFields()
    {
        return new TelegramNotificationSettings
        {
            Enabled = TelegramEnabledBox.IsChecked == true,
            BotToken = TelegramBotTokenBox.Password,
            HasStoredBotToken = _telegramHasStoredBotToken,
            ChannelId = TelegramChannelIdBox.Text,
            UniqueSpawnEnabled = _telegramUniqueSpawnEnabled,
            UniqueKillEnabled = _telegramUniqueKillEnabled,
            ShowKillerName = _telegramShowKillerName,
            EventReminderEnabled = TelegramEventReminderBox.IsChecked == true,
            ReminderMinutes = TelegramReminderMinutesBox.Text,
            EventStartedEnabled = TelegramEventStartedBox.IsChecked == true,
            EventFinishedEnabled = TelegramEventFinishedBox.IsChecked == true,
            ServerOnlineEnabled = TelegramServerOnlineBox.IsChecked == true,
            FortressWarEnabled = TelegramFortressWarBox.IsChecked == true
        };
    }

    private async Task LoadWebViewerAsync()
    {
        try
        {
            BindTable(WebViewerGrid, await CreateSqlService().LoadWebViewerButtonsAsync());
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task LoadPacketRulesAsync(string tableName)
    {
        try
        {
            BindTable(PacketRulesGrid, await CreateSqlService().LoadPacketRulesAsync(tableName.Trim()));
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private async Task LoadDiagnosticsAsync()
    {
        try
        {
            BindTable(DiagnosticsGrid, await CreateSqlService().LoadDiagnosticsAsync());
            DiagnosticsResultText.Text = "Diagnostics completed for the configured proxy database.";
        }
        catch (Exception ex)
        {
            DiagnosticsResultText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async Task LoadDataStudioAsync()
    {
        try
        {
            var service = CreateSqlService();
            var tables = service.GetManagedTables();
            AdvancedTablesList.ItemsSource = tables;

            if (AdvancedTablesList.SelectedItem is ManagedTableInfo selected)
            {
                _selectedManagedTableKey = selected.Key;
                await LoadManagedTableAsync(selected);
            }
            else if (tables.Count > 0)
            {
                AdvancedTablesList.SelectedIndex = 0;
            }
        }
        catch (Exception ex)
        {
            ManagedStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async Task LoadManagedTableAsync(ManagedTableInfo table)
    {
        ManagedTableTitleText.Text = $"{table.Title} - {table.Module}";
        ManagedTableNotesText.Text = $"{table.TableName}. {table.Notes}";
        _managedColumns = await CreateSqlService().LoadManagedTableColumnsAsync(table.Key);
        RenderManagedForm();
        await LoadManagedRowsAsync();
    }

    private async Task LoadManagedRowsAsync()
    {
        if (string.IsNullOrWhiteSpace(_selectedManagedTableKey))
            return;

        try
        {
            BindTable(ManagedRowsGrid, await CreateSqlService().LoadManagedTableRowsAsync(_selectedManagedTableKey, ManagedSearchBox.Text));
            ManagedStatusText.Text = _managedColumns.Count == 0
                ? "Table was not found in the current database. Run its migration first."
                : "Double click a row to edit it.";
        }
        catch (Exception ex)
        {
            ManagedStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void RenderManagedForm()
    {
        _managedInputs.Clear();
        ManagedFormPanel.Children.Clear();

        if (_managedColumns.Count == 0)
        {
            ManagedFormPanel.Children.Add(new TextBlock
            {
                Text = "This table does not exist yet.",
                Foreground = Brushes.SlateGray,
                TextWrapping = TextWrapping.Wrap
            });
            return;
        }

        foreach (var column in _managedColumns)
        {
            var row = new StackPanel { Margin = new Thickness(0, 0, 0, 12) };
            row.Children.Add(new TextBlock
            {
                Text = $"{SplitSettingName(column.Name)} ({column.DataType})",
                Style = (Style)FindResource("FieldLabel")
            });

            FrameworkElement input = column.DataType.Equals("bit", StringComparison.OrdinalIgnoreCase)
                ? new CheckBox { Content = "Enabled / True", IsEnabled = !column.IsComputed, FontWeight = FontWeights.SemiBold }
                : new TextBox
                {
                    Style = (Style)FindResource("InputBox"),
                    IsReadOnly = column.IsComputed || column.DataType.Equals("timestamp", StringComparison.OrdinalIgnoreCase) || column.DataType.Equals("rowversion", StringComparison.OrdinalIgnoreCase)
                };

            if (input is TextBox textBox && column.IsIdentity)
                textBox.IsReadOnly = true;

            row.Children.Add(input);
            _managedInputs[column.Name] = input;
            ManagedFormPanel.Children.Add(row);
        }
    }

    private IReadOnlyDictionary<string, string?> ReadManagedFormValues()
    {
        var values = new Dictionary<string, string?>(StringComparer.OrdinalIgnoreCase);
        foreach (var column in _managedColumns)
        {
            if (!_managedInputs.TryGetValue(column.Name, out var input))
                continue;

            values[column.Name] = input switch
            {
                CheckBox checkBox => checkBox.IsChecked == true ? "1" : "0",
                TextBox textBox => textBox.Text.Trim(),
                _ => null
            };
        }

        return values;
    }

    private void FillManagedForm(DataRowView row)
    {
        foreach (var column in _managedColumns)
        {
            if (!row.Row.Table.Columns.Contains(column.Name) || !_managedInputs.TryGetValue(column.Name, out var input))
                continue;

            var value = row[column.Name] == DBNull.Value ? string.Empty : Convert.ToString(row[column.Name]) ?? string.Empty;
            switch (input)
            {
                case CheckBox checkBox:
                    checkBox.IsChecked = value.Equals("True", StringComparison.OrdinalIgnoreCase) || value == "1";
                    break;
                case TextBox textBox:
                    textBox.Text = value;
                    break;
            }
        }
    }

    private async Task LoadDbSettingsAsync()
    {
        try
        {
            _allSettings = await CreateSqlService().LoadSettingsAsync();
            BuildSettingsSections();
            var loadedCount = _allSettings.Count(setting => setting.Store == _settingsStoreView);
            SettingsFooterText.Text = $"{loadedCount:N0} {GetSettingsStoreLabel()} setting(s) loaded.";
        }
        catch (Exception ex)
        {
            SettingsFooterText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private async Task CheckMigrationsAsync()
    {
        try
        {
            var migrationsPath = GetMigrationsPath();
            MigrationsGrid.ItemsSource = await CreateSqlService().CheckMigrationsAsync(migrationsPath);
            MigrationsResultText.Text = Directory.Exists(migrationsPath)
                ? $"Loaded versioned database updates from {migrationsPath}."
                : "Database updates folder was not found.";
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
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

    private void LoadLogs()
    {
        var logs = _logService.DiscoverLogs();
        LogFilesList.ItemsSource = logs;
        if (logs.Count > 0)
            LogFilesList.SelectedIndex = 0;
        else
            LogTailBox.Text = "No KMTGuard log files were found in the standard output folders.";
    }

    private void LoadSettingsFromFile(string path)
    {
        try
        {
            if (string.IsNullOrWhiteSpace(path))
                throw new InvalidOperationException("Select Settings.json first.");

            _loadedSettings = _settingsFileService.Load(path.Trim());
            ApplySettingsToFields(_loadedSettings);
            SetStatus("Settings.json loaded.", true);
        }
        catch (Exception ex)
        {
            ShowError(ex.Message);
        }
    }

    private void BuildSettingsSections()
    {
        var term = SettingsSearchBox.Text.Trim();
        var sections = _allSettings
            .Where(setting => setting.Store == _settingsStoreView)
            .Where(setting => MatchesSettingSearch(setting, term))
            .Select(GetSection)
            .DistinctBy(section => section.Key, StringComparer.OrdinalIgnoreCase)
            .OrderBy(section => section.Order)
            .ThenBy(section => section.Title)
            .ToList();

        SettingsCategoryList.ItemsSource = sections;

        var selected = sections.FirstOrDefault(section => section.Key.Equals(_selectedSettingsSection, StringComparison.OrdinalIgnoreCase))
            ?? sections.FirstOrDefault();

        if (selected is null)
        {
            _selectedSettingsSection = string.Empty;
            SettingsSectionTitleText.Text = "No settings";
            SettingsSectionHelpText.Text = "No matching settings were found.";
            SettingsFormPanel.Children.Clear();
            return;
        }

        SettingsCategoryList.SelectedItem = selected;
        _selectedSettingsSection = selected.Key;
        RenderSettingsSection(selected.Key);
    }

    private void RenderSettingsSection(string sectionKey)
    {
        _settingInputs.Clear();
        SettingsFormPanel.Children.Clear();

        var section = ResolveSection(sectionKey);
        SettingsSectionTitleText.Text = section.Title;
        SettingsSectionHelpText.Text = section.Help;

        if (sectionKey.StartsWith("GameServer.", StringComparison.OrdinalIgnoreCase))
        {
            SettingsFormPanel.Children.Add(new Border
            {
                Background = new SolidColorBrush(Color.FromRgb(255, 247, 230)),
                BorderBrush = new SolidColorBrush(Color.FromRgb(214, 150, 54)),
                BorderThickness = new Thickness(1),
                CornerRadius = new CornerRadius(6),
                Padding = new Thickness(14, 11, 14, 11),
                Margin = new Thickness(0, 0, 0, 12),
                Child = new TextBlock
                {
                    Text = "Restart every GameServer after saving. Changes on this page are loaded once during GameServer startup. Negative guild-point protection also requires a ShardManager restart.",
                    Foreground = new SolidColorBrush(Color.FromRgb(116, 72, 20)),
                    FontWeight = FontWeights.SemiBold,
                    TextWrapping = TextWrapping.Wrap
                }
            });
        }

        var settings = GetVisibleSettingsForSection(sectionKey).ToList();
        if (settings.Count == 0)
        {
            SettingsFormPanel.Children.Add(new TextBlock
            {
                Text = "No settings in this section.",
                Foreground = Brushes.SlateGray,
                TextWrapping = TextWrapping.Wrap
            });
            return;
        }

        foreach (var entry in settings)
        {
            var definition = ResolveDefinition(entry);
            var row = new Grid();
            row.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1, GridUnitType.Star) });
            row.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(260) });

            var labelStack = new StackPanel { Margin = new Thickness(0, 0, 24, 0), VerticalAlignment = VerticalAlignment.Center };
            labelStack.Children.Add(new TextBlock
            {
                Text = definition.Label,
                FontWeight = FontWeights.SemiBold,
                FontSize = 13,
                TextWrapping = TextWrapping.Wrap
            });
            labelStack.Children.Add(new TextBlock
            {
                Text = definition.Help,
                Foreground = GetThemeBrush("MutedBrush", Brushes.DimGray),
                FontSize = 11.5,
                Margin = new Thickness(0, 4, 0, 0),
                TextWrapping = TextWrapping.Wrap
            });
            Grid.SetColumn(labelStack, 0);
            row.Children.Add(labelStack);

            var input = CreateSettingInput(entry, definition.Kind);
            Grid.SetColumn(input, 1);
            row.Children.Add(input);
            _settingInputs[entry.SettingName] = input;

            SettingsFormPanel.Children.Add(new Border
            {
                Background = GetThemeBrush("PanelBrush", Brushes.White),
                BorderBrush = GetThemeBrush("LineBrush", Brushes.Gainsboro),
                BorderThickness = new Thickness(0, 0, 0, 1),
                CornerRadius = new CornerRadius(0),
                Padding = new Thickness(4, 14, 4, 14),
                Child = row
            });
        }

        if (sectionKey.Equals("GameServer.Patch", StringComparison.OrdinalIgnoreCase) &&
            _settingInputs.TryGetValue("EnablePartyMonsterSpawn", out var enableInput) &&
            enableInput is CheckBox enableCheckBox)
        {
            enableCheckBox.Checked += PartyMonsterEnableCheckBox_Changed;
            enableCheckBox.Unchecked += PartyMonsterEnableCheckBox_Changed;
            UpdatePartyMonsterSettingInputsState(enableCheckBox.IsChecked == true);
        }
    }

    private void PartyMonsterEnableCheckBox_Changed(object sender, RoutedEventArgs e)
    {
        if (sender is CheckBox checkBox)
            UpdatePartyMonsterSettingInputsState(checkBox.IsChecked == true);
    }

    private void UpdatePartyMonsterSettingInputsState(bool enabled)
    {
        if (_settingInputs.TryGetValue("PartyMonsterMinimumMembers", out var minimumMembersInput))
            minimumMembersInput.IsEnabled = enabled;
        if (_settingInputs.TryGetValue("PartyMonsterSpawnRate", out var spawnRateInput))
            spawnRateInput.IsEnabled = enabled;
    }

    private async Task LoadProxyServicesConnectionAsync()
    {
        _proxyServiceInputs.Clear();
        ProxyServicesConnectionPanel.Children.Clear();
        ProxyServicesConnectionPanel.Children.Add(new TextBlock
        {
            Text = "Loading proxy services...",
            Foreground = Brushes.SlateGray,
            TextWrapping = TextWrapping.Wrap
        });

        try
        {
            _proxyServices = await CreateSqlService().LoadProxyServicesAsync();
            ProxyServicesConnectionPanel.Children.Clear();

            if (_proxyServices.Count == 0)
            {
                ProxyServicesConnectionPanel.Children.Add(new TextBlock
                {
                    Text = "System_ProxyServices was not found, or it has no rows.",
                    Foreground = Brushes.SlateGray,
                    TextWrapping = TextWrapping.Wrap
                });
                ProxyServicesStatusText.Text = "No proxy services loaded.";
                return;
            }

            foreach (var service in _proxyServices.OrderBy(GetProxyServiceOrder).ThenBy(service => service.ServiceId))
                RenderProxyServiceCard(service);

            ProxyServicesStatusText.Text = $"{_proxyServices.Count:N0} proxy service(s) loaded. Save, then restart/reload the filter service using these ports.";
        }
        catch (Exception ex)
        {
            ProxyServicesConnectionPanel.Children.Clear();
            ProxyServicesConnectionPanel.Children.Add(new TextBlock
            {
                Text = ex.Message,
                Foreground = Brushes.IndianRed,
                TextWrapping = TextWrapping.Wrap
            });
            ProxyServicesStatusText.Text = ex.Message;
            ShowError(ex.Message);
        }
    }

    private void RenderProxyServiceCard(ProxyServiceEntry service)
    {
        var remoteIpBox = CreateProxyTextBox(service.RemoteIP, 160);
        var remotePortBox = CreateProxyTextBox(service.RemotePort.ToString(), 96);
        var bindPortBox = CreateProxyTextBox(service.BindPort.ToString(), 96);
        var bindIpBox = CreateProxyTextBox(service.BindIP, 132);
        var autoStartBox = new CheckBox
        {
            IsChecked = service.AutoStart,
            VerticalAlignment = VerticalAlignment.Center,
            HorizontalAlignment = HorizontalAlignment.Left,
            Content = "AutoStart",
            FontWeight = FontWeights.SemiBold
        };

        var grid = new Grid();
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(1.2, GridUnitType.Star) });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(170) });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(108) });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(108) });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(142) });
        grid.ColumnDefinitions.Add(new ColumnDefinition { Width = new GridLength(104) });

        var titleStack = new StackPanel { Margin = new Thickness(0, 0, 18, 0), VerticalAlignment = VerticalAlignment.Center };
        var roleLine = new StackPanel { Orientation = Orientation.Horizontal };
        roleLine.Children.Add(new Border
        {
            Width = 8,
            Height = 8,
            CornerRadius = new CornerRadius(4),
            Background = GetProxyServiceAccent(service),
            Margin = new Thickness(0, 0, 8, 0),
            VerticalAlignment = VerticalAlignment.Center
        });
        roleLine.Children.Add(new TextBlock
        {
            Text = GetProxyServiceRole(service),
            FontWeight = FontWeights.SemiBold,
            FontSize = 13
        });
        titleStack.Children.Add(roleLine);
        titleStack.Children.Add(new TextBlock
        {
            Text = string.IsNullOrWhiteSpace(service.Name) ? $"ServiceId {service.ServiceId}" : service.Name,
            Foreground = GetThemeBrush("MutedBrush", Brushes.DimGray),
            FontSize = 11.5,
            Margin = new Thickness(0, 4, 0, 0),
            TextWrapping = TextWrapping.Wrap
        });

        Grid.SetColumn(titleStack, 0);
        grid.Children.Add(titleStack);
        AddProxyField(grid, "IP", remoteIpBox, 1);
        AddProxyField(grid, "Real Port", remotePortBox, 2);
        AddProxyField(grid, "Fake Port", bindPortBox, 3);
        AddProxyField(grid, "Listen IP", bindIpBox, 4);
        Grid.SetColumn(autoStartBox, 5);
        grid.Children.Add(autoStartBox);

        ProxyServicesConnectionPanel.Children.Add(new Border
        {
            Background = GetThemeBrush("PanelBrush", Brushes.White),
            BorderBrush = GetThemeBrush("LineBrush", Brushes.Gainsboro),
            BorderThickness = new Thickness(0, 0, 0, 1),
            CornerRadius = new CornerRadius(0),
            Padding = new Thickness(4, 14, 4, 14),
            Child = grid
        });

        _proxyServiceInputs[service.ServiceId] = new ProxyServiceInputs(
            remoteIpBox,
            remotePortBox,
            bindPortBox,
            bindIpBox,
            autoStartBox);
    }

    private async Task SaveProxyServicesAsync()
    {
        if (_proxyServices.Count == 0 || _proxyServiceInputs.Count == 0)
            throw new InvalidOperationException("Load proxy services first.");

        var serviceClient = CreateSqlService();
        var changed = 0;

        foreach (var current in _proxyServices)
        {
            if (!_proxyServiceInputs.TryGetValue(current.ServiceId, out var input))
                continue;

            var updated = new ProxyServiceEntry
            {
                ServiceId = current.ServiceId,
                Name = current.Name,
                ServerType = current.ServerType,
                RemoteIP = ReadRequiredIp(input.RemoteIpBox.Text, $"{GetProxyServiceRole(current)} IP"),
                RemotePort = ReadRequiredPort(input.RemotePortBox.Text, $"{GetProxyServiceRole(current)} real port"),
                BindIP = ReadRequiredIp(input.BindIpBox.Text, $"{GetProxyServiceRole(current)} listen IP"),
                BindPort = ReadRequiredPort(input.BindPortBox.Text, $"{GetProxyServiceRole(current)} fake port"),
                ByteLimitation = current.ByteLimitation,
                AutoStart = input.AutoStartBox.IsChecked == true
            };

            if (ProxyServiceEquals(current, updated))
                continue;

            await serviceClient.UpdateProxyServiceAsync(updated);
            changed++;
        }

        await LoadProxyServicesConnectionAsync();
        ProxyServicesStatusText.Text = changed == 0
            ? "No proxy service changes to save."
            : $"{changed:N0} proxy service(s) saved. Restart/reload the filter for port changes to take effect.";
    }

    private static TextBox CreateProxyTextBox(string value, double width)
    {
        return new TextBox
        {
            Text = value,
            Width = width,
            Height = 36,
            Padding = new Thickness(10, 6, 10, 6),
            VerticalContentAlignment = VerticalAlignment.Center
        };
    }

    private static void AddProxyField(Grid grid, string label, FrameworkElement input, int column)
    {
        var stack = new StackPanel { Margin = new Thickness(0, 0, 10, 0) };
        stack.Children.Add(new TextBlock
        {
            Text = label,
            Foreground = new SolidColorBrush(Color.FromRgb(112, 106, 96)),
            FontSize = 12,
            FontWeight = FontWeights.SemiBold,
            Margin = new Thickness(0, 0, 0, 5)
        });
        stack.Children.Add(input);
        Grid.SetColumn(stack, column);
        grid.Children.Add(stack);
    }

    private static string ReadRequiredIp(string value, string label)
    {
        var trimmed = value.Trim();
        if (!IPAddress.TryParse(trimmed, out _))
            throw new InvalidOperationException($"{label} must be a valid IP address.");

        return trimmed;
    }

    private static int ReadRequiredPort(string value, string label)
    {
        var port = ParseRequiredInt(value, label);
        if (port is < 1 or > 65535)
            throw new InvalidOperationException($"{label} must be between 1 and 65535.");

        return port;
    }

    private static bool ProxyServiceEquals(ProxyServiceEntry left, ProxyServiceEntry right)
    {
        return left.RemoteIP.Equals(right.RemoteIP, StringComparison.OrdinalIgnoreCase) &&
               left.RemotePort == right.RemotePort &&
               left.BindIP.Equals(right.BindIP, StringComparison.OrdinalIgnoreCase) &&
               left.BindPort == right.BindPort &&
               left.ByteLimitation == right.ByteLimitation &&
               left.AutoStart == right.AutoStart;
    }

    private static string GetProxyServiceRole(ProxyServiceEntry service)
    {
        return service.ServerType switch
        {
            1 => "Download",
            2 => "Gateway",
            3 => "Agent",
            _ when service.Name.Contains("download", StringComparison.OrdinalIgnoreCase) => "Download",
            _ when service.Name.Contains("gateway", StringComparison.OrdinalIgnoreCase) => "Gateway",
            _ when service.Name.Contains("agent", StringComparison.OrdinalIgnoreCase) => "Agent",
            _ => "Service"
        };
    }

    private static int GetProxyServiceOrder(ProxyServiceEntry service)
    {
        return GetProxyServiceRole(service) switch
        {
            "Agent" => 10,
            "Download" => 20,
            "Gateway" => 30,
            _ => 100
        };
    }

    private static SolidColorBrush GetProxyServiceAccent(ProxyServiceEntry service)
    {
        return GetProxyServiceRole(service) switch
        {
            "Agent" => new SolidColorBrush(Color.FromRgb(196, 87, 56)),
            "Download" => new SolidColorBrush(Color.FromRgb(101, 121, 92)),
            "Gateway" => new SolidColorBrush(Color.FromRgb(166, 106, 33)),
            _ => new SolidColorBrush(Color.FromRgb(122, 89, 103))
        };
    }

    private IEnumerable<SettingEntry> GetVisibleSettingsForSection(string sectionKey)
    {
        var term = SettingsSearchBox.Text.Trim();
        return _allSettings
            .Where(setting => setting.Store == _settingsStoreView)
            .Where(setting => GetSection(setting).Key.Equals(sectionKey, StringComparison.OrdinalIgnoreCase))
            .Where(setting => MatchesSettingSearch(setting, term))
            .OrderBy(setting => ResolveDefinition(setting).Order)
            .ThenBy(setting => ResolveDefinition(setting).Label);
    }

    private static FrameworkElement CreateSettingInput(SettingEntry entry, SettingKind kind)
    {
        if (kind == SettingKind.Boolean)
        {
            return new CheckBox
            {
                IsChecked = entry.Value.Equals("True", StringComparison.OrdinalIgnoreCase) || entry.Value == "1",
                VerticalAlignment = VerticalAlignment.Center,
                HorizontalAlignment = HorizontalAlignment.Left,
                Content = "Enabled",
                FontWeight = FontWeights.SemiBold
            };
        }

        return new TextBox
        {
            Text = entry.Value,
            Height = 36,
            Padding = new Thickness(10, 6, 10, 6),
            VerticalContentAlignment = VerticalAlignment.Center
        };
    }

    private static SolidColorBrush GetSettingAccent(SettingKind kind)
    {
        return kind switch
        {
            SettingKind.Boolean => new SolidColorBrush(Color.FromRgb(101, 121, 92)),
            SettingKind.Number => new SolidColorBrush(Color.FromRgb(122, 89, 103)),
            SettingKind.Url => new SolidColorBrush(Color.FromRgb(166, 106, 33)),
            _ => new SolidColorBrush(Color.FromRgb(196, 87, 56))
        };
    }

    private static string ReadSettingInputValue(SettingEntry entry, FrameworkElement input)
    {
        var kind = ResolveDefinition(entry).Kind;
        return kind switch
        {
            SettingKind.Boolean when input is CheckBox checkBox => checkBox.IsChecked == true ? "True" : "False",
            _ when input is TextBox textBox => textBox.Text.Trim(),
            _ => entry.Value
        };
    }

    private static void ValidateSettingValue(SettingEntry entry, string value)
    {
        var definition = ResolveDefinition(entry);
        if (definition.Kind == SettingKind.Number)
        {
            if (!long.TryParse(value, out var number))
                throw new InvalidOperationException($"{definition.Label} must be a whole number.");
            if (definition.Minimum is not null && number < definition.Minimum.Value)
                throw new InvalidOperationException($"{definition.Label} must be at least {definition.Minimum.Value}.");
            if (definition.Maximum is not null && number > definition.Maximum.Value)
                throw new InvalidOperationException($"{definition.Label} must not exceed {definition.Maximum.Value}.");
        }

        if (definition.Kind == SettingKind.Url &&
            !string.IsNullOrWhiteSpace(value) &&
            !Uri.TryCreate(value, UriKind.Absolute, out _))
            throw new InvalidOperationException($"{definition.Label} must be a valid URL or empty.");
    }

    private static bool MatchesSettingSearch(SettingEntry setting, string term)
    {
        if (string.IsNullOrWhiteSpace(term))
            return true;

        var definition = ResolveDefinition(setting);
        var section = GetSection(setting);
        return setting.SettingName.Contains(term, StringComparison.OrdinalIgnoreCase) ||
               setting.Value.Contains(term, StringComparison.OrdinalIgnoreCase) ||
               setting.Category.Contains(term, StringComparison.OrdinalIgnoreCase) ||
               definition.Label.Contains(term, StringComparison.OrdinalIgnoreCase) ||
               section.Title.Contains(term, StringComparison.OrdinalIgnoreCase);
    }

    private static SectionDefinition GetSection(SettingEntry entry)
    {
        var key = ResolveDefinition(entry).Section;
        return ResolveSection(key);
    }

    private static SectionDefinition ResolveSection(string key)
    {
        return SectionDefinitions.TryGetValue(key, out var section)
            ? section
            : new SectionDefinition(key, key, "Additional settings discovered in the database.", 999);
    }

    private static SettingDefinition ResolveDefinition(SettingEntry entry)
    {
        if (entry.Store == SettingStore.GameServer &&
            GameServerSettingDefinitions.TryGetValue(entry.SettingName, out var gameServerDefinition))
            return gameServerDefinition;

        if (SettingDefinitions.TryGetValue(entry.SettingName, out var definition))
            return definition;

        var kind = entry.ValueKind switch
        {
            "Toggle" => SettingKind.Boolean,
            "Number" => SettingKind.Number,
            _ when entry.SettingName.Contains("URL", StringComparison.OrdinalIgnoreCase) => SettingKind.Url,
            _ => SettingKind.Text
        };

        return new SettingDefinition(
            entry.SettingName,
            entry.Category,
            SplitSettingName(entry.SettingName),
            string.IsNullOrWhiteSpace(entry.Description) ? entry.SettingName : entry.Description,
            kind,
            500);
    }

    private static string SplitSettingName(string name)
    {
        return string.Concat(name.Select((ch, index) =>
            index > 0 && char.IsUpper(ch) && !char.IsUpper(name[index - 1]) ? $" {ch}" : ch.ToString()));
    }

    private void ApplySettingsToFields(AdminSettings settings)
    {
        AddressBox.Text = settings.Address;
        PortBox.Text = settings.Port?.ToString() ?? string.Empty;
        UsernameBox.Text = settings.Username;
        PasswordBox.Password = settings.Password;
        ProxyDbBox.Text = settings.ProxyDb;
        MaximumPoolBox.Text = settings.MaximumPool.ToString();
        MinimumPoolBox.Text = settings.MinimumPool.ToString();
        ConnectionLifetimeBox.Text = settings.ConnectionLifetime.ToString();
        ServerIpBox.Text = settings.ServerIP;
        SelectPlayerLanguage(settings.Language);
    }

    private AdminSettings ReadSettingsFromFields()
    {
        var settings = new AdminSettings
        {
            Address = AddressBox.Text.Trim(),
            Username = UsernameBox.Text.Trim(),
            Password = PasswordBox.Password,
            ProxyDb = ProxyDbBox.Text.Trim(),
            ServerIP = ServerIpBox.Text.Trim(),
            Port = TryParseNullableInt(PortBox.Text),
            MaximumPool = ParseInt(MaximumPoolBox.Text, 1500),
            MinimumPool = ParseInt(MinimumPoolBox.Text, 50),
            ConnectionLifetime = ParseInt(ConnectionLifetimeBox.Text, 0),
            Language = ReadSelectedPlayerLanguage(),
            CredentialTarget = _loadedSettings?.CredentialTarget ?? "KMTGuard/Sql",
            QuickLoginCredentialTarget = _loadedSettings?.QuickLoginCredentialTarget ?? "KMTGuard/QuickLoginMasterKey",
            QuickLoginMasterKey = _loadedSettings?.QuickLoginMasterKey ?? string.Empty,
            UpstreamConnectTimeoutSeconds = _loadedSettings?.UpstreamConnectTimeoutSeconds ?? 10,
            HandshakeTimeoutSeconds = _loadedSettings?.HandshakeTimeoutSeconds ?? 15,
            LoginTimeoutSeconds = _loadedSettings?.LoginTimeoutSeconds ?? 60,
            UnauthenticatedIdleTimeoutSeconds = _loadedSettings?.UnauthenticatedIdleTimeoutSeconds ?? 90,
            AuthenticatedHeartbeatTimeoutSeconds = _loadedSettings?.AuthenticatedHeartbeatTimeoutSeconds ?? 180,
            GatewaySessionCap = _loadedSettings?.GatewaySessionCap ?? 10000,
            AgentSessionCap = _loadedSettings?.AgentSessionCap ?? 10000,
            DownloadSessionCap = _loadedSettings?.DownloadSessionCap ?? 2000
        };

        if (string.IsNullOrWhiteSpace(settings.Address))
            throw new InvalidOperationException("SQL host is required.");

        if (string.IsNullOrWhiteSpace(settings.ProxyDb))
            throw new InvalidOperationException("Proxy database is required.");

        return settings;
    }

    private SqlAdminService CreateSqlService()
    {
        _loadedSettings = ReadSettingsFromFields();
        return new SqlAdminService(_loadedSettings.BuildConnectionString());
    }

    private void ConfigureClientlessPage(string page)
    {
        ClientlessOverviewSection.Visibility = page == "ClientlessOverview" ? Visibility.Visible : Visibility.Collapsed;
        ClientlessRuntimeSection.Visibility = page == "ClientlessRuntime" ? Visibility.Visible : Visibility.Collapsed;
        ClientlessProvisioningSection.Visibility = page == "ClientlessProvisioning" ? Visibility.Visible : Visibility.Collapsed;
        ClientlessAccountsSection.Visibility = page == "ClientlessAccounts" ? Visibility.Visible : Visibility.Collapsed;
        ClientlessHuntingSection.Visibility = page == "ClientlessHunting" ? Visibility.Visible : Visibility.Collapsed;
        ClientlessPartySection.Visibility = page == "ClientlessParty" ? Visibility.Visible : Visibility.Collapsed;
        ClientlessDefaultsPanel.Visibility = page is "ClientlessProvisioning" or "ClientlessAccounts"
            ? Visibility.Visible
            : Visibility.Collapsed;

        var (icon, title, description) = page switch
        {
            "ClientlessOverview" => ("\uE9D2", "Clientless Overview", "See account readiness and city totals, then continue with one clear action."),
            "ClientlessProvisioning" => ("\uE8FA", "Create Accounts", "Choose race, weapon, equipment, skills, levels, and account totals for each city."),
            "ClientlessAccounts" => ("\uE910", "Accounts & Status", "Filter accounts by city and readiness, then edit or enable the selected account."),
            "ClientlessHunting" => ("\uE7FC", "Hunting Areas", "Enter five safe coordinates per city, choose combat behavior, and monitor every bot without code."),
            "ClientlessParty" => ("\uE902", "Party Matching", "Configure automatic Party Matching listings for eligible clientless characters."),
            _ => ("\uE768", "Run by City", "Select one or several cities, then start, reload, or stop only those groups.")
        };

        ClientlessPageIconText.Text = icon;
        ClientlessPageTitleText.Text = title;
        ClientlessPageDescriptionText.Text = description;
    }

    private void ConfigureSettingsPage(string page)
    {
        var gameServerPage = page.Equals("GameServerPatch", StringComparison.OrdinalIgnoreCase);
        var requestedStore = gameServerPage ? SettingStore.GameServer : SettingStore.Filter;
        var storeChanged = requestedStore != _settingsStoreView;

        _settingsStoreView = requestedStore;
        SettingsPageIconText.Text = gameServerPage ? "\uE950" : "\uE713";
        SettingsPageTitleText.Text = gameServerPage ? "Gameserver Patch" : "Filter Settings";
        SettingsPageDescriptionText.Text = gameServerPage
            ? "Customer-editable GameServer configuration loaded once when GameServer starts"
            : "Database-backed filter configuration";
        SaveSettingsButtonText.Text = gameServerPage ? "Save Changes" : "Save Section";

        SettingsSectionsPanel.Visibility = Visibility.Visible;
        SettingsSectionsColumn.Width = new GridLength(280);
        SettingsSectionsSpacerColumn.Width = new GridLength(18);
        Grid.SetColumn(SettingsFormContainer, 2);
        Grid.SetColumnSpan(SettingsFormContainer, 1);

        if (!storeChanged)
            return;

        _selectedSettingsSection = gameServerPage ? "GameServer.Patch" : string.Empty;
        if (SettingsSearchBox.Text.Length > 0)
            SettingsSearchBox.Clear();
        else if (_allSettings.Count > 0)
            BuildSettingsSections();
    }

    private string GetSettingsStoreLabel() =>
        _settingsStoreView == SettingStore.GameServer ? "GameServer" : "Filter";

    private void ShowPage(string page)
    {
        var clientlessPageSelected = IsClientlessPage(page);
        var settingsPageSelected = page is "Settings" or "GameServerPatch";
        DashboardPage.Visibility = page == "Dashboard" ? Visibility.Visible : Visibility.Collapsed;
        ConnectionPage.Visibility = page == "Connection" ? Visibility.Visible : Visibility.Collapsed;
        SettingsPage.Visibility = settingsPageSelected ? Visibility.Visible : Visibility.Collapsed;
        PlayersPage.Visibility = page == "Players" ? Visibility.Visible : Visibility.Collapsed;
        PlayerCommandsPage.Visibility = page == "PlayerCommands" ? Visibility.Visible : Visibility.Collapsed;
        RewardsPage.Visibility = page == "Rewards" ? Visibility.Visible : Visibility.Collapsed;
        LuckySpinPage.Visibility = page == "LuckySpin" ? Visibility.Visible : Visibility.Collapsed;
        SpecialOffersPage.Visibility = page == "SpecialOffers" ? Visibility.Visible : Visibility.Collapsed;
        KillerAnimationsPage.Visibility = page == "KillerAnimations" ? Visibility.Visible : Visibility.Collapsed;
        VipSystemPage.Visibility = page == "VipSystem" ? Visibility.Visible : Visibility.Collapsed;
        EconomyLogsPage.Visibility = page == "EconomyLogs" ? Visibility.Visible : Visibility.Collapsed;
        AutoEventsPage.Visibility = page == "AutoEvents" ? Visibility.Visible : Visibility.Collapsed;
        SurvivalPartyPage.Visibility = page == "SurvivalParty" ? Visibility.Visible : Visibility.Collapsed;
        SurvivalSoloPage.Visibility = page == "SurvivalSolo" ? Visibility.Visible : Visibility.Collapsed;
        HideAndSeekPage.Visibility = page == "HideAndSeek" ? Visibility.Visible : Visibility.Collapsed;
        CompetitiveEventPage.Visibility = page is "LastManStanding" or "MadnessSolo" or "DefendTower" ? Visibility.Visible : Visibility.Collapsed;
        PvpChallengePage.Visibility = page == "PvpChallenge" ? Visibility.Visible : Visibility.Collapsed;
        RegionControlPage.Visibility = page == "RegionControl" ? Visibility.Visible : Visibility.Collapsed;
        SecurityPage.Visibility = page == "Security" ? Visibility.Visible : Visibility.Collapsed;
        UniqueRulesPage.Visibility = page == "UniqueRules" ? Visibility.Visible : Visibility.Collapsed;
        ChatLogsPage.Visibility = page == "ChatLogs" ? Visibility.Visible : Visibility.Collapsed;
        SchedulerPage.Visibility = page == "Scheduler" ? Visibility.Visible : Visibility.Collapsed;
        DiscordPage.Visibility = page == "Discord" ? Visibility.Visible : Visibility.Collapsed;
        TelegramPage.Visibility = page == "Telegram" ? Visibility.Visible : Visibility.Collapsed;
        WebViewerPage.Visibility = page == "WebViewer" ? Visibility.Visible : Visibility.Collapsed;
        PacketRulesPage.Visibility = page == "PacketRules" ? Visibility.Visible : Visibility.Collapsed;
        DataStudioPage.Visibility = page == "DataStudio" ? Visibility.Visible : Visibility.Collapsed;
        AuditPage.Visibility = page == "Audit" ? Visibility.Visible : Visibility.Collapsed;
        RuntimePage.Visibility = page == "Runtime" ? Visibility.Visible : Visibility.Collapsed;
        ClientlessPage.Visibility = clientlessPageSelected ? Visibility.Visible : Visibility.Collapsed;
        DiagnosticsPage.Visibility = page == "Diagnostics" ? Visibility.Visible : Visibility.Collapsed;
        MigrationsPage.Visibility = page == "Migrations" ? Visibility.Visible : Visibility.Collapsed;
        LogsPage.Visibility = page == "Logs" ? Visibility.Visible : Visibility.Collapsed;

        if (clientlessPageSelected)
            ConfigureClientlessPage(page);
        if (settingsPageSelected)
            ConfigureSettingsPage(page);

        HeaderTitleText.Text = page switch
        {
            "Connection" => "Connection",
            "Settings" => "Filter Settings",
            "GameServerPatch" => "Gameserver Patch",
            "Players" => "Players",
            "PlayerCommands" => "Player Commands",
            "Rewards" => "Rewards",
            "LuckySpin" => "Lucky Spin",
            "SpecialOffers" => "Special Offers",
            "KillerAnimations" => "Killer Animations",
            "VipSystem" => "VIP System",
            "EconomyLogs" => "Economy Logs",
            "AutoEvents" => "Auto Events",
            "SurvivalParty" => "Survival Party",
            "SurvivalSolo" => "Survival Solo",
            "HideAndSeek" => "Hide and Seek",
            "LastManStanding" => "Last Man Standing",
            "MadnessSolo" => "Madness Solo",
            "DefendTower" => "Defend The Tower",
            "PvpChallenge" => "PvP Challenge",
            "RegionControl" => "Region Control",
            "Security" => "Security Center",
            "UniqueRules" => "Unique Rules",
            "Scheduler" => "Scheduler",
            "Discord" => "Discord Notifications",
            "Telegram" => "Telegram Notifications",
            "ChatLogs" => "Chat History",
            "WebViewer" => "WebViewer Buttons",
            "PacketRules" => "Packet Rules",
            "DataStudio" => "Data Studio",
            "Audit" => "Audit Log",
            "Diagnostics" => "System Health",
            "Migrations" => "Database Updates",
            "Logs" => "Service Logs",
            "Runtime" => "Runtime Actions",
            "ClientlessOverview" => "Clientless Overview",
            "ClientlessRuntime" => "Run by City",
            "ClientlessProvisioning" => "Create Accounts",
            "ClientlessAccounts" => "Accounts & Status",
            "ClientlessHunting" => "Hunting Areas",
            "ClientlessParty" => "Party Matching",
            _ => "Dashboard"
        };

        HeaderSectionText.Text = page switch
        {
            "Dashboard" or "Connection" or "Players" => "WORKSPACE",
            "ClientlessOverview" or "ClientlessRuntime" or "ClientlessProvisioning" or "ClientlessAccounts" or "ClientlessHunting" or "ClientlessParty" => "BOT OPERATIONS",
            "Settings" or "GameServerPatch" or "Runtime" or "PlayerCommands" or "Security" or "PacketRules" => "OPERATIONS",
            "AutoEvents" or "SurvivalParty" or "SurvivalSolo" or "HideAndSeek" or "LastManStanding" or "MadnessSolo" or "DefendTower" => "EVENT AUTOMATION",
            "Rewards" or "LuckySpin" or "SpecialOffers" or "KillerAnimations" or "VipSystem" or "EconomyLogs" or "PvpChallenge" => "CONTENT & ECONOMY",
            "UniqueRules" or "RegionControl" or "Scheduler" or "WebViewer" => "SYSTEM GAME",
            "Discord" or "Telegram" => "NOTIFICATIONS",
            _ => "DATA & SYSTEM"
        };

        HeaderSubtitleText.Text = page switch
        {
            "Connection" => "Settings.json and SQL connectivity",
            "Settings" => "Database-backed filter configuration",
            "GameServerPatch" => "GameServer rules and patch configuration",
            "Players" => "Live sessions and character search",
            "PlayerCommands" => "Direct economy, movement and session commands by exact character name",
            "Rewards" => "Item chest rewards",
            "LuckySpin" => "Lucky Spin reward catalog",
            "SpecialOffers" => "Active shop offers",
            "KillerAnimations" => "Client animation catalog",
            "VipSystem" => "Silk spending tiers, rank icons and optional buffs",
            "EconomyLogs" => "Reward, shop and silk transactions",
            "AutoEvents" => "Event configuration and runtime queue",
            "SurvivalParty" => "Survival arena setup and rewards",
            "SurvivalSolo" => "Individual free-for-all arena setup and rewards",
            "HideAndSeek" => "Automated system character, hiding locations and winner rewards",
            "LastManStanding" => "Elimination arena setup and rewards",
            "MadnessSolo" => "Ranked free-for-all setup, mobs and rewards",
            "DefendTower" => "Team spawns, towers and rewards",
            "PvpChallenge" => "Arenas and challenge matches",
            "RegionControl" => "Region rules and restrictions",
            "Security" => "Chat blocks and HWID actions",
            "UniqueRules" => "Unique attack permissions and character restrictions",
            "Scheduler" => "Stored-procedure jobs",
            "Discord" => "Bot delivery, channel destinations and queued SQL notifications",
            "Telegram" => "Live alerts, event reminders and announcements",
            "ChatLogs" => "Searchable player conversations and support history",
            "WebViewer" => "In-client web buttons",
            "PacketRules" => "Named whitelist and blacklist entries",
            "DataStudio" => "Advanced, permission-aware database maintenance",
            "Audit" => "Recorded administrator actions",
            "Diagnostics" => "Feature readiness and required database objects",
            "Migrations" => "Review and apply packaged database updates",
            "Logs" => "Filter and service log files in one place",
            "Runtime" => "Service orchestration and live module telemetry",
            "ClientlessOverview" => "Account readiness and totals for every city",
            "ClientlessRuntime" => "Start, stop and reload one or several city groups",
            "ClientlessProvisioning" => "Create game-ready clientless accounts with a guided setup",
            "ClientlessAccounts" => "Filter, review and manage clientless accounts",
            "ClientlessHunting" => "Configure areas and monitor automated combat",
            "ClientlessParty" => "Configure automatic Party Matching listings",
            _ => "Live server and database status"
        };

        AnimatePageTransition();

        UpdateOnlinePlayersTimer();
        if (page == "SurvivalParty")
            _ = LoadSurvivalPartyAsync();
        else if (page == "SurvivalSolo")
            _ = LoadSurvivalSoloAsync();
        else if (page == "HideAndSeek")
            _ = LoadHideAndSeekAsync();
        else if (page == "Discord")
            _ = LoadDiscordAsync();
        else if (page == "Telegram")
            _ = LoadTelegramAsync();
        else if (page == "VipSystem" && _vipTiers.Count == 0)
            _ = LoadVipTiersAsync();
        else if (page == "ClientlessHunting")
            _ = LoadClientlessHuntingAsync();
        else if (IsClientlessPage(page))
            _ = LoadClientlessAsync();
        else if (page == "ChatLogs")
            _ = SearchChatLogsAsync();
        else if (page == "DataStudio")
            _ = LoadDataStudioAsync();
        else if (page == "Diagnostics")
            _ = LoadDiagnosticsAsync();
        else if (page == "Migrations")
            _ = CheckMigrationsAsync();
        else if (page == "Logs")
            LoadLogs();
        else if (page is "LastManStanding" or "MadnessSolo" or "DefendTower")
        {
            _competitiveEventCode = page switch { "LastManStanding" => "LMS", "MadnessSolo" => "MADNESS", _ => "DTT" };
            _ = LoadCompetitiveEventAsync();
        }

        if (page == "Connection" && _loadedSettings is not null && ProxyServicesConnectionPanel.Children.Count == 0)
            _ = LoadProxyServicesConnectionAsync();

        if (page == "Players")
            _ = RefreshOnlinePlayersAsync();
    }

    private string ReadSelectedPlayerLanguage()
    {
        return PlayerLanguageBox.SelectedItem is ComboBoxItem item
            ? NormalizePlayerLanguage(item.Tag?.ToString() ?? item.Content?.ToString())
            : "English";
    }

    private void SelectPlayerLanguage(string? language)
    {
        var normalized = NormalizePlayerLanguage(language);
        var matchingItem = PlayerLanguageBox.Items
            .OfType<ComboBoxItem>()
            .FirstOrDefault(item => string.Equals(
                item.Tag?.ToString(),
                normalized,
                StringComparison.OrdinalIgnoreCase));
        if (matchingItem == null)
        {
            matchingItem = new ComboBoxItem { Content = normalized, Tag = normalized };
            PlayerLanguageBox.Items.Add(matchingItem);
        }

        PlayerLanguageBox.SelectedItem = matchingItem;
        PlayerLanguageStatusText.Text = $"Current selection: {normalized}";
    }

    private static string NormalizePlayerLanguage(string? language)
    {
        var value = string.IsNullOrWhiteSpace(language) ? "English" : language.Trim();
        return value.ToLowerInvariant() switch
        {
            "en" or "en-us" or "en-gb" or "english" => "English",
            "tr" or "tr-tr" or "turkish" or "türkçe" => "Turkish",
            _ => value
        };
    }

    private void AnimatePageTransition()
    {
        if (PageHost is null || PageHostTransform is null)
            return;

        var easing = new CubicEase { EasingMode = EasingMode.EaseOut };
        PageHost.BeginAnimation(OpacityProperty, new DoubleAnimation(0.35, 1, TimeSpan.FromMilliseconds(150))
        {
            EasingFunction = easing
        });
        PageHostTransform.BeginAnimation(TranslateTransform.YProperty,
            new DoubleAnimation(7, 0, TimeSpan.FromMilliseconds(165)) { EasingFunction = easing });
    }

    private void UpdateOnlinePlayersTimer()
    {
        if (!_uiReady)
            return;

        var isPlayersPage = NavigationList.SelectedItem is ListBoxItem { Tag: string page } && page == "Players";
        if (isPlayersPage && OnlinePlayersAutoRefreshBox.IsChecked == true)
            _onlinePlayersTimer.Start();
        else
            _onlinePlayersTimer.Stop();
    }

    private static void BindTable(DataGrid grid, DataTable table)
    {
        grid.ItemsSource = table.DefaultView;
    }

    private static DataRowView? GetSelectedRow(DataGrid grid)
    {
        return grid.SelectedItem as DataRowView;
    }

    private string SaveSettingsForRuntime()
    {
        var path = SettingsPathBox.Text.Trim();
        if (string.IsNullOrWhiteSpace(path))
        {
            path = _settingsFileService.GetRuntimeSettingsPath();
            SettingsPathBox.Text = path;
        }

        var settings = ReadSettingsFromFields();
        _settingsFileService.Save(path, settings);
        _loadedSettings = settings.Clone();
        Environment.SetEnvironmentVariable("KMTGUARD_SETTINGS_PATH", path);
        RuntimeResultText.Text = $"Settings saved for KMTGuard engine: {path}";
        return path;
    }

    private static AdminSettings CreateDefaultConnectionSettings() => new()
    {
        Username = "sa",
        ProxyDb = "KMTGuard",
        MaximumPool = 1500,
        MinimumPool = 50,
        ConnectionLifetime = 0,
        Language = "English"
    };

    private async Task RunFilterProcessActionAsync(Func<Task<FilterProcessStatus>> action, string successMessage)
    {
        try
        {
            FilterProcessStatusText.Text = "Working...";
            FilterProcessDetailText.Text = "Applying the service action...";
            var status = await action();
            ApplyFilterProcessStatus(status);
            RefreshRuntimeLogStreams();
            SetStatus(successMessage, true);
        }
        catch (Exception ex)
        {
            FilterProcessDetailText.Text = ex.Message;
            RefreshRuntimeLogStreams();
            ShowError(ex.Message);
        }
    }

    private async void RefreshFilterProcessStatus()
    {
        try
        {
            ApplyFilterProcessStatus(await _filterRuntimeService.GetStatusAsync());
            if (RuntimePage.Visibility == Visibility.Visible)
                RefreshRuntimeLogStreams();
        }
        catch (Exception ex)
        {
            FilterProcessStatusText.Text = "Error";
            FilterProcessDetailText.Text = ex.Message;
            DashboardRuntimeStatusText.Text = "Unavailable";
            DashboardRuntimeDetailText.Text = ex.Message;
            DashboardRunningServicesText.Text = "0 / 3";
            TopRuntimeStatusText.Text = "Unavailable";
            var errorBrush = GetThemeBrush("RoseBrush", Brushes.IndianRed);
            DashboardRuntimeStatusDot.Fill = errorBrush;
            TopRuntimeStatusDot.Fill = errorBrush;
        }
    }

    private void ApplyFilterProcessStatus(FilterProcessStatus status)
    {
        var selectedRole = GetSelectedRuntimeRole();
        FilterProcessStatusText.Text = status.Message;
        var statusBrush = status.IsRunning
            ? GetThemeBrush("SuccessBrush", Brushes.ForestGreen)
            : status.AnyRunning
                ? GetThemeBrush("AmberBrush", Brushes.DarkGoldenrod)
                : GetThemeBrush("RoseBrush", Brushes.IndianRed);
        FilterProcessStatusText.Foreground = statusBrush;
        RuntimeServicesGrid.ItemsSource = status.Services;
        DashboardServiceList.ItemsSource = status.Services;
        RuntimeServicesGrid.SelectedItem = status.Services.FirstOrDefault(service => service.Role == selectedRole)
                                            ?? status.Services.FirstOrDefault();
        SetRuntimeChannelStatus(
            AgentRuntimeChannelStatus,
            status.Services.Any(service => service.Role == FilterRole.Agent && service.IsRunning),
            Color.FromRgb(217, 120, 88));
        SetRuntimeChannelStatus(
            DownloadRuntimeChannelStatus,
            status.Services.Any(service => service.Role == FilterRole.Download && service.IsRunning),
            Color.FromRgb(141, 165, 119));
        SetRuntimeChannelStatus(
            GatewayRuntimeChannelStatus,
            status.Services.Any(service => service.Role == FilterRole.Gateway && service.IsRunning),
            Color.FromRgb(214, 169, 84));

        var totalMemory = status.Services.Where(service => service.IsRunning).Sum(service => service.MemoryMb);
        var running = status.Services.Count(service => service.IsRunning);
        FilterProcessDetailText.Text =
            $"Processes: {running}/3 | Total working set: {totalMemory:N1} MB | " +
            "Startup order: Agent, Download, Gateway";
        var conciseStatus = status.IsRunning ? "Running" : status.AnyRunning ? "Partial" : "Stopped";
        DashboardRuntimeStatusText.Text = conciseStatus;
        TopRuntimeStatusText.Text = conciseStatus;
        DashboardRunningServicesText.Text = $"{running} / 3";
        DashboardRuntimeDetailText.Text =
            $"{running} of 3 services running. Total working set: {totalMemory:N1} MB.";
        DashboardRuntimeStatusDot.Fill = statusBrush;
        TopRuntimeStatusDot.Fill = statusBrush;

        if (MetricSqlStatus.Text == "Online")
        {
            DashboardSystemHeadlineText.Text = status.IsRunning
                ? "All core systems are operational"
                : status.AnyRunning
                    ? "Database online, engine services partially running"
                    : "Database online, engine services stopped";
        }
    }

    private void RefreshRuntimeLogStreams()
    {
        AgentRuntimeLogBox.Text = _filterRuntimeService.ReadRecentLogLines(FilterRole.Agent);
        DownloadRuntimeLogBox.Text = _filterRuntimeService.ReadRecentLogLines(FilterRole.Download);
        GatewayRuntimeLogBox.Text = _filterRuntimeService.ReadRecentLogLines(FilterRole.Gateway);
        AgentRuntimeLogBox.ScrollToEnd();
        DownloadRuntimeLogBox.ScrollToEnd();
        GatewayRuntimeLogBox.ScrollToEnd();
    }

    private static void SetRuntimeChannelStatus(TextBlock label, bool isRunning, Color accent)
    {
        label.Text = isRunning ? " / LIVE" : " / IDLE";
        label.Foreground = new SolidColorBrush(isRunning ? accent : Color.FromRgb(145, 135, 123));
    }

    private void SetRegionChecks(bool value)
    {
        RegionTeleportBox.IsChecked = value;
        RegionReverseBox.IsChecked = value;
        RegionTraceBox.IsChecked = value;
        RegionMoveBox.IsChecked = value;
        RegionChatBox.IsChecked = value;
        RegionGlobalBox.IsChecked = value;
        RegionPartyBox.IsChecked = value;
        RegionExchangeBox.IsChecked = value;
        RegionStallBox.IsChecked = value;
        RegionPvpBox.IsChecked = value;
        RegionAlchemyBox.IsChecked = value;
        RegionSpecialItemsBox.IsChecked = value;
        RegionZerkBox.IsChecked = value;
    }

    private static bool ReadBool(DataRowView row, string columnName, bool fallback)
    {
        if (!row.Row.Table.Columns.Contains(columnName) || row[columnName] == DBNull.Value)
            return fallback;

        return Convert.ToBoolean(row[columnName]);
    }

    private static int ReadInt(DataRowView row, string columnName, int fallback)
    {
        if (!row.Row.Table.Columns.Contains(columnName) || row[columnName] == DBNull.Value)
            return fallback;

        return int.TryParse(Convert.ToString(row[columnName]), out var value) ? value : fallback;
    }

    private static string ReadColumnText(DataRowView row, string columnName, string fallback)
    {
        if (!row.Row.Table.Columns.Contains(columnName) || row[columnName] == DBNull.Value)
            return fallback;

        return Convert.ToString(row[columnName]) ?? fallback;
    }

    private void SetStatus(string message, bool ok)
    {
        var statusBrush = ok
            ? GetThemeBrush("SuccessBrush", Brushes.ForestGreen)
            : GetThemeBrush("RoseBrush", Brushes.IndianRed);
        SidebarStatusText.Text = message;
        SidebarStatusText.Foreground = statusBrush;
        SidebarStatusDot.Fill = statusBrush;
        GlobalStatusDot.Fill = statusBrush;
        StatusTimestampText.Text = DateTime.Now.ToString("HH:mm:ss");
    }

    private Brush GetThemeBrush(string key, Brush fallback)
    {
        return TryFindResource(key) as Brush ?? fallback;
    }

    private void ShowError(string message)
    {
        SetStatus(message, false);
        MessageBox.Show(this, message, "KMTGuard Admin Desktop", MessageBoxButton.OK, MessageBoxImage.Error);
    }

    private static int ParseInt(string value, int fallback)
    {
        return int.TryParse(value.Trim(), out var parsed) ? parsed : fallback;
    }

    private static int ParseRequiredInt(string value, string label)
    {
        if (!int.TryParse(value.Trim(), out var parsed))
            throw new InvalidOperationException($"{label} must be a number.");

        return parsed;
    }

    private static byte ParseRequiredByte(string value, string label)
    {
        var parsed = ParseRequiredInt(value, label);
        if (parsed is < byte.MinValue or > byte.MaxValue)
            throw new InvalidOperationException($"{label} must be between {byte.MinValue} and {byte.MaxValue}.");

        return (byte)parsed;
    }

    private static long ParseLong(string value, long fallback)
    {
        return long.TryParse(value.Trim(), out var parsed) ? parsed : fallback;
    }

    private static int? ParseOptionalInt(string value)
    {
        return int.TryParse(value.Trim(), out var parsed) && parsed > 0 ? parsed : null;
    }

    private static string ReadComboText(ComboBox comboBox)
    {
        return comboBox.SelectedItem is ComboBoxItem item
            ? Convert.ToString(item.Content) ?? string.Empty
            : Convert.ToString(comboBox.Text) ?? string.Empty;
    }

    private static int ReadPaymentType(ComboBox comboBox)
    {
        if (comboBox.SelectedItem is ComboBoxItem item && int.TryParse(Convert.ToString(item.Tag), out var tagValue))
            return tagValue;

        var text = ReadComboText(comboBox);
        return text.Equals("Gold", StringComparison.OrdinalIgnoreCase) ? 1 : 0;
    }

    private static int ReadTaggedValue(ComboBox comboBox, int fallback)
    {
        return comboBox.SelectedItem is ComboBoxItem item &&
               int.TryParse(Convert.ToString(item.Tag), out var value)
            ? value
            : fallback;
    }

    private static string ReadComboTag(ComboBox comboBox, string fallback)
    {
        if (comboBox.SelectedItem is ComboBoxItem item)
        {
            var value = item.Tag?.ToString();
            if (!string.IsNullOrWhiteSpace(value))
                return value;
        }

        return fallback;
    }

    private static void SetComboTag(ComboBox comboBox, string? value, string fallback)
    {
        var expected = string.IsNullOrWhiteSpace(value) ? fallback : value;
        foreach (var item in comboBox.Items.OfType<ComboBoxItem>())
        {
            if (string.Equals(Convert.ToString(item.Tag), expected, StringComparison.OrdinalIgnoreCase))
            {
                comboBox.SelectedItem = item;
                return;
            }
        }

        foreach (var item in comboBox.Items.OfType<ComboBoxItem>())
        {
            if (string.Equals(Convert.ToString(item.Tag), fallback, StringComparison.OrdinalIgnoreCase))
            {
                comboBox.SelectedItem = item;
                return;
            }
        }

        comboBox.SelectedIndex = 0;
    }

    private static void SetTaggedValue(ComboBox comboBox, int value)
    {
        foreach (var item in comboBox.Items.OfType<ComboBoxItem>())
        {
            if (int.TryParse(Convert.ToString(item.Tag), out var tagValue) && tagValue == value)
            {
                comboBox.SelectedItem = item;
                return;
            }
        }

        comboBox.SelectedIndex = 0;
    }

    private static void SetPaymentType(ComboBox comboBox, int value)
    {
        foreach (var item in comboBox.Items.OfType<ComboBoxItem>())
        {
            if (int.TryParse(Convert.ToString(item.Tag), out var tagValue) && tagValue == value)
            {
                comboBox.SelectedItem = item;
                return;
            }
        }

        comboBox.SelectedIndex = 0;
    }

    private static void SetComboText(ComboBox comboBox, string value)
    {
        foreach (var item in comboBox.Items.OfType<ComboBoxItem>())
        {
            if (string.Equals(Convert.ToString(item.Content), value, StringComparison.OrdinalIgnoreCase))
            {
                comboBox.SelectedItem = item;
                return;
            }
        }

        comboBox.Text = value;
    }

    private static int ParseMsgId(string value)
    {
        var trimmed = value.Trim();
        if (trimmed.StartsWith("0x", StringComparison.OrdinalIgnoreCase))
            return Convert.ToInt32(trimmed[2..], 16);

        return ParseRequiredInt(trimmed, "MsgId");
    }

    private static int? TryParseNullableInt(string value)
    {
        return int.TryParse(value.Trim(), out var parsed) && parsed > 0 ? parsed : null;
    }

    private static IReadOnlyDictionary<string, SectionDefinition> BuildSectionDefinitions()
    {
        var sections = new[]
        {
            new SectionDefinition("Core.Database", "Database", "Database names used by the filter. Keep these aligned with your server databases.", 10),
            new SectionDefinition("Gateway.Login", "Gateway & Login", "Login, register, captcha, server list, and secondary password behavior.", 20),
            new SectionDefinition("Client.MenuButtons", "Menu Buttons", "Feature buttons shown inside the custom client menu.", 30),
            new SectionDefinition("Client.GuideIcons", "Guide Icons", "Small guide/minimap icon visibility switches.", 40),
            new SectionDefinition("Client.UI", "Client UI", "Client visual patches and quality-of-life features.", 50),
            new SectionDefinition("Client.Links", "Social Links", "Website, Discord, and Facebook URLs opened by client buttons.", 60),
            new SectionDefinition("Client.ItemTranslation", "Item Translation", "Pricing and currency for item translation.", 70),
            new SectionDefinition("Client.Macro", "Macro", "Custom macro support and guide visibility.", 75),
            new SectionDefinition("LuckySpin", "Lucky Spin", "Lucky Spin feature switch, price, and payment mode.", 80),
            new SectionDefinition("SpecialOffers", "Special Offers", "Special offers shop feature visibility.", 85),
            new SectionDefinition("Security.Limits", "Security Limits", "IP, HWID, alchemy and gameplay protection limits.", 90),
            new SectionDefinition("Gameplay.Delays", "Gameplay Delays", "Cooldowns and level restrictions for player actions.", 100),
            new SectionDefinition("GameServer.Patch", "Gameserver Patch", "All customer-editable GameServer runtime settings in one place. Save the values, then restart every GameServer to apply them.", 104),
            new SectionDefinition("GameServer.Trade", "Trade System", "GameServer controls for the original Silkroad trade reward flow. Changes require a GameServer restart.", 105),
            new SectionDefinition("Trade.Captcha", "Trade Captcha", "Trade goods selling verification protection.", 110),
            new SectionDefinition("Unsorted", "Other Settings", "Known settings that do not have a dedicated screen yet.", 999)
        };

        return sections.ToDictionary(section => section.Key, StringComparer.OrdinalIgnoreCase);
    }

    private static IReadOnlyDictionary<string, SettingDefinition> BuildSettingDefinitions()
    {
        static SettingDefinition D(
            string name,
            string section,
            string label,
            string help,
            SettingKind kind,
            int order,
            long? minimum = null,
            long? maximum = null) =>
            new(name, section, label, help, kind, order, minimum, maximum);

        var definitions = new[]
        {
            D("AccountDB", "Core.Database", "Account database", "Database that contains accounts and silk balance.", SettingKind.Text, 10),
            D("ShardDB", "Core.Database", "Shard database", "Main shard database used for characters, items, and refs.", SettingKind.Text, 20),
            D("LogDB", "Core.Database", "Log database", "Shard log database that contains _LogEventItem drop records.", SettingKind.Text, 30),

            D("RemoveCaptcha", "Gateway.Login", "Remove login captcha", "Skips the normal captcha flow when enabled.", SettingKind.Boolean, 10),
            D("CaptchaValue", "Gateway.Login", "Captcha value", "Default captcha answer/value used by the gateway flow.", SettingKind.Text, 20),
            D("SecondaryPassword", "Gateway.Login", "Secondary password", "Enable account secondary-password protection.", SettingKind.Boolean, 30),
            D("NewCharInfo", "Gateway.Login", "New character info", "Use the custom character info UI.", SettingKind.Boolean, 40),
            D("NewIdPw", "Gateway.Login", "New ID/password panel", "Use the new login ID/password client panel.", SettingKind.Boolean, 50),
            D("OldLogin", "Gateway.Login", "Old login UI", "Use the legacy login interface.", SettingKind.Boolean, 60),
            D("CheckStatus", "Gateway.Login", "Check server status", "Show server status through the gateway response.", SettingKind.Boolean, 70),
            D("ShowOnlinePlayers", "Gateway.Login", "Show online players", "Display online player count in the client.", SettingKind.Boolean, 80),
            D("ServerName", "Gateway.Login", "Server name", "Friendly server name shown in client UI.", SettingKind.Text, 90),
            D("FakePlayerCount", "Gateway.Login", "Fake player count", "Extra visible count added to online players.", SettingKind.Number, 100),
            D("EnableQuickLogin", "Gateway.Login", "Quick login", "Enable secure quick-login tokens.", SettingKind.Boolean, 110),

            D("GrantNameButton", "Client.MenuButtons", "Grant name button", "Show the grant-name button.", SettingKind.Boolean, 10),
            D("IconManagerButton", "Client.MenuButtons", "Icon manager button", "Show icon manager entry.", SettingKind.Boolean, 20),
            D("IconManagerRight", "Client.MenuButtons", "Right-side icon manager", "Enable right-side icon manager features.", SettingKind.Boolean, 30),
            D("TitleManager", "Client.MenuButtons", "Title manager", "Show title manager entry.", SettingKind.Boolean, 40),
            D("TitleManagerColor", "Client.MenuButtons", "Title color manager", "Enable title color management.", SettingKind.Boolean, 50),
            D("DynamicRanking", "Client.MenuButtons", "Dynamic ranking", "Show ranking page in the custom menu.", SettingKind.Boolean, 60),
            D("UniqueHistory", "Client.MenuButtons", "Unique history", "Show unique-kill history.", SettingKind.Boolean, 70),
            D("EventRegister", "Client.MenuButtons", "Event registration", "Show event register button.", SettingKind.Boolean, 80),
            D("EventSchedule", "Client.MenuButtons", "Event schedule", "Show event schedule button.", SettingKind.Boolean, 90),
            D("Achievements", "Client.MenuButtons", "Achievements", "Show achievements button.", SettingKind.Boolean, 100),
            D("Changelog", "Client.MenuButtons", "Changelog", "Enable changelog window.", SettingKind.Boolean, 110),
            D("ShowChangelogFirstSpawn", "Client.MenuButtons", "Show changelog on first spawn", "Open changelog when a character first enters.", SettingKind.Boolean, 120),

            D("ShowGuideMenu", "Client.GuideIcons", "Guide menu", "Show the main guide icon/menu.", SettingKind.Boolean, 10),
            D("ShowGuideLuckySpin", "Client.GuideIcons", "Lucky Spin icon", "Show Lucky Spin guide icon.", SettingKind.Boolean, 20),
            D("ShowGuideItemChest", "Client.GuideIcons", "Item chest icon", "Show item chest guide icon.", SettingKind.Boolean, 30),
            D("ShowGuideDropLogs", "Client.GuideIcons", "Drop logs icon", "Show drop logs guide icon.", SettingKind.Boolean, 40),
            D("ShowGuideMacro", "Client.GuideIcons", "Macro icon", "Show macro guide icon.", SettingKind.Boolean, 50),
            D("ShowGuideDailyLogin", "Client.GuideIcons", "Daily login icon", "Show attendance/daily login icon.", SettingKind.Boolean, 60),
            D("ShowGuideDiscord", "Client.GuideIcons", "Discord icon", "Show Discord guide icon.", SettingKind.Boolean, 70),
            D("ShowGuideWebsite", "Client.GuideIcons", "Website icon", "Show website guide icon.", SettingKind.Boolean, 80),
            D("ShowGuideFacebook", "Client.GuideIcons", "Facebook icon", "Show Facebook guide icon.", SettingKind.Boolean, 90),
            D("ShowGuideAutoEquip", "Client.GuideIcons", "Auto equip icon", "Show auto-equip guide icon.", SettingKind.Boolean, 100),
            D("AutoEquipMaxLevel", "Client.GuideIcons", "Auto equip maximum level", "Highest player level allowed to see and use Auto Equip. Use 0 for no level limit.", SettingKind.Number, 101),
            D("ShowGuideWebViewer", "Client.GuideIcons", "Web viewer icon", "Show web viewer guide icon.", SettingKind.Boolean, 110),
            D("ShowGuideMapLocation", "Client.GuideIcons", "Map location icon", "Show map location guide icon.", SettingKind.Boolean, 120),
            D("ShowGuideSpecialOffers", "Client.GuideIcons", "Special Offers icon", "Show special offers guide icon.", SettingKind.Boolean, 130),
            D("ShowGuidePvpChallenge", "Client.GuideIcons", "PvP Challenge icon", "Show PvP challenge guide icon.", SettingKind.Boolean, 140),
            D("ShowGuideKillerAnimation", "Client.GuideIcons", "Killer Animation icon", "Show killer animation guide icon.", SettingKind.Boolean, 150),

            D("OldExpBar", "Client.UI", "Old EXP bar", "Use the old experience bar layout.", SettingKind.Boolean, 10),
            D("OldAlchemy", "Client.UI", "Old alchemy", "Use legacy alchemy UI.", SettingKind.Boolean, 20),
            D("OldMainPopup", "Client.UI", "Old main popup", "Use the legacy main popup.", SettingKind.Boolean, 30),
            D("HideTitleWhileTagActive", "Client.UI", "Hide title while tag is active", "Hide the player title while a player tag is active.", SettingKind.Boolean, 40),
            D("ItemComparison", "Client.UI", "Item comparison", "Enable item comparison UI.", SettingKind.Boolean, 50),
            D("AutoSort", "Client.UI", "Auto sort", "Enable inventory auto-sort.", SettingKind.Boolean, 60),
            D("AutoSkillUpdate", "Client.UI", "Auto skill update", "Enable automatic skill update behavior.", SettingKind.Boolean, 70),
            D("MasteryLimit", "Client.UI", "Mastery limit", "Maximum allowed mastery level.", SettingKind.Number, 80),
            D("ServerMaxLevel", "Client.UI", "Server max level", "Maximum character level shown/enforced by the UI.", SettingKind.Number, 90),
            D("FixDamageText", "Client.UI", "Fix damage text", "Enable damage text display fix.", SettingKind.Boolean, 100),
            D("AutoStrInt", "Client.UI", "Auto STR/INT", "Enable auto stat distribution support.", SettingKind.Boolean, 110),
            D("PickupEffect", "Client.UI", "Pickup effect", "Enable item pickup visual effect.", SettingKind.Boolean, 120),
            D("PermanentAlchemy", "Client.UI", "Permanent alchemy", "Enable permanent alchemy behavior.", SettingKind.Boolean, 130),
            D("ShowGuildInJobMode", "Client.UI", "Show guild in job mode", "Show guild identity while in job mode.", SettingKind.Boolean, 140),
            D("UniqueTarget", "Client.UI", "Unique target", "Enable unique target UI feature.", SettingKind.Boolean, 150),
            D("SecondarySlot", "Client.UI", "Secondary slot", "Enable secondary slot UI.", SettingKind.Boolean, 160),
            D("MoveSkillBoard", "Client.UI", "Move skill board", "Allow custom skill-board placement.", SettingKind.Boolean, 170),
            D("ServerInfoSkill", "Client.UI", "Server info skill", "Enable server info skill panel.", SettingKind.Boolean, 180),
            D("FixNewJobSuit", "Client.UI", "Fix new job suit", "Enable new job suit fix.", SettingKind.Boolean, 190),
            D("OldItemMall", "Client.UI", "Old item mall", "Use the legacy item mall.", SettingKind.Boolean, 200),
            D("InsertCommaPrices", "Client.UI", "Comma prices", "Format prices with commas.", SettingKind.Boolean, 210),
            D("WriteCharacterBound", "Client.UI", "Character-bound text", "Show character-bound item text.", SettingKind.Boolean, 220),
            D("NewItemMall", "Client.UI", "New item mall", "Enable custom item mall.", SettingKind.Boolean, 230),
            D("Menu-like-maxi", "Client.UI", "Menu like Maxi", "Use the Maxi-style main menu; disable it to keep the legacy menu.", SettingKind.Boolean, 235),
            D("MenuCasy", "Client.UI", "Menu CASY", "Use the compact transparent CASY main menu.", SettingKind.Boolean, 236),
            D("EmojiSystem", "Client.UI", "Emoji system", "Enable emoji support.", SettingKind.Boolean, 240),
            D("NewPartyMatch", "Client.UI", "New party match", "Enable custom party matching UI.", SettingKind.Boolean, 250),
            D("NewJobUI", "Client.UI", "New job UI", "Enable new job interface.", SettingKind.Boolean, 260),
            D("NewAlchemy", "Client.UI", "New alchemy", "Enable new alchemy UI.", SettingKind.Boolean, 270),
            D("PartyMemberViewer", "Client.UI", "Party member viewer", "Enable party member viewer.", SettingKind.Boolean, 280),
            D("NonClosePTForm", "Client.UI", "Keep party form open", "Prevent the party form from closing automatically.", SettingKind.Boolean, 290),

            D("FacebookURL", "Client.Links", "Facebook URL", "Facebook page opened by the client button.", SettingKind.Url, 10),
            D("DiscordURL", "Client.Links", "Discord URL", "Discord invite or community URL.", SettingKind.Url, 20),
            D("WebsiteURL", "Client.Links", "Website URL", "Main website URL opened by the client button.", SettingKind.Url, 30),

            D("EnableItemTranslation", "Client.ItemTranslation", "Item translation", "Enable item translation feature.", SettingKind.Boolean, 10),
            D("ItemTranslationPayment", "Client.ItemTranslation", "Payment type", "Currency/payment type for item translation.", SettingKind.Number, 20),
            D("ItemTranslationPrice", "Client.ItemTranslation", "Price", "Price charged for item translation.", SettingKind.Number, 30),
            D("Macro", "Client.Macro", "Macro feature", "Enable the custom macro feature.", SettingKind.Boolean, 10),

            D("EnableLuckySpin", "LuckySpin", "Lucky Spin", "Enable Lucky Spin.", SettingKind.Boolean, 10),
            D("EnableLuckySpinSilk", "LuckySpin", "Use silk payment", "Charge silk for spins when enabled.", SettingKind.Boolean, 20),
            D("LuckySpinPrice", "LuckySpin", "Spin price", "Price per spin.", SettingKind.Number, 30),
            D("EnableSpecialOffers", "SpecialOffers", "Special Offers shop", "Enable the special offers shop.", SettingKind.Boolean, 10),

            D("IPLimit", "Security.Limits", "IP limit", "Maximum allowed sessions per IP.", SettingKind.Number, 10),
            D("HWID_LIMIT", "Security.Limits", "HWID limit per PC", "Maximum allowed sessions per HWID.", SettingKind.Number, 20),
            D("HWID_JOB_LIMIT", "Security.Limits", "HWID job limit", "Maximum job-mode sessions per HWID.", SettingKind.Number, 30),
            D("AlchemyItemLinkMinLevel", "Security.Limits", "Alchemy link min level", "Minimum item level required for alchemy item link.", SettingKind.Number, 40),
            D("MaxPlus", "Security.Limits", "Max plus", "Maximum normal item plus.", SettingKind.Number, 50),
            D("MaxPlusDevil", "Security.Limits", "Max devil plus", "Maximum devil/spirit plus.", SettingKind.Number, 60),
            D("DisableAcademy", "Security.Limits", "Disable academy", "Disable academy system.", SettingKind.Boolean, 70),
            D("DisableAutoAttack", "Security.Limits", "Disable auto attack", "Disable auto attack above configured level.", SettingKind.Boolean, 90),
            D("AutoAttackMaxLevel", "Security.Limits", "Auto attack max level", "Level threshold for auto attack restriction.", SettingKind.Number, 100),
            D("DisableTraceWhileJob", "Security.Limits", "Disable trace in job", "Block trace while in job mode.", SettingKind.Boolean, 110),
            D("DisableReverseInJob", "Security.Limits", "Disable reverse in job", "Block reverse scroll while in job mode.", SettingKind.Boolean, 120),

            D("ReverseDelay", "Gameplay.Delays", "Reverse delay", "Cooldown for reverse scroll usage.", SettingKind.Number, 10),
            D("StallDelay", "Gameplay.Delays", "Stall delay", "Cooldown before stall actions.", SettingKind.Number, 20),
            D("StallLevel", "Gameplay.Delays", "Stall minimum level", "Minimum level required for stall actions.", SettingKind.Number, 30),
            D("ExchangeDelay", "Gameplay.Delays", "Exchange delay", "Cooldown before exchange actions.", SettingKind.Number, 40),
            D("ExchangeLevel", "Gameplay.Delays", "Exchange minimum level", "Minimum level required for exchange.", SettingKind.Number, 50),
            D("GuildInviteDelay", "Gameplay.Delays", "Guild invite delay", "Cooldown for guild invite actions.", SettingKind.Number, 60),
            D("UnionInviteDelay", "Gameplay.Delays", "Union invite delay", "Cooldown for union invite actions.", SettingKind.Number, 70),
            D("GlobalDelay", "Gameplay.Delays", "Global chat delay", "Cooldown for global chat.", SettingKind.Number, 80),
            D("GlobalLevel", "Gameplay.Delays", "Global chat level", "Minimum level required for global chat.", SettingKind.Number, 90),
            D("LiveItemDelay", "Gameplay.Delays", "Live item delay", "Cooldown for live item actions.", SettingKind.Number, 100),
            D("TradePetSpawnDelay", "Gameplay.Delays", "Trade pet spawn delay", "Cooldown for trade pet spawning.", SettingKind.Number, 110),
            D("RestartDelay", "Gameplay.Delays", "Restart delay", "Restart delay in seconds.", SettingKind.Number, 120),
            D("ExitDelay", "Gameplay.Delays", "Exit delay", "Exit/logout delay in seconds.", SettingKind.Number, 130),
            D("SHOW_CHAR_INFO_DELAY", "Gameplay.Delays", "Character info delay", "Cooldown for character info requests.", SettingKind.Number, 140),

            D("EnableTradeSellCaptcha", "Trade.Captcha", "Trade sell captcha", "Enable captcha protection for trade goods selling.", SettingKind.Boolean, 10),
            D("TradeSellCaptchaTimeoutSeconds", "Trade.Captcha", "Captcha timeout", "Seconds before the trade captcha expires.", SettingKind.Number, 20),
            D("TradeSellCaptchaMaxAttempts", "Trade.Captcha", "Max attempts", "Maximum failed captcha attempts.", SettingKind.Number, 30),
            D("EnablePvpChallenge", "Unsorted", "PvP Challenge", "Enable PvP Challenge system.", SettingKind.Boolean, 10)
        };

        return definitions.ToDictionary(definition => definition.Name, StringComparer.OrdinalIgnoreCase);
    }

    private static IReadOnlyDictionary<string, SettingDefinition> BuildGameServerSettingDefinitions()
    {
        static SettingDefinition D(
            string name,
            string label,
            string help,
            SettingKind kind,
            int order,
            long? minimum = null,
            long? maximum = null) =>
            new(name, "GameServer.Patch", label, help, kind, order, minimum, maximum);

        static SettingDefinition T(
            string name,
            string label,
            string help,
            SettingKind kind,
            int order) =>
            new(name, "GameServer.Trade", label, help, kind, order);

        var definitions = new[]
        {
            T("DisableOriginalTradeGold", "Disable Original Trade Gold", "When enabled, original Silkroad trade gold reward is disabled. KMTGuard custom trade rewards can be used instead. Changes require GameServer restart.", SettingKind.Boolean, 10),

            D("SERVER_MAX_LEVEL", "Maximum character level", "Highest character level supported by the GameServer.", SettingKind.Number, 10, 1, 255),
            D("CH_MAX_MASTERY_LEVEL", "Chinese mastery total", "Maximum combined mastery level for Chinese characters.", SettingKind.Number, 20, 1, 10000),
            D("EU_MAX_MASTERY_LEVEL", "European mastery total", "Maximum combined mastery level for European characters.", SettingKind.Number, 30, 1, 10000),
            D("MIN_PK_LEVEL", "Minimum PK level", "Minimum character level allowed to attack another player through the normal PK system.", SettingKind.Number, 40, 1, 255),
            D("PENALTY_DROP_LEVEL_MIN", "PK drop-penalty minimum level", "Minimum character level at which the PK item-drop penalty can apply.", SettingKind.Number, 50, 1, 255),
            D("PENALTY_DROP_PROBABILITY", "PK item-drop chance (%)", "Chance from 0 to 100 that the PK item-drop penalty drops an item.", SettingKind.Number, 60, 0, 100),
            D("STALL_EXCHANGE_GOLD_LIMIT", "Stall and exchange gold limit", "Maximum stall price. Exchange is limited by the original client protocol to 4,000,000,000 gold.", SettingKind.Number, 70, 1, 1099511627775),
            D("HIGH_RATES_CONFIG", "High-rate server support", "Enable the GameServer patches required for rates above the original limits.", SettingKind.Boolean, 80),
            D("PARTY_LEVEL_MIN", "Minimum party creation level", "Minimum character level required to create a party.", SettingKind.Number, 90, 1, 255),
            D("RESURRECT_SAME_POINT_LEVEL_MAX", "Same-place resurrection max level", "Highest level that may resurrect at the same location.", SettingKind.Number, 100, 1, 255),
            D("NPC_RETURN_DEAD_LEVEL_MAX", "NPC return-to-death max level", "Highest level allowed to use the NPC return-to-last-death option.", SettingKind.Number, 110, 1, 255),
            D("BEGINNER_MARK_LEVEL_MAX", "Beginner mark max level", "Highest level that displays the beginner mark.", SettingKind.Number, 120, 1, 255),
            D("DROP_ITEM_MAGIC_PROBABILITY", "Magic-option drop chance (%)", "Chance from 0 to 100 for dropped items to include magic options.", SettingKind.Number, 130, 0, 100),

            D("DisableDurability", "Disable durability loss", "Prevent weapons and equipment from losing durability.", SettingKind.Boolean, 200),
            D("DisableGreenBook", "Disable Green Book restrictions", "Disable the original Green Book fatigue restrictions.", SettingKind.Boolean, 210),
            D("ShowGmUniqueKillNotice", "Show Unique kill notice for GMs", "Broadcast the normal Unique kill notice when a Game Master delivers the killing blow.", SettingKind.Boolean, 212),
            D("ForceGmVisibleOnSpawn", "Force GMs visible when spawning", "Clear the GM Invisible state before a Game Master character enters the world.", SettingKind.Boolean, 214),
            D("FIX_EXPLOIT_INVISIBLE_INVINCIBLE", "Block invisible/invincible exploit", "Enable the GameServer protection against the invalid invisible and invincible state exploit.", SettingKind.Boolean, 220),
            D("FIX_AGENT_SERVER_CAPACITY", "Agent capacity limit", "Maximum AgentServer capacity accepted by the GameServer patch.", SettingKind.Number, 230, 1, 100000),
            D("EXCHANGE_ATTACK_CANCEL", "Keep attacks active during exchange", "Prevent an exchange request from cancelling the current attack action.", SettingKind.Boolean, 240),
            D("GUILD_POINTS", "Protect guild points from negative values", "Prevent guild-point overflow from producing negative values. Restart both GameServer and ShardManager after changing this option.", SettingKind.Boolean, 250),
            D("FIX_GRAP_PET_PAGE", "Fix grab-pet inventory page", "Enable the inventory-page correction for grab pets.", SettingKind.Boolean, 260),
            D("GRAP_PET_INVENTORY_SIZE", "Grab-pet inventory size", "Number of inventory slots available to grab pets.", SettingKind.Number, 270, 1, 255),

            D("EnablePartyMonsterSpawn", "Enable Party Monster spawning", "Enable or disable Party Monster spawning. The member and chance values remain saved while disabled.", SettingKind.Boolean, 300),
            D("PartyMonsterMinimumMembers", "Party Monster minimum members", "Minimum party-member count required before Party Monsters can spawn.", SettingKind.Number, 310, 1, 9),
            D("PartyMonsterSpawnRate", "Party Monster spawn chance (%)", "Chance from 0 to 100 for an eligible monster to spawn as a Party Monster.", SettingKind.Number, 320, 0, 100),

            D("MEMBERS_LIMIT_LEVEL1", "Guild members at level 1", "Maximum guild members when the guild is level 1.", SettingKind.Number, 400, 1, 1000),
            D("MEMBERS_LIMIT_LEVEL2", "Guild members at level 2", "Maximum guild members when the guild is level 2.", SettingKind.Number, 410, 1, 1000),
            D("MEMBERS_LIMIT_LEVEL3", "Guild members at level 3", "Maximum guild members when the guild is level 3.", SettingKind.Number, 420, 1, 1000),
            D("MEMBERS_LIMIT_LEVEL4", "Guild members at level 4", "Maximum guild members when the guild is level 4.", SettingKind.Number, 430, 1, 1000),
            D("MEMBERS_LIMIT_LEVEL5", "Guild members at level 5", "Maximum guild members when the guild is level 5.", SettingKind.Number, 440, 1, 1000),
            D("STORAGE_SLOTS_MIN", "Guild storage starting slots", "Guild storage slots available at the first storage level.", SettingKind.Number, 450, 1, 1000),
            D("STORAGE_SLOTS_INCREASE", "Guild storage slots per level", "Additional guild storage slots granted at each following level.", SettingKind.Number, 460, 0, 1000),
            D("UNION_LIMIT", "Guild union limit", "Maximum number of guilds allowed in one union.", SettingKind.Number, 470, 1, 255),
            D("UNION_CHAT_PARTICIPANTS", "Union chat participants per guild", "Maximum members from each guild allowed to participate in union chat.", SettingKind.Number, 480, 1, 255),
            D("MIN_GUILD_LEVEL_FOR_MERCENARY_SPAWN", "Mercenary guild level", "Minimum guild level required to summon a guild mercenary.", SettingKind.Number, 490, 1, 255),
            D("ALLOW_NON_GM_MERCENARY_SPAWN", "Allow non-master mercenary summon", "Allow guild members other than the guild master to summon guild mercenaries.", SettingKind.Boolean, 500),
            D("DISABLE_GRANT_NAME_CONDITIONS", "Disable guild-name grant requirements", "Allow guild and grant-name actions without the original guild-level and guild-master requirements.", SettingKind.Boolean, 510),

            D("ALCHEMY_FUSING_DELAY", "Alchemy fusing delay", "Delay applied after an alchemy fuse attempt.", SettingKind.Number, 600, 0, 255),
            D("MinItemLevelForAstralToTakeEffect", "Astral activation plus level", "Minimum item plus level at which Astral protection begins to take effect.", SettingKind.Number, 610, 0, 255),
            D("ItemLevelForAstralRecovery", "Astral recovery plus level", "Item plus level restored after a protected alchemy failure.", SettingKind.Number, 620, 0, 255),

            D("CTF_ITEM_WIN_REWARD", "CTF winner reward item", "Server item code name awarded for winning Capture the Flag.", SettingKind.Text, 700),
            D("CTF_ITEM_WIN_REWARD_AMOUNT", "CTF winner reward amount", "Number of reward items awarded to a Capture the Flag winner.", SettingKind.Number, 710, 0, 255),
            D("CTF_ITEM_KILL_REWARD", "CTF kill reward item", "Server item code name awarded for a Capture the Flag kill.", SettingKind.Text, 720),
            D("CTF_ITEM_KILL_REWARD_AMOUNT", "CTF kill reward amount", "Number of reward items awarded for a Capture the Flag kill.", SettingKind.Number, 730, 0, 255),
            D("BA_ITEM_REWARD", "Battle Arena reward item", "Server item code name used for Battle Arena rewards.", SettingKind.Text, 740),
            D("BA_ITEM_REWARD_GJ_W_AMOUNT", "Battle Arena guild/job win amount", "Reward amount for winning Guild or Job Battle Arena mode.", SettingKind.Number, 750, 0, 255),
            D("BA_ITEM_REWARD_GJ_L_AMOUNT", "Battle Arena guild/job loss amount", "Reward amount for losing Guild or Job Battle Arena mode.", SettingKind.Number, 760, 0, 255),
            D("BA_ITEM_REWARD_PR_W_AMOUNT", "Battle Arena party/random win amount", "Reward amount for winning Party or Random Battle Arena mode.", SettingKind.Number, 770, 0, 255),
            D("BA_ITEM_REWARD_PR_L_AMOUNT", "Battle Arena party/random loss amount", "Reward amount for losing Party or Random Battle Arena mode.", SettingKind.Number, 780, 0, 255),

            D("JOB_LEVEL_MAX", "Maximum job level", "Highest level supported for Trader, Hunter, and Thief progression.", SettingKind.Number, 800, 1, 255),
            D("DISABLE_MOB_SPAWN_WHILE_TRADE", "Disable trade monster spawning", "Disable Thief and Hunter monster spawning while players are trading.", SettingKind.Boolean, 810),
            D("JOB_TEMPLE_LEVEL", "Job Temple minimum level", "Minimum character level required to enter the Job Temple.", SettingKind.Number, 820, 1, 255)
        };

        return definitions.ToDictionary(definition => definition.Name, StringComparer.OrdinalIgnoreCase);
    }

    private enum SettingKind
    {
        Boolean,
        Number,
        Text,
        Url
    }

    private sealed record SettingDefinition(
        string Name,
        string Section,
        string Label,
        string Help,
        SettingKind Kind,
        int Order,
        long? Minimum = null,
        long? Maximum = null);

    private sealed record SectionDefinition(
        string Key,
        string Title,
        string Help,
        int Order)
    {
        public override string ToString() => Title;
    }

    private sealed record ProxyServiceInputs(
        TextBox RemoteIpBox,
        TextBox RemotePortBox,
        TextBox BindPortBox,
        TextBox BindIpBox,
        CheckBox AutoStartBox);
}
