namespace KMTGuard.AdminDesktop.Models;

public sealed class LogFileItem
{
    public string Name { get; set; } = string.Empty;
    public string Path { get; set; } = string.Empty;
    public DateTime LastWriteTime { get; set; }
    public long Length { get; set; }

    public override string ToString() => $"{Name}  ({LastWriteTime:g})";
}
