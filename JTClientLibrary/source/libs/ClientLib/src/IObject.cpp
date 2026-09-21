#include "IObject.h"


uregion CIObject::GetRegion() const {
    return region;
}

D3DVECTOR CIObject::GetLocation() const {
    return location;
}
CRTNavCellQuad *CIObject::GetNavCell() const {
    return m_navcell;
}
