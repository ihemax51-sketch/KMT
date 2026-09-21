#pragma once

#include <RegionRestrictionDBSet.h>
#include "DbConnection.h"
#include "AutoCriticalSection.h"
#include "../../../../OutPut/src/Util.h"
#include <vector>

struct _RefSkillByItemOptLevel
{
    int Link;
    int RefSkillID;
};
struct _RefAbilityByItemOptLevel
{
    int ID;
    int RefItemID;
    BYTE OptLevel;
};

struct STimedItemPlusDbRecord
{
    int CharID;
    INT64 dwItemID64;
    long endTime;
    int CurrentPlus;
};
struct STimedDevillPlusDbRecord
{
    int CharID;
    INT64 dwItemID64;
    long endTime;
    int CurrentPlus;
};

struct SServerAutoCapebyRegionID
{
    int nID;
    WORD wRegionID;
};
struct SServerAutoCapebyWorldID
{
    int nID;
    WORD wWorldID;
};

struct SServerJobMobExpList
{
    int nID;
    int MobID;
    int MobClass;
    int Exp;
};

struct _ServerFortressDpsInfo
{
    int ObjID;
};
struct SCustomNpcInteractionDbRecord
{
    int nID;
    char szCodeName128[128];
    int nInteractionID;
};

struct SAttackRestrictionByMob
{
    int   MobRefObjID;
    bool  OnlyOffJob;
    bool  OnlyOnJob;
    bool  OnlyByThief;
    bool  OnlyByTrader;
    bool  OnlyStrPlayer;
    bool  OnlyIntPlayer;

    BYTE  AllowedJobMask;   // 0 = disabled
    BYTE  AllowedCapeMask;  // 0 = disabled
    BYTE  AllowedRaceMask;  // 0 = disabled

    int   RequireParty;     // -1=N/A, 0=false, 1=true
    int   RequireGuild;     // -1=N/A, 0=false, 1=true

    SAttackRestrictionByMob()
        : MobRefObjID(0), OnlyOffJob(false), OnlyOnJob(false),
        OnlyByThief(false), OnlyByTrader(false),
        OnlyStrPlayer(false), OnlyIntPlayer(false),
        AllowedJobMask(0), AllowedCapeMask(0), AllowedRaceMask(0),
        RequireParty(-1), RequireGuild(-1) {
    }
};

struct SItemRegionRestriction
{
    int WorldID;
    int RegionID;
    int ItemID;

    SItemRegionRestriction() : WorldID(0), RegionID(0), ItemID(0) {}
};

class CSqlCon
{
public:

    enum ItemLockStateResult
    {
        ITEM_LOCK_STATE_FAILED = 0,
        ITEM_LOCK_STATE_CHANGED = 1,
        ITEM_LOCK_STATE_ALREADY_SET = 2
    };

    static CDbConnection* m_connectionstr;
    static CRegionRestrictionDBSet* s_pRegionRestrictionDbSet;
    static bool TryExecNonQuery(const char* szQuery);
    static bool Initialize();
    static void Shutdown();
    static bool LoadInternalPacketSharedSecret();
    static bool StartSecuritySnapshotRefresh();
    static bool LoadGameServerSettings();
    static std::string GetProcessInstanceId();

    static void StringReplaceAll(std::string& Value, const std::string& From, const std::string& To)
    {
        if (From.empty())
            return;
        size_t start_pos = 0;
        while ((start_pos = Value.find(From, start_pos)) != std::string::npos)
        {
            Value.replace(start_pos, From.length(), To);
            start_pos += To.length(); // Increase the same length in case 'To' contains 'From', like replacing 'x' with 'yx'
        }
    }

    static void GameServerInitialized();

    static CRegionRestrictionDBSet* GetRegionRestrictionDbSet();
    static bool LoadFortressDPSInfo();
    static bool IsFortressDpsEnabled(int structObjId);
    static bool ServerAutoCapebyWorldID();
    static std::list<SServerAutoCapebyRegionID> AutoCapeList;
    static std::list<SServerAutoCapebyWorldID> AutoCapeListWorldId;
    static std::list<SCustomNpcInteractionDbRecord> s_CustomNpcInteractions;
    static std::map<INT64, STimedDevillPlusDbRecord> STimedDevillList;
    static std::map<INT64, STimedItemPlusDbRecord> TimedItemList;
    static std::map<int, _RefAbilityByItemOptLevel> RefAbilitybyItemOptLevel;
    static std::map<int, _RefSkillByItemOptLevel> RefSkillByItemOptLevel;

    static std::map<int, _ServerFortressDpsInfo> ServerFortressDpsInfo;


    // mob restrict
    static bool LoadAttackRestrictionsByMob(); // loader
    static bool TryGetAttackRestriction(int mobRefObjId, SAttackRestrictionByMob& result);
    static std::map<int, SAttackRestrictionByMob> s_AttackRestrByMob; // key: MobRefObjID
    static bool IsCharInParty(int CharID); // not set yet just put declare

    // Startup-only cache. Table changes intentionally require a GameServer restart.
    static bool LoadItemRegionRestrictions();
    static bool IsItemTrackedForRegion(int itemId);
    static bool IsItemBlockedInRegion(int worldId, int regionId, int itemId);
    static std::vector<SItemRegionRestriction> s_ItemRegionRestrictions;

    static bool ApplyRuntimeSettings();

    static bool ServerAutoCapebyRegionID();


    static bool LoadRefSkillByItemOptLevel();

    static bool LoadRefAbilitybyItemOptLevel();

    static bool TimedPlusItems();
    static bool TimedDevillPlusItems();




    static bool LoadLockedItems();
    static bool IsItemLocked(INT64 itemId);
    static void AddLockedItem(INT64 itemId);
    static void RemoveLockedItem(INT64 itemId);
    static ItemLockStateResult SetItemLockState(INT64 itemId, bool locked);
    static std::vector<INT64> LockedItemList;

    static BYTE GetItemBindingOpt(INT64 ID64);

    static bool GetCustomNpcInteractionRecords(std::list<SCustomNpcInteractionDbRecord> &result);
};
