#include "BSObj.h"
#include "../Console/ShardManagerConsole.h"

#include <Windows.h>
#include <cstdio>
#include <cstdarg>

void ReportConsole(int nLogLevel, const char* format, ...)
{
    if (format == NULL)
        return;

    char buffer[2048] = { 0 };
    va_list arguments;
    va_start(arguments, format);
    _vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, arguments);
    va_end(arguments);

    ShardManagerConsole::WriteFormat(nLogLevel, "%s", buffer);
}

const char* GetReportConsoleType(int type)
{
    switch (type)
    {
    case PUT_DEBUG_LOW: return "DEBUG";
    case PUT_DEBUG: return "DEBUG";
    case PUT_INFO: return "INFO";
    case PUT_WARNING: return "WARN";
    case PUT_ERROR: return "ERROR";
    case PUT_CRITICAL: return "CRITICAL";
    default: return "INFO";
    }
}

void MsgBoxError(const char* format, ...)
{
    if (format == NULL)
        return;

    char buffer[1024] = { 0 };
    va_list arguments;
    va_start(arguments, format);
    _vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, arguments);
    va_end(arguments);

    ShardManagerConsole::WriteFailure(buffer);

#ifndef CONFIG_DEBUG_REDIRECT_MSGBOX
    wchar_t wideBuffer[1024] = { 0 };
    size_t converted = 0;
    mbstowcs_s(&converted, wideBuffer, buffer, _TRUNCATE);
    MessageBoxW(NULL, wideBuffer, L"KMTGuard ShardManager", MB_OK | MB_ICONERROR);
#endif
}
