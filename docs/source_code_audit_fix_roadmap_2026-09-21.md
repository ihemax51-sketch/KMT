# KMTGuard vSRO 188 — Production Fix Roadmap

**Roadmap date:** 2026-09-21  
**Source audit:** `docs/source_code_audit_2026-09-21.md`  
**Scope:** repair planning only; this document does not change runtime code, packets, SQL, hooks, or media

## Operating rules for the repair programme

This plan preserves the current vSRO 188 architecture. It does not move native player-object work away from the GameServer thread, replace the Filter proxy model, change opcodes, or make the Client DLL binary-independent. Fixed addresses remain acceptable only for the already-supported executable fingerprint. Database work remains on bounded workers, and player-visible protocol behaviour remains compatible with native clients and supported bots.

Before every production deployment:

1. reproduce the issue or its unsafe state in an isolated v188 staging shard;
2. build only the affected component and retain the previous known-good binary;
3. validate with the exact production `SR_GameServer.exe`, `ShardManager.exe`, and `sro_client.exe` builds;
4. test clean startup, active-player operation, graceful shutdown, and forced SQL/network failure;
5. deploy native DLL changes only during maintenance, one component group at a time;
6. preserve packet/opcode layouts unless a finding explicitly requires rejecting malformed input;
7. use a production-sized database copy with sensitive data removed for blocking and recovery tests.

## Phase summary and gates

| Phase | Findings | Exit gate |
|---|---|---|
| **1 — Release Blocking Stability** | KMT-AUD-001, 002 | No worker can outlive native state; repeated SQL-stalled shutdowns are clean |
| **2 — Server Stability** | KMT-AUD-004, 005, 006, 008, 009, 010, 011, 014 | Bounded workers/packets, deterministic SQL cancellation, reliable events |
| **3 — Client Stability** | KMT-AUD-003 | Hook publication is atomic/gated on the supported client and survives D3D9 resets |
| **4 — Security Improvements** | KMT-AUD-007, 012, 013, 016 | Native packet strings are bounded and public trust/error boundaries are explicit |
| **5 — Optimization** | KMT-AUD-015, 017 | Native regression harness and immutable/startup-only packet registrations are enforced |

---

# PHASE 1 — Release Blocking Stability

Only the two critical worker-lifetime findings belong in this phase. Do not combine these repairs with gameplay changes.

## KMT-AUD-001 — ShardManager unique-log worker shutdown lifetime

### 1. Issue ID

`KMT-AUD-001`

### 2. Priority

**Critical / P0 — release blocking**

### 3. Affected component

**ShardManager**, **Database**, worker/thread lifecycle.

### 4. Current problem explanation

`UniqueLogQueue::Shutdown` signals the worker and waits five seconds, but it releases the event, queue data, connection string, and thread handle without proving that the worker exited. In a real vSRO shard, `Hook_UniqueKill` or `Hook_UniqueSpawn` may be inside ODBC while SQL Server is blocked or unreachable. Closing a Windows thread handle does not stop that worker. It can return after teardown and touch ShardManager DLL state that is already being unloaded.

This most commonly appears during maintenance restart, not normal killing/spawning. The symptom can therefore be misdiagnosed as a random ShardManager shutdown crash or a Windows service stop failure.

### 5. Recommended fix

**Files/functions:**

- `ShardManager/vSRO-ShardManager/Runtime/UniqueLogQueue.cpp`
  - anonymous `Worker`
  - `UniqueLogQueue::Initialize`
  - `UniqueLogQueue::Shutdown`
- `ShardManager/vSRO-ShardManager/Database/SQLCommand.cpp/.h`
  - add a safe active-statement cancellation operation if one is not already exposed.

**Expected modifications:**

1. Introduce an explicit lifecycle state (`Stopped`, `Running`, `Stopping`) instead of using the running flag as both state and probe.
2. Keep ownership of the worker handle until a successful join.
3. On shutdown, set `Stopping`, signal `s_event`, and call `SQLCancelHandle(SQL_HANDLE_STMT, ...)` on the worker's active statement through synchronized ownership.
4. Make reconnect delays wait on the stop event instead of using uninterruptible `Sleep`.
5. Wait for worker completion and inspect `WAIT_OBJECT_0`, `WAIT_TIMEOUT`, and `WAIT_FAILED` separately.
6. Clear the queue, connection string, event, and critical-section-owned state only after `WAIT_OBJECT_0`.
7. If the service must enforce a hard stop deadline, log the failed join and let the host process terminate; do not unload the DLL and continue with a live worker.

This is safe for vSRO because it changes only add-on worker ownership. It does not touch ShardManager message dispatch, `0x8888`, unique packet formats, or GameServer communication.

### 6. Risk assessment

| Question | Assessment |
|---|---|
| Online players | No gameplay change. Unique-history persistence can pause briefly during SQL failure. |
| Packets | No packet/opcode change. Unique spawn/kill messages are queued exactly as before. |
| Database consistency | Improved. At shutdown, queued events need an explicit drain-or-drop policy and a logged count. Do not replay a partially successful procedure blindly. |
| Downtime required | **Yes.** Replace the ShardManager add-on and restart ShardManager in a full maintenance window. |

### 7. Testing procedure

- **Local test:** start/stop the queue 1,000 times; assert one worker generation, no leaked handles, and no access after `Shutdown` returns.
- **Stress test:** enqueue 4,096 mixed spawn/kill events while SQL has 2–10 second latency; confirm bounded memory and orderly draining.
- **Restart test:** block `Hook_UniqueKill` in SQL for longer than five seconds, request ShardManager shutdown, unblock it, and confirm teardown waits safely or deliberately terminates the host without DLL unload.
- **Player simulation:** run repeated unique spawns/kills from multiple GameServers while starting maintenance shutdown.
- **Packet test:** verify original unique messages and cross-GameServer broadcasts are byte-for-byte unchanged.
- **Database test:** count accepted, committed, retried, and dropped events; verify no duplicate unique-history rows after reconnect.

## KMT-AUD-002 — GameServer security-refresh worker lifetime

### 1. Issue ID

`KMT-AUD-002`

### 2. Priority

**Critical / P0 — release blocking**

### 3. Affected component

**GameServer**, **Database**, native memory/thread lifetime.

### 4. Current problem explanation

`CSqlCon::Shutdown` signals the security snapshot worker and waits five seconds. It then closes the stop event and deletes `m_connectionstr` even when the wait did not prove worker exit. During real operation the worker refreshes locked-item and fortress-DPS snapshots. A SQL block during GameServer maintenance can leave it inside ODBC while the main shutdown path frees the shared connection.

The SQL critical section prevents simultaneous use but does not solve lifetime: a worker already owning or waiting for that lock still exists after the timed wait. When it resumes, the result is a native use-after-free or shutdown deadlock inside `SR_GameServer.exe`.

### 5. Recommended fix

**Files/functions:**

- `.../GameServer/GameServer/src/SqlConnection/sqlCon.cpp/.h`
  - `SecuritySnapshotRefreshWorker`
  - `CSqlCon::StartSecuritySnapshotRefresh`
  - `CSqlCon::Shutdown`
  - snapshot loader statement helpers
- the scoped ODBC statement helper used by `LoadLockedItems` and `LoadFortressDPSInfo`.

**Expected modifications:**

1. Give the refresh worker exclusive lifecycle ownership of its stop event and retain the thread handle until join.
2. Track the active ODBC statement under a small dedicated lock and cancel it during shutdown.
3. Configure finite login/query timeouts before executing snapshot SQL.
4. Recheck the stop event between each snapshot query and before telemetry/log writes.
5. Treat `WAIT_TIMEOUT` as shutdown failure; do not close the event or delete `m_connectionstr` on that path.
6. Preserve staged-cache semantics: swap a snapshot into live state only after a complete successful query.
7. Keep all refresh SQL off the vSRO game loop.

This keeps native object and packet handling unchanged. It hardens only the add-on's auxiliary SQL worker and therefore matches the vSRO restriction that the real-time loop must never wait on database I/O.

### 6. Risk assessment

| Question | Assessment |
|---|---|
| Online players | No visible change while running. Security snapshots may remain at the last valid version during SQL failure, as today. |
| Packets | None. No packet encoding or handler order changes. |
| Database consistency | Read-only snapshot queries; no expected data mutation. |
| Downtime required | **Yes.** Replace the GameServer add-on and restart every GameServer. |

### 7. Testing procedure

- **Local test:** inject a controllable blocking statement and verify the worker sees stop/cancel before connection destruction.
- **Stress test:** refresh large locked-item and fortress datasets repeatedly while players generate normal combat/inventory load.
- **Restart test:** perform 100 GameServer restarts with SQL delays of 0, 4, 6, and 30 seconds; require zero hangs, crashes, or leaked worker handles.
- **Player simulation:** keep characters fighting, teleporting, and using locked items during refresh and shutdown initiation.
- **Packet test:** compare combat/inventory packet traces before and after; they must not change.
- **Database test:** force mid-fetch failure and prove the previous complete snapshot stays active and no partial cache is published.

### Phase 1 release gate

- Both native components pass stalled-SQL shutdown testing.
- No worker handle is closed before a successful join.
- No native shared connection/event/queue state is released while a worker generation is alive.
- GameServer and ShardManager each complete at least 100 controlled restart cycles.

---

# PHASE 2 — Server Stability

## KMT-AUD-004 — Bound Filter server-to-client traffic

### 1. Issue ID

`KMT-AUD-004`

### 2. Priority

**High / P1**

### 3. Affected component

**Filter**, **Packet System**

### 4. Current problem explanation

Client ingress is guarded, but upstream Agent/GameServer traffic can assemble up to the configured massive-packet ceiling and then be copied to a client without an equivalent opcode-aware budget. A broken custom broadcast can multiply a large allocation by every online session. Native vSRO character/item packets can legitimately be massive, so a generic small limit would cause real login or teleport disconnects.

### 5. Recommended fix

- Change `Session.DoReceiveFromServer` and add a dedicated `TryPassServerPacketGuards` helper in `filter/KMTGuardnew/KMTGuard/Session/Session.cs`.
- Define separate limits for ordinary native packets, approved native massive responses, and KMTGuard custom opcodes.
- Count assembled bytes and packets in a rolling per-session window before handler execution and before egress.
- Build the native massive allow-list from captured v188 login, inventory, storage, guild, teleport, and character-data traffic; do not guess it.
- On violation, log sampled opcode/length/service metadata and stop only the affected session. Never log credentials or full packet bodies.
- Add counters for rejected upstream packets and massive bytes.

This preserves native framing and opcode behavior; only structurally impossible or administratively bounded traffic is rejected.

### 6. Risk assessment

- **Players:** a wrong threshold can disconnect players on character load, storage, guild data, or crowded spawns.
- **Packets:** yes; this deliberately adds validation but does not rewrite valid packets.
- **Database:** none.
- **Downtime:** Filter restart required; GameServer/ShardManager/client maintenance is not required.

### 7. Testing procedure

- Local: unit-test boundary lengths and rolling-window reset logic.
- Stress: replay production-sized massive packets to 500–1,000 simulated sessions and measure LOH/GC behavior.
- Restart: restart only Filter roles and ensure counters reset safely.
- Player: login full inventories, open storage/guild/academy, teleport, stall, and participate in large events.
- Packet: replay valid captures plus oversized, over-fragmented, truncated, and custom broadcast payloads.
- Database: not applicable beyond confirming no session cleanup backlog after rejected sessions.

## KMT-AUD-005 — Make Filter database-worker generations joinable

### 1. Issue ID

`KMT-AUD-005`

### 2. Priority

**High / P1**

### 3. Affected component

**Filter**, **Database**, workers

### 4. Current problem explanation

The static queue can cancel, wait two seconds, dispose its token source, and later start a new generation even if an old SQL action is still completing. During a Filter restart under SQL blocking, old and new workers can overlap and apply cleanup or commands out of lifecycle order.

### 5. Recommended fix

- Refactor `DatabaseJobQueue` in `Helpers/DatabaseJobQueue.cs` around a private per-generation object containing its channel, CTS, workers, and generation ID.
- Workers capture that object rather than reading replaceable static fields.
- `StopAsync` prevents `Start` until all workers join; it may report a failed graceful stop but must not dispose state used by live workers.
- Ensure every queued Dapper operation receives the worker token through `CommandDefinition`.
- Classify jobs as idempotent or non-idempotent. Retry only the former; lease/persist important non-idempotent commands.
- Emit queue depth, oldest-job age, dropped-job count, stop duration, and generation ID.

### 6. Risk assessment

- **Players:** disconnect cleanup and delayed commands may complete later during SQL failure, but duplicate effects should be reduced.
- **Packets:** no wire changes; packet handlers may wait under bounded back-pressure only where they already call `RunAsync`.
- **Database:** positive impact, but incorrect retry classification could duplicate rewards or cleanup.
- **Downtime:** Filter restart required.

### 7. Testing procedure

- Local: lifecycle tests for start/stop/start, cancellation, full queue, and exception propagation.
- Stress: saturate 512 jobs with two workers and repeated cancellation; prove bounded memory.
- Restart: block a 30-second SQL command, stop, attempt start, and verify no second generation starts early.
- Player: mass disconnect/reconnect, party cleanup, HWID updates, stalls, and concurrent commands.
- Packet: verify packet receive loops remain responsive while background jobs are saturated.
- Database: use transaction/audit IDs to prove each non-idempotent action executes once.

## KMT-AUD-006 — Add deterministic native ODBC timeouts

### 1. Issue ID

`KMT-AUD-006`

### 2. Priority

**High / P1**

### 3. Affected component

**GameServer**, **Database**

### 4. Current problem explanation

GameServer add-on statements do not consistently set a query timeout. A blocked query can own the shared SQL lock indefinitely, prevent security refresh, and make GameServer shutdown unsafe. Any synchronous call made from a hooked game-thread path would also freeze the entire shard loop.

### 5. Recommended fix

- In `sqlCon.cpp` scoped statement creation, set `SQL_ATTR_QUERY_TIMEOUT` before execution and configure login timeout at connection creation.
- Add a narrow configuration value with a conservative production default; do not read settings from SQL inside the failing SQL path.
- Pass the shutdown cancellation state to statement execution and cancel active handles.
- Inventory every `TryExecNonQuery` caller. Move any call reachable from combat, inventory, packet, or timer hooks to the existing worker/command bridge.
- Retain telemetry for elapsed time, timeout, cancellation, and caller operation.

### 6. Risk assessment

- **Players:** overly short timeouts may defer security/config refresh but prevent whole-shard freezes.
- **Packets:** none unless a previously blocking hook is moved async; preserve response ordering when doing so.
- **Database:** timed-out writes require explicit unknown-outcome handling; do not automatically retry non-idempotent writes.
- **Downtime:** full GameServer maintenance required.

### 7. Testing procedure

- Local: confirm statement attributes and timeout diagnostics.
- Stress: lock target tables for longer than the timeout under combat load.
- Restart: stop during connect, execute, fetch, and retry delay.
- Player: combat, item use, locked-item operations, fortress DPS, and teleport while SQL is degraded.
- Packet: measure GameServer tick/packet latency; no stall should match SQL duration.
- Database: distinguish cancelled-before-commit from unknown commit outcome using audit keys.

## KMT-AUD-008 — Event SQL cancellation and recovery

### 1. Issue ID

`KMT-AUD-008`

### 2. Priority

**High / P1**

### 3. Affected component

**Filter**, **Database**, **Event System**

### 4. Current problem explanation

Auto-event run/round persistence uses provider defaults and often lacks the owning event cancellation token. A SQL block can leave an event visibly stuck between round notices, winner selection, reward, and finalization. Operators may then issue stop/start commands and accidentally create overlapping recovery actions.

### 5. Recommended fix

- Update `AutoEventService` persistence helpers (`CreateRunAsync`, `CreateRoundAsync`, `CompleteRoundAsync`, `FinishRunAsync`) to accept the event lifetime token.
- Use `CommandDefinition` with explicit timeout and cancellation for opens and commands.
- Preserve conditional updates such as `FinishedAtUtc IS NULL` and check affected-row counts.
- On startup, lease/reconcile stale `Running` runs before scheduling new ones.
- Give reward issuance a durable idempotency key `(RunID, RoundID, RewardType, WinnerCharID)`.
- Apply the same pattern to Survival Party/Solo and other event services after the base implementation is proven.

### 6. Risk assessment

- **Players:** event cancellation behavior changes; notices/rewards must remain ordered.
- **Packets:** event notice opcodes remain unchanged.
- **Database:** high sensitivity around exactly-once rewards; use unique constraints or transactional claim records.
- **Downtime:** Filter restart; SQL update only if idempotency/lease columns or constraints are introduced.

### 7. Testing procedure

- Local: cancel each helper before open, during execute, and after commit.
- Stress: run maximum concurrent events with artificial SQL delay and connection-pool limits.
- Restart: terminate Filter at every event state and verify deterministic recovery.
- Player: simulate winners disconnecting, teleporting, changing character, and reconnecting during reward.
- Packet: assert one start/end/winner notice sequence per run.
- Database: verify no duplicate rewards and no permanent stale `Running` rows.

## KMT-AUD-009 — Correct ShardManager ODBC result classification

### 1. Issue ID

`KMT-AUD-009`

### 2. Priority

**Medium / P2**

### 3. Affected component

**ShardManager**, **Database**

### 4. Current problem explanation

`SQLCommand::GetData` rejects `SQL_SUCCESS_WITH_INFO`. For command strings at a buffer boundary, ODBC truncation is treated like a database failure, causing reconnect/retry behavior instead of permanently rejecting bad command data.

### 5. Recommended fix

- Modify `SQLCommand::GetData` to use `SQL_SUCCEEDED` and always inspect the indicator length.
- Return a typed result (`Success`, `Null`, `Truncated`, `Error`) rather than a single boolean.
- In `AsyncGSCommands`, mark truncated/invalid parameters failed with a safe reason; reconnect only on real transport/statement errors.
- Ensure all character arrays are zero-initialized and require room for a terminator.

### 6. Risk assessment

- Players: only queued GM/system actions are affected.
- Packets: invalid commands will no longer reach `0x8888`; valid layouts stay unchanged.
- Database: command status transitions change and must remain atomic.
- Downtime: ShardManager restart required.

### 7. Testing procedure

- Test values of 0, 1, 126, 127, 128, and 129 bytes plus NULL/non-ASCII.
- Force `SQL_SUCCESS_WITH_INFO` and verify no reconnect loop.
- Restart with claimed commands and verify lease recovery.
- Simulate affected GM commands against online/offline characters.
- Capture valid `0x8888` messages and compare bytes.
- Confirm invalid rows reach one terminal failure state.

## KMT-AUD-010 — Permanently isolate the dormant timed-item worker

### 1. Issue ID

`KMT-AUD-010`

### 2. Priority

**Medium / P2; Critical if re-enabled**

### 3. Affected component

**GameServer**, **Database**

### 4. Current problem explanation

The dormant worker accesses `g_pCGame`, player maps, inventory/items, skills, and message allocation from a foreign infinite thread while also running SQL. That violates GameServer thread ownership and can race login/logout or item mutation. It currently has no active caller, which is why it is not an active critical defect.

### 5. Recommended fix

- Add an explicit compile-time retirement guard around `CustomTimedJobManager` and a source-safety test forbidding `CreateConsumeThread` calls.
- Do not merely add a mutex around GameServer objects.
- If timed-plus returns later, let a DB worker discover expirations and enqueue a validated native command to the GameServer thread; perform all `CGObjPC`/inventory/message operations there.
- Persist the item transition transactionally before/after the in-memory mutation according to a documented recovery rule.

### 6. Risk assessment

- Players/packets/database: no impact while it stays dormant.
- Downtime: no runtime deployment needed for documentation/test guard; a future functional replacement requires GameServer maintenance and likely SQL validation.

### 7. Testing procedure

- Source test ensures no production call exists.
- If replaced, stress login/logout, inventory moves, deaths, teleports, and 10,000 expirations.
- Restart at every persistence/mutation boundary.
- Verify item packets and final plus values.
- Reconcile database and in-memory item state after crash recovery.

## KMT-AUD-011 — Fix dormant locked-item fetch termination

### 1. Issue ID

`KMT-AUD-011`

### 2. Priority

**Medium / P2**

### 3. Affected component

**ShardManager**, **Database**

### 4. Current problem explanation

The dormant loader loops until `SQL_NO_DATA`; `SQL_ERROR` also remains inside the loop, potentially pegging a ShardManager CPU core forever if the loader is re-enabled.

### 5. Recommended fix

- Keep the loader disabled until repaired.
- Replace the raw loop with `FetchDataResult()` and explicit `ROW`, `NO_DATA`, and `ERROR` branches.
- Load into a local map and swap only after complete success.
- Add row-count and maximum-duration telemetry.

### 6. Risk assessment

- No online impact while dormant.
- No packet changes.
- Read-only database operation; last valid cache must remain active on error.
- ShardManager restart required only if the repaired loader is deployed/enabled.

### 7. Testing procedure

- Inject fetch error after rows 0, 1, and N.
- Test a production-sized locked-item table.
- Restart during fetch.
- Verify online locked-item behavior remains unchanged.
- Confirm no `0x8888` changes.
- Compare complete cache contents to a database snapshot.

## KMT-AUD-014 — Remove signed narrowing in the native wide-string reader

### 1. Issue ID

`KMT-AUD-014`

### 2. Priority

**Medium / P2**

### 3. Affected component

**GameServer**, **Packet System**

### 4. Current problem explanation

`ReadStringW` validates a `size_t` byte count and then narrows it to signed `__int16`. Large declared lengths can wrap negative and enter exception handling after allocation. It is contained today, but malformed internal/log traffic can create avoidable allocation and exception pressure.

### 5. Recommended fix

- Change the primitive checked `Read` count to `size_t`, narrowing only at the verified native ABI call boundary in chunks if required.
- Establish an explicit maximum wide-string length suitable for the actual log packet.
- Check multiplication overflow, remaining bytes, and maximum length before allocation.
- Reject malformed input without modifying read position.

### 6. Risk assessment

- Players: no expected visible effect.
- Packets: malformed log/internal packets are rejected; valid wire format is unchanged.
- Database: none.
- Downtime: GameServer add-on replacement/restart required.

### 7. Testing procedure

- Boundary/fuzz lengths including truncated UTF-16.
- Sustained malformed packet stress with memory monitoring.
- Repeated GameServer restart after parser exceptions.
- Normal player log/combat simulation.
- Byte-for-byte valid packet tests.
- Database not applicable.

### Phase 2 release gate

- Filter remains responsive at target peak connections during oversized upstream traffic and SQL delay.
- All worker generations join deterministically.
- Event restart produces no duplicate rewards or overlapping runs.
- Native ODBC errors are classified without infinite retry/spin.
- Dormant unsafe workers remain mechanically disabled.

---

# PHASE 3 — Client Stability

## KMT-AUD-003 — Publish client hooks safely

### 1. Issue ID

`KMT-AUD-003`

### 2. Priority

**High / P1**

### 3. Affected component

**Client DLL**, DirectX9/UI hook initialization

### 4. Current problem explanation

The DLL correctly avoids doing heavy work under loader lock, but `SetupWithDiagnostics` publishes fixed-address/vtable hooks from a created worker while the client startup thread may already be creating `CGInterface`, runtime classes, or the D3D9 device. On fast starts, late injection, or another loader's hook ownership, the client can execute a partially published hook set. This manifests as intermittent startup crashes, missing custom windows, black screen, or first-reset failure.

### 5. Recommended fix

- Work in `JTClientLibrary/source/DevKit_DLL/src/DllMain.cpp`, the bootstrap-gate implementation, `Util.cpp` hook setup, and the D3D9/UI callback registration helpers.
- Keep `DllMain` limited to host validation, bootstrap gate installation, and worker creation.
- Marshal final `SetupWithDiagnostics` publication onto the known client startup/main thread before `InstallRuntimeClasses`/UI asset creation.
- If a loader conflict prevents that route, commit all patch writes as one checked transaction while the bootstrap gate blocks entry; validate original bytes, change protection, write, flush instruction cache, restore protection, then publish ready state.
- Do not hold the client/game thread during resource I/O or SQL/network work.
- Make D3D callbacks tolerate device lost/reset and missing/not-yet-created UI/resources; do not dereference cached UI pointers across character or interface recreation.
- Preserve the exact supported v188 fingerprint and fail closed before any partial hook is usable.

### 6. Risk assessment

| Question | Assessment |
|---|---|
| Online players | Existing clients are unaffected until they replace/restart the DLL. A bad initialization change can prevent all new client starts. |
| Packets | No protocol changes intended. Confirm registration order does not drop early custom packets. |
| Database | None. |
| Downtime required | Server can remain online, but every player must replace the DLL and restart the client. Schedule a client maintenance release to avoid mixed versions. |

### 7. Testing procedure

- **Local:** verify original bytes/fingerprint and one-time initialization state transitions.
- **Stress:** launch/close hundreds of clients with randomized injection timing and competing overlay/loader presence.
- **Restart:** character relog, client restart, reconnect after DC, and repeated launcher starts.
- **Player:** character select, teleport, death/resurrection, stalls, all custom windows, high-object-count towns.
- **Packet:** send early custom packets before/while UI creation; they must queue or fail safely, not touch missing objects.
- **DirectX9:** alt-tab, minimize/restore, resolution/fullscreen changes, device lost/reset loops, texture load failure, missing optional resources, and at least a two-hour UI/D3D soak.

### Phase 3 release gate

- Zero partial-hook starts across the launch matrix.
- Zero D3D9 reset or missing-resource crashes.
- All custom UI classes register once and are reacquired safely after interface recreation.
- Supported native/bot packet behavior remains unchanged.

---

# PHASE 4 — Security Improvements

## KMT-AUD-007 — Bound the GameServer internal command string

### 1. Issue ID

`KMT-AUD-007`

### 2. Priority

**High / P1**

### 3. Affected component

**GameServer**, **ShardManager**, **Packet System**

### 4. Current problem explanation

Action 8 reads a length-prefixed CodeName into `char[128]`. The current ShardManager producer normally bounds the database read, but a vSRO internal consumer must validate independently: version skew, corruption, or a compromised internal sender can still deliver a larger declared length. Authentication proves sender/message integrity, not semantic safety.

### 5. Recommended fix

- In `Game.cpp` `CGame::ProcessMessage`, read action 8 into a checked `std::string` with maximum 127 bytes and validate remaining payload before use.
- Add reusable bounded string readers to the GameServer message wrapper.
- In `AsyncGSCommands.cpp`, preserve producer bounds and reject truncation using the KMT-AUD-009 typed ODBC result.
- Reject the command without calling `EngageBuffSkill`; log command/action IDs, never raw secrets.
- Audit adjacent fixed arrays through the same helper.

### 6. Risk assessment

- Players: only malformed/admin commands are rejected.
- Packets: valid `0x8888` action 8 wire layout stays unchanged.
- Database: invalid rows should reach a terminal failure state rather than retry forever.
- Downtime: GameServer and ShardManager add-ons should be deployed together during full maintenance.

### 7. Testing procedure

- Unit/fuzz strings at all boundaries and truncated frames.
- Flood invalid action 8 messages without growing memory/log volume.
- Restart with invalid claimed commands.
- Apply valid skills to online/offline characters.
- Compare valid wire bytes before/after.
- Verify invalid command terminal status and valid command completion.

## KMT-AUD-016 — Centralize dynamic SQL identifier validation

### 1. Issue ID

`KMT-AUD-016`

### 2. Priority

**Low / P3**

### 3. Affected component

**Filter**, **Database**, **Event System**

### 4. Current problem explanation

SQL data values are parameterized, but database/table identifiers must be interpolated. Existing helpers make current usage controlled; the risk is a future event/clientless query bypassing scattered identifier validation.

### 5. Recommended fix

- Introduce one identifier type/helper that accepts only configured database names and known constant table names.
- Quote with SQL Server identifier rules after allow-list validation; do not accept arbitrary multipart text.
- Replace distributed `EventTable`/shard-name interpolation gradually without changing query semantics.
- Add analyzer/source tests that reject direct configured-identifier interpolation.

### 6. Risk assessment

- Players: none if validated configuration is unchanged.
- Packets: none.
- Database: a too-strict validator can prevent event/clientless startup on unusual but valid database names.
- Downtime: Filter restart; no SQL update expected.

### 7. Testing procedure

- Accept production database names and reject brackets, quotes, comments, separators, whitespace tricks, and unapproved multipart names.
- Run all event/clientless queries against staging.
- Restart with valid and intentionally invalid configuration.
- Simulate event/player operations that touch every migrated query.
- Verify generated SQL targets exactly the intended database/schema/table.

### Phase 4 release gate

- Every native variable-length command field is consumer-bounded.
- Invalid input affects only the command/session, never the host process.
- Public HTTP responses disclose no internal paths/provider messages.
- Only the configured reverse proxy can establish client IP identity.
- Dynamic identifiers pass one central allow-list.

---

# PHASE 5 — Optimization, code quality, and monitoring

## KMT-AUD-015 — Native packet/lifecycle regression harness

### 1. Issue ID

`KMT-AUD-015`

### 2. Priority

**Low / P3**

### 3. Affected component

**GameServer**, **ShardManager**, **Packet System** testing

### 4. Current problem explanation

Source-safety PowerShell checks catch known text patterns but cannot execute malformed stream lengths, worker teardown, ODBC wait behavior, or ABI assumptions. Native failures therefore appear late in injected-host staging.

### 5. Recommended fix

- Add an x86 native test target for checked message readers, lifecycle state machines, wait-result handling, and ODBC result classification.
- Add fixture packets captured from the supported v188 binaries, sanitized of credentials.
- Keep a separate injected-host smoke suite for the exact GameServer/ShardManager executables; do not pretend a mock proves ABI compatibility.
- Publish queue/SQL/packet rejection counters for soak-test assertions.

### 6. Risk assessment

- No player, packet, or database production impact.
- No downtime; test infrastructure only.

### 7. Testing procedure

- Run harness locally and in CI for every native change.
- Fuzz length/count fields under memory diagnostics.
- Run 100 restart cycles in injected hosts.
- Replay representative player/internal packets.
- Execute against a disposable SQL database with delay/error injection.

## KMT-AUD-017 — Enforce safe packet-handler registration

### 1. Issue ID

`KMT-AUD-017`

### 2. Priority

**Low / P3**

### 3. Affected component

**Filter**, **Packet System**

### 4. Current problem explanation

Mutable `SortedDictionary` handler collections are safe under the current startup-only convention, but public unregister methods could race a live session enumeration if runtime feature reload is added.

### 5. Recommended fix

- Preferred minimal fix: seal registration after server initialization and reject later mutation with a clear exception/log.
- If live reload is truly required, build immutable per-opcode arrays and atomically swap complete snapshots.
- Do not put a global lock around awaited packet handlers; that would serialize players and create vSRO lag.
- Update `PacketHandler.cs` registration/unregistration and `ServerManager` initialization sequencing.

### 6. Risk assessment

- Players: startup-only sealing has no effect today; incorrect seal timing can omit handlers.
- Packets: handler order/priority must remain identical.
- Database: none.
- Downtime: Filter restart to deploy.

### 7. Testing procedure

- Verify duplicate priorities and deterministic order.
- Race attempted registration against high packet concurrency.
- Restart every Filter role and compare registered opcode counts.
- Simulate login, movement, combat, chat, custom UI, and bot compatibility.
- Replay packet pipeline tests byte-for-byte.

### Phase 5 release gate

- Native boundary/lifecycle tests execute automatically.
- Production-host smoke tests cover every native release.
- Packet handler sets are immutable during live traffic or atomically swapped.
- Dashboards alert on worker stop time, queue depth, SQL timeout, packet rejection, event recovery, and dropped jobs.

---

# Recommended fixing order

| Order | Finding | Difficulty | Deployment unit | Reason |
|---:|---|---|---|---|
| 1 | KMT-AUD-001 | Hard | ShardManager DLL | Active native teardown crash risk |
| 2 | KMT-AUD-002 | Hard | GameServer DLL | Active native connection lifetime risk |
| 3 | KMT-AUD-006 | Medium–Hard | GameServer DLL | Enables deterministic fix/testing for 002 |
| 4 | KMT-AUD-007 | Medium | GameServer + ShardManager DLLs | Native memory-corruption boundary |
| 5 | KMT-AUD-005 | Medium | Filter | Prevent overlapping database worker generations |
| 6 | KMT-AUD-004 | Hard | Filter | Requires real v188 packet baselines to avoid false DCs |
| 7 | KMT-AUD-008 | Hard | Filter, optional SQL | Reward/event recovery correctness |
| 8 | KMT-AUD-003 | Hard | Client DLL | High regression surface across loader/UI/D3D9 |
| 9 | KMT-AUD-009 | Medium | ShardManager DLL | Correct invalid-command handling |
| 10 | KMT-AUD-014 | Easy–Medium | GameServer DLL | Local parser correction |
| 11 | KMT-AUD-010 | Easy to retire; Hard to restore | Source guard or GameServer DLL | Keep dormant unsafe feature disabled |
| 12 | KMT-AUD-011 | Easy | ShardManager source/DLL if enabled | Dormant fetch-loop correction |
| 13 | KMT-AUD-016 | Medium | Filter | Central hardening with broad query coverage |
| 14 | KMT-AUD-017 | Easy | Filter | Enforce existing startup convention |
| 15 | KMT-AUD-015 | Medium–Hard | Test infrastructure | Prevent recurrence across native releases |

## Deployment and downtime matrix

### Can be prepared or applied without shutting down the game server

- `KMT-AUD-010`: add a source/test retirement guard while the worker remains unused; no binary deployment is necessary until a normal release.
- `KMT-AUD-015`: test harness and monitoring work.
- Documentation, packet-baseline capture, SQL test fixtures, dashboards, and alerting.

### Requires Filter restart but not full shard maintenance

- `KMT-AUD-004`, `005`, `008`, `016`, `017`.
- Use separate Agent/Gateway/Download roles carefully; do not run mixed Filter packet-limit behavior longer than the validation window.
- Existing sessions will disconnect when their proxy role restarts, so schedule a player notice even though GameServer binaries remain online.

### Requires component maintenance

- `KMT-AUD-001`, `009`, and enabled `011`: ShardManager restart. Because GameServers depend on ShardManager coordination, treat this as shard maintenance rather than a transparent hot swap.
- `KMT-AUD-002`, `006`, `014`: restart every GameServer using the add-on.
- `KMT-AUD-003`: servers may stay online, but all clients require DLL replacement and restart; avoid a prolonged mixed-client rollout.

### Requires full server maintenance

- Bundle `KMT-AUD-001`, `002`, `006`, `007`, `009`, and `014` only after each passes independent staging. Replace matching GameServer and ShardManager add-ons together.
- Any `KMT-AUD-008` implementation that adds SQL constraints/columns should be deployed in the same controlled maintenance window after backup and validation.
- Never hot-unload or overwrite an injected native DLL inside a running vSRO process.

# Possible regression risks

| Area | Main regression | Control |
|---|---|---|
| Worker shutdown | Shutdown waits forever because cancellation is incomplete | Finite ODBC timeout, active statement cancellation, explicit hard-stop process policy |
| Unique history | Event lost or duplicated around cancellation | Per-event identity/audit, defined drain/drop rule, stored-procedure idempotency verification |
| GameServer snapshots | Empty/partial security cache becomes live | Stage then swap only after full successful fetch |
| Upstream packet limits | Valid character/storage/guild massive packet causes DC | Build thresholds from real v188 captures and explicit opcode allow-list |
| Filter queue lifecycle | Important cleanup dropped when queue is full | Metrics, differentiated durable jobs, bounded back-pressure |
| Event recovery | Duplicate reward or overlapping run | Durable idempotency keys, conditional updates, startup reconciliation |
| Client hooks | Client cannot start or crashes on D3D reset | Exact fingerprint, original-byte checks, atomic gate, launch/reset soak matrix |
| Internal command parser | Valid long CodeName rejected | Align DB schema, producer maximum, consumer maximum, and terminator rules |
| ODBC result handling | Truncated value accidentally accepted | Check indicator length and return typed truncation result |
| Proxy trust | Legitimate heartbeat seen as proxy IP | Known-proxy configuration tests before production switch |
| SQL identifiers | Valid custom DB name rejected | Validate all current production names before enabling strict helper |
| Registration sealing | Handler registration occurs after seal | Assert initialization order and compare opcode/priority manifest at startup |

# Final production acceptance checklist

1. Phase 1 is complete before any other native release is approved.
2. All exact production binaries pass fingerprint/ABI checks.
3. Valid packet captures remain byte-compatible; malformed inputs fail at the narrowest boundary.
4. GameServer loop latency remains independent of SQL delay.
5. ShardManager and GameServer survive repeated SQL-stalled restarts.
6. Filter survives packet and database saturation with bounded memory.
7. Event crash recovery issues no duplicate reward.
8. Client passes launch, UI, teleport, and DirectX9 lost/reset soak testing.
9. Database migrations, if any, have backup, validation, and rollback procedures.
10. Operators have component-specific rollback binaries and know which services must restart.

The repair programme is complete only after staging evidence is recorded for each finding. Live stored-procedure behavior, production indexes/data volume, third-party loader interaction, and deployed binary fingerprints remain **Unknown / Needs Verification** until tested in the actual customer environment.
