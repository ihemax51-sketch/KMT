#include "CGame_Hook.h"
#include "Hooks.h"
#include "../ClientStartupCompatibility.h"
#include <support/hook.h>
#include <vector>

namespace
{
    const DWORD kInitGameAssetsCallSite = 0x00832A11;
    const DWORD kOriginalInitGameAssetsTarget = 0x00849110;
    const DWORD kInitializationWaitMilliseconds = 120000;
    const DWORD kOnActionEventJumpBack = 0x00793B06;
}

extern std::vector<void_cgame_handler_t> hooks_lgo_pre;
extern std::vector<void_cgame_handler_t> hooks_lgo_post;

extern std::vector<void_cgame_handler_t> hooks_iga_pre;
extern std::vector<void_cgame_handler_t> hooks_iga_post;

void CGame_Hook::LoadGameOption() {
    for (std::vector<void_cgame_handler_t>::iterator it = hooks_lgo_pre.begin(); it != hooks_lgo_pre.end(); ++it) {
        (*it)(this);
    }

    reinterpret_cast<void (__thiscall *)(CGame_Hook *)>(0x008469c0)(this);

    for (std::vector<void_cgame_handler_t>::iterator it = hooks_lgo_post.begin(); it != hooks_lgo_post.end(); ++it) {
        (*it)(this);
    }
}

void CGame_Hook::InitGameAssets_Impl() {
    if (!WaitForClientInitialization(kInitializationWaitMilliseconds)) {
        WriteClientStartupDiagnostic(
            "Game asset initialization stopped because KMTGuard initialization did not complete.");
        TerminateProcess(GetCurrentProcess(), ERROR_DLL_INIT_FAILED);
        return;
    }

    for (std::vector<void_cgame_handler_t>::iterator it = hooks_iga_pre.begin(); it != hooks_iga_pre.end(); ++it) {
        (*it)(this);
    }

    CGame::InitGameAssets();

    for (std::vector<void_cgame_handler_t>::iterator it = hooks_iga_post.begin(); it != hooks_iga_post.end(); ++it) {
        (*it)(this);
    }
}

bool InstallInitGameAssetsBootstrapGate()
{
    BYTE* const callSite = reinterpret_cast<BYTE*>(kInitGameAssetsCallSite);
    if (callSite[0] != 0xE8)
        return false;

    const DWORD hookTarget = static_cast<DWORD>(
        addr_from_this(&CGame_Hook::InitGameAssets_Impl));
    const LONG currentDisplacement = *reinterpret_cast<const LONG*>(callSite + 1);
    const DWORD currentTarget = kInitGameAssetsCallSite + 5 + currentDisplacement;

    if (currentTarget == hookTarget)
        return true;
    if (currentTarget != kOriginalInitGameAssetsTarget)
        return false;

    const LONG hookDisplacement = static_cast<LONG>(
        hookTarget - kInitGameAssetsCallSite - 5);
    DWORD oldProtection = 0;
    if (!VirtualProtect(callSite + 1, sizeof(hookDisplacement), PAGE_EXECUTE_READWRITE, &oldProtection))
        return false;

    memcpy(callSite + 1, &hookDisplacement, sizeof(hookDisplacement));
    FlushInstructionCache(GetCurrentProcess(), callSite, 5);

    DWORD ignoredProtection = 0;
    return VirtualProtect(
        callSite + 1,
        sizeof(hookDisplacement),
        oldProtection,
        &ignoredProtection) != FALSE;
}

void CGame_Hook::OnActionEvent(int actionWndID)
{
    // ActionWnd IDs below 9000 belong to the native client and must keep
    // their original behavior without generating a custom packet.
    if (actionWndID < 9000)
        return;

    CMsgStreamBuffer buf(0xCC1E);
    buf << actionWndID;
    SendMsg(buf);
}

void __declspec(naked) CGame_Hook_Naked_OnActionEvent()
{
    __asm
    {
        // Preserve the original execution context while dispatching the
        // custom ActionWnd event.
        pushad
        pushfd

        // ActionWndID from the original stack frame.
        mov eax, [esp + 0x2C]
        push eax
        call CGame_Hook::OnActionEvent
        add esp, 4

        popfd
        popad

        // Restore the instructions replaced by the five-byte hook.
        push ebp
        mov ebp, esp
        and esp, 0xFFFFFFF8

        jmp dword ptr [kOnActionEventJumpBack]
    }
}
