//
// Created by YUMBUL on 26.06.2024.
//

#include <Macro/IFMacroMenu.h>
#include <CustomData/CustomCICPlayer.h>
#include "DropItemManager.h"
#include "GInterface.h"
#include "IItem.h"
#include "ICPlayer.h"




int CDropItemManager::sub_8AF7F0(char a2,unsigned __int16 PetRegionID,int PetPosition,DWORD a5)
{
    CIFMacroMenu* macroMenu = g_pCGInterface ? g_pCGInterface->m_IRM.GetResObj<CIFMacroMenu>(MacroMenuID, 1) : NULL;
    if (macroMenu &&
        macroMenu->PickupFilterSlot &&
        macroMenu->PickupFilterSlot->PickViaPetCheckBox &&
        macroMenu->PickupFilterSlot->Macro_PetFilter &&
        macroMenu->PickupFilterSlot->PickViaPetCheckBox->GetCheckedState_MAYBE())
    {
        return 0;
    }
    else
    {
        return reinterpret_cast<bool(__thiscall*)(CDropItemManager*, char,unsigned __int16,int,DWORD)>(0x008AF7F0)(this, a2,PetRegionID,PetPosition,a5);
    }
}
