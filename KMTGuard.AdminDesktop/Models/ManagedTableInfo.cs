namespace KMTGuard.AdminDesktop.Models;

public sealed class ManagedTableInfo
{
    public string Key { get; set; } = string.Empty;
    public string Title { get; set; } = string.Empty;
    public string Module { get; set; } = string.Empty;
    public string TableName { get; set; } = string.Empty;
    public bool AllowInsert { get; set; }
    public bool AllowUpdate { get; set; }
    public bool AllowDelete { get; set; }
    public string Notes { get; set; } = string.Empty;

    public override string ToString() => Title;
}
