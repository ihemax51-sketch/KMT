#include "ShardManagerRuntimeSafety.h"

#include <Windows.h>
#include "../Utils/Memory/hook.h"
#include "../Utils/Memory/Process.h"
#include "../Network/MainProcess.h"
#include "../Utils/BSObj.h"

namespace
{
    const DWORD PROCESS_MESSAGE_POINTER = 0x00783238;
    const DWORD ORIGINAL_PROCESS_MESSAGE = 0x0040DDA0;
    const DWORD ORIGINAL_HANDLE_MESSAGE = 0x006990D0;
    const DWORD GET_MAIN_PROCESS_INSTANCE = 0x00401D30;
    const DWORD NET_ENGINE_REFERENCE = 0x00858868;
    const DWORD BROADCAST_MESSAGE = 0x004121E0;
    const DWORD ALLOCATE_MESSAGE = 0x006B8260;

    DWORD s_originalProcessMessagePointer = 0;
    volatile LONG s_hooksInstalled = 0;

    bool IsCommittedAddress(DWORD address, bool executable)
    {
        MEMORY_BASIC_INFORMATION info;
        if (VirtualQuery(reinterpret_cast<const void*>(address), &info, sizeof(info)) != sizeof(info))
            return false;
        if (info.State != MEM_COMMIT || (info.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0)
            return false;
        if (!executable)
            return true;
        const DWORD protection = info.Protect & 0xFF;
        return protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ ||
            protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
    }
}

bool ShardManagerRuntimeSafety::ValidateHost()
{
    if (sizeof(void*) != 4)
    {
        BS_ERROR("KMTGuard ShardManager requires a 32-bit host process");
        return false;
    }

    HMODULE module = GetModuleHandle(NULL);
    if (module == NULL)
        return false;
	if (reinterpret_cast<DWORD>(module) != 0x00400000)
	{
		BS_ERROR("ShardManager executable is not loaded at the supported base address");
		return false;
	}
    const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(module);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return false;
    const IMAGE_NT_HEADERS32* nt = reinterpret_cast<const IMAGE_NT_HEADERS32*>(
        reinterpret_cast<const BYTE*>(module) + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
        nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 ||
		nt->OptionalHeader.SizeOfImage <= NET_ENGINE_REFERENCE - 0x00400000)
    {
        BS_ERROR("Unsupported ShardManager executable format");
        return false;
    }

    const bool valid =
        IsCommittedAddress(PROCESS_MESSAGE_POINTER, false) &&
        IsCommittedAddress(ORIGINAL_PROCESS_MESSAGE, true) &&
        IsCommittedAddress(ORIGINAL_HANDLE_MESSAGE, true) &&
        IsCommittedAddress(GET_MAIN_PROCESS_INSTANCE, true) &&
        IsCommittedAddress(NET_ENGINE_REFERENCE, false) &&
        IsCommittedAddress(BROADCAST_MESSAGE, true) &&
        IsCommittedAddress(ALLOCATE_MESSAGE, true);
    if (!valid)
        BS_ERROR("Unsupported or modified ShardManager runtime layout");
    return valid;
}

bool ShardManagerRuntimeSafety::InstallHooks()
{
    if (InterlockedCompareExchange(&s_hooksInstalled, 1, 0) != 0)
        return true;

    if (!ReadMemoryValue<DWORD>(PROCESS_MESSAGE_POINTER, s_originalProcessMessagePointer) ||
        !replaceAddr(PROCESS_MESSAGE_POINTER, addr_from_this(&CMainProcess::_OnProcessMessageSafe)))
    {
        InterlockedExchange(&s_hooksInstalled, 0);
        return false;
    }

    if (!CMainProcess::InitializeSafe())
    {
        replaceAddr(PROCESS_MESSAGE_POINTER, s_originalProcessMessagePointer);
        s_originalProcessMessagePointer = 0;
        InterlockedExchange(&s_hooksInstalled, 0);
        return false;
    }
    return true;
}

void ShardManagerRuntimeSafety::RollbackHooks()
{
    if (InterlockedExchange(&s_hooksInstalled, 0) == 0)
        return;
    CMainProcess::ShutdownSafe();
    if (s_originalProcessMessagePointer != 0)
        replaceAddr(PROCESS_MESSAGE_POINTER, s_originalProcessMessagePointer);
    s_originalProcessMessagePointer = 0;
}
