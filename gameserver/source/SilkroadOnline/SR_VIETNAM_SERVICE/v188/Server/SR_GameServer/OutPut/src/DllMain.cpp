
#include <stdio.h>
#include "Util.h"
#include <memory/hook.h>
#include <memory/MemoryUtility.h>
#include <memory/detours.h>
#include "GameServerConsole.h"
#include "GameServerCrashHandler.h"
#include <KMTGuardCustom/GameServerRuntimeSafety.h>
#include <MainProcess.h>
#include <Objects/GObjPC.h>

typedef int (WINAPI* fnMessageBoxA)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType);
typedef int (WINAPI* fnMessageBoxW)(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType);
typedef BOOL (WINAPI* fnGetModuleHandleExACompat)(DWORD dwFlags, LPCSTR lpModuleName, HMODULE* phModule);
fnMessageBoxA pfnOrigMessageBoxA = NULL;
fnMessageBoxW pfnOrigMessageBoxW = NULL;

#ifndef GET_MODULE_HANDLE_EX_FLAG_PIN
#define GET_MODULE_HANDLE_EX_FLAG_PIN 0x00000001
#endif

#ifndef GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
#define GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS 0x00000004
#endif

namespace
{
    __declspec(thread) bool s_insideMessageBoxHook = false;
    HMODULE s_processLifetimeModule = NULL;

    bool IsIgnorablePackagePriceMessage(const char* text)
    {
        if (text == NULL)
            return false;
        return strstr(text, "register silk price") != NULL ||
               strstr(text, "register gold price") != NULL;
    }

    bool IsIgnorablePackagePriceMessage(const wchar_t* text)
    {
        if (text == NULL)
            return false;
        return wcsstr(text, L"register silk price") != NULL ||
               wcsstr(text, L"register gold price") != NULL;
    }

    void HandleGameServerMessage(const char* text)
    {
        if (IsIgnorablePackagePriceMessage(text))
            return;
        char boundedText[768] = { 0 };
        if (text != NULL && text[0] != '\0')
        {
            strncpy_s(boundedText, sizeof(boundedText), text, _TRUNCATE);
            GameServerConsole::WriteWarning(boundedText);
        }
        else
            GameServerConsole::WriteWarning("GameServer reported a system message");
    }
}


int WINAPI MyMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
    if (IsIgnorablePackagePriceMessage(lpText))
        return IDOK;
    HandleGameServerMessage(lpText);
    if (pfnOrigMessageBoxA == NULL || s_insideMessageBoxHook)
        return IDOK;
    s_insideMessageBoxHook = true;
    const int result = pfnOrigMessageBoxA(hWnd, lpText, lpCaption, uType);
    s_insideMessageBoxHook = false;
    return result;
}

int WINAPI MyMessageBoxW(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType)
{
    if (IsIgnorablePackagePriceMessage(lpText))
        return IDOK;
    char text[768] = { 0 };
    if (lpText != NULL)
        WideCharToMultiByte(CP_UTF8, 0, lpText, -1, text, sizeof(text), NULL, NULL);
    HandleGameServerMessage(text);
    if (pfnOrigMessageBoxW == NULL || s_insideMessageBoxHook)
        return IDOK;
    s_insideMessageBoxHook = true;
    const int result = pfnOrigMessageBoxW(hWnd, lpText, lpCaption, uType);
    s_insideMessageBoxHook = false;
    return result;
}


class CGObj;
class CInstance;
typedef const char* (__thiscall* fnGetCharName)(CGObj* pObj);
typedef const char* (__thiscall* fnGetNickName)(CGObj* pObj);

fnGetCharName pfnOrigGetCharName = NULL;
fnGetNickName pfnOrigGetNickName = NULL;

const char* szUnknown = "Unknown";

CInstance* GetGObjInstance(CGObj* pObj)
{
    return MEMUTIL_READ_BY_PTR_OFFSET(pObj, 0x34, CInstance*);
}

const char* __fastcall MyGetCharName(CGObj* pObj, LPVOID /* dummy edx */)
{
    if (pObj == NULL)
        return szUnknown;

    if (GetGObjInstance(pObj) == NULL)
        return szUnknown;

    return pfnOrigGetCharName(pObj);
}

const char* __fastcall MyGetNickName(CGObj* pObj, LPVOID /* dummy edx */)
{
    if (pObj == NULL)
        return szUnknown;

    if (GetGObjInstance(pObj) == NULL)
        return szUnknown;

    return pfnOrigGetNickName(pObj);
}


//Null instance fix
#define GOBJ_GET_CHAR_NAME_FUNC_OFFSET									0x004A66D0
#define GOBJ_GET_NICK_NAME_FUNC_OFFSET									0x004DDC50


static void RemoveSystemMessageHooks()
{
    if (pfnOrigMessageBoxA == NULL || pfnOrigMessageBoxW == NULL)
        return;
    GameServerRuntimeSafety::DetachDetour(
        reinterpret_cast<PVOID*>(&pfnOrigMessageBoxW),
        reinterpret_cast<PVOID>(MyMessageBoxW), "MessageBoxW detour");
    GameServerRuntimeSafety::DetachDetour(
        reinterpret_cast<PVOID*>(&pfnOrigMessageBoxA),
        reinterpret_cast<PVOID>(MyMessageBoxA), "MessageBoxA detour");
}

static DWORD InitializeGameServerAddonCore(HMODULE hModule)
{
        GameServerConsole::Initialize();
        if (s_processLifetimeModule == NULL)
        {
            GameServerConsole::WriteFailure("GameServer add-on process-lifetime pin failed");
            return ERROR_DLL_INIT_FAILED;
        }
        GameServerCrashHandler::Initialize(hModule);

        HMODULE hUser32 = GetModuleHandleA("User32.dll");
        if (hUser32 == NULL)
        {
            GameServerConsole::WriteFailure("User32 initialization failed");
            return ERROR_DLL_INIT_FAILED;
        }

        pfnOrigMessageBoxA = reinterpret_cast<fnMessageBoxA>(
            GetProcAddress(hUser32, "MessageBoxA"));
        pfnOrigMessageBoxW = reinterpret_cast<fnMessageBoxW>(
            GetProcAddress(hUser32, "MessageBoxW"));
        if (pfnOrigMessageBoxA == NULL || pfnOrigMessageBoxW == NULL)
        {
            GameServerConsole::WriteFailure("MessageBox API resolution failed");
            return ERROR_PROC_NOT_FOUND;
        }

        if (!GameServerRuntimeSafety::AttachDetour(
                reinterpret_cast<PVOID*>(&pfnOrigMessageBoxA),
                reinterpret_cast<PVOID>(MyMessageBoxA), "MessageBoxA detour") ||
            !GameServerRuntimeSafety::AttachDetour(
                reinterpret_cast<PVOID*>(&pfnOrigMessageBoxW),
                reinterpret_cast<PVOID>(MyMessageBoxW), "MessageBoxW detour"))
        {
            RemoveSystemMessageHooks();
            GameServerConsole::WriteFailure("System message hook installation failed");
            return ERROR_DLL_INIT_FAILED;
        }


        pfnOrigGetCharName = reinterpret_cast<fnGetCharName>(GOBJ_GET_CHAR_NAME_FUNC_OFFSET);
        pfnOrigGetNickName = reinterpret_cast<fnGetNickName>(GOBJ_GET_NICK_NAME_FUNC_OFFSET);

        //DetourTransactionBegin();
        //DetourAttach(&(PVOID&)pfnOrigGetCharName, MyGetCharName);
        //DetourAttach(&(PVOID&)pfnOrigGetNickName, MyGetNickName);
        //DetourTransactionCommit();

        if (!Init())
        {
            RemoveSystemMessageHooks();
            GameServerConsole::WriteFailure("GameServer add-on initialization stopped safely");
            return ERROR_DLL_INIT_FAILED;
        }

        GameServerRuntimeSafety::MarkInitializationReady();
        GameServerConsole::WriteSuccess("Security, packet guards and telemetry are active");
        SetConsoleTitleA("KMTGuard GameServer | READY");
        return 0;
}

static DWORD WINAPI InitializeGameServerAddon(LPVOID parameter)
{
    DWORD result = ERROR_DLL_INIT_FAILED;
    __try
    {
        result = InitializeGameServerAddonCore(reinterpret_cast<HMODULE>(parameter));
    }
    __except (GameServerCrashHandler::HandleException(GetExceptionInformation()))
    {
        result = ERROR_DLL_INIT_FAILED;
    }

    if (result != ERROR_SUCCESS)
    {
        GameServerRuntimeSafety::MarkInitializationFailed();
        GameServerConsole::WriteFailure("Fail-closed shutdown: required protection did not initialize");
        TerminateProcess(GetCurrentProcess(), result);
    }
    return result;
}

extern "C" _declspec(dllexport) BOOL WINAPI DllMain(HINSTANCE hModule, DWORD fdwReason, LPVOID lpReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        if (!GameServerRuntimeSafety::ValidateHost())
            return FALSE;
        if (!GameServerRuntimeSafety::BeginInitialization())
            return FALSE;

        HMODULE kernel32 = GetModuleHandleA("Kernel32.dll");
        fnGetModuleHandleExACompat pinModule = kernel32 == NULL ? NULL
            : reinterpret_cast<fnGetModuleHandleExACompat>(
                GetProcAddress(kernel32, "GetModuleHandleExA"));
        if (pinModule == NULL || !pinModule(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                reinterpret_cast<LPCSTR>(&DllMain), &s_processLifetimeModule))
        {
            GameServerRuntimeSafety::MarkInitializationFailed();
            return FALSE;
        }

        // Publish the custom-packet gate before the loader resumes the host.
        // Native vSRO traffic continues through the original handler, while
        // privileged 0x35xx commands fail closed until every prerequisite is
        // ready. The module is later pinned for process life; hot unload is
        // intentionally unsupported while hook targets exist.
        if (!GameServerRuntimeSafety::ReplacePointer(
                0x00AF5FDC, 0x0050EEE0,
                static_cast<DWORD>(addr_from_this(&CGObjPC::ReaderPacket)),
                "bootstrap custom-packet gate"))
        {
            GameServerRuntimeSafety::MarkInitializationFailed();
            return FALSE;
        }
        HANDLE thread = CreateThread(NULL, 0, InitializeGameServerAddon, hModule, 0, NULL);
        if (thread == NULL) {
            GameServerRuntimeSafety::ReplacePointer(
                0x00AF5FDC,
                static_cast<DWORD>(addr_from_this(&CGObjPC::ReaderPacket)),
                0x0050EEE0,
                "bootstrap custom-packet gate rollback");
            GameServerRuntimeSafety::MarkInitializationFailed();
            return FALSE;
        }
        CloseHandle(thread);
    }

    return TRUE;
}
