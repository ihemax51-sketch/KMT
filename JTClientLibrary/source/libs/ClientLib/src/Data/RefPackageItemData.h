#pragma once

#include "BSLib/_internal/custom_stl.h"
#include "RefScrapOfPackageItemData.h"

#pragma pack(push,4)
class CRefPackageItemData
{
public:
    virtual void Unknown();

    char gap[4];
    unsigned __int32 m_id;
    char gap1[4];
    std::n_wstring m_codeName128;
    unsigned __int16 m_saleTag;
    std::wstring m_expandTerm;
    std::wstring m_nameStrId;
    std::wstring m_descStrId;
    std::string m_assocFileIcon;
    char pad_00A0[28];
    std::vector<CRefScrapOfPackageItemData *> m_vScrapData; //0x00BC
    char pad_00CC[0x14];
    int m_nSilkPrice;
    //std::string m_empty; maybe something
};
#pragma pack(pop)