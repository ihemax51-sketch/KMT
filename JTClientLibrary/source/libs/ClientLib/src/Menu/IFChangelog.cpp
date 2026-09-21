#include <Game.h>
#include "IFChangelog.h"

#include <NetProcessIn.h>
#include <GInterface.h>
#include <BSLib/Debug.h>
#include <TextStringManager.h>
#include <CustomData/CustomDataManager.h>
#include <IFPML.h>
#include <CustomData/CustomSettingManager.h>


GFX_IMPLEMENT_DYNCREATE(CIFChangelog, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFChangelog, CIFMainFrame)
GFX_END_MESSAGE_MAP()

CIFChangelog::CIFChangelog(void){
    BS_DEBUG_LOW(">" __FUNCTION__);
}
CIFChangelog::~CIFChangelog(void){
    BS_DEBUG_LOW(">" __FUNCTION__);
}
int CIFChangelog::Func_4(int a2) {
    int v1 = 0;
    while (a2 != v1 + 100) {
        if (++v1 >= 5)
            return -1;
    }

    return 100;
}

bool CIFChangelog::OnCreate(long ln)
{
    // Populate inherited members
    CIFMainFrame::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifchangelog.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    this->SetText(KmtGetText(L"UIIT_KMT_CHANGELOG"));
    // TB_Func_13("interface\\frame\\mframe_wnd_", 0, 0);

    m_Scroll = m_IRM.GetResObj<CIFVerticalScroll>(11, 1);
    this->m_IRM.GetResObj<CIFPML>(10, 1)->LinkScrollbar(m_Scroll);



    /* this->m_IRM.GetResObj<CIFPML>(10, 1)->m_HeightInLines = 10;

     this->m_IRM.GetResObj<CIFPML>(10, 1)->SetLineHeight(32);
      this->m_IRM.GetResObj<CIFPML>(10, 1)->sub_638B50(0);
      this->m_IRM.GetResObj<CIFPML>(10, 1)->sub_64E380(0);
      this->m_IRM.GetResObj<CIFPML>(10, 1)->sub_638C70(1);
      this->m_IRM.GetResObj<CIFPML>(10, 1)->sub_638D50(1);
     this->m_IRM.GetResObj<CIFPML>(10, 1)->sub_638D40(1);
 */


    this->m_IRM.GetResObj<CIFPML>(10, 1)->SetText(TSM_GETTEXTPTR(L"VFILTER_CHANGELOG_TEXT"));
    //this->m_IRM.GetResObj<CIFPML>(10, 1)->m_FontTexture.FUN_008b3bd0(&newt);
    //this->m_IRM.GetResObj<CIFPML>(10, 1)->m_FontTexture.sub_8B3B60(&newt);
    m_Scroll->sub_65A5A0(9999);



    UpdateMenuSize();
    if(m_Settings->EnableChangeLog)
    {
        if(m_Settings->ShowChangeLogFirstLogin)
        {
            this->ShowGWnd(true);
        }
        else
        {
            this->ShowGWnd(false);
        }
    }
    else
    {
        this->ShowGWnd(false);
    }



    return true;
}
int CIFChangelog::Func_36(int a1, short action, int a3, int a4) {
    if (action <= 0) {
        if (action < 0) {
            m_Scroll->sub_65A5C0(0);
        }
    } else {
        m_Scroll->sub_65A5A0(0);
    }

    return 1;
}

void CIFChangelog::OnUpdate()
{
}

void CIFChangelog::UpdateMenuSize()
{

    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}


