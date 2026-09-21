#pragma once
#include "BSLib/_internal/custom_stl.h"
#include "RefShopdata.h"


class CItemMallGroupData {
public:
    char pad_0000[8];
    std::n_wstring m_wStrGroupName; //0x0008
    char pad_0024[6]; //0x0024
    std::vector<CRefShopdata*> m_vShopData;
};