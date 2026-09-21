#pragma once


#define LOG_NORMAL			0
#define LOG_WARNING			1
#define LOG_ERROR			2
#define LOG_FATAL			3

#define LOG_MSG_BUF_LEN				1024


#define LOG_WRITE(level, msg, ...) \
	CLogger::WriteLogA(level, msg, __VA_ARGS__)


class CLogger
{
private:

public:
    static void WriteLogA(int nLogLevel, const char* szMsg, ...);
};
