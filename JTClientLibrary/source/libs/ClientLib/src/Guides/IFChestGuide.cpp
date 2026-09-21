#include <GInterface.h>
#include <GEffSoundBody.h>
#include <TextStringManager.h>
#include <iostream>
#include <Game.h>
#include "IFChestGuide.h"
#include "IFChest.h"

GFX_IMPLEMENT_DYNCREATE(CIFChestGuide, CIFDecoratedStatic)


bool CIFChestGuide::OnCreate(long ln)
{
    CIFDecoratedStatic::OnCreate(ln);

    TB_Func_13("clientlibrary\\guides\\chest1.ddj", 0, 0);
    sub_634470("clientlibrary\\guides\\chest2.ddj");

    // TB_Func_13("interface\\vlib\\location_1.ddj", 0, 0);
    //  sub_634470("interface\\vlib\\location_2.ddj");

    set_N00009BD4(2);
    set_N00009BD3(500);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifsimple.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    RECT rect = { 0,0,40,40 };
    namelabel = this->m_IRM.GetResObj<CIFStatic>(1, 0);
    std::n_wstring msg(KmtGetText(L"UIIT_KMT_ITEM_CHEST"));
    namelabel->SetTooltip(msg);
    namelabel->SetStyleThingy(TOOLTIP);

    return true;
}
int CIFChestGuide::OnMouseLeftUp(int a1, int x, int y)
{
    if (!g_pCGInterface)
        return 0;

    CIFChest* chest = g_pCGInterface->m_IRM.GetResObj<CIFChest>(ChestID, 1);
    if (!chest)
        return 0;

    if(chest->IsVisible())
    {
        chest->OnCloseWnd();
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else
    {
        CMsgStreamBuffer buf(0x169A);
        buf << byte(0x5);
        SendMsg(buf);
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
        chest->ActivateTabPage(0);
        chest->UpdateMenuSize();
        chest->ShowGWnd(true);


    }
    return 0;
}

void CIFChestGuide::OnCIFReady()
{
    CIFDecoratedStatic::OnCIFReady();
    sub_633990();
}

void CIFChestGuide::OnUpdate()
{

}

