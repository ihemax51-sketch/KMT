using Dapper;
using Microsoft.Data.SqlClient;

namespace KMTGuard.Features.AutoEvents;

internal static class PartyDataLocator
{
    internal const string QualifiedTableName = "[dbo].[Party_Members]";
    private static readonly SemaphoreSlim ResolveLock = new(1, 1);
    private static string? _qualifiedTableName;

    public static async Task<string> GetTableNameAsync(SqlConnection connection)
    {
        if (_qualifiedTableName != null)
            return _qualifiedTableName;

        await ResolveLock.WaitAsync();
        try
        {
            if (_qualifiedTableName != null)
                return _qualifiedTableName;

            var tableExists = await connection.ExecuteScalarAsync<int>(@"
SELECT CASE
    WHEN OBJECT_ID(N'dbo.Party_Members', N'U') IS NOT NULL THEN 1
    ELSE 0
END;");

            if (tableExists != 1)
            {
                throw new InvalidOperationException(
                    $"dbo.Party_Members was not found in the configured filter database '{connection.Database}'.");
            }

            _qualifiedTableName = QualifiedTableName;
            return _qualifiedTableName;
        }
        finally
        {
            ResolveLock.Release();
        }
    }
}
