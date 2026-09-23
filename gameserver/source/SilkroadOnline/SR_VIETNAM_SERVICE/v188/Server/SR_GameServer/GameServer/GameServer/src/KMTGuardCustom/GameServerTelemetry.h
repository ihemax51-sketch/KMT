#pragma once

#include <Windows.h>
#include <cstddef>

namespace GameServerTelemetry
{
    void Initialize();
    void Shutdown();
    void RecordMalformedPacket();
    void RecordPacketAuthFailure(int reason);
    void RecordRuntimeError();
    void RecordUniqueSpawnRejected(int reason);
    void RecordMalformedPacketForSession(DWORD sessionId, WORD opcode);
    void ForgetSession(DWORD sessionId);
    void RecordLiveDpsBatch(unsigned int packetCount);
    void RecordFortressDpsSnapshot();
    void SetFreeForAllState(bool enabled);
    void RecordDatabaseQuery(DWORD elapsedMs, bool success);
    void RecordSecuritySnapshotRefresh(bool success);
    WORD GetLastClientOpcode();
    WORD GetLastServerOpcode();

    void SetSessionKeyCacheSize(size_t value);
    void SetUniqueTypeCacheSize(size_t value);
    void SetLiveDpsPendingCacheSize(size_t value);
    void SetLockedItemCacheSize(size_t value);

    class ScopedPacketTimer
    {
    public:
        ScopedPacketTimer(bool clientPacket, WORD opcode);
        ~ScopedPacketTimer();

    private:
        bool m_clientPacket;
        WORD m_opcode;
        LARGE_INTEGER m_started;
    };

    class ScopedGameLoopTimer
    {
    public:
        ScopedGameLoopTimer();
        ~ScopedGameLoopTimer();

    private:
        LARGE_INTEGER m_started;
    };

    class ScopedQueueTick
    {
    public:
        ScopedQueueTick();
        ~ScopedQueueTick();

    private:
        LARGE_INTEGER m_started;
    };
}
