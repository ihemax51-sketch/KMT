using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Dapper;
using KMTGuard.Database.Models;
using KMTGuard.Database.RankModels;
using Microsoft.Data.SqlClient;
using Serilog;

namespace KMTGuard.ServerManagers
{
    public class RankManager
    {
        public static ConcurrentDictionary<int, _SilkRank> m_SilkRank { get; private set; } = new();

        public static async Task Initialize()
        {
            await LoadSilkRank();

        }
        public static async Task LoadSilkRank()
        {
            try
            {
                // Diğer önbellekleri de temizleyin
                // Örnek: m_AnotherReferenceTable.Clear();

                using (var connection = new SqlConnection(Program.Connectionstring))
                {
                    await connection.OpenAsync();

                    // İlk sorgu: _ActiveTitleColors tablosu
                    string query1 = "SELECT * FROM [dbo].[Rank_Silk] with (nolock)";
                    var result1 = await connection.QueryAsync<_SilkRank>(query1);
                    m_SilkRank = new ConcurrentDictionary<int, _SilkRank>(
                        result1.GroupBy(item => item.CharID)
                            .ToDictionary(group => group.Key, group => group.First()));
                    //Log.Warning("LoadSilkRank loaded into cache. Total count: " + m_SilkRank.Count);
                }
            }
            catch (Exception ex)
            {
                Log.Warning($"Error LoadSilkRank tables: {ex.Message}");
            }
        }
    }
}
