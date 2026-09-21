#pragma once

#include "support/AbiTest.h"
#include <string>

#include <TypeId.h>
#include "CommonData.h"

struct SItemData : public SCommonData {

    bool IsGlobalMessageScroll() const;

    bool IsItemEtc() const;

    bool IsAmmo() const;

    bool IsItemCOS() const;

    bool IsReverse() const;
    bool IsAvatar() const;
    bool IsAvatarHat() const;
    bool IsAvatarAttach() const;
    bool IsNasrun() const;
    bool IsHPPotion() const;
    bool IsMPPotion() const;
    bool IsVIGOR() const;
    bool IsPill() const;
    bool IsPuriPill() const;
    bool IsPETHpPotion() const;
    bool IsPETMpPotion() const;
    bool IsCureCOS() const;
    bool IsPetRes() const;
    bool IsAttackPet() const;
    bool IsResurrectScroll() const;
    bool IsReturnScroll() const;
    bool IsRepairHammer() const;
    bool IsArrow() const;
    bool IsBolt() const;
    bool IsAlchemyTablet() const;
    bool IsMagicStone() const;
    bool IsMagicStone2() const;
    bool IsAttrStone() const;
    bool IsElixir() const;
    bool IsAlchemyMaterial() const;
    bool IsZerkScroll() const;
    bool IsMagicPop() const;
    bool IsMagicPopWinCard() const;
    bool IsLuckyPowder() const;
    bool IsEnhancer() const;
    bool IsProofStone() const;

    bool IsChSword() const;
    bool IsChBlade() const;
    bool IsChSpear() const;
    bool IsChBow() const;
    bool IsChTBlade() const;


    bool IsEuSword() const;
    bool IsEuTSword() const;
    bool IsEuAxe() const;
    bool IsEuDarkStaff() const;
    bool IsEuTStaff() const;
    bool IsEuCrossbow() const;
    bool IsEuDagger() const;
    bool IsEuHarp() const;
    bool IsEuStaff() const;


    unsigned int m_maxStack;          //0x01C0
    unsigned int m_reqGender;         //0x01C4
    unsigned int n_reqStr;            //0x01C8
    unsigned int m_reqInt;            //0x01CC
    byte m_itemClass;         //0x01D0
    char pad[3];
    unsigned int m_setId;             //0x01D4
    unsigned int m_durabilityMin;     //0x01D8
    unsigned int m_durabilityMax;     //0x01DC
    float m_phyDefMin;                    //0x01E0
    float m_phyDefMax;                    //0x01E4
    float m_phyDefIncrease;               //0x01E8
    unsigned int m_evasionRatioMin;   //0x01EC
    unsigned int m_evasionRatioMax;   //0x01F0
    float m_evasionRatioIncrease;         //0x01F4
    unsigned int m_PARMin;            //0x01F8
    unsigned int m_PARMax;            //0x01FC
    float m_PARIncrease;                  //0x0200
    unsigned int m_blockRatioMin;     //0x0204
    unsigned int m_blockRatioMax;     //0x0208
    float m_magDefMin;                    //0x020C
    float m_magDefMax;                    //0x0210
    float m_magDefIncrease;               //0x0214
    unsigned int m_MARMin;            //0x0218
    unsigned int m_MARMax;            //0x021C
    float m_MARIncrease;                  //0x0220
    unsigned int m_phyDefStrMin;      //0x0224
    unsigned int m_phyDefStrMax;      //0x0228
    unsigned int m_magDefIntMin;      //0x022C
    unsigned int m_magDefIntMax;      //0x0230
    unsigned int m_quivered;          //0x0234
    unsigned int m_ammo1TID4;         //0x0238
    unsigned int m_ammo2TID4;         //0x023C
    unsigned int m_ammo3TID4;         //0x0240
    unsigned int m_ammo4TID4;         //0x0244
    unsigned int m_ammo5TID4;         //0x0248
    unsigned int m_speedClass;        //0x024C
    unsigned int m_twoHanded;         //0x0250
    unsigned int m_range;             //0x0254
    unsigned int m_phyAttackMinMin;   //0x0258
    unsigned int m_phyAttackMinMax;   //0x025C
    unsigned int m_phyAttackMaxMin;   //0x0260
    unsigned int m_phyAttackMaxMax;   //0x0264
    float m_phyAttackIncrease;            //0x0268
    unsigned int m_magAttackMinMin;   //0x026C
    unsigned int m_magAttackMinMax;   //0x0280
    unsigned int m_magAttackMaxMin;   //0x0284
    unsigned int m_magAttackMaxMax;   //0x0288
    float m_magAttackIncrease;            //0x028C
    unsigned int m_phyAttackStrMinMin;//0x0280
    unsigned int m_phyAttackStrMinMax;//0x0284
    unsigned int m_phyAttackStrMaxMin;//0x0288
    unsigned int m_phyAttackStrMaxMax;//0x028C
    unsigned int m_magAttackIntMinMin;//0x0290
    unsigned int m_magAttackIntMinMax;//0x0294
    unsigned int m_magAttackIntMaxMin;//0x0298
    unsigned int m_magAttackIntMaxMax;//0x029C
    unsigned int m_hitRateMin;        //0x02A0
    unsigned int m_hitRateMax;        //0x02A4
    float m_hitRateIncrease;              //0x02A8
    unsigned int m_criticalHitRateMin;//0x02AC
    unsigned int m_criticalHitRateMax;//0x02B0
    int m_param1;              //0x02B4
    int m_param2;              //0x02B8
    int m_param3;              //0x02BC
    int m_param4;              //0x02C0
    int m_param5;              //0x02C4
    int m_param6;              //0x02C8
    int m_param7;              //0x02CC
    int m_param8;              //0x02D0
    int m_param9;              //0x02D4
    int m_param10;             //0x02D8
    int m_param11;             //0x02DC
    int m_param12;             //0x02E0
    int m_param13;             //0x02E4
    int m_param14;             //0x02E8
    int m_param15;             //0x02EC
    int m_param16;             //0x02F0
    int m_param17;             //0x02F4
    int m_param18;             //0x02F8
    int m_param19;             //0x02FC
    int m_param20;             //0x0300
    std::n_wstring m_desc1_128;
    std::n_wstring m_desc2_128;
    std::n_wstring m_desc3_128;
    std::n_wstring m_desc4_128;
    std::n_wstring m_desc5_128;
    std::n_wstring m_desc6_128;
    std::n_wstring m_desc7_128;
    std::n_wstring m_desc8_128;
    std::n_wstring m_desc9_128;
    std::n_wstring m_desc10_128;
    std::n_wstring m_desc11_128;
    std::n_wstring m_desc12_128;
    std::n_wstring m_desc13_128;
    std::n_wstring m_desc14_128;
    std::n_wstring m_desc15_128;
    std::n_wstring m_desc16_128;
    std::n_wstring m_desc17_128;
    std::n_wstring m_desc18_128;
    std::n_wstring m_desc19_128;
    std::n_wstring m_desc20_128;
    // m_maxMagicOptCount ?? m_childItemCount ??

private:
    BEGIN_FIXTURE()
        ENSURE_OFFSET(m_maxStack, 0x01C0)
    END_FIXTURE()

    RUN_FIXTURE(SItemData)

};

class CItemData {
public:
    virtual ~CItemData();

    /// \address 00973420
    const SItemData &GetData() const;

private:
    // CItemData constructor is not to be used yet. Its private so no one can make an instance of this class
    CItemData();

private:
    char pad_0000[8]; //0x0000

    const SItemData mData;

private:
    BEGIN_FIXTURE()
        ENSURE_OFFSET(mData, 0x0010)
    END_FIXTURE()

    RUN_FIXTURE(CItemData)
};
