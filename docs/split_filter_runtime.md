# Split Filter Runtime

KMTGuard runs the filter as three isolated Windows processes:

| Process | Responsibility |
| --- | --- |
| `KMTGuard.Gateway.exe` | Gateway proxy, login protection, client settings and redirects |
| `KMTGuard.Download.exe` | Patch and download proxy only |
| `KMTGuard.Agent.exe` | Agent proxy, gameplay systems, schedulers, live sessions and clientless accounts |

`KMTGuard.exe` is the desktop control plane. It does not load the filter core into
its own process.

## Lifecycle

The desktop starts services in dependency order:

1. Agent
2. Download
3. Gateway

It stops them in reverse order. Each service can also be started, stopped or
restarted independently. A process is reported as ready only after its configured
TCP listeners are bound and its local control pipe responds.

Closing the desktop does not terminate running services. Use **Stop All** when the
filter itself must be shut down.

## Isolation And IPC

Each service owns only sessions and background work for its role. The Agent
process is the single owner of gameplay state.

Local named pipes provide the small cross-process contract needed for:

- Runtime health and memory snapshots.
- Live online-player snapshots.
- Clientless start, reload, stop and status commands.
- Short-lived secure quick-login handoff from Gateway to Agent.
- Offline Stall login coordination between Gateway and Agent.

Pipes use Windows current-user isolation and are not exposed on a TCP port.

## Memory Controls

- Download skips server settings, game reference caches and all gameplay timers.
- Gateway loads only notices and the Item Mall references used during login.
- Agent alone loads full game references, scheduled jobs and gameplay services.
- Gateway and Download use zero SQL minimum-pool connections and bounded maximum
  pools appropriate to their workload.
- Workstation GC and memory-conservation settings are applied to the workers.
- Expired IP rate-limit entries are removed periodically.

## Files And Logs

The deployment folder contains:

```text
KMTGuard.exe
KMTGuard.Agent.exe
KMTGuard.Download.exe
KMTGuard.Gateway.exe
Settings.json
```

All processes read the same `Settings.json`. Logs are separated by role:

```text
logs/kmtguard-agent-YYYYMMDD.log
logs/kmtguard-download-YYYYMMDD.log
logs/kmtguard-gateway-YYYYMMDD.log
```

Proxy endpoints remain configured in `[KMT].[System_ProxyServices]`. Multiple
rows of one type, including multiple AgentServer endpoints, are hosted by the
matching role process.

The desktop and three memory-optimized workers target the installed .NET 8 x64
Desktop Runtime. Windows can therefore share runtime image pages between all four
processes instead of loading private runtime copies. Install the .NET 8 Desktop
Runtime x64 before deploying KMTGuard on a new machine.
