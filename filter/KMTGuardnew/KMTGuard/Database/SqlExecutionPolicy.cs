namespace KMTGuard.Database;

public static class SqlExecutionPolicy
{
    public const int AuthenticationSeconds = 10;
    public const int PacketCriticalSeconds = 8;
    public const int RuntimeHealthSeconds = 3;
    public const int BackgroundSeconds = 30;
    public const int CleanupSeconds = 30;
    public const int QueueSeconds = 30;
    public const int EventSeconds = 30;
    public const int MaintenanceSeconds = 120;

    public static readonly TimeSpan PacketCriticalTimeout =
        TimeSpan.FromSeconds(PacketCriticalSeconds);
}
