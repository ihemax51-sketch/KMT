#include "GameServerCrashHandler.h"

#include <DbgHelp.h>
#include <stdio.h>
#include <KmtGuardProductVersion.h>
#include <KMTGuardCustom/GameServerTelemetry.h>

namespace
{
    typedef BOOL (WINAPI* MiniDumpWriteDumpFn)(
        HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
        PMINIDUMP_EXCEPTION_INFORMATION,
        PMINIDUMP_USER_STREAM_INFORMATION,
        PMINIDUMP_CALLBACK_INFORMATION);

    volatile LONG s_dumpInProgress = 0;
    char s_dumpDirectory[MAX_PATH] = { 0 };

    void BuildDumpDirectory()
    {
        char executablePath[MAX_PATH] = { 0 };
        if (GetModuleFileNameA(NULL, executablePath, MAX_PATH) == 0)
        {
            lstrcpynA(s_dumpDirectory, ".\\KMTGuard-Dumps", MAX_PATH);
            return;
        }

        char* separator = strrchr(executablePath, '\\');
        if (separator != NULL)
            *separator = '\0';
        _snprintf(s_dumpDirectory, MAX_PATH - 1, "%s\\KMTGuard-Dumps", executablePath);
        s_dumpDirectory[MAX_PATH - 1] = '\0';
    }

    void BuildCrashPath(char* path, size_t pathSize, const char* extension)
    {
        SYSTEMTIME now = { 0 };
        GetLocalTime(&now);
        _snprintf(path, pathSize - 1,
            "%s\\KMTGuard-GameServer-%04u%02u%02u-%02u%02u%02u-p%lu-t%lu.%s",
            s_dumpDirectory,
            now.wYear, now.wMonth, now.wDay,
            now.wHour, now.wMinute, now.wSecond,
            static_cast<unsigned long>(GetCurrentProcessId()),
            static_cast<unsigned long>(GetCurrentThreadId()),
            extension);
        path[pathSize - 1] = '\0';
    }

    bool WriteMiniDump(EXCEPTION_POINTERS* exceptionPointers, const char* dumpPath)
    {
        HMODULE dbgHelp = LoadLibraryA("dbghelp.dll");
        if (dbgHelp == NULL)
            return false;

        MiniDumpWriteDumpFn writeDump = reinterpret_cast<MiniDumpWriteDumpFn>(
            GetProcAddress(dbgHelp, "MiniDumpWriteDump"));
        if (writeDump == NULL)
        {
            FreeLibrary(dbgHelp);
            return false;
        }

        HANDLE dumpFile = CreateFileA(
            dumpPath, GENERIC_WRITE, FILE_SHARE_READ, NULL,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (dumpFile == INVALID_HANDLE_VALUE)
        {
            FreeLibrary(dbgHelp);
            return false;
        }

        MINIDUMP_EXCEPTION_INFORMATION exceptionInfo = { 0 };
        exceptionInfo.ThreadId = GetCurrentThreadId();
        exceptionInfo.ExceptionPointers = exceptionPointers;
        exceptionInfo.ClientPointers = FALSE;

        const MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(
            MiniDumpNormal | MiniDumpWithDataSegs | MiniDumpWithHandleData);
        const BOOL result = writeDump(
            GetCurrentProcess(), GetCurrentProcessId(), dumpFile, dumpType,
            exceptionPointers != NULL ? &exceptionInfo : NULL,
            NULL, NULL);

        CloseHandle(dumpFile);
        FreeLibrary(dbgHelp);
        return result != FALSE;
    }

    void WriteCrashSummary(EXCEPTION_POINTERS* exceptionPointers, const char* dumpPath, bool dumpWritten)
    {
        char summaryPath[MAX_PATH] = { 0 };
        BuildCrashPath(summaryPath, sizeof(summaryPath), "txt");
        HANDLE file = CreateFileA(
            summaryPath, GENERIC_WRITE, FILE_SHARE_READ, NULL,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (file == INVALID_HANDLE_VALUE)
            return;

        const DWORD code = exceptionPointers != NULL && exceptionPointers->ExceptionRecord != NULL
            ? exceptionPointers->ExceptionRecord->ExceptionCode : 0;
        const void* address = exceptionPointers != NULL && exceptionPointers->ExceptionRecord != NULL
            ? exceptionPointers->ExceptionRecord->ExceptionAddress : NULL;
        char text[1024] = { 0 };
        const int length = _snprintf(text, sizeof(text) - 1,
            "KMTGuard GameServer crash report\r\n"
            "AddonVersion: %s\r\n"
            "ProcessId: %lu\r\nThreadId: %lu\r\n"
            "ExceptionCode: 0x%08lX\r\nExceptionAddress: %p\r\n"
            "LastClientOpcode: 0x%04X\r\nLastServerOpcode: 0x%04X\r\n"
            "DumpWritten: %s\r\nDumpPath: %s\r\n",
            KMTGUARD_VERSION_STRING,
            static_cast<unsigned long>(GetCurrentProcessId()),
            static_cast<unsigned long>(GetCurrentThreadId()),
            static_cast<unsigned long>(code), address,
            GameServerTelemetry::GetLastClientOpcode(),
            GameServerTelemetry::GetLastServerOpcode(),
            dumpWritten ? "yes" : "no", dumpPath);
        DWORD written = 0;
        if (length > 0)
            WriteFile(file, text, static_cast<DWORD>(length), &written, NULL);
        CloseHandle(file);
    }

    LONG WINAPI UnhandledGameServerException(EXCEPTION_POINTERS* exceptionPointers)
    {
        return GameServerCrashHandler::HandleException(exceptionPointers);
    }
}

void GameServerCrashHandler::Initialize(HMODULE)
{
    BuildDumpDirectory();
    CreateDirectoryA(s_dumpDirectory, NULL);
    SetUnhandledExceptionFilter(UnhandledGameServerException);
}

LONG GameServerCrashHandler::HandleException(EXCEPTION_POINTERS* exceptionPointers)
{
    if (InterlockedCompareExchange(&s_dumpInProgress, 1, 0) != 0)
        return EXCEPTION_EXECUTE_HANDLER;

    CreateDirectoryA(s_dumpDirectory, NULL);
    char dumpPath[MAX_PATH] = { 0 };
    BuildCrashPath(dumpPath, sizeof(dumpPath), "dmp");
    const bool dumpWritten = WriteMiniDump(exceptionPointers, dumpPath);
    WriteCrashSummary(exceptionPointers, dumpPath, dumpWritten);

    char debugLine[MAX_PATH + 96] = { 0 };
    _snprintf(debugLine, sizeof(debugLine) - 1,
        "[KMTGuard] Fatal GameServer exception; crash dump: %s\n", dumpPath);
    OutputDebugStringA(debugLine);
    return EXCEPTION_EXECUTE_HANDLER;
}
