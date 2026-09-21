#include <GInterface.h>
#include <GEffSoundBody.h>
#include <TextStringManager.h>
#include <iostream>
#include <Game.h>
#include <Menu/IFMenu.h>
#include "IFWebGuide.h"
#include "IFWeb.h"


GFX_IMPLEMENT_DYNCREATE(CIFWebGuide, CIFDecoratedStatic)


bool CIFWebGuide::OnCreate(long ln)
{
    CIFDecoratedStatic::OnCreate(ln);

    TB_Func_13("icon\\etc\\location_1.ddj", 0, 0);
    sub_634470("icon\\etc\\location_2.ddj");

    // TB_Func_13("interface\\vlib\\location_1.ddj", 0, 0);
    //  sub_634470("interface\\vlib\\location_2.ddj");

    set_N00009BD4(2);
    set_N00009BD3(500);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifsimple.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    RECT rect = { 0,0,40,40 };
    namelabel = this->m_IRM.GetResObj<CIFStatic>(1, 0);
    std::n_wstring msg(KmtGetText(L"UIIT_KMT_MAP_LOCATION"));
    namelabel->SetTooltip(msg);
    namelabel->SetStyleThingy(TOOLTIP);

    return true;
}
int CIFWebGuide::OnMouseLeftUp(int a1, int x, int y)
{
    CIFMenu *menu = g_pCGInterface->m_IRM.GetResObj<CIFMenu>(MainMenuID, 1);
    if(menu->PingedRegionID != 0)
    {
        if(!g_pCGInterface->GetCNIFWorldMap()->IsVisible())
        {
            g_pCGInterface->GetCNIFWorldMap()->ShowGWnd(true);
            g_pCGInterface->GetCNIFWorldMap()->BringToFront();
        }

            if(menu->MapType == 1 || menu->MapType == 0)
            {
                int local_34 = (unsigned int)(menu->PingedRegionID >> 8);
                int uVar10 = menu->PingedRegionID & 0xff;
                int PosX = menu->PingedPosX;
                int PosZ = menu->PingedPosZ;
                g_pCGInterface->GetCNIFWorldMap()->field_229ad = 1;
                g_pCGInterface->GetCNIFWorldMap()->Fun_0061a620();
                g_pCGInterface->GetCNIFWorldMap()->FUN_006256e0(0);

                g_pCGInterface->GetCNIFWorldMap()->Fun_00622f00(1073741824, 6881382, 7077989, 100, (undefined4)g_pCGInterface->GetCNIFWorldMap(), 5, 7, uVar10, local_34, PosX, PosZ);

            }
            else
            {
                int local_34 = (unsigned int)(menu->PingedRegionID >> 8);
                int uVar10 = menu->PingedRegionID & 0xff;
                int PosX = menu->PingedPosX;
                int PosZ = menu->PingedPosZ;
                g_pCGInterface->GetCNIFWorldMap()->field_229ad = 1;
                g_pCGInterface->GetCNIFWorldMap()->Fun_0061a620();
              //  g_pCGInterface->GetCNIFWorldMap()->FUN_006256e0(0);
                g_pCGInterface->GetCNIFWorldMap()->FUN_006217e0(menu->MapIndex);
             //   g_pCGInterface->GetCNIFWorldMap()->Fun_00622f00(1073741824, 6881382, 7077989, 100, (undefined4)g_pCGInterface->GetCNIFWorldMap(), 5, 7, uVar10, local_34, PosX, PosZ);

            }


        if(g_pCGInterface->GetAlarmManager()->GetGuide(GDR_WEB_GUIDE) != NULL)
        {
            g_pCGInterface->GetAlarmManager()->RemoveGuide((GDR_WEB_GUIDE));
        }
    }

  /*  if(g_pCGInterface->m_IRM.GetResObj<CIFWeb>(CUSTOMWEBGUI, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFWeb>(CUSTOMWEBGUI, 1)->OnCloseWnd();
        CGEffSoundBody::get()->PlaySound(L"snd_window_close");
    }
    else
    {
        CGEffSoundBody::get()->PlaySound(L"snd_window_open");
        g_pCGInterface->m_IRM.GetResObj<CIFWeb>(CUSTOMWEBGUI, 1)->UpdateMenuSize();
        g_pCGInterface->m_IRM.GetResObj<CIFWeb>(CUSTOMWEBGUI, 1)->ShowGWnd(true);


    }*/
    return 0;
}

void CIFWebGuide::OnCIFReady()
{
    CIFDecoratedStatic::OnCIFReady();
    sub_633990();
}

void CIFWebGuide::OnUpdate()
{

}

