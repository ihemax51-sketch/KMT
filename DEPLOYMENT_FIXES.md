# Deployment notes

The filter, client library, and GameServer must be rebuilt and deployed
together. Their custom packet and HWID handshakes were intentionally changed
as one compatible protocol update.

1. Back up the filter database.
2. Run `database/migrations/20260622_secondary_password_security.sql`.
3. Run `database/migrations/20260622_self_teleport.sql`.
4. Rebuild and deploy the modified GameServer.
5. Rebuild the client library and update the client.
6. Publish and deploy `filter/KMTGuardnew/KMTGuard`.
7. Keep AgentServer/GameServer ports private; players should only reach the
   filter bind ports.
8. Ensure opcodes `0x35FE` and `0x3539` are not present in any client
   whitelist. They are internal filter-to-GameServer packets.
9. Scheduler rows must call one stored procedure using a single `EXEC`
   statement. Free-form SQL batches are rejected.
10. Restart all components together.

The SQL password storage and existing password logging behavior were not
changed, as explicitly requested.

## Phase 1 runtime stability

- Former fire-and-forget gameplay database writes now pass through a bounded
  queue and the packet handler waits for completion.
- The queue uses two workers and a capacity of 512 jobs to apply backpressure
  instead of exhausting the SQL connection pool.
- Packet-handler SQL timeouts were reduced from 600 seconds to 60 seconds.
- Automatic SQL retry is limited to idempotent operations. It is intentionally
  not applied to rewards, purchases, or other writes that could be duplicated.
- Delayed gameplay packets are now awaited by the delayed-job worker.
- Pending database jobs are drained during filter shutdown.

## Self teleport

Execute the procedure in the filter database:

```sql
EXEC dbo._self_Teleport @CharID = 1234;
```

The filter consumes command ID `41` and sends protected internal opcode
`0x3539`. The GameServer reloads the online, game-ready character at the same
world, region, and coordinates. Repeated pending requests are merged and each
character is limited to one request per second.
