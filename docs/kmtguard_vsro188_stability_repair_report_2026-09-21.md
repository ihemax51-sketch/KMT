# KMTGuard vSRO 188 Stability Repair Report

**Repair date:** 2026-09-21

**Scope:** targeted production stability repair based on KMT-AUD-001, 002, 003, 005, 006, 007, 009, 010, 014, and 016

**Compatibility rule:** no packet format, opcode, supported executable fingerprint, Filter proxy role, or native vSRO communication flow was changed

## 1. Fixed issues

### 1.1 ShardManager unique-log worker lifetime

**Before:** `UniqueLogQueue::Shutdown` changed a running flag, waited five seconds, and then closed handles and cleared shared data regardless of the wait result. Retry delays used `Sleep`, and an ODBC statement had no cancellation entry point.

**Unsafe vSRO scenario:** SQL blocks during a unique kill while ShardManager is restarting. The injected worker survives the five-second wait and returns into cleared DLL state.

**After:**

- explicit `Stopped`, `Running`, and `Stopping` states control queue acceptance and worker lifetime;
- retry delays wait on the queue event and become immediately shutdown-aware;
- `SQLCommand::Cancel` calls `SQLCancelHandle` for the active worker statement;
- shutdown handles `WAIT_OBJECT_0`, `WAIT_TIMEOUT`, `WAIT_FAILED`, and unexpected results separately;
- thread/event/queue/connection-string state is retained after a failed join and released only after confirmed exit;
- a stale retained generation prevents a replacement worker from starting.

Unique spawn/kill SQL and ShardManager/GameServer message behavior remain unchanged.

### 1.2 GameServer security-refresh worker lifetime

**Before:** shutdown waited five seconds, ignored the result, closed the stop event, and deleted the shared SQL connection. A delayed locked-item or fortress-DPS refresh could resume against freed state.

**After:**

- the current scoped statement is registered under a dedicated critical section;
- shutdown signals the worker and cancels the active ODBC statement;
- the thread handle is retained until `WAIT_OBJECT_0`;
- timeout/failure retains the SQL connection, event, locks, and thread handle instead of risking unload-time use-after-free;
- the shutdown wait covers the configured statement timeout;
- existing staged cache loaders still swap only complete successful data.

The refresh remains off the GameServer thread.

### 1.3 Deterministic native SQL timeouts

**Before:** statement allocation already attempted a 30-second query timeout, but GameServer connection establishment had no explicit login timeout and refresh shutdown could not cancel a statement.

**After:**

- GameServer ODBC connections apply a 10-second `SQL_LOGIN_TIMEOUT` before `SQLDriverConnectA`;
- all statements created through `CDbConnection::AllocStmt` retain the 30-second query timeout;
- security-refresh statements have a shutdown cancellation path;
- the synchronous item lock/unlock stored procedure uses a three-second statement timeout to limit game-loop impact without changing its transaction ordering.

**Gameplay-blocking SQL audit:** `CSqlCon::SetItemLockState` remains synchronous in the item lock/unlock packet path. Moving it to a worker would separate the database commit from scroll consumption and live inventory mutation, creating a larger consistency risk. It is therefore intentionally retained with a reduced deterministic timeout. The archived `CustomTimedJobManager` SQL calls cannot run after this repair. Other reviewed direct GameServer SQL calls are initialization/cache-loading paths, not steady combat packet handlers. `IsCharInParty` is defined but has no source caller.

### 1.4 ShardManager ODBC result classification

**Before:** `SQLCommand::GetData` accepted only exact `SQL_SUCCESS`; `SQL_SUCCESS_WITH_INFO` was treated as a database failure, and truncation was not represented distinctly.

**After:**

- `SQL_DATA_RESULT` distinguishes success, success-with-info, NULL, truncation, no-data, and error;
- `GetDataResult` accepts recoverable info results but rejects character truncation explicitly using the indicator length and `SQL_NO_TOTAL`;
- the existing boolean API remains compatible and fails safely for truncation/no-data/error;
- invalid command parameters continue through the existing rejected-command path instead of changing packet layout.

### 1.5 Native string and packet-reader safety

**Before:** GameServer action 8 read a ShardManager length-prefixed skill CodeName through a fixed `char[128]` template read. The custom log wide-string reader validated a `size_t` then narrowed it to signed 16-bit.

**After:**

- action 8 uses the existing checked `CMsg::ReadString` with a 127-byte maximum and passes `c_str()` only after successful validation;
- empty values are rejected before invoking the native skill operation;
- `CMsgStreamBufferCustom::Read` now accepts `size_t` and retains remaining-payload validation;
- wide-string byte counts no longer narrow to `__int16`.

Valid `0x8888` action packets keep the same length-prefixed wire format and opcode.

### 1.6 Filter database queue generation lifetime

**Before:** workers referenced replaceable static queue/token fields. Stop could cancel, wait two seconds, dispose the token source, and allow a new generation while an old SQL action was alive.

**After:**

- each generation owns its channel, cancellation source, workers, and ID;
- every worker captures its generation and never reads replaceable lifecycle state;
- `Start` refuses to overlap a generation that is still stopping;
- after the graceful drain deadline, cancellation propagates to each queued action and shutdown waits for all workers before disposing the token source;
- completion waiters left in the channel are cancelled with the correct generation token.

Queue capacity, worker count, retry rules, and packet-facing APIs are unchanged.

### 1.7 Client DLL initialization and D3D9 safety

**Before:** if the early `InitGameAssets` gate could not be installed, the DLL continued and allowed the worker to publish many fixed-address hooks while client startup advanced. `EndSceneHook` also called through `m_pd3dDevice` without a final null guard.

**After:**

- the exact v188 fingerprint check is unchanged;
- supported clients now fail DLL loading before the worker starts when the verified startup gate is unavailable;
- hook setup cannot publish on the prior ungated recovery path;
- D3D create callbacks run only after successful native creation with a live device;
- `EndSceneHook` snapshots and validates the current device pointer before custom work or `EndScene`.

No address, vtable slot, packet registration, or supported client version changed. ImGui reset callbacks remain disabled by the existing build configuration.

### 1.8 Unsafe CustomTimedJobManager worker disabled

**Before:** the archived public entry point could create a detached infinite worker that accessed `CGObjPC`, inventories, skills, and messages outside the GameServer thread.

**After:**

- a compile-time guard emits `#error` if anyone attempts to enable the unsafe worker macro;
- `CreateConsumeThread` is retained for ABI/source compatibility but only reports that the archived worker is disabled;
- the implementation remains in source for a future game-thread-command redesign, as requested.

### 1.9 Dynamic SQL identifier protection

**Before:** identifiers were syntax-validated centrally, but `AutoEventService.EventTable` accepted any syntactically valid table name from its string parameter.

**After:**

- `SqlIdentifier.QuoteAllowed` combines the existing strict identifier grammar with an explicit allow-list;
- `AutoEventService` declares the exact eleven table names it uses, including the three dynamically selected survival/hide-and-seek schedule tables;
- event SQL text and normal database behavior are unchanged.

## 2. Changed files

| Component | Files | Purpose |
|---|---|---|
| ShardManager | `Runtime/UniqueLogQueue.cpp` | explicit lifecycle, interruptible waits, conclusive join |
| ShardManager | `Database/SQLCommand.cpp`, `.h` | cancellation and typed ODBC result handling |
| GameServer | `SqlConnection/DbConnection.cpp` | login/query timeout enforcement |
| GameServer | `SqlConnection/sqlCon.cpp` | active-statement cancellation and safe worker join |
| GameServer | `App/src/Game.cpp` | bounded action-8 CodeName reader |
| GameServer | `GSLog/MsgCustom.cpp`, `.h` | remove signed read-size narrowing |
| GameServer | `Objects/CustomTimedJobManager.cpp`, `.h` | compile-time and runtime activation prevention |
| Filter | `Helpers/DatabaseJobQueue.cs` | per-generation queue/CTS/worker ownership |
| Filter | `Database/SqlIdentifier.cs` | reusable identifier allow-list API |
| Filter | `Features/AutoEvents/AutoEventService.cs` | exact event-table allow-list |
| Client DLL | `DllMain.cpp` | fail closed without the startup gate |
| Client DLL | `hooks/GFXVideo3d_Hook.cpp` | device/create guards |
| Verification | `scripts/test_stability_repairs.py` | source-contract regression checks |

## 3. Important code decisions

1. **No native worker termination:** `TerminateThread` was not introduced. Cancellation plus confirmed join is the only safe route for CRT/ODBC-owned state.
2. **Retain rather than free on failed join:** leaking state during a failed process shutdown is safer than a live worker using freed memory. Operators must terminate the host process rather than hot-unload the DLL.
3. **No GameServer object work on workers:** the timed-item worker is disabled, not repaired with locks.
4. **Synchronous item-lock SQL is bounded, not moved:** database commit, cache update, scroll consumption, and response ordering remain intact.
5. **No protocol changes:** all validation is consumer-side and valid length-prefixed messages remain identical.
6. **Client fails before partial publication:** a loader conflict at the bootstrap call site now stops loading instead of relying on late recovery.
7. **Filter waits for its workers:** shutdown can take as long as a non-cooperative SQL provider call, but a new generation can never overlap it.

## 4. Testing performed

The Linux review environment cannot build the Windows x86 native projects and does not provide the .NET SDK or PowerShell. The following checks were performed here:

- `python3 scripts/test_stability_repairs.py` — verifies all repaired lifecycle, cancellation, timeout, string, queue-generation, hook-gate, D3D, timed-worker, and identifier contracts in current source;
- `git diff --check` — verifies patch whitespace/integrity;
- targeted call-site searches for `TryExecNonQuery`, raw ODBC execution, `SetItemLockState`, `IsCharInParty`, `CustomTimedJobManager`, and client hook/D3D callbacks.

Required Windows staging tests remain:

- build Filter, ShardManager, GameServer, and Client DLL with their supported toolchains;
- 100+ ShardManager and GameServer shutdown/start cycles with SQL delays of 0, 5, and 30 seconds;
- table-lock and network-interruption tests during refresh, item lock, and unique logging;
- action-8 and log-string lengths 0, 1, 127, 128, 4095, 65535, and truncated payloads;
- Filter queue saturation and restart during a cancelled SQL command;
- exact-client fast/late injection, reconnect, teleport, alt-tab, resolution change, and D3D9 reset soak.

## 5. Remaining risks

- A provider call that ignores ODBC cancellation can extend shutdown to its driver/network timeout. State is now retained safely, but service stop may be slow.
- `SetItemLockState` can still pause the GameServer thread for up to three seconds. This is an explicit consistency trade-off and should be monitored; a future asynchronous redesign requires a durable two-phase item operation, not a simple worker call.
- Some legacy raw SQL loader functions manually manage the shared lock and statement. They receive the central 30-second timeout but do not all participate in active-statement cancellation; reviewed active security refresh functions use scoped cancellable statements.
- Client patch helpers do not provide rollback after a mid-setup structured exception. The startup gate prevents the client from consuming the partial setup and the process fails closed, but transactional rollback remains future hardening.
- Runtime SQL blocking, injected binary ABI compatibility, and third-party loader conflicts are **Unknown / Needs Verification** until the Windows staging matrix is complete.

## 6. Deployment order

1. Back up current binaries/configuration and validate database backups.
2. Deploy ShardManager and GameServer add-ons together during full shard maintenance.
3. Start ShardManager, then GameServers, and validate cache/command/unique logs before opening Gateway.
4. Deploy/restart Filter Agent, Gateway, and Download roles.
5. Distribute the Client DLL and require a full client restart; avoid a long mixed-version window.
6. Run unique kill, locked-item, event schedule, reconnect, teleport, and D3D9 smoke tests before reopening production.

No SQL migration or media update is required for this repair.

## 7. Rollback plan

1. Stop Gateway access and allow or force sessions to close.
2. Stop Filter roles, all GameServers, then ShardManager; never overwrite an injected DLL in a running process.
3. Restore the previous ShardManager/GameServer DLLs as a matched set.
4. Restore the previous Filter binaries and Client DLL package.
5. No database rollback is required because this repair adds no migration and changes no stored procedure.
6. Restart ShardManager, GameServers, Filter roles, then Gateway; verify command queues and locked-item cache before player access.
7. Preserve crash dumps and logs from the failed release for comparison with worker state, wait result, SQL timeout, and client initialization diagnostics.
