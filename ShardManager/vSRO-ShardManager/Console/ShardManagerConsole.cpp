#include "ShardManagerConsole.h"

#include <Windows.h>
#include <Psapi.h>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <iostream>
#include <streambuf>
#include <string>
#include <KmtGuardProductVersion.h>

#ifndef ENABLE_QUICK_EDIT_MODE
#define ENABLE_QUICK_EDIT_MODE 0x0040
#endif
#ifndef ENABLE_EXTENDED_FLAGS
#define ENABLE_EXTENDED_FLAGS 0x0080
#endif

namespace
{
    const char* LOG_FILE = "KMTGuard-ShardManager.log";
    const char* LOG_BACKUP_FILE = "KMTGuard-ShardManager.1.log";
    const DWORD MAX_LOG_BYTES = 5 * 1024 * 1024;

    HANDLE s_output = INVALID_HANDLE_VALUE;
    CRITICAL_SECTION s_writeLock;
    volatile LONG s_lockState = 0;
    volatile LONG s_initialized = 0;
	volatile LONG s_monitorStarted = 0;

    enum ConsoleLevel
    {
        LevelDebug = 1,
        LevelInfo = 2,
        LevelWarning = 3,
        LevelError = 4,
        LevelFailure = 5,
        LevelSuccess = 6
    };

    void EnsureLock()
    {
        if (InterlockedCompareExchange(&s_lockState, 1, 0) == 0)
        {
            InitializeCriticalSection(&s_writeLock);
            InterlockedExchange(&s_lockState, 2);
            return;
        }

        while (InterlockedCompareExchange(&s_lockState, 2, 2) != 2)
            Sleep(0);
    }

    const char* LevelName(int level)
    {
        switch (level)
        {
        case LevelDebug: return "DEBUG";
        case LevelWarning: return "WARN";
        case LevelSuccess: return "READY";
        case LevelError:
        case LevelFailure: return "ERROR";
        default: return "INFO";
        }
    }

    WORD LevelColor(int level)
    {
        switch (level)
        {
        case LevelDebug:
            return FOREGROUND_BLUE | FOREGROUND_GREEN;
        case LevelWarning:
            return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        case LevelSuccess:
            return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        case LevelError:
        case LevelFailure:
            return FOREGROUND_RED | FOREGROUND_INTENSITY;
        default:
            return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        }
    }

    void RotateLogIfNeeded()
    {
        WIN32_FILE_ATTRIBUTE_DATA data;
        if (!GetFileAttributesExA(LOG_FILE, GetFileExInfoStandard, &data))
            return;

        ULARGE_INTEGER size;
        size.HighPart = data.nFileSizeHigh;
        size.LowPart = data.nFileSizeLow;
        if (size.QuadPart < MAX_LOG_BYTES)
            return;

        DeleteFileA(LOG_BACKUP_FILE);
        MoveFileExA(LOG_FILE, LOG_BACKUP_FILE, MOVEFILE_REPLACE_EXISTING);
    }

    void AppendLog(const char* line)
    {
        if (line == NULL)
            return;

        RotateLogIfNeeded();
        HANDLE file = CreateFileA(
            LOG_FILE,
            FILE_APPEND_DATA,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (file == INVALID_HANDLE_VALUE)
            return;

        DWORD written = 0;
        WriteFile(file, line, static_cast<DWORD>(std::strlen(line)), &written, NULL);
        CloseHandle(file);
    }

    void WriteLine(int level, const char* text)
    {
        if (text == NULL || text[0] == '\0')
            return;

        EnsureLock();
        EnterCriticalSection(&s_writeLock);

        SYSTEMTIME time;
        GetLocalTime(&time);

        char line[2304] = { 0 };
        _snprintf_s(
            line,
            sizeof(line),
            _TRUNCATE,
            "[%02u:%02u:%02u] [%-5s] %s\r\n",
            time.wHour,
            time.wMinute,
            time.wSecond,
            LevelName(level),
            text);

        if (s_output != INVALID_HANDLE_VALUE)
            SetConsoleTextAttribute(s_output, LevelColor(level));
        std::printf("%s", line);
        std::fflush(stdout);
        if (s_output != INVALID_HANDLE_VALUE)
            SetConsoleTextAttribute(
                s_output,
                FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

        AppendLog(line);
        LeaveCriticalSection(&s_writeLock);
    }

    int InferLevel(const std::string& line)
    {
        if (line.find("Exception") != std::string::npos ||
            line.find("failed") != std::string::npos ||
            line.find("Error") != std::string::npos)
            return LevelFailure;
        if (line.find("stopped") != std::string::npos)
            return LevelWarning;
        return LevelInfo;
    }

    std::string NormalizeLegacyLine(const std::string& line)
    {
        size_t start = 0;
        if (line.compare(0, 9, "[INFO]-> ") == 0)
            start = 9;
        else
        {
            while (start < line.size() && (line[start] == ' ' || line[start] == '\t'))
                ++start;
        }
        return line.substr(start);
    }

    class NarrowConsoleBuffer : public std::streambuf
    {
    public:
        virtual int_type overflow(int_type value)
        {
            if (traits_type::eq_int_type(value, traits_type::eof()))
                return traits_type::not_eof(value);
            const char character = traits_type::to_char_type(value);
            if (character == '\n')
                FlushLine();
            else if (character != '\r')
                m_line.push_back(character);
            return value;
        }

        virtual int sync()
        {
            FlushLine();
            return 0;
        }

    private:
        void FlushLine()
        {
            if (m_line.empty())
                return;
            const std::string normalized = NormalizeLegacyLine(m_line);
            if (!normalized.empty())
                WriteLine(InferLevel(normalized), normalized.c_str());
            m_line.clear();
        }

        std::string m_line;
    };

    class WideConsoleBuffer : public std::wstreambuf
    {
    public:
        virtual int_type overflow(int_type value)
        {
            if (traits_type::eq_int_type(value, traits_type::eof()))
                return traits_type::not_eof(value);
            const wchar_t character = traits_type::to_char_type(value);
            if (character == L'\n')
                FlushLine();
            else if (character != L'\r')
                m_line.push_back(character);
            return value;
        }

        virtual int sync()
        {
            FlushLine();
            return 0;
        }

    private:
        void FlushLine()
        {
            if (m_line.empty())
                return;
            const int length = WideCharToMultiByte(
                CP_UTF8, 0, m_line.c_str(), -1, NULL, 0, NULL, NULL);
            if (length > 1)
            {
                std::string text(static_cast<size_t>(length), '\0');
                WideCharToMultiByte(
                    CP_UTF8, 0, m_line.c_str(), -1, &text[0], length, NULL, NULL);
                text.resize(static_cast<size_t>(length - 1));
                const std::string normalized = NormalizeLegacyLine(text);
                if (!normalized.empty())
                    WriteLine(InferLevel(normalized), normalized.c_str());
            }
            m_line.clear();
        }

        std::wstring m_line;
    };

    NarrowConsoleBuffer s_narrowStreamBuffer;
    WideConsoleBuffer s_wideStreamBuffer;

    void RedirectLegacyStreams()
    {
        std::cout.rdbuf(&s_narrowStreamBuffer);
        std::cerr.rdbuf(&s_narrowStreamBuffer);
        std::wcout.rdbuf(&s_wideStreamBuffer);
        std::wcerr.rdbuf(&s_wideStreamBuffer);
    }

    unsigned long long FileTimeValue(const FILETIME& value)
    {
        ULARGE_INTEGER result;
        result.LowPart = value.dwLowDateTime;
        result.HighPart = value.dwHighDateTime;
        return result.QuadPart;
    }

    DWORD WINAPI StatusMonitor(LPVOID)
    {
        SYSTEM_INFO systemInfo;
        GetSystemInfo(&systemInfo);
        const DWORD processorCount = systemInfo.dwNumberOfProcessors > 0
            ? systemInfo.dwNumberOfProcessors
            : 1;

        FILETIME creation = { 0 };
        FILETIME exitTime = { 0 };
        FILETIME kernel = { 0 };
        FILETIME user = { 0 };
        unsigned long long previousCpu = 0;
        unsigned long long previousTick = GetTickCount64();
        if (GetProcessTimes(GetCurrentProcess(), &creation, &exitTime, &kernel, &user))
            previousCpu = FileTimeValue(kernel) + FileTimeValue(user);

        for (;;)
        {
            Sleep(5000);

            double cpuPercent = 0.0;
            const unsigned long long nowTick = GetTickCount64();
            if (GetProcessTimes(GetCurrentProcess(), &creation, &exitTime, &kernel, &user))
            {
                const unsigned long long currentCpu = FileTimeValue(kernel) + FileTimeValue(user);
                const unsigned long long elapsedMs = nowTick - previousTick;
                if (elapsedMs > 0 && currentCpu >= previousCpu)
                {
                    cpuPercent = static_cast<double>(currentCpu - previousCpu) /
                        (static_cast<double>(elapsedMs) * 10000.0 * processorCount) * 100.0;
                    if (cpuPercent > 100.0)
                        cpuPercent = 100.0;
                }
                previousCpu = currentCpu;
                previousTick = nowTick;
            }

            PROCESS_MEMORY_COUNTERS_EX memory = { 0 };
            memory.cb = sizeof(memory);
            double privateMemoryMb = 0.0;
            if (GetProcessMemoryInfo(
                    GetCurrentProcess(),
                    reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),
                    sizeof(memory)))
            {
                privateMemoryMb = static_cast<double>(memory.PrivateUsage) / (1024.0 * 1024.0);
            }

            DWORD handleCount = 0;
            GetProcessHandleCount(GetCurrentProcess(), &handleCount);

            char title[192] = { 0 };
            _snprintf_s(
                title,
                sizeof(title),
                _TRUNCATE,
                "KMTGuard ShardManager | READY | RAM %.0f MB | CPU %.1f%% | Handles %lu",
                privateMemoryMb,
                cpuPercent,
                static_cast<unsigned long>(handleCount));
            SetConsoleTitleA(title);
        }
    }
}

bool ShardManagerConsole::Initialize(bool allocateConsole)
{
    if (InterlockedCompareExchange(&s_initialized, 1, 0) != 0)
        return true;

    if (allocateConsole && GetConsoleWindow() == NULL && !AllocConsole())
    {
        InterlockedExchange(&s_initialized, 0);
        return false;
    }

    FILE* stream = NULL;
    if (GetConsoleWindow() != NULL)
    {
        freopen_s(&stream, "CONOUT$", "w", stdout);
        freopen_s(&stream, "CONOUT$", "w", stderr);
        freopen_s(&stream, "CONIN$", "r", stdin);
    }

    s_output = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleA("KMTGuard ShardManager | STARTING");

    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD inputMode = 0;
    if (input != INVALID_HANDLE_VALUE && GetConsoleMode(input, &inputMode))
    {
        inputMode |= ENABLE_EXTENDED_FLAGS;
        inputMode &= ~ENABLE_QUICK_EDIT_MODE;
        SetConsoleMode(input, inputMode);
    }

    EnsureLock();
    EnterCriticalSection(&s_writeLock);
    std::printf("\n");
    if (s_output != INVALID_HANDLE_VALUE)
        SetConsoleTextAttribute(s_output, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    std::printf("  KMTGuard ShardManager  v%s\n", KMTGUARD_VERSION_STRING);
    if (s_output != INVALID_HANDLE_VALUE)
        SetConsoleTextAttribute(s_output, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    std::printf("  Secure routing, command bridge and runtime protection\n");
    std::printf("  -----------------------------------------------------\n\n");
    std::fflush(stdout);
    LeaveCriticalSection(&s_writeLock);

    RedirectLegacyStreams();

    WriteInfo("Starting ShardManager add-on");
    return true;
}

void ShardManagerConsole::SetReady()
{
    SetConsoleTitleA("KMTGuard ShardManager | READY");
    WriteSuccess("Packet routing, database bridge and protection are active");
	if (InterlockedCompareExchange(&s_monitorStarted, 1, 0) == 0)
	{
		HANDLE thread = CreateThread(NULL, 0, StatusMonitor, NULL, 0, NULL);
		if (thread != NULL)
			CloseHandle(thread);
		else
		{
			InterlockedExchange(&s_monitorStarted, 0);
			WriteWarning("Live console status monitor could not be started");
		}
	}
}

void ShardManagerConsole::SetFailed()
{
    SetConsoleTitleA("KMTGuard ShardManager | INITIALIZATION FAILED");
}

void ShardManagerConsole::WriteDebug(const char* text) { WriteLine(LevelDebug, text); }
void ShardManagerConsole::WriteInfo(const char* text) { WriteLine(LevelInfo, text); }
void ShardManagerConsole::WriteWarning(const char* text) { WriteLine(LevelWarning, text); }
void ShardManagerConsole::WriteSuccess(const char* text) { WriteLine(LevelSuccess, text); }
void ShardManagerConsole::WriteFailure(const char* text) { WriteLine(LevelFailure, text); }

void ShardManagerConsole::WriteFormat(int level, const char* format, ...)
{
    if (format == NULL)
        return;

    char buffer[2048] = { 0 };
    va_list arguments;
    va_start(arguments, format);
    _vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, arguments);
    va_end(arguments);
    WriteLine(level, buffer);
}
