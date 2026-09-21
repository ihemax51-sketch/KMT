# Dynamic Ranking SQL Guide

The Dynamic Ranking window contains nine generic SQL-controlled slots. Nothing
in the filter awards Unique, Honor, job, event, or other points automatically.
The server owner decides what every slot means and writes its score data.

## Tables

| Category ID | Data table |
| --- | --- |
| 1 | `KMTGuard.dbo.Rank_Data01` |
| 2 | `KMTGuard.dbo.Rank_Data02` |
| 3 | `KMTGuard.dbo.Rank_Data03` |
| 4 | `KMTGuard.dbo.Rank_Data04` |
| 5 | `KMTGuard.dbo.Rank_Data05` |
| 6 | `KMTGuard.dbo.Rank_Data06` |
| 7 | `KMTGuard.dbo.Rank_Data07` |
| 8 | `KMTGuard.dbo.Rank_Data08` |
| 9 | `KMTGuard.dbo.Rank_Data09` |

Each table has the same contract:

```text
CharID int
CharName16 varchar(16)
GuildName varchar(16)
Point int
```

`CharID` and `CharName16` must identify the same current character. The filter
deliberately rejects stale rows when a deleted character ID has been reused.

## Configure a slot

```sql
EXEC KMTGuard.dbo.Rank_SetCategory
    @CategoryID = 1,
    @Category = 'Arena Wins',
    @Active = 1;
```

Category names must be unique. Set `@Active = 0` to hide a slot.
Category inserts, edits, activation and deactivation automatically enqueue an
immediate filter refresh, so a disabled category disappears without waiting for
the 10-minute fallback timer.

## Safely write a score

Use `Rank_UpsertEntry` when updating one player. It resolves the current
character and guild names and prevents a reused `CharID` from inheriting an old
row.

```sql
EXEC KMTGuard.dbo.Rank_UpsertEntry
    @CategoryID = 1,
    @CharID = 1234,
    @Point = 25;
```

For a bulk rebuild, populate the matching `Rank_DataXX` table inside one SQL
transaction. Always copy the current `CharID` and `CharName16` together from
the configured shard database.

## Refresh behavior

The Agent filter atomically reloads all nine tables and the category list every
10 minutes. A failed refresh keeps the complete previous snapshot; it never
publishes a half-loaded mix.

The interval is controlled by the `DynamicRankingRefreshMinutes` row in
`KMTGuard.dbo.System_Settings` and is clamped to 1–1440 minutes. Its installed
default is `10`.

For an immediate refresh:

```sql
EXEC KMTGuard.dbo.Rank_RequestImmediateRefresh;
```

The existing `CommandID = 40` path remains compatible. Players receive the new
snapshot the next time they press **Result**; ranking data is not pushed while
the window is idle.

## Reset and recovery

This procedure archives every current row before clearing the nine slots:

```sql
EXEC KMTGuard.dbo.Rank_ArchiveAndResetAll
    @Reason = N'Rebuilding custom ranking systems';
```

Archived data remains in `KMTGuard.dbo.Rank_DataArchive`.
