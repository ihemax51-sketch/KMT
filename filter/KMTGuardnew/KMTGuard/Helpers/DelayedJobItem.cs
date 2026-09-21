namespace KMTGuard.Helpers
{
    public class DelayedJobItem
    {
        public DateTime RegisterTime { get; set; }
        public int ExecAfterMs { get; set; }
        public object Session { get; set; }
        public object? Param { get; set; }
        public Func<object, object?, Task> Handler { get; set; }

        public DelayedJobItem(
            int execAfterMs,
            object session,
            object? param,
            Func<object, object?, Task> handler)
        {
            ExecAfterMs = execAfterMs;
            Session = session;
            Param = param;
            Handler = handler;
            RegisterTime = DateTime.UtcNow;
        }
    }
}
