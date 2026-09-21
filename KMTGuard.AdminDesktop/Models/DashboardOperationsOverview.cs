using System.Data;

namespace KMTGuard.AdminDesktop.Models;

public sealed class DashboardOperationsOverview
{
    public int ActiveEvents { get; set; }
    public int ActiveSchedules { get; set; }
    public int FailedSchedules { get; set; }
    public int PendingTelegramNotifications { get; set; }
    public int FailedTelegramNotifications { get; set; }
    public int SecurityActionsLast24Hours { get; set; }
    public DataTable UpcomingEvents { get; set; } = CreateUpcomingEventsTable();
    public DataTable RecentAdminActivity { get; set; } = CreateRecentActivityTable();

    public static DataTable CreateUpcomingEventsTable()
    {
        var table = new DataTable();
        table.Columns.Add("Event", typeof(string));
        table.Columns.Add("Time", typeof(string));
        table.Columns.Add("Days", typeof(string));
        table.Columns.Add("State", typeof(string));
        return table;
    }

    public static DataTable CreateRecentActivityTable()
    {
        var table = new DataTable();
        table.Columns.Add("When", typeof(string));
        table.Columns.Add("Admin", typeof(string));
        table.Columns.Add("Action", typeof(string));
        table.Columns.Add("Detail", typeof(string));
        return table;
    }
}

public sealed record DashboardAttentionItem(
    string Severity,
    string Title,
    string Detail,
    string TargetPage);
