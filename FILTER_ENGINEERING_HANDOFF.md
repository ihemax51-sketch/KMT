# Filter Engineering Handoff

Last updated: 2026-06-24

This document summarizes the stabilization and cleanup work done on the filter/client stack, why it was done, what files were changed, and what remains for the next developer.

## Current build status

- Filter build: passing
- Packet pipeline smoke tests: passing
- Latest verified commands:

```powershell
dotnet build filter\KMTGuardnew\KMTGuard\KMTGuard.csproj -c Release
dotnet build filter\KMTGuardnew\KMTGuard\KMTGuard.csproj -c Release -t:Rebuild
dotnet run --project filter\KMTGuardnew\KMTGuard.PacketPipelineTests\KMTGuard.PacketPipelineTests.csproj -c Release
```

Latest observed full rebuild output after this pass: 0 warnings, 0 errors.

Latest UI verification build used a temporary output folder because the live Release output was locked by a running `KMTGuard.exe` process:

```powershell
dotnet build filter\KMTGuardnew\KMTGuard\KMTGuard.csproj -c Release -t:Rebuild -o <repo root>\build_verify\ui_release
```

Build output path:

```text
<repo root>\filter\KMTGuardnew\KMTGuard\bin\Release\net8.0\win-x64
```

Desktop build script used by the operator:

```text
C:\Users\Administrator\Desktop\03_BUILD_FILTER.cmd
```

Full rebuild script:

```text
<repo root>\04_BUILD_FILTER_FULL_REBUILD.cmd
C:\Users\Administrator\Desktop\04_BUILD_FILTER_FULL_REBUILD.cmd
```

## Main production issue addressed

The filter was producing SQL timeout errors such as:

```text
Execution Timeout Expired
Error in ProcessPlannedCommands
Error during OnTimerTick
HandleHwidList hata
```

Earlier behavior could cause player disconnects because some database work was executed inside sensitive packet/session paths. The work below moved the dangerous pieces away from live packet flow, added throttling/backoff, and made the session pipeline more defensive.

## Major changes completed

### 1. Removed Battle Pass system

The old Royal/Battle Pass work was removed because it was unstable, visually poor, and no longer wanted.

### 2. WebViewer architecture

Implemented a safer architecture:

- `KMTGuardKit.dll` remains old VC80/x86 and is responsible only for game icon/frame integration.
- `WebViewerBridge.dll` is a separate modern x86 DLL responsible for WebView2.
- The old DLL and bridge communicate through simple boundaries, not STL objects.
- WebViewer button configuration was moved toward DB-backed loading instead of loose local JSON files.

Related migration:

```text
database/migrations/20260624_webviewer_buttons.sql
```

Important runtime note:

- If WebView2 Runtime is missing on a client PC, the web viewer cannot render. The game-side DLL should fail gracefully and not crash the game.

### 3. Character select animation

Character select animation support was added and confirmed working by the user.

### 4. Client DLL cleanup

The client DLL was reviewed beyond only the new features. Key fixes included:

- Removed noisy runtime/debug prints.
- Guarded unsafe traversal loops.
- Reduced WebViewer repeated load-failure spam.
- Built/deployed `KMTGuardKit.dll` to the user’s current client path at the time.

Current client game folder mentioned by user:

```text
C:\Users\Administrator\Downloads\Qivin-X
```

## Filter stabilization phases completed

### Phase 1: HWID update isolation

File:

```text
filter/KMTGuardnew/KMTGuard/Server/AgentServer/PacketHandler/GameServer/CustomGameServerPacketHandler.cs
```

Changes:

- `_HandleHwidList` is no longer awaited directly inside sensitive packet flow.
- HWID list updates are queued through `DatabaseJobQueue.TryQueueBackground`.
- Added per-character cooldown for HWID updates to avoid flooding SQL.
- Added guard checks so `_HandleHwidList` is not called with missing `CharID`, `CharName`, `ClientIP`, or `Hwid`.
- Uses `CommandDefinition` with `commandTimeout` and `cancellationToken`.

### Phase 2: DatabaseJobQueue

File:

```text
filter/KMTGuardnew/KMTGuard/Helpers/DatabaseJobQueue.cs
```

Changes:

- Added safe background DB job queue behavior.
- Background SQL timeouts are logged as warnings instead of crashing packet/session flow.
- Queue has bounded capacity and drops background jobs if full to protect live packet processing.

Result:

- SQL timeout can still appear if the test SQL server is slow, but it should not disconnect players by itself.

### Phase 3: `_HwidList` indexes and optional normalization

Files:

```text
database/migrations/20260624_hwidlist_indexes.sql
database/migrations/20260624_hwidlist_hwid_normalize_optional.sql
```

Changes:

- Added conditional index on `_HwidList(CharID)`.
- Added conditional `_HwidList(Hwid, Active)` index only when `Hwid` is an indexable bounded type.
- Added fallback `_HwidList(Active)` index when `Hwid` is not indexable.
- Added optional migration to convert `Hwid` from `text/ntext/varchar(max)/nvarchar(max)` to `NVARCHAR(128)`.

Important warning:

Run `20260624_hwidlist_hwid_normalize_optional.sql` only during maintenance/offline time. It may take a schema lock because it uses `ALTER COLUMN`.

### Phase 4: Async command timer stabilization

File:

```text
filter/KMTGuardnew/KMTGuard/Database/DatabaseCommands.cs
```

Changes:

- Timers are now nullable and guarded against duplicate initialization.
- `StopTimers()` disposes timers and resets references to `null`.
- `ProcessCommands` and `ProcessPlannedCommands` use non-reentrant locks.
- SQL reads use explicit `ReadCommitted` transactions to avoid the `READPAST` isolation error.
- Command batch sizes and retry backoff are centralized.

Related index migration:

```text
database/migrations/20260623_async_filter_commands_indexes.sql
```

This supports:

- `_AsyncFilterCommands(Status, ID)`
- `_AsyncFilterCommandsPlanned(Status, DateToExecute, ID)`

### Phase 5: Async command status updates

File:

```text
filter/KMTGuardnew/KMTGuard/Database/DatabaseCommands.cs
```

Changes:

- Replaced repeated raw SQL:

```sql
UPDATE _AsyncFilterCommands SET Status = 0 WHERE ID = @ID
```

with:

```csharp
MarkCommandCompleteAsync(...)
```

Benefit:

- One central timeout and update path for normal async commands.
- Easier to debug/extend later.

### Phase 6: Session lifecycle and sockets

Files:

```text
filter/KMTGuardnew/KMTGuard/Session/Session.cs
filter/KMTGuardnew/KMTGuard/ServerManagers/ServerManager.cs
```

Changes:

- Fixed send semaphore handling so `Release()` is only called if the lock was acquired.
- Avoids possible semaphore errors during disconnect/reconnect.
- `Stop()` now avoids DB cleanup unless `CharID > 0`.
- Removes the player IP from flood tracking map on stop.
- Enables `NoDelay` on the client socket to reduce packet latency.
- Initializes `ServerManager` session collections at startup and before timers.
- `Servers`, `AgentSessions`, `DownloadSessions`, and `GatewaySessions` now have safe default initializers.

### Phase 7: Packet pipeline safety

Files:

```text
filter/KMTGuardnew/KMTGuard/PacketHandler/IPacketHandler.cs
filter/KMTGuardnew/KMTGuard/PacketHandler/PacketHandler.cs
```

Changes:

- Added `PacketData.Empty`.
- Stopped passing `null` packet data to handlers.
- Added safe handler invocation wrapper.
- If a packet handler returns `null` or throws, the result becomes a controlled `Disconnect` instead of crashing the packet loop.

### Phase 8: DatabaseCommands session snapshots

File:

```text
filter/KMTGuardnew/KMTGuard/Database/DatabaseCommands.cs
```

Changes:

- Added:

```csharp
SnapshotAgentSessions()
FindAgentSessionByCharId(...)
FindAgentSessionByCharName(...)
```

- Replaced direct `AgentSessions.FirstOrDefault(...)`/`ToList()` patterns with snapshot-based access.
- Removed dead `CompleteCount` code/comment block.

Benefit:

- Reduces race conditions when admin commands run while sessions disconnect.

### Phase 9: ServerManager async cleanup

File:

```text
filter/KMTGuardnew/KMTGuard/ServerManagers/ServerManager.cs
```

Changes:

- Converted synchronous count/find helpers from `async Task<T>` to `Task.FromResult(...)`.
- Kept public return types as `Task<T>` so existing `await` call sites do not break.
- Converted `StartAsync` to return `Task.CompletedTask` instead of being an `async` method without `await`.

### Phase 10: RefManager timer cleanup

File:

```text
filter/KMTGuardnew/KMTGuard/ServerManagers/RefManager.cs
```

Changes:

- Made the rank reload timer nullable and disposable/restart-safe.
- Guarded `StartLoadRanksTimer()` against duplicate timer creation.
- Added a non-reentrant rank reload helper so slow SQL cannot overlap multiple `LoadRanks()` refreshes.
- Converted cache-only `GetRefObjCommonValidate(...)` to return `Task.FromResult(...)`.
- Switched cached ref object lookups to `TryGetValue(...)` to avoid contains/indexer races.

### Phase 11: Packet handler async cleanup

Files:

```text
filter/KMTGuardnew/KMTGuard/Server/AgentServer/PacketHandler/Guild/GuildPackets.cs
filter/KMTGuardnew/KMTGuard/Server/AgentServer/PacketHandler/Stall/StallPackets.cs
filter/KMTGuardnew/KMTGuard/Server/AgentServer/PacketHandler/COS/COSPackets.cs
filter/KMTGuardnew/KMTGuard/Server/AgentServer/PacketHandler/ExploitFixPackets.cs
filter/KMTGuardnew/KMTGuard/Server/AgentServer/PacketHandler/CharacterActions/CharDataPackets.cs
filter/KMTGuardnew/KMTGuard/Server/AgentServer/PacketHandler/CharacterActions/CharAction.cs
```

Changes:

- Converted small synchronous packet handlers from `async Task<PacketResult>` to direct `Task<PacketResult>` with `Task.FromResult(...)`.
- Left handlers with `await session.SendToClient(...)`, async SQL, or `DatabaseJobQueue` work unchanged.
- Scope was intentionally conservative to avoid changing gameplay behavior.

### Phase 12: Chat packet null handling

Files:

```text
filter/KMTGuardnew/KMTGuard/Server/AgentServer/PacketHandler/Chat/ChatPackets.cs
filter/KMTGuardnew/KMTGuard/Server/AgentServer/PacketHandler/Chat/ChatLogger.cs
```

Changes:

- Made chat packet parsing default receiver/message values to `string.Empty` instead of nullable locals.
- Made `PacketCursor.TryReadBytes(...)` return `Array.Empty<byte>()` on failure instead of assigning `null`.
- Updated `ChatLogger.LogAsync(...)` so `receiver` is explicitly nullable while sender/message stay non-null.
- Preserved existing DB behavior where missing receiver is stored as `NULL`.

### Phase 13: CustomGameServerPacketHandler warning cleanup

File:

```text
filter/KMTGuardnew/KMTGuard/Server/AgentServer/PacketHandler/GameServer/CustomGameServerPacketHandler.cs
```

Changes:

- Converted `SERVER_REGION_HAS_CHANGED(...)` and `SERVER_ENTITY_STATE_UPDATE(...)` from `async Task<PacketResult>` to direct `Task<PacketResult>` because they perform synchronous session/packet updates only.
- Replaced nullable `___SR_GSNpcUniqueIdList npcData = null` locals with inline `out var npcData` lookups in trade-good request/complete paths.
- Removed the unused exception variable warning from the async kill-log background job catch.
- Kept DB/trigger behavior unchanged.

### Phase 14: Remaining packet async cleanup

Files:

```text
filter/KMTGuardnew/KMTGuard/Server/AgentServer/PacketHandler/PvpCape/PvpCapePackets.cs
filter/KMTGuardnew/KMTGuard/Server/AgentServer/PacketHandler/CustomUIPackets.cs
filter/KMTGuardnew/KMTGuard/Server/AgentServer/AgentServer.cs
filter/KMTGuardnew/KMTGuard/Server/GatewayServer/GatewayServer.cs
filter/KMTGuardnew/KMTGuard/Server/DownloadServer/DownloadServer.cs
filter/KMTGuardnew/KMTGuard/AsyncServer/AsyncServer.cs
filter/KMTGuardnew/KMTGuard/CommandManager/CommandHandler.cs
```

Changes:

- Completed remaining `async`-without-`await` cleanup for packet/shell handlers.
- Converted ping handlers to return `Task.FromResult(...)`.
- Converted synchronous packet handlers in `PvpCapePackets`, `CustomUIPackets`, and gateway redirect handling to direct `Task<PacketResult>`.
- Converted synchronous utility/task methods to `Task.CompletedTask` or `Task.FromResult(...)` while preserving public return types.
- Latest build has no remaining `CS1998` warnings.

### Phase 15: Scattered nullable cleanup

Files:

```text
filter/KMTGuardnew/KMTGuard/AsyncServer/TokenProvider.cs
filter/KMTGuardnew/KMTGuard/CommandManager/CommandHandler.cs
filter/KMTGuardnew/KMTGuard/Features/Skills/SkillRuleManager.cs
filter/KMTGuardnew/KMTGuard/Server/AgentServer/PacketHandler/CharacterActions/CharAction.cs
filter/KMTGuardnew/KMTGuard/Server/AgentServer/PacketHandler/COS/COSPackets.cs
filter/KMTGuardnew/KMTGuard/Database/DatabaseCommands.cs
filter/KMTGuardnew/KMTGuard/Startup.cs
filter/KMTGuardnew/KMTGuard/Database/sqlQueryHelper.cs
```

Changes:

- Made `TokenProvider.GetToken(...)` return `TokenProvider?` because lookup can legitimately miss.
- Marked console input and skill-rule out parameters nullable where appropriate.
- Marked teleport deny reason nullable.
- Marked temporary COS pet lookups nullable.
- Made timer state nullable in `DatabaseCommands.UpdateTimers(...)` and replaced unused `TryRemove` out variables with discards.
- Replaced nullable-sensitive `TryParse`/`ExecuteScalarAsync` locals with nullable-aware declarations.

### Phase 16: Full rebuild warning cleanup

Files:

```text
filter/KMTGuardnew/KMTGuard/Database/Models/_UniqueHistory.cs
filter/KMTGuardnew/KMTGuard/Database/Shard/_RefObjCommon.cs
filter/KMTGuardnew/KMTGuard/Server/GatewayServer/PacketHandler/SERVER_DLL_SETTINGS_RESPONSE.cs
filter/KMTGuardnew/KMTGuard/Session/Session.cs
filter/KMTGuardnew/KMTGuard/Startup.cs
filter/KMTGuardnew/SilkroadSecurityAPI/Packet.cs
filter/KMTGuardnew/SilkroadSecurityAPI/Security.cs
filter/KMTGuardnew/SilkroadSecurityAPI/TransferBuffer.cs
```

Changes:

- Completed the remaining full `-t:Rebuild` warning cleanup.
- Initialized missing model collections and removed dead private cache/debug fields.
- Guarded Windows-only registry access with `OperatingSystem.IsWindows()`.
- Made the gateway shard-list passthrough branch explicit without unreachable code.
- Cleaned `SilkroadSecurityAPI` null-state handling while preserving the original packet read/write mode behavior.
- Kept the legacy UTF-7 transform in `Packet.cs` because it is part of the Silkroad wire compatibility path, and documented the warning suppression in code.
- Replaced null transfer buffers with empty buffers and marked transfer methods nullable where callers already handle the no-packets case.

### Phase 17: Console UI polish

Files:

```text
filter/KMTGuardnew/KMTGuard/ConsoleUi/FilterConsole.cs
filter/KMTGuardnew/KMTGuard/Startup.cs
filter/KMTGuardnew/KMTGuard/CommandManager/CommandHandler.cs
filter/KMTGuardnew/KMTGuard/ServerManagers/ServerManager.cs
```

Changes:

- Added a dedicated console UI helper for startup banners, status panels, prompts, and command feedback.
- Replaced the plain `Enter a command` loop with a styled `KMTGuard >` prompt.
- Added `/help` and `/status` commands for operator usability.
- Updated the console title to show live Gateway/Agent/Download connection counts in a cleaner format.
- Kept the implementation dependency-free so the filter remains a simple console service and startup behavior stays stable.

## Important SQL notes

### Slow test SQL can still produce timeouts

If SQL Server is very slow, warnings like this may still appear:

```text
Execution Timeout Expired
```

That does not automatically mean the filter is broken. The key improvement is that many SQL failures are now isolated from player packet handling.

### If `_HandleHwidList` still times out

The stored procedure body is not present in this source tree. If `HandleHwidList` still times out after the code changes:

1. Extract/provide the SQL definition of `_HandleHwidList`.
2. Confirm `_HwidList.Hwid` type.
3. If `Hwid` is `text`, `ntext`, `varchar(max)`, or `nvarchar(max)`, consider running:

```text
database/migrations/20260624_hwidlist_hwid_normalize_optional.sql
```

4. Recheck indexes:

```text
IX_HwidList_CharID
IX_HwidList_Hwid_Active
IX_HwidList_Active
```

## Remaining work / recommended next phases

### 1. Better command claiming system

Current async commands read pending commands and mark them complete after execution.

Future safer design:

- Add `Status = 2` meaning `Processing`.
- Atomically claim commands before execution.
- Add recovery for stuck `Processing` commands after filter crash.

Do not implement this without planning the DB migration and recovery behavior.

### 2. Stored procedure review

The C# side is now more defensive, but some real performance issues may live in SQL stored procedures.

Priority procedures:

- `_HandleHwidList`
- `_FilterStartup`
- Any procedures called by `_AsyncFilterCommands`

### 3. Operational monitoring

Keep watching production logs after deployment for SQL timeout patterns:

```text
Database background job timed out
ProcessPlannedCommands
OnTimerTick
HandleHwidList
```

If players stay connected but these warnings appear, treat it as SQL/server performance first.
## Operational checklist for next developer

1. Build:

```powershell
dotnet build filter\KMTGuardnew\KMTGuard\KMTGuard.csproj -c Release
```

2. Test:

```powershell
dotnet run --project filter\KMTGuardnew\KMTGuard.PacketPipelineTests\KMTGuard.PacketPipelineTests.csproj -c Release
```

3. Deploy from:

```text
<repo root>\filter\KMTGuardnew\KMTGuard\bin\Release\net8.0\win-x64
```

4. Watch logs for:

```text
Database background job timed out
ProcessPlannedCommands
OnTimerTick
HandleHwidList
```

5. If players do not disconnect but warnings appear, treat it as SQL/server performance first, not packet pipeline failure.

## Files changed in this stabilization pass

Core C#:

```text
filter/KMTGuardnew/KMTGuard/AsyncServer/AsyncServer.cs
filter/KMTGuardnew/KMTGuard/CommandManager/CommandHandler.cs
filter/KMTGuardnew/KMTGuard/ConsoleUi/FilterConsole.cs
filter/KMTGuardnew/KMTGuard/Database/DatabaseCommands.cs
filter/KMTGuardnew/KMTGuard/Database/Models/_UniqueHistory.cs
filter/KMTGuardnew/KMTGuard/Database/Shard/_RefObjCommon.cs
filter/KMTGuardnew/KMTGuard/Helpers/DatabaseJobQueue.cs
filter/KMTGuardnew/KMTGuard/PacketHandler/IPacketHandler.cs
filter/KMTGuardnew/KMTGuard/PacketHandler/PacketHandler.cs
filter/KMTGuardnew/KMTGuard/Server/AgentServer/PacketHandler/GameServer/CustomGameServerPacketHandler.cs
filter/KMTGuardnew/KMTGuard/Server/GatewayServer/PacketHandler/SERVER_DLL_SETTINGS_RESPONSE.cs
filter/KMTGuardnew/KMTGuard/ServerManagers/ServerManager.cs
filter/KMTGuardnew/KMTGuard/Session/Session.cs
filter/KMTGuardnew/KMTGuard/Startup.cs
filter/KMTGuardnew/SilkroadSecurityAPI/Packet.cs
filter/KMTGuardnew/SilkroadSecurityAPI/Security.cs
filter/KMTGuardnew/SilkroadSecurityAPI/TransferBuffer.cs
```

Database migrations:

```text
database/migrations/20260623_async_filter_commands_indexes.sql
database/migrations/20260624_hwidlist_indexes.sql
database/migrations/20260624_hwidlist_hwid_normalize_optional.sql
database/migrations/20260624_webviewer_buttons.sql
```

Docs/scripts already present:

```text
BATTLE_PASS_ADMIN_GUIDE.md
DEPLOYMENT_FIXES.md
WebViewerBridge/README.md
C:\Users\Administrator\Desktop\03_BUILD_FILTER.cmd
```
