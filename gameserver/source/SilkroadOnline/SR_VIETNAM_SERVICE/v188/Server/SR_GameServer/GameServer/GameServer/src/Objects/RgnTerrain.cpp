#include "RgnTerrain.h"

bool CRgnTerrain::GetZoneState() const {
    return m_sRegionData->m_bIsBattleField;
}

WORD CRgnTerrain::GetRegionID() const {
    return m_sRegionData->m_wRegionId;
}

std::string CRgnTerrain::GetAreaName() const {
    return m_sRegionData->m_strAreaName;
}

std::string CRgnTerrain::GetContinentName() const {
    return m_sRegionData->m_strContinentName;
}