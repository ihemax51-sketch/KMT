#include <Game.h>
#include <ICPlayer.h>
#include <GInterface.h>
#include <TextStringManager.h>
#include <BSLib/multibyte.h>
#include <GlobalDataManager.h>
#include <CustomData/CustomDataManager.h>
#include "IFWeb.h"



#define GDR_GRANTNAME_BTN_APPLY 9
#define GDR_GRANTNAME_EDIT 7
#define GDR_GRANTNAME_BG3 52
#define GDR_GRANTNAME_BG2 51
#define GDR_GRANTNAME_BG1 50
#define GDR_GRANTNAME_LABEL 53


GFX_IMPLEMENT_DYNCREATE(CIFWeb, CIFMainFrame)

GFX_BEGIN_MESSAGE_MAP(CIFWeb, CIFMainFrame)
                    ONG_COMMAND(4, &CIFWeb::Charge)
GFX_END_MESSAGE_MAP()

CIFWeb::CIFWeb(void){
}
CIFWeb::~CIFWeb(void){

}
void CIFWeb::Charge()
{
    ShellExecute(NULL, "open", m_CustomDataManager->WebSiteUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
}
bool CIFWeb::OnCreate(long ln)
{

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifweb.txt");
    m_IRM.CreateInterfaceSection("Create", this);


    m_pCloseBtn->ShowGWnd(true);
    m_pCloseBtn->BringToFront();
    m_pCloseBtn->TB_Func_13("clientlibrary\\mall\\mall_web_close_button.ddj", 1, 0);
    m_pCloseBtn->SetGWndSize(28,32);
    m_pCloseBtn->MoveGWnd(764, 2);
    m_IRM.GetResObj<CIFButton>(4, 1)->SetText(KmtGetText(L"UIIT_KMT_SILK_CHARGE"));
    m_IRM.GetResObj<CIFButton>(4, 1)->BringToFront();

    m_Web = new SilkroadWeb();
    this->ShowGWnd(false);
    return true;
}

void CIFWeb::UpdateMenuSize()
{



    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();


    m_Web->Start(this->GetPos().x, this->GetPos().y+30, 800, 570, "http://localhost:8080/ranking_level.php");


}


void CIFWeb::OnUpdate()
{

}
undefined CIFWeb::OnCloseWnd(){

    m_Web->Close();
    return CIFWnd::OnCloseWnd();
}

