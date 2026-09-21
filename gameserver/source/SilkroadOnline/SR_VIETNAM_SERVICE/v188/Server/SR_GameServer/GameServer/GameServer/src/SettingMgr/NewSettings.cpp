//
// Created by YUMBUL on 13.02.2025.
//

#include "NewSettings.h"
#include "IniReader.h"
#include <sstream>

#define GS_CONFIG_PATH									".\\KMTGuard-Addon.ini"
GameCfgStruct* CNewSettings::m_Settings = NULL;
void CNewSettings::LoadIniSettings() {

    // Value-initialize every scalar before database overrides are applied.
    // Missing or temporarily unavailable settings must never leave patch
    // values containing indeterminate process memory.
    delete m_Settings;
    m_Settings = new GameCfgStruct();
    m_Settings->DisableDurability = false;
    m_Settings->DisableGreenBook = false;
    m_Settings->ShowGmUniqueKillNotice = false;
    m_Settings->ForceGmVisibleOnSpawn = false;
    m_Settings->DisableOriginalTradeGold = false;
    m_Settings->EnablePartyMonsterSpawn = true;
    m_Settings->PartyMonsterMinimumMembers = 2;
    m_Settings->PartyMonsterSpawnRate = 50;

    //==============================================================================================================================
    //GameServer common
    CIniReader reader(GS_CONFIG_PATH);

    std::string SQLSERVER = reader.ReadStringA("Database", "SQLSERVER", "[None]");
    std::string LoginID = reader.ReadStringA("Database", "LoginId", "[None]");
    std::string Password = reader.ReadStringA("Database", "Password", "[None]");

    m_Settings->DatabaseConnectionString = "DRIVER={SQL Server};SERVER=" + SQLSERVER + ";UID=" + LoginID + ";PWD=" + Password + ";DATABASE=KMTGuard";
}

namespace
{
    bool IsHexSecret(const std::string& value)
    {
        if (value.size() != 64)
            return false;
        for (size_t i = 0; i < value.size(); ++i)
        {
            const char c = value[i];
            if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
                  (c >= 'a' && c <= 'f')))
                return false;
        }
        return true;
    }

    bool RequireRange(const char* name, __int64 value, __int64 minimum, __int64 maximum, std::string& error)
    {
        if (value >= minimum && value <= maximum)
            return true;

        std::ostringstream stream;
        stream << name << " is outside the supported range [" << minimum << ", " << maximum << "]";
        error = stream.str();
        return false;
    }

    bool RequireCodeName(const char* name, const std::string& value, std::string& error)
    {
        if (!value.empty() && value.size() <= 128)
            return true;
        error = std::string(name) + " must contain a code name of 1-128 bytes";
        return false;
    }
}

bool CNewSettings::Validate(std::string& error)
{
    error.clear();
    if (m_Settings == NULL)
    {
        error = "settings object is unavailable";
        return false;
    }

    if (m_Settings->DatabaseConnectionString.find("[None]") != std::string::npos)
    {
        error = "database connection settings are incomplete";
        return false;
    }

    if (!IsHexSecret(m_Settings->InternalPacketSharedSecret))
    {
        error = "Security_InternalPacketSharedSecret must contain exactly 64 hexadecimal characters";
        return false;
    }

#define KMT_REQUIRE_RANGE(field, minimum, maximum) \
    if (!RequireRange(#field, m_Settings->field, minimum, maximum, error)) return false

    KMT_REQUIRE_RANGE(SERVER_MAX_LEVEL, 1, 255);
    KMT_REQUIRE_RANGE(CH_MAX_MASTERY_LEVEL, 1, 10000);
    KMT_REQUIRE_RANGE(EU_MAX_MASTERY_LEVEL, 1, 10000);
    KMT_REQUIRE_RANGE(MIN_PK_LEVEL, 1, 255);
    KMT_REQUIRE_RANGE(PENALTY_DROP_LEVEL_MIN, 1, 255);
    KMT_REQUIRE_RANGE(STALL_EXCHANGE_GOLD_LIMIT, 1, 0xFFFFFFFFFFLL);
    KMT_REQUIRE_RANGE(PARTY_LEVEL_MIN, 1, 255);
    KMT_REQUIRE_RANGE(PartyMonsterMinimumMembers, 1, 9);
    KMT_REQUIRE_RANGE(PartyMonsterSpawnRate, 0, 100);
    KMT_REQUIRE_RANGE(PENALTY_DROP_PROBABILITY, 0, 100);
    KMT_REQUIRE_RANGE(RESURRECT_SAME_POINT_LEVEL_MAX, 1, 255);
    KMT_REQUIRE_RANGE(NPC_RETURN_DEAD_LEVEL_MAX, 1, 255);
    KMT_REQUIRE_RANGE(BEGINNER_MARK_LEVEL_MAX, 1, 255);
    KMT_REQUIRE_RANGE(DROP_ITEM_MAGIC_PROBABILITY, 0, 100);
    KMT_REQUIRE_RANGE(GRAP_PET_INVENTORY_SIZE, 1, 255);
    KMT_REQUIRE_RANGE(FIX_AGENT_SERVER_CAPACITY, 1, 100000);
    KMT_REQUIRE_RANGE(MEMBERS_LIMIT_LEVEL1, 1, 1000);
    KMT_REQUIRE_RANGE(MEMBERS_LIMIT_LEVEL2, 1, 1000);
    KMT_REQUIRE_RANGE(MEMBERS_LIMIT_LEVEL3, 1, 1000);
    KMT_REQUIRE_RANGE(MEMBERS_LIMIT_LEVEL4, 1, 1000);
    KMT_REQUIRE_RANGE(MEMBERS_LIMIT_LEVEL5, 1, 1000);
    KMT_REQUIRE_RANGE(STORAGE_SLOTS_MIN, 1, 1000);
    KMT_REQUIRE_RANGE(STORAGE_SLOTS_INCREASE, 0, 1000);
    KMT_REQUIRE_RANGE(UNION_LIMIT, 1, 255);
    KMT_REQUIRE_RANGE(UNION_CHAT_PARTICIPANTS, 1, 255);
    KMT_REQUIRE_RANGE(MIN_GUILD_LEVEL_FOR_MERCENARY_SPAWN, 1, 255);
    KMT_REQUIRE_RANGE(ALCHEMY_FUSING_DELAY, 0, 255);
    KMT_REQUIRE_RANGE(MinItemLevelForAstralToTakeEffect, 0, 255);
    KMT_REQUIRE_RANGE(ItemLevelForAstralRecovery, 0, 255);
    KMT_REQUIRE_RANGE(CTF_ITEM_WIN_REWARD_AMOUNT, 0, 255);
    KMT_REQUIRE_RANGE(CTF_ITEM_KILL_REWARD_AMOUNT, 0, 255);
    KMT_REQUIRE_RANGE(BA_ITEM_REWARD_GJ_W_AMOUNT, 0, 255);
    KMT_REQUIRE_RANGE(BA_ITEM_REWARD_GJ_L_AMOUNT, 0, 255);
    KMT_REQUIRE_RANGE(BA_ITEM_REWARD_PR_W_AMOUNT, 0, 255);
    KMT_REQUIRE_RANGE(BA_ITEM_REWARD_PR_L_AMOUNT, 0, 255);
    KMT_REQUIRE_RANGE(JOB_LEVEL_MAX, 1, 255);
    KMT_REQUIRE_RANGE(TEMPLE_LEVEL, 1, 255);
#undef KMT_REQUIRE_RANGE

    return RequireCodeName("CTF_ITEM_WIN_REWARD", m_Settings->CTF_ITEM_WIN_REWARD, error) &&
           RequireCodeName("CTF_ITEM_KILL_REWARD", m_Settings->CTF_ITEM_KILL_REWARD, error) &&
           RequireCodeName("BA_ITEM_REWARD", m_Settings->BA_ITEM_REWARD, error);
}
GameCfgStruct* CNewSettings::GetGameCfg()
{
    return m_Settings;
}
