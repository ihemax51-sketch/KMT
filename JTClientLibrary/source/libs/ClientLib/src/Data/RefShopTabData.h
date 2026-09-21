#pragma once

#include <string>
#include "SOItemPackage.h"

#pragma pack(push, 4)
class CRefShopTabData
{
    virtual void Unknown();
public:
    unsigned __int32 m_country;
    unsigned __int32 m_id;
    std::wstring m_codeName128;
    unsigned __int32 m_tabGroupLinkId;
    std::wstring m_strId128Tab;
    std::vector<CSOItemPackage*> m_vPackageItems;
};
#pragma pack(pop)