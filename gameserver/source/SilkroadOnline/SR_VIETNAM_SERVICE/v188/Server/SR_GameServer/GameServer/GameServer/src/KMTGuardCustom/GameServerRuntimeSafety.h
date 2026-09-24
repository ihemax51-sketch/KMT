#pragma once

#include <Windows.h>
#include <cstddef>
#include <vector>

class GameServerMemoryPatchTransaction
{
public:
    GameServerMemoryPatchTransaction();
    ~GameServerMemoryPatchTransaction();

    bool WriteRaw(DWORD address, const void* value, size_t length, const char* name = NULL);

    template <typename T>
    bool WriteValue(DWORD address, const T& value, const char* name = NULL)
    {
        return WriteRaw(address, &value, sizeof(T), name);
    }

    bool Commit();
    void Rollback();

private:
    struct Record
    {
        DWORD address;
        std::vector<BYTE> originalBytes;
    };

    std::vector<Record> m_records;
    bool m_failed;
    bool m_committed;

    GameServerMemoryPatchTransaction(const GameServerMemoryPatchTransaction&);
    GameServerMemoryPatchTransaction& operator=(const GameServerMemoryPatchTransaction&);
};

namespace GameServerRuntimeSafety
{
    enum InitializationState
    {
        INITIALIZATION_UNINITIALIZED = 0,
        INITIALIZATION_INITIALIZING = 1,
        INITIALIZATION_READY = 2,
        INITIALIZATION_FAILED = 3,
        INITIALIZATION_STOPPING = 4
    };

    bool BeginInitialization();
    void MarkInitializationReady();
    void MarkInitializationFailed();
    bool IsInitializationReady();
    InitializationState GetInitializationState();
    bool ValidateHost();
    bool MatchesBytes(DWORD address, const BYTE* expected, size_t length);
    bool AttachDetour(PVOID* originalFunction, PVOID replacementFunction, const char* name);
    bool DetachDetour(PVOID* originalFunction, PVOID replacementFunction, const char* name);
    bool ReplacePointer(DWORD address, DWORD expectedValue, DWORD replacementValue, const char* name);
    bool RestoreBytes(
        DWORD address,
        const BYTE* expectedCurrentBytes,
        const BYTE* originalBytes,
        size_t length,
        const char* name);
    bool InstallRelativeJump(
        DWORD address,
        const BYTE* expectedBytes,
        size_t patchLength,
        const void* replacementFunction,
        const char* name);
}
