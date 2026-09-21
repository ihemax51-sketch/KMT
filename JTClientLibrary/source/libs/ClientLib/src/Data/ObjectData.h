#pragma once
#include <Windows.h>
#include <BSLib/_internal/custom_stl.h>
class ObjectData
{
public:
    TypeId m_typeId; //0x0000
    char pad_0002[2]; //0x0002
    UINT ObjID; //0x0004
    std::n_wstring ObjCodeName; //0x0008
    std::n_wstring ObjName; //0x0024
    std::n_wstring ObjOrgCodeName; //0x0040
    char pad_0064[4]; //0x005C
    std::wstring ObjNameStrID; //0x0060
    std::wstring DescStrID; //0x007C
    UINT DecayTime; //0x0098
    BYTE Country; //0x009C
    BYTE UNK_009D; //0x009D | Double country? WTF?
    char pad_009E[2]; //0x009E
    UINT Rarity; //0x00A0
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
    long long Price; //0x00B0
    long long SellPrice; //0x00B8
    UINT CostRepair; //0x00C0
    UINT CostRevive; //0x00C4
    UINT CostBorrow; //0x00C8
    UINT KeepingFee; //0x00CC
    int ReqLevelType1; //0x00D0
    int ReqLevelType2; //0x00D4
    int ReqLevelType3; //0x00D8
    int ReqLevelType4; //0x00DC
    int ReqLevel1; //0x00E0
    int ReqLevel2; //0x00E4
    int ReqLevel3; //0x00E8
    int ReqLevel4; //0x00EC
    int MaxContain; //0x00F0
    UINT RegionID; //0x00F4
    int Direction; //0x098
    int OffsetX; //0x09C
    int OffsetY; //0x0100
    int OffsetZ; //0x0104
    UINT Speed1; //0x0108
    UINT Speed2; //0x010C

    int Scale; //0x0110
    int BCHeight; //0x0114
    int BCRadius; //0x0118
    UINT EventID; //0x011C

    UINT ObjItemLinkID; //0x0120
    std::string AssocFileObj; //0x0124
    std::string AssocFileDrop; //0x0140
    std::string AssocFileIcon; //0x015C
    std::string AssocFile1; //0x0178
    std::string AssocFile2; //0x0194
};

class CharData : public ObjectData
{
public:

};