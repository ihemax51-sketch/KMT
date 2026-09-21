SET NOCOUNT ON;
SET XACT_ABORT ON;

IF OBJECT_ID(N'dbo.UniqueHistory', N'U') IS NOT NULL
BEGIN
    UPDATE history
       SET MapIndex = worldInfo.WorldIndex,
           MapType = CASE
                         WHEN history.RegionID > 32767 OR worldInfo.WorldIndex > 1 THEN 2
                         ELSE 0
                     END,
           UpdatedAt = SYSUTCDATETIME()
      FROM dbo.UniqueHistory AS history
      CROSS APPLY
      (
          VALUES
          (
              CONVERT(int,
                  (CONVERT(bigint, history.WorldID) & CONVERT(bigint, 4294901760)) / 65536)
          )
      ) AS worldInfo(WorldIndex)
     WHERE history.MapIndex <> worldInfo.WorldIndex
        OR history.MapType <> CASE
                                 WHEN history.RegionID > 32767 OR worldInfo.WorldIndex > 1 THEN 2
                                 ELSE 0
                             END;
END;
