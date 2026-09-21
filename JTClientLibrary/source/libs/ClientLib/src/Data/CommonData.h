#pragma once

#include "BSLib/BSLib.h"
#include "TypeId.h"
#include "ghidra/undefined.h"

/**
 * Common data from CharacteData and ItemData
 * @note Field names match column names of file
 */
struct SCommonData {
    TypeId m_typeId; //0x0000
    int RefObjectId; //0x0004
    std::n_wstring CodeName; //0x0008
    std::n_wstring ObjName; //0x0024
    std::n_wstring OrgObjCodeName; //0x0040
    undefined pad_005C[4]; //0x005C
    std::n_wstring NameStrID; //0x0060
    std::n_wstring DescStrID; //0x007C
    int DecayTime; //0x0098
    int Country; //0x009C
    int Rarity; //0x00A0
    bool CanTrade; //0x00A4
    bool CanSell; //0x00A5
    bool CanBuy; //0x00A6
    bool CanBorrow; //0x00A7
    bool CanDrop; //0x00A8
    bool CanPick; //0x00A9
    bool CanRepair; //0x00AA
    bool CanRevive; //0x00AB
    bool CanUse; //0x00AC
    bool CanThrow; //0x00AD
    char pad_00AE[2]; //0x00AE
    unsigned long long Price; //0x00B0
    unsigned long long SellPrice; //0x00B8
    int CostRepair; //0x00C0
    int CostRevive; //0x00C4
    int CostBorrow; //0x00C8
    int KeepingFee; //0x00CC
    int ReqLevelType1; //0x00D0
    int ReqLevelType2; //0x00D4
    int ReqLevelType3; //0x00D8
    int ReqLevelType4; //0x00DC
    int ReqLevel1; //0x00E0
    int ReqLevel2; //0x00E4
    int ReqLevel3; //0x00E8
    int ReqLevel4; //0x00EC
    int MaxContain; //0x00F0
    int RegionId; //0x00F4
    int Dir; //0x00F8
    int OffsetX; //0x00FC
    int OffsetY; //0x0100
    int OffsetZ; //0x0104
    int Speed1; //0x0108
    int Speed2; //0x010C
    int Scale; //0x0110
    int BCHeight; //0x0114
    int BCRadius; //0x0118
    undefined pad_011C[8]; //0x011C
    std::n_string AssocFileObj; //0x0124
    std::n_string AssocFileDrop; //0x0140
    std::n_string AssocFileIcon; //0x015C
    std::n_string AssocFile1; //0x0178
    std::n_string AssocFile2; //0x0194
    undefined pad_01B0[16]; //0x01B0

private:
    BEGIN_FIXTURE()
        ENSURE_OFFSET(m_typeId, 0x0000)
        ENSURE_OFFSET(RefObjectId, 0x0004)
        ENSURE_OFFSET(CodeName, 0x0008)
        ENSURE_OFFSET(ObjName, 0x0024)
        ENSURE_OFFSET(OrgObjCodeName, 0x0040)
        ENSURE_OFFSET(NameStrID, 0x0060)
        ENSURE_OFFSET(DescStrID, 0x007c)
        ENSURE_OFFSET(DecayTime, 0x0098)
        ENSURE_OFFSET(Country, 0x009C)
        ENSURE_OFFSET(Rarity, 0x00A0)
        ENSURE_OFFSET(CanTrade, 0x00A4)
        ENSURE_OFFSET(CanSell, 0x00A5)
        ENSURE_OFFSET(CanBuy, 0x00A6)
        ENSURE_OFFSET(CanBorrow, 0x00A7)
        ENSURE_OFFSET(CanDrop, 0x00A8)
        ENSURE_OFFSET(CanPick, 0x00A9)
        ENSURE_OFFSET(CanRepair, 0x00AA)
        ENSURE_OFFSET(CanRevive, 0x00AB)
        ENSURE_OFFSET(CanUse, 0x00AC)
        ENSURE_OFFSET(CanThrow, 0x00AD)
        ENSURE_OFFSET(Price, 0x00B0)
        ENSURE_OFFSET(SellPrice, 0x00B8)
        ENSURE_OFFSET(CostRepair, 0x00C0)
        ENSURE_OFFSET(CostRevive, 0x00C4)
        ENSURE_OFFSET(CostBorrow, 0x00C8)
        ENSURE_OFFSET(KeepingFee, 0x00CC)
        ENSURE_OFFSET(ReqLevelType1, 0x00D0)
        ENSURE_OFFSET(ReqLevelType2, 0x00D4)
        ENSURE_OFFSET(ReqLevelType3, 0x00D8)
        ENSURE_OFFSET(ReqLevelType4, 0x00DC)
        ENSURE_OFFSET(ReqLevel1, 0x00E0)
        ENSURE_OFFSET(ReqLevel2, 0x00E4)
        ENSURE_OFFSET(ReqLevel3, 0x00E8)
        ENSURE_OFFSET(ReqLevel4, 0x00EC)
        ENSURE_OFFSET(MaxContain, 0x00F0)
        ENSURE_OFFSET(RegionId, 0x00F4)
        ENSURE_OFFSET(Dir, 0x00F8)
        ENSURE_OFFSET(OffsetX, 0x00FC)
        ENSURE_OFFSET(OffsetY, 0x0100)
        ENSURE_OFFSET(OffsetZ, 0x0104)
        ENSURE_OFFSET(Speed1, 0x0108)
        ENSURE_OFFSET(Speed2, 0x010C)
        ENSURE_OFFSET(Scale, 0x0110)
        ENSURE_OFFSET(BCHeight, 0x0114)
        ENSURE_OFFSET(BCRadius, 0x0118)
        ENSURE_OFFSET(AssocFileObj, 0x0124)
        ENSURE_OFFSET(AssocFileDrop, 0x0140)
        ENSURE_OFFSET(AssocFileIcon, 0x015c)
        ENSURE_OFFSET(AssocFile1, 0x0178)
        ENSURE_OFFSET(AssocFile2, 0x0194)

    END_FIXTURE()

    RUN_FIXTURE(SCommonData)
};
