#include "TradeGoldControl.h"

#include <SettingMgr/NewSettings.h>
#include <BSObj/BSObj.h>
#include <KMTGuardCustom/GameServerRuntimeSafety.h>

#include <Windows.h>
#include <climits>
#include <cstring>

namespace
{
    const DWORD kTradeProfitScaleCallAddress = 0x004C8DBC;

    // The surrounding instructions belong to the trade-goods profit path in
    // the supported v188 SR_GameServer. Only the five-byte CALL is replaced.
    const BYTE kExpectedTradeProfitContext[] =
    {
        0x56, 0x8B, 0xD8, 0x8B, 0x44, 0x24, 0x28, 0x57,
        0xE8, 0xCF, 0xD5, 0xFB, 0xFF,
        0x8B, 0x4C, 0x24, 0x34, 0x89, 0x01, 0x8B, 0xC7,
        0x2B, 0x01, 0x89, 0x51, 0x04, 0x8B, 0xD6, 0x1B, 0x51, 0x04
    };
    const DWORD kTradeProfitContextAddress = 0x004C8DB4;
    const BYTE kOriginalTradeProfitCall[] = { 0xE8, 0xCF, 0xD5, 0xFB, 0xFF };

    // The original scale helper removes its two DWORD arguments with ret 8.
    const DWORD kScaleHelperEpilogueAddress = 0x004863B0;
    const BYTE kExpectedScaleHelperEpilogue[] = { 0x59, 0xC2, 0x08, 0x00 };

    BYTE g_installedTradeProfitCall[sizeof(kOriginalTradeProfitCall)] = { 0 };
    bool g_tradeProfitHookInstalled = false;

#if defined(_M_IX86)
    __int64 __declspec(naked) __stdcall ReturnUnscaledTradeValue(__int64)
    {
        __asm
        {
            // The caller subtracts the scale-helper result from this same
            // input value. Returning it unchanged makes that final profit
            // exactly zero while preserving the original caller's flow.
            mov eax, dword ptr [esp + 4]
            mov edx, dword ptr [esp + 8]
            ret 8
        }
    }
#endif

    bool BuildRelativeCall(BYTE (&replacement)[5])
    {
#if !defined(_M_IX86)
        return false;
#else
        const INT_PTR target = reinterpret_cast<INT_PTR>(&ReturnUnscaledTradeValue);
        const INT_PTR displacement = target - static_cast<INT_PTR>(kTradeProfitScaleCallAddress + 5);
        if (displacement < INT_MIN || displacement > INT_MAX)
            return false;

        replacement[0] = 0xE8;
        const int relativeOffset = static_cast<int>(displacement);
        std::memcpy(replacement + 1, &relativeOffset, sizeof(relativeOffset));
        return true;
#endif
    }

    bool InstallHook()
    {
        if (!GameServerRuntimeSafety::MatchesBytes(
                kTradeProfitContextAddress,
                kExpectedTradeProfitContext,
                sizeof(kExpectedTradeProfitContext)) ||
            !GameServerRuntimeSafety::MatchesBytes(
                kScaleHelperEpilogueAddress,
                kExpectedScaleHelperEpilogue,
                sizeof(kExpectedScaleHelperEpilogue)))
            return false;

        BYTE replacement[sizeof(kOriginalTradeProfitCall)] = { 0 };
        if (!BuildRelativeCall(replacement))
            return false;

        GameServerMemoryPatchTransaction transaction;
        if (!transaction.WriteRaw(
                kTradeProfitScaleCallAddress,
                replacement,
                sizeof(replacement),
                "original trade-goods gold calculation") ||
            !transaction.Commit())
            return false;

        std::memcpy(g_installedTradeProfitCall, replacement, sizeof(replacement));
        g_tradeProfitHookInstalled = true;
        return true;
    }
}

bool TradeGoldControl::Initialize()
{
    if (CNewSettings::m_Settings == NULL)
    {
        BS_INFO("DisableOriginalTradeGold -> FAILED (settings are unavailable)");
        return false;
    }

    if (!CNewSettings::m_Settings->DisableOriginalTradeGold)
    {
        BS_INFO("DisableOriginalTradeGold -> OFF");
        return true;
    }

    if (!InstallHook())
    {
        BS_INFO("DisableOriginalTradeGold -> FAILED (unsupported GameServer instructions at 0x004C8DBC)");
        return false;
    }

    BS_INFO("DisableOriginalTradeGold -> ON");
    return true;
}

void TradeGoldControl::Shutdown()
{
    if (!g_tradeProfitHookInstalled)
        return;

    GameServerRuntimeSafety::RestoreBytes(
        kTradeProfitScaleCallAddress,
        g_installedTradeProfitCall,
        kOriginalTradeProfitCall,
        sizeof(kOriginalTradeProfitCall),
        "original trade-goods gold calculation");
    g_tradeProfitHookInstalled = false;
}
