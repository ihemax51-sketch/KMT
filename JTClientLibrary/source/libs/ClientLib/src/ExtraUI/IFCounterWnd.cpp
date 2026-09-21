#include <Game.h>
#include <ICPlayer.h>
#include <GInterface.h>
#include <TextStringManager.h>
#include <BSLib/multibyte.h>
#include <GlobalDataManager.h>
#include <Macro/IFMacroMenuAutoHunt.h>
#include <EntityManagerClient.h>
#include <ICMonster.h>
#include <NavMesh/LocationInfo.h>
#include <NavMesh/IRegionManager.h>
#include <Macro/IFMacroMenu.h>
#include <sstream>
#include "IFCounterWnd.h"



GFX_IMPLEMENT_DYNCREATE(CIFCounterWnd, CIFWnd)

GFX_BEGIN_MESSAGE_MAP(CIFCounterWnd, CIFWnd)

GFX_END_MESSAGE_MAP()

CIFCounterWnd::CIFCounterWnd(void){
}
CIFCounterWnd::~CIFCounterWnd(void){

}

bool CIFCounterWnd::OnCreate(long ln)
{

    // Populate inherited members
    CIFWnd::OnCreate(ln);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifcounterwnd.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    m_CountLeft = m_IRM.GetResObj<CIFStatic>(8, 1);
    m_CountRight = m_IRM.GetResObj<CIFStatic>(10, 1);

    UpdateCounter(0, 5);
    UpdateMenuSize();
    this->ShowGWnd(false);
    return true;
}

void CIFCounterWnd::UpdateMenuSize()
{


    CIFWnd * minimap = g_pCGInterface->m_IRM.GetResObj(10, 1);
    if(minimap != NULL)
    {
        if(minimap->IsVisible())
        {
            wnd_pos x = minimap->GetPos();
            this->MoveGWnd(x.x - 100, x.y + 130);
        }
    }
    BringToFront();
}

void CIFCounterWnd::UpdateCounter(int Left, int Right)
{
    if(Left == 0 || Left == 2 || Left == 4 || Left == 5 || Left == 6 || Left == 8 || Left == 9)
    {
        m_CountLeft->SetGWndSize(21, 31);
    }
    else if(Left == 1)
    {
        m_CountLeft->SetGWndSize(16, 31);
    }
    else if(Left == 3)
    {
        m_CountLeft->SetGWndSize(20, 31);
    }
    else if(Left == 7)
    {
        m_CountLeft->SetGWndSize(19, 31);
    }

    if(Right == 0 || Right == 2 || Right == 4 || Right == 5 || Right == 6 || Right == 8 || Right == 9)
    {
        m_CountRight->SetGWndSize(21, 31);
    }
    else if(Right == 1)
    {
        m_CountRight->SetGWndSize(16, 31);
    }
    else if(Right == 3)
    {
        m_CountRight->SetGWndSize(20, 31);
    }
    else if(Right == 7)
    {
        m_CountRight->SetGWndSize(19, 31);
    }


    char Leftddj[100];
    char Rightddj[100];

    sprintf(Leftddj, "clientlibrary\\counter\\count_%d.ddj", Left);
    sprintf(Rightddj, "clientlibrary\\counter\\count_%d.ddj", Right);

    m_CountLeft->TB_Func_13(Leftddj, 0, 0);
    m_CountRight->TB_Func_13(Rightddj, 0, 0);
}

void CIFCounterWnd::OnUpdate()
{

}


void CIFCounterWnd::On_BtnClick() {


}
