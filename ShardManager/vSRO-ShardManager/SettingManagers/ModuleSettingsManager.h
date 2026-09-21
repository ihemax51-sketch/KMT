#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include "../Database/SQLConnection.h"

struct ModuleSetting {
    int id;
    std::wstring name;
    std::wstring value;
};

class ModuleSettingsManager {
public:
    static bool Initialize(const std::wstring& connectionString);
    static void Close();
    static bool ReadModuleSettings(std::vector<ModuleSetting>& settings);

private:
    static SQLConnection m_Connection;
    static std::wstring m_ConnectionString;
};
