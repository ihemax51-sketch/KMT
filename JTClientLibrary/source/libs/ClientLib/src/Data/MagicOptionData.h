#pragma once

class CMagicOptionData
{
public:
    char pad_0000[4]; //0x0000
    int ID; //0x0004
    std::n_wstring MOptName128; //0x0008
    std::n_wstring AttrType; //0x0024
    int MLevel; //0x0040
    char pad_0044[4]; //0x0044
    int Weight; //0x0048
    int Param1; //0x004C
    int Param2; //0x0050
    int Param3; //0x0054
    int Param4; //0x0058
    int Param5; //0x005C
    int Param6; //0x0060
    int Param7; //0x0064
    int Param8; //0x0068
    int Param9; //0x006C
    int Param10; //0x0070
    int Param11; //0x0074
    int Param12; //0x0078
    int Param13; //0x007C
    int Param14; //0x0080
    int Param15; //0x0084
    int Param16; //0x0088
    int ExpFunc1; //0x008C
    int ExpFunc2; //0x0090
    int ExpFunc3; //0x0094
    int ExpFunc4; //0x0098
    int ExpFunc5; //0x009C
    int ExpFunc6; //0x00A0
    std::n_wstring AvailItemGroup1; //0x00A4
    std::n_wstring AvailItemGroup2; //0x00C0
    std::n_wstring AvailItemGroup3; //0x00DC
    std::n_wstring AvailItemGroup4; //0x00F8
    std::n_wstring AvailItemGroup5; //0x0114
    std::n_wstring AvailItemGroup6; //0x0130
    std::n_wstring AvailItemGroup7; //0x014C
    std::n_wstring AvailItemGroup8; //0x0168
    std::n_wstring AvailItemGroup9; //0x0184
    std::n_wstring AvailItemGroup10; //0x01A0
    int ReqClass1; //0x01BC
    int ReqClass2; //0x01C0
    int ReqClass3; //0x01C4
    int ReqClass4; //0x01C8
    int ReqClass5; //0x01CC
    int ReqClass6; //0x01D0
    int ReqClass7; //0x01D4
    int ReqClass8; //0x01D8
    int ReqClass9; //0x01DC
    int ReqClass10; //0x01E0

};