using System;
using System.Collections.Concurrent;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using Dapper;
using Microsoft.Data.SqlClient;

namespace KMTGuard.Features.Skills
{
    public static class SkillRuleManager
    {
        private const int GeneralRegion = int.MinValue;
        private static ConcurrentDictionary<int, ConcurrentDictionary<int, SkillRule>> _rules
            = new();

        private static DateTime _lastLoadUtc = DateTime.MinValue;
        private static readonly TimeSpan _reloadInterval = TimeSpan.FromSeconds(10);
        private static int _loading = 0;

        public static async Task EnsureLoadedAsync()
        {
            if (DateTime.UtcNow - _lastLoadUtc < _reloadInterval) return;
            if (Interlocked.Exchange(ref _loading, 1) == 1) return;

            try
            {
                using var cn = new SqlConnection(Program.Connectionstring);
                await cn.OpenAsync();

                const string sql = @"
SELECT SkillID, RegionID, Blocked, DelaySeconds, MinLevel, OnlyInJob
FROM [dbo].[Security_SkillRules] WITH (NOLOCK);";

                var list = (await cn.QueryAsync<SkillRule>(sql)).ToList();

                var replacement =
                    new ConcurrentDictionary<int, ConcurrentDictionary<int, SkillRule>>();
                foreach (var r in list)
                {
                    var map = replacement.GetOrAdd(
                        r.SkillID, _ => new ConcurrentDictionary<int, SkillRule>());
                    map[r.RegionID ?? GeneralRegion] = r;
                }

                Volatile.Write(ref _rules, replacement);
                _lastLoadUtc = DateTime.UtcNow;
            }
            finally
            {
                Interlocked.Exchange(ref _loading, 0);
            }
        }

        public static bool TryGetBest(int skillId, int regionId, out SkillRule? rule)
        {
            rule = null;
            if (!_rules.TryGetValue(skillId, out var map) || map.Count == 0)
                return false;

            if (map.TryGetValue(regionId, out var regionRule))
            {
                rule = regionRule;
                return true;
            }

            if (map.TryGetValue(GeneralRegion, out var generalRule))
            {
                rule = generalRule;
                return true;
            }

            return false;
        }
    }
}
