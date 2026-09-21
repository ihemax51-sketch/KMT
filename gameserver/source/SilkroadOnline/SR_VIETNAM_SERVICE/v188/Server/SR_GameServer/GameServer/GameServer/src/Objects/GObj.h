//
// Created by Kurama on 12/11/2022.
//
#pragma once

#include "Base.h"
#include "InstanceObj.h"

#include "Interface/IGObj.h"

#include "World/GameWorldContextEX.h"
#include "InstancePC.h"

class CGObj : public IGObj, public CBase {
DECLARE_DYNAMIC_EXISTING(CGObj, 0x00adebb8)

public:
    void OnDeleteObject() override;

    DWORD GetRefObjID() const override;

    DWORD GetGameID() const override;

    WORD GetTypeID() const override;

    void GetWorldID(SWorldID &dwResult) const override;

    bool IsChar() const override;

    bool IsPC() const override;

    bool IsNPC() const override;

    bool IsNPCNPC() const override;

    bool IsMonster() const override;

    bool IsCOS() const override;

    bool IsCart() const override;

    bool IsVehicle() const override;

    bool IsPet() const override;

    bool IsMercenary() const override;

    bool IsAttackableCOS() const override;

    bool IsStruct() const override;

    bool IsItem() const override;

    bool IsExpendables() const override;

    bool IsContainerItem() const override;

    bool IsContainerSummon() const override;

    bool IsSpecialty() const override;

    bool IsPotion() const override;

    bool IsScroll() const override;

    bool IsBullet() const override;

    bool IsArrow() const override;

    bool IsBolt() const override;

    bool IsHPPotion() const override;

    bool IsMPPotion() const override;

    bool IsElixir() const override;

    bool IsGold() const override;

    bool IsTownPortal() const override;

    bool IsEquip() const override;

    bool IsWeapon() const override;

    bool IsShield() const override;

    bool IsAccessory() const override;

    bool IsTeleportGate() const override;

    bool IsRobe() const override;

    bool IsArmor() const override;

    bool IsBow() const override;

    bool IsCrossBow() const override;

    bool IsSummonScroll() const override;

    bool IsReinforceRecipe() const override;

    bool IsReinforceProbUP() const override;

    bool IsFreeBattleEquip() const override;

    bool IsQuest_n_EventItem() const override;

    bool IsCashItem() const override;

    bool CanTrade() const override;

    bool CanSell() const override;

    bool CanBorrow() const override;

    bool CanDrop() const override;

    bool CanPick() const override;

    bool CanRepair() const override;

    bool CanRevive() const override;

    bool CanUse() const override;

    bool CanThrow() const override;

    const char *GetCodeName() const override;

    void GetPosInfo(SPosInfo &pInfo) const override;

    LIFESTATE GetLifeState() const override;

    void Func_77() override;

    void Func_78() override;

    void Func_79() override;

    void Func_80() override;

    void Func_81() override;

    void Func_82() override;

    void Func_83() override;

    void Func_124() override;

    void Func_148() override;

    void Func_150() override;

    void Func_151() override;

    void Func_152() override;

    void Func_172() override;

    void Func_181() override;

    void Func_182() override;

    void Func_183() override;

    void Func_195() override;

public:
    virtual void Func_211();

    virtual void Func_212();

    virtual void Func_213() = 0;

    virtual void Func_214();

    virtual void Func_215();

    virtual void Func_216();

    virtual void Func_217();

    virtual void Func_218();

    virtual void Func_219();

    virtual void Func_220();

    virtual void Func_221();

    virtual void Func_222();

    virtual void Func_223() = 0;

    virtual void Func_224() = 0;

    virtual void Func_225();

    virtual void Func_226();

    virtual void Func_227();

    virtual void Func_228();

    virtual void Func_229();

    virtual void Func_230();

    virtual void Func_231();

    virtual void Func_232();

    virtual void Func_233();

    virtual void Func_234();

    virtual void Func_235();

    virtual void Func_236();

    virtual void Func_237();

    virtual void Func_238();

    virtual void Func_239();

    virtual void Func_240();

    virtual void Func_241();

    virtual void Func_242();

    virtual void Func_243();

    virtual void Func_244();

    virtual void Func_245();

    virtual void Func_246();

    virtual void Func_247();

    virtual void Func_248();

    virtual void Func_249();

    virtual void Func_250();

    virtual void Func_251();

    virtual void Func_252();

    virtual void Func_253();

    virtual void Func_254();

    virtual void Func_255();

    virtual void Func_256();

    virtual void Func_257();

    virtual void Func_258();

    virtual void Func_259();

    virtual void Func_260();

    virtual void Func_261();

    virtual void Func_262();

    virtual void Func_263();

    virtual void Func_264();

    virtual void Func_265();

    virtual void Func_266();

    virtual void Func_267();

    virtual void Func_268();

    virtual void Func_269();

    virtual void Func_270();

    virtual void Func_271();

    virtual void Func_272();

    virtual void Func_273();

    virtual void Func_274();

    virtual void Func_275();

    virtual void Func_276();

    virtual void Func_277();

    virtual void Func_278();

    virtual void Func_279();

    virtual void Func_280();

    virtual void Func_281();

    virtual void Func_282();

    virtual void Func_283();

    virtual void Func_284();

    virtual void Func_285();

    virtual void Func_286();

    virtual void Func_287();

    virtual void Func_288();

    virtual void Func_289();

    virtual void Func_290();

    virtual void Func_291();

    virtual void Func_292();

    virtual void Func_293();

    virtual void Func_294() = 0;

    virtual void Func_295();

    virtual void Func_296();

    virtual void Func_297();

    virtual void Func_298();

    virtual void Func_299();

public:
    DWORD m_dwGameId; //0x0008
    char pad_000c[36]; //0x000C
    stObjectStates *m_pLifeState; //0x0030
    CInstancePC *m_pObjDataInstance; //0x0034
    char pad_0038[60]; //0x0038
    GameWorldContextEX m_ObjWorldContext; //0x0074
    char pad_00dc[112]; //0x00DC
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x14c)
        ENSURE_OFFSET(m_dwGameId, 0x0008)
        ENSURE_OFFSET(m_pLifeState, 0x0030)
        ENSURE_OFFSET(m_pObjDataInstance, 0x0034)
        ENSURE_OFFSET(m_ObjWorldContext, 0x0074)
    END_FIXTURE()

    RUN_FIXTURE(CGObj)
};