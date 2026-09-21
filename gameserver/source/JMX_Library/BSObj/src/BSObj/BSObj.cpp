///******************************************************************************
/// \File BSObj.cpp
///
/// \Desc
///
/// \Author kyuubi09 on 7/9/2023.
///
/// \Copyright Unpublished Work © 2023 Kyuubi.
///
///******************************************************************************

#include "BSObj.h"

#include <cstdio>
#include <cstdarg>
#include <Windows.h>

void ReportConsole(int nLogLevel, const char *fmt, ...) {
    va_list args;
            va_start(args, fmt);

    printf("%s", GetReportConsoleType(nLogLevel));
    vprintf(fmt, args);
    putchar('\n');
}

const char *GetReportConsoleType(int nType) {
    switch (nType) {
        case PUT_DEBUG_LOW:
            return "[DEBUG_LOW]-> ";
        case PUT_DEBUG:
            return "[DEBUG]-> ";
        case PUT_INFO:
            return "[INFO]-> ";
        case PUT_WARNING:
            return "[WARNING]-> ";
        case PUT_ERROR:
            return "[ERROR]-> ";
        case PUT_CRITICAL:
            return "[CRITICAL]-> ";
        default:
            return "-> ";
    }
}

void MsgBoxError(const char *lpsczText, ...) {
    va_list args;
            va_start(args, lpsczText);

    if (lstrlenA(lpsczText) >= 1024)
        return;

    char buffer[1024] = {0};

    int nCount = vsnprintf_s(buffer, sizeof(buffer), sizeof(buffer) - 1, lpsczText, args);
    if ((nCount < 0) || (0x3ff <= nCount)) {
        buffer[1023] = '\0';
    }

#ifdef CONFIG_DEBUG_REDIRECT_MSGBOX
    BS_ERROR(buffer)
#else
    MessageBox(NULL, buffer, "BSObj Plugin", NULL);
#endif // CONFIG_DEBUG_REDIRECT_MSGBOX
}