namespace KMTGuard.AdminDesktop.Models;

public sealed class ProxyServiceEntry
{
    public int ServiceId { get; set; }
    public string Name { get; set; } = string.Empty;
    public int ServerType { get; set; }
    public string RemoteIP { get; set; } = string.Empty;
    public int RemotePort { get; set; }
    public string BindIP { get; set; } = string.Empty;
    public int BindPort { get; set; }
    public int ByteLimitation { get; set; }
    public bool AutoStart { get; set; }
}
