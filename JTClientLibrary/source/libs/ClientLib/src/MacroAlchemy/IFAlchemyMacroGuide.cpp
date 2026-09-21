#include <GInterface.h>
#include <GEffSoundBody.h>
#include <TextStringManager.h>
#include <iostream>
#include <Game.h>
#include <Macro/IFMacro.h>
#include <SRIFLib/NIFEnchantWnd.h>
#include "IFAlchemyMacroGuide.h"
#include "IFAlchemyMacro.h"

GFX_IMPLEMENT_DYNCREATE(CIFAlchemyMacroGuide, CIFDecoratedStatic)


bool CIFAlchemyMacroGuide::OnCreate(long ln)
{
    CIFDecoratedStatic::OnCreate(ln);

    TB_Func_13("clientlibrary\\guides\\webgacha_1.ddj", 0, 0);
    sub_634470("clientlibrary\\guides\\webgacha_2.ddj");

    // TB_Func_13("interface\\vlib\\location_1.ddj", 0, 0);
    //  sub_634470("interface\\vlib\\location_2.ddj");

    set_N00009BD4(2);
    set_N00009BD3(500);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifsimple.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    RECT rect = { 0,0,40,40 };
    namelabel = this->m_IRM.GetResObj<CIFStatic>(1, 0);
    std::n_wstring msg(KmtGetText(L"UIIT_KMT_ALCHEMY_MACRO"));
    namelabel->SetTooltip(msg);
    namelabel->SetStyleThingy(TOOLTIP);

    return true;
}
int CIFAlchemyMacroGuide::OnMouseLeftUp(int a1, int x, int y)
{
    if(g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1)->OnCloseWnd();
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else
    {
        if(g_pCGInterface->GetGuiFromList<CNIFEnchantWnd>(168) != NULL)
        {
           return 0;
        }
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
        g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1)->ShowGWnd(true);
        g_pCGInterface->GetMainPopup()->ShowGWnd(true);
        g_pCGInterface->GetMainPopup()->ShowSubPage(GDR_INVENTORY);
        g_pCGInterface->m_IRM.GetResObj<CIFAlchemyMacro>(AlchemyMacro, 1)->UpdateMenuSize();
        g_pCGInterface->LockMovement(13);
    }

    return 0;
}

void CIFAlchemyMacroGuide::OnCIFReady()
{
    CIFDecoratedStatic::OnCIFReady();
    sub_633990();
}

void CIFAlchemyMacroGuide::OnUpdate()
{

}

