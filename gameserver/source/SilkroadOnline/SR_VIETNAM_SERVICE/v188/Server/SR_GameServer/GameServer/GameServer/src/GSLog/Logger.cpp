#include <cstdarg>
#include <cstdio>
#include <ctime>
#include "Logger.h"

const char* arrLogLevelText[] =
        {
                "Normal",
                "Warning",
                "Error",
                "Fatal"
        };

void CLogger::WriteLogA(int nLogLevel, const char* szMsg, ...)
{
    if (szMsg == NULL)
        return;

    if (nLogLevel < LOG_NORMAL || nLogLevel > LOG_FATAL)
        nLogLevel = LOG_ERROR;

    va_list lstVA;
    va_start(lstVA, szMsg);

    char szBuffer[LOG_MSG_BUF_LEN] = { 0 };
    _vsnprintf_s(szBuffer, sizeof(szBuffer), _TRUNCATE, szMsg, lstVA);
    va_end(lstVA);

    time_t now = time(0);
    struct tm nowTm = { 0 };
    localtime_s(&nowTm, &now);
    char szSysTime[100] = { 0 };
    strftime(szSysTime, sizeof(szSysTime), "%Y-%m-%d %H:%M:%S", &nowTm);

    char szTmpBuffer[LOG_MSG_BUF_LEN] = { 0 };
    _snprintf_s(
        szTmpBuffer,
        sizeof(szTmpBuffer),
        _TRUNCATE,
        "[%s] %s | %s\n",
        arrLogLevelText[nLogLevel],
        szSysTime,
        szBuffer);

    printf("%s", szTmpBuffer);
}
