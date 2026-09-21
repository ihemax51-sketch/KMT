//
// Created by Admin on 16/2/2022.
//


#include "IFPartyMatchSlot.h"
#include "IFStatic.h"
#include <Game.h>
#include <GInterface.h>
#include <CustomInterface/IFPartyMemberViewer.h>
#include <CustomData/CustomSettingManager.h>
#include <BSLib/multibyte.h>

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFPartyMatchSlot, 0x00EEB94C)
bool CIFPartyMatchSlot::OnCreateIMPL(long ln) {

    bool b = reinterpret_cast<bool (__thiscall *)(CIFPartyMatchSlot *, long)>(0x00741250)(this, ln);
    if(m_Settings->ENABLE_NEW_PT_MATCH)
    {
        CIFStatic *PartyIconType = this->m_IRM.GetResObj<CIFStatic>(12, 1);
        wnd_pos p = PartyIconType->GetPos();
        PartyIconType->MoveGWnd(p.x + 1, p.y - 2);


        CIFStatic *PartyIconTypxe = this->m_IRM.GetResObj<CIFStatic>(15, 1);
        wnd_pos as = PartyIconTypxe->GetPos();
        PartyIconTypxe->MoveGWnd(as.x + 7, as.y - 2);
    }

    return b;
}

void CIFPartyMatchSlot::OnUpdateIMPL()
{

    reinterpret_cast<void(__thiscall*)(CIFPartyMatchSlot*)>(0x006528a0)(this);
}
int CIFPartyMatchSlot::OnMouseRightUpIMPL(int a1, int x, int y)
{
    int a = reinterpret_cast<bool(__thiscall*)(CIFWnd*, int, int, int)>(0x0046fd30)(this, a1, x, y);
    if(m_Settings->EnablePtMemberViewer)
    {
        POINT mMouse;
        GetCursorPos(&mMouse);
        POINT m_show_point;
        RECT r1;
        GetWindowRect(theApp.GetHWnd(), &r1);
        m_show_point.x = mMouse.x - (r1.left);
        m_show_point.y = mMouse.y - r1.top;
        if ((wcscmp(GetPartyMatchSlotByID(16), L"1/4") == 0) || (wcscmp(GetPartyMatchSlotByID(16), L"1/8") == 0))
        {
            CMsgStreamBuffer buf(0x185A);
            byte type = 1;
            buf << type;
            std::string Master = TO_STRING(m_MasterOfParty);
            buf << Master;
            SendMsg(buf);
            g_pCGInterface->m_IRM.GetResObj<CIFPartyMemberViewer>(PartyMemberViewer, 1)->MoveGWnd(m_show_point.x + 30, m_show_point.y - 30);
            g_pCGInterface->m_IRM.GetResObj<CIFPartyMemberViewer>(PartyMemberViewer, 1)->ShowGWnd(true);
            g_pCGInterface->m_IRM.GetResObj<CIFPartyMemberViewer>(PartyMemberViewer, 1)->BringToFront();
        }
        else
        {
            CMsgStreamBuffer buf(0x185A);
            byte type = 0;
            buf << type;
            std::string Master = TO_STRING(m_MasterOfParty);
            buf << Master;
            SendMsg(buf);
           g_pCGInterface->m_IRM.GetResObj<CIFPartyMemberViewer>(PartyMemberViewer, 1)->MoveGWnd(m_show_point.x + 30, m_show_point.y - 30);
           g_pCGInterface->m_IRM.GetResObj<CIFPartyMemberViewer>(PartyMemberViewer, 1)->ShowGWnd(true);
           g_pCGInterface->m_IRM.GetResObj<CIFPartyMemberViewer>(PartyMemberViewer, 1)->BringToFront();
        }
    }
    return  a;
}

const wchar_t* CIFPartyMatchSlot::GetPartyMatchSlotByID(int ID)
{
    CIFStatic* PartyRaceType = this->m_IRM.GetResObj<CIFStatic>(ID, 1);
    const wchar_t *race2 = PartyRaceType->GetText();
    return race2;
}
