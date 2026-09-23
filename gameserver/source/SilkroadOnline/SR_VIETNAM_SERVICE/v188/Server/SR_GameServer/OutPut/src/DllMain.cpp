
#include <stdio.h>
#include "Util.h"
#include <memory/hook.h>
#include <memory/MemoryUtility.h>
#include <memory/detours.h>
#include "GameServerConsole.h"
#include "GameServerCrashHandler.h"
#include <KMTGuardCustom/GameServerRuntimeSafety.h>

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
    LONG result = DetourTransactionBegin();
    if (result == NO_ERROR)
        result = DetourUpdateThread(GetCurrentThread());
    if (result == NO_ERROR)
        result = DetourDetach(&(PVOID&)pfnOrigMessageBoxW, MyMessageBoxW);
    if (result == NO_ERROR)
        result = DetourDetach(&(PVOID&)pfnOrigMessageBoxA, MyMessageBoxA);
    if (result == NO_ERROR)
        DetourTransactionCommit();
    else
        DetourTransactionAbort();
}

static DWORD InitializeGameServerAddonCore(HMODULE hModule)
{
        GameServerConsole::Initialize();
        HMODULE hKernel32 = GetModuleHandleA("Kernel32.dll");
        fnGetModuleHandleExACompat getModuleHandleExA = hKernel32 == NULL
            ? NULL
            : reinterpret_cast<fnGetModuleHandleExACompat>(
                GetProcAddress(hKernel32, "GetModuleHandleExA"));
        if (getModuleHandleExA == NULL || !getModuleHandleExA(
                GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                reinterpret_cast<LPCSTR>(&InitializeGameServerAddonCore),
                &s_processLifetimeModule))
        {
            GameServerConsole::WriteFailure("GameServer add-on process-lifetime pin failed");
            return ERROR_DLL_INIT_FAILED;
        }
        GameServerCrashHandler::Initialize(hModule);

        if (!GameServerRuntimeSafety::ValidateHost())
        {
            GameServerConsole::WriteFailure("Unsupported or modified SR_GameServer.exe");
            return ERROR_BAD_EXE_FORMAT;
        }

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

        LONG detourResult = DetourTransactionBegin();
        if (detourResult == NO_ERROR)
            detourResult = DetourUpdateThread(GetCurrentThread());
        if (detourResult == NO_ERROR)
            detourResult = DetourAttach(&(PVOID&)pfnOrigMessageBoxA, MyMessageBoxA);
        if (detourResult == NO_ERROR)
            detourResult = DetourAttach(&(PVOID&)pfnOrigMessageBoxW, MyMessageBoxW);
        if (detourResult == NO_ERROR)
            detourResult = DetourTransactionCommit();
        else
            DetourTransactionAbort();
        if (detourResult != NO_ERROR)
        {
            GameServerConsole::WriteFailure("System message hook installation failed");
            return detourResult;
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
        GameServerConsole::WriteFailure("Fail-closed shutdown: required protection did not initialize");
        TerminateProcess(GetCurrentProcess(), result);
    }
    return result;
}

extern "C" _declspec(dllexport) BOOL WINAPI DllMain(HINSTANCE hModule, DWORD fdwReason, LPVOID lpReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        HANDLE thread = CreateThread(NULL, 0, InitializeGameServerAddon, hModule, 0, NULL);
        if (thread == NULL)
            return FALSE;
        CloseHandle(thread);
    }

    return TRUE;
}
