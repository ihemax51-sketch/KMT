using System.Collections.Concurrent;
using Dapper;
using KMTGuard.SessionManager;
using Microsoft.Data.SqlClient;
using Serilog;

namespace KMTGuard.ServerManagers;

public static class ItemRegionRestrictionService
{
    private static readonly ConcurrentDictionary<RuleKey, CacheEntry> Cache = new();
    private static readonly SemaphoreSlim CacheLock = new(1, 1);
    private static readonly TimeSpan CacheTtl = TimeSpan.FromSeconds(2);
    private static bool _schemaReady;

    private readonly record struct RuleKey(int WorldID, int RegionID, int ItemID);
    private sealed record CacheEntry(bool IsAllowed, DateTime FetchedAt);

    private static async Task EnsureSchemaAsync()
    {
        if (_schemaReady) return;
        await CacheLock.WaitAsync();
        try
        {
            if (_schemaReady) return;
            await using var connection = new SqlConnection(Program.Connectionstring);
            await connection.ExecuteAsync(@"
                IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[Security_ItemRegionRestrictions]') AND type in (N'U'))
                BEGIN
                    CREATE TABLE [dbo].[Security_ItemRegionRestrictions](
                        [ID] INT IDENTITY(1,1) NOT NULL PRIMARY KEY,
                        [WorldID] INT NOT NULL DEFAULT 0,
                        [RegionID] INT NOT NULL,
                        [ItemID] INT NOT NULL,
                        [Enabled] BIT NOT NULL DEFAULT 1,
                        [Note] NVARCHAR(250) NULL
                    );
                    CREATE INDEX IX_Security_ItemRegionRestrictions ON [dbo].[Security_ItemRegionRestrictions] (WorldID, RegionID, ItemID);
                END
            ");
            _schemaReady = true;
        }
        catch (Exception ex)
        {
            Log.Error(ex, "[ItemRegionRestrictionService] Failed to ensure schema.");
        }
        finally
        {
            CacheLock.Release();
        }
    }

    public static async Task<bool> IsItemAllowedForDestinationAsync(int worldId, int regionId, int itemId)
    {
        regionId = RegionControlService.NormalizeRegionId(regionId);
        // ItemID 0 is the intentional wildcard used by Reverse destination rules.
        if (!RegionControlService.IsValidRegionId(regionId) || itemId < 0)
            return true;

        var key = new RuleKey(Math.Max(0, worldId), regionId, itemId);
        
        if (Cache.TryGetValue(key, out var cached) && DateTime.UtcNow - cached.FetchedAt < CacheTtl)
            return cached.IsAllowed;

        await EnsureSchemaAsync();

        try
        {
            await using var connection = new SqlConnection(Program.Connectionstring);
            const string sql = @"
                SELECT TOP 1 Enabled
                FROM [dbo].[Security_ItemRegionRestrictions] WITH (NOLOCK)
                WHERE RegionID = @RegionID
                  AND (ItemID = @ItemID OR ItemID = 0)
                  AND (WorldID = @WorldID OR WorldID = 0)
                ORDER BY 
                  CASE WHEN WorldID = @WorldID THEN 0 ELSE 1 END,
                  CASE WHEN ItemID = @ItemID THEN 0 ELSE 1 END,
                  ID DESC;";

            var rule = await connection.QueryFirstOrDefaultAsync<bool?>(
                sql,
                new { WorldID = key.WorldID, RegionID = key.RegionID, ItemID = key.ItemID });

            bool isAllowed = !(rule ?? false); // An enabled matching row means blocked.

            Cache[key] = new CacheEntry(isAllowed, DateTime.UtcNow);
            return isAllowed;
        }
        catch (Exception ex)
        {
            Log.Warning("[ItemRegionRestrictionService] read failed for WorldID={WorldID}, RegionID={RegionID}, ItemID={ItemID}: {Message}", 
                key.WorldID, key.RegionID, key.ItemID, ex.Message);
            
            // Fail open
            Cache[key] = new CacheEntry(true, DateTime.UtcNow);
            return true;
        }
    }
}
