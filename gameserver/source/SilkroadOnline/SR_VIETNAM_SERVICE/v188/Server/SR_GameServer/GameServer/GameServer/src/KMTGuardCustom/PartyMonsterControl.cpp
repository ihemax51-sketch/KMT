#include "PartyMonsterControl.h"

#include <SettingMgr/NewSettings.h>
#include <BSObj/BSObj.h>
#include <Windows.h>
#include <cstring>

namespace
{
    const DWORD kMinimumMembersInstructionAddress = 0x00558F20;
    const DWORD kSpawnRateInstructionAddress = 0x005608E2;
    bool g_partyMonsterPatchApplied = false;
    BYTE g_originalMinimumMembers = 2;
    BYTE g_originalSpawnRate = 50;

    const BYTE kExpectedMinimumMembersInstruction[] =
    {
        0x83, 0x7C, 0x24, 0x08 // cmp dword ptr [esp+08], imm8
    };

    const BYTE kExpectedSpawnRateEdxInstruction[] =
    {
        0x83, 0xFA // cmp edx, imm8
    };

    const BYTE kExpectedSpawnRateEbpInstruction[] =
    {
        0x83, 0xFD // cmp ebp, imm8
    };

    bool IsReadableInstruction(const DWORD address, const BYTE* expected, const SIZE_T size)
    {
        MEMORY_BASIC_INFORMATION info = { 0 };
        if (VirtualQuery(reinterpret_cast<LPCVOID>(address), &info, sizeof(info)) == 0)
            return false;

        if (info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0)
            return false;

        const DWORD_PTR regionEnd = reinterpret_cast<DWORD_PTR>(info.BaseAddress) + info.RegionSize;
        if (static_cast<DWORD_PTR>(address) + size > regionEnd)
            return false;

        return std::memcmp(reinterpret_cast<const void*>(address), expected, size) == 0;
    }

    bool WriteImmediateByte(const DWORD address, const BYTE value)
    {
        BYTE* target = reinterpret_cast<BYTE*>(address);
        DWORD oldProtect = 0;
        if (!VirtualProtect(target, 1, PAGE_EXECUTE_READWRITE, &oldProtect))
            return false;

        *target = value;
        FlushInstructionCache(GetCurrentProcess(), target, 1);

        DWORD ignoredProtect = 0;
        return VirtualProtect(target, 1, oldProtect, &ignoredProtect) != FALSE;
    }

    bool ApplySettings(const BYTE minimumMembers, const BYTE spawnRate)
    {
        const bool minimumMembersInstructionSupported = IsReadableInstruction(
            kMinimumMembersInstructionAddress,
            kExpectedMinimumMembersInstruction,
            sizeof(kExpectedMinimumMembersInstruction));
        const bool spawnRateInstructionSupported =
            IsReadableInstruction(
                kSpawnRateInstructionAddress,
                kExpectedSpawnRateEdxInstruction,
                sizeof(kExpectedSpawnRateEdxInstruction)) ||
            IsReadableInstruction(
                kSpawnRateInstructionAddress,
                kExpectedSpawnRateEbpInstruction,
                sizeof(kExpectedSpawnRateEbpInstruction));

        if (!minimumMembersInstructionSupported || !spawnRateInstructionSupported)
        {
            return false;
        }

        const DWORD minimumImmediateAddress = kMinimumMembersInstructionAddress + 4;
        const DWORD spawnRateImmediateAddress = kSpawnRateInstructionAddress + 2;
        const BYTE originalMinimumMembers = *reinterpret_cast<const BYTE*>(minimumImmediateAddress);
        const BYTE originalSpawnRate = *reinterpret_cast<const BYTE*>(spawnRateImmediateAddress);
        g_originalMinimumMembers = originalMinimumMembers;
        g_originalSpawnRate = originalSpawnRate;

        if (!WriteImmediateByte(minimumImmediateAddress, minimumMembers))
            return false;

        if (!WriteImmediateByte(spawnRateImmediateAddress, spawnRate))
        {
            WriteImmediateByte(minimumImmediateAddress, originalMinimumMembers);
            return false;
        }

        if (*reinterpret_cast<const BYTE*>(minimumImmediateAddress) != minimumMembers ||
            *reinterpret_cast<const BYTE*>(spawnRateImmediateAddress) != spawnRate)
        {
            WriteImmediateByte(minimumImmediateAddress, originalMinimumMembers);
            WriteImmediateByte(spawnRateImmediateAddress, originalSpawnRate);
            return false;
        }

        g_partyMonsterPatchApplied = true;
        return true;
    }
}

bool PartyMonsterControl::Initialize()
{
    if (CNewSettings::m_Settings == NULL)
    {
        BS_INFO("PartyMonsterSpawn -> FAILED (settings are unavailable)");
        return false;
    }

    const bool enabled = CNewSettings::m_Settings->EnablePartyMonsterSpawn;
    const BYTE minimumMembers = static_cast<BYTE>(
        enabled ? CNewSettings::m_Settings->PartyMonsterMinimumMembers : 9);
    const BYTE spawnRate = static_cast<BYTE>(
        enabled ? CNewSettings::m_Settings->PartyMonsterSpawnRate : 0);

    if (!ApplySettings(minimumMembers, spawnRate))
    {
        BS_INFO("PartyMonsterSpawn -> FAILED (unsupported GameServer instructions at 0x00558F20 or 0x005608E2)");
        return false;
    }

    BS_INFO("PartyMonsterSpawn -> %s (minimum_members=%d, spawn_rate=%d)",
            enabled ? "ON" : "OFF",
            CNewSettings::m_Settings->PartyMonsterMinimumMembers,
            CNewSettings::m_Settings->PartyMonsterSpawnRate);
    return true;
}

void PartyMonsterControl::Shutdown()
{
    if (!g_partyMonsterPatchApplied)
        return;
    WriteImmediateByte(kSpawnRateInstructionAddress + 2, g_originalSpawnRate);
    WriteImmediateByte(kMinimumMembersInstructionAddress + 4, g_originalMinimumMembers);
    g_partyMonsterPatchApplied = false;
}
