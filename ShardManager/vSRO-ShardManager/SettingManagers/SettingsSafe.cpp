#include "Settings.h"

#include <Windows.h>
#include <string>
#include <vector>
#include <limits>
#include <new>
#include <cwchar>
#include <cstdlib>
#include "../Utils/IO/SimpleIni.h"
#include "ModuleSettingsManager.h"
#include "../Utils/Memory/Process.h"
#include "../AsyncGSCommands.h"
#include "../Utils/BSObj.h"
#include "../AsmEdition.h"
#include "../Utils/Memory/hook.h"
#include "../Network/ShardNetManager.h"
#include "../Console/ShardManagerConsole.h"
#include "../Runtime/UniqueLogQueue.h"

namespace
{
    const char* SETTINGS_FILE = "KMTGuard-Addon.ini";

    struct PatchBackup
    {
        DWORD address;
        std::vector<BYTE> bytes;
    };

    std::vector<PatchBackup> s_patchBackups;

    bool RememberPatch(DWORD address, size_t length)
    {
        PatchBackup backup;
        backup.address = address;
        backup.bytes.resize(length);
        if (!ReadProcessBytes(
                GetCurrentProcess(),
                address,
                &backup.bytes[0],
                static_cast<DWORD>(length)))
            return false;
        s_patchBackups.push_back(backup);
        return true;
    }

    void RollbackPatches()
    {
        for (std::vector<PatchBackup>::reverse_iterator it = s_patchBackups.rbegin();
             it != s_patchBackups.rend();
             ++it)
        {
            WriteProcessBytes(
                GetCurrentProcess(),
                it->address,
                &it->bytes[0],
                static_cast<DWORD>(it->bytes.size()));
        }
        s_patchBackups.clear();
    }

    bool IsPlaceholder(const char* value)
    {
        if (value == NULL || value[0] == '\0')
            return true;
        return _stricmp(value, "CHANGE_ME") == 0 ||
            _stricmp(value, "[None]") == 0;
    }

    std::wstring ToWide(const std::string& value)
    {
        if (value.empty())
            return std::wstring();

        UINT codePage = CP_UTF8;
        int length = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, value.c_str(), -1, NULL, 0);
        if (length <= 0)
        {
            codePage = CP_ACP;
            length = MultiByteToWideChar(codePage, 0, value.c_str(), -1, NULL, 0);
        }
        if (length <= 0)
            return std::wstring();

        std::vector<wchar_t> buffer(static_cast<size_t>(length), L'\0');
        if (MultiByteToWideChar(codePage, 0, value.c_str(), -1, &buffer[0], length) <= 0)
            return std::wstring();
        return std::wstring(&buffer[0]);
    }

    bool IsTrue(const std::wstring& value)
    {
        return _wcsicmp(value.c_str(), L"true") == 0 || value == L"1";
    }

    bool TryParseBoolean(const std::wstring& value, bool& result)
    {
        if (_wcsicmp(value.c_str(), L"true") == 0 || value == L"1")
        {
            result = true;
            return true;
        }
        if (_wcsicmp(value.c_str(), L"false") == 0 || value == L"0")
        {
            result = false;
            return true;
        }
        return false;
    }

    bool TryParseByte(const std::wstring& value, BYTE& result)
    {
        wchar_t* end = NULL;
        const unsigned long parsed = wcstoul(value.c_str(), &end, 10);
        if (end == value.c_str() || *end != L'\0' || parsed > 255)
            return false;
        result = static_cast<BYTE>(parsed);
        return true;
    }

    bool PatchByte(const char* name, DWORD address, const std::wstring& value)
    {
        BYTE newValue = 0;
        BYTE oldValue = 0;
        if (!TryParseByte(value, newValue))
        {
            BS_ERROR("Invalid value for %s", name);
            return false;
        }
        if (!ReadMemoryValue<BYTE>(address, oldValue) ||
            !RememberPatch(address, sizeof(BYTE)) ||
            !WriteMemoryValue<BYTE>(address, newValue))
        {
            BS_ERROR("Unable to apply %s", name);
            return false;
        }
        BS_INFO("%s applied (%u -> %u)", name, oldValue, newValue);
        return true;
    }

    bool CreateSettingsTemplate()
    {
        CSimpleIniA ini;
        ini.SetSpaces(false);
        ini.SetValue("Database", "SQLSERVER", ".", "; SQL Server instance name");
        ini.SetValue("Database", "LoginId", "sa", "; SQL Server login username");
        ini.SetValue("Database", "Password", "CHANGE_ME", "; SQL Server login password");
        ini.SetBoolValue("App", "DEBUG_CONSOLE", true, "; Show KMTGuard ShardManager operational console");
        return ini.SaveFile(SETTINGS_FILE) == SI_OK;
    }
}

GameCfgStruct* CSettings::m_Settings = NULL;

bool CSettings::ShouldAllocateConsole()
{
    CSimpleIniA ini;
    if (ini.LoadFile(SETTINGS_FILE) != SI_OK)
        return true;
    return ini.GetBoolValue("App", "DEBUG_CONSOLE", true);
}

bool CSettings::LoadIniSettings()
{
    CSimpleIniA ini;
    if (ini.LoadFile(SETTINGS_FILE) != SI_OK)
    {
        if (!CreateSettingsTemplate())
        {
            BS_ERROR("KMTGuard-Addon.ini is missing and its template could not be created");
        }
        else
        {
            BS_WARNING("Created KMTGuard-Addon.ini; configure its database credentials before restart");
        }
        return false;
    }

    const char* server = ini.GetValue("Database", "SQLSERVER", "");
    const char* login = ini.GetValue("Database", "LoginId", "");
    const char* password = ini.GetValue("Database", "Password", "");
    if (IsPlaceholder(server) || IsPlaceholder(login) || IsPlaceholder(password))
    {
        BS_ERROR("Database settings are missing or still use placeholders");
        return false;
    }

    Shutdown();
    m_Settings = new (std::nothrow) GameCfgStruct;
    if (m_Settings == NULL)
    {
        BS_ERROR("Unable to allocate ShardManager settings");
        return false;
    }

    const std::wstring wideServer = ToWide(server);
    const std::wstring wideLogin = ToWide(login);
    const std::wstring widePassword = ToWide(password);
    if (wideServer.empty() || wideLogin.empty() || widePassword.empty())
    {
        BS_ERROR("Database settings contain invalid text encoding");
        Shutdown();
        return false;
    }

    m_Settings->DatabaseConnectionString
        << L"DRIVER={SQL Server};SERVER=" << wideServer
        << L";UID=" << wideLogin
        << L";PWD=" << widePassword
        << L";DATABASE=KMTGuard";

    if (!CShardNetManager::Setup())
    {
        BS_ERROR("ShardManager network integration is unavailable");
        Shutdown();
        return false;
    }

    if (!ModuleSettingsManager::Initialize(m_Settings->DatabaseConnectionString.str()))
    {
        Shutdown();
        return false;
    }

    std::vector<ModuleSetting> settings;
    if (!ModuleSettingsManager::ReadModuleSettings(settings))
    {
        BS_ERROR("Unable to load System_ShardSettings");
        ModuleSettingsManager::Close();
        Shutdown();
        return false;
    }

    bool success = true;
    bool unionLimitLoaded = false;
    bool guildPointProtectionLoaded = false;
    for (std::vector<ModuleSetting>::const_iterator it = settings.begin(); it != settings.end(); ++it)
    {
        const ModuleSetting& setting = *it;
        if (setting.name == L"AllocDebugConsole")
        {
            if (IsTrue(setting.value))
                ShardManagerConsole::Initialize(true);
        }
        else if (setting.name == L"UnionLimit")
        {
            if (unionLimitLoaded)
            {
                BS_ERROR("Duplicate UNION_LIMIT setting");
                success = false;
            }
            else
            {
                unionLimitLoaded = true;
                success = PatchByte("UnionLimit", 0x00434312, setting.value) && success;
            }
        }
        else if (setting.name == L"CTFMinParticipans")
            success = PatchByte("CTFMinParticipans", 0x00672895, setting.value) && success;
        else if (setting.name == L"BAMinParticipans")
            success = PatchByte("BAMinParticipans", 0x0066A1B4, setting.value) && success;
        else if (setting.name == L"PtFormPageForm")
            success = PatchByte("PtFormPageForm", 0x004517EE, setting.value) && success;
        else if (setting.name == L"FixPartyMatchDCINHour" && IsTrue(setting.value))
        {
            success = RememberPatch(0x0045055C, 5) &&
                WriteMemoryValue<WORD>(0x0045055C, 0x30EB) && success;
            for (int index = 0; index < 3; ++index)
                success = WriteMemoryValue<BYTE>(0x0045055E + index, 0x90) && success;
            if (success)
                BS_INFO("Party matching disconnect fix enabled");
        }
        else if (setting.name == L"FixNegativeGuildPoint")
        {
            bool enabled = false;
            if (guildPointProtectionLoaded)
            {
                BS_ERROR("Duplicate GUILD_POINTS setting");
                success = false;
            }
            else if (!TryParseBoolean(setting.value, enabled))
            {
                BS_ERROR("Invalid value for GUILD_POINTS");
                success = false;
            }
            else
            {
                guildPointProtectionLoaded = true;
                if (enabled)
                {
                    const bool hooksInstalled = RememberPatch(0x004364EE, 5) &&
                        RememberPatch(0x00438B68, 5) &&
                        RememberPatch(0x0043A9F6, 5) &&
                        placeHook(0x004364EE, addr_from_this(&AsmEdition::OnDonateGuildPoints)) &&
                        placeHook(0x00438B68, addr_from_this(&AsmEdition::OnDonateGuildPointsErrorCode)) &&
                        placeHook(0x0043A9F6, addr_from_this(&AsmEdition::OnDonateGuildPointsErrorMsg));
                    success = hooksInstalled && success;
                    if (hooksInstalled)
                        BS_INFO("Negative guild-point protection enabled");
                }
                else
                {
                    BS_INFO("Negative guild-point protection disabled");
                }
            }
        }
        else if (setting.name == L"EnableAsyncDatabaseCommands")
        {
            if (IsTrue(setting.value))
            {
                const bool bridgeStarted = AsyncGSCommands::Initialize();
                success = bridgeStarted && success;
                if (!bridgeStarted)
                    BS_ERROR("GameServer command bridge failed to initialize");
            }
            else
                BS_WARNING("GameServer command bridge is disabled by EnableAsyncDatabaseCommands");
        }
    }

    if (!unionLimitLoaded)
    {
        BS_ERROR("Required UNION_LIMIT setting is missing");
        success = false;
    }
    if (!guildPointProtectionLoaded)
    {
        BS_ERROR("Required GUILD_POINTS setting is missing or invalid");
        success = false;
    }

    ModuleSettingsManager::Close();
    if (!success)
    {
        BS_ERROR("One or more required ShardManager settings could not be applied");
        Shutdown();
        return false;
    }

    if (!UniqueLogQueue::Initialize(m_Settings->DatabaseConnectionString.str()))
    {
        BS_ERROR("Unable to start the unique history database worker");
        Shutdown();
        return false;
    }

    BS_INFO("ShardManager runtime settings loaded (%u entries)", static_cast<unsigned>(settings.size()));
    return true;
}

void CSettings::InitDebugConsole()
{
    ShardManagerConsole::Initialize(true);
}

void CSettings::Shutdown()
{
    AsyncGSCommands::Shutdown();
    UniqueLogQueue::Shutdown();
    ModuleSettingsManager::Close();
    RollbackPatches();
    if (m_Settings != NULL)
    {
        delete m_Settings;
        m_Settings = NULL;
    }
}
