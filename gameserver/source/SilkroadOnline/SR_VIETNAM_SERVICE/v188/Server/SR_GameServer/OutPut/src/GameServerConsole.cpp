#include "GameServerConsole.h"

#include <Windows.h>
#include <cstdio>
#include <KmtGuardProductVersion.h>

#ifndef ENABLE_QUICK_EDIT_MODE
#define ENABLE_QUICK_EDIT_MODE 0x0040
#endif
#ifndef ENABLE_EXTENDED_FLAGS
#define ENABLE_EXTENDED_FLAGS 0x0080
#endif

namespace
{
    HANDLE s_output = INVALID_HANDLE_VALUE;

    void WriteLine(const char* state, const char* text, WORD color)
    {
        if (text == NULL)
            return;

        if (s_output != INVALID_HANDLE_VALUE)
            SetConsoleTextAttribute(s_output, color);
        std::printf("[%-8s] %s\n", state, text);
        if (s_output != INVALID_HANDLE_VALUE)
            SetConsoleTextAttribute(s_output, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }
}

void GameServerConsole::Initialize()
{
#ifdef CONFIG_DEBUG_CONSOLE
    if (GetConsoleWindow() == NULL)
        AllocConsole();

    FILE* stream = NULL;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);

    s_output = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleA("KMTGuard GameServer | STARTING");

    HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
    DWORD inputMode = 0;
    if (input != INVALID_HANDLE_VALUE && GetConsoleMode(input, &inputMode))
    {
        inputMode |= ENABLE_EXTENDED_FLAGS;
        inputMode &= ~ENABLE_QUICK_EDIT_MODE;
        SetConsoleMode(input, inputMode);
    }

    std::printf("\n  KMTGuard GameServer  v%s\n", KMTGUARD_VERSION_STRING);
    std::printf("  Secure runtime and performance monitor\n\n");
    WriteInfo("Starting GameServer add-on");
#endif
}

void GameServerConsole::WriteInfo(const char* text)
{
#ifdef CONFIG_DEBUG_CONSOLE
    WriteLine("INFO", text, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
}

void GameServerConsole::WriteWarning(const char* text)
{
#ifdef CONFIG_DEBUG_CONSOLE
    WriteLine("WARNING", text, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#endif
}

void GameServerConsole::WriteSuccess(const char* text)
{
#ifdef CONFIG_DEBUG_CONSOLE
    WriteLine("READY", text, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
#endif
}

void GameServerConsole::WriteFailure(const char* text)
{
#ifdef CONFIG_DEBUG_CONSOLE
    WriteLine("FAILED", text, FOREGROUND_RED | FOREGROUND_INTENSITY);
    SetConsoleTitleA("KMTGuard GameServer | INITIALIZATION FAILED");
#endif
}
