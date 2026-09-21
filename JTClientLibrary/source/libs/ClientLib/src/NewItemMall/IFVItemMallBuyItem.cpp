#include <Game.h>
#include "IFVItemMallBuyItem.h"
#include <NetProcessIn.h>
#include <BSLib/multibyte.h>
#include <GInterface.h>
#include <sstream>

#include <BSLib/Debug.h>
#include <TextStringManager.h>

#define GDR_STATIC1 6
#define GDR_SLOT1 7
#define GDR_TEXTBOX 8
#define GDR_STATIC3 9
#define GDR_STATIC4 10
#define GDR_PURCHASE 11
#define GDR_CANCEL_BTN 12
#define GDR_EKSI 13
#define GDR_ARTI 14
#define GDR_STATIC5 15

GFX_IMPLEMENT_DYNCREATE(CIFVItemMallBuyItem, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFVItemMallBuyItem, CIFMainFrame)
                    ONG_COMMAND(GDR_PURCHASE, &CIFVItemMallBuyItem::On_BtnClick_1)
                    ONG_COMMAND(GDR_CANCEL_BTN, &CIFVItemMallBuyItem::On_BtnClick_2)
                    ONG_COMMAND(GDR_ARTI, &CIFVItemMallBuyItem::AddQuantityBtn)
                    ONG_COMMAND(GDR_EKSI, &CIFVItemMallBuyItem::RemoveQuantityBtn)
GFX_END_MESSAGE_MAP()

CIFVItemMallBuyItem::CIFVItemMallBuyItem(void) {

    IconID = 0;

    Quan = 0;
    Pricex = 0;

    MallID = 0;
    BS_DEBUG_LOW(">" __FUNCTION__);
}
CIFVItemMallBuyItem::~CIFVItemMallBuyItem(void){
    BS_DEBUG_LOW(">" __FUNCTION__);
}


bool CIFVItemMallBuyItem::OnCreate(long ln)
{
    //	printf("> " __FUNCTION__ "(%d)\n", ln);

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifvitemmallbuyitem.txt");
    m_IRM.CreateInterfaceSection("Create", this);


    m_IRM.GetResObj(12, 1)->SetText(KmtGetText(L"UIIT_KMT_CANCEL"));

    this->m_pCloseBtn->ShowGWnd(false);
    m_itemInfo = m_IRM.GetResObj<CIFSlotWithHelp>(4, 1);
    m_itemInfo->ShowGWnd(true);
    m_itemInfo->SetClickable(false);
    // m_itemInfo->TB_Func_14("item\\etc\\icon_edge_rare.ddj", 1,0);

    IconName = m_IRM.GetResObj<CIFTextBox>(5, 1);
    IconName->JustifyVertical(JUSTIFY_TOP);
    IconName->JustifyHorizontal(JUSTIFY_LEFT);
    IconName->SetLineHeight(14);
    IconName->BringToFront();





    this->m_IRM.GetResObj(11, 1)->SetText(KmtGetText(L"UIIT_KMT_PURCHASE"));

    //ItemSlotPic->ShowGWnd(false);
    m_itemInfo->BringToFront();
    m_itemInfo->SetType(19);

    this->Quan = 1;
    this->Pricex = 31;


    this->ShowGWnd(false);

    UpdateMenuSize();


    return true;
}

void CIFVItemMallBuyItem::OnUpdate()
{
    //if(!g_pCGInterface->m_IRM.GetResObj<CIFVItemMall>(1353, 1)->IsVisible() || g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(1352, 1)->IsVisible())
    //{
    //    this->ShowGWnd(false);
    //}
}

void CIFVItemMallBuyItem::UpdateMenuSize()
{
    m_IRM.GetResObj(15, 1)->SetText(L"1");

    wchar_t Priceb[255];



    std::wstring test = Insert(Pricex);
    swprintf_s(Priceb, KmtGetText(L"UIIT_KMT_TEXT_SILK"), test.c_str());


    m_IRM.GetResObj<CIFStatic>(7, 1)->SetText(Priceb);


    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}

void CIFVItemMallBuyItem::On_BtnClick_1()
{
    if (MallID == 1)
    {
        if (IconID != 0)
        {
            CMsgStreamBuffer buf(0XB297);
            byte type = 11;
            buf << type;
            buf << IconID << Quan;
            SendMsg(buf);
        }

    }
    else if (MallID == 2)
    {
        if (IconID != 0)
        {
            NEWMSG(0x204D)
                pReq << IconID << Quan;
            SENDMSG()
        }
    }
    this->OnCloseWnd();

}

void CIFVItemMallBuyItem::On_BtnClick_2()
{
    this->OnCloseWnd();
}

void CIFVItemMallBuyItem::AddQuantityBtn(){
    wchar_t Priceb[255];

    if(Quan > 0 && Quan <= 50)
    {
        Quan++; /// +1 adet

        swprintf_s(Priceb, L"%d", Quan);
        m_IRM.GetResObj(15, 1)->SetText(Priceb);

        // Pricex = SabitPrice*Quan;

        int x = Quan;

        int Para = Pricex*x;




        std::wstring test = Insert(Para);
        swprintf_s(Priceb, KmtGetText(L"UIIT_KMT_SILK_TEXT_8D4D7493"), test.c_str());


        m_IRM.GetResObj<CIFStatic>(7, 1)->SetText(Priceb);

    }
}
int Para;
void CIFVItemMallBuyItem::RemoveQuantityBtn(){
    wchar_t Priceb[255];

    if(Quan > 1)
    {
        Quan--;
        swprintf_s(Priceb, L"%d", Quan);
        m_IRM.GetResObj(15, 1)->SetText(Priceb);

        //Pricex = SabitPrice*Quan;

        int x = Quan;

        Para = Pricex*x;


        std::wstring test = Insert(Para);
        swprintf_s(Priceb, KmtGetText(L"UIIT_KMT_SILK_TEXT_8D4D7493"), test.c_str());


        m_IRM.GetResObj<CIFStatic>(7, 1)->SetText(Priceb);
    }
}
