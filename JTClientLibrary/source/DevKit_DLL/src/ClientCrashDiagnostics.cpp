#include "ClientCrashDiagnostics.h"

#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <string.h>

namespace
{
    LONG WINAPI WriteClientCrashDump(EXCEPTION_POINTERS* exceptionPointers)
    {
        char executablePath[MAX_PATH] = {0};
        if (GetModuleFileNameA(NULL, executablePath, MAX_PATH - 1) == 0)
            return EXCEPTION_CONTINUE_SEARCH;

        char* separator = strrchr(executablePath, '\\');
        if (separator != NULL)
            *separator = '\0';

        char settingsDirectory[MAX_PATH] = {0};
        _snprintf(settingsDirectory, MAX_PATH - 1, "%s\\Setting", executablePath);
        settingsDirectory[MAX_PATH - 1] = '\0';
        CreateDirectoryA(settingsDirectory, NULL);

        char dumpPath[MAX_PATH] = {0};
        _snprintf(dumpPath, MAX_PATH - 1, "%s\\KMTGuardKit-crash.dmp", settingsDirectory);
        dumpPath[MAX_PATH - 1] = '\0';

        HANDLE dumpFile = CreateFileA(
            dumpPath,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (dumpFile != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
            dumpInfo.ThreadId = GetCurrentThreadId();
            dumpInfo.ExceptionPointers = exceptionPointers;
            dumpInfo.ClientPointers = FALSE;
            MiniDumpWriteDump(
                GetCurrentProcess(),
                GetCurrentProcessId(),
                dumpFile,
                MiniDumpNormal,
                exceptionPointers != NULL ? &dumpInfo : NULL,
                NULL,
                NULL);
            FlushFileBuffers(dumpFile);
            CloseHandle(dumpFile);
        }

        char textPath[MAX_PATH] = {0};
        _snprintf(textPath, MAX_PATH - 1, "%s\\KMTGuardKit-crash.txt", settingsDirectory);
        textPath[MAX_PATH - 1] = '\0';

        HANDLE textFile = CreateFileA(
            textPath,
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);

        if (textFile != INVALID_HANDLE_VALUE) {
            DWORD exceptionCode = 0;
            void* exceptionAddress = NULL;
            if (exceptionPointers != NULL && exceptionPointers->ExceptionRecord != NULL) {
                exceptionCode = exceptionPointers->ExceptionRecord->ExceptionCode;
                exceptionAddress = exceptionPointers->ExceptionRecord->ExceptionAddress;
            }

            char message[256] = {0};
            _snprintf(
                message,
                sizeof(message) - 1,
                "KMTGuardKit unhandled exception\r\nCode: 0x%08lX\r\nAddress: %p\r\nThread: %lu\r\n",
                exceptionCode,
                exceptionAddress,
                GetCurrentThreadId());
            message[sizeof(message) - 1] = '\0';

            DWORD written = 0;
            WriteFile(textFile, message, static_cast<DWORD>(strlen(message)), &written, NULL);
            FlushFileBuffers(textFile);
            CloseHandle(textFile);
        }

        return EXCEPTION_CONTINUE_SEARCH;
    }
}

void InstallClientCrashDiagnostics()
{
    SetUnhandledExceptionFilter(WriteClientCrashDump);
}
