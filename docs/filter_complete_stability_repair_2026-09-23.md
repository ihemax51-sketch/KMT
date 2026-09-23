# KMTGuard Filter Complete Stability Repair — 2026-09-23

## Summary

This change hardens the current .NET 8 Filter without changing its Agent,
Gateway, or Download proxy roles, vSRO packet layouts, opcodes, encryption, or
mBot-facing gameplay traffic. The result requires a Windows build and staging
run before deployment; it is not claimed production-ready from source checks.

## Changed files

- `SilkroadSecurityAPI/Packet.cs`: synchronized, idempotent finalization and
  independent read-only session clones.
- `ServerManagers/ServerManager.cs` and `Server/AgentServer/AgentServer.cs`:
  immutable-per-recipient broadcasts and awaited service shutdown.
- `Session/Session.cs`: conservative server-direction size/rate/custom budgets,
  repeat-offender diagnostics, and malformed packet accounting.
- `PacketHandler/PacketHandler.cs`: centralized malformed-handler telemetry.
- `Helpers/DatabaseJobQueue.cs` and `Database/SqlExecutionPolicy.cs`: bounded
  packet-critical waits plus queue depth, age, duration, completion, and drop
  metrics.
- `RuntimeContract/RuntimeContracts.cs` and `Runtime/RuntimeControlServer.cs`:
  timestamped nonce/HMAC requests, bounded input, connection caps, replay
  rejection, timeouts, and deterministic handler draining.
- `Session/SessionData.cs`, `Session/ISessionData.cs`, and
  `Helpers/ConcurrentList.cs`: concurrent collections for shared mutable state.
- `Server/QuickLoginAgentAuthBridge.cs`, `Startup.cs`, and Agent auth handling:
  encrypted, expiring one-time credential state with IP/device binding and
  immediate cleanup after consumption.
- Scheduler, Auto Events, Telegram, Discord, delayed jobs, and teleport-freeze
  services: owned cancellation and awaited `StopAsync` completion.
- Auto Event service and the v7.0.1 database migration: atomic winner/outbox
  creation and unique reward delivery records.
- Packet pipeline tests: 1,000-recipient packet-finalization stress and runtime
  request authentication tamper coverage.

## Fixed

### FLT-H-01 — shared writable broadcasts

`Packet.ToReadOnly` is synchronized and repeatable. Broadcast entry points
finalize once and give every target an independently owned byte array and read
cursor. A delayed sender can no longer share mutable writer or reader state
with another session.

### FLT-H-02 — server-to-client protection

Server traffic now has packet-size, massive-packet, byte-rate, packet-rate, and
custom-rate controls. Character-load and massive snapshots receive explicit
headroom; Download receives a separate patch-traffic budget. Rejections log
opcode, size, session, service, reason, and repeated count. No native gameplay
opcode is rewritten or filtered by identity.

### FLT-H-03 — database queue resilience

The bounded queue now exposes depth, queue age, execution duration, completed
and dropped jobs. Packet-critical callers have an eight-second upper wait and
receive a timeout rather than waiting forever. Background producers remain
nonblocking and drop safely at capacity. New SQL code uses the centralized
authentication, packet-critical, queue, event, and maintenance timeout policy.

## Lifecycle repairs

Scheduler workers are owned and drained. Auto Events, Telegram, Discord,
Delayed Jobs, Teleport Freeze, runtime pipe handlers, and the database queue
retain task/CTS ownership until completion. `ServerManager.DisposeAsync` and
the top-level cleanup path await those stops in dependency order.

## Auto Event reward reliability

The winner row and one outbox row per configured first-place reward are created
in one SQL transaction. The `(RunID, RoundID, CharID, RewardID)` unique key is
the idempotency boundary. Delivery attempts transition `Pending` to
`Processing` and then `Completed`; confirmed validation or delivery failures
return to `Pending`. A `Processing` row after an exception or indeterminate
process failure is intentionally not blindly replayed because doing so could
duplicate an externally committed reward; it requires reconciliation during
staging/operations.

Database deployment files:

- `database/migrations/20260923_event_reward_outbox.sql`
- `database/tests/event_reward_outbox_validation.sql`

## Runtime control security

Named pipes retain `CurrentUserOnly`. Requests are capped at 64 KiB, limited to
16 concurrent handlers, cancelled after five seconds while reading, signed by
a per-user local HMAC key, timestamp-limited to 60 seconds, and protected by a
server nonce cache. Admin Desktop and split Filter clients use the same updated
runtime contract.

## Session and quick-login safety

Shared title, icon, achievement, reverse-location, and pet collections now use
concurrent dictionaries or snapshot enumeration. Quick-login bridge entries
hold AES-GCM ciphertext rather than passwords, expire after 45 seconds, are
cleaned every 15 seconds, bind to IP and available device thumbprint, and are
zeroed after removal. The unavoidable plaintext exists only while constructing
the native `0x6103` packet and is not retained in Agent session state.

## Compatibility

- No opcode changed.
- No vSRO packet field, ordering, encoding, encryption, or massive framing
  changed.
- Agent/Gateway/Download separation remains intact.
- Movement, combat, skill, inventory, exchange, stall, guild, party, character
  loading, patch/download, and mBot-compatible traffic are not rewritten.
- SQL and notification workers never access native game objects; this managed
  Filter has no GameServer native ownership change.

## Partially fixed / remaining risks

- The repository contains extensive legacy SQL call sites. The central policy,
  bounded queue, and live wait ceiling prevent indefinite forwarding stalls,
  but a future mechanical migration should pass cancellation and explicit
  timeout values through every legacy direct query.
- Reward rows left `Processing` by a crash are deliberately quarantined rather
  than automatically replayed. Operational reconciliation is required because
  the existing item-chest/database procedures do not expose a common external
  idempotency key.
- Runtime HMAC key-file ACL behavior and all service shutdown paths require
  Windows validation.

## Required Windows staging

1. Build all Filter and Admin Desktop projects together on Windows.
2. Run packet pipeline tests, including the 1,000-clone staggered broadcast.
3. Load-test server traffic during character loading, inventory/storage,
   guild/party/stall bursts, Download patching, and mBot login/gameplay.
4. Delay SQL for 30 seconds and verify forwarding wait is bounded, queue health
   is visible, background overload drops safely, and shutdown cancels/drains.
5. Stop during Scheduler, Auto Event, Telegram, Discord, delayed teleport, and
   runtime-control work; verify no task survives process cleanup.
6. Test runtime requests for oversized input, 17 concurrent clients, timeout,
   stale timestamp, modified payload, and replayed nonce from Admin Desktop.
7. Apply and validate the v7.0.1 SQL migration; inject reward failures before
   and after delivery and reconcile ambiguous `Processing` rows.
8. Test quick login success, expiry, replay, changed IP/device, split roles, and
   memory-dump inspection after cleanup.

## Final status

**REQUIRES WINDOWS VERIFICATION**
