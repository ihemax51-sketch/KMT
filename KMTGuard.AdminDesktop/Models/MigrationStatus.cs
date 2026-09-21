namespace KMTGuard.AdminDesktop.Models;

public sealed class MigrationStatus
{
    public string Name { get; set; } = string.Empty;
    public string Status { get; set; } = "Not checked";
    public string Details { get; set; } = string.Empty;
    public string FullPath { get; set; } = string.Empty;
}
