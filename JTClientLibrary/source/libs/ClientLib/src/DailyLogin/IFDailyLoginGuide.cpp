#include <GInterface.h>
#include <GEffSoundBody.h>
#include <TextStringManager.h>
#include <DailyLogin/IFDailyLogin.h>
#include "IFDailyLoginGuide.h"

GFX_IMPLEMENT_DYNCREATE(CIFDailyLoginGuide, CIFDecoratedStatic)


bool CIFDailyLoginGuide::OnCreate(long ln)
{
    CIFDecoratedStatic::OnCreate(ln);

    TB_Func_13("clientlibrary\\guides\\daily_login1.ddj", 0, 0);
    sub_634470("clientlibrary\\guides\\daily_login2.ddj");

    // TB_Func_13("interface\\vlib\\location_1.ddj", 0, 0);
    //  sub_634470("interface\\vlib\\location_2.ddj");

    set_N00009BD4(2);
    set_N00009BD3(500);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifsimple.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    RECT rect = { 0,0,40,40 };
    namelabel = this->m_IRM.GetResObj<CIFStatic>(1, 0);
    std::n_wstring msg(KmtGetText(L"UIIT_KMT_ATTENDANCE"));
    namelabel->SetTooltip(msg);
    namelabel->SetStyleThingy(TOOLTIP);


    return true;
}

int CIFDailyLoginGuide::OnMouseLeftUp(int a1, int x, int y)
{

    if(g_pCGInterface->m_IRM.GetResObj<CIFDailyLogin>(DailyLoginID, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFDailyLogin>(DailyLoginID, 1)->ShowGWnd(false);
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else
    {
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
        g_pCGInterface->m_IRM.GetResObj<CIFDailyLogin>(DailyLoginID, 1)->ActivateTabPage(0);
        g_pCGInterface->m_IRM.GetResObj<CIFDailyLogin>(DailyLoginID, 1)->UpdateMenuSize();
        g_pCGInterface->m_IRM.GetResObj<CIFDailyLogin>(DailyLoginID, 1)->ShowGWnd(true);

        CMsgStreamBuffer buf(0x169A);
        buf << BYTE(13);
        SendMsg(buf);

    }
    return 0;
}

void CIFDailyLoginGuide::OnCIFReady()
{
    CIFDecoratedStatic::OnCIFReady();
    sub_633990();
}

void CIFDailyLoginGuide::OnUpdate()
{

}

