using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using Microsoft.Data.SqlClient;
using Serilog;

namespace KMTGuard.ChatFiltering
{
    public static class ChatWordFilter
    {
        private const int TTL_SECONDS = 10;

        private static long _lastReloadUnix = 0;
        private static BlockedWordRule[] _rules = Array.Empty<BlockedWordRule>();
        private static readonly SemaphoreSlim _reloadLock = new(1, 1);

        private static long UnixNow() => DateTimeOffset.UtcNow.ToUnixTimeSeconds();

        private static bool IsExpired(long lastReloadUnix)
            => (UnixNow() - lastReloadUnix) >= TTL_SECONDS;

        private static async Task ReloadIfNeededAsync()
        {
            var last = Interlocked.Read(ref _lastReloadUnix);
            var snapshot = Volatile.Read(ref _rules);

            if (!IsExpired(last) && snapshot.Length > 0)
                return;

            await _reloadLock.WaitAsync().ConfigureAwait(false);
            try
            {
                last = Interlocked.Read(ref _lastReloadUnix);
                snapshot = Volatile.Read(ref _rules);

                if (!IsExpired(last) && snapshot.Length > 0)
                    return;

                var fetched = await FetchRulesFromDbAsync().ConfigureAwait(false);

                Volatile.Write(ref _rules, fetched);
                Interlocked.Exchange(ref _lastReloadUnix, UnixNow());
            }
            finally
            {
                _reloadLock.Release();
            }
        }

        private static async Task<BlockedWordRule[]> FetchRulesFromDbAsync()
        {
            var list = new List<BlockedWordRule>(256);

            try
            {
                using var conn = new SqlConnection(Program.Connectionstring);
                await conn.OpenAsync().ConfigureAwait(false);

                using var cmd = conn.CreateCommand();
                cmd.CommandText = @"
SELECT Word, MatchMode
FROM [KMTGuard].[dbo].[Security_BlockedWords] WITH (NOLOCK)
WHERE IsActive = 1;";

                using var r = await cmd.ExecuteReaderAsync().ConfigureAwait(false);
                while (await r.ReadAsync().ConfigureAwait(false))
                {
                    var word = r.GetString(0)?.Trim();
                    if (string.IsNullOrWhiteSpace(word))
                        continue;

                    var modeObj = r.GetValue(1);
                    byte mode = (modeObj is byte b) ? b : Convert.ToByte(modeObj);

                    list.Add(new BlockedWordRule(word, mode));
                }
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "[ChatWordFilter] Failed to load blocked words from DB.");
            }

            return list
                .OrderByDescending(x => x.Word.Length)
                .ToArray();
        }

        public static async Task<bool> IsBlockedAsync(string message)
        {
            if (string.IsNullOrWhiteSpace(message))
                return false;

            await ReloadIfNeededAsync().ConfigureAwait(false);

            var rules = Volatile.Read(ref _rules);
            if (rules.Length == 0)
                return false;

            for (int i = 0; i < rules.Length; i++)
            {
                var rule = rules[i];
                var w = rule.Word;

                switch (rule.MatchMode)
                {
                    case 0: // Contains
                        if (message.IndexOf(w, StringComparison.OrdinalIgnoreCase) >= 0)
                            return true;
                        break;

                    case 1: // WholeWord
                        if (Regex.IsMatch(
                                message,
                                $@"\b{Regex.Escape(w)}\b",
                                RegexOptions.IgnoreCase | RegexOptions.CultureInvariant))
                            return true;
                        break;

                    case 2: // StartsWith
                        if (message.StartsWith(w, StringComparison.OrdinalIgnoreCase))
                            return true;
                        break;

                    case 3: // EndsWith
                        if (message.EndsWith(w, StringComparison.OrdinalIgnoreCase))
                            return true;
                        break;

                    default:
                        if (message.IndexOf(w, StringComparison.OrdinalIgnoreCase) >= 0)
                            return true;
                        break;
                }
            }

            return false;
        }
    }
}
