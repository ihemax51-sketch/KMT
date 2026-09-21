#include "Settings.h"
// Console stuffs
#pragma warning(disable:4996) // _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <iostream>
#include "../Utils/IO/SimpleIni.h"
#include "ModuleSettingsManager.h"
#include "../Utils/Memory/Process.h"
#include "../AsyncGSCommands.h"
#include "../Utils/BSObj.h"
#include "../AsmEdition.h"
#include "../Utils/Memory/hook.h"
#include "../Network/ShardNetManager.h"
GameCfgStruct* CSettings::m_Settings = NULL;


#define CfgFileName ".\\KMTGuard-Addon.ini"

std::wstring s2ws(const std::string& s) {
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    std::wstring r(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, &r[0], len);
    return r;
}
bool ConvertToBool(const std::wstring& value) {
    if (value == L"true" || value == L"1") {
        return true;
    }
    return false;
}
void CSettings::LoadIniSettings() {

    CSimpleIniA ini;
    // Try to load it or create a new one
    if (ini.LoadFile("KMTGuard-Addon.ini") != SI_Error::SI_OK)
    {
        ini.SetSpaces(false);
        // Memory
        ini.SetValue("Database", "SQLSERVER", ".", "; Sql Server Instance Name");
        ini.SetValue("Database", "LoginId", "sa", "; Sql Server Login Username");
        ini.SetValue("Database", "Password", "1234", "; Sql Server Password");
        ini.SetBoolValue("App", "DEBUG_CONSOLE", true, "; Attach debug console and show shard manager logs");
        ini.SaveFile("KMTGuard-Addon.ini");
    }


    bool debugConsole = ini.GetBoolValue("App", "DEBUG_CONSOLE", true);
    if (debugConsole) {
        InitDebugConsole();
        printf("[INFO]-> DEBUG_CONSOLE (True)\n");
    }

    CShardNetManager::Setup();
    m_Settings = new GameCfgStruct;


    ini.LoadFile(CfgFileName);


    std::string SQLSERVER = ini.GetValue("Database", "SQLSERVER", ".");
    std::string LoginID = ini.GetValue("Database", "LoginId", "sa");
    std::string Password = ini.GetValue("Database", "Password", "1234");

    // Convert std::string to std::wstring
    std::wstring wSQLSERVER = s2ws(SQLSERVER);
    std::wstring wLoginID = s2ws(LoginID);
    std::wstring wPassword = s2ws(Password);


    m_Settings->DatabaseConnectionString << L"DRIVER={SQL Server};SERVER=" << wSQLSERVER << L";UID=" << wLoginID << L";PWD=" << wPassword << L";DATABASE=KMTGuard";


    ModuleSettingsManager::Initialize(m_Settings->DatabaseConnectionString.str());
    std::vector<ModuleSetting> settings;
    uint8_t byteValue;
    if (ModuleSettingsManager::ReadModuleSettings(settings)) {
        for (const auto& setting : settings) {
            //printf("%ls \n", setting.name.c_str());
            if (setting.name == L"AllocDebugConsole")
            {
                if (setting.value == L"True")
                {
                    InitDebugConsole();
                    printf("[INFO]-> AllocDebugConsole (True)\n");
                }
                else
                {
                    printf("[INFO]-> AllocDebugConsole (False)\n");
                }
            }
            else if (setting.name == L"UnionLimit")
            {
                if (ReadMemoryValue<uint8_t>(0x00434311 + 1, byteValue))
                {
                    byte newValue = std::stoi(setting.value);
                    printf("[INFO]-> UnionLimit (%d) -> (%d)\r\n", byteValue, newValue);
                    WriteMemoryValue<uint8_t>(0x00434311 + 1, newValue);
                }
            }
            else if (setting.name == L"CTFMinParticipans")
            {
                if (ReadMemoryValue<uint8_t>(0x00672891 + 4, byteValue))
                {
                    uint8_t newValue = std::stoi(setting.value);
                    printf("[INFO]-> CTFMinParticipans (%d) -> (%d)\r\n", byteValue, newValue);
                    WriteMemoryValue<uint8_t>(0x00672891 + 4, newValue);
                }
            }
            else if (setting.name == L"BAMinParticipans")
            {
                if (ReadMemoryValue<uint8_t>(0x0066A1B0 + 4, byteValue))
                {
                    uint8_t newValue = std::stoi(setting.value);
                    printf("[INFO]-> BAMinParticipans (%d) -> (%d)\r\n", byteValue, newValue);
                    WriteMemoryValue<uint8_t>(0x0066A1B0 + 4, newValue);
                }
            }
            else if (setting.name == L"FixPartyMatchDCINHour")
            {
                if (setting.value == L"True")
                {
                    printf("[INFO]-> FixPartyMatchDCINHour (True)\n");
                    WriteMemoryValue<uint16_t>(0x0045055C, 0x30EB); // jmp,+30
                    for (int i = 0; i < 3; i++)
                        WriteMemoryValue<uint8_t>(0x0045055C + 2 + i, 0x90); // nop
                }
                else
                {
                    printf("[INFO]-> FixPartyMatchDCINHour = (False)\n");
                }
            }
            else if (setting.name == L"FixNegativeGuildPoint")
            {
                if (setting.value == L"True")
                {
                    printf("[INFO]-> FixNegativeGuildPoint (True)\n");
                    if (placeHook(0x004364EE, addr_from_this(&AsmEdition::OnDonateGuildPoints)))
                    {
                        std::cout << "[INFO]-> OnDonateGuildPoints" << std::endl;
                    }
                    if (placeHook(0x00438B68, addr_from_this(&AsmEdition::OnDonateGuildPointsErrorCode)))
                    {
                        std::cout << "[INFO]-> OnDonateGuildPointsErrorCode" << std::endl;
                    }
                    if (placeHook(0x0043A9F6, addr_from_this(&AsmEdition::OnDonateGuildPointsErrorMsg)))
                    {
                        std::cout << "[INFO]-> OnDonateGuildPointsErrorMsg" << std::endl;
                    }
                }
                else
                {
                    printf("[INFO]-> FixNegativeGuildPoint = (False)\n");
                }
            }
            else if (setting.name == L"EnableAsyncDatabaseCommands")
            {
                if (setting.value == L"True")
                {
                    printf("[INFO]-> EnableAsyncDatabaseCommands (True)\n");
                    AsyncGSCommands::Initialize();
                }
                else
                {
                    printf("[INFO]-> EnableAsyncDatabaseCommands = (False)\n");
                }
            }
            else if (setting.name == L"PtFormPageForm")
            {
                if (ReadMemoryValue<uint8_t>(0x004517ED + 1, byteValue))
                {
                    uint8_t newValue = std::stoi(setting.value);
                    printf("[INFO]-> PT_FORM_MAX_PAGE_COUNT (%d) -> (%d)\r\n", byteValue, newValue);
                    WriteMemoryValue<uint8_t>(0x004517ED + 1, newValue);
                }
            }
            //std::wcout << L"[INFO]-> " << L", SettingName: " << setting.name << L", Value: " << setting.value << std::endl;
        }
    }
    else {
        std::wcerr << L"Veritabaný baðlantýsý baþarýsýz" << std::endl;
    }

    ModuleSettingsManager::Close();
}

void CSettings::InitDebugConsole()
{

    if (GetConsoleWindow() == NULL)
        AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    freopen("CONIN$", "r", stdin);

    BS_INFO("KMTGuard-ShardManager");
    BS_INFO("******************************************************************************");
}