#include "GameServerTelemetry.h"

#include <cstdio>
#include <climits>
#include <cstring>
#include <map>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

namespace
{
    const DWORD TELEMETRY_INTERVAL_MS = 60000;
    const ULONGLONG TELEMETRY_MAX_FILE_SIZE = 10ULL * 1024ULL * 1024ULL;
    const char* TELEMETRY_FILE = "KMTGuard-GameServer-Telemetry.log";
    const char* TELEMETRY_BACKUP_FILE = "KMTGuard-GameServer-Telemetry.1.log";
    const size_t LATENCY_BUCKET_COUNT = 16;
    const size_t OPCODE_TABLE_SIZE = 1024;
    const size_t OPCODE_PROBE_LIMIT = 16;
    const size_t TOP_OPCODE_COUNT = 5;
    const size_t TELEMETRY_QUEUE_CAPACITY = 64;
    const size_t TELEMETRY_LINE_CAPACITY = 2048;

    struct TelemetryLine { char text[TELEMETRY_LINE_CAPACITY]; DWORD length; };
    TelemetryLine s_telemetryQueue[TELEMETRY_QUEUE_CAPACITY];
    CRITICAL_SECTION s_telemetryQueueLock;
    HANDLE s_telemetryWakeEvent = NULL;
    HANDLE s_telemetryStopEvent = NULL;
    HANDLE s_telemetryThread = NULL;
    // 0=stopped, 1=running, 2=stopping, 3=terminal after an unsafe-to-clean timeout,
    // 4=initializing. A terminal process-lifetime instance is never restarted.
    volatile LONG s_telemetryState = 0;
    size_t s_telemetryHead = 0, s_telemetryTail = 0, s_telemetryCount = 0;
    volatile LONG s_telemetryDropped = 0;
    volatile LONG s_uniqueSpawnRejected[7] = { 0 };
    CRITICAL_SECTION s_malformedSessionLock;
    std::map<DWORD, DWORD> s_malformedSessions;

    const LONG LATENCY_BUCKET_UPPER_US[LATENCY_BUCKET_COUNT] =
    {
        10, 25, 50, 100, 250, 500, 1000, 2500,
        5000, 10000, 25000, 50000, 100000, 250000, 500000, 1000000
    };

    struct OpcodeMetric
    {
        volatile LONG key;
        volatile LONG count;
        volatile LONG maxUs;
    };

    struct OpcodeSnapshot
    {
        LONG key;
        LONG count;
        LONG maxUs;
    };

    volatile LONG s_clientPackets = 0;
    volatile LONG s_serverPackets = 0;
    volatile LONG s_malformedPackets = 0;
    volatile LONG s_packetAuthFailures[7] = { 0 };
    volatile LONG s_runtimeErrors = 0;
    volatile LONG s_clientHandlerMaxUs = 0;
    volatile LONG s_serverHandlerMaxUs = 0;
    volatile LONG s_gameLoopGapMaxMs = 0;
    volatile LONG s_gameLoopWorkMaxUs = 0;
    volatile LONG s_queueTimerGapMaxMs = 0;
    volatile LONG s_queueTimerWorkMaxUs = 0;
    volatile LONG s_liveDpsBatches = 0;
    volatile LONG s_liveDpsPackets = 0;
    volatile LONG s_fortressDpsSnapshots = 0;
    volatile LONG s_freeForAllEnabled = 0;
    volatile LONG s_databaseQueries = 0;
    volatile LONG s_databaseFailures = 0;
    volatile LONG s_databaseTotalMs = 0;
    volatile LONG s_databaseMaxMs = 0;
    volatile LONG s_securitySnapshotDatabaseOk = 0;
    volatile LONG s_securitySnapshotLastSuccessTick = 0;
    volatile LONG s_lastClientOpcode = 0;
    volatile LONG s_lastServerOpcode = 0;

    volatile LONG s_clientLatencyBuckets[LATENCY_BUCKET_COUNT] = { 0 };
    volatile LONG s_serverLatencyBuckets[LATENCY_BUCKET_COUNT] = { 0 };
    volatile LONG s_gameLoopLatencyBuckets[LATENCY_BUCKET_COUNT] = { 0 };
    OpcodeMetric s_opcodeMetrics[OPCODE_TABLE_SIZE] = { 0 };

    volatile LONG s_sessionKeyCacheSize = 0;
    volatile LONG s_uniqueTypeCacheSize = 0;
    volatile LONG s_liveDpsPendingCacheSize = 0;
    volatile LONG s_liveDpsPendingCachePeak = 0;
    volatile LONG s_lockedItemCacheSize = 0;

    volatile LONG s_lastGameLoopTick = 0;
    volatile LONG s_lastQueueTimerTick = 0;
    volatile LONG s_lastSnapshotTick = 0;
    LARGE_INTEGER s_performanceFrequency = { 0 };
    ULONGLONG s_lastProcessCpu100ns = 0;
    DWORD s_processorCount = 1;

    struct TelemetryLocks
    {
        TelemetryLocks() { InitializeCriticalSection(&s_telemetryQueueLock); InitializeCriticalSection(&s_malformedSessionLock); }
        ~TelemetryLocks()
        {
            if (s_telemetryThread != NULL) return;
            DeleteCriticalSection(&s_malformedSessionLock); DeleteCriticalSection(&s_telemetryQueueLock);
        }
    } s_telemetryLocks;

    LONG ClampSize(size_t value)
    {
        return value > static_cast<size_t>(LONG_MAX)
            ? LONG_MAX
            : static_cast<LONG>(value);
    }

    void UpdateMaximum(volatile LONG* target, LONG value)
    {
        LONG current = InterlockedCompareExchange(target, 0, 0);
        while (value > current)
        {
            const LONG observed = InterlockedCompareExchange(target, value, current);
            if (observed == current)
                break;
            current = observed;
        }
    }

    LONG ElapsedMicroseconds(const LARGE_INTEGER& started)
    {
        LARGE_INTEGER finished;
        QueryPerformanceCounter(&finished);
        if (s_performanceFrequency.QuadPart <= 0 || finished.QuadPart <= started.QuadPart)
            return 0;

        const LONGLONG ticks = finished.QuadPart - started.QuadPart;
        const LONGLONG microseconds = (ticks * 1000000LL) / s_performanceFrequency.QuadPart;
        return microseconds > LONG_MAX ? LONG_MAX : static_cast<LONG>(microseconds);
    }

    size_t GetLatencyBucket(LONG elapsedUs)
    {
        for (size_t i = 0; i + 1 < LATENCY_BUCKET_COUNT; ++i)
        {
            if (elapsedUs <= LATENCY_BUCKET_UPPER_US[i])
                return i;
        }
        return LATENCY_BUCKET_COUNT - 1;
    }

    void RecordLatency(volatile LONG* buckets, LONG elapsedUs)
    {
        InterlockedIncrement(&buckets[GetLatencyBucket(elapsedUs)]);
    }

    LONG PercentileFromSnapshot(const LONG* values, LONGLONG total, unsigned int percentile)
    {
        if (total <= 0)
            return 0;

        const LONGLONG target = (total * percentile + 99) / 100;
        LONGLONG seen = 0;
        for (size_t i = 0; i < LATENCY_BUCKET_COUNT; ++i)
        {
            seen += values[i];
            if (seen >= target)
                return LATENCY_BUCKET_UPPER_US[i];
        }
        return LATENCY_BUCKET_UPPER_US[LATENCY_BUCKET_COUNT - 1];
    }

    void SnapshotPercentiles(
        volatile LONG* buckets,
        LONG& p50,
        LONG& p95,
        LONG& p99)
    {
        LONG values[LATENCY_BUCKET_COUNT] = { 0 };
        LONGLONG total = 0;
        for (size_t i = 0; i < LATENCY_BUCKET_COUNT; ++i)
        {
            values[i] = InterlockedExchange(&buckets[i], 0);
            total += values[i];
        }

        p50 = PercentileFromSnapshot(values, total, 50);
        p95 = PercentileFromSnapshot(values, total, 95);
        p99 = PercentileFromSnapshot(values, total, 99);
    }

    OpcodeMetric* FindOpcodeMetric(bool clientPacket, WORD opcode)
    {
        const LONG key = static_cast<LONG>(opcode) | (clientPacket ? 0x10000 : 0x20000);
        size_t index = (static_cast<unsigned int>(key) * 2654435761u) & (OPCODE_TABLE_SIZE - 1);
        for (size_t probe = 0; probe < OPCODE_PROBE_LIMIT; ++probe)
        {
            OpcodeMetric* metric = &s_opcodeMetrics[(index + probe) & (OPCODE_TABLE_SIZE - 1)];
            LONG currentKey = InterlockedCompareExchange(&metric->key, 0, 0);
            if (currentKey == key)
                return metric;
            if (currentKey == 0 && InterlockedCompareExchange(&metric->key, key, 0) == 0)
                return metric;
        }
        return NULL;
    }

    void RecordOpcode(bool clientPacket, WORD opcode, LONG elapsedUs)
    {
        OpcodeMetric* metric = FindOpcodeMetric(clientPacket, opcode);
        if (metric == NULL)
            return;
        InterlockedIncrement(&metric->count);
        UpdateMaximum(&metric->maxUs, elapsedUs);
    }

    void InsertTopOpcode(OpcodeSnapshot* top, const OpcodeSnapshot& candidate)
    {
        for (size_t i = 0; i < TOP_OPCODE_COUNT; ++i)
        {
            if (candidate.count > top[i].count ||
                (candidate.count == top[i].count && candidate.maxUs > top[i].maxUs))
            {
                for (size_t move = TOP_OPCODE_COUNT - 1; move > i; --move)
                    top[move] = top[move - 1];
                top[i] = candidate;
                return;
            }
        }
    }

    void SnapshotTopOpcodes(bool clientPacket, OpcodeSnapshot* top)
    {
        std::memset(top, 0, sizeof(OpcodeSnapshot) * TOP_OPCODE_COUNT);
        const LONG direction = clientPacket ? 0x10000 : 0x20000;
        for (size_t i = 0; i < OPCODE_TABLE_SIZE; ++i)
        {
            const LONG key = InterlockedCompareExchange(&s_opcodeMetrics[i].key, 0, 0);
            if ((key & 0x30000) != direction)
                continue;

            const LONG count = InterlockedExchange(&s_opcodeMetrics[i].count, 0);
            const LONG maxUs = InterlockedExchange(&s_opcodeMetrics[i].maxUs, 0);
            if (count <= 0)
                continue;

            OpcodeSnapshot candidate = { key, count, maxUs };
            InsertTopOpcode(top, candidate);
        }
    }

    void FormatTopOpcodes(char* output, size_t outputSize, const OpcodeSnapshot* top)
    {
        if (output == NULL || outputSize == 0)
            return;
        output[0] = '\0';

        size_t used = 0;
        for (size_t i = 0; i < TOP_OPCODE_COUNT && top[i].count > 0; ++i)
        {
            const WORD opcode = static_cast<WORD>(top[i].key & 0xFFFF);
            const int written = _snprintf_s(
                output + used,
                outputSize - used,
                _TRUNCATE,
                "%s0x%04X:%ld/%ldus",
                used == 0 ? "" : ",",
                opcode,
                top[i].count,
                top[i].maxUs);
            if (written <= 0)
                break;
            used += static_cast<size_t>(written);
            if (used >= outputSize)
                break;
        }

        if (used == 0)
            _snprintf_s(output, outputSize, _TRUNCATE, "none");
    }

    ULONGLONG FileTimeToUInt64(const FILETIME& value)
    {
        ULARGE_INTEGER result;
        result.LowPart = value.dwLowDateTime;
        result.HighPart = value.dwHighDateTime;
        return result.QuadPart;
    }

    double GetProcessCpuPercent(DWORD elapsedMs)
    {
        FILETIME creation = { 0 };
        FILETIME exitTime = { 0 };
        FILETIME kernel = { 0 };
        FILETIME user = { 0 };
        if (!GetProcessTimes(GetCurrentProcess(), &creation, &exitTime, &kernel, &user) || elapsedMs == 0)
            return 0.0;

        const ULONGLONG current = FileTimeToUInt64(kernel) + FileTimeToUInt64(user);
        const ULONGLONG previous = s_lastProcessCpu100ns;
        s_lastProcessCpu100ns = current;
        if (previous == 0 || current < previous)
            return 0.0;

        const double available100ns =
            static_cast<double>(elapsedMs) * 10000.0 * static_cast<double>(s_processorCount);
        return available100ns > 0.0
            ? (static_cast<double>(current - previous) * 100.0) / available100ns
            : 0.0;
    }

    DWORD GetCurrentProcessHandleCount()
    {
        typedef BOOL (WINAPI* GetProcessHandleCountFn)(HANDLE, PDWORD);
        HMODULE kernel32 = GetModuleHandleA("kernel32.dll");
        GetProcessHandleCountFn function = kernel32 != NULL
            ? reinterpret_cast<GetProcessHandleCountFn>(
                GetProcAddress(kernel32, "GetProcessHandleCount"))
            : NULL;
        DWORD count = 0;
        if (function != NULL)
            function(GetCurrentProcess(), &count);
        return count;
    }

    void RotateTelemetryIfNeeded()
    {
        WIN32_FILE_ATTRIBUTE_DATA data = { 0 };
        if (!GetFileAttributesExA(TELEMETRY_FILE, GetFileExInfoStandard, &data))
            return;

        ULARGE_INTEGER size;
        size.LowPart = data.nFileSizeLow;
        size.HighPart = data.nFileSizeHigh;
        if (size.QuadPart >= TELEMETRY_MAX_FILE_SIZE)
        {
            MoveFileExA(
                TELEMETRY_FILE,
                TELEMETRY_BACKUP_FILE,
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
        }
    }

    bool DequeueTelemetry(TelemetryLine& line)
    {
        EnterCriticalSection(&s_telemetryQueueLock);
        if (s_telemetryCount == 0) { LeaveCriticalSection(&s_telemetryQueueLock); return false; }
        line = s_telemetryQueue[s_telemetryHead];
        s_telemetryHead = (s_telemetryHead + 1) % TELEMETRY_QUEUE_CAPACITY; --s_telemetryCount;
        LeaveCriticalSection(&s_telemetryQueueLock); return true;
    }

    DWORD WINAPI TelemetryWriter(LPVOID)
    {
        HANDLE waits[2] = { s_telemetryStopEvent, s_telemetryWakeEvent };
        for (;;)
        {
            const DWORD result = WaitForMultipleObjects(2, waits, FALSE, INFINITE);
            TelemetryLine line;
            while (DequeueTelemetry(line))
            {
                RotateTelemetryIfNeeded();
                HANDLE file = CreateFileA(TELEMETRY_FILE, FILE_APPEND_DATA,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                    OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (file != INVALID_HANDLE_VALUE) { DWORD written = 0; WriteFile(file, line.text, line.length, &written, NULL); CloseHandle(file); }
            }
            if (result == WAIT_OBJECT_0) break;
        }
        return 0;
    }

    void EnqueueTelemetry(const char* line, DWORD length)
    {
        if (line == NULL || length == 0 || length >= TELEMETRY_LINE_CAPACITY || s_telemetryThread == NULL) return;
        EnterCriticalSection(&s_telemetryQueueLock);
        if (s_telemetryCount == TELEMETRY_QUEUE_CAPACITY) InterlockedIncrement(&s_telemetryDropped);
        else { TelemetryLine& target = s_telemetryQueue[s_telemetryTail]; memcpy(target.text, line, length); target.text[length] = '\0'; target.length = length;
            s_telemetryTail = (s_telemetryTail + 1) % TELEMETRY_QUEUE_CAPACITY; ++s_telemetryCount; }
        LeaveCriticalSection(&s_telemetryQueueLock);
        if (s_telemetryWakeEvent != NULL) SetEvent(s_telemetryWakeEvent);
    }

    void TryWriteSnapshot(DWORD nowTick)
    {
        const DWORD previousTick = static_cast<DWORD>(
            InterlockedCompareExchange(&s_lastSnapshotTick, 0, 0));
        const DWORD elapsedMs = nowTick - previousTick;
        if (elapsedMs < TELEMETRY_INTERVAL_MS)
            return;

        if (InterlockedCompareExchange(
                &s_lastSnapshotTick,
                static_cast<LONG>(nowTick),
                static_cast<LONG>(previousTick)) != static_cast<LONG>(previousTick))
            return;

        const LONG clientPackets = InterlockedExchange(&s_clientPackets, 0);
        const LONG serverPackets = InterlockedExchange(&s_serverPackets, 0);
        const LONG malformedPackets = InterlockedExchange(&s_malformedPackets, 0);
        LONG authFailures[7] = { 0 };
        for (int authIndex = 0; authIndex < 7; ++authIndex)
            authFailures[authIndex] = InterlockedExchange(&s_packetAuthFailures[authIndex], 0);
        const LONG runtimeErrors = InterlockedExchange(&s_runtimeErrors, 0);
        LONG uniqueSpawnRejected = 0;
        for (int rejectIndex = 0; rejectIndex < 7; ++rejectIndex)
            uniqueSpawnRejected += InterlockedExchange(&s_uniqueSpawnRejected[rejectIndex], 0);
        const LONG telemetryDropped = InterlockedExchange(&s_telemetryDropped, 0);
        const LONG clientHandlerMaxUs = InterlockedExchange(&s_clientHandlerMaxUs, 0);
        const LONG serverHandlerMaxUs = InterlockedExchange(&s_serverHandlerMaxUs, 0);
        const LONG gameLoopGapMaxMs = InterlockedExchange(&s_gameLoopGapMaxMs, 0);
        const LONG gameLoopWorkMaxUs = InterlockedExchange(&s_gameLoopWorkMaxUs, 0);
        const LONG queueTimerGapMaxMs = InterlockedExchange(&s_queueTimerGapMaxMs, 0);
        const LONG queueTimerWorkMaxUs = InterlockedExchange(&s_queueTimerWorkMaxUs, 0);
        const LONG liveDpsBatches = InterlockedExchange(&s_liveDpsBatches, 0);
        const LONG liveDpsPackets = InterlockedExchange(&s_liveDpsPackets, 0);
        const LONG fortressDpsSnapshots = InterlockedExchange(&s_fortressDpsSnapshots, 0);
        const LONG freeForAllEnabled = InterlockedCompareExchange(&s_freeForAllEnabled, 0, 0);
        const LONG databaseQueries = InterlockedExchange(&s_databaseQueries, 0);
        const LONG databaseFailures = InterlockedExchange(&s_databaseFailures, 0);
        const LONG databaseTotalMs = InterlockedExchange(&s_databaseTotalMs, 0);
        const LONG databaseMaxMs = InterlockedExchange(&s_databaseMaxMs, 0);
        const LONG snapshotDatabaseOk = InterlockedCompareExchange(&s_securitySnapshotDatabaseOk, 0, 0);
        const DWORD snapshotSuccessTick = static_cast<DWORD>(
            InterlockedCompareExchange(&s_securitySnapshotLastSuccessTick, 0, 0));
        const DWORD snapshotAgeSeconds = snapshotSuccessTick == 0
            ? 0xFFFFFFFF : (nowTick - snapshotSuccessTick) / 1000;

        LONG clientP50Us = 0;
        LONG clientP95Us = 0;
        LONG clientP99Us = 0;
        LONG serverP50Us = 0;
        LONG serverP95Us = 0;
        LONG serverP99Us = 0;
        LONG gameLoopP50Us = 0;
        LONG gameLoopP95Us = 0;
        LONG gameLoopP99Us = 0;
        SnapshotPercentiles(s_clientLatencyBuckets, clientP50Us, clientP95Us, clientP99Us);
        SnapshotPercentiles(s_serverLatencyBuckets, serverP50Us, serverP95Us, serverP99Us);
        SnapshotPercentiles(s_gameLoopLatencyBuckets, gameLoopP50Us, gameLoopP95Us, gameLoopP99Us);

        OpcodeSnapshot topClient[TOP_OPCODE_COUNT];
        OpcodeSnapshot topServer[TOP_OPCODE_COUNT];
        SnapshotTopOpcodes(true, topClient);
        SnapshotTopOpcodes(false, topServer);
        char topClientText[256];
        char topServerText[256];
        FormatTopOpcodes(topClientText, sizeof(topClientText), topClient);
        FormatTopOpcodes(topServerText, sizeof(topServerText), topServer);

        const double seconds = elapsedMs / 1000.0;
        const double clientPacketsPerSecond = seconds > 0.0 ? clientPackets / seconds : 0.0;
        const double serverPacketsPerSecond = seconds > 0.0 ? serverPackets / seconds : 0.0;
        const double cpuPercent = GetProcessCpuPercent(elapsedMs);

        SYSTEMTIME time;
        GetLocalTime(&time);

        PROCESS_MEMORY_COUNTERS memoryCounters;
        ZeroMemory(&memoryCounters, sizeof(memoryCounters));
        memoryCounters.cb = sizeof(memoryCounters);
        GetProcessMemoryInfo(GetCurrentProcess(), &memoryCounters, sizeof(memoryCounters));
        const double privateMemoryMb = memoryCounters.PagefileUsage / (1024.0 * 1024.0);
        const double workingSetMb = memoryCounters.WorkingSetSize / (1024.0 * 1024.0);
        const DWORD handleCount = GetCurrentProcessHandleCount();

        char line[2048];
        const int length = _snprintf_s(
            line,
            sizeof(line),
            _TRUNCATE,
            "%04u-%02u-%02u %02u:%02u:%02u cpu_pct=%.2f private_mb=%.1f working_set_mb=%.1f handles=%lu "
            "client_pps=%.2f server_pps=%.2f malformed=%ld runtime_errors=%ld unique_spawn_rejected=%ld telemetry_dropped=%ld "
            "auth_not_ready=%ld auth_version=%ld auth_gameid=%ld auth_expired=%ld auth_replay=%ld auth_mac=%ld "
            "client_us=%ld/%ld/%ld/%ld server_us=%ld/%ld/%ld/%ld "
            "game_loop_us=%ld/%ld/%ld/%ld game_loop_gap_max_ms=%ld "
            "queue_timer_gap_max_ms=%ld queue_timer_work_max_us=%ld "
            "session_keys=%ld unique_type_cache=%ld live_dps_pending=%ld "
            "live_dps_pending_peak=%ld locked_items=%ld live_dps_batches=%ld live_dps_packets=%ld "
            "fortress_dps_snapshots=%ld ffa_enabled=%ld "
            "db_queries=%ld db_failures=%ld db_avg_ms=%.2f db_max_ms=%ld snapshot_db_ok=%ld snapshot_age_s=%lu "
            "top_client=%s top_server=%s\r\n",
            time.wYear, time.wMonth, time.wDay,
            time.wHour, time.wMinute, time.wSecond,
            cpuPercent, privateMemoryMb, workingSetMb, handleCount,
            clientPacketsPerSecond, serverPacketsPerSecond,
            malformedPackets, runtimeErrors, uniqueSpawnRejected, telemetryDropped,
            authFailures[1], authFailures[2], authFailures[3],
            authFailures[4], authFailures[5], authFailures[6],
            clientP50Us, clientP95Us, clientP99Us, clientHandlerMaxUs,
            serverP50Us, serverP95Us, serverP99Us, serverHandlerMaxUs,
            gameLoopP50Us, gameLoopP95Us, gameLoopP99Us, gameLoopWorkMaxUs,
            gameLoopGapMaxMs, queueTimerGapMaxMs, queueTimerWorkMaxUs,
            InterlockedCompareExchange(&s_sessionKeyCacheSize, 0, 0),
            InterlockedCompareExchange(&s_uniqueTypeCacheSize, 0, 0),
            InterlockedCompareExchange(&s_liveDpsPendingCacheSize, 0, 0),
            InterlockedCompareExchange(&s_liveDpsPendingCachePeak, 0, 0),
            InterlockedCompareExchange(&s_lockedItemCacheSize, 0, 0),
            liveDpsBatches, liveDpsPackets, fortressDpsSnapshots, freeForAllEnabled,
            databaseQueries, databaseFailures,
            databaseQueries > 0 ? static_cast<double>(databaseTotalMs) / databaseQueries : 0.0,
            databaseMaxMs, snapshotDatabaseOk, static_cast<unsigned long>(snapshotAgeSeconds),
            topClientText, topServerText);

        if (length <= 0)
            return;

        EnqueueTelemetry(line, static_cast<DWORD>(length));

        char title[160];
        _snprintf_s(
            title,
            sizeof(title),
            _TRUNCATE,
            "KMTGuard GameServer | READY | RAM %.0f MB | CPU %.1f%% | %.1f packets/s",
            privateMemoryMb,
            cpuPercent,
            clientPacketsPerSecond + serverPacketsPerSecond);
        SetConsoleTitleA(title);
    }
}

void GameServerTelemetry::Initialize()
{
    if (InterlockedCompareExchange(&s_telemetryState, 4, 0) == 0)
    {
        s_telemetryStopEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
        s_telemetryWakeEvent = CreateEventA(NULL, FALSE, FALSE, NULL);
        if (s_telemetryStopEvent != NULL && s_telemetryWakeEvent != NULL)
            s_telemetryThread = CreateThread(NULL, 0, TelemetryWriter, NULL, 0, NULL);
        if (s_telemetryThread != NULL)
            InterlockedExchange(&s_telemetryState, 1);
        else
        {
            if (s_telemetryWakeEvent != NULL) CloseHandle(s_telemetryWakeEvent);
            if (s_telemetryStopEvent != NULL) CloseHandle(s_telemetryStopEvent);
            s_telemetryWakeEvent = NULL;
            s_telemetryStopEvent = NULL;
            InterlockedExchange(&s_telemetryState, 0);
        }
    }
    QueryPerformanceFrequency(&s_performanceFrequency);
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);
    s_processorCount = systemInfo.dwNumberOfProcessors > 0
        ? systemInfo.dwNumberOfProcessors
        : 1;

    FILETIME creation = { 0 };
    FILETIME exitTime = { 0 };
    FILETIME kernel = { 0 };
    FILETIME user = { 0 };
    if (GetProcessTimes(GetCurrentProcess(), &creation, &exitTime, &kernel, &user))
        s_lastProcessCpu100ns = FileTimeToUInt64(kernel) + FileTimeToUInt64(user);

    const DWORD nowTick = GetTickCount();
    InterlockedExchange(&s_lastSnapshotTick, static_cast<LONG>(nowTick));
    InterlockedExchange(&s_lastGameLoopTick, static_cast<LONG>(nowTick));
    InterlockedExchange(&s_lastQueueTimerTick, static_cast<LONG>(nowTick));
}

void GameServerTelemetry::Shutdown()
{
    const LONG previous = InterlockedCompareExchange(&s_telemetryState, 2, 1);
    if (previous == 0 || previous == 2 || previous == 3)
        return;
    if (previous != 1)
        return;
    if (s_telemetryStopEvent != NULL) SetEvent(s_telemetryStopEvent);
    if (s_telemetryThread != NULL)
    {
        if (WaitForSingleObject(s_telemetryThread, 5000) != WAIT_OBJECT_0)
        {
            // The DLL is process-lifetime pinned. Retain handles beneath the live
            // writer and make the subsystem terminal rather than permitting an
            // unsafe duplicate writer or closing objects still in use.
            InterlockedExchange(&s_telemetryState, 3);
            return;
        }
        CloseHandle(s_telemetryThread); s_telemetryThread = NULL;
    }
    if (s_telemetryWakeEvent != NULL) { CloseHandle(s_telemetryWakeEvent); s_telemetryWakeEvent = NULL; }
    if (s_telemetryStopEvent != NULL) { CloseHandle(s_telemetryStopEvent); s_telemetryStopEvent = NULL; }
    s_telemetryHead = s_telemetryTail = s_telemetryCount = 0;
    InterlockedExchange(&s_telemetryState, 0);
}

void GameServerTelemetry::RecordMalformedPacket()
{
    InterlockedIncrement(&s_malformedPackets);
}

void GameServerTelemetry::RecordPacketAuthFailure(int reason)
{
    if (reason >= 0 && reason < 7)
        InterlockedIncrement(&s_packetAuthFailures[reason]);
}

void GameServerTelemetry::RecordRuntimeError()
{
    InterlockedIncrement(&s_runtimeErrors);
}

void GameServerTelemetry::RecordUniqueSpawnRejected(int reason)
{
    if (reason >= 0 && reason < 7) InterlockedIncrement(&s_uniqueSpawnRejected[reason]);
}

void GameServerTelemetry::RecordMalformedPacketForSession(DWORD sessionId, WORD)
{
    RecordMalformedPacket();
    if (sessionId == 0) return;
    EnterCriticalSection(&s_malformedSessionLock);
    DWORD& count = s_malformedSessions[sessionId];
    if (count != 0xFFFFFFFF) ++count;
    if (s_malformedSessions.size() > 8192) s_malformedSessions.erase(s_malformedSessions.begin());
    LeaveCriticalSection(&s_malformedSessionLock);
}

void GameServerTelemetry::ForgetSession(DWORD sessionId)
{
    EnterCriticalSection(&s_malformedSessionLock);
    s_malformedSessions.erase(sessionId);
    LeaveCriticalSection(&s_malformedSessionLock);
}

void GameServerTelemetry::RecordLiveDpsBatch(unsigned int packetCount)
{
    InterlockedIncrement(&s_liveDpsBatches);
    InterlockedExchangeAdd(&s_liveDpsPackets, static_cast<LONG>(packetCount));
}

void GameServerTelemetry::RecordFortressDpsSnapshot()
{
    InterlockedIncrement(&s_fortressDpsSnapshots);
}

void GameServerTelemetry::SetFreeForAllState(bool enabled)
{
    InterlockedExchange(&s_freeForAllEnabled, enabled ? 1 : 0);
}

void GameServerTelemetry::RecordDatabaseQuery(DWORD elapsedMs, bool success)
{
    InterlockedIncrement(&s_databaseQueries);
    if (!success)
        InterlockedIncrement(&s_databaseFailures);
    const LONG boundedElapsed = elapsedMs > static_cast<DWORD>(LONG_MAX)
        ? LONG_MAX : static_cast<LONG>(elapsedMs);
    InterlockedExchangeAdd(&s_databaseTotalMs, boundedElapsed);
    UpdateMaximum(&s_databaseMaxMs, boundedElapsed);
}

void GameServerTelemetry::RecordSecuritySnapshotRefresh(bool success)
{
    InterlockedExchange(&s_securitySnapshotDatabaseOk, success ? 1 : 0);
    if (success)
        InterlockedExchange(&s_securitySnapshotLastSuccessTick, static_cast<LONG>(GetTickCount()));
}

WORD GameServerTelemetry::GetLastClientOpcode()
{
    return static_cast<WORD>(InterlockedCompareExchange(&s_lastClientOpcode, 0, 0));
}

WORD GameServerTelemetry::GetLastServerOpcode()
{
    return static_cast<WORD>(InterlockedCompareExchange(&s_lastServerOpcode, 0, 0));
}

void GameServerTelemetry::SetSessionKeyCacheSize(size_t value)
{
    InterlockedExchange(&s_sessionKeyCacheSize, ClampSize(value));
}

void GameServerTelemetry::SetUniqueTypeCacheSize(size_t value)
{
    InterlockedExchange(&s_uniqueTypeCacheSize, ClampSize(value));
}

void GameServerTelemetry::SetLiveDpsPendingCacheSize(size_t value)
{
    const LONG size = ClampSize(value);
    InterlockedExchange(&s_liveDpsPendingCacheSize, size);
    UpdateMaximum(&s_liveDpsPendingCachePeak, size);
}

void GameServerTelemetry::SetLockedItemCacheSize(size_t value)
{
    InterlockedExchange(&s_lockedItemCacheSize, ClampSize(value));
}

GameServerTelemetry::ScopedPacketTimer::ScopedPacketTimer(bool clientPacket, WORD opcode)
    : m_clientPacket(clientPacket), m_opcode(opcode)
{
    InterlockedExchange(clientPacket ? &s_lastClientOpcode : &s_lastServerOpcode, opcode);
    if (clientPacket)
        InterlockedIncrement(&s_clientPackets);
    else
        InterlockedIncrement(&s_serverPackets);
    QueryPerformanceCounter(&m_started);
}

GameServerTelemetry::ScopedPacketTimer::~ScopedPacketTimer()
{
    const LONG elapsedUs = ElapsedMicroseconds(m_started);
    UpdateMaximum(m_clientPacket ? &s_clientHandlerMaxUs : &s_serverHandlerMaxUs, elapsedUs);
    RecordLatency(m_clientPacket ? s_clientLatencyBuckets : s_serverLatencyBuckets, elapsedUs);
    RecordOpcode(m_clientPacket, m_opcode, elapsedUs);
}

GameServerTelemetry::ScopedGameLoopTimer::ScopedGameLoopTimer()
{
    const DWORD nowTick = GetTickCount();
    const DWORD previousTick = static_cast<DWORD>(
        InterlockedExchange(&s_lastGameLoopTick, static_cast<LONG>(nowTick)));
    UpdateMaximum(&s_gameLoopGapMaxMs, static_cast<LONG>(nowTick - previousTick));
    QueryPerformanceCounter(&m_started);
}

GameServerTelemetry::ScopedGameLoopTimer::~ScopedGameLoopTimer()
{
    const LONG elapsedUs = ElapsedMicroseconds(m_started);
    UpdateMaximum(&s_gameLoopWorkMaxUs, elapsedUs);
    RecordLatency(s_gameLoopLatencyBuckets, elapsedUs);
}

GameServerTelemetry::ScopedQueueTick::ScopedQueueTick()
{
    const DWORD nowTick = GetTickCount();
    const DWORD previousTick = static_cast<DWORD>(
        InterlockedExchange(&s_lastQueueTimerTick, static_cast<LONG>(nowTick)));
    UpdateMaximum(&s_queueTimerGapMaxMs, static_cast<LONG>(nowTick - previousTick));
    QueryPerformanceCounter(&m_started);
}

GameServerTelemetry::ScopedQueueTick::~ScopedQueueTick()
{
    UpdateMaximum(&s_queueTimerWorkMaxUs, ElapsedMicroseconds(m_started));
    TryWriteSnapshot(GetTickCount());
}
