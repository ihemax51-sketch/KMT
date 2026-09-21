#pragma once

#include <GSLog/MsgCustom.h>
#include <Base.h>


#pragma pack(push, 1)

struct SRegionDetails
{
    int m_nClimate;

};

struct sRegionData
{
    WORD m_wRegionId; // 0x0000
    bool m_bIsBattleField; // 0x0002
    char pad1[1];

    SRegionDetails* m_pRegionDetails;

    DWORD m_dwMaxCapacity;

    DWORD m_dwAssocObjID;

    DWORD m_dwAssocServer;

    std::string m_strAssocFile256;

    std::string m_strAreaName; // 0x48

    std::string m_strContinentName;

};

struct SRegionData {
    WORD m_wRegionId; // 0x0000
    bool m_bIsBattleField; // 0x0002
    char pad1[1];

    SRegionDetails* m_pRegionDetails;

    DWORD m_dwMaxCapacity;

    DWORD m_dwAssocObjID;

    DWORD m_dwAssocServer;

    std::string m_strAssocFile256;

    std::string m_strAreaName; // 0x48

    std::string m_strContinentName;

//    sRegionData* m_LinkedRegionSData[10];


};
#pragma pack(pop)
class CRegion : public CBase {

};

class CRgnTerrain  : public CRegion {

public:
    CRgnTerrain();
    virtual ~CRgnTerrain();

    int xxx1;

    WORD m_wAssocServerID;
    WORD m_wRegionRELATED; // check me later

    WORD m_wAssocRelated;
    WORD m_wRegionAssocRelated;

    SRegionData* m_sRegionData; // 0x0010

    WORD GetRegionID() const;
    bool GetZoneState() const;
    std::string GetAreaName() const;
    std::string GetContinentName() const;

};
