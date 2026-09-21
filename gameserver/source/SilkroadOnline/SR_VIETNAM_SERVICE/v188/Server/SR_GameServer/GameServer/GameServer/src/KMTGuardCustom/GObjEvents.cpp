#include "GObjEvents.h"
#include "GObjPC.h"
#include <windows.h>
#include <KMTGuardCustom/GameServerRuntimeSafety.h>

DWORD OnLatestRegionChange_JumpBack = 0x00485AA9;
namespace
{
    bool s_regionChangeHookInstalled = false;
    const BYTE kRegionChangeOriginal[] =
        { 0x89, 0x85, 0x84, 0x00, 0x00, 0x00 };
}

void CGObjEvents::CheckRegionNeedChange(CGObj *obj, short region) {
    try {
        if (!obj) {
            return;
        }
        if (obj->IsPC()) { // checks 0x0000 offset equals to 0x00AF59FC
            CGObjPC *pc = reinterpret_cast<CGObjPC *>(obj);

            // pc's live regionid offset is 0x0084
            if (pc->GetCurrentPosition().wRegionID != region) {
                // check and change cape here
                CMsg* pMsgg = pc->AllocMsg(0x3571);
                if (pMsgg == NULL)
                    return;
                *pMsgg << region;
                pc->SendMsg(pMsgg);
            }
        }
    } catch (...) {}
}

void __declspec(naked) CGObjEvents::Naked_OnLatestRegionChange() {
    __asm {
    pushad
    pushfd

    push eax
    push ebp
    call CGObjEvents::CheckRegionNeedChange
    pop ebp
    pop eax

    popfd
    popad

    mov dword ptr ss:[ebp+0x84], eax
    jmp dword ptr [OnLatestRegionChange_JumpBack]
    }
}

bool CGObjEvents::Initialize() {
    s_regionChangeHookInstalled = GameServerRuntimeSafety::InstallRelativeJump(
        0x00485AA3,
        kRegionChangeOriginal,
        sizeof(kRegionChangeOriginal),
        reinterpret_cast<const void*>(&CGObjEvents::Naked_OnLatestRegionChange),
        "region-change hook");
    return s_regionChangeHookInstalled;
}

void CGObjEvents::Shutdown()
{
    if (!s_regionChangeHookInstalled)
        return;
    GameServerRuntimeSafety::RestoreBytes(
        0x00485AA3, NULL, kRegionChangeOriginal,
        sizeof(kRegionChangeOriginal), "region-change hook");
    s_regionChangeHookInstalled = false;
}
