#include "PSVersionCheck.h"
#include "Game.h"
#include "IFGauge.h"
#include <BSLib/Debug.h>
#include <BSLib/multibyte.h>
#include <GFXMainFrame/Controler.h>

#define g_unknown (*(int*)((int)0x00eb6a48))

bool CPSVersionCheck::OnCreateIMPL(long ln)
{
    PutDump("CPSVersionCheck::OnCreate (%d)\n", ln);

    if(StartNetEngine())
    {
   /*     if(!g_pCFileManager->FileExists("jmxdebug._m_",0))
            Fun_GetCfgGame()->m_bDebugMode = 1;
*/
        long hWndState = GetWindowLongA(g_CGame->GetHWnd(), GWL_STYLE);

        static int hWndwidth = 400;
        static int hWndheight = 148;

        RECT hWndRect = {0,0,hWndwidth,hWndheight};
        AdjustWindowRect(&hWndRect,hWndState,0);

        HWND hDesktopWnd = GetDesktopWindow();
        RECT hDesktopWndRECT;
        GetClientRect(hDesktopWnd, &hDesktopWndRECT);


        hDesktopWndRECT.left = (hDesktopWndRECT.right - hWndwidth) / 2;
        hDesktopWndRECT.top = (hDesktopWndRECT.bottom - hWndheight) / 2;

        SetWindowPos(g_CGame->GetHWnd(),(HWND)0xFFFFFFFE,hDesktopWndRECT.left,hDesktopWndRECT.top ,hWndwidth,hWndheight ,SWP_SHOWWINDOW);
        ShowWindow(g_CGame->GetHWnd(),1);
        UpdateWindow(g_CGame->GetHWnd());

        RECT m_mainWndRect = {0,0,1024,768};
      //  RECT m_mainWndRect = {0,0,400,148};
      CIFWnd* m_mainWnd = (CIFWnd*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFWnd),m_mainWndRect,1,0);



        wchar_t buffer[256];
        std::n_wstring m_ddjImg;

        int m_ddjId = rand() % ((10 - 1) + 1) + 1;
        swprintf(buffer, L"interface\\loading\\start_loading_0%d.ddj", m_ddjId);
        m_ddjImg.assign(buffer);

        m_mainWnd->TB_Func_13(TO_STRING(m_ddjImg).c_str(),0,0);




      /*  wnd_rect sz;
        sz.pos.x = 0;
        sz.pos.y = 480;
        sz.size.width = 400;
        sz.size.height = 80;
        CIFWnd* BarWnd = (CIFWnd*)CreateInstance(this, GFX_RUNTIME_CLASS(CIFWnd),sz,2,0);
        BarWnd->TB_Func_13("clientlibrary\\title\\title_gage.ddj", 1, 1);
        int barwidht = (400 * 1) / 100;
        BarWnd->SetGWndSize(barwidht, 80);
        BarWnd->ShowGWnd(false);
*/
       // printf("%p \n", this);
       // this->StartTimer(10000, 100);
        g_unknown = 0;
        return true;
    }
    else
    {
        CGFXMainFrame::SetNextProcessSTAT(reinterpret_cast<CGfxRuntimeClass*>(0x0EED894));
        return false;
    }

    //int result = reinterpret_cast<char (__thiscall *)(CPSVersionCheck *,int)>(0x0086D370)(this,a2);
}
void CPSVersionCheck::OnTimerIMPL(int TimerId)
{
    if(TimerId == 10000)
    {
        if(this->GetGuiFromList<CIFWnd>(2) != NULL)
        {
            int barwidht = (400 * (this->m_iNumberOfUpdates * 20)) / 100;
            this->GetGuiFromList<CIFWnd>(2)->SetGWndSize(barwidht, 80);
        }
    }
    reinterpret_cast<void(__thiscall *)(CPSVersionCheck *, int)>(0x00860c60)(this, TimerId);
}
void CPSVersionCheck::OnUpdateIMPL()
{
    reinterpret_cast<void(__thiscall *)(CPSVersionCheck *)>(0x0086d220)(this);
}