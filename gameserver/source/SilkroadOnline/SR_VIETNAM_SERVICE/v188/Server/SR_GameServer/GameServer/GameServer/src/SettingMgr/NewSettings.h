#pragma once

#include <string>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
struct GameCfgStruct
{
    std::string DatabaseConnectionString;
    std::string InternalPacketSharedSecret;
    bool DisableDurability;
    bool DisableGreenBook;
    bool ShowGmUniqueKillNotice;
    bool ForceGmVisibleOnSpawn;
    bool DisableOriginalTradeGold;
    int SERVER_MAX_LEVEL;
    int CH_MAX_MASTERY_LEVEL;
    int EU_MAX_MASTERY_LEVEL;
    int MIN_PK_LEVEL;
    __int64 STALL_EXCHANGE_GOLD_LIMIT;
    bool HIGH_RATES_CONFIG;
    int PARTY_LEVEL_MIN;
    bool EnablePartyMonsterSpawn;
    int PartyMonsterMinimumMembers;
    int PartyMonsterSpawnRate;
    int PENALTY_DROP_PROBABILITY;
    int RESURRECT_SAME_POINT_LEVEL_MAX;
    int NPC_RETURN_DEAD_LEVEL_MAX;
    int BEGINNER_MARK_LEVEL_MAX;
    int DROP_ITEM_MAGIC_PROBABILITY;
    int PENALTY_DROP_LEVEL_MIN;
    int GRAP_PET_INVENTORY_SIZE;
    bool FIX_EXPLOIT_INVISIBLE_INVINCIBLE;
    int FIX_AGENT_SERVER_CAPACITY;
    bool EXCHANGE_ATTACK_CANCEL;
    bool GUILD_POINTS;
    bool FIX_GRAP_PET_PAGE;
    int MEMBERS_LIMIT_LEVEL1;
    int MEMBERS_LIMIT_LEVEL2;
    int MEMBERS_LIMIT_LEVEL3;
    int MEMBERS_LIMIT_LEVEL4;
    int MEMBERS_LIMIT_LEVEL5;
    int STORAGE_SLOTS_MIN;
    int STORAGE_SLOTS_INCREASE;
    int UNION_LIMIT;
    int UNION_CHAT_PARTICIPANTS;
    int MIN_GUILD_LEVEL_FOR_MERCENARY_SPAWN;
    bool ALLOW_NON_GM_MERCENARY_SPAWN;
    bool DISABLE_GRANT_NAME_CONDITIONS;
    int ALCHEMY_FUSING_DELAY;
    std::string CTF_ITEM_WIN_REWARD;
    int CTF_ITEM_WIN_REWARD_AMOUNT;
    std::string CTF_ITEM_KILL_REWARD;
    int CTF_ITEM_KILL_REWARD_AMOUNT;
    std::string BA_ITEM_REWARD;
    int BA_ITEM_REWARD_GJ_W_AMOUNT;
    int BA_ITEM_REWARD_GJ_L_AMOUNT;
    int BA_ITEM_REWARD_PR_W_AMOUNT;
    int BA_ITEM_REWARD_PR_L_AMOUNT;
    bool EnableHideNameRegions;
    int GrabPetInventorySize;
    int MinItemLevelForAstralToTakeEffect;
    int ItemLevelForAstralRecovery;
    int MaxItemPlusDropCheck;
    int JOB_LEVEL_MAX;
    bool DISABLE_MOB_SPAWN_WHILE_TRADE;
    int TEMPLE_LEVEL;
};
class CNewSettings {
public:
    static void LoadIniSettings();
    static bool Validate(std::string& error);
    static GameCfgStruct* m_Settings;
    static GameCfgStruct *GetGameCfg();
};
