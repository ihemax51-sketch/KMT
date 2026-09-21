//
// Created by Admin on 18/12/2021.
//

#include "IFPartyMatch.h"
#include "IFStatic.h"
#include "TextStringManager.h"
#include "GInterface.h"
#include "Game.h"
#include "GlobalDataManager.h"
#include <ctime>
#include <GFXMainFrame/Controler.h>
#include <CustomInterface/IFPartyMemberViewer.h>
#include <CustomData/CustomSettingManager.h>

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFPartyMatch, 0x00EEB8CC)

void CIFPartyMatch::FUN_00740750(CIFHelperBubbleWindow* window)
{

    reinterpret_cast<void (__thiscall *)(CIFPartyMatch *, CIFHelperBubbleWindow *)>(0x00740750)(this, window);

}

void CIFPartyMatch::FUN_00740870(char param_1)
{
    if(m_Settings->ENABLE_NEW_PT_MATCH)
    {
        CIFStatic* Race = this->m_IRM.GetResObj<CIFStatic>(12, 1);
        if(param_1 == 0)
        {
            Race->SetGWndSize(16, 16);
            Race->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_china16.ddj", 0,0);
        }
        else
        {
            if (param_1 != '\x01')
                return;

            Race->SetGWndSize(16, 16);
            Race->TB_Func_13("interface\\ifcommon\\ifcommon\\com_kindred_europe16.ddj", 0,0);
        }
        return;
    }
    else
    {
        reinterpret_cast<void(__thiscall*)(CIFPartyMatch*,BYTE)>(0x00740870)(this, param_1);
    }


}
bool CIFPartyMatch::OnCreateIMPL(long ln) {

    bool b = reinterpret_cast<bool (__thiscall *)(CIFPartyMatch *, long)>(0x0073ae90)(this, ln);
    //CIFStatic* PartyIconType = this->m_IRM.GetResObj<CIFStatic>(11, 1);
    //wnd_pos p = PartyIconType->GetPos();
    //PartyIconType->MoveGWnd(p.x, p.y - 2);
    //PartyIconType->SetGWndSize(18, 18);

    return b;
}
void CIFPartyMatch::FUN_00740d30(int p1, int p2)
{
    //printf("%d %d \n", p1, p2);
    reinterpret_cast<void(__thiscall*)(CIFPartyMatch*,int, int)>(0x00740d30)(this, p1, p2);
    if(m_Settings->ENABLE_NEW_PT_MATCH)
    {
        if(p1 == 0 && p2 == 0)
        {
            CIFStatic* Race = this->m_IRM.GetResObj<CIFStatic>(12, 1);
            Race->TB_Func_13("", 0,0);
            CIFStatic* PartyIconType = this->m_IRM.GetResObj<CIFStatic>(15, 1);
            PartyIconType->TB_Func_13("", 0, 0);
        }
    }

}
void CIFPartyMatch::FUN_00740910(BYTE PatyType)
{
    reinterpret_cast<void(__thiscall*)(CIFPartyMatch*,BYTE)>(0x00740910)(this, PatyType);
    if(m_Settings->ENABLE_NEW_PT_MATCH)
    {
        CIFStatic* PartyIconType = this->m_IRM.GetResObj<CIFStatic>(15, 1);
        PartyIconType->SetText(L"");
        switch(PatyType) {
        case 0: //hunting
        {
            PartyIconType->TB_Func_13("interface\\ifcommon\\com_frpvp_voucher_c.ddj", 0, 0);
            PartyIconType->SetGWndSize(16, 16);
        }

            break;
        case 1: //quest
        {
            PartyIconType->TB_Func_13("interface\\ifcommon\\com_frpvp_voucher_c.ddj", 0, 0);
            PartyIconType->SetGWndSize(16, 16);
        }

            break;
        case 2: //trader
        {
            PartyIconType->SetGWndSize(16, 16);
            PartyIconType->TB_Func_13("interface\\ifcommon\\com_job_merchant.ddj", 0, 0);
        }

            break;
        case 3: //thief
        {
            PartyIconType->SetGWndSize(16, 16);
            PartyIconType->TB_Func_13("interface\\ifcommon\\com_job_thief.ddj", 0, 0);
        }
            break;

    }
    }
}


undefined1 CIFPartyMatch::OnCloseWndIMPL() {
    g_pCGInterface->m_IRM.GetResObj<CIFPartyMemberViewer>(1362, 1)->ShowGWnd(false);
    return reinterpret_cast<undefined1(__thiscall*)(CIFPartyMatch*)>(0x00652c00)(this);
}
void CIFPartyMatch::OnUpdateIMPL()
{
/*    for(std::list<CIFPartyMatchSlot*>::iterator it = m_SlotList.begin(); it != m_SlotList.end(); it++)
    {
        printf("%ls \n", (*it)->m_MasterOfParty.c_str());
    }*/
    if(m_Settings->EnablePtMemberViewer)
    {
        if (g_CurrentIfUnderCursor->IsSame(GFX_RUNTIME_CLASS(CIFStatic)))
        {
            CIFStatic* CurSlot = (CIFStatic*)g_CurrentIfUnderCursor;
            short ALTState = GetKeyState(VK_CONTROL);
            short MouseState = GetKeyState(VK_LBUTTON);

            if (CurSlot->GetNText().empty())
            {
                g_pCGInterface->GetGuiFromList<CIFPartyMemberViewer>(PartyMemberViewer)->ShowGWnd(false);
            }

        }
        else
        {
            g_pCGInterface->GetGuiFromList<CIFPartyMemberViewer>(PartyMemberViewer)->ShowGWnd(false);
        }
    }

    reinterpret_cast<void(__thiscall*)(CIFPartyMatch*)>(0x006528a0)(this);
}
int CIFPartyMatch::OnMouseLeftUpIMPL(int a1, int x, int y)
{
    int a =  reinterpret_cast<bool (__thiscall *)(CIFWnd *, int,int,int)>(0x0046fd30)(this,a1,x,y);
    return  a;
}
