#pragma once
#include <string>
#include "sstream"

struct GameCfgStruct
{
    std::wstringstream DatabaseConnectionString;
};


class CSettings
{
public:
    static bool LoadIniSettings();

    static bool ShouldAllocateConsole();

    static void Shutdown();

    static void InitDebugConsole();

    static GameCfgStruct* m_Settings;
};
