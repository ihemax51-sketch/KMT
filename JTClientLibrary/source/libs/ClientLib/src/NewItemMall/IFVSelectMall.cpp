#include <Game.h>
#include "IFVSelectMall.h"
#include <NetProcessIn.h>
#include <BSLib/multibyte.h>
#include <GInterface.h>
#include <IFMessageBox.h>
#include "IFVItemMall.h"
#include "IFVAvatarMall.h"
#include <BSLib/Debug.h>
#include <TextStringManager.h>


#define GDR_ITEMMALL 9
#define GDR_AVATARMALL 10
#define GDR_ITEMMALLBOX 11
#define GDR_AVATARMALLBOX 12
#define GDR_OPENMALL 14
#define GDR_CANCEL 15
#define GDR_BOX 16

GFX_IMPLEMENT_DYNCREATE(CIFVSelectMall, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFVSelectMall, CIFMainFrame)
                    ONG_COMMAND(GDR_ITEMMALL, &CIFVSelectMall::ItemMallBtn)
                    ONG_COMMAND(GDR_AVATARMALL, &CIFVSelectMall::AvatarMallBtn)

                    ONG_COMMAND(GDR_ITEMMALLBOX, &CIFVSelectMall::ItemMallCheckBox)
                    ONG_COMMAND(GDR_AVATARMALLBOX, &CIFVSelectMall::AvatarMallCheckBox)

                    ONG_COMMAND(GDR_OPENMALL, &CIFVSelectMall::OpenMall)
                    ONG_COMMAND(GDR_CANCEL, &CIFVSelectMall::Cancel)
GFX_END_MESSAGE_MAP()

CIFVSelectMall::CIFVSelectMall(void){

    BS_DEBUG_LOW(">" __FUNCTION__);
}
CIFVSelectMall::~CIFVSelectMall(void){
    BS_DEBUG_LOW(">" __FUNCTION__);
}


bool CIFVSelectMall::OnCreate(long ln)
{
    //	printf("> " __FUNCTION__ "(%d)\n", ln);
// Populate inherited members
    CIFMainFrame::OnCreate(ln);
    // TB_Func_13("interface\\frame\\mframe_wnd_", 0, 0);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifvselectmall.txt");
    m_IRM.CreateInterfaceSection("Create", this);
    m_IRM.GetResObj(15, 1)->SetText(KmtGetText(L"UIIT_KMT_CANCEL"));

    m_pCloseBtn->ShowGWnd(false);
    this->m_IRM.GetResObj(4, 1)->SetText(KmtGetText(L"UIIT_KMT_THESE_ITEM_MALL_WINDOWS_ARE_DESIGNED_AND_ADDED_BY_OWN_FOR_YOU_VALUABLE_PLAYERS"));
    m_pTitleText->SetText(KmtGetText(L"UIIT_KMT_ENTER_THE_NEW_ITEM_MALL"));
    m_pTitleText->m_FontTexture.sub_8B4750(7);
    m_pTitleText->m_FontTexture.SetColor(D3DCOLOR_ARGB(255,239,218,164));
    m_pTitleText->BringToFront();
    m_pTitleText->MoveGWnd(m_pTitleText->GetPos().x, m_pTitleText->GetPos().y + 10);



    m_IRM.GetResObj<CIFTextBox>(4, 1)->SetLineHeight(14);

    // opens->SetText(TSM_GETTEXTPTR(L"LEXA_MENU_WEBMALL_START2"));


    this->m_pCloseBtn->ShowGWnd(false);
    //  this->m_handleBar->ShowGWnd(false);
    this->ShowGWnd(false);

    UpdateMenuSize();

    return true;
}

void CIFVSelectMall::OnUpdate()
{
    this->BringToFront();
}

void CIFVSelectMall::UpdateMenuSize()
{

    m_IRM.GetResObj<CIFCheckBox>(GDR_ITEMMALLBOX, 1)->SetCheckBoxState(true);
    m_IRM.GetResObj<CIFCheckBox>(12, 1)->SetCheckBoxState(false);
    // m_IRM.GetResObj<CIFCheckBox>(11, 1)->BackGround(L"interface\\ifcommon\\com_radiobutton_on.ddj");
    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}
void CIFVSelectMall::AvatarMallBtn(){

    m_IRM.GetResObj<CIFCheckBox>(GDR_ITEMMALLBOX, 1)->SetCheckBoxState(false);
    m_IRM.GetResObj<CIFCheckBox>(12, 1)->SetCheckBoxState(true);
}
void CIFVSelectMall::ItemMallBtn(){
    m_IRM.GetResObj<CIFCheckBox>(GDR_ITEMMALLBOX, 1)->SetCheckBoxState(true);
    m_IRM.GetResObj<CIFCheckBox>(12, 1)->SetCheckBoxState(false);
}
void CIFVSelectMall::ItemMallCheckBox()
{
    if(m_IRM.GetResObj<CIFCheckBox>(GDR_ITEMMALLBOX, 1)->GetCheckedState_MAYBE())
    {
        m_IRM.GetResObj<CIFCheckBox>(GDR_AVATARMALLBOX, 1)->SetCheckBoxState(false);
    }

    else if(!m_IRM.GetResObj<CIFCheckBox>(GDR_AVATARMALLBOX, 1)->GetCheckedState_MAYBE())
    {
        m_IRM.GetResObj<CIFCheckBox>(GDR_ITEMMALLBOX, 1)->SetCheckBoxState(true);
    }
}
void CIFVSelectMall::AvatarMallCheckBox()
{
    if(m_IRM.GetResObj<CIFCheckBox>(GDR_AVATARMALLBOX, 1)->GetCheckedState_MAYBE())
    {
        m_IRM.GetResObj<CIFCheckBox>(GDR_ITEMMALLBOX, 1)->SetCheckBoxState(false);
    }

    else if(!m_IRM.GetResObj<CIFCheckBox>(GDR_ITEMMALLBOX, 1)->GetCheckedState_MAYBE())
    {
        m_IRM.GetResObj<CIFCheckBox>(GDR_AVATARMALLBOX, 1)->SetCheckBoxState(true);
    }
}
void CIFVSelectMall::OpenMall(){
    if (m_IRM.GetResObj<CIFCheckBox>(11, 1)->GetCheckedState_MAYBE()) {
        if (!g_pCGInterface->m_IRM.GetResObj<CIFVItemMall>(NewItemMallId, 1)->IsVisible()) {
            g_pCGInterface->m_IRM.GetResObj<CIFVItemMall>(NewItemMallId, 1)->UpdateMenuSize();
            g_pCGInterface->m_IRM.GetResObj<CIFVItemMall>(NewItemMallId, 1)->ShowGWnd(true);

            if (g_pCGInterface->m_IRM.GetResObj<CIFVItemMall>(NewItemMallId, 1)->aaa = !0) {
                g_pCGInterface->m_IRM.GetResObj<CIFVItemMall>(NewItemMallId, 1)->aaa = 0;
            }


            CGEffSoundBody::get()->PlaySound(L"snd_window_open");
        }
    }
    else if (m_IRM.GetResObj<CIFCheckBox>(12, 1)->GetCheckedState_MAYBE()) {
        if (!g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->IsVisible()) {

            g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->UpdateMenuSize();
            g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->ActivedPage = 0;

            g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->ActivateTabPage(0);


            g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(AvatarMallId, 1)->ShowGWnd(true);

            /*  if (g_pCGInterface->GetGuiFromList<CIFVAvatarMall>(1352)->_VMarkets.empty()) {
                  CMsgStreamBuffer buf(0x169A);
                  byte type = 13;
                  buf << type;
                  SendMsg(buf);
              }*/

            CGEffSoundBody::get()->PlaySound(L"snd_window_open");
        }
    }

    this->OnCloseWnd();
}
void CIFVSelectMall::Cancel()
{
    this->OnCloseWnd();
}


