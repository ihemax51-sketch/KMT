using System.Threading.Channels;
using System.Runtime.CompilerServices;
using Microsoft.Data.SqlClient;
using Serilog;

namespace KMTGuard.Helpers;

public static class DatabaseJobQueue
{
    private const int Capacity = 512;
    private const int WorkerCount = 2;

    private sealed record DatabaseJob(
        Func<CancellationToken, Task> Action,
        TaskCompletionSource? Completion,
        string Operation);

    private sealed class WorkerGeneration
    {
        public WorkerGeneration(int id)
        {
            Id = id;
            Queue = CreateQueue();
            Shutdown = new CancellationTokenSource();
        }

        public int Id { get; }
        public Channel<DatabaseJob> Queue { get; }
        public CancellationTokenSource Shutdown { get; }
        public Task[] Workers { get; set; } = Array.Empty<Task>();
    }

    private static readonly object LifecycleLock = new();
    private static int _nextGenerationId;
    private static WorkerGeneration _generation = StartGeneration();
    private static Task _stopCompletion = Task.CompletedTask;
    private static int _stopped;

    private static Channel<DatabaseJob> CreateQueue() =>
        Channel.CreateBounded<DatabaseJob>(new BoundedChannelOptions(Capacity)
        {
            FullMode = BoundedChannelFullMode.Wait,
            SingleReader = false,
            SingleWriter = false,
            AllowSynchronousContinuations = false
        });

    public static void Start()
    {
        lock (LifecycleLock)
        {
            if (Volatile.Read(ref _stopped) == 0)
                return;

            if (!_stopCompletion.IsCompleted)
                throw new InvalidOperationException(
                    "The previous database job queue generation is still stopping.");

            _generation = StartGeneration();
            Volatile.Write(ref _stopped, 0);
        }
    }

    public static async Task RunAsync(
        Action action,
        [CallerMemberName] string operation = "database job",
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(action);

        if (Volatile.Read(ref _stopped) != 0)
            throw new InvalidOperationException("The database job queue is stopping.");

        await RunAsync(
            _ =>
            {
                action();
                return Task.CompletedTask;
            },
            operation,
            cancellationToken);
    }

    public static async Task RunAsync(
        Func<CancellationToken, Task> action,
        [CallerMemberName] string operation = "database job",
        CancellationToken cancellationToken = default)
    {
        ArgumentNullException.ThrowIfNull(action);

        if (Volatile.Read(ref _stopped) != 0)
            throw new InvalidOperationException("The database job queue is stopping.");

        var completion = new TaskCompletionSource(
            TaskCreationOptions.RunContinuationsAsynchronously);
        var job = new DatabaseJob(action, completion, operation);

        var generation = Volatile.Read(ref _generation);
        await generation.Queue.Writer.WriteAsync(job, cancellationToken);
        await completion.Task.WaitAsync(cancellationToken);
    }

    public static bool TryQueueBackground(
        Func<CancellationToken, Task> action,
        [CallerMemberName] string operation = "background database job")
    {
        ArgumentNullException.ThrowIfNull(action);

        if (Volatile.Read(ref _stopped) != 0)
        {
            Log.Warning(
                "Database job queue is stopping; background job was not queued: {Operation}",
                operation);
            return false;
        }

        var job = new DatabaseJob(action, null, operation);
        var generation = Volatile.Read(ref _generation);
        if (generation.Queue.Writer.TryWrite(job))
            return true;

        Log.Warning(
            "Database job queue is full; background job was dropped to protect packet processing: {Operation}",
            operation);
        return false;
    }

    public static Task RunIdempotentAsync(
        Func<CancellationToken, Task> action,
        int maxAttempts = 3,
        [CallerMemberName] string operation = "idempotent database job",
        CancellationToken cancellationToken = default)
    {
        if (maxAttempts < 1)
            throw new ArgumentOutOfRangeException(nameof(maxAttempts));

        return RunAsync(
            async workerToken =>
            {
                for (int attempt = 1; ; attempt++)
                {
                    try
                    {
                        await action(workerToken);
                        return;
                    }
                    catch (SqlException ex) when (
                        attempt < maxAttempts &&
                        IsTransient(ex) &&
                        !workerToken.IsCancellationRequested)
                    {
                        int delayMs = (attempt * 250) + Random.Shared.Next(50, 151);
                        Log.Warning(
                            ex,
                            "Transient SQL failure in {Operation}; retry {Attempt}/{MaxAttempts} after {DelayMs} ms.",
                            operation,
                            attempt + 1,
                            maxAttempts,
                            delayMs);
                        await Task.Delay(delayMs, workerToken);
                    }
                }
            },
            operation,
            cancellationToken);
    }

    public static async Task StopAsync(TimeSpan? timeout = null)
    {
        WorkerGeneration generation;
        TaskCompletionSource? stopCompletion = null;
        Task existingStop;
        lock (LifecycleLock)
        {
            if (Volatile.Read(ref _stopped) != 0)
            {
                existingStop = _stopCompletion;
                generation = _generation;
            }
            else
            {
                Volatile.Write(ref _stopped, 1);
                stopCompletion = new TaskCompletionSource(
                    TaskCreationOptions.RunContinuationsAsynchronously);
                _stopCompletion = stopCompletion.Task;
                existingStop = Task.CompletedTask;
                generation = _generation;
            }
        }

        if (stopCompletion == null)
        {
            await existingStop;
            return;
        }

        generation.Queue.Writer.TryComplete();
        TimeSpan waitTime = timeout ?? TimeSpan.FromSeconds(15);

        try
        {
            await Task.WhenAll(generation.Workers).WaitAsync(waitTime);
        }
        catch (TimeoutException)
        {
            Log.Warning(
                "Database job queue did not drain within {TimeoutSeconds} seconds; cancelling remaining jobs.",
                waitTime.TotalSeconds);
            generation.Shutdown.Cancel();

            // Do not dispose or replace this generation until every worker has
            // observed cancellation and released the token and channel state.
            await Task.WhenAll(generation.Workers);
        }
        finally
        {
            generation.Shutdown.Dispose();
            stopCompletion.TrySetResult();
        }
    }

    private static WorkerGeneration StartGeneration()
    {
        var generation = new WorkerGeneration(
            Interlocked.Increment(ref _nextGenerationId));
        var workers = new Task[WorkerCount];
        for (int i = 0; i < workers.Length; i++)
            workers[i] = Task.Run(() => ProcessQueueAsync(generation));

        generation.Workers = workers;
        return generation;
    }

    private static async Task ProcessQueueAsync(WorkerGeneration generation)
    {
        try
        {
            await foreach (var job in generation.Queue.Reader.ReadAllAsync(
                               generation.Shutdown.Token))
            {
                try
                {
                    await job.Action(generation.Shutdown.Token);
                    job.Completion?.TrySetResult();
                }
                catch (OperationCanceledException)
                    when (generation.Shutdown.IsCancellationRequested)
                {
                    job.Completion?.TrySetCanceled(generation.Shutdown.Token);
                }
                catch (Exception ex)
                {
                    if (ex is SqlException sqlException && IsTimeout(sqlException))
                    {
                        Log.Warning(
                            "Database background job timed out: {Operation}. Number={Number}, State={State}, Class={Class}",
                            job.Operation,
                            sqlException.Number,
                            sqlException.State,
                            sqlException.Class);
                    }
                    else
                    {
                        Log.Error(ex, "Database job failed: {Operation}", job.Operation);
                    }

                    job.Completion?.TrySetException(ex);
                }
            }
        }
        catch (OperationCanceledException) when (generation.Shutdown.IsCancellationRequested)
        {
            while (generation.Queue.Reader.TryRead(out var job))
                job.Completion?.TrySetCanceled(generation.Shutdown.Token);
        }
    }

    private static bool IsTransient(SqlException exception)
    {
        foreach (SqlError error in exception.Errors)
        {
            if (error.Number is
                -2 or 20 or 64 or 233 or
                10053 or 10054 or 10060 or
                10928 or 10929 or
                40197 or 40501 or 40613 or
                49918 or 49919 or 49920)
            {
                return true;
            }
        }

        return false;
    }

    private static bool IsTimeout(SqlException exception)
    {
        return exception.Number == -2;
    }
}
