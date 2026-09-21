//
// Created by kyuubi09 on 3/30/2023.
//
#pragma once

#include "GObjMobile.h"

#include "IStorage.h"
#include "GStorage.h"

#include "CmdSrcNet.h"
#include "Param.h"
#include "GItem.h"
#include <Objects/GObjChar.h>
#include <list>
#include <map>
#include <Objects/GObjPCData.h>
enum StallUpdateType : byte
{
    UpdateItem = 1,
    AddItemToSlot = 2,
    RemoveItem = 3,
    FleaMarketMode = 4,
    State = 5,
    Message = 6,
    Name = 7,
};
enum E_INVENTORY_OP_TYPE : uint8_t
{
    UPDATE_SLOTS_INV = 0,
    UPDATE_SLOTS_CHEST = 1,
    DEPOSIT_ITEM = 2,
    WITHDRAW_ITEM = 3,
    ADD_EXCHANGE = 4,
    DEL_EXCHANGE = 5,
    PICK_ITEM = 6,
    DROP_ITEM = 7,
    BUY_ITEM = 8,
    SELL_ITEM = 9,
    DROP_GOLD = 10,
    DEPOSIT_GOLD = 11,
    WITHDRAW_GOLD = 12,
    UPDATE_EXCHANGE_GOLD = 13,
    ADD_ITEM_BY_SERVER = 14,
    DEL_ITEM_BY_SERVER = 15,
    UPDATE_SLOTS_INV_COS = 16,
    PICK_ITEM_COS = 17,
    DROP_ITEM_COS = 18,
    BUY_ITEM_COS = 19,
    SELL_ITEM_COS = 20,
    DEL_COSITEM_BY_SERVER = 22,
    BUY_CASH_ITEM = 24,
    MOVE_ITEM_PET_PC = 26,
    MOVE_ITEM_PC_PET = 27,
    PICK_ITEM_BY_OTHER = 28,
    GUILD_CHEST_UPDATE_SLOT = 29,
    GUILD_CHEST_DEPOSIT_ITEM = 30,
    GUILD_CHEST_WITHDRAW_ITEM = 31,
    GUILD_CHEST_DEPOSIT_GOLD = 32,
    GUILD_CHEST_WITHDRAW_GOLD = 33,
    RESTORE_SOLDITEM_INSHOP = 34,
    MOVE_ITEM_AVATAR_PC = 35,
    MOVE_ITEM_PC_AVATAR = 36,
    MOVE_ITEM_TRADE_NOW = 38,
    PUSH_ITEM_INTO_MAGIC_CUBE = 39,
    POP_ITEM_FROM_MAGIC_CUBE = 40,
    DEL_ITEM_IN_MAGIC_CUBE = 41,
    BUY_ITEM_WITH_TOKEN = 43
};
class CGObjChar : public CGObjMobile {
DECLARE_DYNAMIC_EXISTING(CGObjChar, 0x00adeb7c)

public:
    char pad_0178[16]; //0x0178
    CCmdSrcNet* m_pSrcNet; //0x0188
    char pad_018C[92]; //0x018C
    int ZERKING; //0x01E8
    char pad_01EC[100]; //0x01EC
    CParam* SpeedParamPtr; //0x0250
    char pad_0254[384]; //0x0254
    int N000005C6; //0x03D4
    char pad_03D8[1576]; //0x03D8
    uint32_t CurHealth; //0x0A00
    uint32_t CurMana; //0x0A04
    char pad_0A08[16]; //0x0A08
    uint32_t RegionID; //0x0A18
    char pad_0A1C[20]; //0x0A1C
    CGObjPCData pData; //0x0A30
    char pad_0CA0[3948]; //0x0CA0
    CGStorage m_PCInventory; //0x1C10
    CGStorage m_PCAvatarInventory; //0x1C34
    char pad_1C58[8]; //0x1C58
    void* CAlchemyMagicOptionOP; //0x1C60
    void* CSetMagicOptionOP; //0x1C64
    void* CSocketOptionOP; //0x1C68
    char pad_1C6C[52]; //0x1C6C
public:
    void OnDeleteObject() override;

    void Func_214() override;

    void Func_125() override;

    void Func_172() override;

    void Func_215() override;

    void Func_219() override;

    void Func_194() override;

    void Func_211() override;

    void Func_299() override;

    void Func_300() override;

    void Func_156() override;

    void Func_227() override;

    void Func_293() override;

    void Func_292() override;

    void Func_291() override;

    void Func_124() override;

    void Func_197() override;

    void Func_198() override;

    void Func_303() override;

    MOTIONSTATE GetMotionState() const override;

    BODYMODE GetBodyMode() const override;

    double GetParam(GOBJ_PARAM wParamID) const override;

    int GetHealth() const override;

    int GetMana() const override;

    void Func_130() override;

    void Func_149() override;

    void Func_92() override;

public:
    virtual void Func_306();

    virtual void Func_307();

    virtual void Func_308();

    virtual void Func_309();

    virtual void Func_310();

    virtual void Func_311();

    virtual void Func_312();

    virtual void Func_313();

    virtual void Func_314();

    virtual void Func_315();

    virtual void Func_316();

    virtual void Func_317();

    virtual void Func_318();

    virtual void Func_319();

    virtual void Func_320();

    virtual void Func_321();

    virtual void Func_322();

    virtual void Func_323();

    virtual void Func_324();

    virtual void Func_325();

    virtual void Func_326();

    virtual void Func_327();

    virtual void Func_328();

    virtual void Func_329();

    virtual void Func_330();

    virtual void Func_331();

    virtual void Func_332();

    virtual void Func_333();

    virtual void Func_334();

    virtual void Func_335();

    virtual void Func_336();

    virtual void Func_337();

    virtual void Func_338();

    virtual void Func_339();

    virtual void Func_340();

    virtual void Func_341();

    virtual void Func_342();

    virtual void Func_343();

    virtual void Func_344();

    virtual void Func_345();

    virtual void Func_346();

    virtual void Func_347();

    virtual void Func_348();

    virtual void Func_349();

    virtual void Func_350();

    virtual void Func_351();

    virtual void Func_352();

    virtual void Func_353();

    virtual void Func_354();

    virtual void Func_355();

    virtual void Func_356();

    virtual void Func_357();

    virtual void Func_358();

    virtual void Func_359();

    virtual void Func_360();

    virtual void Func_361();

    virtual void Func_362();

    virtual void Func_363();

    virtual void Func_364();

    virtual void Func_365();

    virtual void Func_366();

    virtual void Func_367();

    virtual void Func_368();

    virtual void Func_369();

    virtual void Func_370();

    virtual void Func_371();

    virtual void Func_372();

    virtual void Func_373();

    virtual void Func_374();

    virtual void Func_375();

    virtual void Func_376();

    virtual void Func_377();

    virtual void Func_378();

    virtual void Func_379();

    virtual void Func_380();

    virtual void Func_381();

    virtual void Func_382();

    virtual void Func_383();

    virtual void Func_384();

    virtual void Func_385();

    virtual void Func_386();

    virtual void Func_387();

    virtual void Func_388();

    virtual void Func_389();

    virtual void Func_390();

    virtual void Func_391();

    virtual void Func_392();

    virtual void Func_393();

    virtual void Func_394();

    virtual void Func_395();

    virtual void Func_396();

public:
    static IStorage_ *GetStorageOP(int nType);
    CGItem *GetItemChar(int slot);
    CGItem* GetItemChar2(int slot);
    void Fun_214();
    void Testx(int param_1, int p2);
    int Test(int p1, int p2);
    unsigned int Fun_005291d0(unsigned int p1, unsigned int p2, unsigned int p3);
    unsigned int MobAttack(int param_1, int param_2, int param_3);
    void Fun_004a9b70(unsigned int p1, unsigned int p2);
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x1CA0)
        ENSURE_OFFSET(m_PCInventory, 0x1C10)
    END_FIXTURE()

    RUN_FIXTURE(CGObjChar)
};