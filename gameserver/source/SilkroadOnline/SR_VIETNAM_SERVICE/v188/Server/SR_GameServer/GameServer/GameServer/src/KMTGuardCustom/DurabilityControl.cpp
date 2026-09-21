#include "DurabilityControl.h"

#include <SettingMgr/NewSettings.h>
#include <BSObj/BSObj.h>
#include <Windows.h>
#include <cstring>
#include <KMTGuardCustom/GameServerRuntimeSafety.h>

namespace
{
    const DWORD kDurabilityUpdateAddress = 0x00496E34;
    const DWORD kDurabilityUpdateReturn = 0x00496E3B;
    const BYTE kExpectedInstructions[] =
    {
        0x83, 0x4E, 0x08, 0x04, // or dword ptr [esi+08], 04
        0x89, 0x7E, 0x38        // mov [esi+38], edi
    };

    DWORD g_durabilityUpdateReturn = kDurabilityUpdateReturn;
    bool g_durabilityHookInstalled = false;

#if defined(_M_IX86)
    void __declspec(naked) DisableDurabilityHook()
    {
        __asm
        {
            // EBX is the previous durability and EDI is the proposed value.
            // Keep increases (repair/stones), but replace decreases with the
            // previous value before the original write is performed.
            cmp ebx, edi
            jb allow_increase
            mov edi, ebx

        allow_increase:
            or dword ptr [esi + 0x08], 0x04
            mov dword ptr [esi + 0x38], edi
            jmp dword ptr [g_durabilityUpdateReturn]
        }
    }
#endif

    bool IsTargetMemoryReady()
    {
        MEMORY_BASIC_INFORMATION info = { 0 };
        if (VirtualQuery(reinterpret_cast<LPCVOID>(kDurabilityUpdateAddress), &info, sizeof(info)) == 0)
            return false;

        if (info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0)
            return false;

        return std::memcmp(
            reinterpret_cast<const void*>(kDurabilityUpdateAddress),
            kExpectedInstructions,
            sizeof(kExpectedInstructions)) == 0;
    }

    bool InstallHook()
    {
#if !defined(_M_IX86)
        return false;
#else
        return IsTargetMemoryReady() && GameServerRuntimeSafety::InstallRelativeJump(
            kDurabilityUpdateAddress,
            kExpectedInstructions,
            sizeof(kExpectedInstructions),
            reinterpret_cast<const void*>(&DisableDurabilityHook),
            "durability hook");
#endif
    }
}

bool DurabilityControl::Initialize()
{
    if (CNewSettings::m_Settings == NULL || !CNewSettings::m_Settings->DisableDurability)
    {
        BS_INFO("DisableDurability -> OFF");
        return true;
    }

    if (!InstallHook())
    {
        BS_INFO("DisableDurability -> FAILED (unsupported GameServer instructions at 0x00496E34)");
        return false;
    }

    g_durabilityHookInstalled = true;
    BS_INFO("DisableDurability -> ON");
    return true;
}

void DurabilityControl::Shutdown()
{
    if (!g_durabilityHookInstalled)
        return;
    GameServerRuntimeSafety::RestoreBytes(
        kDurabilityUpdateAddress, NULL, kExpectedInstructions,
        sizeof(kExpectedInstructions), "durability hook");
    g_durabilityHookInstalled = false;
}
