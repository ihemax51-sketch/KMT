#include <Game.h>
#include "IFSocial.h"

#include <NetProcessIn.h>
#include <GInterface.h>
#include <BSLib/Debug.h>
#include <TextStringManager.h>
#include <CustomData/CustomDataManager.h>



#define GDR_YES 9
#define GDR_NO 10

GFX_IMPLEMENT_DYNCREATE(CIFSocial, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFSocial, CIFMainFrame)
                    ONG_COMMAND(GDR_YES, &CIFSocial::On_BtnClick_1)
                    ONG_COMMAND(GDR_NO, &CIFSocial::On_BtnClick_2)
GFX_END_MESSAGE_MAP()

CIFSocial::CIFSocial(void){
    SocialID = 0;
    BS_DEBUG_LOW(">" __FUNCTION__);
}
CIFSocial::~CIFSocial(void){
    BS_DEBUG_LOW(">" __FUNCTION__);
}
bool CIFSocial::OnCreate(long ln)
{
    // Populate inherited members
    CIFMainFrame::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifsocial.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    // TB_Func_13("interface\\frame\\mframe_wnd_", 0, 0);
    this->m_pCloseBtn->ShowGWnd(false);
    UpdateMenuSize();
    this->ShowGWnd(false);


    return true;
}

void CIFSocial::OnUpdate()
{
    if(SocialID == 0)
    {
        m_IRM.GetResObj<CIFStatic>(8, 1)->SetText(KmtGetText(L"UIIT_KMT_DO_YOU_WANT_VISIT_OFFICIAL_FACEBOOK_SITE"));
    }
    else if(SocialID == 1)
    {
        m_IRM.GetResObj<CIFStatic>(8, 1)->SetText(KmtGetText(L"UIIT_KMT_DO_YOU_WANT_VISIT_OFFICIAL_DISCORD_CHANNEL"));
    }
    else if(SocialID == 2)
    {
        m_IRM.GetResObj<CIFStatic>(8, 1)->SetText(KmtGetText(L"UIIT_KMT_DO_YOU_WANT_VISIT_OFFICIAL_WEB_SITE"));
    }
    else if (SocialID == 3)
    {
        m_IRM.GetResObj<CIFStatic>(8, 1)->SetText(KmtGetText(L"UIIT_KMT_DO_YOU_RECALL_TO_AUTO_EQUIP"));
    }
}

void CIFSocial::UpdateMenuSize()
{

    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}

void CIFSocial::On_BtnClick_1()
{
    if(SocialID == 0)
    {
        ShellExecute(NULL, "open", m_CustomDataManager->FacebookUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
    else if(SocialID == 1)
    {
        ShellExecute(NULL, "open", m_CustomDataManager->DiscordUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
    else if(SocialID == 2)
    {
        ShellExecute(NULL, "open", m_CustomDataManager->WebSiteUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
    else if (SocialID == 3)
    {
        CMsgStreamBuffer buf(0x400B);
        SendMsg(buf);
    }
    this->OnCloseWnd();
}

void CIFSocial::On_BtnClick_2() {
    this->OnCloseWnd();

}
