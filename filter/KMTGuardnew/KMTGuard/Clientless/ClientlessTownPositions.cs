namespace KMTGuard.Clientless;

/// <summary>
/// Stable town parking points for managed vSRO characters. Every active city
/// group is spread across five safe town areas with an explicit separation
/// target, so both new and previously imported accounts use the same layout.
/// </summary>
internal static class ClientlessTownPositions
{
    internal readonly record struct Position(int RegionID, float X, float Y, float Z);
    private readonly record struct Area(int RegionID, float X, float Y, float Z, float RadiusX, float RadiusZ);

    internal static Position Resolve(string? city, int accountId = 0)
    {
        var areas = ResolveAreas(city);
        var stableId = Math.Max(1, accountId);
        var area = areas[(stableId - 1) % areas.Length];
        var hash = unchecked((uint)stableId * 2654435761u + 2246822519u);
        hash ^= hash >> 16;
        var angleUnit = (hash & 0xFFFFu) / 65535d;
        hash = unchecked(hash * 3266489917u + 668265263u);
        hash ^= hash >> 15;
        var radiusUnit = ((hash & 0xFFFFu) + 1d) / 65536d;
        var angle = angleUnit * Math.PI * 2d;
        var radial = Math.Sqrt(0.08d + (radiusUnit * 0.84d));

        return new Position(
            area.RegionID,
            Math.Clamp(area.X + (float)(Math.Cos(angle) * area.RadiusX * radial), 16f, 1904f),
            area.Y,
            Math.Clamp(area.Z + (float)(Math.Sin(angle) * area.RadiusZ * radial), 16f, 1904f));
    }

    internal static IReadOnlyList<Position> ResolveAll(string? city, int count)
    {
        var requestedCount = Math.Max(0, count);
        if (requestedCount == 0)
            return Array.Empty<Position>();

        var areas = ResolveAreas(city);
        var minimumDistance = requestedCount switch
        {
            <= 25 => 110f,
            <= 50 => 80f,
            <= 100 => 58f,
            <= 250 => 38f,
            <= 500 => 27f,
            <= 1000 => 19f,
            <= 2500 => 11f,
            _ => 7.5f
        };
        var minimumDistanceSquared = minimumDistance * minimumDistance;
        var cellSize = minimumDistance;
        var positions = new List<Position>(requestedCount);
        var spatialGrid = new Dictionary<(int X, int Z), List<(float X, float Z)>>();
        var random = new Random(CreateStableSeed(city));

        static (float X, float Z) ToWorldPosition(Position position)
        {
            const float regionSize = 1920f;
            return (
                ((position.RegionID & 0xFF) * regionSize) + position.X,
                (((position.RegionID >> 8) & 0xFF) * regionSize) + position.Z);
        }

        (int X, int Z) CellFor(float x, float z) =>
            ((int)Math.Floor(x / cellSize), (int)Math.Floor(z / cellSize));

        float FindNearestDistanceSquared(float x, float z)
        {
            var cell = CellFor(x, z);
            var nearest = float.MaxValue;
            for (var cellX = cell.X - 1; cellX <= cell.X + 1; cellX++)
            {
                for (var cellZ = cell.Z - 1; cellZ <= cell.Z + 1; cellZ++)
                {
                    if (!spatialGrid.TryGetValue((cellX, cellZ), out var nearby))
                        continue;

                    foreach (var point in nearby)
                    {
                        var dx = point.X - x;
                        var dz = point.Z - z;
                        nearest = Math.Min(nearest, (dx * dx) + (dz * dz));
                    }
                }
            }

            return nearest;
        }

        void AddPosition(Position position)
        {
            positions.Add(position);
            var worldPosition = ToWorldPosition(position);
            var cell = CellFor(worldPosition.X, worldPosition.Z);
            if (!spatialGrid.TryGetValue(cell, out var bucket))
            {
                bucket = new List<(float X, float Z)>();
                spatialGrid[cell] = bucket;
            }

            bucket.Add(worldPosition);
        }

        for (var index = 0; index < requestedCount; index++)
        {
            Position? accepted = null;
            Position? bestFallback = null;
            var bestNearestDistanceSquared = -1f;
            for (var attempt = 0; attempt < 960; attempt++)
            {
                var area = areas[(index + attempt) % areas.Length];
                var angle = random.NextDouble() * Math.PI * 2d;
                var radial = Math.Sqrt(0.01d + (random.NextDouble() * 0.99d));
                var x = area.X + (float)(Math.Cos(angle) * area.RadiusX * radial);
                var z = area.Z + (float)(Math.Sin(angle) * area.RadiusZ * radial);
                x += (float)((random.NextDouble() - 0.5d) * 4d);
                z += (float)((random.NextDouble() - 0.5d) * 4d);

                if (x is < 16f or > 1904f || z is < 16f or > 1904f)
                    continue;

                var candidate = new Position(area.RegionID, x, area.Y, z);
                var worldPosition = ToWorldPosition(candidate);
                var nearestDistanceSquared = FindNearestDistanceSquared(worldPosition.X, worldPosition.Z);
                if (nearestDistanceSquared > bestNearestDistanceSquared)
                {
                    bestNearestDistanceSquared = nearestDistanceSquared;
                    bestFallback = candidate;
                }

                if (nearestDistanceSquared < minimumDistanceSquared)
                    continue;

                accepted = candidate;
                break;
            }

            if (!accepted.HasValue)
            {
                var fallbackArea = areas[index % areas.Length];
                accepted = bestFallback ?? new Position(
                    fallbackArea.RegionID,
                    fallbackArea.X,
                    fallbackArea.Y,
                    fallbackArea.Z);
            }

            AddPosition(accepted.Value);
        }

        return positions;
    }

    private static int CreateStableSeed(string? city)
    {
        var hash = 2166136261u;
        foreach (var character in (city ?? string.Empty).Trim().ToUpperInvariant())
        {
            hash ^= character;
            hash = unchecked(hash * 16777619u);
        }

        return unchecked((int)(hash ^ 0x4B4D5455u));
    }

    private static Area[] ResolveAreas(string? city) => (city ?? string.Empty).Trim().ToLowerInvariant() switch
    {
        "donwhang" =>
        [
            new(26265, 957f, -80f, 1508f, 220f, 180f), new(26265, 600f, -105f, 700f, 190f, 160f),
            new(26265, 1300f, -105f, 700f, 190f, 160f), new(26265, 1000f, -105f, 300f, 190f, 150f),
            new(26521, 900f, -95f, 1700f, 190f, 150f)
        ],
        "hotan" =>
        [
            new(23687, 1138f, 245f, 600f, 220f, 180f), new(23431, 1100f, 245f, 1700f, 200f, 150f),
            new(23686, 1073f, 13f, 475f, 180f, 140f), new(23688, 1254f, 14f, 480f, 180f, 140f),
            new(23943, 1145f, 145f, 1608f, 190f, 140f)
        ],
        "samarkand" =>
        [
            new(27244, 600f, 180f, 1400f, 210f, 170f), new(27243, 1550f, 180f, 1550f, 190f, 160f),
            new(27244, 500f, 180f, 500f, 190f, 160f), new(27499, 1400f, 180f, 500f, 200f, 160f),
            new(27500, 500f, 180f, 700f, 200f, 170f)
        ],
        "constantinople" =>
        [
            new(26959, 950f, 84f, 1070f, 220f, 180f), new(26958, 1400f, 84f, 900f, 200f, 170f),
            new(26957, 1450f, 80f, 1400f, 190f, 160f), new(26702, 900f, 84f, 1000f, 210f, 180f),
            new(27471, 1250f, 80f, 500f, 190f, 150f)
        ],
        "alexandria north" or "alexandria north (sd)" or "alexandrianorth" or "sd" or "sd2" =>
        [
            new(23603, 111f, 1537f, 524f, 90f, 160f), new(23603, 462f, 1530f, 241f, 180f, 150f),
            new(23603, 1180f, 1560f, 990f, 220f, 180f), new(23602, 829f, 1408f, 346f, 210f, 170f),
            new(23602, 1225f, 1448f, 529f, 210f, 170f)
        ],
        _ =>
        [
            new(25000, 995f, -32f, 1132f, 220f, 180f), new(24999, 1100f, 0f, 900f, 210f, 180f),
            new(25001, 850f, 0f, 1100f, 220f, 190f), new(25000, 1000f, 0f, 250f, 190f, 160f),
            new(25000, 1000f, 0f, 1700f, 190f, 160f)
        ]
    };
}
