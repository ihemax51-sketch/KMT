# Runtime Honor Rank Refresh

This feature refreshes the Training Camp Honor Rank without restarting
`SR_ShardManager.exe` or any `SR_GameServer.exe`.

## Requirements

- `EnableAsyncDatabaseCommands` must be enabled for the ShardManager module.
- The current `KMTGuard_ShardManager.dll` must be loaded beside
  `SR_ShardManager.exe`.
- Run `database\v1.0.0\20260726_honor_rank_runtime_refresh.sql` once against the SQL Server instance.

The migration is idempotent and may be run again during an upgrade.

## Request a refresh

Run:

```sql
EXEC KMTGuard.dbo.Command_RefreshHonorRank
    @RequestedBy = N'Admin';
```

The procedure returns:

- `RequestID`: the queue and audit identifier.
- `Queued = 1`: a new request was created.
- `Queued = 0`: a refresh was already queued or running, so no duplicate was
  created.

The ShardManager polls the queue once per second. It executes
`SRO_VT_SHARD.dbo._TRAINING_CAMP_UPDATEHONORRANK` through a dedicated database
connection. Only a positive procedure result causes the ShardManager to
broadcast the native `0x3C80 / 0x0B` refresh notification to every GameServer.

## Check status

Latest requests:

```sql
EXEC KMTGuard.dbo.Command_GetHonorRankRefreshStatus;
```

One request:

```sql
EXEC KMTGuard.dbo.Command_GetHonorRankRefreshStatus
    @RequestID = 123;
```

Statuses:

- `Queued`: waiting for the ShardManager poller.
- `Running`: the rank procedure is executing.
- `Succeeded`: the database update succeeded and the native refresh was
  broadcast to all GameServers.
- `Failed`: no broadcast was sent; inspect `ProcedureResult` and
  `ErrorMessage`.

## Runtime behavior

Each GameServer uses its existing `AQ_TrainingCampQuery` flow to load the new
ranking. Its native reconciliation logic compares the old and new ranks,
removes an old Honor buff when required, applies the new buff, and skips work
when the rank did not change.

After that native query completes, the GameServer add-on copies the completed
ranking snapshot to opcode `0x34FE` and sends it once to every online player on
that GameServer. The client add-on passes the payload through the stock
`0xB478` Honor Ranking parser, so the native client cache and rows are replaced
using the original format. An already-visible Honor Ranking window is redrawn
immediately; a closed window stays closed.

The snapshot is published from the query-completion callback, not when subtype
`0x0B` first arrives. Clients therefore cannot receive the previous cache
while the asynchronous database query is still running.

Do not manually send the refresh packet to individual GameServers and do not
manually add or remove Honor skills. The ShardManager command is the single
entry point for the whole shard.

## Important distinction

The stock `_TRAINING_CAMP_UPDATEHONORRANK` procedure resets the current rank
assignments and immediately recalculates the top 50 eligible camps in one
transaction. This command is a live **refresh/recalculation**, not a permanent
empty-rank reset.
