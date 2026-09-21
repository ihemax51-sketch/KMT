# KMTGuard / vSRO 188 Source-Code Audit

**Audit date:** 2026-09-21  
**Audit type:** whole-repository, source-assisted static review  
**Target:** vSRO 188 Filter, GameServer add-on, ShardManager add-on, Client DLL/hooks, database layer, events, packet paths, native memory, and worker threads

## 1. Executive summary

This review treats KMTGuard as a vSRO 188 system, not as a conventional web or line-of-business application. In particular, the review accepts these normal vSRO constraints rather than reporting them as defects:

- fixed-address hooks and x86 layouts are valid when protected by an exact host fingerprint;
- native GameServer and ShardManager calls are necessarily ABI- and build-specific;
- packet processing is sequential per Filter session by design;
- most original GameServer object operations must remain on the native game thread;
- Filter-side asynchronous SQL is acceptable when it does not block the GameServer loop and has bounded concurrency/back-pressure;
- a fail-closed response to an incompatible native host binary is intentional operational behaviour.

The current tree has substantially better defensive controls than a typical vSRO extension: bounded massive-packet assembly, client opcode allow-lists, packet-handler exception containment, bounded database and unique-event queues, parameterised C# data values, internal command authentication, staged GameServer cache refreshes, host fingerprint checks, and transactional hook installation. Those controls materially reduce common packet-injection, malformed-packet, and partial-hook failures.

The audit nevertheless found **two critical shutdown/lifetime defects**, **six high-priority stability or security risks**, **six medium-priority hardening items**, and **three low-priority maintenance items**. The most urgent problems are native worker shutdown paths that release objects after a timed wait without proving that the worker has exited. Under a slow or hung ODBC call, the surviving worker can access freed connection, event, queue, or lock state. These are real native use-after-free/race conditions and can crash ShardManager or GameServer during restart or DLL teardown.

This document is an audit report, not a claim that static inspection can prove the absence of defects. Runtime-only dependencies—live stored-procedure bodies, the deployed v188 executables, the production SQL schema, injected DLL load order, and real packet captures—must be verified in the staging plan in section 5.

### Finding totals

| Severity | Count | Release position |
|---|---:|---|
| Critical | 2 | Fix before the next production deployment |
| High | 6 | Fix in the same stability release where practical |
| Medium | 4 | Schedule after critical/high soak testing |
| Low | 3 | Maintenance backlog |

### Component coverage

| Area | Primary paths reviewed | Result |
|---|---|---|
| Filter and packet proxy | `Session`, packet pipeline, Agent/Gateway handlers, runtime queues | Strong baseline; upstream bounds and shutdown semantics need work |
| GameServer add-on | command dispatch, custom packets, SQL/cache refresh, telemetry, native object operations | Critical worker-lifetime issue; dormant unsafe timer subsystem must remain disabled |
| ShardManager add-on | message bridge, command claimant, unique logging, ODBC wrappers | Critical worker-lifetime issue; ODBC status handling needs correction |
| Client DLL / hooks | `DllMain`, initialization gate, hook setup, D3D/UI paths | Compatible with the pinned client; initialization-thread race window remains |
| Database / SQL | Dapper callers, native ODBC, migrations and command queues | Identifiers are mostly controlled; deployed procedure bodies remain unverified |
| Events / custom systems | auto-events, survival, clientless, stalls, scheduling | Async separation is generally appropriate; cancellation/timeouts are inconsistent |

## 2. Critical problems

### KMT-AUD-001 — ShardManager unique-log worker may outlive and access released runtime state

- **Category:** A) Real crashes; B) Server performance
- **Priority:** **CRITICAL — P0**
- **File:** `ShardManager/vSRO-ShardManager/Runtime/UniqueLogQueue.cpp`
- **Function / class:** `UniqueLogQueue::Shutdown`, anonymous `Worker`
- **Exact location:** worker database use at lines 80-126; timed shutdown and unconditional handle/state cleanup at lines 177-197.
- **Danger:** shutdown waits only five seconds for the worker, ignores `WAIT_TIMEOUT`/`WAIT_FAILED`, then closes the thread/event handles, clears the queue and connection string, and returns. The worker can remain inside an ODBC connect or query whose statement timeout is 30 seconds. It can subsequently resume and read `s_event`, `s_connectionString`, `s_queue`, or the critical section after shutdown/teardown has invalidated surrounding process state. Closing a thread handle does not stop the thread.
- **Real vSRO scenario:** ShardManager is restarted while SQL Server is blocked, unavailable, or resolving a dead TCP route during `Hook_UniqueKill`. The service shutdown path reaches the five-second deadline while ODBC is still active; the injected worker later resumes during hook rollback or process teardown.
- **Expected impact:** access violation, heap/CRT corruption, shutdown hang, lost unique history, or an apparently random ShardManager crash during restart.
- **Recommended fix:** make shutdown cooperative and conclusive. Cancel the active ODBC statement (`SQLCancelHandle`), signal the worker, wait until it exits, and only then close handles or clear shared state. If bounded service shutdown is mandatory, intentionally terminate the host process without unloading the add-on; never continue DLL teardown with a live worker. Record and branch on every wait result. Add a forced 30+ second SQL-stall shutdown test.

### KMT-AUD-002 — GameServer security-refresh worker can use a deleted database connection

- **Category:** A) Real crashes; D) Architecture
- **Priority:** **CRITICAL — P0**
- **File:** `gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/GameServer/src/SqlConnection/sqlCon.cpp`
- **Function / class:** `SecuritySnapshotRefreshWorker`, `CSqlCon::Shutdown`
- **Exact location:** refresh loop at lines 118-133; five-second wait followed by connection deletion at lines 254-275.
- **Danger:** `CSqlCon::Shutdown` waits five seconds but does not verify that `s_securityRefreshThread` exited. It then closes the stop event and deletes `m_connectionstr`. The worker can still be blocked in `LoadLockedItems` or `LoadFortressDPSInfo`; when the call returns it continues through telemetry/logging and may make another access through shared SQL/cache state. The SQL critical section protects simultaneous connection operations but does not provide worker lifetime ownership after the timed wait.
- **Real vSRO scenario:** a GameServer restart or add-on teardown occurs while the minute refresh is blocked behind a SQL lock or network timeout. The shutdown wait expires, deletes the connection, and the refresh resumes against freed native state.
- **Expected impact:** GameServer access violation during maintenance, deadlock in native ODBC cleanup, corrupted cache state, or a process that cannot shut down cleanly.
- **Recommended fix:** give the refresh worker an interruptible statement/connection, treat a timed wait as a failed shutdown, and delete `m_connectionstr` only after a successful worker join. Keep the existing off-game-thread refresh model; no architectural rewrite is needed. Add a test hook that deliberately stalls the refresh beyond five seconds and proves that teardown cannot race it.

## 3. High-priority problems

### KMT-AUD-003 — Client hook publication still occurs from a loader-created worker thread

- **Category:** A) Real crashes; E) Client DLL
- **Priority:** **HIGH — P1**
- **File:** `JTClientLibrary/source/DevKit_DLL/src/DllMain.cpp`
- **Function / class:** `KMTGuardInitializationThread`, exported `DllMain`, `InitializeKMTGuardClient`
- **Exact location:** hook installation at lines 379-395; worker creation at lines 400-443.
- **Danger:** moving work out of loader lock is correct, and the bootstrap gate reduces exposure, but the final `SetupWithDiagnostics` hook writes still run concurrently with the client startup/UI thread. A host call site can execute while a multi-byte patch, vtable replacement, or related dependency state is being published unless every individual patch is transactional and every entry point is gated. This is especially sensitive for D3D9 device/UI creation.
- **Real vSRO scenario:** a fast machine or third-party loader injects after the client has already started creating `CGInterface` or the D3D9 device. The client thread enters a call site while the initialization worker commits hooks.
- **Expected impact:** intermittent startup access violation, half-installed hook, missing custom UI class, black screen, or crash on the first device reset.
- **Recommended fix:** retain the compatibility fingerprint and early gate, but marshal final hook publication to a known client-main-thread bootstrap callback before native assets/UI creation. Where that is impossible, use one transaction for all patches, suspend only the proven target thread for the shortest possible interval, flush the instruction cache, and fail closed before releasing the gate. Test late injection and repeated D3D reset.

### KMT-AUD-004 — Filter does not apply equivalent limits to server-to-client packet traffic

- **Category:** A) Real crashes; B) Server performance; C) Security
- **Priority:** **HIGH — P1**
- **File:** `filter/KMTGuardnew/KMTGuard/Session/Session.cs`
- **Function / class:** `Session.DoReceiveFromServer`
- **Exact location:** upstream receive/assembly and packet loop at lines 561-645; compare client emergency guards at lines 438-464 and configured security limits near the class fields.
- **Danger:** client ingress has packet count, byte, custom-opcode, and payload checks. Upstream ingress relies mainly on `Security(maxMassiveFragments: 4096, maxMassiveBytes: 16 MiB)` and forwards each resulting packet without a server-side per-session or broadcast budget. A malformed custom GameServer packet, compromised upstream service, or accidental oversized broadcast can cause large allocations and multiplication across all connected sessions.
- **Real vSRO scenario:** a custom event sends an unexpectedly large massive packet to every Agent session, or a damaged upstream packet declares thousands of fragments. Hundreds of sessions simultaneously assemble and copy multi-megabyte buffers.
- **Expected impact:** large-object-heap pressure, GC pauses, Filter memory spike, mass disconnects, or an out-of-memory process termination.
- **Recommended fix:** add protocol-aware upstream maximums before handler execution, a lower maximum for custom packets, and an aggregate per-session server-egress window. Preserve native packets known to be legitimately massive (character/item data) through an explicit v188 allow-list rather than a blanket limit. Log sampled metadata and stop only the affected session.

### KMT-AUD-005 — Filter database queue shutdown may dispose cancellation state while workers still run

- **Category:** A) Real crashes; B) Server performance
- **Priority:** **HIGH — P1**
- **File:** `filter/KMTGuardnew/KMTGuard/Helpers/DatabaseJobQueue.cs`
- **Function / class:** `DatabaseJobQueue.StopAsync`, `ProcessQueueAsync`
- **Exact location:** shutdown at lines 149-179; worker loop at lines 191-227.
- **Danger:** after a 15-second drain timeout the queue cancels and waits only two more seconds, catches every remaining failure, then disposes the shared `CancellationTokenSource` even if a job ignored cancellation or is still blocked in SQL. `Start` can replace the static queue and token while an old worker is alive. This is managed code rather than raw native UAF, but it creates overlapping worker generations and disposal races.
- **Real vSRO scenario:** Filter restart during a 30-second SQL command timeout. The two-second post-cancel wait expires; an administrator starts the runtime again in-process while the old Dapper call completes against the previous queue generation.
- **Expected impact:** duplicate cleanup/command effects, `ObjectDisposedException`, noisy unobserved worker failures, delayed shutdown, or stale jobs modifying player state after restart.
- **Recommended fix:** capture queue and cancellation objects per worker generation, do not dispose a generation until all its workers have completed, propagate cancellation into every `SqlCommand`, and disallow `Start` while the previous generation is draining. For non-idempotent jobs, persist/lease them rather than replaying them implicitly.

### KMT-AUD-006 — Native GameServer database execution has no explicit query timeout

- **Category:** B) Server performance; D) Architecture
- **Priority:** **HIGH — P1**
- **File:** `gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/GameServer/src/SqlConnection/sqlCon.cpp`
- **Function / class:** `CSqlCon::TryExecNonQuery` and snapshot loaders using `ScopedSqlStatement`
- **Exact location:** direct execution at lines 297-328; periodic refresh calls at lines 118-131.
- **Danger:** unlike the ShardManager wrapper, these native ODBC statements do not visibly set `SQL_ATTR_QUERY_TIMEOUT`. Off-thread refresh protects the real-time loop, but unlimited calls prevent deterministic shutdown (KMT-AUD-002), hold the single SQL lock, and can starve later security snapshot operations. Any call from a game-thread hook would be worse.
- **Real vSRO scenario:** SQL is reachable but a schema lock blocks the locked-item query indefinitely. The refresh worker owns the shared connection lock, subsequent operations queue, and maintenance restart hits the lifetime race.
- **Expected impact:** stale security restrictions, stuck maintenance shutdown, growing operational lag, and amplification of the critical teardown defect.
- **Recommended fix:** apply a short, configurable ODBC query/login timeout to every statement and cancel on shutdown. Keep DB work off the native game thread. Audit every `TryExecNonQuery` caller and move any live game-thread call to the existing command/worker bridge.

### KMT-AUD-007 — Internal command string decoding writes into a fixed 128-byte stack buffer

- **Category:** A) Real crashes; C) Security
- **Priority:** **HIGH — P1**
- **File:** `gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/App/src/Game.cpp`
- **Function / class:** `CGame::ProcessMessage`, `ActionAddSkillByCode`
- **Exact location:** lines 432-440.
- **Danger:** the code deserializes a length-prefixed string directly through `operator>>` into `char SkillCodeName[128]`. The ShardManager producer currently reads at most 128 bytes, which lowers ordinary exposure, but the GameServer consumer does not independently enforce a 127-byte limit. Trusting the producer is insufficient at a native packet boundary; a forged, replayed, version-skewed, or corrupted internal message can overflow the stack depending on the native extraction overload.
- **Real vSRO scenario:** mismatched ShardManager/GameServer add-on versions or a compromised internal service emits action 8 with a declared string length above the local buffer. Internal packet authentication establishes origin/integrity, not semantic buffer size.
- **Expected impact:** GameServer crash, stack corruption, or potentially controlled native memory corruption.
- **Recommended fix:** decode into a bounded `std::string` using a checked remaining-length API, reject values over the CodeName limit, and copy with guaranteed termination only after validation. Apply the same consumer-side rule to every fixed array, even when the ShardManager producer is currently bounded.

### KMT-AUD-008 — Auto-event SQL calls have no consistent timeout or cancellation token

- **Category:** B) Server performance; D) Architecture
- **Priority:** **HIGH — P1**
- **File:** `filter/KMTGuardnew/KMTGuard/Features/AutoEvents/AutoEventService.cs`
- **Function / class:** run/round persistence helpers (`CreateRunAsync`, `CreateRoundAsync`, `CompleteRoundAsync`, `FinishRunAsync`)
- **Exact location:** lines 1540-1615.
- **Danger:** these event-state calls open SQL connections and execute commands without `CommandDefinition.commandTimeout` or a service cancellation token. They are asynchronous and therefore do not block the GameServer loop, but an event transition can remain pending until provider defaults expire, delaying notices, rewards, or finalization while retaining pooled resources.
- **Real vSRO scenario:** SQL blocking begins during a survival or trivia round transition. The event state machine awaits persistence, the visible event stops progressing, and manual stop/restart cannot promptly cancel its database work.
- **Expected impact:** stuck event, duplicated operator intervention, connection-pool pressure, delayed rewards, or inconsistent run status after restart.
- **Recommended fix:** pass the owning event cancellation token and an explicit bounded timeout to every Dapper command. Preserve idempotent predicates (`FinishedAtUtc IS NULL`) and add a recovery query for runs left `Running` after a process restart.

## 4. Medium- and low-priority improvements

### KMT-AUD-009 — ShardManager ODBC wrapper rejects `SQL_SUCCESS_WITH_INFO`

- **Category:** A) Real crashes; D) Architecture
- **Priority:** **MEDIUM — P2**
- **File:** `ShardManager/vSRO-ShardManager/Database/SQLCommand.cpp`
- **Function / class:** `SQLCommand::GetData`
- **Exact location:** lines 82-93.
- **Danger:** ODBC returns `SQL_SUCCESS_WITH_INFO` for truncation and other recoverable conditions, but the wrapper accepts only exact `SQL_SUCCESS`. This is fail-safe for oversized command strings, yet it conflates truncation with transport failure and can repeatedly retry a permanently invalid queue row.
- **Real vSRO scenario:** a command contains a CodeName or grant-name at the database column maximum and ODBC reports truncation/diagnostic info for the 128-byte buffer.
- **Expected impact:** command stuck/retried, noisy logs, or an operator-visible action that never completes; not normally a process crash.
- **Recommended fix:** use `SQL_SUCCEEDED`, inspect the indicator length and diagnostics, explicitly reject truncation as invalid command data, and mark that row permanently failed rather than reconnecting.

### KMT-AUD-010 — Dormant timed-item worker is unsafe to re-enable

- **Category:** A) Real crashes; B) Server performance; D) Architecture
- **Priority:** **MEDIUM — P2 (becomes CRITICAL if enabled)**
- **File:** `gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/GameServer/src/Objects/CustomTimedJobManager.cpp`
- **Function / class:** `ConsumeTimedItemPlusRecords`, `ConsumeTimedItemPlusRecordsDevill`, `CreateConsumeThread`
- **Exact location:** native object/map access and SQL at lines 16-152 and 156-345; detached infinite worker at lines 348-371.
- **Danger:** the worker directly traverses `g_pCGame` player maps, inventories, skills and message allocation from a foreign thread; erases map entries while iterating; performs synchronous SQL; has no stop event; and immediately closes its thread handle. That violates vSRO game-thread ownership. Current source search shows no active caller, so it is not classified as an active crash.
- **Real vSRO scenario:** a developer re-enables `CreateConsumeThread` to restore timed-plus items. Login/logout or inventory mutation races the worker and invalidates `CGObjPC`, item, or map iterators.
- **Expected impact:** GameServer memory corruption/deadlock, item-state divergence, or restart hang.
- **Recommended fix:** mark the subsystem retired at compile time or delete it after archival. If functionality returns, let a DB worker discover expirations but enqueue all object mutations onto the GameServer's native command/game thread; use stored procedures/transactions for item persistence.

### KMT-AUD-011 — ShardManager locked-item loader can spin forever on fetch error if re-enabled

- **Category:** B) Server performance; D) Architecture
- **Priority:** **MEDIUM — P2 (dormant)**
- **File:** `ShardManager/vSRO-ShardManager/AsyncGSCommands.cpp`
- **Function / class:** `AsyncGSCommands::LoadLockedItemList`
- **Exact location:** lines 1207-1253, especially the loop at lines 1231-1234.
- **Danger:** the loop terminates only on `SQL_NO_DATA`. A persistent `SQL_ERROR`/`SQL_INVALID_HANDLE` also satisfies `!= SQL_NO_DATA`, so it repeatedly inserts stale values without advancing. The only visible call is currently commented out.
- **Real vSRO scenario:** the loader is re-enabled and SQL drops during result fetch.
- **Expected impact:** pegged CPU, ShardManager hang, and unresponsive startup.
- **Recommended fix:** use the existing tri-state fetch wrapper, break/log on error, stage results, and swap only after a complete successful read.

### KMT-AUD-014 — GameServer custom wide-string reader narrows byte count to signed 16-bit

- **Category:** A) Real crashes; D) Architecture
- **Priority:** **MEDIUM — P2**
- **File:** `gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/GameServer/src/GSLog/MsgCustom.cpp`
- **Function / class:** `CMsgStreamBufferCustom::ReadStringW`
- **Exact location:** lines 105-119.
- **Danger:** the method validates a `size_t byteCount`, then casts it to `__int16` for `Read`. Lengths over 16,383 UTF-16 characters become negative on 32-bit Windows and throw after allocation. It is contained by callers that catch C++ exceptions, but it is an inconsistent parser boundary.
- **Real vSRO scenario:** a malformed internal/log packet declares a large wide string within an unusually large native buffer.
- **Expected impact:** dropped packet/log and exception-path load; a crash is unlikely with the current checked `Read`.
- **Recommended fix:** cap the character length before allocation/cast and make the primitive read count `size_t` throughout.

### KMT-AUD-015 — Source has no single automated native packet-boundary test suite

- **Category:** D) Architecture
- **Priority:** **LOW — P3**
- **Files:** `gameserver/tests/*.Tests.ps1`, `ShardManager/tests/ShardManagerSourceSafety.Tests.ps1`
- **Exact location:** repository test scripts as a group.
- **Danger:** existing source-safety scripts are valuable regression checks but mainly assert source patterns. They cannot exercise corrupted stream lengths, ABI mismatch, thread teardown, or ODBC wait behaviour in the injected v188 host.
- **Real vSRO scenario:** a syntactically safe change passes scripts but fails only on a real `SR_GameServer.exe` packet or shutdown sequence.
- **Expected impact:** late discovery during staging/production.
- **Recommended fix:** add a small x86 native harness around checked message readers and worker lifecycle code, plus an injected-host smoke test on the exact supported binaries.

### KMT-AUD-016 — Dynamic SQL identifiers rely on distributed validation conventions

- **Category:** C) Security; D) Architecture
- **Priority:** **LOW — P3**
- **Files:** `filter/KMTGuardnew/KMTGuard/Features/AutoEvents/AutoEventService.cs` and related event/clientless services
- **Function / class:** `EventTable`-based persistence helpers and configured shard-database queries
- **Exact location:** representative interpolation at lines 1542-1584.
- **Danger:** values are parameterised, while database/table identifiers are interpolated because SQL Server cannot parameterise identifiers. Current helpers/config validation make this a controlled pattern, not a confirmed injection. The risk is future code bypassing validation.
- **Real vSRO scenario:** an operator-controlled database name is later accepted without the existing identifier allow-list and becomes part of an executable query.
- **Expected impact:** configuration-origin SQL injection or queries against an unintended database.
- **Recommended fix:** centralise identifier quoting/allow-listing in one helper, expose typed database identifiers, and add a test rejecting brackets, separators, comments, whitespace, and multipart names outside the approved set.

### KMT-AUD-017 — Packet pipeline registration collections are mutable non-concurrent types

- **Category:** D) Architecture
- **Priority:** **LOW — P3**
- **File:** `filter/KMTGuardnew/KMTGuard/PacketHandler/PacketHandler.cs`
- **Function / class:** `PacketHandler` registration/unregistration and `ExecutePipeline`
- **Exact location:** handler collections and registration methods at lines 20-127; pipeline enumeration later in the same class.
- **Danger:** `SortedDictionary`/`HashSet` are safe under the present startup-only registration convention, but public runtime unregister methods can mutate a dictionary while sessions enumerate it.
- **Real vSRO scenario:** a future live feature reload removes a handler while another session processes that opcode.
- **Expected impact:** `InvalidOperationException` and one affected session disconnect; not a global server crash because handler errors are contained.
- **Recommended fix:** document and enforce startup-only mutation, or publish immutable per-opcode handler snapshots atomically.

## 5. Stability improvement roadmap

### Phase 0 — Release gate (immediate)

1. Fix KMT-AUD-001 and KMT-AUD-002 before shipping another native add-on build.
2. Add deterministic SQL-stall tests: block ODBC for longer than the current wait, request shutdown, and prove no shared resource is released before worker exit.
3. Fix the bounded string consumer in KMT-AUD-007 and fuzz action 8 with lengths 0, 1, 127, 128, 4,095, and truncated payloads.
4. Do not re-enable `CustomTimedJobManager` or `LoadLockedItemList` in their current forms.

### Phase 1 — Packet and runtime containment

1. Add server-to-client packet budgets with explicit native massive-packet exceptions (KMT-AUD-004).
2. Make `DatabaseJobQueue` generation-safe and fully joined (KMT-AUD-005).
3. Apply native ODBC timeouts/cancellation consistently (KMT-AUD-006).
4. Exercise malformed client, custom, and internal packets against the exact supported v188 binaries; verify that only the offending session/message is rejected.

### Phase 2 — Client stability

1. Move final client-hook publication to the proven client main/bootstrap thread or make it transactionally gated (KMT-AUD-003).
2. Test the canonical client fingerprint with early injection, late injection, alt-tab, resolution changes, device lost/reset, teleport, character select, and repeated reconnect.
3. Run at least a two-hour D3D9/UI soak while opening every custom window and forcing resource-load failures.

### Phase 3 — Data/event reliability

1. Standardise Filter command timeout/cancellation and recover abandoned event runs (KMT-AUD-008).
2. Correct ODBC status classification and permanent-invalid command handling (KMT-AUD-009).
3. Verify all deployed `Hook_*`, command-claim/complete/retry, stall, reward, and event stored procedures against the source migrations. **Current live procedure bodies: Unknown / Needs Verification.**
4. Run concurrency tests for duplicate rewards, duplicate queue claims, disconnect cleanup, offline stalls, and Filter restart midway through each event phase.

### Phase 4 — Security and operations

1. Centralise dynamic SQL identifier handling (KMT-AUD-016).
2. Establish metrics/alerts for queue depth, dropped background jobs, SQL duration/timeouts, packet guard trips, massive packet bytes, event recovery, and worker shutdown duration.
3. Retain minidumps and correlate Filter/ShardManager/GameServer logs by command/event/session identifiers without logging secrets or raw authentication packets.

## Verification boundaries

The following cannot be proven from this repository alone and must remain **Unknown / Needs Verification** until tested against the deployed environment:

- actual bodies and permissions of stored procedures not represented by the migration tree;
- exact production database schema, indexes, data sizes, blocking behaviour, and SQL recovery model;
- binary fingerprints and ABI layouts of the deployed `SR_GameServer.exe`, `ShardManager.exe`, and `sro_client.exe`;
- third-party loader order and other injected DLL hooks;
- firewall/reverse-proxy ACLs around Agent/Gateway/Download endpoints;
- production packet distributions, bot variants, peak concurrency, event mix, and network failure modes.

No recommendation in this report requires rewriting the vSRO architecture. The roadmap deliberately keeps native object work on the GameServer thread, SQL work on bounded workers, Filter packet processing compatible with v188, and client hooks pinned to the supported executable.
