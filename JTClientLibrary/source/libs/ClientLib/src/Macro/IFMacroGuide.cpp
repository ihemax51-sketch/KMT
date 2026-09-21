#include <GInterface.h>
#include <GEffSoundBody.h>
#include <TextStringManager.h>
#include <iostream>
#include <Game.h>
#include <Macro/IFMacro.h>
#include "IFMacroGuide.h"

GFX_IMPLEMENT_DYNCREATE(CIFMacroGuide, CIFDecoratedStatic)


bool CIFMacroGuide::OnCreate(long ln)
{
    CIFDecoratedStatic::OnCreate(ln);

    TB_Func_13("clientlibrary\\guides\\macro_0.ddj", 0, 0);
    sub_634470("clientlibrary\\guides\\macro_0.ddj");

    // TB_Func_13("interface\\vlib\\location_1.ddj", 0, 0);
    //  sub_634470("interface\\vlib\\location_2.ddj");

    set_N00009BD4(2);
    set_N00009BD3(500);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifsimple.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    RECT rect = { 0,0,40,40 };
    namelabel = this->m_IRM.GetResObj<CIFStatic>(1, 0);
    std::n_wstring msg(KmtGetText(L"UIIT_KMT_SRO_MACRO"));
    namelabel->SetTooltip(msg);
    namelabel->SetStyleThingy(TOOLTIP);

    return true;
}
int CIFMacroGuide::OnMouseLeftUp(int a1, int x, int y)
{
    if(g_pCGInterface->m_IRM.GetResObj<CIFMacro>(MacroID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFMacro>(MacroID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else
    {
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
        g_pCGInterface->m_IRM.GetResObj<CIFMacro>(MacroID, 1)->ShowGWnd(true);
        g_pCGInterface->m_IRM.GetResObj<CIFMacro>(MacroID, 1)->UpdateMenuSize();
    }

    return 0;
}

void CIFMacroGuide::OnCIFReady()
{
    CIFDecoratedStatic::OnCIFReady();
    sub_633990();
}

void CIFMacroGuide::OnUpdate()
{

}

