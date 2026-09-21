#include "IFCOSStatus.h"
#include "ICPlayer.h"
#include "ICCos.h"
#include "unsorted.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFCOSStatus, 0x00eec108)
void CIFCOSStatus::OnUpdateIMPL(){

    reinterpret_cast<void (__thiscall *)(const CIFCOSStatus *)>(0x006550c0)(this);
    CICCos* pUser = static_cast<CICCos*>(GetCharacterObjectByID_MAYBE(this->m_petUniqueID));
    if(pUser == NULL) {
        return;
    }

}
