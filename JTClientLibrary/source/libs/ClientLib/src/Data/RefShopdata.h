#pragma once

#include "RefShopTabGroupData.h"
#include <string>

#pragma pack(push, 4)
class CRefShopdata
{
public:
    virtual void Unknown();

    unsigned __int32 m_country;
    unsigned __int32 m_id;
    std::wstring m_codeName128;
    std::vector<CRefShopTabGroupData*> m_vRefShopTabGroupData;
};
#pragma pack(pop)