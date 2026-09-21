using System.Collections.Concurrent;
using System.Diagnostics;
using System.Text.RegularExpressions;
using Dapper;
using Microsoft.Data.SqlClient;
using Serilog;

namespace KMTGuard.Database;

public enum RepeatType
{
    None,
    Interval,
    Daily,
    Weekly
}

public sealed class SchedulerJob
{
    public int Idx { get; init; }
    public string Name { get; init; } = string.Empty;
    public string Query { get; init; } = string.Empty;
    public DateOnly? ScheduledDate { get; init; }
    public DateTime? StartDateTime { get; init; }
    public TimeSpan Time { get; init; }
    public RepeatType Repeat { get; init; }
    public byte? RepeatDayOfWeek { get; init; }
    public byte? DaysOfWeekMask { get; init; }
    public int? IntervalSeconds { get; init; }
    public bool IsEnabled { get; init; }
    public DateTime? LastScheduledDateTime { get; init; }
    public int ExecutionTimeoutSeconds { get; init; }
    public int CatchUpWindowSeconds { get; init; }
}

public static class Scheduler
{
    private const int DefaultExecutionTimeoutSeconds = 7200;
    private const int DefaultCatchUpWindowSeconds = 300;
    private const int ScheduleRefreshSeconds = 30;
    private const int LeaseSeconds = 120;
    private const int HeartbeatSeconds = 30;
    private const int MaxConcurrentJobs = 4;
    private const int MaxExecutionTimeoutSeconds = 604800;
    private const int MinIntervalSeconds = 10;
    private const int MaxIntervalSeconds = 604800;

    private static readonly SemaphoreSlim InitializeLock = new(1, 1);
    private static readonly SemaphoreSlim ReloadLock = new(1, 1);
    private static readonly SemaphoreSlim ExecutionSlots = new(MaxConcurrentJobs, MaxConcurrentJobs);
    private static readonly ConcurrentDictionary<int, byte> ActiveJobs = new();
    private static readonly ConcurrentDictionary<int, DateTime> LocallyClaimedOccurrences = new();
    private static readonly string InstanceId =
        $"{Environment.MachineName}:{Environment.ProcessId}:{Guid.NewGuid():N}";
    private static readonly Regex ProcedureIdentifierPattern = new(
        @"^(?:\[[^\]\r\n]+\]|[A-Za-z_][A-Za-z0-9_]*)(?:\.(?:\[[^\]\r\n]+\]|[A-Za-z_][A-Za-z0-9_]*)){0,2}$",
        RegexOptions.Compiled | RegexOptions.CultureInvariant);

    private static SchedulerJob[] _jobs = Array.Empty<SchedulerJob>();
    private static CancellationTokenSource? _stopSource;
    private static Task? _schedulerLoop;

    public static async Task InitializeSchedulerTimer()
    {
        await InitializeLock.WaitAsync();
        try
        {
            if (_schedulerLoop is { IsCompleted: false })
                return;

            _stopSource = new CancellationTokenSource();
            await ReloadCoreAsync(_stopSource.Token);
            _schedulerLoop = RunSchedulerLoopAsync(_stopSource.Token);

            Log.Information(
                "Clockwork armed :: scheduled operations={OperationCount:N0}, instance={InstanceId}, concurrency={Concurrency}",
                _jobs.Length,
                InstanceId,
                MaxConcurrentJobs);
        }
        finally
        {
            InitializeLock.Release();
        }
    }

    public static async Task ReloadAsync()
    {
        var token = _stopSource?.Token ?? CancellationToken.None;
        await ReloadCoreAsync(token);
        Log.Information("Clockwork resynchronized :: scheduled operations={OperationCount:N0}", _jobs.Length);
    }

    private static async Task ReloadCoreAsync(CancellationToken token)
    {
        await ReloadLock.WaitAsync(token);
        try
        {
            await EnsureSchedulerSchemaAsync(token);
            _jobs = await LoadJobsFromDatabaseAsync(token);

            foreach (var job in _jobs)
            {
                if (job.LastScheduledDateTime.HasValue)
                    LocallyClaimedOccurrences.AddOrUpdate(
                        job.Idx,
                        job.LastScheduledDateTime.Value,
                        (_, current) => current >= job.LastScheduledDateTime.Value
                            ? current
                            : job.LastScheduledDateTime.Value);
            }
        }
        catch (OperationCanceledException) when (token.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Scheduler reload failed; the previous in-memory schedule remains active");
        }
        finally
        {
            ReloadLock.Release();
        }
    }

    private static async Task<SchedulerJob[]> LoadJobsFromDatabaseAsync(CancellationToken token)
    {
        const string sql = """
            SELECT Idx,
                   Name,
                   Query,
                   ScheduledDate,
                   StartDateTime,
                   Time,
                   RepeatType,
                   RepeatDayOfWeek,
                   DaysOfWeekMask,
                   IntervalSeconds,
                   IsEnabled,
                   LastScheduledDateTime,
                   ExecutionTimeoutSeconds,
                   CatchUpWindowSeconds
            FROM dbo.System_Schedule
            WHERE IsEnabled = 1;
            """;

        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(token);
        await using var command = new SqlCommand(sql, connection) { CommandTimeout = 60 };
        await using var reader = await command.ExecuteReaderAsync(token);
        var jobs = new List<SchedulerJob>();

        while (await reader.ReadAsync(token))
        {
            var repeatText = reader.GetString(reader.GetOrdinal("RepeatType"));
            if (!Enum.TryParse(repeatText, true, out RepeatType repeatType) ||
                !Enum.IsDefined(repeatType))
            {
                Log.Error(
                    "Scheduler ignored job {JobId} because RepeatType '{RepeatType}' is invalid",
                    reader.GetInt32(reader.GetOrdinal("Idx")),
                    repeatText);
                continue;
            }

            var job = new SchedulerJob
            {
                Idx = reader.GetInt32(reader.GetOrdinal("Idx")),
                Name = reader.GetString(reader.GetOrdinal("Name")),
                Query = reader.GetString(reader.GetOrdinal("Query")),
                IsEnabled = reader.GetBoolean(reader.GetOrdinal("IsEnabled")),
                Time = reader.GetTimeSpan(reader.GetOrdinal("Time")),
                Repeat = repeatType,
                ScheduledDate = reader.IsDBNull(reader.GetOrdinal("ScheduledDate"))
                    ? null
                    : DateOnly.FromDateTime(reader.GetDateTime(reader.GetOrdinal("ScheduledDate"))),
                StartDateTime = reader.IsDBNull(reader.GetOrdinal("StartDateTime"))
                    ? null
                    : reader.GetDateTime(reader.GetOrdinal("StartDateTime")),
                RepeatDayOfWeek = reader.IsDBNull(reader.GetOrdinal("RepeatDayOfWeek"))
                    ? null
                    : reader.GetByte(reader.GetOrdinal("RepeatDayOfWeek")),
                DaysOfWeekMask = reader.IsDBNull(reader.GetOrdinal("DaysOfWeekMask"))
                    ? null
                    : reader.GetByte(reader.GetOrdinal("DaysOfWeekMask")),
                IntervalSeconds = reader.IsDBNull(reader.GetOrdinal("IntervalSeconds"))
                    ? null
                    : reader.GetInt32(reader.GetOrdinal("IntervalSeconds")),
                LastScheduledDateTime = reader.IsDBNull(reader.GetOrdinal("LastScheduledDateTime"))
                    ? null
                    : reader.GetDateTime(reader.GetOrdinal("LastScheduledDateTime")),
                ExecutionTimeoutSeconds = NormalizeExecutionTimeout(
                    reader.GetInt32(reader.GetOrdinal("ExecutionTimeoutSeconds"))),
                CatchUpWindowSeconds = NormalizeCatchUpWindow(
                    reader.GetInt32(reader.GetOrdinal("CatchUpWindowSeconds")))
            };

            if (!IsJobConfigurationValid(job, out var reason))
            {
                Log.Error("Scheduler ignored invalid job {JobId} ({JobName}): {Reason}", job.Idx, job.Name, reason);
                continue;
            }

            jobs.Add(job);
        }

        return jobs.ToArray();
    }

    private static async Task RunSchedulerLoopAsync(CancellationToken token)
    {
        using var timer = new PeriodicTimer(TimeSpan.FromSeconds(1));
        var nextRefreshUtc = DateTime.UtcNow.AddSeconds(ScheduleRefreshSeconds);

        try
        {
            while (await timer.WaitForNextTickAsync(token))
            {
                await ProcessScheduler();

                if (DateTime.UtcNow < nextRefreshUtc)
                    continue;

                await ReloadCoreAsync(token);
                nextRefreshUtc = DateTime.UtcNow.AddSeconds(ScheduleRefreshSeconds);
            }
        }
        catch (OperationCanceledException) when (token.IsCancellationRequested)
        {
            // Normal shutdown.
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Scheduler loop stopped unexpectedly");
        }
    }

    public static Task ProcessScheduler()
    {
        var token = _stopSource?.Token ?? CancellationToken.None;
        if (token.IsCancellationRequested)
            return Task.CompletedTask;

        var now = DateTime.Now;
        foreach (var job in _jobs)
        {
            if (!TryGetDueOccurrence(job, now, out var occurrence))
                continue;

            if (LocallyClaimedOccurrences.TryGetValue(job.Idx, out var localOccurrence) &&
                localOccurrence >= occurrence)
            {
                continue;
            }

            if (!ActiveJobs.TryAdd(job.Idx, 0))
                continue;

            _ = ExecuteJobWorkerAsync(job, occurrence, token);
        }

        return Task.CompletedTask;
    }

    private static async Task ExecuteJobWorkerAsync(
        SchedulerJob job,
        DateTime occurrence,
        CancellationToken shutdownToken)
    {
        var enteredExecutionSlot = false;
        try
        {
            await ExecutionSlots.WaitAsync(shutdownToken);
            enteredExecutionSlot = true;

            var runToken = Guid.NewGuid();
            if (!await TryClaimJobAsync(job.Idx, occurrence, runToken, shutdownToken))
                return;

            LocallyClaimedOccurrences.AddOrUpdate(
                job.Idx,
                occurrence,
                (_, current) => current >= occurrence ? current : occurrence);

            await ExecuteClaimedJobAsync(job, occurrence, runToken, shutdownToken);
        }
        catch (OperationCanceledException) when (shutdownToken.IsCancellationRequested)
        {
            Log.Information("Scheduler worker cancelled during shutdown :: operation={OperationName}", job.Name);
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Scheduler worker failed :: operation={OperationName}", job.Name);
        }
        finally
        {
            if (enteredExecutionSlot)
                ExecutionSlots.Release();

            ActiveJobs.TryRemove(job.Idx, out _);
        }
    }

    private static async Task<bool> TryClaimJobAsync(
        int jobId,
        DateTime occurrence,
        Guid runToken,
        CancellationToken token)
    {
        const string sql = """
            UPDATE dbo.System_Schedule WITH (UPDLOCK, ROWLOCK)
            SET LastScheduledDateTime = @Occurrence,
                RunningToken = @RunToken,
                RunningBy = @RunningBy,
                RunningSinceUtc = SYSUTCDATETIME(),
                LeaseUntilUtc = DATEADD(SECOND, @LeaseSeconds, SYSUTCDATETIME()),
                LastStatus = N'Running',
                LastError = NULL
            OUTPUT INSERTED.RunningToken
            WHERE Idx = @Idx
              AND IsEnabled = 1
              AND (LastScheduledDateTime IS NULL OR LastScheduledDateTime < @Occurrence)
              AND (RunningToken IS NULL OR LeaseUntilUtc < SYSUTCDATETIME());
            """;

        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(token);
        await using var command = new SqlCommand(sql, connection) { CommandTimeout = 30 };
        command.Parameters.Add("@Idx", System.Data.SqlDbType.Int).Value = jobId;
        command.Parameters.Add("@Occurrence", System.Data.SqlDbType.DateTime2).Value = occurrence;
        command.Parameters.Add("@RunToken", System.Data.SqlDbType.UniqueIdentifier).Value = runToken;
        command.Parameters.Add("@RunningBy", System.Data.SqlDbType.NVarChar, 160).Value = InstanceId;
        command.Parameters.Add("@LeaseSeconds", System.Data.SqlDbType.Int).Value = LeaseSeconds;

        var claimedToken = await command.ExecuteScalarAsync(token);
        return claimedToken is Guid value && value == runToken;
    }

    private static async Task ExecuteClaimedJobAsync(
        SchedulerJob job,
        DateTime occurrence,
        Guid runToken,
        CancellationToken shutdownToken)
    {
        using var timeoutSource = job.ExecutionTimeoutSeconds > 0
            ? new CancellationTokenSource(TimeSpan.FromSeconds(job.ExecutionTimeoutSeconds))
            : new CancellationTokenSource();
        using var executionSource =
            CancellationTokenSource.CreateLinkedTokenSource(shutdownToken, timeoutSource.Token);
        using var heartbeatSource =
            CancellationTokenSource.CreateLinkedTokenSource(shutdownToken, executionSource.Token);

        var startedAtUtc = DateTime.UtcNow;
        var stopwatch = Stopwatch.StartNew();
        var heartbeatTask = MaintainLeaseAsync(runToken, heartbeatSource.Token);
        string status;
        string? error = null;

        try
        {
            Log.Information(
                "Clockwork dispatch started :: operation={OperationName}, occurrence={Occurrence}, timeoutSeconds={TimeoutSeconds}",
                job.Name,
                occurrence,
                job.ExecutionTimeoutSeconds);

            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync(executionSource.Token);
            await using var command = new SqlCommand(job.Query, connection)
            {
                CommandTimeout = job.ExecutionTimeoutSeconds
            };
            await command.ExecuteNonQueryAsync(executionSource.Token);
            status = "Succeeded";
        }
        catch (OperationCanceledException ex) when (shutdownToken.IsCancellationRequested)
        {
            status = "Cancelled";
            error = ex.Message;
        }
        catch (OperationCanceledException ex) when (timeoutSource.IsCancellationRequested)
        {
            status = "TimedOut";
            error = $"Execution exceeded {job.ExecutionTimeoutSeconds} seconds. {ex.Message}";
        }
        catch (SqlException ex) when (ex.Number == -2)
        {
            status = "TimedOut";
            error = $"Execution exceeded {job.ExecutionTimeoutSeconds} seconds. {ex.Message}";
        }
        catch (Exception ex)
        {
            status = "Failed";
            error = ex.ToString();
        }
        finally
        {
            stopwatch.Stop();
            heartbeatSource.Cancel();
            try
            {
                await heartbeatTask;
            }
            catch (OperationCanceledException)
            {
                // Expected when execution finishes.
            }
        }

        await CompleteRunAsync(
            job,
            occurrence,
            startedAtUtc,
            runToken,
            status,
            error,
            stopwatch.ElapsedMilliseconds);
    }

    private static async Task MaintainLeaseAsync(Guid runToken, CancellationToken token)
    {
        using var timer = new PeriodicTimer(TimeSpan.FromSeconds(HeartbeatSeconds));
        while (await timer.WaitForNextTickAsync(token))
        {
            const string sql = """
                UPDATE dbo.System_Schedule
                SET LeaseUntilUtc = DATEADD(SECOND, @LeaseSeconds, SYSUTCDATETIME())
                WHERE RunningToken = @RunToken;
                """;

            try
            {
                await using var connection = new SqlConnection(Program.Connectionstring);
                await connection.OpenAsync(token);
                await using var command = new SqlCommand(sql, connection) { CommandTimeout = 30 };
                command.Parameters.Add("@LeaseSeconds", System.Data.SqlDbType.Int).Value = LeaseSeconds;
                command.Parameters.Add("@RunToken", System.Data.SqlDbType.UniqueIdentifier).Value = runToken;
                var updated = await command.ExecuteNonQueryAsync(token);
                if (updated == 0)
                {
                    Log.Error("Scheduler lease was lost :: runToken={RunToken}", runToken);
                    return;
                }
            }
            catch (OperationCanceledException) when (token.IsCancellationRequested)
            {
                throw;
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "Scheduler heartbeat failed :: runToken={RunToken}", runToken);
            }
        }
    }

    private static async Task CompleteRunAsync(
        SchedulerJob job,
        DateTime occurrence,
        DateTime startedAtUtc,
        Guid runToken,
        string status,
        string? error,
        long durationMilliseconds)
    {
        const string sql = """
            SET XACT_ABORT ON;
            BEGIN TRANSACTION;

            UPDATE dbo.System_Schedule
            SET RunningToken = NULL,
                RunningBy = NULL,
                RunningSinceUtc = NULL,
                LeaseUntilUtc = NULL,
                LastStatus = @Status,
                LastError = @LastError,
                LastDurationMs = @DurationMs,
                LastCompletedDateTimeUtc = SYSUTCDATETIME(),
                LastRunDateTime = CASE WHEN @Status = N'Succeeded' THEN SYSDATETIME() ELSE LastRunDateTime END
            WHERE RunningToken = @RunToken;

            DECLARE @UpdatedRows INT = @@ROWCOUNT;
            IF @UpdatedRows = 1
            BEGIN
                INSERT INTO dbo.System_ScheduleHistory
                    (JobId, JobName, TriggerType, ScheduledFor, StartedAtUtc, CompletedAtUtc,
                     Status, DurationMs, ExecutedBy, Error)
                VALUES
                    (@JobId, @JobName, N'Scheduled', @ScheduledFor, @StartedAtUtc, SYSUTCDATETIME(),
                     @Status, @DurationMs, @ExecutedBy, @LastError);
            END;

            COMMIT TRANSACTION;
            SELECT @UpdatedRows;
            """;

        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await using var command = new SqlCommand(sql, connection) { CommandTimeout = 60 };
            command.Parameters.Add("@RunToken", System.Data.SqlDbType.UniqueIdentifier).Value = runToken;
            command.Parameters.Add("@JobId", System.Data.SqlDbType.Int).Value = job.Idx;
            command.Parameters.Add("@JobName", System.Data.SqlDbType.NVarChar, 128).Value = job.Name;
            command.Parameters.Add("@ScheduledFor", System.Data.SqlDbType.DateTime2).Value = occurrence;
            command.Parameters.Add("@StartedAtUtc", System.Data.SqlDbType.DateTime2).Value = startedAtUtc;
            command.Parameters.Add("@ExecutedBy", System.Data.SqlDbType.NVarChar, 160).Value = InstanceId;
            command.Parameters.Add("@Status", System.Data.SqlDbType.NVarChar, 16).Value = status;
            command.Parameters.Add("@LastError", System.Data.SqlDbType.NVarChar, 2048).Value =
                string.IsNullOrWhiteSpace(error)
                    ? DBNull.Value
                    : error.Length <= 2048 ? error : error[..2048];
            command.Parameters.Add("@DurationMs", System.Data.SqlDbType.BigInt).Value = durationMilliseconds;

            var updated = Convert.ToInt32(await command.ExecuteScalarAsync());
            if (updated == 0)
            {
                Log.Error(
                    "Scheduler could not finalize a run because its lease was lost :: operation={OperationName}, status={Status}",
                    job.Name,
                    status);
                return;
            }

            if (status == "Succeeded")
            {
                Log.Information(
                    "Clockwork dispatch complete :: operation={OperationName}, durationMs={DurationMs}",
                    job.Name,
                    durationMilliseconds);
            }
            else
            {
                Log.Error(
                    "Clockwork dispatch ended :: operation={OperationName}, status={Status}, durationMs={DurationMs}, error={Error}",
                    job.Name,
                    status,
                    durationMilliseconds,
                    error);
            }
        }
        catch (Exception ex)
        {
            Log.Error(ex, "Scheduler could not persist run completion :: operation={OperationName}", job.Name);
        }
    }

    internal static bool TryGetDueOccurrence(
        SchedulerJob job,
        DateTime now,
        out DateTime occurrence)
    {
        occurrence = default;
        if (!job.IsEnabled)
            return false;

        DateTime candidate;
        switch (job.Repeat)
        {
            case RepeatType.None:
                if (!job.ScheduledDate.HasValue)
                    return false;
                candidate = job.ScheduledDate.Value.ToDateTime(TimeOnly.FromTimeSpan(job.Time));
                break;

            case RepeatType.Interval:
                if (!job.StartDateTime.HasValue ||
                    !job.IntervalSeconds.HasValue ||
                    job.IntervalSeconds is < MinIntervalSeconds or > MaxIntervalSeconds ||
                    now < job.StartDateTime.Value)
                {
                    return false;
                }

                var intervalTicks = TimeSpan.FromSeconds(job.IntervalSeconds.Value).Ticks;
                var elapsedTicks = now.Ticks - job.StartDateTime.Value.Ticks;
                candidate = job.StartDateTime.Value.AddTicks((elapsedTicks / intervalTicks) * intervalTicks);
                break;

            case RepeatType.Daily:
                candidate = now.Date.Add(job.Time);
                if (candidate > now)
                    candidate = candidate.AddDays(-1);
                break;

            case RepeatType.Weekly:
                var daysMask = job.DaysOfWeekMask ??
                    (job.RepeatDayOfWeek is >= 1 and <= 7
                        ? (byte)(1 << (job.RepeatDayOfWeek.Value - 1))
                        : (byte)0);
                if (daysMask is < 1 or > 127)
                    return false;

                candidate = default;
                for (var daysBack = 0; daysBack < 7; daysBack++)
                {
                    var date = now.Date.AddDays(-daysBack);
                    var dayNumber = ToScheduleDayOfWeek(date.DayOfWeek);
                    if ((daysMask & (1 << (dayNumber - 1))) == 0)
                        continue;

                    var weeklyCandidate = date.Add(job.Time);
                    if (weeklyCandidate <= now)
                    {
                        candidate = weeklyCandidate;
                        break;
                    }
                }

                if (candidate == default)
                {
                    for (var daysBack = 7; daysBack < 14; daysBack++)
                    {
                        var date = now.Date.AddDays(-daysBack);
                        var dayNumber = ToScheduleDayOfWeek(date.DayOfWeek);
                        if ((daysMask & (1 << (dayNumber - 1))) == 0)
                            continue;

                        candidate = date.Add(job.Time);
                        break;
                    }
                }

                if (candidate == default)
                    return false;
                break;

            default:
                return false;
        }

        var age = now - candidate;
        if (age < TimeSpan.Zero ||
            age > TimeSpan.FromSeconds(NormalizeCatchUpWindow(job.CatchUpWindowSeconds)))
        {
            return false;
        }

        if (job.LastScheduledDateTime.HasValue &&
            job.LastScheduledDateTime.Value >= candidate)
        {
            return false;
        }

        occurrence = candidate;
        return true;
    }

    internal static bool IsAllowedScheduledSql(string query)
    {
        if (string.IsNullOrWhiteSpace(query))
            return false;

        var normalized = query.Trim();
        if (!normalized.StartsWith("EXEC ", StringComparison.OrdinalIgnoreCase) ||
            normalized.Contains(';') ||
            normalized.Contains("--", StringComparison.Ordinal) ||
            normalized.Contains("/*", StringComparison.Ordinal) ||
            normalized.Contains("*/", StringComparison.Ordinal))
        {
            return false;
        }

        var commandBody = normalized[5..].TrimStart();
        var separator = commandBody.IndexOfAny([' ', '\t', '\r', '\n']);
        var procedureIdentifier = separator < 0 ? commandBody : commandBody[..separator];
        if (!ProcedureIdentifierPattern.IsMatch(procedureIdentifier))
            return false;

        var procedureName = procedureIdentifier
            .Split('.')
            .Last()
            .Trim('[', ']');
        return !procedureName.Equals("sp_executesql", StringComparison.OrdinalIgnoreCase) &&
               !procedureName.StartsWith("xp_", StringComparison.OrdinalIgnoreCase);
    }

    private static bool IsJobConfigurationValid(SchedulerJob job, out string reason)
    {
        if (!IsAllowedScheduledSql(job.Query))
        {
            reason = "only one EXEC stored-procedure command without comments or semicolons is allowed";
            return false;
        }

        if (job.Repeat == RepeatType.None && !job.ScheduledDate.HasValue)
        {
            reason = "ScheduledDate is required when RepeatType is None";
            return false;
        }

        if (job.Repeat == RepeatType.Interval &&
            (!job.StartDateTime.HasValue ||
             !job.IntervalSeconds.HasValue ||
             job.IntervalSeconds is < MinIntervalSeconds or > MaxIntervalSeconds))
        {
            reason = $"Interval schedules require a start time and an interval between {MinIntervalSeconds} and {MaxIntervalSeconds} seconds";
            return false;
        }

        if (job.Repeat == RepeatType.Weekly &&
            (job.DaysOfWeekMask is not (>= 1 and <= 127) &&
             (!job.RepeatDayOfWeek.HasValue || job.RepeatDayOfWeek is < 1 or > 7)))
        {
            reason = "Weekly schedules require at least one weekday";
            return false;
        }

        reason = string.Empty;
        return true;
    }

    private static int ToScheduleDayOfWeek(DayOfWeek day) =>
        day == DayOfWeek.Sunday ? 7 : (int)day;

    private static int NormalizeExecutionTimeout(int seconds) =>
        seconds is < 0 or > MaxExecutionTimeoutSeconds
            ? DefaultExecutionTimeoutSeconds
            : seconds;

    private static int NormalizeCatchUpWindow(int seconds) =>
        seconds is < 1 or > 604800 ? DefaultCatchUpWindowSeconds : seconds;
    private static async Task EnsureSchedulerSchemaAsync(CancellationToken token)
    {
        await using var connection = new SqlConnection(Program.Connectionstring);
        await connection.OpenAsync(token);
        var ready = await connection.ExecuteScalarAsync<int>(new CommandDefinition(@"
SELECT CASE WHEN OBJECT_ID(N'dbo.System_Schedule', N'U') IS NOT NULL
                  AND OBJECT_ID(N'dbo.System_ScheduleHistory', N'U') IS NOT NULL
                  AND COL_LENGTH(N'dbo.System_Schedule', N'RunningToken') IS NOT NULL
                  AND COL_LENGTH(N'dbo.System_Schedule', N'LeaseUntilUtc') IS NOT NULL
                  AND COL_LENGTH(N'dbo.System_Schedule', N'ExecutionTimeoutSeconds') IS NOT NULL
                  AND COL_LENGTH(N'dbo.System_Schedule', N'CatchUpWindowSeconds') IS NOT NULL
             THEN 1 ELSE 0 END;",
            cancellationToken: token));
        if (ready != 1)
            throw new InvalidOperationException(
                "Scheduler schema is incomplete. Apply the packaged database updates.");
    }

    public static void Stop()
    {
        var source = Interlocked.Exchange(ref _stopSource, null);
        source?.Cancel();
        _schedulerLoop = null;
    }
}
