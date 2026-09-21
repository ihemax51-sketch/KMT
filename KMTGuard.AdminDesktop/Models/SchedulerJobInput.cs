namespace KMTGuard.AdminDesktop.Models;

public sealed class SchedulerJobInput
{
    public int? Idx { get; init; }
    public string Name { get; init; } = string.Empty;
    public string DatabaseName { get; init; } = string.Empty;
    public string ProcedureName { get; init; } = string.Empty;
    public string Arguments { get; init; } = string.Empty;
    public string RepeatType { get; init; } = "None";
    public DateTime? StartDateTime { get; init; }
    public DateTime? ScheduledDate { get; init; }
    public TimeSpan Time { get; init; }
    public int? IntervalSeconds { get; init; }
    public byte? DaysOfWeekMask { get; init; }
    public bool Enabled { get; init; } = true;
    public int ExecutionTimeoutSeconds { get; init; } = 7200;
    public int CatchUpWindowSeconds { get; init; } = 300;
}
