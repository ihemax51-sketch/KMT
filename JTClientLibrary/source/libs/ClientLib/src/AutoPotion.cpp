#include "AutoPotion.h"

#include "BSLib/Debug.h"
#include "GInterface.h"
#include "ICPlayer.h"
#include "SRIFLib/NIFUnderMenuBar.h"
#include "SRIFLib/NInterfaceResource.h"


void CAutoPotion::UsePotion(unsigned char byType) {
    BS_ASSERT(byType < AUTO_POTION_COUNT);

    if (this->m_data_in_array[byType].InitState == InitComplete) {
        CNIFUnderMenuBar *pMenuBar = g_sNewInterfaceMgr.GetInterfaceObj<CNIFUnderMenuBar>(0xaf);

        if (pMenuBar->IsPotionOrPillInQuickslot(m_data_in_array[byType].BeltAndSlotCombined)) {
            pMenuBar->UseSlot(m_data_in_array[byType].BeltAndSlotCombined);
        }
    }
}


int CAutoPotion::GetPotionDelay() const {
    return m_nUseDelay;
}
