//
// Created by YUMBUL on 24.12.2024.
//

#include <BSLib/Debug.h>
#include <Menu/IFMenu.h>
#include <CustomData/CustomDataManager.h>
#include <ExtraUI/IFItemTranslationWnd.h>
#include "IFNPCWindow.h"
#include "GInterface.h"


GFX_IMPLEMENT_DYNAMIC_EXISTING(CIF_NPCWindow, 0x00eeb1bc)

void CIF_NPCWindow::WriteNpcInfo_Maybe()
{
    reinterpret_cast<void (__thiscall *)(CIF_NPCWindow *)>(0x00700440)(this);
}
undefined CIF_NPCWindow::OnCloseWndIMPL(){

    undefined aa = reinterpret_cast<undefined(__thiscall *)(CIF_NPCWindow *)>(0x007040b0)(this);

        if(g_pCGInterface->m_IRM.GetResObj<CIFItemTranslationWnd>(1361, 1)->IsVisible())
        {
            g_pCGInterface->m_IRM.GetResObj<CIFItemTranslationWnd>(1361, 1)->OnCloseWnd();
        }

    return aa;
}

int CIF_NPCWindow::GetNpcObjID(){
    int aa = reinterpret_cast<int(__thiscall *)(CIF_NPCWindow *)>(0x007040a0)(this);

    return aa;
}

void CIF_NPCWindow::ShowGWndIMPL(bool bVisible)
{
    if(!bVisible)
    {
        if(g_pCGInterface->m_IRM.GetResObj<CIFItemTranslationWnd>(1361, 1)->IsVisible())
        {
            g_pCGInterface->m_IRM.GetResObj<CIFItemTranslationWnd>(1361, 1)->OnCloseWnd();
        }
    }
    reinterpret_cast<void(__thiscall *)(CIF_NPCWindow *, bool)>(0x00704100)(this, bVisible);

}
