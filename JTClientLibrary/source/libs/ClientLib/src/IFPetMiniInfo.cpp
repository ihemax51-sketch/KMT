//
// Created by Admin on 19/4/2022.
//

#include "IFPetMiniInfo.h"
#include "GlobalDataManager.h"
#include "ICPlayer.h"
#include "ICCos.h"
#include "unsorted.h"
#include <SimpleViewer/VBDynamic.h>
#include <sstream>
#include <ctime>
#include <IFCheckBox.h>
#include <Game.h>
#include <CustomData/CustomDataManager.h>
GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFPetMiniInfo, 0x00EEC228)


bool CIFPetMiniInfo::OnCreateIMPL(long ln)
{


    bool b = reinterpret_cast<bool(__thiscall*)(CIFPetMiniInfo*, long)>(0x007b4490)(this, ln);

    return  b;
}

void CIFPetMiniInfo::OnUpdateIMPL() {


    reinterpret_cast<void(__thiscall*)(const CIFPetMiniInfo*)>(0x007b41f0)(this);


    CICCos* pUser = static_cast<CICCos*>(GetCharacterObjectByID_MAYBE(this->m_petUniqueID));
    if (pUser == NULL) {
        return;
    }
    if(this->m_petHGP != NULL)
    {
        if(m_CustomDataManager->m_RefFellowPetSystem.find(pUser->GetCommonData()->NameStrID) != m_CustomDataManager->m_RefFellowPetSystem.end())
        {
            if(this->m_petHGP->m_valueFg <= 0.30)
            {
                this->m_petHGP->TB_Func_13("clientlibrary\\fellowpets\\pt_hgp_bad.ddj", 1, 1);
            }
            else if(this->m_petHGP->m_valueFg > 0.30 && this->m_petHGP->m_valueFg <= 0.80)
            {
                this->m_petHGP->TB_Func_13("clientlibrary\\fellowpets\\pt_mp.ddj", 1, 1);

            }
            else if(this->m_petHGP->m_valueFg > 0.80)
            {
                this->m_petHGP->TB_Func_13("clientlibrary\\fellowpets\\pt_hgp_good.ddj", 1, 1);

            }
        }
        else
        {
            this->m_petHGP->TB_Func_13("interface\\playerminiinfo\\pmi_pet_hgp.ddj", 1, 1);
        }
    }

}
