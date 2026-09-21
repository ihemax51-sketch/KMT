using KMTGuard.RuntimeContract;

namespace KMTGuard.AdminDesktop.Models;

public sealed class FilterProcessStatus
{
    public bool IsRunning { get; set; }
    public bool AnyRunning { get; set; }
    public IReadOnlyList<FilterServiceStatus> Services { get; set; } = Array.Empty<FilterServiceStatus>();
    public string Message { get; set; } = "Stopped";
}

public sealed class FilterServiceStatus
{
    public FilterRole Role { get; set; }
    public string Status { get; set; } = "Stopped";
    public bool IsRunning { get; set; }
    public int? ProcessId { get; set; }
    public double MemoryMb { get; set; }
    public int Sessions { get; set; }
    public int Listeners { get; set; }
    public string Uptime { get; set; } = "-";
    public string Executable { get; set; } = string.Empty;
}
