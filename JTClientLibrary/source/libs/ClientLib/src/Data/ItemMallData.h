#pragma once
#include <BSLib/BSLib.h>
#include "SOItemPackage.h"
class CItemMallData
{
public:
    char pad_0000[4]; //0x0000
    int m_nShopId; //0x0004
    byte m_btTabIndex; //0x0008
    byte m_btItemIndex; //0x0009
    byte m_btUnknown0A; //0x000A
    byte m_btUnknown0B; //0x000B
    char pad_000C[4]; //0x000C
    CSOItemPackage *m_pSOPackage; //0x0010
    int m_nUnknown14; //0x0014
    char pad_0018[4]; //0x0018
    byte N00001207; //0x001C
    byte N0000121B; //0x001D
    byte N00001224; //0x001E
    byte N0000121C; //0x001F
    byte N00001208; //0x0020
    byte N0000121E; //0x0021
    byte N00001227; //0x0022
    byte N0000121F; //0x0023
    byte N00001209; //0x0024
    byte N00001221; //0x0025
    byte N0000122A; //0x0026
    byte N00001222; //0x0027
    char pad_0028[8]; //0x0028

};
