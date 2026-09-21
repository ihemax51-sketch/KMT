using System.Collections.Concurrent;
using System.Globalization;
using System.Text.Json;
using Dapper;
using KMTGuard.Database.Models;
using KMTGuard.Helpers;
using KMTGuard.ServerManagers;
using Microsoft.Data.SqlClient;
using Serilog;
using SilkroadSecurityAPI;

namespace KMTGuard.Features.UniqueHistory;

public static class UniqueHistoryService
{
    public const int MaximumDpsEntries = 8;

    private sealed class StoredUniqueHistory
    {
        public int UniqueID { get; init; }
        public string KillerName { get; init; } = string.Empty;
        public byte State { get; init; }
        public long EventTime { get; init; }
        public int RegionID { get; init; }
        public float PosX { get; init; }
        public float PosY { get; init; }
        public float PosZ { get; init; }
        public int WorldID { get; init; }
        public byte MapType { get; init; }
        public int MapIndex { get; init; }
        public string DpsJson { get; init; } = "{}";
    }

    public static _UniqueHistory CreateKilled(
        int uniqueId,
        string killerName,
        long eventTime,
        int regionId,
        float x,
        float y,
        float z,
        int worldId,
        IEnumerable<KeyValuePair<string, int>> dps)
    {
        var worldIndex = GetWorldIndex(worldId);
        return new _UniqueHistory
        {
            UniqueID = uniqueId,
            KillerName = killerName ?? string.Empty,
            State = 0,
            Time = eventTime,
            KilledRegionID = regionId,
            KilledX = x,
            KilledY = y,
            KilledZ = z,
            WorldID = worldId,
            MapType = GetMapType(regionId, worldIndex),
            MapIndex = worldIndex,
            DMGMETER = BuildDpsSnapshot(dps)
        };
    }

    public static _UniqueHistory CreateAlive(int uniqueId, long eventTime)
    {
        return new _UniqueHistory
        {
            UniqueID = uniqueId,
            KillerName = string.Empty,
            State = 1,
            Time = eventTime,
            DMGMETER = new ConcurrentDictionary<string, int>(StringComparer.OrdinalIgnoreCase)
        };
    }

    public static _UniqueHistory SetKilled(_UniqueHistory value)
    {
        RefManager.UniqueLog.AddOrUpdate(value.UniqueID, value, (_, _) => value);
        return value;
    }

    public static _UniqueHistory SetAlive(int uniqueId, long eventTime)
    {
        var value = CreateAlive(uniqueId, eventTime);
        RefManager.UniqueLog.AddOrUpdate(uniqueId, value, (_, _) => value);
        return value;
    }

    public static _UniqueHistory? FillMissingKiller(int uniqueId, string killerName)
    {
        if (!RefManager.UniqueLog.TryGetValue(uniqueId, out var current) ||
            current.State != 0 ||
            !string.IsNullOrWhiteSpace(current.KillerName))
        {
            return null;
        }

        var updated = Clone(current, killerName);
        RefManager.UniqueLog.TryUpdate(uniqueId, updated, current);
        return RefManager.UniqueLog.TryGetValue(uniqueId, out var result) ? result : null;
    }

    public static Packet CreateUpdatePacket(_UniqueHistory value)
    {
        var packet = new Packet(0x208A);
        WriteEntry(packet, value);
        return packet;
    }

    public static void WriteEntry(Packet packet, _UniqueHistory value)
    {
        packet.WriteInt32(value.UniqueID);
        packet.WriteUnicode(value.KillerName ?? string.Empty);
        packet.WriteUInt8(value.State);
        packet.WriteInt64(value.Time);
        packet.WriteInt32(value.KilledRegionID);
        packet.WriteFloat(value.KilledX);
        packet.WriteFloat(value.KilledY);
        packet.WriteFloat(value.KilledZ);
        packet.WriteInt32(value.WorldID);
        packet.WriteUInt8(value.MapType);
        packet.WriteInt32(value.MapIndex);

        var dpsEntries = value.DMGMETER
            .Where(x => !string.IsNullOrWhiteSpace(x.Key))
            .OrderByDescending(x => x.Value)
            .ThenBy(x => x.Key, StringComparer.OrdinalIgnoreCase)
            .Take(MaximumDpsEntries)
            .ToList();

        packet.WriteUInt8((byte)dpsEntries.Count);
        foreach (var dps in dpsEntries)
        {
            packet.WriteAscii(dps.Key);
            packet.WriteAscii(FormatDamage(dps.Value));
        }
    }

    public static async Task LoadAsync()
    {
        try
        {
            using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            var rows = await connection.QueryAsync<StoredUniqueHistory>("""
                SELECT UniqueID, KillerName, State, EventTime, RegionID,
                       PosX, PosY, PosZ, WorldID, MapType, MapIndex, DpsJson
                FROM dbo.UniqueHistory WITH (NOLOCK)
                """);

            var loaded = new ConcurrentDictionary<int, _UniqueHistory>();
            foreach (var row in rows)
            {
                Dictionary<string, int>? dps = null;
                try
                {
                    dps = JsonSerializer.Deserialize<Dictionary<string, int>>(row.DpsJson);
                }
                catch (JsonException ex)
                {
                    Log.Warning(ex, "Ignored invalid DPS history for unique {UniqueID}", row.UniqueID);
                }

                loaded[row.UniqueID] = new _UniqueHistory
                {
                    UniqueID = row.UniqueID,
                    KillerName = row.KillerName,
                    State = row.State,
                    Time = row.EventTime,
                    KilledRegionID = row.RegionID,
                    KilledX = row.PosX,
                    KilledY = row.PosY,
                    KilledZ = row.PosZ,
                    WorldID = row.WorldID,
                    MapType = GetMapType(row.RegionID, GetWorldIndex(row.WorldID)),
                    MapIndex = GetWorldIndex(row.WorldID),
                    DMGMETER = BuildDpsSnapshot(dps ?? Enumerable.Empty<KeyValuePair<string, int>>())
                };
            }

            RefManager.UniqueLog = loaded;
            Log.Information("Unique history restored :: records={RecordCount:N0}", loaded.Count);
        }
        catch (SqlException ex)
        {
            Log.Warning(ex, "Unique history could not be restored; apply the current database update");
        }
    }

    public static async Task PersistAsync(_UniqueHistory value)
    {
        try
        {
            var dpsJson = JsonSerializer.Serialize(value.DMGMETER.ToDictionary(x => x.Key, x => x.Value));
            using var connection = new SqlConnection(Program.Connectionstring);
            await connection.OpenAsync();
            await connection.ExecuteAsync("""
                MERGE dbo.UniqueHistory WITH (HOLDLOCK) AS target
                USING (SELECT @UniqueID AS UniqueID) AS source
                   ON target.UniqueID = source.UniqueID
                WHEN MATCHED THEN UPDATE SET
                    KillerName = @KillerName, State = @State, EventTime = @Time,
                    RegionID = @KilledRegionID, PosX = @KilledX, PosY = @KilledY,
                    PosZ = @KilledZ, WorldID = @WorldID, MapType = @MapType,
                    MapIndex = @MapIndex, DpsJson = @DpsJson, UpdatedAt = SYSUTCDATETIME()
                WHEN NOT MATCHED THEN INSERT
                    (UniqueID, KillerName, State, EventTime, RegionID, PosX, PosY,
                     PosZ, WorldID, MapType, MapIndex, DpsJson)
                VALUES
                    (@UniqueID, @KillerName, @State, @Time, @KilledRegionID,
                     @KilledX, @KilledY, @KilledZ, @WorldID, @MapType, @MapIndex, @DpsJson);
                """, new
            {
                value.UniqueID,
                value.KillerName,
                value.State,
                value.Time,
                value.KilledRegionID,
                value.KilledX,
                value.KilledY,
                value.KilledZ,
                value.WorldID,
                value.MapType,
                value.MapIndex,
                DpsJson = dpsJson
            });
        }
        catch (SqlException ex)
        {
            Log.Error(ex, "Failed to persist unique history record {UniqueID}", value.UniqueID);
        }
    }

    private static ConcurrentDictionary<string, int> BuildDpsSnapshot(
        IEnumerable<KeyValuePair<string, int>> dps)
    {
        var result = new ConcurrentDictionary<string, int>(StringComparer.OrdinalIgnoreCase);
        foreach (var entry in dps)
        {
            if (!string.IsNullOrWhiteSpace(entry.Key))
                result.AddOrUpdate(entry.Key, entry.Value, (_, existing) => Math.Max(existing, entry.Value));
        }
        return result;
    }

    private static _UniqueHistory Clone(_UniqueHistory value, string killerName)
    {
        return new _UniqueHistory
        {
            UniqueID = value.UniqueID,
            KillerName = killerName,
            State = value.State,
            Time = value.Time,
            KilledRegionID = value.KilledRegionID,
            KilledX = value.KilledX,
            KilledY = value.KilledY,
            KilledZ = value.KilledZ,
            WorldID = value.WorldID,
            MapType = value.MapType,
            MapIndex = value.MapIndex,
            DMGMETER = BuildDpsSnapshot(value.DMGMETER)
        };
    }

    private static string FormatDamage(long value)
    {
        if (value >= 100_000_000) return (value / 1_000_000D).ToString("0.#M", CultureInfo.InvariantCulture);
        if (value >= 1_000_000) return (value / 1_000_000D).ToString("0.##M", CultureInfo.InvariantCulture);
        if (value >= 100_000) return (value / 1_000D).ToString("0.#k", CultureInfo.InvariantCulture);
        if (value >= 10_000) return (value / 1_000D).ToString("0.##k", CultureInfo.InvariantCulture);
        return value.ToString(CultureInfo.InvariantCulture);
    }

    private static int GetWorldIndex(int worldId)
    {
        // SWorldID is packed as { WORD LayerID, WORD WorldID }. The lower
        // word identifies a runtime layer and can change between instances;
        // the upper word identifies the actual map definition.
        return (int)((uint)worldId >> 16);
    }

    private static byte GetMapType(int regionId, int worldIndex)
    {
        return (byte)((regionId > short.MaxValue || worldIndex > 1) ? 2 : 0);
    }
}
