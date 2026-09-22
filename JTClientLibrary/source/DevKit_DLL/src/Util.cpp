#include "Util.h"

#include <sys/stat.h>
#include <stdio.h>

#include "support/hook.h"
#include "ClientStartupCompatibility.h"

#include "hooks/Hooks.h"
#include "hooks/GFXVideo3d_Hook.h"
#include "hooks/CGame_Hook.h"

#include "hooks/WndProc_Hook.h"

#include <GInterface.h>
#include <IFChatViewer.h>
#include <NetProcessIn.h>
#include <NetProcessSecond.h>
#include <NetProcessThird.h>
#include <BSLib/Debug.h>
#include "QuickStart.h"
#include "ICPlayer.h"
#include "ICMonster.h"
#include <PSCharacterSelect.h>
#include <ICUser.h>
#include <GFX3DFunction/RStateMgr.h>
#include <AutoPotion.h>
#include <CustomData/CustomSettingManager.h>
#include <SRIFLib/NIFWorldMap.h>
#include <PSTitle.h>
#include <CObjCharacter.h>
#include <IFPartyMatch.h>
#include <IFMessageBox.h>
#include <IFStallSlot.h>
#include <IFItemMallConfirmBuy.h>
#include <IFGhaCha.h>
#include <IFGhachaSelectWnd.h>
#include <DropItemManager.h>
#include <SRIFLib/NIFEnchantWnd.h>
#include <NIFAlchemySubWndType1.h>
#include <IFTargetWindow.h>
#include <IFCOS.h>
#include <IFCOSManager.h>
#include <IFPetMiniInfo.h>
#include <ICScriptObjManager.h>
#include <ICCos.h>
#include <GFXMainFrame/GWndBase.h>
#include <PSVersionCheck.h>
#include <IFPlayerMiniInfo.h>
#include <DiscordRichPresence/DiscordManager.h>
#include <NavMesh/RTNavMeshTerrain.h>
#include <Macro/IFMacroMenuAutoHunt.h>
#include <GFX3DFunction/DrawingHelpers.h>
#include <IFItemMallShopSlot.h>
#include <InterfaceNetSender.h>
#include <IFWnd.h>
#include "KmtGuardClientVersion.h"
#include <support/MemberFunctionHook.h>


std::vector<const CGfxRuntimeClass *> register_objects;
std::vector<overrideFnPtr> override_objects;

QuickStart quickstart;

namespace {

void ApplyMagicPopAnimationSpeedPatch()
{
    unsigned char *address = reinterpret_cast<unsigned char *>(0x00745A63);
    const unsigned char command[] = {0x68, 0x64, 0x00, 0x00, 0x00};
    MEMORY_BASIC_INFORMATION region = {0};
    if (VirtualQuery(address, &region, sizeof(region)) != sizeof(region) ||
        region.State != MEM_COMMIT || region.Type != MEM_IMAGE ||
        region.AllocationBase != GetModuleHandle(NULL) ||
        (region.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0 ||
        (region.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
                           PAGE_EXECUTE_WRITECOPY)) == 0 ||
        reinterpret_cast<DWORD>(address) + sizeof(command) >
            reinterpret_cast<DWORD>(region.BaseAddress) + region.RegionSize) {
        WriteClientStartupDiagnostic("Magic POP animation patch skipped: incompatible memory region.");
        return;
    }

    // User-supplied client patch: PUSH 100. This opcode check is a basic
    // compatibility guard, not a full signature verification of the client.
    if (address[0] != 0x68) {
        WriteClientStartupDiagnostic("Magic POP animation patch skipped: expected PUSH imm32.");
        return;
    }
    if (memcmp(address, command, sizeof(command)) == 0)
        return;

    DWORD oldProtect = 0;
    if (!VirtualProtect(address, sizeof(command), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        WriteClientStartupDiagnostic("Magic POP animation patch skipped: VirtualProtect failed.");
        return;
    }
    memcpy(address, command, sizeof(command));
    const BOOL cacheFlushed = FlushInstructionCache(GetCurrentProcess(), address, sizeof(command));
    DWORD ignoredProtect = 0;
    const BOOL protectionRestored = VirtualProtect(address, sizeof(command), oldProtect, &ignoredProtect);
    WriteClientStartupDiagnostic(cacheFlushed && protectionRestored
        ? "Magic POP animation patch applied: PUSH 100."
        : "Magic POP animation patch written: cache flush or protection restore failed.");
}

void WriteSetupOperation(
    const char* state,
    const char* operation,
    int sourceLine,
    DWORD firstAddress,
    DWORD secondAddress)
{
    char message[640] = {0};
    _snprintf(
        message,
        sizeof(message) - 1,
        "Setup stage %s: %s line=%d address=0x%08lX target=0x%08lX thread=%lu.",
        state,
        operation,
        sourceLine,
        firstAddress,
        secondAddress,
        GetCurrentThreadId());
    message[sizeof(message) - 1] = '\0';
    WriteClientStartupDiagnostic(message);
}

LONG LogSetupException(
    const char* operation,
    int sourceLine,
    EXCEPTION_POINTERS* exceptionPointers)
{
    DWORD exceptionCode = 0;
    DWORD exceptionAddress = 0;
    if (exceptionPointers != NULL &&
        exceptionPointers->ExceptionRecord != NULL) {
        exceptionCode = exceptionPointers->ExceptionRecord->ExceptionCode;
        exceptionAddress = reinterpret_cast<DWORD>(
            exceptionPointers->ExceptionRecord->ExceptionAddress);
    }

    char message[640] = {0};
    _snprintf(
        message,
        sizeof(message) - 1,
        "Setup stage exception: %s line=%d code=0x%08lX address=0x%08lX thread=%lu.",
        operation,
        sourceLine,
        exceptionCode,
        exceptionAddress,
        GetCurrentThreadId());
    message[sizeof(message) - 1] = '\0';
    WriteClientStartupDiagnostic(message);
    return EXCEPTION_EXECUTE_HANDLER;
}

bool DiagnosticApplyRegisteredMemberHooks(int sourceLine)
{
    const char* operation = "Core/ApplyRegisteredMemberHooks";
    WriteSetupOperation("begin", operation, sourceLine, 0, 0);
    __try {
        if (!ApplyRegisteredMemberHooks())
            return false;
    }
    __except (LogSetupException(operation, sourceLine, GetExceptionInformation())) {
        return false;
    }
    WriteSetupOperation("end", operation, sourceLine, 0, 0);
    return true;
}

bool DiagnosticPlaceHook(
    const char* functionName,
    int sourceLine,
    int address,
    int target)
{
    WriteSetupOperation("begin", functionName, sourceLine, address, target);
    __try {
        if (!placeHook(address, target))
            return false;
    }
    __except (LogSetupException(functionName, sourceLine, GetExceptionInformation())) {
        return false;
    }
    WriteSetupOperation("end", functionName, sourceLine, address, target);
    return true;
}

bool DiagnosticReplaceOffset(
    const char* functionName,
    int sourceLine,
    int address,
    int target)
{
    WriteSetupOperation("begin", functionName, sourceLine, address, target);
    __try {
        if (!replaceOffset(address, target))
            return false;
    }
    __except (LogSetupException(functionName, sourceLine, GetExceptionInformation())) {
        return false;
    }
    WriteSetupOperation("end", functionName, sourceLine, address, target);
    return true;
}

bool DiagnosticReplaceAddress(
    const char* functionName,
    int sourceLine,
    int address,
    int target)
{
    WriteSetupOperation("begin", functionName, sourceLine, address, target);
    __try {
        if (!replaceAddr(address, target))
            return false;
    }
    __except (LogSetupException(functionName, sourceLine, GetExceptionInformation())) {
        return false;
    }
    WriteSetupOperation("end", functionName, sourceLine, address, target);
    return true;
}

bool DiagnosticVftableHook(
    const char* functionName,
    int sourceLine,
    unsigned int address,
    int index,
    int target)
{
    WriteSetupOperation("begin", functionName, sourceLine, address, target);
    __try {
        if (!vftableHook(address, index, target))
            return false;
    }
    __except (LogSetupException(functionName, sourceLine, GetExceptionInformation())) {
        return false;
    }
    WriteSetupOperation("end", functionName, sourceLine, address, target);
    return true;
}

bool DiagnosticRenderNop(
    const char* functionName,
    int sourceLine,
    void* address,
    int count)
{
    WriteSetupOperation(
        "begin",
        functionName,
        sourceLine,
        reinterpret_cast<DWORD>(address),
        count);
    bool result = false;
    __try {
        result = RenderNop(address, count);
    }
    __except (LogSetupException(functionName, sourceLine, GetExceptionInformation())) {
        return false;
    }
    WriteSetupOperation(
        result ? "end" : "failed",
        functionName,
        sourceLine,
        reinterpret_cast<DWORD>(address),
        count);
    return result;
}

bool ValidateClientImage() {
    const DWORD expectedBase = 0x00400000;
    const DWORD expectedImageSize = 0x00D70000;
    const DWORD expectedTimestamp = 0x4E311CB6;
    HMODULE module = GetModuleHandle(NULL);

    if (module == NULL || reinterpret_cast<DWORD>(module) != expectedBase) {
        OutputDebugStringA("[KMTGuardKit] Unsupported client image base; hooks were not installed.\n");
        return false;
    }

    IMAGE_DOS_HEADER *dosHeader = reinterpret_cast<IMAGE_DOS_HEADER *>(module);
    if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
        OutputDebugStringA("[KMTGuardKit] Invalid DOS header; hooks were not installed.\n");
        return false;
    }

    IMAGE_NT_HEADERS *ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS *>(
        reinterpret_cast<BYTE *>(module) + dosHeader->e_lfanew);
    if (ntHeaders->Signature != IMAGE_NT_SIGNATURE ||
        ntHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        OutputDebugStringA("[KMTGuardKit] Unsupported PE image; hooks were not installed.\n");
        return false;
    }

    if (ntHeaders->FileHeader.TimeDateStamp != expectedTimestamp ||
        ntHeaders->OptionalHeader.SizeOfImage != expectedImageSize) {
        OutputDebugStringA("[KMTGuardKit] Client image fingerprint is not the supported v188 layout.\n");
        return false;
    }

    return true;
}

} // namespace

#define ApplyRegisteredMemberHooks() \
    do { if (!DiagnosticApplyRegisteredMemberHooks(__LINE__)) return false; } while (0)
#define placeHook(address, target) \
    do { if (!DiagnosticPlaceHook("placeHook " #target, __LINE__, (int)(address), (int)(target))) return false; } while (0)
#define replaceOffset(address, target) \
    do { if (!DiagnosticReplaceOffset("replaceOffset " #target, __LINE__, (int)(address), (int)(target))) return false; } while (0)
#define replaceAddr(address, target) \
    do { if (!DiagnosticReplaceAddress("replaceAddr " #target, __LINE__, (int)(address), (int)(target))) return false; } while (0)
#define vftableHook(address, index, target) \
    do { if (!DiagnosticVftableHook("vftableHook " #target, __LINE__, (unsigned int)(address), (int)(index), (int)(target))) return false; } while (0)
#define RenderNop(address, count) \
    do { if (!DiagnosticRenderNop("RenderNop", __LINE__, (void*)(address), (int)(count))) return false; } while (0)

bool Setup() {
    if (!ValidateClientImage())
        return false;

    ApplyRegisteredMemberHooks();

    // mBot can expose a native CObjChild teardown ordering edge case where the
    // final, redundant list clear observes a sentinel invalidated by the
    // immediately preceding parent detach. Guard only that final call site.
    if (!InstallCObjChildTeardownCompatibilityGuard())
        return false;

    vftableHook(0x00E0963C, 17, addr_from_this(&CGFXVideo3D_Hook::CreateThingsHook));
    vftableHook(0x00E0963C, 26, addr_from_this(&CGFXVideo3D_Hook::EndSceneHook));
    vftableHook(0x00E0963C, 20, addr_from_this(&CGFXVideo3D_Hook::SetSizeHook));

    vftableHook(0x00db95a4, 10, addr_from_this(&CGInterface::OnCreateIMPL));
    vftableHook(0x00db95a4, 5, addr_from_this(&CGInterface::OnTimerIMPL));

    replaceAddr(0x00d71100 + 1, addr_from_this(&CGInterface::OnCharIMPL));

    vftableHook(0x00de2e7c, 15, addr_from_this(&CICUser::Func_15_impl));
    vftableHook(0x00de256c, 15, addr_from_this(&CICharactor::Func_15_impl));
    vftableHook(0x00de2c24, 15, addr_from_this(&CICPlayer::Func_15_impl));
    vftableHook(0x00de26c4, 15, addr_from_this(&CICMonster::Func_15_impl));
    vftableHook(0x00de211c, 15, addr_from_this(&CICCos::Func_15_impl));
    replaceOffset(0x009D0221, addr_from_this(&CICPlayer::UpdateNameColor));//charname color hook


    replaceAddr(0x00831337 + 4, (int) &WndProcHook);

    placeHook(0x0065c6f0, addr_from_this(&CAlramGuideMgrWnd::CreateGuideIcon));

    placeHook(0x00793B00, addr_from_this(&CGame_Hook_Naked_OnActionEvent));

    replaceOffset(0x008491d1, addr_from_this(&CGame_Hook::LoadGameOption));

    replaceOffset(0x00832a11, addr_from_this(&CGame_Hook::InitGameAssets_Impl));

    replaceOffset(0x0084c9bf, addr_from_this(&CNetProcessIn::RegisterPacketHandlers));
    replaceOffset(0x00898656, addr_from_this(&CNetProcessSecond::RegisterPacketHandlers));
    replaceOffset(0x008a4876, addr_from_this(&CNetProcessThird::RegisterPacketHandlers));

  //  replaceOffset(0x009ded0d, addr_from_this(&CRStateMgr::FUN_00470060));

    replaceOffset(0x0060bbbf, addr_from_this(&CNIFUnderMenuBar::UseSlot));
   // replaceAddr(0x00d69d8e + 1, addr_from_this(&CNIFUnderMenuBar::PressButtonAutoPotion));


 /// TODO NEWS
    static const char *resinfoplace = "clientlibrary\\resinfo\\ginterface.txt";
    replaceAddr(0x0078c9ce+1, (int)(resinfoplace));
    /// TODO UNIQUE HISTORY
    replaceOffset(0x006243e5, addr_from_this(&CNIFWorldMap::OnWMRenderMySelf));
    vftableHook(0x00d99b94, 5, addr_from_this(&CNIFWorldMap::OnTimerIMPL));
    vftableHook(0x00d99b94, 10, addr_from_this(&CNIFWorldMap::OnWMCreate));
    vftableHook(0x00d99b94, 3, addr_from_this(&CNIFWorldMap::MessageMap));
    vftableHook(0x00d99b94, 17, addr_from_this(&CNIFWorldMap::OnMouseActions));
    replaceAddr(0x00d99c40, addr_from_this(&CNIFWorldMap::OnCloseWndIMPL));

   /* replaceOffset(0x00625814, addr_from_this(&CNIFWorldMap::FUN_00622b60));

    replaceOffset(0x006230d4, addr_from_this(&CNIFWorldMap::GetRegionTypeMaybe));

    replaceOffset(0x00624442, addr_from_this(&CNIFWorldMap::GetRegionTypeMaybe));

    replaceOffset(0x006245d6, addr_from_this(&CNIFWorldMap::GetRegionTypeMaybe));

    replaceOffset(0x006257f6, addr_from_this(&CNIFWorldMap::GetRegionTypeMaybe));

    replaceOffset(0x00621c7f, addr_from_this(&CNIFWorldMap::FUN_006217e0));
    replaceOffset(0x00622f78, addr_from_this(&CNIFWorldMap::FUN_006217e0));*/
    //replaceOffset(0x006231a4, addr_from_this(&CNIFWorldMap::FUN_006217e0));
    replaceOffset(0x006238fd, addr_from_this(&CNIFWorldMap::FUN_006217e0));
  //  replaceOffset(0x00624509, addr_from_this(&CNIFWorldMap::FUN_006217e0));

   // replaceOffset(0x00628ac5, addr_from_this(&CNIFWorldMap::FUN_006217e0));
  //  replaceOffset(0x00628b94, addr_from_this(&CNIFWorldMap::FUN_006217e0));


    //   replaceAddr(0x00d9ae70, addr_from_this(&CIFEdit::RenderMyself_IMPL));


    // Emoji


  /*  replaceOffset(0x00779145, addr_from_this(&CGInterface::FUN_00777a70));
    replaceOffset(0x00779242, addr_from_this(&CGInterface::FUN_00777a70));
    replaceOffset(0x0077b528, addr_from_this(&CGInterface::FUN_00777a70));
    replaceOffset(0x0077c3ff, addr_from_this(&CGInterface::FUN_00777a70));
    replaceOffset(0x0078cf64, addr_from_this(&CGInterface::FUN_00777a70));
    replaceOffset(0x009d49db, addr_from_this(&CGInterface::FUN_00777a70));
    replaceOffset(0x009d5a5f, addr_from_this(&CGInterface::FUN_00777a70));
*/


    /// TODO PS TİTLE
    static const char *pstitle = "clientlibrary\\resinfo\\pstitle.txt";
    replaceAddr(0x0086b32a+1, (int)(pstitle));

    replaceAddr(0x00dd92d4, addr_from_this(&CPSTitle::OnServerPacketRecv));
    replaceAddr(0x00dd92e4, addr_from_this(&CPSTitle::OnCreateIMPL));
    vftableHook(0x00dd92bc, 12, addr_from_this(&CPSTitle::OnUpdateIMPL));
    replaceAddr(0x00d758c3+1, addr_from_this(&CPSTitle::PressButtonServerList));

    replaceAddr(0x00d758cd+1, addr_from_this(&CPSTitle::PressConnectButton));
    replaceOffset(0x0086ae77, addr_from_this(&CPSTitle::PressConnectButton));

    /// CCObjCharacter
    replaceOffset(0x00a61412, addr_from_this(&CCObjCharacter::FUN_00a5fe00));

    replaceOffset(0x00a5e0c9, addr_from_this(&CCObjCharacter::FUN_00a5cca0));
 ///Helper Bble
    replaceOffset(0x0067984d, addr_from_this(&CIFHelperBubbleWindow::WriteLine));
    //Gold 0x00678e1f
    replaceOffset(0x00678e1f, addr_from_this(&CIFHelperBubbleWindow::WriteLine));
    //Silk
    replaceOffset(0x00678ee8, addr_from_this(&CIFHelperBubbleWindow::WriteLine));
    //Silk
    replaceOffset(0x00678fb1, addr_from_this(&CIFHelperBubbleWindow::WriteLine));
    //PSilk
    replaceOffset(0x00679143, addr_from_this(&CIFHelperBubbleWindow::WriteLine));
    //GP
    replaceOffset(0x0067907a, addr_from_this(&CIFHelperBubbleWindow::WriteLine));
    //Point
    replaceOffset(0x0067920c, addr_from_this(&CIFHelperBubbleWindow::WriteLine));
    //TokenSD1
    replaceOffset(0x006792d5, addr_from_this(&CIFHelperBubbleWindow::WriteLine));
    //TokenSD2
    replaceOffset(0x0067939d, addr_from_this(&CIFHelperBubbleWindow::WriteLine));
    //TokenSD3
    replaceOffset(0x00679469, addr_from_this(&CIFHelperBubbleWindow::WriteLine));
    //TokenSD4
    replaceOffset(0x00679535, addr_from_this(&CIFHelperBubbleWindow::WriteLine));
    //ArenaCoin
    replaceOffset(0x00679601, addr_from_this(&CIFHelperBubbleWindow::WriteLine));
    //
    replaceOffset(0x00678ced, addr_from_this(&CIFHelperBubbleWindow::WriteLine));


    /// AUTO SKILL

    vftableHook(0x00da5b44, 3, addr_from_this(&CIFSkillBoard::MessageMap));
    vftableHook(0x00da5b44, 5, addr_from_this(&CIFSkillBoard::OnTimerIMPL));
    vftableHook(0x00da5b44, 10, addr_from_this(&CIFSkillBoard::OnCreateIMPL));
    replaceOffset(0x0069d981, addr_from_this(&CIFSkillBoard::SelectMastery));

    static const char *equip = "clientlibrary\\resinfo\\ifequipment.txt";
    replaceAddr(0x006ac1fa+1, (int)(equip));

    static const char *inventory = "clientlibrary\\resinfo\\ifinventory.txt";
    replaceAddr(0x006b244b+1, (int)(inventory));

    // Default to the current design until the startup settings packet arrives.
    // The packet handler reapplies either the new or exact native coordinates
    // before the gameplay interface (and CIFEquipment) is created.
    ConfigureInventoryDesignPatches(true);

    vftableHook(0x00da6edc, 3, addr_from_this(&CIFEquipment::MessageMap));
    vftableHook(0x00da6edc, 10, addr_from_this(&CIFEquipment::OnCreateIMPL));
    vftableHook(0x00da6edc, 12, addr_from_this(&CIFEquipment::OnUpdateIMPL));


    /// CHARACTER SELECT


    replaceAddr(0x00d7110a + 1, addr_from_this(&CGInterface::OnKeyDown));


    replaceAddr(0x00dd8134, addr_from_this(&CPSCharacterSelect::OnServerPacketRecv));
    replaceAddr(0x00dd8160, addr_from_this(&CPSCharacterSelect::FUN_0085ddb0));

    vftableHook(0x00dd811c, 3, addr_from_this(&CPSCharacterSelect::MessageMap));

    replaceAddr(0x00d74e9d+1, addr_from_this(&CPSCharacterSelect::FUN_008548b0));

    vftableHook(0x00dd811c, 10, addr_from_this(&CPSCharacterSelect::OnCreateIMPL));
    replaceOffset(0x0085DDE1, addr_from_this(&CPSCharacterSelect::FUN_0085B680));

    replaceOffset(0x0085C488, addr_from_this(&CPSCharacterSelect::OnDisappear));

    static const char *charsel = "clientlibrary\\resinfo\\pscharacterselect.txt";
    replaceAddr(0x0085eb82+1, (int)(charsel));


    ///SECOND BAR
    replaceOffset(0x007872cb, addr_from_this(&CGInterface::Fun_00777f00));
    replaceOffset(0x0068213e, addr_from_this(&CNIFUnderMenuBar::GetSlotUseItem));
    replaceOffset(0x0088c684, addr_from_this(&CNIFUnderMenuBar::Fun_0060a940));


    replaceOffset(0x0087b613, addr_from_this(&CNIFUnderMenuBar::FindAndUpdate));// 32
    replaceOffset(0x0088bd53, addr_from_this(&CNIFUnderMenuBar::FindAndUpdate));// 32
    replaceOffset(0x0088c20b, addr_from_this(&CNIFUnderMenuBar::FindAndUpdate));// 32


    replaceOffset(0x00778851, addr_from_this(&CNIFUnderMenuBar::UpdateSkillSlot));// 32
    replaceOffset(0x006aed1b, addr_from_this(&CNIFUnderMenuBar::UpdateSkillSlot));// 32
    replaceOffset(0x00593fd6, addr_from_this(&CNIFUnderMenuBar::UpdateSkillSlot));// 32

    replaceOffset(0x0087bfd4, addr_from_this(&CNIFUnderMenuBar::FUN_0060e540));// 32
    replaceOffset(0x0087c30c, addr_from_this(&CNIFUnderMenuBar::FUN_0060e540));// 32
    replaceOffset(0x0087c4cf, addr_from_this(&CNIFUnderMenuBar::FUN_0060e540));// 32
    replaceOffset(0x0087c592, addr_from_this(&CNIFUnderMenuBar::FUN_0060e540));// 32
    replaceOffset(0x0087c6f1, addr_from_this(&CNIFUnderMenuBar::FUN_0060e540));// 32
    replaceOffset(0x0087c826, addr_from_this(&CNIFUnderMenuBar::FUN_0060e540));// 32

    replaceOffset(0x0087c976, addr_from_this(&CNIFUnderMenuBar::FUN_0060e540));// 32

    replaceOffset(0x0087ca7f, addr_from_this(&CNIFUnderMenuBar::FUN_0060e540));// 32

    replaceOffset(0x0087cb11, addr_from_this(&CNIFUnderMenuBar::FUN_0060e540));// 32

    replaceOffset(0x0087cc8e, addr_from_this(&CNIFUnderMenuBar::FUN_0060e540));// 32
    replaceOffset(0x0087ce0b, addr_from_this(&CNIFUnderMenuBar::FUN_0060e540));// 32
    replaceOffset(0x0087cf71, addr_from_this(&CNIFUnderMenuBar::FUN_0060e540));// 32*/
    replaceOffset(0x0060b2b4, addr_from_this(&CNIFUnderMenuBar::LoadSlots));
    replaceOffset(0x007877c6, addr_from_this(&CNIFUnderMenuBar::LoadSlots));


    replaceOffset(0x0060c1d6, addr_from_this(&CNIFUnderMenuBar::SaveQuickslotData));

    replaceOffset(0x0060c313, addr_from_this(&CNIFUnderMenuBar::SaveQuickslotData));
    replaceOffset(0x0060c397, addr_from_this(&CNIFUnderMenuBar::SaveQuickslotData));
    replaceOffset(0x0060c403, addr_from_this(&CNIFUnderMenuBar::SaveQuickslotData));
    replaceOffset(0x0060c5d6, addr_from_this(&CNIFUnderMenuBar::SaveQuickslotData));
    replaceOffset(0x0060e762, addr_from_this(&CNIFUnderMenuBar::SaveQuickslotData));
    replaceOffset(0x0060e92b, addr_from_this(&CNIFUnderMenuBar::SaveQuickslotData));
    replaceOffset(0x0060eb82, addr_from_this(&CNIFUnderMenuBar::SaveQuickslotData));
    replaceOffset(0x0060ebfd, addr_from_this(&CNIFUnderMenuBar::SaveQuickslotData));

    replaceOffset(0x0060ec7f, addr_from_this(&CNIFUnderMenuBar::SaveQuickslotData));
    replaceOffset(0x007877d8, addr_from_this(&CNIFUnderMenuBar::SaveQuickslotData));

    replaceOffset(0x0059c967, addr_from_this(&CIFSlotWithHelp::ClearSlotEmpty));
    replaceOffset(0x0059c96e, addr_from_this(&CIFSlotWithHelp::ClearSlotEmpty));




/// MSG BOX
    static const char *msgboox = "clientlibrary\\resinfo\\ifmessagebox.txt";
    replaceAddr(0x0063e3a1+1, (int)(msgboox));

    replaceOffset(0x006ec77b, addr_from_this(&CIFMessageBox::SetMsgBoxHandler));// NEW REV
    replaceOffset(0x004f8e51, addr_from_this(&CIFMessageBox::SetMsgBoxHandler));
    replaceOffset(0x00676cc3, addr_from_this(&CIFMessageBox::SetMsgBoxHandler));
    replaceOffset(0x0086252a, addr_from_this(&CIFMessageBox::SetMsgBoxHandler));
    replaceOffset(0x0086c5a8, addr_from_this(&CIFMessageBox::SetMsgBoxHandler));
    replaceOffset(0x0087acdd, addr_from_this(&CIFMessageBox::SetMsgBoxHandler));
    replaceOffset(0x00883af9, addr_from_this(&CIFMessageBox::SetMsgBoxHandler));

    replaceOffset(0x0058567F, addr_from_this(&CIFMessageBox::SetEditMsgBoxHandler));
    replaceOffset(0x005A71E8, addr_from_this(&CIFMessageBox::SetEditMsgBoxHandler));
    replaceOffset(0x005B671D, addr_from_this(&CIFMessageBox::SetEditMsgBoxHandler));
    replaceOffset(0x006BA8AB, addr_from_this(&CIFMessageBox::SetEditMsgBoxHandler));
    replaceOffset(0x006F7835, addr_from_this(&CIFMessageBox::SetEditMsgBoxHandler));
    replaceOffset(0x0070DEC8, addr_from_this(&CIFMessageBox::SetEditMsgBoxHandler));
    replaceOffset(0x0071D1F0, addr_from_this(&CIFMessageBox::SetEditMsgBoxHandler));
    replaceOffset(0x0071FF30, addr_from_this(&CIFMessageBox::SetEditMsgBoxHandler));
    replaceOffset(0x007562F6, addr_from_this(&CIFMessageBox::SetEditMsgBoxHandler));
    replaceOffset(0x0075B98C, addr_from_this(&CIFMessageBox::SetEditMsgBoxHandler));
    replaceOffset(0x00764E58, addr_from_this(&CIFMessageBox::SetEditMsgBoxHandler));
    replaceOffset(0x00764F07, addr_from_this(&CIFMessageBox::SetEditMsgBoxHandler));
    replaceOffset(0x007A43E1, addr_from_this(&CIFMessageBox::SetEditMsgBoxHandler));

    replaceOffset(0x00643D54, addr_from_this(&CIFMessageBox::MsgBoxStore));
    replaceOffset(0x00643D60, addr_from_this(&CIFMessageBox::MsgBoxStoreMoney));
    replaceOffset(0x006B9DAA, addr_from_this(&CIFStallSlot::sub_6CB8A0));

    replaceOffset(0x0056605F, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0057AC52, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0057AC6F, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0058487E, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x005880F3, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x00588B01, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x00588B2A, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x00588BDA, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x005939E3, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x00593A19, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x00593A2F, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x005A788B, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x005A78AB, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x005A87E2, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x005A87FA, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x005B6A1A, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x005BBFC2, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x005BBFDA, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x005C2ABE, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x005C2AD6, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x00641F10, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006425A3, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0064335D, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0064344F, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006434BB, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x00646CBD, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006B4636, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006B4646, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006B4654, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006B4662, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006B4672, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006B4680, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006B9A3B, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006BB133, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006BB29C, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006C8056, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006E4ABF, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006E884C, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006EA71A, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006F7A31, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006F7A66, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006F7AD3, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x006F91F9, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0070D12F, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0070D147, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0070E6CB, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0070E6EB, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0071A8B4, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0071F071, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0071F089, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0071F12F, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0072039F, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x007205D1, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x007205F1, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0073269B, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x007326B3, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0074E1F0, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0074E38F, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x00763BAD, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x0076FDE2, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x00777E3B, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x00788894, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x007888B6, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x007888C4, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x007888EF, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x00788C6D, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x00788E39, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x00788E8D, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x00788EF3, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x00788F28, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x007987D3, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x00798925, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x007989E8, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x007A41E0, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x007A45BE, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x007A463D, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x007A6ABD, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x007A6B0B, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x007A6B1D, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x007A6B2F, addr_from_this(&CIFMessageBox::OnClickConfirm));
    replaceOffset(0x007A7660, addr_from_this(&CIFMessageBox::OnClickConfirm));


    vftableHook(0x00d9b9cc, 3, addr_from_this(&CIFMessageBox::MessageMap));
    vftableHook(0x00d9b9cc, 10, addr_from_this(&CIFMessageBox::OnCreateIMPL));

    ///CIDECOSKILL
    replaceOffset(0x00a49564, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x009dd84a, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x009dd6f6, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x009dd6d4, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x009dbb54, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x009d37cd, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x009d1a32, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x009cf982, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x009cf37d, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x009cf2bf, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x009cd723, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x009c5d65, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x0095db67, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x0095daad, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x008a96db, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x008a9476, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x008a941c, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x008a7092, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x008a706e, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x008a5d49, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x008a5b53, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x008a4ae1, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x008a4a9f, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x0087381d, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x008737b5, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x004e8f6b, addr_from_this(&CIDecoSkill::CreateEffect));
    replaceOffset(0x004e8ebb, addr_from_this(&CIDecoSkill::CreateEffect));


    ///SLOT
    replaceAddr(0x00da1918, addr_from_this(&CIFSlotWithHelp::RenderMyselfIMPL));


    /// OLD ITEM MALL
    replaceAddr(0x00D72260 + 1, addr_from_this(&CIFItemMallConfirmBuy::OnBuy_BtnClick));
    replaceAddr(0x00EBF890, addr_from_this(&CIFItemMallConfirmBuy::OnBuy_BtnClick));
    replaceOffset(0x007C5DE0, addr_from_this(&CIFItemMallConfirmBuy::OnBuy_BtnClick));
    replaceAddr(0x00DC8A48, addr_from_this(&CIFItemMall::OnCloseWnd_IMPL));

    static const char *itemmall = "clientlibrary\\resinfo\\ifitemmall.txt";
    replaceAddr(0x007c0a2a+1, (int)(itemmall));

    /// GHACHA
    ApplyMagicPopAnimationSpeedPatch();
    const int magicPopPlayHandler = addr_from_this(&CIFGhaCha::PlayButton);

    // 0x00D7009A is the dynamic initializer that writes the ID 10 (Play)
    // message handler into 0x00EBCE80. KMTGuardKit installs hooks from its
    // initialization thread, so the initializer may already have run before
    // this patch is applied. Patch both sides to make the hook independent of
    // startup ordering: the initializer covers the not-yet-run case and the
    // live message-map entry covers the already-run case.
    replaceAddr(0x00d7009a + 1, magicPopPlayHandler);
    replaceAddr(0x00ebce80, magicPopPlayHandler);
    vftableHook(0x00db484c, 5, addr_from_this(&CIFGhaCha::OnTimerIMPL));
    static const char *magic = "clientlibrary\\resinfo\\ifghachaselectwnd.txt";
    replaceAddr(0x0074a7d4+1, (int)(magic));

    replaceAddr(0x00db4b9c, addr_from_this(&CIFGhachaSelectWnd::OnCreateIMPL));
    vftableHook(0x00db4b74, 3, addr_from_this(&CIFGhachaSelectWnd::MessageMap));

    const int magicPopStartHandler = addr_from_this(&CIFGhachaSelectWnd::StartGame);

    // The select window's ID 10 Start handler is initialized by the MOV at
    // 0x00D70185 and stored in the live message-map entry at 0x00EBCFD0.
    // Hook both for the same asynchronous-startup ordering guarantee used by
    // the main Magic Pop Play handler above.
    replaceAddr(0x00d70185 + 1, magicPopStartHandler);
    replaceAddr(0x00ebcfd0, magicPopStartHandler);

    ///MACRO SHIT
    replaceOffset(0x00685efd, addr_from_this(&CIFSlotWithHelp::FUN_00682040));
    replaceOffset(0x0065fc56, addr_from_this(&CIFSlotWithHelp::FUN_00682040));
    vftableHook(0x00da18e4, 29, addr_from_this(&CIFSlotWithHelp::OnMouseLeftUpIMPL));
    replaceOffset(0x008AFCF9,addr_from_this(&CDropItemManager::sub_8AF7F0));




  /*  replaceOffset(0x007b3a4d, addr_from_this(&CGInterface::CallNIFEnchantWnd));
    replaceOffset(0x0079d984, addr_from_this(&CGInterface::CallNIFEnchantWnd));
    replaceOffset(0x0079c07d, addr_from_this(&CGInterface::CallNIFEnchantWnd));
    replaceOffset(0x00794118, addr_from_this(&CGInterface::CallNIFEnchantWnd));
    replaceOffset(0x0079410d, addr_from_this(&CGInterface::CallNIFEnchantWnd));
    replaceOffset(0x00780a87, addr_from_this(&CGInterface::CallNIFEnchantWnd));
    replaceOffset(0x00780a7b, addr_from_this(&CGInterface::CallNIFEnchantWnd));
    replaceOffset(0x0077b55d, addr_from_this(&CGInterface::CallNIFEnchantWnd));

    replaceOffset(0x006b9775, addr_from_this(&CGInterface::CallNIFEnchantWnd));
    replaceOffset(0x00652c5d, addr_from_this(&CGInterface::CallNIFEnchantWnd));

    replaceOffset(0x0060ce9a, addr_from_this(&CGInterface::CallNIFEnchantWnd));
    replaceOffset(0x005942de, addr_from_this(&CGInterface::CallNIFEnchantWnd));
*/
   // replaceAddr(0x00d8eda4, addr_from_this(&CNIFEnchantWnd::OnCreateIMPL));

  //  replaceOffset(0x0077825d, addr_from_this(&CGInterface::WriteSystemMessageForHook));

/*    replaceOffset(0x005952bd, addr_from_this(&CGInterface::WriteSystemMessageForHook));

    replaceOffset(0x00593e7f, addr_from_this(&CGInterface::WriteSystemMessageForHook));

    replaceOffset(0x00593e7f, addr_from_this(&CGInterface::WriteSystemMessageForHook));
*/
    //vftableHook(0x00d8ed7c, 43, addr_from_this(&CNIFEnchantWnd::OnCloseWndIMPL));

    //replaceAddr(0x00d90d2c, addr_from_this(&CNIFAlchemySubWndType1::OnCreateIMPL));
    //vftableHook(0x00d90d04, 11, addr_from_this(&CNIFAlchemySubWndType1::OnReleaseIMPL));
    //vftableHook(0x00d90d04, 12, addr_from_this(&CNIFAlchemySubWndType1::OnUpdateIMPL));

   // replaceAddr(0x00d90db0, addr_from_this(&CNIFAlchemySubWndType1::OnCloseWndIMPL));
/// ITEM MALL SHIT
    replaceAddr(0x00da195c, addr_from_this(&CIFSlotWithHelp::OnMouseLeftDownIMPL));
    replaceOffset(0x0063AA3C, addr_from_this(&CIFTextBox::OnHover));
    replaceOffset(0x0063AD88, addr_from_this(&CIFTextBox::OnHover));
    replaceAddr(0x00dbdaf4, addr_from_this(&CIFSystemMessage::OnCreateIMPL));
    vftableHook(0x00dbdacc, 12, addr_from_this(&CIFSystemMessage::OnUpdateIMPL));

/// EVENT SUIT
    replaceOffset(0x009de765, addr_from_this(&CICUser::Func_23_IMPL));
    replaceAddr(0x00da5108, addr_from_this(&CIFTargetWindow::ShowGWndIMPL));
//// PET2

    replaceAddr(0x00dba0f4, addr_from_this(&CIFCOS::OnCreateIMPL));
    replaceAddr(0x00dbabfc, addr_from_this(&CIFCOSInfo::OnCreateIMPL));
    replaceAddr(0x00dbac04, addr_from_this(&CIFCOSInfo::OnUpdateIMPL));

    replaceOffset(0x0079fc6a, addr_from_this(&CIFCOSInfo::FUN_007a5610));

    replaceOffset(0x008a6697, addr_from_this(&CIFCOS::Switch));
    replaceOffset(0x008110d9, addr_from_this(&CIFCOS::Switch));
    replaceOffset(0x0079bd43, addr_from_this(&CIFCOS::Switch));

    replaceAddr(0x00dbd37c, addr_from_this(&CIFPetMiniInfo::OnUpdateIMPL));
    replaceOffset(0x008afabb, addr_from_this(&CIFCOS::FUN_0079fcd0));

    static const char *cos = "clientlibrary\\resinfo\\ifcos.txt";
    replaceAddr(0x0079f9e4+1, (int)(cos));

    static const char *cosinfo = "clientlibrary\\resinfo\\ifcosinfo.txt";
    replaceAddr(0x007a48db+1, (int)(cosinfo));




    replaceAddr(0x00dd182c, addr_from_this(&CIFCOSManager::OnCreateIMPL));
    replaceAddr(0x00dd1834, addr_from_this(&CIFCOSManager::OnUpdateIMPL));

    vftableHook(0x00dba0cc, 3, addr_from_this(&CIFCOS::MessageMap));
    vftableHook(0x00dba0cc, 12, addr_from_this(&CIFCOS::OnUpdateIMPL));

    replaceAddr(0x00d711e0+1, addr_from_this(&CIFCOS::PressTabs));
    replaceOffset(0x00686ee3, addr_from_this(&CIFSlotWithHelp::FUN_006821f02));
    replaceOffset(0x0068710a, addr_from_this(&CIFSlotWithHelp::FUN_006821f02));


    PatchMe(0x00686dea + 1, 0xfa);// extend bubble
   // PatchMe(0x00686dea +2 , 0xfa);// extend bubble

    PatchMe(0x00BA2156 + 7, 0XB0); /// IF STATIC SIZE
    PatchMe(0x00BA2156 + 6, 0x36); /// IF STATIC SIZE

    ////
   /* replaceOffset(0x0067b54e, addr_from_this(&CIFSlotWithHelp::WriteItemname));
    replaceOffset(0x00682262, addr_from_this(&CIFSlotWithHelp::WriteItemname));
    replaceOffset(0x0068249d, addr_from_this(&CIFSlotWithHelp::WriteItemname));
    replaceOffset(0x0068284d, addr_from_this(&CIFSlotWithHelp::WriteItemname));

    replaceOffset(0x00682c2d, addr_from_this(&CIFSlotWithHelp::WriteItemname));

    replaceOffset(0x00682e7d, addr_from_this(&CIFSlotWithHelp::WriteItemname));
    replaceOffset(0x006830c7, addr_from_this(&CIFSlotWithHelp::WriteItemname));
    replaceOffset(0x006847cf, addr_from_this(&CIFSlotWithHelp::WriteItemname));
    replaceOffset(0x00685c07, addr_from_this(&CIFSlotWithHelp::WriteItemname));
    replaceOffset(0x00686138, addr_from_this(&CIFSlotWithHelp::WriteItemname));
*/


   /* replaceOffset(0x00685fe5, addr_from_this(&CIFSlotWithHelp::WriteItemType2));
    replaceOffset(0x00685ff3, addr_from_this(&CIFSlotWithHelp::WriteItemType3));
    replaceOffset(0x00685ff3, addr_from_this(&CIFSlotWithHelp::WriteItemType4));
    replaceOffset(0x00685fc9, addr_from_this(&CIFSlotWithHelp::WriteItemType5));
*/
    //  replaceOffset(0x00682aa6, addr_from_this(&CIFSlotWithHelp::WriteItemType));
  //  replaceOffset(0x00682aa6, addr_from_this(&CIFSlotWithHelp::WriteItemType));

    replaceOffset(0x00686857, addr_from_this(&CIFSlotWithHelp::WriteItemType));
    replaceOffset(0x00685cfe, addr_from_this(&CIFSlotWithHelp::WriteItemType));
    replaceOffset(0x006848d4, addr_from_this(&CIFSlotWithHelp::WriteItemType));
    replaceOffset(0x006831d1, addr_from_this(&CIFSlotWithHelp::WriteItemType));
    replaceOffset(0x00682f81, addr_from_this(&CIFSlotWithHelp::WriteItemType));
    replaceOffset(0x00682d31, addr_from_this(&CIFSlotWithHelp::WriteItemType));
    replaceOffset(0x00682abf, addr_from_this(&CIFSlotWithHelp::WriteItemType));
    replaceOffset(0x0068259c, addr_from_this(&CIFSlotWithHelp::WriteItemType));
    replaceOffset(0x0068235d, addr_from_this(&CIFSlotWithHelp::WriteItemType));
    replaceOffset(0x0067b651, addr_from_this(&CIFSlotWithHelp::WriteItemType));


    replaceOffset(0x00682aa6, addr_from_this(&CIFSlotWithHelp::WriteItemClass));
    replaceOffset(0x00682d18, addr_from_this(&CIFSlotWithHelp::WriteItemClass));
    replaceOffset(0x00682f68, addr_from_this(&CIFSlotWithHelp::WriteItemClass));
    replaceOffset(0x006831b8, addr_from_this(&CIFSlotWithHelp::WriteItemClass));

    replaceOffset(0x006824ba, addr_from_this(&CIFSlotWithHelp::SetRareName));
    replaceOffset(0x0068286a, addr_from_this(&CIFSlotWithHelp::SetRareName));
    replaceOffset(0x00682c4a, addr_from_this(&CIFSlotWithHelp::SetRareName));
    replaceOffset(0x00682e9a, addr_from_this(&CIFSlotWithHelp::SetRareName));
    replaceOffset(0x006830e4, addr_from_this(&CIFSlotWithHelp::SetRareName));


//    replaceOffset(0x009e9a92, addr_from_this(&CICScriptObjManager::FUN_009e5240));

   // PatchMe(0x009e8aa8+9, 0x20);

 //   replaceOffset(0x009e8ac8, addr_from_this(&CICScriptObjManager::Fun_005d2a80));
   //replaceOffset(0x009e8a99, addr_from_this(&CRStateMgr::FUN_0047e880));

  // replaceOffset(0x009e8a64, addr_from_this(&CGScript::Fun_0051fa20));

  //vftableHook(0x00de33e4, 10, addr_from_this(&CICScriptObj::OnCreateIMPL));
   // vftableHook(0x00de33e4, 9, addr_from_this(&CICScriptObj::FUN_009e5130));



//replaceAddr(0x00d85934, addr_from_this(&CGScript::OnCreateIMPL));

    vftableHook(0x00dd9384,10, addr_from_this(&CPSVersionCheck::OnCreateIMPL));

    vftableHook(0x00dd9384,12, addr_from_this(&CPSVersionCheck::OnUpdateIMPL));
    vftableHook(0x00dd9384,5, addr_from_this(&CPSVersionCheck::OnTimerIMPL));
    /*   replaceAddr(0x00dd12a0, addr_from_this(&CCObjCharacter::FUN_00a5bcd0));
       replaceAddr(0x00dd7c30, addr_from_this(&CCObjCharacter::FUN_00a5bcd0));
       replaceAddr(0x00de5ff4, addr_from_this(&CCObjCharacter::FUN_00a5bcd0));
   */
    ///PARTY MATCH
    replaceAddr(0x00db37cc, addr_from_this(&CIFPartyMatch::OnUpdateIMPL));
    replaceAddr(0x00db37e4, addr_from_this(&CIFPartyMatch::OnMouseLeftUpIMPL));
    replaceAddr(0x00db4304, addr_from_this(&CIFPartyMatchSlot::OnUpdateIMPL));
    replaceAddr(0x00db42fc, addr_from_this(&CIFPartyMatchSlot::OnCreateIMPL));
    replaceAddr(0x00db4358, addr_from_this(&CIFPartyMatchSlot::OnMouseRightUpIMPL));


    replaceOffset(0x0074122d, addr_from_this(&CIFPartyMatch::FUN_00740870));
    replaceOffset(0x0073a830, addr_from_this(&CIFPartyMatch::FUN_00740d30));
    replaceOffset(0x0073a8b0, addr_from_this(&CIFPartyMatch::FUN_00740d30));
    replaceOffset(0x0073a8ff, addr_from_this(&CIFPartyMatch::FUN_00740d30));
    replaceOffset(0x0073aa25, addr_from_this(&CIFPartyMatch::FUN_00740d30));
    replaceOffset(0x007412a5, addr_from_this(&CIFPartyMatch::FUN_00740d30));
    replaceOffset(0x00741239, addr_from_this(&CIFPartyMatch::FUN_00740910));

    replaceAddr(0x00da1968, addr_from_this(&CIFSlotWithHelp::OnMouseRightUPIMPL));



    vftableHook(0x00d8ed7c, 3, addr_from_this(&CNIFEnchantWnd::MessageMap));
    vftableHook(0x00d8ed7c, 5, addr_from_this(&CNIFEnchantWnd::OnTimerIMPL));
    vftableHook(0x00d8ed7c, 10, addr_from_this(&CNIFEnchantWnd::OnCreateIMPL));
    replaceAddr(0x00d66ead+1, addr_from_this(&CNIFEnchantWnd::PressAlchemyTabButton));
    replaceAddr(0x00d66eb7+1, addr_from_this(&CNIFEnchantWnd::PressSocketTabButton));


    replaceOffset(0x006fd803, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x006fdcc4, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x006fde4b, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x006fde59, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x006fded6, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x006fdefd, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x006fdf49, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x006fdf5e, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x006fdf6f, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x006fdf7a, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x006fdf86, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x006fe014, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x006febe5, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x006fee27, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x006ff271, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x007001b5, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x0070095e, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x00700b05, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x00700b7a, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x00700ba6, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x00700bb4, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x00700c08, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x00700c8d, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x00700ce1, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x00700f88, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x00700fdc, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x00701104, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x00701110, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x0070111c, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x0070112d, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x0070114d, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x00701159, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x0070116c, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x007011b2, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x007011f8, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x0070130a, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x00701350, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x00701396, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x007013dc, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x00701434, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x0070147a, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x007014c0, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x00701e56, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceOffset(0x00882258, addr_from_this(&CIF_NPCTalk::FUN_006fcd60));
    replaceAddr(0x00d6e5c0 + 1, addr_from_this(&CIF_NPCTalk::OnListChatThingIMPL));
   //  vftableHook(0x00daf0dc, 23, addr_from_this(&CIF_NPCWindow::ShowGWndIMPL));
    vftableHook(0x00daf0dc, 43, addr_from_this(&CIF_NPCWindow::OnCloseWndIMPL));

   // vftableHook(0x00da4a1c, 28, addr_from_this(&CIFSlotWithHelpEx::Func_28IMPL));
    //vftableHook(0x00da18e4, 28, addr_from_this(&CIFSlotWithHelp::Func_28IMPL));
  //  replaceOffset(0x0088b864, addr_from_this(&CIF_NPCTalk::WriteNpcInfo_Maybe));
//    replaceOffset(0x0088b977, addr_from_this(&CIF_NPCTalk::WriteNpcInfo_Maybe));
 //   replaceOffset(0x0088b890, addr_from_this(&CIF_NPCTalk::WriteNpcInfo_Maybe));
   // replaceOffset(0x0088ba3f, addr_from_this(&CIF_NPCTalk::WriteNpcInfo_Maybe));

   // vftableHook(0x00daf0dc, 23, addr_from_this(&CIF_NPCWindow::ShowGWndIMPL));
   // replaceOffset(0x006fd12c, addr_from_this(&CIFTextBox::sub_64F8A0));
  //  replaceOffset(0x006fd25a, addr_from_this(&CIFTextBox::sub_64F8A0));

    replaceAddr(0x00d6a74d +1, addr_from_this(&CIFEdit::FUN_00634cb0));


    replaceAddr(0x00dbd4fc, addr_from_this(&CIFPlayerMiniInfo::OnUpdateIMPL));
    replaceAddr(0x00D9841C, addr_from_this(&CNIFUnderMenuBar::Update));

    /*   replaceOffset(0x006fae12, addr_from_this(&CIF_NPCWindow::GetNpcObjID));
       replaceOffset(0x006fb687, addr_from_this(&CIF_NPCWindow::GetNpcObjID));
       replaceOffset(0x006fbc17, addr_from_this(&CIF_NPCWindow::GetNpcObjID));
       replaceOffset(0x006fc94b, addr_from_this(&CIF_NPCWindow::GetNpcObjID));

       replaceOffset(0x006fd443, addr_from_this(&CIF_NPCWindow::GetNpcObjID));
       replaceOffset(0x006fd907, addr_from_this(&CIF_NPCWindow::GetNpcObjID));
       replaceOffset(0x006fde38, addr_from_this(&CIF_NPCWindow::GetNpcObjID));
       replaceOffset(0x00700499, addr_from_this(&CIF_NPCWindow::GetNpcObjID));

       replaceOffset(0x0070192e, addr_from_this(&CIF_NPCWindow::GetNpcObjID));
       replaceOffset(0x00702850, addr_from_this(&CIF_NPCWindow::GetNpcObjID));
       replaceOffset(0x00748a6c, addr_from_this(&CIF_NPCWindow::GetNpcObjID));
   */

  /*  replaceOffset(0x008a611b, addr_from_this(&CCObjCharacter::Fun_00a54be0));
    replaceOffset(0x008a6137, addr_from_this(&CCObjCharacter::Fun_00a54be0));
    replaceOffset(0x00a55045, addr_from_this(&CCObjCharacter::Fun_00a54be0));
    replaceOffset(0x00a566e5, addr_from_this(&CCObjCharacter::Fun_00a54be0));



    static const wchar_t *new_unicode_str = L"ANI_READY06";
    replaceAddr(0x00de38c0, (int)(new_unicode_str));
*/
    replaceOffset(0x009ebde2, addr_from_this(&OngoingNetMessage));
    /*
    replaceOffset(0x0053dc15, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0053dd35, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0053de55, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0053df75, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0053e097, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0053e1c7, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0053e2f5, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0054298a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x005473fc, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0054785a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x005479dd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00547b5d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00547cdd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00547e5d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00548181, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0054836a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00548560, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x005486dd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0054885f, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x005489dd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00548b9f, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00548de5, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0054930d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0054948d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00549a2b, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00549cdd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00549e5d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00549fdd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0054a15d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0054a496, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0054aa24, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0054cb32, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0054cc00, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0054cfe0, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0054d14d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00551b57, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00553066, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00557cc3, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x005646ea, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x005849ce, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x005ad076, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x005d950f, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x005df3c2, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x005df46d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x005e3933, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x005f481e, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x005f5a56, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0060c06d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0065b9a0, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0068d087, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0070241c, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00702518, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007025dc, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007026a0, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00702766, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00702ce3, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00702f0c, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0070306d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00703312, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007033d6, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007035c1, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007036c4, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007037b9, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007038ca, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00703c24, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00746e88, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00780b65, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007875e3, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007889b9, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00788a65, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00788ed3, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0078f56f, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0078f5c0, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0078f6ce, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0078f78f, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0078f89a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0078f998, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0078fcc3, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0078fddc, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0078ff30, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00790038, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00790347, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007909ae, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00790b01, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00791ea9, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00791f73, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007920a2, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00793871, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00793c84, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00793e98, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007942f0, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0079439e, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00794424, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007944aa, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0079452f, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007945b5, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0079463b, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007946c1, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007947c9, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00796410, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007967ff, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00797244, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007977fc, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00797d08, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00797f7a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00799e23, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007a0d1c, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007a0e30, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007a42a9, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007adc72, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007d41d9, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007d42c7, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007d5fd0, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007d60ab, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007eac97, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x007eccc0, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081c00b, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081c3c9, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081c695, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081c8bd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081ca9e, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081cb3a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081cbfd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081cce7, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081cdd0, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081ceb7, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081cfa0, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081d05d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081d11f, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081d1dd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081d2c1, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081d37d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081d43f, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081d4fd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081d5e1, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081d6c1, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081d75a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081d972, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081da0a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081daaa, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081db4a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081dc32, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081dced, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081ddbd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081deef, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081e063, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081e13a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081e216, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081e2ba, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081e37f, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081e63a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081e71f, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081e7df, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081e89d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081e95d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081ea3d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081eb21, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081ebf3, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081ecad, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081ed6d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081ee2d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081eef0, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081efdc, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081f0cf, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081f16a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081f23d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081f2fd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081f3e1, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081f4ad, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081f56d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081f6da, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081f7a4, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081f864, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081f9a7, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081fa7d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081fb3d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081fca9, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081fd88, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081fe54, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081ff1d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0081ffdd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082009d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00820181, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008202be, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082035a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082041d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008204f6, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008205cd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082068d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082074d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082080d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008208da, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082094e, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00820a2d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00820aed, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00820bd1, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00820c7a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00820d3f, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00820dff, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00820ee1, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00820fc1, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008210b7, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00821196, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00821221, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00821379, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008214bc, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008215c2, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008216f2, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082181a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008218ed, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00821aa7, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00821ba1, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00821c3a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00821d51, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00821e0d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00821ecd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008220ad, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082214a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082220f, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008222aa, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082236d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008226f3, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008228b4, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008229fd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00822aff, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00822bfd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00822cd9, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00822d7a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00822e86, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00822f96, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008230c1, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008231a7, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082325f, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082331d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008233dd, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008234c1, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082357d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00823693, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082376d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008238b0, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082396a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00823a2f, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00823b23, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00823bf3, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00823dbb, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00823e7b, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00823f3b, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00823ffb, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00824108, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082430b, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00824477, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00824677, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00824947, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00824bb2, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00824d4e, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00824eff, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082509d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082521d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082537f, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00825792, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082596f, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00825aa4, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00825c21, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00825d81, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00825f10, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082635b, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008265e9, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0082d41f, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008431f5, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0084c1f8, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0084c255, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0084e85a, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0084eee0, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008522ea, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00852962, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00856175, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00857d85, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008588ef, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00858d29, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00858dc7, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0085925e, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00859422, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x00863c05, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008642b7, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x0086e16d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008886a0, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008a638d, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008a8e56, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008a8f86, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x008afd9c, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x009eb5f5, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x009eb698, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x009eb806, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x009eb8ad, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x009ebe82, addr_from_this(&OngoingNetMessage));
    replaceOffset(0x009ec208, addr_from_this(&OngoingNetMessage));
*/
    replaceOffset(0x00637a50, addr_from_this(&CIFEdit::OnCharIMPL));
  //  replaceAddr(0x00831337 + 4, (int) &WndProcHook);

    vftableHook(0x00da775c, 28, addr_from_this(&CIFInventory::Func_28_IMPL));


   // vftableHook(0x00d9ae3c, 17, addr_from_this(&CIFEdit::Func_17_IMPL));

    WriteClientStartupDiagnostic("Setup stage begin: Core/DiscordManager.");
    m_dc = new DiscordManager();
    m_dc->Start(895258301107957760);
    WriteClientStartupDiagnostic("Setup stage end: Core/DiscordManager.");

 //   PatchMe(0x00681405+1, 2);


    replaceOffset(0x00a566e5, addr_from_this(&CCObjCharacter::FUN_00a548e0));
#ifdef CONFIG_DEBUG_REDIRECT_PUTDUMP
    replaceAddr(0x00832927 + 1, (int) &DebugPrintCallback);
#endif // CONFIG_DEBUG_REDIRECT_PUTDUMP

#ifdef CONFIG_TRANSLATIONS_DEBUG
    placeHook(0x008C9C30, addr_from_this(&CTextStringManager::GetString));
#endif // CONFIG_TRANSLATIONS_DEBUG

    replaceOffset(0x008774f4, (int)&WriteToChatWindow);
    replaceOffset(0x00877b5c, (int)&WriteToChatWindow);

    // Keep unique kill notices visible when the killer is a GM.
    RenderNop((void*)0x004C1D23, 6);

    placeHook(0x007a9bd0, addr_from_this(&CIFChatViewer::ShowHideControls));

    replaceOffset(0x00558618, addr_from_this(&CIFConsole::SetVisibleMode));
    WriteClientStartupDiagnostic("Setup stage begin: UI/PatchWatermark.");
    PatchWatermark();
    WriteClientStartupDiagnostic("Setup stage end: UI/PatchWatermark.");
    WriteClientStartupDiagnostic("Setup stage begin: Game/QuickStart.Setup.");
    quickstart.Setup();
    WriteClientStartupDiagnostic("Setup stage end: Game/QuickStart.Setup.");
    return true;
}

#undef ApplyRegisteredMemberHooks
#undef placeHook
#undef replaceOffset
#undef replaceAddr
#undef vftableHook
#undef RenderNop

bool SetupWithDiagnostics()
{
    const char* operation = "Setup outer boundary";
    __try {
        return Setup();
    }
    __except (LogSetupException(operation, 0, GetExceptionInformation())) {
        return false;
    }
}

void PatchWatermark()
{

#pragma pack(push, 1)
    struct
    {
        byte opcode;
        int color;
    } colorPatch;
#pragma pack(pop)
    colorPatch.opcode = 0x68; // push (constant)
    colorPatch.color = 0x87ceeb;
    CopyBytes(0x0086BC33, &colorPatch, sizeof(colorPatch));

    static std::wstring versionFormat;
    if (versionFormat.length() == 0)
    {
        versionFormat.append(KmtGetText(L"UIIT_KMT_VERSION"));
        versionFormat.append(L" %d.%03d\nKMTGuard v" KMTGUARD_CLIENT_VERSION_WSTRING);
    }



#pragma pack(push, 1)
    struct
    {
        byte opcode;
        const wchar_t* address;
    } textPatch;
#pragma pack(pop)
    textPatch.opcode = 0x68;;
    textPatch.address = versionFormat.c_str();
    CopyBytes(0x0086BC6F, &textPatch, sizeof(textPatch));
}
bool DoesFileExists(const std::string &name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

void RegisterObject(const CGfxRuntimeClass *obj) {
    register_objects.push_back(obj);
}

void OverrideObject(overrideFnPtr fn) {
    override_objects.push_back(fn);
}

namespace
{
volatile LONG g_runtimeClassRegistrationState = 0;

void InstallRuntimeClassesUnsafe()
{
    // Replace Create & Delete for existing classes
    // Note: We can't just inject existing objects like we would do with new objects.
    //       Joymax uses == on GFX_RUNTIME_CLASS(), so we would end up breaking this comparison

    for (std::vector<const CGfxRuntimeClass *>::const_iterator it = register_objects.begin();
         it != register_objects.end(); ++it) {
        reinterpret_cast<void (__thiscall *)(const CGfxRuntimeClass *, const char *, void *, void *,const CGfxRuntimeClass *, size_t, int)>(0x00B9C9C0)(*it,(*it)->m_lpszClassName, (*it)->m_pfnCreateObject, (*it)->m_pfnDeleteObject, (*it)->m_pBaseClass, (*it)->m_nObjectSize, 0);
    }

    for (std::vector<overrideFnPtr>::const_iterator it = override_objects.begin(); it != override_objects.end(); ++it) {
        (*it)();
    }
}
}

bool EnsureRuntimeClassesInstalled()
{
    LONG state = InterlockedCompareExchange(
        &g_runtimeClassRegistrationState,
        1,
        0);
    if (state == 2)
        return true;
    if (state < 0)
        return false;

    if (state == 1) {
        const DWORD startedAt = GetTickCount();
        do {
            Sleep(1);
            state = InterlockedCompareExchange(
                &g_runtimeClassRegistrationState,
                0,
                0);
            if (state == 2)
                return true;
            if (state < 0)
                return false;
        } while (GetTickCount() - startedAt < 30000);

        WriteClientStartupDiagnostic(
            "Runtime-class registration did not finish within the safety timeout.");
        return false;
    }

    __try {
        InstallRuntimeClassesUnsafe();
    }
    __except (LogSetupException(
        "Runtime-class registration",
        __LINE__,
        GetExceptionInformation())) {
        InterlockedExchange(&g_runtimeClassRegistrationState, -1);
        return false;
    }

    InterlockedExchange(&g_runtimeClassRegistrationState, 2);
    WriteClientStartupDiagnostic("Runtime-class registration completed.");
    return true;
}

void InstallRuntimeClasses(CGame *)
{
    if (EnsureRuntimeClassesInstalled())
        return;

    WriteClientStartupDiagnostic(
        "Client startup stopped because runtime-class registration was incomplete.");
    TerminateProcess(GetCurrentProcess(), ERROR_DLL_INIT_FAILED);
}
bool ObjIntersect(CIObject* Src, CIObject* Dis)
{
    /*   LocationInfo l1 = { 0, 0, 0, 0 };
       l1.pos = Src->GetLocation();
       l1.region = Src->GetRegion();
       l1.field_0 = Src->m_navcell;
       l1.field_1 = Src->m_object_under_foot;
       LocationInfo l2 = { 0, 0, 0,0 };
       l1.pos = Dis->GetLocation();
       l1.region = Dis->GetRegion();

       return !g_CRegionManagerBody.SomethingWithMapCollision_MAYBE(1, 1, &l1, &l2, 0, g_pCICPlayer);*/
    CGFXVideo3d* gfx = CGFXVideo3d::get();

    for (std::map<short, CRTNavMeshTerrain*>::iterator mit = CIFMacroMenuAutoHunt::CRTNavMeshTerrainAll.begin();
         mit != CIFMacroMenuAutoHunt::CRTNavMeshTerrainAll.end(); ++mit)
    {
        CRTNavMeshTerrain* m_pNavmesh = mit->second;
        BYTE bHIGH, bLow;
        bHIGH = m_pNavmesh->m_Region >> 8;
        bLow = m_pNavmesh->m_Region & 0xFF;
        //printf("%d,%d,%d\r\n",m_pNavmesh->m_Region, bHIGH,bLow);
        uregion reg;
        reg.single.x = bHIGH;
        reg.single.y = bLow;
        for (std::vector<SNavMeshInst*>::iterator it = m_pNavmesh->m_sObjList.begin();
             it != m_pNavmesh->m_sObjList.end(); ++it) {
            SNavMeshInst* pInst = *it;
            CRTNavMeshObj* pObj = pInst->m_pObject;
            const D3DXVECTOR3& offset = pInst->m_sObj.Offset;
            float yaw = -pInst->m_sObj.Yaw;

            for (std::vector<PrimNavMeshEdge>::iterator it = pObj->m_GlobalEdges.begin();

                 it != pObj->m_GlobalEdges.end(); ++it) {
                PrimNavMeshEdge& edge = *it;
                if (edge.btFlag == EDGE_FLAG_BLOCKED) {
                    D3DXVECTOR3 p1, p2, p1_2d, p2_2d, p3_2d, p4_2d;
                    p1 = edge.AssocVertex[0]->Position;
                    rotatey(p1, yaw);
                    p1 += offset;


                    p2 = edge.AssocVertex[1]->Position;
                    rotatey(p2, yaw);
                    p2 += offset;

                    D3DXVECTOR2 aa, bb, cc, dd;

                    D3DXVECTOR3 a3 = Src->GetLocation();
                    D3DXVECTOR3 b3 = Dis->GetLocation();
                    GetSilkPos(reg, p1);
                    GetSilkPos(reg, p2);
                    GetSilkPos(Src->GetRegion(), a3);
                    GetSilkPos(Dis->GetRegion(), b3);
                    aa.x = a3.x, aa.y = a3.z,
                    bb.x = b3.x, bb.y = b3.z,
                    cc.x = p1.x, cc.y = p1.z,
                    dd.x = p2.x, dd.y = p2.z;
                    if (intersect(aa, bb, cc, dd)) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}
double determinant(double v1, double v2, double v3, double v4)  // ?????
{
    return (v1 * v4 - v2 * v3);
}

bool intersect(D3DXVECTOR2 aa, D3DXVECTOR2 bb, D3DXVECTOR2 cc, D3DXVECTOR2 dd)
{
    double delta = determinant(bb.x - aa.x, cc.x - dd.x, bb.y - aa.y, cc.y - dd.y);
    if (delta <= (1e-6) && delta >= -(1e-6))  // delta=0?????????????????
    {
        return false;
    }
    double namenda = determinant(cc.x - aa.x, cc.x - dd.x, cc.y - aa.y, cc.y - dd.y) / delta;
    if (namenda > 1 || namenda < 0)
    {
        return false;
    }
    double miu = determinant(bb.x - aa.x, cc.x - aa.x, bb.y - aa.y, cc.y - aa.y) / delta;
    if (miu > 1 || miu < 0)
    {
        return false;
    }
    return true;
}
void GetSilkPos(uregion dis, D3DVECTOR& location) {
    float offsetZ=0,offsetX = 0;
    offsetX = static_cast<float>(abs((g_pMyPlayerObj->GetRegion().single.y - dis.single.y) * 1920));
    offsetZ = static_cast<float>(abs((g_pMyPlayerObj->GetRegion().single.x - dis.single.x) * 1920));
    if (g_pMyPlayerObj->GetRegion().single.y - dis.single.y < 0) {
        location.x = location.x + offsetX;
    }
    else if(g_pMyPlayerObj->GetRegion().single.y - dis.single.y > 0) {

        location.x = location.x-offsetX;
    }
    if (g_pMyPlayerObj->GetRegion().single.x - dis.single.x< 0) {
        location.z = location.z + offsetZ;

    }
    else if(g_pMyPlayerObj->GetRegion().single.x - dis.single.x > 0) {
        location.z = location.z-offsetZ;
    }
}
D3DVECTOR GetSilkPosD3D(uregion dis, D3DVECTOR location) {
    float offsetZ = 0, offsetX = 0;
    offsetX = static_cast<float>(abs((g_pMyPlayerObj->GetRegion().single.y - dis.single.y) * 1920));
    offsetZ = static_cast<float>(abs((g_pMyPlayerObj->GetRegion().single.x - dis.single.x) * 1920));
    if (g_pMyPlayerObj->GetRegion().single.y - dis.single.y < 0) {
        location.x = location.x + offsetX;
    }
    else if (g_pMyPlayerObj->GetRegion().single.y - dis.single.y > 0) {

        location.x = location.x - offsetX;
    }
    if (g_pMyPlayerObj->GetRegion().single.x - dis.single.x < 0) {
        location.z = location.z + offsetZ;

    }
    else if (g_pMyPlayerObj->GetRegion().single.x - dis.single.x > 0) {
        location.z = location.z - offsetZ;
    }
    return location;
}
