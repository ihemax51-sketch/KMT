using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;


namespace KMTGuard.Helpers
{
    public sealed class DelayedJobManager : IDisposable
    {
        private readonly List<DelayedJobItem> _jobs = new List<DelayedJobItem>();
        private readonly object _lock = new object();
        private readonly object _lifecycleLock = new object();
        private readonly SemaphoreSlim _wakeUp = new(0);
        private CancellationTokenSource? _shutdown;
        private Task? _worker;
        private bool _disposed;

        public void Run()
        {
            lock (_lifecycleLock)
            {
                ObjectDisposedException.ThrowIf(_disposed, this);
                if (_worker is { IsCompleted: false })
                    return;

                _shutdown?.Dispose();
                _shutdown = new CancellationTokenSource();
                var cancellationToken = _shutdown.Token;
                _worker = Task.Run(() => ThreadWorkerAsync(cancellationToken));
            }
        }

        public void Stop()
        {
            Task? worker;
            CancellationTokenSource? shutdown;
            lock (_lifecycleLock)
            {
                worker = _worker;
                shutdown = _shutdown;
                _worker = null;
                _shutdown = null;
            }

            if (shutdown == null)
                return;

            try { shutdown.Cancel(); } catch { }
            SignalWorker();
            try { worker?.Wait(TimeSpan.FromSeconds(2)); } catch { }
            shutdown.Dispose();
        }

        public void CreateJob(DelayedJobItem jobItem)
        {
            ArgumentNullException.ThrowIfNull(jobItem);
            lock (_lifecycleLock)
            {
                ObjectDisposedException.ThrowIf(_disposed, this);
                lock (_lock)
                {
                    jobItem.RegisterTime = DateTime.UtcNow;
                    _jobs.Add(jobItem);
                }

                SignalWorker();
            }
        }

        private async Task ThreadWorkerAsync(CancellationToken cancellationToken)
        {
            try
            {
                while (!cancellationToken.IsCancellationRequested)
                {
                    while (_wakeUp.Wait(0)) { }

                    List<DelayedJobItem> executedJobs = new();
                    DateTime? nextExecutionUtc = null;
                    lock (_lock)
                    {
                        var currentTime = DateTime.UtcNow;
                        for (var index = _jobs.Count - 1; index >= 0; index--)
                        {
                            var job = _jobs[index];
                            var dueAtUtc = job.RegisterTime.AddMilliseconds(Math.Max(0, job.ExecAfterMs));
                            if (dueAtUtc <= currentTime)
                            {
                                executedJobs.Add(job);
                                _jobs.RemoveAt(index);
                            }
                            else if (!nextExecutionUtc.HasValue || dueAtUtc < nextExecutionUtc.Value)
                            {
                                nextExecutionUtc = dueAtUtc;
                            }
                        }
                    }

                    if (executedJobs.Count > 0)
                    {
                        executedJobs.Reverse();
                        foreach (var job in executedJobs)
                        {
                            try
                            {
                                await job.Handler(job.Session, job.Param);
                            }
                            catch (Exception ex)
                            {
                                Serilog.Log.Error(ex, "Delayed gameplay job failed.");
                            }
                        }

                        continue;
                    }

                    if (!nextExecutionUtc.HasValue)
                    {
                        await _wakeUp.WaitAsync(cancellationToken);
                        continue;
                    }

                    var wait = nextExecutionUtc.Value - DateTime.UtcNow;
                    var waitMilliseconds = wait <= TimeSpan.Zero
                        ? 0
                        : (int)Math.Min(int.MaxValue, Math.Ceiling(wait.TotalMilliseconds));
                    await _wakeUp.WaitAsync(waitMilliseconds, cancellationToken);
                }
            }
            catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested) { }
        }

        private void SignalWorker()
        {
            try { _wakeUp.Release(); }
            catch (ObjectDisposedException) { }
        }

        public void Dispose()
        {
            lock (_lifecycleLock)
            {
                if (_disposed)
                    return;
                _disposed = true;
            }

            Stop();
            _wakeUp.Dispose();
        }
    }


}
