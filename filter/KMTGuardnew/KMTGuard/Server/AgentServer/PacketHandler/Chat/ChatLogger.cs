using System;
using System.Data;
using System.Threading.Tasks;
using Microsoft.Data.SqlClient;
using Serilog;

namespace KMTGuard.Database
{
    public static class ChatLogger
    {
        private static string ConnStr => Program.Connectionstring;

        public static async Task LogAsync(string sender, string? receiver, byte chatType, string message)
        {
            try
            {
                using var conn = new SqlConnection(ConnStr);
                using var cmd = conn.CreateCommand();

                // افتح الاتصال
                await conn.OpenAsync().ConfigureAwait(false);

                // وقت محلي بلا ملي ثانية (ساعات:دقائق:ثواني)—نفس السلوك الحالي
                DateTime now = DateTime.Now;
                var ts = new DateTime(now.Year, now.Month, now.Day, now.Hour, now.Minute, now.Second);

                cmd.CommandText = @"
INSERT INTO [KMTGuard].[dbo].[Log_Chat] ([Timestamp],[Sender],[Receiver],[ChatType],[Message])
VALUES (@Timestamp, @Sender, @Receiver, @ChatType, @Message)";
                cmd.CommandType = CommandType.Text;
                cmd.CommandTimeout = 5;

                // @Timestamp
                {
                    var p = cmd.Parameters.Add(new SqlParameter("@Timestamp", SqlDbType.DateTime2));
                    p.Value = ts;
                }

                // @Sender (NVARCHAR(64))
                {
                    var p = cmd.Parameters.Add(new SqlParameter("@Sender", SqlDbType.NVarChar, 64));
                    p.Value = sender ?? string.Empty;
                }

                // @Receiver (NVARCHAR(64) nullable)
                {
                    var p = cmd.Parameters.Add(new SqlParameter("@Receiver", SqlDbType.NVarChar, 64));
                    p.Value = (object?)receiver ?? DBNull.Value;
                }

                // @ChatType (TINYINT)
                {
                    var p = cmd.Parameters.Add(new SqlParameter("@ChatType", SqlDbType.TinyInt));
                    p.Value = chatType;
                }

                // @Message (NVARCHAR(MAX))
                {
                    var p = cmd.Parameters.Add(new SqlParameter("@Message", SqlDbType.NVarChar, -1));
                    p.Value = message ?? string.Empty;
                }

                await cmd.ExecuteNonQueryAsync().ConfigureAwait(false);
            }
            catch (Exception ex)
            {
                Log.Warning(ex, "[ChatLogger] Insert failed");
            }
        }
    }
}
