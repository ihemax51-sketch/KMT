# KMTGuard vSRO 188 — Complete GameServer deep audit

**Audit date:** 2026-09-23
**Baseline:** current repository at commit `5178263`
**Scope:** `gameserver/`, including the vSRO 188 `SR_GameServer` add-on, shared framework code reached by it, hooks, patches, native wrappers, packets, SQL, workers, timers, and shutdown. This is an audit-only change.

## 1. Executive summary

The current GameServer add-on is materially safer than an unguarded native extension: it validates the exact x86 executable and expected bytes/pointers before publication, rolls back partial hook installation, applies ODBC login/query timeouts, serializes the shared connection, owns and joins its SQL refresh worker, bounds most custom packet strings, authenticates Filter-originated client extensions, and caps several formerly unbounded caches/batches.

No Critical memory-corruption path was proven in active code. The audit found **2 High, 5 Medium, and 4 Low** active/conditional defects. The leading production risks are (1) an authenticated custom opcode that can spawn fixed unique objects without a local authorization/rate boundary, and (2) main queue-tick Live DPS batching that can sort and serialize 256 mob snapshots in one native tick. Synchronous database calls in the old custom alchemy/timed-item implementation are dangerous, but that implementation is mechanically dormant and is reported separately rather than as active.

| Severity | Active | Conditional/Unknown | Dormant | Total |
|---|---:|---:|---:|---:|
| Critical | 0 | 0 | 0 | **0** |
| High | 2 | 0 | 0 | **2** |
| Medium | 3 | 2 | 0 | **5** |
| Low | 3 | 0 | 1 | **4** |

**Overall:** `REQUIRES TARGETED REPAIR AND WINDOWS STAGING`. The native vSRO design, fixed addresses, packet layouts, and game-thread object ownership should remain intact.

### Audit coverage

- **127** `.cpp/.h` units in `SR_GameServer` (**53 implementation units**) were enumerated; all were searched and the active call graph was traced through the core add-on and reached shared framework/network files.
- Repository scope contained 493 GameServer files including build/documentation assets; 238,470 physical C/C++ lines exist under `gameserver/source`, much of which is reverse-engineered native layout declarations rather than active custom behavior.
- Primary execution units reviewed line-by-line: `DllMain`, `Util`, `StaticPatches`, runtime safety, `CMainProcess`, `CGame`, `CGObjPC`, `CGObjMob`, pets/items/storage, region/trade/party/durability systems, packet authentication, telemetry, ODBC connection/query code, timed-job code, log hooks, crash handling, and native message wrappers.

## 2. Architecture map

```text
SR_GameServer.exe (supported vSRO 188 x86 fingerprint)
  └─ KMT GameServer DLL / DllMain
       └─ initialization thread
            ├─ host metadata + hook-site manifest validation
            ├─ INI and KMTGuard SQL snapshot loading
            ├─ Filter session-key authentication setup
            ├─ reversible static patches/detours/pointer hooks
            ├─ one SQL security-snapshot worker
            └─ READY or fail-closed process termination

Native GameServer owner thread
  ├─ CMainProcess::_OnProcessMessage → CGame::ProcessMessage
  │    ├─ ShardManager/internal commands (0x8888, 0x506x)
  │    └─ bounded exception containment and original handler
  ├─ CGObjPC::ReaderPacket
  │    ├─ original client packets
  │    ├─ Filter-authenticated custom packets
  │    └─ original native handler where not consumed
  └─ CMainProcess::_OnQueueTimer
       ├─ throttled Live DPS pending set/batch
       ├─ telemetry snapshot once/minute
       └─ original native queue timer

SQL refresh worker (only custom long-lived worker)
  └─ every 60 s: locked-item + fortress-DPS snapshots
       └─ serialized ODBC connection; no CGObjPC/world/UI access
```

### Custom-system inventory

| System | Architecture and state | Database/performance/crash assessment |
|---|---|---|
| Item lock/unlock | authenticated client request → ShardManager; refreshed locked-ID snapshot | map lookups are locked; snapshot worker owns SQL; good separation |
| Region/item travel guard | per-player pending state, region hooks, cached restrictions | cleanup hook forgets players; bounded state; native thread |
| Attack/party/FFA restrictions | validated runtime maps and native detours | shared snapshots locked; aggro prune throttled to 1 s/mob |
| Live/Fortress DPS | attack hook queues mob IDs; queue tick resolves current objects | active High performance finding GS-H-02 |
| Durability/trade gold | byte-verified detours and scoped native behavior | reversible; expected vSRO thread affinity |
| Custom packets/features | `CGObjPC::ReaderPacket`, Filter session key/HMAC registration | bounds/auth are good; unique-spawn capability lacks rate/authorization boundary (GS-H-01) |
| Internal GameServer commands | ShardManager `0x8888` actions with field/range validation | trusted service boundary; native main-thread mutation is correct |
| SQL settings/security snapshots | startup loads + 60 s worker refresh | 10 s login, 30 s query timeout, active cancellation/join |
| Timed plus/devil system | archived worker/source remains | mechanically disabled; dangerous dormant code GS-L-04 |
| Telemetry/crash dump | lock-free counters and periodic file snapshot | synchronous file I/O on queue tick: GS-M-01 |

## 3. Complete findings table

| ID | Severity | Area | File | Function | Problem | Risk |
|---|---|---|---|---|---|---|
| GS-H-01 | High | Packet/custom spawn | `KMTGuardCustom/CGObjPCCustom.cpp` | `CGObjPC::ReaderPacket` | authenticated `0x3538` selects five hard-coded uniques with no local privilege/cooldown/quota | spawn abuse, entity pressure, lag/crash |
| GS-H-02 | High | Performance/main loop | `Objects/GObjMob.cpp`, `MainProcess.cpp` | `FlushLiveDpsBatch`, `_OnQueueTimer` | one native tick can resolve/sort/send 256 mob damage maps | queue stall, lag, DC burst |
| GS-M-01 | Medium | Telemetry/I/O | `GameServerTelemetry.cpp` | `ScopedQueueTick::~ScopedQueueTick`, `TryWriteSnapshot` | file stat/rotation/open/write/flush is synchronous on native queue thread | periodic global hitch under slow disk/AV |
| GS-M-02 | Medium | Loader/shutdown | `OutPut/src/DllMain.cpp` | `DllMain`, initialization worker | init-thread handle is discarded and no detach/pin contract prevents DLL unload during/after active hooks | conditional UAF/crash on hot unload |
| GS-M-03 | Medium | Packet fan-out | `CGObjPCCustom.cpp` | `ReaderPacket` (`0x705D`) | custom item-link bridge has no local frequency/budget limit before forwarding to ShardManager | packet amplification/SM load/DC under spam |
| GS-M-04 | Medium | Authentication thread safety | `InternalPacketAuth.cpp` | `ValidateRegistration` | `s_initialized` is read before taking the lock used by initialize/shutdown | data race during conditional shutdown/reinit |
| GS-M-05 | Medium | SQL availability | `sqlCon.cpp` | snapshot worker/loaders | one shared serialized connection means a 30 s refresh query blocks all custom SQL users and shutdown cancellation targets only one active statement | stale security snapshot/delayed shutdown; narrower active impact |
| GS-L-01 | Low | Packet failure policy | `Game.cpp`, `CGObjPCCustom.cpp` | packet wrappers | malformed packets are logged/flushed but repeated offenders are not locally disconnected/rate-counted | log/CPU pressure; Filter normally owns abuse policy |
| GS-L-02 | Low | Error observability | `GObjEvents.cpp` | `CheckRegionNeedChange` | blanket empty catch suppresses allocation/native send failure evidence | silent missing region notification |
| GS-L-03 | Low | Telemetry | `GameServerTelemetry.cpp` | counters/snapshot | 32-bit counters can wrap in exceptionally busy 60 s windows | inaccurate diagnostics, not gameplay corruption |
| GS-L-04 | Low | Dormant memory/thread safety | `CustomTimedJobManager.cpp/.h` | consume workers | archived code accesses live players off-thread, sleeps without stop, erases then reads iterators, and executes synchronous SQL | dormant; catastrophic if compile guard bypassed |

## 4. Critical issues

**None proven in active code.** Fixed addresses and native pointer wrappers were not escalated because the add-on validates the exact image, byte signatures, and vtable/function-pointer owners before installation. Dormant timed-job defects are not active because the header deliberately emits a compile error if the feature is enabled.

## 5. High issues

### GS-H-01 — Authenticated client extension can request unique spawns without authorization or throttling

- **Severity:** High / Active
- **File / function / lines:** `GameServer/GameServer/src/KMTGuardCustom/CGObjPCCustom.cpp`, `CGObjPC::ReaderPacket`, lines 617–646.
- **Current behavior:** after validating the Filter session key, byte `uniquetype` 0–4 directly creates one of five fixed mobs at the player’s current world/position. There is no GM/event-state authorization, per-player cooldown, world allowlist, or aggregate spawn budget.
- **Execution scenario:** a valid Filter-originated request is duplicated/replayed rapidly, or an exposed Filter route permits a normal account to request `0x3538` repeatedly.
- **Expected symptom:** entity growth, event corruption, queue/combat lag, and eventual GameServer instability; unexpected uniques visible to original clients/mBot.
- **Why real:** authentication proves the packet came through the registered Filter session; it does not prove that the player is authorized to spawn server objects or constrain frequency.
- **Minimal compatible fix:** keep opcode/layout; validate an existing server-side GM/event entitlement, allowed world/region, and a conservative per-character plus global token bucket before `CreateMob`. Reject to no-op and record telemetry.
- **Regression risk:** medium; event tooling that relies on this path must supply the same existing entitlement/state.
- **Required test:** normal/GM accounts, five valid types, type 5/255, 1/10/1,000 requests per second, teleport during request, disconnect, and multi-client aggregate cap.

### GS-H-02 — Live DPS batching can monopolize one native queue tick

- **Severity:** High / Active
- **File / function / lines:** `Objects/GObjMob.cpp`, `FlushLiveDpsBatch`, lines 155–195; `App/src/MainProcess.cpp`, `_OnQueueTimer`, lines 19–40.
- **Current behavior:** every second up to 256 pending unique mob IDs are removed, resolved, each complete damage map copied and sorted, and a packet emitted. All work runs before the original native queue timer.
- **Execution scenario:** custom unique event creates many damage-tracked mobs with large attacker maps; 256 become pending in the same interval.
- **Expected symptom:** queue-tick spike, global movement/combat delay, client timeout/DC burst, and cascading backlog.
- **Why real:** explicit batch cap bounds memory but still permits `O(256 × A log A)` work and 256 sends in a single owner-thread slice.
- **Minimal compatible fix:** preserve `0x5010`; impose a time budget and smaller configurable mob/record budget per native tick, retaining unprocessed IDs for later ticks. Compute top eight with bounded selection rather than sorting every record.
- **Regression risk:** low/medium; DPS UI refresh becomes slightly delayed under overload but packet layout remains identical.
- **Required test:** 1/64/256/1,000 pending mobs, 8/100/1,000 attackers each, queue p95/p99 timing, disconnect/despawn during backlog, and client display correctness.

## 6. Medium issues

### GS-M-01 — Telemetry performs synchronous filesystem work on the native queue thread

- **File / function / lines:** `KMTGuardCustom/GameServerTelemetry.cpp`, `TryWriteSnapshot` 315 onward and `ScopedQueueTick::~ScopedQueueTick` 612–615.
- **Scenario/symptom:** once per minute, slow/unavailable storage or antivirus scanning delays stat/rotation/write; the queue thread stalls, producing a periodic lag spike.
- **Fix:** format into a fixed in-memory snapshot and enqueue it to a bounded telemetry-only writer, or use a pre-opened buffered handle with strict drop-on-failure behavior. Never touch native objects from that writer.
- **Regression risk/test:** low; test read-only directory, disk-full, locked log, 10 MB rotation, and queue p99.

### GS-M-02 — Process-lifetime DLL contract is not enforced

- **File / function / lines:** `OutPut/src/DllMain.cpp`, lines 208–237.
- **Scenario/symptom:** loader hot-unloads the DLL while detached initialization or installed callbacks remain; execution enters unmapped DLL code and crashes. Normal process shutdown is not the same scenario.
- **Fix:** explicitly pin the module or require/verify process-lifetime loading; retain an owned init handle/state for diagnostics. Do not attempt complex unhooking under loader lock.
- **Regression risk/test:** low for supported loaders; test early/late injection, attempted `FreeLibrary`, init failure, and service shutdown.

### GS-M-03 — Item-link bridge lacks a local request budget

- **File / function / lines:** `KMTGuardCustom/CGObjPCCustom.cpp`, `ReaderPacket`, lines 742–777.
- **Scenario/symptom:** repeated `0x705D` causes bounded strings to be serialized and forwarded to ShardManager on every request; packet spam increases main-thread and inter-service load and may cause disconnects.
- **Fix:** retain fields/opcodes; enforce legal chat type/slot and existing chat cadence per player, with a small burst budget.
- **Regression risk/test:** medium; test every chat type, mBot item links, normal burst chat, malformed/truncated strings, and sustained spam.

### GS-M-04 — Authentication initialization flag has a lockless read

- **File / function / lines:** `KMTGuardCustom/InternalPacketAuth.cpp`, `ValidateRegistration` lines 193–215 versus `Initialize`/`Shutdown` lines 172–190.
- **Scenario/symptom:** registration overlaps rollback/shutdown/reinitialize; unsynchronized C++ access to `s_initialized` races the locked writer, potentially returning inconsistent auth status.
- **Fix:** read state and secret only under `s_authLock`, or publish state with `Interlocked` while retaining the secret lock.
- **Regression risk/test:** low; loop registrations while repeatedly initializing/shutting down in a harness and run Thread Sanitizer-equivalent instrumentation.

### GS-M-05 — One connection and one active-statement cancellation slot couple SQL operations

- **File / function / lines:** `SqlConnection/sqlCon.cpp`, connection lock/active handle lines 20–117, worker 145–173, shutdown 293–333; `DbConnection.cpp` query timeout 105–130.
- **Scenario/symptom:** a refresh query holds the connection lock for up to 30 seconds. Other custom SQL waits; shutdown can cancel the registered statement, but only one statement can be represented. Current active callers are mainly startup/worker, narrowing impact.
- **Fix:** keep snapshot SQL on its worker but use a dedicated connection/active handle for that worker; preserve atomic cache swap and last-known-good behavior.
- **Regression risk/test:** medium; inject 30 s SQL delay, deadlock, connection loss, simultaneous shutdown, and verify no stale-handle use.

## 7. Low issues

### GS-L-01 — Malformed packet containment has no local repeat-offender action
The packet wrappers correctly validate base buffer state, catch read exceptions, flush remaining bytes, and record telemetry. A sustained stream can still consume exception/log CPU. Prefer a bounded per-session malformed counter handed to the existing Filter/disconnect policy; do not invent a new packet rule.

### GS-L-02 — Empty exception handler hides region-notification failures
`CGObjEvents::CheckRegionNeedChange` catches everything without telemetry. Keep gameplay fail-open, but increment the existing runtime-error counter so missing `0x3571` can be diagnosed.

### GS-L-03 — Diagnostic counters are 32-bit
Interlocked `LONG` counters reset each minute and are adequate normally. Extreme packet rates can wrap, producing false telemetry. Saturating increments or 64-bit aligned counters are hardening only.

### GS-L-04 — Archived timed-item worker is unsafe if re-enabled
`CustomTimedJobManager.cpp` sleeps without a stop event, touches `g_pCGame`, `CGObjPC`, inventory, packets, and SQL from a worker, and erases map entries before further iterator dereferences. `CustomTimedJobManager.h` has a compile-time error preventing activation; preserve that guard until the feature is redesigned around owner-thread commands.

## 8. Existing good protections

- Exact executable name, base, PE timestamp, image size, hook-byte signatures, and pointer owners are verified before hooks publish.
- Pointer-hook installation rolls back prior hooks on the first failure; initialization terminates fail-closed.
- Patch writes restore page protection, flush instruction cache, verify bytes, and support rollback.
- Client and server packet wrappers verify base cursor/buffer invariants and contain parsing exceptions.
- Sensitive custom client commands require a per-session HMAC-authenticated Filter registration; replay cache is bounded to 8,192 entries and entries expire.
- Most strings use explicit maxima; the custom message wrapper checks remaining bytes before reads.
- ODBC has a 10-second login timeout and 30-second statement timeout; statement handles are released on normal failure paths.
- The SQL worker accesses database/cache state only, never live `CGObjPC` or world objects; stop event, cancel, join, and resource-retention-on-timeout avoid shutdown UAF.
- Security snapshot readers/writers use locks and publish last-known-good collections rather than partial results.
- Live DPS pending IDs are deduplicated, bounded per batch, and resolved back to current native objects instead of caching object pointers.
- Player deletion clears Filter session, travel, and restriction state.
- Aggro restriction scanning is throttled and its auxiliary cache has TTL cleanup.
- The unsafe new-alchemy opcode is explicitly consumed and retired; the timed worker is compile-time disabled.

## 9. Normal vSRO behavior that should not be changed

- Fixed addresses, x86 naked hooks, `thiscall`/`fastcall`, native vtables, layout padding, and raw owner pointers are required for this exact fingerprint.
- Mutating world/player/inventory state on the native GameServer message/queue thread is correct; moving it to generic workers would be unsafe.
- `CMsg`, native allocation/free/send ownership, ShardManager `0x8888`, and original client opcode layouts must remain byte-compatible.
- Synchronous SQL during controlled add-on startup is acceptable because READY is not published until snapshots exist. The defect boundary is database work reachable from live gameplay hot paths.
- `NOLOCK` on the read-only binding lookup is a legacy consistency choice, not itself a crash bug.
- Process-exit teardown may rely on OS reclamation; unsupported hot unload is the distinct issue.
- Native maps/lists exposed in reconstructed classes are not automatically data races when used only on the owner thread.

## 10. SQL / ODBC disposition

| Category | Functions | Verdict |
|---|---|---|
| Startup configuration/security loads | `CSqlCon::Initialize`, `Load*` | acceptable synchronous startup; timed and fail-closed |
| Periodic security snapshots | `SecuritySnapshotRefreshWorker` | correct worker ownership; shared-connection coupling GS-M-05 |
| Live non-query helper | `TryExecNonQuery` | timed/serialized; active gameplay callers not found after new alchemy retirement |
| Binding option lookup | `GetItemBindingOpt` | synchronous and unsuitable for packets, but current calls are inside retired new-alchemy implementation |
| Timed plus loaders/writes | `TimedPlusItems`, `TimedDevillPlusItems`, archived manager | dormant; do not enable |
| SQL construction | numeric IDs and fixed queries; escaped setting writes | no active injection proven; parameterization remains preferable hardening |
| Handles | `CDbConnection`, `ScopedSqlStatement` | login/query timeout and cleanup are good; no active statement leak proven |

## 11. Packet compatibility and mBot/client assessment

- Original client packet dispatch is preserved after KMT inspection; custom-consumed opcodes are private extensions.
- Item-link strings have 64/1,024 limits; base packet underflow/overflow raises into containment.
- No packet layout or timing change is recommended. Repairs should only reject unauthorized/rate-excess requests.
- mBot uses original gameplay packets and is not singled out by GameServer source. **No proven GameServer-side mBot incompatibility was found.** Item-link and custom UI functions need staging with the exact bot because external behavior is unavailable here.
- ShardManager commands are trusted service traffic; recommendations do not add fields or alter `0x8888`.

## 12. Memory-leak and object-lifetime assessment

No active unbounded allocation leak was proven. Native message buffers are generally transferred to native send ownership or explicitly freed. Live DPS/auth/restriction/session caches are deduplicated, expired, capped, or cleared on deletion. ODBC environment, connection, statement, worker, and event handles have explicit release paths. Remaining lifetime risk is unsupported DLL hot unload (GS-M-02), not normal process shutdown.

## 13. Shutdown/restart assessment

The SQL worker uses an event, `SQLCancel`, a 35-second join, and intentionally retains synchronization/connection state if join fails—safer than freeing beneath the worker. Initialization rollback removes reversible hooks in dependency order. There is no normal `DLL_PROCESS_DETACH` shutdown path, so the effective contract is process-lifetime injection. Maintenance restart by terminating the GameServer process is safe; hot unload/reload is not supported and should be mechanically prevented/documented.

## 14. Recommended repair order

1. **GS-H-01:** server-side entitlement and rate/global spawn budgets.
2. **GS-H-02:** time-sliced DPS backlog and bounded top-eight selection.
3. **GS-M-01:** remove filesystem I/O from the native queue tick.
4. **GS-M-03:** item-link validation and cadence budget.
5. **GS-M-02:** pin/document process-lifetime DLL and own init state.
6. **GS-M-04/M-05:** auth publication synchronization and dedicated snapshot SQL connection.
7. Add malformed-packet offender telemetry and region-hook error telemetry.
8. Keep archived timed workers disabled; do not repair them incidentally.

## 15. Required Windows testing plan

1. Build Win32 Release with the supported MSVC toolchain and exact `SR_GameServer.exe` fingerprint.
2. Hook manifest: exact host, every altered byte/pointer, foreign owner first/middle/last, rollback failure, and no READY on partial publication.
3. Packets: every custom opcode with zero/truncated/exact/trailing payload, max strings, invalid enums/IDs/slots, replayed auth registration, and 1/10/1,000 packets/s.
4. Spawn control: normal/GM/event users, all `0x3538` values, forbidden regions/worlds, multi-client global flood, despawn/restart recovery.
5. DPS: 1–1,000 mobs and 8–1,000 attackers; record queue/game-loop p50/p95/p99 and client update latency.
6. SQL: 30 s delay, deadlock, login failure, disconnect/reconnect, cancellation during refresh, shutdown at every ODBC phase, last-known-good snapshot validation.
7. Filesystem: telemetry directory missing/read-only/full/locked, antivirus delay, rotation at 10 MB; prove no queue hitch.
8. Lifecycle: early/late injection, failed initialization, attempted hot unload, normal console shutdown, maintenance restart, 100 restart cycles, handle-count comparison.
9. Objects: disconnect/teleport/despawn during item link, DPS, region change, party/trade, pet and fortress events; enable page heap/Application Verifier.
10. Compatibility: original client plus exact supported Filter, ShardManager, Gateway, databases, and mBot; packet capture must show unchanged layouts/opcodes.

---

## Final audit status

- Files inspected: 127 `SR_GameServer` C/C++ units, 53 implementations; shared reached dependencies included.
- Critical: **0**
- High: **2**
- Medium: **5**
- Low: **4**
- Primary crash/lag risks: unique-spawn flood, DPS queue-tick monopolization, synchronous telemetry disk stall, unsupported hot unload, and item-link amplification.
- Overall: **REQUIRES TARGETED REPAIR AND WINDOWS STAGING**
