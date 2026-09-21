#include "GameServerRuntimeSafety.h"

#include <BSObj/BSObj.h>
#include <memory/detours.h>

#include <climits>
#include <cstring>

namespace
{
    const DWORD EXPECTED_IMAGE_BASE = 0x00400000;
    const DWORD EXPECTED_IMAGE_TIMESTAMP = 0x4E3FB0DB;
    const DWORD EXPECTED_IMAGE_SIZE = 0x00971000;

    struct ByteSignature
    {
        DWORD address;
        const BYTE* bytes;
        size_t length;
        const char* name;
    };

    struct PointerSignature
    {
        DWORD address;
        DWORD value;
        const char* name;
    };

    const BYTE SIGNATURE_GREEN_BOOK_1[] =
        { 0x81, 0xE9, 0x22, 0x31, 0x00, 0x00, 0x74, 0x51 };
    const BYTE SIGNATURE_GREEN_BOOK_2[] =
        { 0xE8, 0xEE, 0x1E, 0x52, 0x00 };
    const BYTE SIGNATURE_REGION_CHANGE[] =
        { 0x89, 0x85, 0x84, 0x00, 0x00, 0x00 };
    const BYTE SIGNATURE_DURABILITY[] =
        { 0x83, 0x4E, 0x08, 0x04, 0x89, 0x7E, 0x38 };
    const BYTE SIGNATURE_AGGRO[] =
        { 0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8 };
    const BYTE SIGNATURE_CAN_ATTACK[] =
        { 0x83, 0xB9, 0xD8, 0x00, 0x00, 0x00, 0x00 };
    const BYTE SIGNATURE_PARTY_MEMBERS[] =
        { 0x83, 0x7C, 0x24, 0x08, 0x02 };
    const BYTE SIGNATURE_PARTY_RATE[] =
        { 0x83, 0xFA, 0x32 };
    const BYTE SIGNATURE_LOG_HANDLER[] =
        { 0x6A, 0xFF, 0x68, 0xF8, 0x10, 0xA8 };
    const BYTE SIGNATURE_QUEUE_TIMER[] =
        { 0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8 };
    const BYTE SIGNATURE_GM_UNIQUE_KILL_NOTICE[] =
        { 0x0F, 0x84, 0xB7, 0x00, 0x00, 0x00 };

    const ByteSignature BYTE_SIGNATURES[] =
    {
        { 0x004142E2, SIGNATURE_GREEN_BOOK_1, sizeof(SIGNATURE_GREEN_BOOK_1), "Green Book patch 1" },
        { 0x0041474D, SIGNATURE_GREEN_BOOK_2, sizeof(SIGNATURE_GREEN_BOOK_2), "Green Book patch 2" },
        { 0x00485AA3, SIGNATURE_REGION_CHANGE, sizeof(SIGNATURE_REGION_CHANGE), "region-change hook" },
        { 0x00496E34, SIGNATURE_DURABILITY, sizeof(SIGNATURE_DURABILITY), "durability hook" },
        { 0x004C44A0, SIGNATURE_AGGRO, sizeof(SIGNATURE_AGGRO), "aggro hook" },
        { 0x004AA640, SIGNATURE_CAN_ATTACK, sizeof(SIGNATURE_CAN_ATTACK), "attack hook" },
        { 0x00558F20, SIGNATURE_PARTY_MEMBERS, sizeof(SIGNATURE_PARTY_MEMBERS), "party-member patch" },
        { 0x005608E2, SIGNATURE_PARTY_RATE, sizeof(SIGNATURE_PARTY_RATE), "party-rate patch" },
        { 0x00935BD0, SIGNATURE_LOG_HANDLER, sizeof(SIGNATURE_LOG_HANDLER), "log hook" },
        { 0x0094CE60, SIGNATURE_QUEUE_TIMER, sizeof(SIGNATURE_QUEUE_TIMER), "queue timer" },
        { 0x004C1D23, SIGNATURE_GM_UNIQUE_KILL_NOTICE, sizeof(SIGNATURE_GM_UNIQUE_KILL_NOTICE), "GM Unique kill notice branch" }
    };

    const PointerSignature POINTER_SIGNATURES[] =
    {
        { 0x00ADF020, 0x00402C20, "main process message slot" },
        { 0x00ADF028, 0x0094CE60, "main process timer slot" },
        { 0x00AF59FC, 0x004DE9B0, "player destroy slot" },
        { 0x00AF5F0C, 0x004A7540, "player kill slot" },
        { 0x00AF5FDC, 0x0050EEE0, "player packet slot" },
        { 0x00AF6018, 0x004DF9E0, "player spawn slot" },
        { 0x00AEE3E4, 0x004C1C80, "monster death slot" },
        { 0x00AF2594, 0x004D2AD0, "gold pet slot" },
        { 0x00AEE2A0, 0x0052A8E0, "live DPS slot" }
    };

    bool IsReadableRange(const void* address, size_t length)
    {
        if (address == NULL || length == 0)
            return false;

        MEMORY_BASIC_INFORMATION info = { 0 };
        if (VirtualQuery(address, &info, sizeof(info)) == 0 ||
            info.State != MEM_COMMIT ||
            (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0)
            return false;

        const DWORD_PTR start = reinterpret_cast<DWORD_PTR>(address);
        const DWORD_PTR end = reinterpret_cast<DWORD_PTR>(info.BaseAddress) + info.RegionSize;
        return start <= end && length <= static_cast<size_t>(end - start);
    }

    bool IsExpectedExecutableName()
    {
        char path[MAX_PATH] = { 0 };
        if (GetModuleFileNameA(NULL, path, MAX_PATH) == 0)
            return false;

        const char* name = std::strrchr(path, '\\');
        name = name != NULL ? name + 1 : path;
        return _stricmp(name, "SR_GameServer.exe") == 0;
    }
}

bool GameServerRuntimeSafety::MatchesBytes(DWORD address, const BYTE* expected, size_t length)
{
    return expected != NULL &&
           IsReadableRange(reinterpret_cast<const void*>(address), length) &&
           std::memcmp(reinterpret_cast<const void*>(address), expected, length) == 0;
}

bool GameServerRuntimeSafety::ValidateHost()
{
#if !defined(_M_IX86)
    BS_INFO("[KMTGuard] Unsupported architecture; 32-bit SR_GameServer is required");
    return false;
#else
    HMODULE module = GetModuleHandleA(NULL);
    if (module == NULL || reinterpret_cast<DWORD>(module) != EXPECTED_IMAGE_BASE ||
        !IsExpectedExecutableName())
    {
        BS_INFO("[KMTGuard] Unsupported host executable");
        return false;
    }

    const IMAGE_DOS_HEADER* dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
    if (!IsReadableRange(dosHeader, sizeof(*dosHeader)) || dosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        BS_INFO("[KMTGuard] Invalid host DOS header");
        return false;
    }

    const IMAGE_NT_HEADERS32* ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS32*>(
        reinterpret_cast<const BYTE*>(module) + dosHeader->e_lfanew);
    if (!IsReadableRange(ntHeaders, sizeof(*ntHeaders)) ||
        ntHeaders->Signature != IMAGE_NT_SIGNATURE ||
        ntHeaders->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
        ntHeaders->FileHeader.TimeDateStamp != EXPECTED_IMAGE_TIMESTAMP ||
        ntHeaders->OptionalHeader.ImageBase != EXPECTED_IMAGE_BASE ||
        ntHeaders->OptionalHeader.SizeOfImage != EXPECTED_IMAGE_SIZE)
    {
        BS_INFO("[KMTGuard] Unsupported SR_GameServer build");
        return false;
    }

    for (size_t i = 0; i < sizeof(BYTE_SIGNATURES) / sizeof(BYTE_SIGNATURES[0]); ++i)
    {
        const ByteSignature& signature = BYTE_SIGNATURES[i];
        if (!MatchesBytes(signature.address, signature.bytes, signature.length))
        {
            BS_INFO("[KMTGuard] Compatibility check failed: %s", signature.name);
            return false;
        }
    }

    for (size_t i = 0; i < sizeof(POINTER_SIGNATURES) / sizeof(POINTER_SIGNATURES[0]); ++i)
    {
        const PointerSignature& signature = POINTER_SIGNATURES[i];
        if (!IsReadableRange(reinterpret_cast<const void*>(signature.address), sizeof(DWORD)) ||
            *reinterpret_cast<const DWORD*>(signature.address) != signature.value)
        {
            BS_INFO("[KMTGuard] Compatibility check failed: %s", signature.name);
            return false;
        }
    }

    return true;
#endif
}

bool GameServerRuntimeSafety::AttachDetour(
    PVOID* originalFunction,
    PVOID replacementFunction,
    const char* name)
{
    if (originalFunction == NULL || *originalFunction == NULL || replacementFunction == NULL)
        return false;

    LONG result = DetourTransactionBegin();
    if (result == NO_ERROR)
        result = DetourUpdateThread(GetCurrentThread());
    if (result == NO_ERROR)
        result = DetourAttach(originalFunction, replacementFunction);
    if (result == NO_ERROR)
        result = DetourTransactionCommit();
    else
        DetourTransactionAbort();

    if (result != NO_ERROR)
        BS_INFO("[KMTGuard] Failed to install %s (error=%ld)", name != NULL ? name : "detour", result);
    return result == NO_ERROR;
}

bool GameServerRuntimeSafety::DetachDetour(
    PVOID* originalFunction,
    PVOID replacementFunction,
    const char* name)
{
    if (originalFunction == NULL || *originalFunction == NULL || replacementFunction == NULL)
        return false;

    LONG result = DetourTransactionBegin();
    if (result == NO_ERROR)
        result = DetourUpdateThread(GetCurrentThread());
    if (result == NO_ERROR)
        result = DetourDetach(originalFunction, replacementFunction);
    if (result == NO_ERROR)
        result = DetourTransactionCommit();
    else
        DetourTransactionAbort();

    if (result != NO_ERROR)
        BS_INFO("[KMTGuard] Failed to remove %s (error=%ld)", name != NULL ? name : "detour", result);
    return result == NO_ERROR;
}

bool GameServerRuntimeSafety::RestoreBytes(
    DWORD address,
    const BYTE* expectedCurrentBytes,
    const BYTE* originalBytes,
    size_t length,
    const char* name)
{
    if (originalBytes == NULL || length == 0 || length > 64 ||
        !IsReadableRange(reinterpret_cast<const void*>(address), length) ||
        (expectedCurrentBytes != NULL &&
         std::memcmp(reinterpret_cast<const void*>(address), expectedCurrentBytes, length) != 0))
    {
        BS_INFO("[KMTGuard] Refused incompatible rollback for %s", name != NULL ? name : "memory patch");
        return false;
    }

    BYTE* target = reinterpret_cast<BYTE*>(address);
    DWORD oldProtect = 0;
    if (!VirtualProtect(target, length, PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;

    std::memcpy(target, originalBytes, length);
    FlushInstructionCache(GetCurrentProcess(), target, length);

    DWORD ignoredProtect = 0;
    const bool restoredProtection =
        VirtualProtect(target, length, oldProtect, &ignoredProtect) != FALSE;
    const bool restoredBytes =
        std::memcmp(reinterpret_cast<const void*>(address), originalBytes, length) == 0;
    if (!restoredProtection || !restoredBytes)
        BS_INFO("[KMTGuard] Failed to rollback %s", name != NULL ? name : "memory patch");
    return restoredProtection && restoredBytes;
}

GameServerMemoryPatchTransaction::GameServerMemoryPatchTransaction()
    : m_failed(false), m_committed(false)
{
}

GameServerMemoryPatchTransaction::~GameServerMemoryPatchTransaction()
{
    if (!m_committed)
        Rollback();
}

bool GameServerMemoryPatchTransaction::WriteRaw(
    DWORD address,
    const void* value,
    size_t length,
    const char* name)
{
    if (m_failed || m_committed || value == NULL || length == 0 || length > 64 ||
        !IsReadableRange(reinterpret_cast<const void*>(address), length))
    {
        m_failed = true;
        BS_INFO("[KMTGuard] Refused invalid %s", name != NULL ? name : "runtime setting");
        return false;
    }

    Record record;
    record.address = address;
    record.originalBytes.resize(length);
    std::memcpy(&record.originalBytes[0], reinterpret_cast<const void*>(address), length);

    BYTE* target = reinterpret_cast<BYTE*>(address);
    DWORD oldProtect = 0;
    if (!VirtualProtect(target, length, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        m_failed = true;
        return false;
    }

    std::memcpy(target, value, length);
    FlushInstructionCache(GetCurrentProcess(), target, length);
    DWORD ignoredProtect = 0;
    const bool restoredProtection =
        VirtualProtect(target, length, oldProtect, &ignoredProtect) != FALSE;
    const bool verified = std::memcmp(target, value, length) == 0;
    if (!restoredProtection || !verified)
    {
        DWORD rollbackProtect = 0;
        if (VirtualProtect(target, length, PAGE_EXECUTE_READWRITE, &rollbackProtect))
        {
            std::memcpy(target, &record.originalBytes[0], length);
            FlushInstructionCache(GetCurrentProcess(), target, length);
            VirtualProtect(target, length, rollbackProtect, &ignoredProtect);
        }
        m_failed = true;
        BS_INFO("[KMTGuard] Failed to apply %s", name != NULL ? name : "runtime setting");
        return false;
    }

    m_records.push_back(record);
    return true;
}

bool GameServerMemoryPatchTransaction::Commit()
{
    if (m_failed || m_committed)
        return false;
    m_committed = true;
    return true;
}

void GameServerMemoryPatchTransaction::Rollback()
{
    while (!m_records.empty())
    {
        const Record& record = m_records.back();
        GameServerRuntimeSafety::RestoreBytes(
            record.address,
            NULL,
            &record.originalBytes[0],
            record.originalBytes.size(),
            "runtime-setting transaction");
        m_records.pop_back();
    }
}

bool GameServerRuntimeSafety::ReplacePointer(
    DWORD address,
    DWORD expectedValue,
    DWORD replacementValue,
    const char* name)
{
    if (replacementValue == 0 ||
        !IsReadableRange(reinterpret_cast<const void*>(address), sizeof(DWORD)) ||
        *reinterpret_cast<const DWORD*>(address) != expectedValue)
    {
        BS_INFO("[KMTGuard] Refused incompatible %s", name != NULL ? name : "pointer hook");
        return false;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(reinterpret_cast<void*>(address), sizeof(DWORD), PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;

    *reinterpret_cast<volatile DWORD*>(address) = replacementValue;
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<const void*>(address), sizeof(DWORD));

    DWORD ignoredProtect = 0;
    const bool restored = VirtualProtect(
        reinterpret_cast<void*>(address),
        sizeof(DWORD),
        oldProtect,
        &ignoredProtect) != FALSE;
    const bool installed = *reinterpret_cast<const DWORD*>(address) == replacementValue;
    if (!restored || !installed)
        BS_INFO("[KMTGuard] Failed to install %s", name != NULL ? name : "pointer hook");
    return restored && installed;
}

bool GameServerRuntimeSafety::InstallRelativeJump(
    DWORD address,
    const BYTE* expectedBytes,
    size_t patchLength,
    const void* replacementFunction,
    const char* name)
{
    if (patchLength < 5 || patchLength > 16 || replacementFunction == NULL ||
        !MatchesBytes(address, expectedBytes, patchLength))
    {
        BS_INFO("[KMTGuard] Refused incompatible %s", name != NULL ? name : "code hook");
        return false;
    }

    const INT_PTR displacement =
        reinterpret_cast<const BYTE*>(replacementFunction) -
        reinterpret_cast<const BYTE*>(address) - 5;
    if (displacement < INT_MIN || displacement > INT_MAX)
        return false;

    BYTE original[16] = { 0 };
    std::memcpy(original, reinterpret_cast<const void*>(address), patchLength);

    BYTE* target = reinterpret_cast<BYTE*>(address);
    DWORD oldProtect = 0;
    if (!VirtualProtect(target, patchLength, PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;

    target[0] = 0xE9;
    *reinterpret_cast<int*>(target + 1) = static_cast<int>(displacement);
    for (size_t i = 5; i < patchLength; ++i)
        target[i] = 0x90;
    FlushInstructionCache(GetCurrentProcess(), target, patchLength);

    DWORD ignoredProtect = 0;
    if (!VirtualProtect(target, patchLength, oldProtect, &ignoredProtect))
    {
        DWORD rollbackProtect = 0;
        if (VirtualProtect(target, patchLength, PAGE_EXECUTE_READWRITE, &rollbackProtect))
        {
            std::memcpy(target, original, patchLength);
            VirtualProtect(target, patchLength, rollbackProtect, &ignoredProtect);
        }
        return false;
    }

    return true;
}
