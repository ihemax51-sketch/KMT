//
// Created by Kurama on 12/11/2022.
//
#pragma once

#include "Data.h"

#include "Helpers.h"
#include "Msg.h"

class IGObj {
public:
    virtual void OnDeleteObject() = 0;

    virtual DWORD GetRefObjID() const {
        return 0;
    }

    virtual int GetJID() const {
        return 0;
    }

    virtual DWORD GetGameID() const {
        return 0;
    }

    virtual WORD GetTypeID() const {
        return 0;
    }

    virtual void GetWorldID(SWorldID &dwResult) const {

        dwResult.wLayerID = 1;
        dwResult.wWorldID = 1;
    }

    virtual bool IsChar() const {

        return false;
    }

    virtual bool IsPC() const {

        return false;
    }

    virtual bool IsNPC() const {

        return false;
    }

    virtual bool IsNPCNPC() const {

        return false;
    }

    virtual bool IsMonster() const {

        return false;
    }

    virtual bool IsCOS() const {

        return false;
    }

    virtual bool IsCart() const {

        return false;
    }

    virtual bool IsVehicle() const {

        return false;
    }

    virtual bool IsPet() const {

        return false;
    }

    virtual bool IsMercenary() const {

        return false;
    }

    virtual bool IsAttackableCOS() const {

        return false;
    }

    virtual bool IsStruct() const {

        return false;
    }

    virtual bool IsItem() const {

        return false;
    }

    virtual bool IsExpendables() const {

        return false;
    }

    virtual bool IsContainerItem() const {

        return false;
    }

    virtual bool IsContainerSummon() const {

        return false;
    }

    virtual bool IsSpecialty() const {

        return false;
    }

    virtual bool IsPotion() const {

        return false;
    }

    virtual bool IsScroll() const {

        return false;
    }

    virtual bool IsBullet() const {

        return false;
    }

    virtual bool IsArrow() const {

        return false;
    }

    virtual bool IsBolt() const {

        return false;
    }

    virtual bool IsHPPotion() const {

        return false;
    }

    virtual bool IsMPPotion() const {

        return false;
    }

    virtual bool IsElixir() const {

        return false;
    }

    virtual bool IsGold() const {

        return false;
    }

    virtual bool IsTownPortal() const {

        return false;
    }

    virtual bool IsEquip() const {

        return false;
    }

    virtual bool IsWeapon() const {

        return false;
    }

    virtual bool IsShield() const {

        return false;
    }

    virtual bool IsAccessory() const {

        return false;
    }

    virtual bool IsTeleportGate() const {

        return false;
    }

    virtual bool IsRobe() const {

        return false;
    }

    virtual bool IsArmor() const {

        return false;
    }

    virtual bool IsBow() const {

        return false;
    }

    virtual bool IsCrossBow() const {

        return false;
    }

    virtual bool IsSummonScroll() const {

        return false;
    }

    virtual bool IsReinforceRecipe() const {

        return false;
    }

    virtual bool IsReinforceProbUP() const {

        return false;
    }

    virtual bool IsFreeBattleEquip() const {

        return false;
    }

    virtual bool IsQuest_n_EventItem() const {

        return false;
    }

    virtual bool IsCashItem() const {

        return false;
    }

    virtual bool Func_48() const {
        return false;
    }

    virtual bool CanTrade() const {

        return false;
    }

    virtual bool CanSell() const {

        return false;
    }

    virtual bool CanBorrow() const {

        return false;
    }

    virtual bool CanDrop() const {

        return false;
    }

    virtual bool CanPick() const {

        return false;
    }

    virtual bool CanRepair() const {

        return false;
    }

    virtual bool CanRevive() const {

        return false;
    }

    virtual bool CanUse() const {

        return false;
    }

    virtual bool CanThrow() const {

        return false;
    }

    virtual const char *GetCodeName() const {

        return 0;
    }

    virtual const char *GetCharName() const {

        return 0;
    }

    virtual const char *GetNickName() const {

        return 0;
    }

    virtual void GetPosInfo(SPosInfo &pInfo) const {

    }

    virtual LIFESTATE GetLifeState() const {

        return LIFESTATE_EMBRYO;
    }

    virtual MOTIONSTATE GetMotionState() const {

        return MOTIONSTATE_STAND;
    }

    virtual BODYMODE GetBodyMode() const {

        return BODYMODE_NORMAL;
    }

    virtual double GetParam(GOBJ_PARAM wParamID) const {
        return 0.0f;
    }

    virtual int GetHealth() const {

        return 0;
    }

    virtual int GetMana() const {

        return 0;
    }

    virtual int GetMaxHealth() const {

        return 0;
    }

    virtual int GetMaxMana() const {

        return 0;
    }

    virtual bool IsTargeting() const {

        return false;
    }

    virtual JOBSTATE GetJobState() const {

        return JOBSTATE_NONE;
    }

    virtual TELEPORTSTATE GetTeleportState() const {

        return TELEPORTSTATE_NONE;
    }

    virtual BYTE GetLevel() const {

        return 0;
    }

    virtual BYTE GetMaxLevel() const {

        return 0;
    }

    virtual MONSTERCLASS GetMonsterClass() const {

        return MONSTERCLASS_NONE;
    }

    virtual bool GetPartyInfo(struct SPartyInfo *) const {

        return false;
    }

    virtual void Func_77();

    virtual void Func_78();

    virtual void Func_79();

    virtual void Func_80();

    virtual void Func_81();

    virtual void Func_82();

    virtual void Func_83();

    virtual void Func_84();

    virtual void Func_85();

    virtual void Func_86();

    virtual void Func_87();

    virtual void Func_88();

    virtual void Func_89();

    virtual void Func_90();

    virtual void Func_91();

    virtual void Func_92();

    virtual void Func_93();

    virtual void Func_94();

    virtual void Func_95();

    virtual void Func_96();

    virtual void Func_97();

    virtual void Func_98();

    virtual void Func_99();

    virtual void Func_100();

    virtual void Func_101();

    virtual void Func_102();

    virtual void Func_103();

    virtual void Func_104();

    virtual void Func_105();

    virtual void Func_106();

    virtual void Func_107();

    virtual void Func_108();

    virtual void Func_109();

    virtual void Func_110();

    virtual void Func_111();

    virtual void Func_112();

    virtual void Func_113();

    virtual void Func_114();

    virtual void Func_115();

    virtual void Func_116();

    virtual void Func_117();

    virtual void Func_118();

    virtual void Func_119();

    virtual void Func_120();

    virtual void Func_121();

    virtual void Func_122();

    virtual void Func_123();

    virtual void Func_124();

    virtual void Func_125();

    virtual void Func_126();

    virtual void Func_127();

    virtual void Func_128();

    virtual void Func_129();

    virtual void Func_130();

    virtual void Func_131();

    virtual void Func_132();

    virtual void Func_133();

    virtual void Func_134();

    virtual void Func_135();

    virtual void Func_136();

    virtual void Func_137();

    virtual void Func_138();

    virtual void Func_139();

    virtual void Func_140();

    virtual void Func_141();

    virtual void Func_142();

    virtual void Func_143();

    virtual void Func_144();

    virtual void Func_145();

    virtual void Func_146();

    virtual void Func_147();

    virtual void Func_148();

    virtual void Func_149();

    virtual void Func_150();

    virtual void Func_151();

    virtual void Func_152();

    virtual void Func_153();

    virtual void Func_154();

    virtual void Func_155();

    virtual void Func_156();

    virtual void Func_157();

    virtual CMsg *AllocMsgForPeer(WORD wMsgId);

    virtual void SendMsgToPeer(CMsg *pMSg);

    virtual void Func_160();

    virtual void Func_161();

    virtual void Func_162();

    virtual void Func_163();

    virtual void Func_164();

    virtual void Func_165();

    virtual void Func_166();

    virtual void Func_167();

    virtual void Func_168();

    virtual void Func_169();

    virtual void Func_170();

    virtual void Func_171();

    virtual void Func_172();

    virtual void Func_173();

    virtual void Func_174();

    virtual void Func_175();

    virtual void Func_176();

    virtual void Func_177();

    virtual void Func_178();

    virtual void Func_179();

    virtual void Func_180();

    virtual void Func_181();

    virtual void Func_182();

    virtual void Func_183();

    virtual void Func_184();

    virtual void Func_185();

    virtual void Func_186();

    virtual void Func_187();

    virtual void Func_188();

    virtual void Func_189();

    virtual void Func_190();

    virtual void Func_191();

    virtual void Func_192();

    virtual void Func_193();

    virtual void Func_194();

    virtual void Func_195();

    virtual void Func_196();

    virtual void Func_197();

    virtual void Func_198();

    virtual void Func_199();

    virtual void Func_200();

    virtual void Func_201();

    virtual void Func_202();

    virtual void Func_203();

    virtual void Func_204();

    virtual void Func_205();

    virtual void Func_206();

    virtual void Func_207();

    virtual void Func_208();

    virtual void Func_209();

    virtual void Func_210();

public:
    IGObj* GetOwner() const;
};