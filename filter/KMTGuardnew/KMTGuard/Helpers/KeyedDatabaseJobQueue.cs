using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

namespace KMTGuard.Helpers
{
    internal static class KeyedDatabaseJobQueue
    {
        private sealed class KeyState
        {
            internal Task Tail = Task.CompletedTask;
            internal int References;
        }

        private static readonly object Sync = new();
        private static readonly Dictionary<int, KeyState> States = new();

        internal static bool TryQueueBackground(
            int key,
            Func<CancellationToken, Task> action,
            string operation)
        {
            KeyState state;
            Task predecessor;
            var completion = new TaskCompletionSource<bool>(TaskCreationOptions.RunContinuationsAsynchronously);
            lock (Sync)
            {
                if (!States.TryGetValue(key, out state))
                {
                    state = new KeyState();
                    States.Add(key, state);
                }

                predecessor = state.Tail;
                state.Tail = completion.Task;
                state.References++;
            }

            bool queued = DatabaseJobQueue.TryQueueBackground(async cancellationToken =>
            {
                try
                {
                    await predecessor.WaitAsync(cancellationToken).ConfigureAwait(false);
                    await action(cancellationToken).ConfigureAwait(false);
                }
                finally
                {
                    Complete(key, state, completion);
                }
            }, operation);

            if (!queued)
                Complete(key, state, completion);
            return queued;
        }

        private static void Complete(
            int key,
            KeyState state,
            TaskCompletionSource<bool> completion)
        {
            if (!completion.TrySetResult(true))
                return;

            lock (Sync)
            {
                state.References--;
                if (state.References == 0 &&
                    States.TryGetValue(key, out var current) &&
                    ReferenceEquals(current, state))
                {
                    States.Remove(key);
                }
            }
        }
    }
}
