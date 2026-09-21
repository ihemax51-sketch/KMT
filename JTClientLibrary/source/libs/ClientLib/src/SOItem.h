#pragma once

#include "Data/ItemData.h"
#include <BSLib/BSLib.h>
#include <ghidra/undefined.h>
#include "Data/ObjectData.h"

enum Blue : short {
    BLUE_INT = 0x0005,
    BLUE_STR = 0x000b,
    BLUE_DURABILITY = 0x0050,
    BLUE_LUCKY = 0x003d,
    BLUE_LUCKY2 = 0x003b,
    BLUE_MP = 0x0098,
    BLUE_HP = 0x008C,
    BLUE_PARRY = 0x0080,
    BLUE_REPAIR_INVALID = 0x0041,
    BLUE_STEADY = 0x0035,
};

class CSOItem {
public:

    CSOItem();
    ~CSOItem();

    /// \address 008ba440
    const SItemData *GetItemData() const;

    /// \address 008ba460
    int GetQuantity() const;

    /// Checks if given items are of same type by comparing RefObjId
    /// \address 008bac80
    bool IsSameItemType(const CSOItem *other) const;
    ObjectData *GetObjectData();
    void ReadFromPacket(CMsgStreamBuffer* msg, BYTE param_3);
    void SetEnabled(bool enable);
public:
    char pad_0000[32]; //0x0000
    int enableItem; //0x0020
    int pad; //0x0024
    BYTE m_blValid; //0x0028
    char pad_0029[3]; //0x0029
    int activepetname; //0x002C
    char pad_0030[4]; //0x0030
    int m_refObjItemId; //0x0034
    int CosItemId; //0x0038
    int MaybeCosUQID; //0x003C
    std::n_wstring PetName;
    BYTE ActiveItemTime; //0x005C /// TODO FOR PETS 2 SPAWNED 3 CAN USED BUT NOT SPAWNED 4 DEAD UNUSABLE
    char pad_005D[3]; //0x005D
    INT64 MyitemTime; //0x0060
    INT64 MyitemTime2; //0x0068
    byte N00002B83; //0x0070
    byte AdvValue; //0x0071
    byte N00002B96; //0x0072
    byte N00002B98; //0x0073
    int SocketOptID1; //0x0074
    int nParam1; //0x0078
    int SocketOptID2; //0x007C
    int nParam2; //0x0080
    int SocketOptID3; //0x0084
    int nParam3; //0x0088
    BYTE m_OptLevel; //0x008C
    char pad_008D[3]; //0x008D
    UINT64 Variance; //0x0090
    int m_CurrDurability; //0x0098
    int m_quantity; //0x009C
    char pad_00A0[36]; //0x00A0
    std::n_map<Blue, int> bluemap; //0x00C4
    char pad_00D0[16]; //0x00D0
    int m_PhyAtkPwrMax; //0x00E0
    int m_PhyAtkPwrMin; //0x00E4
    int m_MagAtkPwrMax; //0x00E8
    int m_MagAtkPwrMin; //0x00EC
    int m_BlockingRateValue; //0x00F0
    int m_MaxDurability; //0x00F4
    float m_PhyDefPwrValue; //0x00F8
    int m_ParryRateValue; //0x00FC
    float m_PhyAbsorption; //0x0100
    int m_AttackRateValue; //0x0104
    int m_CriticalValue; //0x0108
    float m_MagDefPwrValue; //0x010C
    float m_MagAbsorption; //0x0110
    float m_PhyReinforcementMin; //0x0114
    float m_PhyReinforcementMax; //0x0118
    float m_MagReinforcementMin; //0x011C
    float m_MagReinforcementMax; //0x0120
    float m_PhyReinforcementValue; //0x0124
    float m_MagReinforcementValue; //0x0128
    char pad_012C[4]; //0x012C
    int m_AttackRateValuex; //0x0130
    int N00000F03; //0x0134
    int N00000F04; //0x0138
    int N00000F05; //0x013C
    int N00000F06; //0x0140
    char pad_0144[36]; //0x0144
    INT64 itemtimes; //0x0168
    //int its0; //0x016C
    int its1; //0x0170
    int its2; //0x0174
    int its1too; //0x0178
    char pad_017C[70]; //0x017C
    int ActiveMS;
    int ItemLocked;//0x01C8
    int havetime;//0x01CC

BEGIN_FIXTURE()
        ENSURE_SIZE(464)// approved by CIFExchange::ctor
        ENSURE_OFFSET(m_blValid, 0x28)
        ENSURE_OFFSET(m_refObjItemId, 0x34)
        ENSURE_OFFSET(m_OptLevel, 0x008C)
        ENSURE_OFFSET(m_CurrDurability, 0x0098)
        ENSURE_OFFSET(m_quantity, 0x9c)
        ENSURE_OFFSET(bluemap, 0xc4)
        ENSURE_OFFSET(m_PhyAtkPwrMax, 0x00E0)
        ENSURE_OFFSET(m_PhyAtkPwrMin, 0x00E4)
        ENSURE_OFFSET(m_MagAtkPwrMax, 0x00E8)
        ENSURE_OFFSET(m_MagAtkPwrMin, 0x00EC)
        ENSURE_OFFSET(m_BlockingRateValue, 0x00F0)
        ENSURE_OFFSET(m_MaxDurability, 0x00F4)
        ENSURE_OFFSET(m_PhyDefPwrValue, 0x00F8)
        ENSURE_OFFSET(m_ParryRateValue, 0x00FC)
        ENSURE_OFFSET(m_PhyAbsorption, 0x0100)
        ENSURE_OFFSET(m_CriticalValue, 0x0108)
        ENSURE_OFFSET(m_MagDefPwrValue, 0x010C)
        ENSURE_OFFSET(m_MagAbsorption, 0x0110)
        ENSURE_OFFSET(m_PhyReinforcementMin, 0x0114)
        ENSURE_OFFSET(m_PhyReinforcementMax, 0x0118)
        ENSURE_OFFSET(m_MagReinforcementMin, 0x011C)
        ENSURE_OFFSET(m_MagReinforcementMax, 0x0120)
        ENSURE_OFFSET(m_PhyReinforcementValue, 0x0124)
        ENSURE_OFFSET(m_MagReinforcementValue, 0x0128)
        ENSURE_OFFSET(m_AttackRateValuex, 0x0130)
    END_FIXTURE()

    RUN_FIXTURE(CSOItem)

};
