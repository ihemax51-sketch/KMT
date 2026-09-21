//
// Created by Kurama on 12/12/2022.
//
#pragma once

#include "Windows.h"
#include <string>

#include <Test/Test.h>

#include "Database/DBRecStream.h"
#include "TypeId.h"

class   RefObjBaseData;
class   RefObjCommon;
class   RefObjChar;
class   RefObjItem;

class RefObjBaseData {
public:
private:
};

class RefObjCommon : public RefObjBaseData {
public:
    char pad_0004[4]; //0x0004
    DWORD m_dwObjectId; //0x0008
    char pad_000c[4]; //0x000C
    std::string m_strObjectCode; //0x0010
    std::string m_strObjectName; //0x002C
    std::string m_strObjectNameStrID; //0x0048
    std::string m_strObjectNameStrIdDesc; //0x0064
    TypeId TID; //0x0080
    char pad_0084[8]; //0x0084
    DWORD dwAvailableOPs; //0x008C
    char pad_0090[12]; //0x0090
    DWORD m_dwPrice; //0x009C
    char pad_00A0[4]; //0x00A0
    DWORD m_dwCostRepair; //0x00A4
    DWORD m_dwCostRevive; //0x00A8
    DWORD m_dwKeepingFee; //0x00AC
    DWORD m_dwSellPrice; //0x00B0
    DWORD m_dwReqLevelType1; //0x00B4
    DWORD m_dwReqLevel1; //0x00B
    DWORD m_dwReqLevelType2; //0x00BC
    DWORD m_dwReqLevel2; //0x00C0
    DWORD m_dwReqLevelType3; //0x00C4
    DWORD m_dwReqLevel3; //0x00C8
    DWORD m_dwReqLevelType4; //0x00CC
    DWORD m_dwReqLevel4; //0x00D0
    DWORD m_dwMaxContain; //0x00D4
    DWORD m_dwRegionId; //0x00D8
    DWORD m_dwDir; //0x00DC
    DWORD m_dwSpeed1; //0x00E0
    DWORD m_dwSpeed2; //0x00E4
    DWORD m_dwScale; //0x00E8
    DWORD m_dwBCHeight; //0x00EC
    DWORD m_dwBCRadius; //0x00F0
    std::string m_strAssocFileObj; //0x00F4
    std::string m_strAssocFileDrop; //0x0110
    std::string m_strAssocFileIcon; //0x012C
    std::string m_strAssocFile1; //0x0148
    std::string m_strAssocFile2; //0x0164
    char pad_0180[4]; //0x0180
    DWORD m_dwLink; //0x0184
    char pad_0188[16]; //0x0188
public:
    virtual ~RefObjCommon();

    virtual DWORD ExtractData(CDBRecStream* pTable);

private:
    BEGIN_FIXTURE()
        ENSURE_SIZE(0x0198) // stolen from RefObjDummy, Thanks Joymax ! ^_^
        END_FIXTURE()

        RUN_FIXTURE(RefObjCommon)
};

class RefObjChar : public RefObjCommon {
public:
    DWORD ExtractData(CDBRecStream* pTable) override;

private:
    char pad_0198[624]; //0x0198
private:
    BEGIN_FIXTURE()
        ENSURE_SIZE(0x0408) // stolen from 006b3fa3
        END_FIXTURE()

        RUN_FIXTURE(RefObjChar)
};

enum {
    PHYSICAL_MAGIC_MIN,
    PHYSICAL_MAGIC_MAX,
};

class RefObjItem : public RefObjCommon {
public:
    DWORD m_dwItemMaxStack; //0x0198
    BYTE m_btItemReqGender; //0x019C
    DWORD m_dwItemReqStr; //0x01A0
    DWORD m_dwItemReqInt; //0x01A4
    BYTE m_btItemClass; //0x01A8
    DWORD m_dwItemSetID; //0x01AC
    float m_fItemDurability[2]; //0x01B0
    float m_fItemDurabilityIncrease; //0x01B8 // NULL
    float m_fPhyDefense[2]; //0x01BC
    float m_fPhyDefenseIncrease; //0x01C4 // NULL
    float m_fEvasionRatio[2]; //0x01C8
    float m_fEvasionRatioIncrease; //0x01D0 // NULL
    float m_fPhyReinforce[2]; //0x01D4
    float m_fPhyReinforceIncrease; //0x01DC // NULL
    float m_fBlockRatio[2]; //0x01E0
    float m_fBlockRatioIncrease; //0x01E8 // NULL
    float m_fMagicDefense[2]; //0x01EC
    float m_fMagicDefenseIncrease; //0x01F4 // NULL
    float m_fMagAbsorption[2]; //0x01F8
    float m_fMagAbsorptionIncrease; //0x0200 // NULL
    float m_fPhyDefenseStr[2]; //0x0204
    float m_fPhyDefenseStrIncrease; //0x020C // NULL
    float m_fMagDefenseInt[2]; //0x0210
    float m_fMagDefenseIntIncrease; //0x0218 // NULL
    bool m_bQuiveredItem; //0x021C
    BYTE m_btAmmoTID4[5]; //0x021D
    BYTE m_btItemSpeedClass; //0x0222
    bool m_bTwoHandedItem; //0x0223
    WORD m_wItemRange; //0x0224
    float m_fPhyAttackMin[2]; //0x0228
    float m_fPhyAttackMinIncrease; //0x0230 // NULL
    float m_fPhyAttackMax[2]; //0x0234
    float m_fPhyAttackIncrease; //0x023C
    float m_fMagAttackMin[2]; //0x0240
    float m_fMagAttackMinIncrease; //0x0248 // NULL
    float m_fMagAttackMax[2]; //0x024C
    float m_fMagAttackIncrease; //0x0254
    float m_fPhyAttackStrMin[2]; //0x0258
    float m_fPhyAttackStrMinIncrease; //0x0260 // NULL
    float m_fPhyAttackStrMax[2]; //0x0264
    float m_fPhyAttackStrMaxIncrease; //0x026C // NULL
    float m_fMagAttackIntMin[2]; //0x0270
    float m_fMagAttackIntMinIncrease; //0x0278 // NULL
    float m_fMagAttackIntMax[2]; //0x027C
    float m_fMagAttackIntMaxIncrease; //0x0284 // NULL
    float m_fHitRate[2]; //0x0288
    float m_fHitRateIncrease; //0x0290
    float m_fCriticalHitRate[2]; //0x0294
    float m_fCriticalHitRateIncrease; //0x029C // NULL
    int Param1; // 0x2A0
    std::string Desc1; //0x02A4
    int Param2; //0x02C0
    std::string Desc2; //0x02C4
    int Param3; //0x02E0
    std::string Desc3; //0x02E4
    int Param4; //0x0300
    std::string Desc4; //0x0304
    int Param5; //0x0320
    std::string Desc5; //0x0324
    int Param6; //0x0340
    std::string Desc6; //0x0344
    int Param7; //0x0360
    std::string Desc7; //0x0364
    int Param8; //0x0380
    std::string Desc8; //0x0384
    int Param9; //0x03A0
    std::string Desc9; //0x03A4
    int Param10; //0x03C0
    std::string Desc10; //0x03C4
    int Param11; //0x03E0
    std::string Desc11; //0x03E4

    char pad_02A0[318]; //0x02A0
public:
    DWORD ExtractData(CDBRecStream* pTable) override;

private:
    BEGIN_FIXTURE()
        ENSURE_SIZE(0x0540)
        END_FIXTURE()

        RUN_FIXTURE(RefObjItem)
};
