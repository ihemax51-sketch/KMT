using System;
using System.Collections.Concurrent;

namespace KMTGuard.Features.Skills
{
    public static class CooldownService
    {
        private static readonly ConcurrentDictionary<string, DateTime> _until = new();

        public static bool IsOnCooldown(string key, out int remainingSeconds)
        {
            remainingSeconds = 0;
            if (_until.TryGetValue(key, out var until))
            {
                var now = DateTime.UtcNow;
                if (until > now)
                {
                    remainingSeconds = (int)Math.Ceiling((until - now).TotalSeconds);
                    return true;
                }
                _until.TryRemove(key, out _);
            }
            return false;
        }

        public static void SetCooldown(string key, TimeSpan duration)
        {
            _until[key] = DateTime.UtcNow.Add(duration);
        }
    }
}
