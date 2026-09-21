using System.Data;
using System.Windows.Media;
using KMTGuard.AdminDesktop.Models;
using KMTGuard.AdminDesktop.Services;

namespace KMTGuard.AdminDesktop;

public partial class MainWindow
{
    private bool _dashboardOperationsRefreshBusy;

    private async Task RefreshDashboardOperationsAsync(
        SqlAdminService? sqlService,
        DbHealth? health,
        string? loadError = null)
    {
        if (_dashboardOperationsRefreshBusy)
            return;

        _dashboardOperationsRefreshBusy = true;
        try
        {
            var attentions = new List<DashboardAttentionItem>();
            DashboardOperationsOverview overview = new();
            var diagnostics = new DataTable();
            FilterProcessStatus? runtimeStatus = null;
            DataTable? onlinePlayers = null;
            string? runtimeError = null;
            string? operationsError = loadError;

            try
            {
                runtimeStatus = await _filterRuntimeService.GetStatusAsync();
                ApplyFilterProcessStatus(runtimeStatus);
                DashboardServiceList.ItemsSource = runtimeStatus.Services;
                onlinePlayers = await _filterRuntimeService.LoadOnlinePlayersSnapshotAsync();
            }
            catch (Exception ex)
            {
                runtimeError = ex.Message;
                DashboardServiceList.ItemsSource = null;
                DashboardRuntimeStatusText.Text = "Unavailable";
                DashboardRuntimeDetailText.Text = ex.Message;
                DashboardRunningServicesText.Text = "0 / 3";
                DashboardRuntimeStatusDot.Fill = GetThemeBrush("RoseBrush", Brushes.IndianRed);
            }

            if (sqlService is not null && health?.IsConnected == true)
            {
                try
                {
                    var overviewTask = sqlService.LoadDashboardOperationsAsync();
                    var diagnosticsTask = sqlService.LoadDiagnosticsAsync();
                    await Task.WhenAll(overviewTask, diagnosticsTask);
                    overview = await overviewTask;
                    diagnostics = await diagnosticsTask;
                }
                catch (Exception ex)
                {
                    operationsError = ex.Message;
                }
            }

            ApplyDashboardCapacity(onlinePlayers?.Rows.Count ?? 0, runtimeStatus?.Services.Any(item => item.Role == KMTGuard.RuntimeContract.FilterRole.Agent && item.IsRunning) == true);
            ApplyDashboardReadiness(diagnostics);
            ApplyDashboardOperationsData(overview);

            if (health?.IsConnected != true)
            {
                attentions.Add(new DashboardAttentionItem(
                    "Critical",
                    "Database connection is unavailable",
                    loadError ?? health?.Message ?? "Open Connection and verify the active Settings.json file.",
                    "Connection"));
            }

            if (runtimeStatus is null)
            {
                attentions.Add(new DashboardAttentionItem(
                    "Critical",
                    "Runtime status could not be read",
                    runtimeError ?? "The desktop could not contact the local filter services.",
                    "Runtime"));
            }
            else if (!runtimeStatus.IsRunning)
            {
                var running = runtimeStatus.Services.Count(item => item.IsRunning);
                attentions.Add(new DashboardAttentionItem(
                    running == 0 ? "Critical" : "Warning",
                    running == 0 ? "Filter services are stopped" : "Filter services are only partially running",
                    $"{running} of {runtimeStatus.Services.Count} services report as running.",
                    "Runtime"));
            }

            if (health is { InvalidSettings: > 0 })
            {
                attentions.Add(new DashboardAttentionItem(
                    "Critical",
                    "Configuration contains invalid values",
                    $"{health.InvalidSettings:N0} setting(s) need correction before relying on the current configuration.",
                    "Settings"));
            }

            if (health is { PendingCommands: > 0 })
            {
                attentions.Add(new DashboardAttentionItem(
                    health.PendingCommands >= 25 ? "Critical" : "Warning",
                    "Runtime commands are waiting",
                    $"{health.PendingCommands:N0} queued action(s) have not completed yet.",
                    "Runtime"));
            }

            var requiredMissing = diagnostics.AsEnumerable()
                .Count(row => string.Equals(row.Field<string>("Status"), "Missing", StringComparison.OrdinalIgnoreCase));
            if (requiredMissing > 0)
            {
                attentions.Add(new DashboardAttentionItem(
                    "Critical",
                    "Required database components are missing",
                    $"{requiredMissing:N0} required object(s) are not ready. Review System Health before using affected features.",
                    "Diagnostics"));
            }

            if (overview.FailedSchedules > 0)
            {
                attentions.Add(new DashboardAttentionItem(
                    "Warning",
                    "Scheduled jobs need review",
                    $"{overview.FailedSchedules:N0} job(s) ended with an error or timeout.",
                    "Scheduler"));
            }

            if (overview.FailedTelegramNotifications > 0)
            {
                attentions.Add(new DashboardAttentionItem(
                    "Warning",
                    "Telegram deliveries failed",
                    $"{overview.FailedTelegramNotifications:N0} notification(s) are waiting for investigation or retry.",
                    "Telegram"));
            }

            if (overview.SecurityActionsLast24Hours >= 50)
            {
                attentions.Add(new DashboardAttentionItem(
                    "Warning",
                    "Protection activity is elevated",
                    $"{overview.SecurityActionsLast24Hours:N0} bot-protection actions were recorded during the last 24 hours.",
                    "Security"));
            }

            if (!string.IsNullOrWhiteSpace(operationsError) && health?.IsConnected == true)
            {
                attentions.Add(new DashboardAttentionItem(
                    "Warning",
                    "Part of the operations brief could not be loaded",
                    operationsError,
                    "Diagnostics"));
            }

            var issueCount = attentions.Count;
            if (issueCount == 0)
            {
                attentions.Add(new DashboardAttentionItem(
                    "Healthy",
                    "No immediate action is required",
                    "Database checks, filter services and operational queues look healthy.",
                    "Diagnostics"));
            }

            DashboardAttentionCountText.Text = issueCount.ToString("N0");
            DashboardAttentionList.ItemsSource = attentions;
            DashboardUpdatedText.Text = $"Brief refreshed {DateTime.Now:HH:mm:ss}";
        }
        finally
        {
            _dashboardOperationsRefreshBusy = false;
        }
    }

    private void ApplyDashboardCapacity(int onlinePlayers, bool agentRunning)
    {
        DashboardOnlinePlayersText.Text = onlinePlayers.ToString("N0");

        DashboardCapacityBar.Value = agentRunning ? 100 : 0;
        DashboardCapacityBar.Foreground = GetThemeBrush("SuccessBrush", Brushes.ForestGreen);
        DashboardCapacityText.Text = agentRunning
            ? "Unified edition · no product-imposed player cap"
            : "Agent is stopped · live player count is unavailable";
    }

    private void ApplyDashboardReadiness(DataTable diagnostics)
    {
        if (diagnostics.Rows.Count == 0)
        {
            DashboardModuleReadinessText.Text = "Unavailable";
            DashboardModuleProgress.Value = 0;
            DashboardModulesDetailText.Text = "Connect to SQL to inspect installed feature components.";
            return;
        }

        var ready = diagnostics.AsEnumerable()
            .Count(row => string.Equals(row.Field<string>("Status"), "Ready", StringComparison.OrdinalIgnoreCase));
        var requiredMissing = diagnostics.AsEnumerable()
            .Count(row => string.Equals(row.Field<string>("Status"), "Missing", StringComparison.OrdinalIgnoreCase));
        var percentage = (ready * 100d) / diagnostics.Rows.Count;
        DashboardModuleProgress.Value = percentage;
        DashboardModuleProgress.Foreground = requiredMissing > 0
            ? GetThemeBrush("RoseBrush", Brushes.IndianRed)
            : percentage < 80
                ? GetThemeBrush("AmberBrush", Brushes.DarkGoldenrod)
                : GetThemeBrush("SuccessBrush", Brushes.ForestGreen);
        DashboardModuleReadinessText.Text = $"{ready:N0} / {diagnostics.Rows.Count:N0}";
        DashboardModulesDetailText.Text = requiredMissing > 0
            ? $"{requiredMissing:N0} required component(s) are missing; optional modules are shown separately in System Health."
            : "Every required component in the current readiness catalog is available.";
    }

    private void ApplyDashboardOperationsData(DashboardOperationsOverview overview)
    {
        DashboardActiveEventsText.Text = $"{overview.ActiveEvents:N0} event(s)";
        DashboardSchedulesText.Text = $"{overview.ActiveSchedules:N0} schedule(s)";

        if (overview.UpcomingEvents.Rows.Count == 0)
            overview.UpcomingEvents.Rows.Add("No event schedules configured", "—", "—", "Review");
        DashboardUpcomingEventsGrid.ItemsSource = overview.UpcomingEvents.DefaultView;

        if (overview.RecentAdminActivity.Rows.Count == 0)
            overview.RecentAdminActivity.Rows.Add("—", "—", "No recorded activity", "Administrator actions will appear here.");
        DashboardRecentActivityGrid.ItemsSource = overview.RecentAdminActivity.DefaultView;
    }
}
