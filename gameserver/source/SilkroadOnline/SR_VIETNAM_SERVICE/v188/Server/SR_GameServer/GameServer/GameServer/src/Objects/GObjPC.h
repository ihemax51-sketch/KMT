//
// Created by Kurama on 12/11/2022.
//
#pragma once
#include "InstanceObj.h"
#include "IGObj.h"
#include "GObjChar.h"
#include <memory/hook.h>
#include "CGObjCOS_GoldPet.h"
#include "Guild.h"
#include "SkillManager.h"
#include "RgnTerrain.h"
//Size: 0x0040


#pragma pack(push, 1)
struct SObjectStateFlags
{
    //Dunno exact count atm fck it
    BYTE first;
    BYTE second;
    BYTE third;
    BYTE fourth;
    BYTE btMsgBlockState;
};
#pragma pack(pop)
class CGObjPC : public CGObjChar
{
public:
    static void Setup();
    int GetDBID() const;
    BOOL CanSendMessage();

    /// NEWS
    SPosInfo GetCurrentPosition();
    void HandleGrantNameRequest(CMsg* pMsg);
    void HandleTitleChangeRequest(CMsg* pmsg);
    void HandleChangePvpCapeRequest(CMsg* pmsg);
    void HandlePvpChallengeGoldSyncRequest(CMsg* pmsg);
    void HandleCustomReverseUseRequest(CMsg* pmsg);
    void HandleSelfTeleportRequest();
    void HandleFilterTeleportRequest(CMsg* pmsg);
    void HandleItemTranslateRequest(CMsg*pmsg);
    void HandleLiveItemChestPacket(CMsg*pmsg);
    void HandleFellowSkill(CMsg*pmsg);
    void HandleSilkPacket(CMsg*pmsg);
    void HandleCosSkill(CMsg*pmsg);
    void HandleCustomScrollUsage(CMsg*pmsg);
    void HandleItemLockRequest(CMsg*pmsg);
    void HandleItemUnlockRequest(CMsg*pmsg);
    static void FlushItemLockCompletions();
    void HandleAlchemyLinkRequest(CMsg*pmsg);
    void HandleNewAlchemyRequest(CMsg*pmsg);
    void HandleDisplayCharInfoRequest(CMsg*pmsg);
    void HandleGlobalItemLink(CMsg*pmsg);


    typedef IGObj* (__thiscall* FN_GETITEMATSTORAGESLOT)(CGObjPC* pPC, int nSlotIndex);
    typedef void(__thiscall* FN_OFFSET_EXP_POINT)(CGObjPC*, DWORD dwExpSourceObjUniqueID, long long qwLevelExp, long long qwSkillExp, DWORD& dwUnk);
    static FN_OFFSET_EXP_POINT s_pfnOffsetExpPoint;
    static FN_GETITEMATSTORAGESLOT s_pfnGetItemAtStorageSlot;
    IGObj* GetItemAtStorageSlot(int nSlotIndex);

    void GiveExp(DWORD dwExpSourceObjUniqueID, long long qwLevelExp, long long qwSkillExp, DWORD& dwUnk);
    void Ressurect(float fRecoverHpMult, float fRecoverMpMult);
    void UpdateHpMp(int nHp, int nMp);
    void Func_201();
    void GroupSpawn(unsigned int p1, int p2);

    unsigned int CharacterSpawn_VfTable391();

    void KillLoggerFunction(CGObjPC* param_1, int param_2);

    CMsg* AllocMsg(unsigned int param_1);
    void SendMsg(CMsg *param_1);
    void BroadcastMsgToNearbyPlayers(CMsg *pMsg);

    void SetGrantName(std::string* grantname);
    void CancelBuff(int nRefSkillID);
    void UpdateHwan(char hwanLevel);
    void FUNCMovement(unsigned int param);
    CSkillManager* GetSkillManager();

    void ReaderPacket(CMsg *pMsg);
    void OnDeleteObjectCustom();
    void SetCharState(char a2);
    void FuncJobExp(unsigned int p1, unsigned int p2, unsigned int p3);
    void FuncJobExpTest(unsigned int p1);
    void FuncJobExpTest2(unsigned int p1);
    void CharacterStats();
    CRgnTerrain* GetRgnTerrain() const;
    bool IsInBattleField() const;
    void UpdatePVPCapeType(BYTE CapeType);
    bool MoveTo(uint32_t param_1, unsigned short param_2, float param_3, float param_4, float param_5, unsigned int param_6);
    int SetLiveDeleteItem(int slot, int reducecount);
    void SetLiveItem(int slot, const char* itemcode);
    void UpdateSilk(int nSilk, int nSilkGift, int nSilkPoint, bool bSendPacket);
    void EngageBuffSkill(CGObjPC* pPC, const char* szSkillCodeName128);
    void LiveSkill(int SkillID);
    unsigned __int16 AddItem(const char* Codename, __int32 Count, bool RandomizeStats, __int8 OptLevel);

    void UpdateGold(__int64 Offset);
    void RemoveGold(__int64 Offset);
    void DealDamage(int nReasonMask, int nAmount);
    void TeleportToTown();
private:
    void UpdateGold(__int64 Amount, __int32 Unknown, bool Realtime, bool ShowMessage);

public:

    char pad_1CA0[24]; //0x1CA0
    void* MyParty; //0x1CB8
    CGuild* MyGuild; //0x1CBC
    char pad_1CC0[8]; //0x1CC0
    void* CMessenger; //0x1CC8
    char pad_1CCC[76]; //0x1CCC
    CGObjCOS_GoldPet* GoldPetPtr; //0x1D18
    char pad_1D1C[88]; //0x1D1C
    CGStorage Storage; //0x1D74
    char pad_1D98[400]; //0x1D98
    uint8_t N00000CF4; //0x1F28
    uint8_t N00004499; //0x1F29
    uint8_t Walking; //0x1F2A
    uint8_t N0000449C; //0x1F2B
    char pad_1F2C[8]; //0x1F2C
    uint8_t N00000CF7; //0x1F34
    uint8_t N000044FB; //0x1F35
    uint8_t OnPet; //0x1F36
    uint8_t N000044FC; //0x1F37
    char pad_1F38[172]; //0x1F38
    CGStorage CGStoragee; //0x1FE4
    char pad_2008[484]; //0x2008
    int PvpCape; //0x21EC
    char pad_21F0[280]; //0x21F0


    //char pad_1CA0[28]; //0x1CA0
    //CGuild* MyGuild; //0x1CBC
    //char pad_1CC0[8]; //0x1CC0
    //void* CMessenger; //0x1CC8
    //char pad_1CCC[76]; //0x1CCC
    //CGObjCOS_GoldPet* GoldPetPtr; // 0x1D18
    //char pad_1D1C[88]; //0x1D1C
    //CGStorage Storage; //0x1D74
    //char pad_1D98[588]; //0x1D98
    //CGStorage CGStoragee; //0x1FE4
    //char pad_2008[768]; //0x2008

    //char pad_1CA0[28]; //0x1CA0
    //CGuild* CGuild; //0x1CBC
    //char pad_1CC0[8]; //0x1CC0
    //void* CMessenger; //0x1CC8
    //char pad_1CCC[76]; //0x1CCC
    //void* GoldPetPtr; //0x1D18
    //char pad_1D1C[88]; //0x1D1C
    //CGStorage Storage; //0x1D74
    //char pad_1D98[588]; //0x1D98
    //CGStorage CGStoragee; //0x1FE4
    //char pad_2008[744]; //0x2008

private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x2308)
        ENSURE_OFFSET(MyParty, 0x1CB8)
        ENSURE_OFFSET(MyGuild, 0x1CBC)
        ENSURE_OFFSET(CMessenger, 0x1CC8)
        ENSURE_OFFSET(Storage, 0x1D74)
        ENSURE_OFFSET(CGStoragee, 0x1FE4)
        ENSURE_OFFSET(GoldPetPtr, 0x1d18)
    END_FIXTURE()

    RUN_FIXTURE(CGObjPC)


    bool ItemIsWeapon(TypeId TID);

    bool ItemIsArmor(TypeId TID);

    bool ItemIsAccessory(TypeId TID);

    bool ItemIsShield(TypeId TID);

    bool ItemIsEnhancer(TypeId TID);

    bool ItemIsProofStone(TypeId TID);

    void Send3040(byte ItemSlot, byte NewOptLevel);
}; //Size: 0x2308


class CGObjPC;
class CGObjMob;
class IGObj;


class CRegionAttackRestrictionsMgr
{
private:
    typedef BOOL(__thiscall* FN_IOBJ_CAN_ATTACK)(IGObj* pObjFirst, IGObj* pObjSecond, int a3, WORD& pwResultCode);

    static FN_IOBJ_CAN_ATTACK s_pfnIGObj_CanAttack;

    static volatile LONG s_towerDefenseEnabled;
    static volatile LONG s_towerDefenseWorldId;
    static volatile LONG s_towerDefenseRegionId;
    static volatile LONG s_towerDefenseTeam1MobId;
    static volatile LONG s_towerDefenseTeam1Cape;
    static volatile LONG s_towerDefenseTeam2MobId;
    static volatile LONG s_towerDefenseTeam2Cape;
    static volatile LONG s_freeForAllEnabled;
    static volatile LONG s_freeForAllWorldId;
    static volatile LONG s_freeForAllRegionId;

private:
    static BOOL __fastcall MyIGObj_CanAttack(IGObj* pObjFirst, void*, IGObj* pObjSecond, int a3, WORD& pwResultCode);
public:
    static bool Initialize();
    static void Shutdown();
    static void ConfigureTowerDefense(bool enabled, int worldId, int regionId,
                                      int team1MobId, int team1Cape,
                                      int team2MobId, int team2Cape);
    static void ConfigureFreeForAll(bool enabled, int worldId, int regionId);

    /// Returns false when __AttackRestrictions prevents this player from
    /// interacting with this mob. The same decision is used for player->mob
    /// and mob->player combat to keep restrictions symmetric.
    static bool CanMobInteractWithPlayer(CGObjMob* pMob, CGObjPC* pPC, BYTE* pReason = NULL);
    static void ForgetPlayer(CGObjPC* pPC);
};
