namespace KMTGuard.AdminDesktop.Models;

public sealed class DbHealth
{
    public bool IsConnected { get; set; }
    public string ServerVersion { get; set; } = "Not connected";
    public int TableCount { get; set; }
    public int SettingCount { get; set; }
    public int PendingCommands { get; set; }
    public int InvalidSettings { get; set; }
    public string Message { get; set; } = string.Empty;
}
