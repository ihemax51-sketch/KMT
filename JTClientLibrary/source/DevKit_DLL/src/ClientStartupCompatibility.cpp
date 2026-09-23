#include "ClientStartupCompatibility.h"

#include <windows.h>
#include <stdio.h>
#include <string.h>

namespace
{
    const DWORD kStartupLogMaximumBytes = 1024 * 1024;
    const DWORD kCObjChildFinalListClearCallSite = 0x00B92BA5;
    const DWORD kNativeListClear = 0x00571540;
    const DWORD kNativeListSentinelCreate = 0x005CED50;
    volatile LONG g_clientInitializationState = 0;
    volatile LONG g_invalidCObjChildTeardownCount = 0;
    volatile LONG g_startupLogLock = 0;
    HANDLE g_startupLog = INVALID_HANDLE_VALUE;

    void BuildStartupLogPath(char* path, size_t pathSize)
    {
        if (path == NULL || pathSize == 0)
            return;

        path[0] = '\0';
        char executablePath[MAX_PATH] = {0};
        if (GetModuleFileNameA(NULL, executablePath, MAX_PATH - 1) == 0)
            return;

        char* separator = strrchr(executablePath, '\\');
        if (separator == NULL)
            return;

        *separator = '\0';

        char settingDirectory[MAX_PATH] = {0};
        _snprintf(settingDirectory, MAX_PATH - 1, "%s\\Setting", executablePath);
        settingDirectory[MAX_PATH - 1] = '\0';
        CreateDirectoryA(settingDirectory, NULL);

        _snprintf(path, pathSize - 1, "%s\\KMTGuardKit-startup.log", settingDirectory);
        path[pathSize - 1] = '\0';
    }

    void WriteInvalidCObjChildTeardownDiagnostic(void* listObject, LONG occurrence)
    {
        BYTE* owner = reinterpret_cast<BYTE*>(listObject) - 0x14;
        char message[640] = {0};
        _snprintf(message, sizeof(message) - 1,
            "mBot compatibility: recovered invalid CObjChild teardown state occurrence=%ld owner=0x%08lX list=0x%08lX parent=0x%08lX next=0x%08lX previous=0x%08lX count=%lu.",
            occurrence, reinterpret_cast<DWORD>(owner), reinterpret_cast<DWORD>(listObject),
            *reinterpret_cast<DWORD*>(owner + 0x04), *reinterpret_cast<DWORD*>(owner + 0x08),
            *reinterpret_cast<DWORD*>(owner + 0x10),
            *reinterpret_cast<DWORD*>(reinterpret_cast<BYTE*>(listObject) + 0x08));
        message[sizeof(message) - 1] = '\0';
        WriteClientStartupDiagnostic(message);
    }

    void __fastcall ClearCObjChildListSafely(void* listObject, void*)
    {
        BYTE* list = reinterpret_cast<BYTE*>(listObject);
        DWORD* head = reinterpret_cast<DWORD*>(list + 0x04);
        if (*head != 0) {
            reinterpret_cast<void (__thiscall *)(void*)>(kNativeListClear)(listObject);
            return;
        }
        *head = reinterpret_cast<DWORD (__cdecl *)()>(kNativeListSentinelCreate)();
        *reinterpret_cast<DWORD*>(list + 0x08) = 0;
        const LONG occurrence = InterlockedIncrement(&g_invalidCObjChildTeardownCount);
        if (occurrence <= 32)
            WriteInvalidCObjChildTeardownDiagnostic(listObject, occurrence);
    }
}

bool InstallCObjChildTeardownCompatibilityGuard()
{
    BYTE* callSite = reinterpret_cast<BYTE*>(kCObjChildFinalListClearCallSite);
    if (callSite[0] != 0xE8) {
        WriteClientStartupDiagnostic("CObjChild teardown compatibility guard skipped: expected CALL opcode was not present.");
        return false;
    }
    const DWORD guardTarget = reinterpret_cast<DWORD>(&ClearCObjChildListSafely);
    const LONG currentDisplacement = *reinterpret_cast<const LONG*>(callSite + 1);
    const DWORD currentTarget = kCObjChildFinalListClearCallSite + 5 + currentDisplacement;
    if (currentTarget == guardTarget)
        return true;
    if (currentTarget != kNativeListClear) {
        char message[256] = {0};
        _snprintf(message, sizeof(message) - 1,
            "CObjChild teardown compatibility guard skipped: call target 0x%08lX is owned by another module.",
            currentTarget);
        message[sizeof(message) - 1] = '\0';
        WriteClientStartupDiagnostic(message);
        return false;
    }
    const LONG guardDisplacement = static_cast<LONG>(guardTarget - kCObjChildFinalListClearCallSite - 5);
    DWORD oldProtection = 0;
    if (!VirtualProtect(callSite + 1, sizeof(guardDisplacement), PAGE_EXECUTE_READWRITE, &oldProtection)) {
        WriteClientStartupDiagnostic("CObjChild teardown compatibility guard failed: VirtualProtect failed.");
        return false;
    }
    memcpy(callSite + 1, &guardDisplacement, sizeof(guardDisplacement));
    const BOOL cacheFlushed = FlushInstructionCache(GetCurrentProcess(), callSite, 5);
    DWORD ignoredProtection = 0;
    const BOOL protectionRestored = VirtualProtect(callSite + 1, sizeof(guardDisplacement), oldProtection, &ignoredProtection);
    const LONG installedDisplacement = *reinterpret_cast<const LONG*>(callSite + 1);
    if (!cacheFlushed || !protectionRestored || installedDisplacement != guardDisplacement) {
        WriteClientStartupDiagnostic("CObjChild teardown compatibility guard failed: patch verification failed.");
        return false;
    }
    WriteClientStartupDiagnostic("CObjChild teardown compatibility guard installed for the final native child-list clear.");
    return true;
}

bool BeginClientInitialization()
{
    return InterlockedCompareExchange(
        &g_clientInitializationState,
        1,
        0) == 0;
}

void MarkClientInitializationSucceeded()
{
    InterlockedExchange(&g_clientInitializationState, 2);
}

void MarkClientInitializationFailed()
{
    InterlockedExchange(&g_clientInitializationState, -1);
}

bool WaitForClientInitialization(DWORD timeoutMilliseconds)
{
    const DWORD startedAt = GetTickCount();

    for (;;) {
        const LONG state = InterlockedCompareExchange(
            &g_clientInitializationState,
            0,
            0);
        if (state == 2)
            return true;
        if (state < 0)
            return false;
        if (GetTickCount() - startedAt >= timeoutMilliseconds)
            return false;

        Sleep(1);
    }
}

void WriteClientStartupDiagnostic(const char* message)
{
    if (message == NULL)
        return;

    while (InterlockedCompareExchange(&g_startupLogLock, 1, 0) != 0)
        Sleep(0);

    if (g_startupLog == INVALID_HANDLE_VALUE) {
        char logPath[MAX_PATH] = {0};
        BuildStartupLogPath(logPath, MAX_PATH);
        if (logPath[0] == '\0') {
            InterlockedExchange(&g_startupLogLock, 0);
            return;
        }

        DWORD creationDisposition = OPEN_ALWAYS;
        WIN32_FILE_ATTRIBUTE_DATA attributes;
        ZeroMemory(&attributes, sizeof(attributes));
        if (GetFileAttributesExA(logPath, GetFileExInfoStandard, &attributes) &&
            (attributes.nFileSizeHigh != 0 ||
             attributes.nFileSizeLow >= kStartupLogMaximumBytes)) {
            creationDisposition = CREATE_ALWAYS;
        }

        g_startupLog = CreateFileA(logPath, GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, creationDisposition,
            FILE_ATTRIBUTE_NORMAL, NULL);
        if (g_startupLog == INVALID_HANDLE_VALUE) {
            InterlockedExchange(&g_startupLogLock, 0);
            return;
        }
        SetFilePointer(g_startupLog, 0, NULL, FILE_END);
    }


    SYSTEMTIME now;
    GetLocalTime(&now);
    char line[768] = {0};
    _snprintf(
        line,
        sizeof(line) - 1,
        "%04u-%02u-%02u %02u:%02u:%02u.%03u [pid:%lu tid:%lu] %s\r\n",
        now.wYear,
        now.wMonth,
        now.wDay,
        now.wHour,
        now.wMinute,
        now.wSecond,
        now.wMilliseconds,
        GetCurrentProcessId(),
        GetCurrentThreadId(),
        message);
    line[sizeof(line) - 1] = '\0';

    DWORD written = 0;
    WriteFile(g_startupLog, line, static_cast<DWORD>(strlen(line)), &written, NULL);
    FlushFileBuffers(g_startupLog);
    InterlockedExchange(&g_startupLogLock, 0);
}
