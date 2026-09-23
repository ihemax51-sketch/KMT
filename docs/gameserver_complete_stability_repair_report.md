# KMTGuard vSRO 188 — GameServer complete stability repair report

**Repair date:** 2026-09-23
**Scope:** `gameserver/` SR_GameServer extension only
**Basis:** `docs/gameserver_deep_audit_2026-09-23.md`

## Summary

This change implements the requested targeted stability architecture while retaining the vSRO 188 native owner thread, fixed-address ABI, existing client/Filter/ShardManager routes, all opcodes, and all packet layouts. Item Linking (`0x705D`) is deliberately unchanged. No player, mob, inventory, or world object is accessed by either new worker.

The code is ready for the supported Windows build and runtime staging cycle, but it is not claimed production-ready because the Linux environment cannot build or run the x86 Windows add-on.

## Changed files

- `CHANGELOG.md`
- `gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/CMakeLists.txt`
- `gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/GameServer/src/KMTGuardCustom/CGObjPCCustom.cpp`
- `gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/GameServer/src/KMTGuardCustom/GameServerTelemetry.cpp`
- `gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/GameServer/src/KMTGuardCustom/GameServerTelemetry.h`
- `gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/GameServer/src/KMTGuardCustom/InternalPacketAuth.cpp`
- `gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/GameServer/src/KMTGuardCustom/UniqueSpawnGuard.cpp`
- `gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/GameServer/src/KMTGuardCustom/UniqueSpawnGuard.h`
- `gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/GameServer/src/Objects/GObjMob.cpp`
- `gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/GameServer/src/SqlConnection/sqlCon.cpp`
- `gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/GameServer/GameServer/src/SqlConnection/sqlCon.h`
- `gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/OutPut/src/DllMain.cpp`
- `gameserver/source/SilkroadOnline/SR_VIETNAM_SERVICE/v188/Server/SR_GameServer/OutPut/src/Util.cpp`
- `gameserver/tests/ValidateGameServerStabilityRepair.py`
- `docs/gameserver_complete_stability_repair_report.md`

## Repaired findings

### GS-H-01 — Unique spawn authorization

`0x3538` still reads the same Filter key and one-byte unique type. Before any native spawn, the owner thread now checks:

- type range 0–4;
- character DB-ID membership in a server-local event authorization list;
- exact allowed world and region lists;
- per-character cooldown;
- replay window keyed by session credential, character, and type;
- a serialized global token bucket;
- one locked validation/consumption transaction, preventing simultaneous over-commit.

Rejected requests no-op and increment telemetry. Configuration is in `KMTGuard-Addon.ini`:

```ini
[UniqueSpawn]
AuthorizedCharacterIds=123,456
AllowedWorlds=1
AllowedRegions=25000,25001
CharacterCooldownMs=10000
ReplayWindowMs=30000
GlobalBurst=5
GlobalPerMinute=5
```

Empty authorization/location lists fail closed. Existing event tools remain packet-compatible but must use an authorized event character in an allowed location.

### GS-H-02 — Time-sliced live DPS

The queue retains unprocessed mob IDs across ticks. Each flush observes configurable `KMT_DPS_MAX_MOBS_PER_TICK` (default 16, maximum 256) and `KMT_DPS_BUDGET_US` (default 2,000 µs, maximum 20,000 µs). Attacker processing uses a fixed top-eight insertion set rather than copying and fully sorting every attacker record. `0x5010` fields and native-thread object resolution are unchanged.

### GS-M-01 — Asynchronous telemetry storage

The native queue thread still creates a plain character-buffer snapshot, but only enqueues it into a fixed 64-entry ring. A dedicated writer performs rotation, open, write, and close operations. The producer never waits for disk and drops records with a counter when overloaded. The worker receives strings only and cannot access GameServer objects. Shutdown joins it and retains synchronization state rather than freeing beneath a stuck writer.

### GS-M-02 — Process-lifetime DLL

Initialization takes a permanent module reference before publishing hooks. Unsupported `FreeLibrary` calls therefore cannot unmap hook code. Existing fail-closed initialization and pre-publication rollback remain intact; no unhooking was added under loader lock.

### GS-M-04 — Authentication synchronization

Initialization state is now sampled under the same critical section used by initialization, shutdown, secret access, and replay publication. Registration no longer performs a C++ lockless read racing lifecycle writers.

### GS-M-05 — SQL refresh isolation

The security snapshot worker owns a dedicated ODBC connection and separate active-statement cancellation slot. Its locked-item and fortress-DPS queries do not acquire the general custom SQL connection lock. Cache publication still swaps complete staged snapshots under cache locks; failures retain last-known-good data. Shutdown independently cancels both statement owners and joins the refresh worker before releasing state.

### Malformed packet abuse diagnostics

Malformed client input is counted per live Game ID in a bounded 8,192-entry table in addition to the aggregate counter. This is observability for the existing Filter policy, not a new disconnect protocol. No packet or enforcement behavior changed.

## Compatibility confirmations

- **Opcode changes:** none.
- **Packet layout changes:** none.
- **ShardManager `0x8888`:** unchanged.
- **Item Linking `0x705D`:** unchanged.
- **Native ownership:** unchanged; spawn and DPS object work remain on the GameServer thread.
- **Telemetry worker:** handles immutable text snapshots and filesystem I/O only.
- **SQL worker:** handles its dedicated ODBC connection and synchronized cache snapshots only.
- **mBot/original client:** no original gameplay opcode or layout was changed.

## Automated validation

`gameserver/tests/ValidateGameServerStabilityRepair.py` checks entitlement/location gates, cooldown/token/replay architecture, DPS time slicing and continuation, bounded top-N selection, telemetry queue/worker isolation, synchronized authentication, independent SQL connection/statement ownership, module pinning, malformed-session tracking, and preservation of Item Linking.

CMake generation cannot complete on this Linux host because the project requires DirectX and the supported Windows toolchain. No binary was produced or deployed.

## Remaining risks

- Exact vSRO runtime behavior, VC toolchain compatibility, hook ownership, and real queue latency require Windows verification.
- Operators must deliberately configure event identities and allowed locations; the safe default denies unique spawning.
- The 5-second telemetry shutdown bound is intentionally fail-safe: a stuck disk worker causes synchronization objects to be retained until process exit.
- Per-session malformed counters are diagnostic only. Disconnect/throttle action remains with the existing Filter policy.
- GS-M-03 remains explicitly unfixed by request.

## Windows staging requirements

1. Build Win32 Release with the supported toolchain using `build-scripts\Build-Component.ps1 -Component GameServer`.
2. Unique spawn: normal account, authorized event account, all types, invalid type, forbidden world/region, duplicate replay, teleport, concurrent requests, and 1/10/1,000 request floods.
3. DPS: 1, 100, and 1,000 pending mobs; 8, 100, and 1,000 attackers; record queue p50/p95/p99 and verify unchanged `0x5010` captures.
4. SQL: 30-second refresh delay, deadlock, connection loss, and shutdown during each query; verify unrelated custom SQL proceeds and last-known-good snapshots remain.
5. Telemetry: missing/locked/read-only directory and full disk; verify queue latency and dropped-record counter.
6. Authentication: concurrent initialize/shutdown/register harness and replay validation.
7. Lifecycle: exact-host injection, partial-init rollback, attempted `FreeLibrary`, normal restart, and 100 maintenance restarts.
8. Compatibility: original client, current Filter, ShardManager, Gateway, database, and supported mBot; compare packet captures.

## Deployment

After successful staging, copy the GameServer add-on output to `D:\KMTGuard-build\ServerAddons\GameServer`, replace it during maintenance, configure `[UniqueSpawn]`, and restart GameServer. No SQL migration is required.

## Final status

**REQUIRES WINDOWS VERIFICATION**
