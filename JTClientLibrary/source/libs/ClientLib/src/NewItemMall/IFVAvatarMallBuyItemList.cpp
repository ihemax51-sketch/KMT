#include "IFVAvatarMallBuyItemList.h"
#include <NetProcessIn.h>
#include <ICPlayer.h>
#include <BSLib/multibyte.h>
#include <GlobalDataManager.h>
#include "GInterface.h"
#include "TextStringManager.h"
#include "IFSystemMessage.h"
#include "Game.h"
#include "IFVItemMall.h"
#include "IFVAvatarMallBuyItemListSlot.h"
#include <BSLib/Debug.h>
#include <sstream>
#include <algorithm>

#define GDR_BAR1 5
#define GDR_BAR2 6
#define GDR_BAR3 7
#define GDR_STATIC1 9
#define GDR_STATIC2 10
#define GDR_TAKE 12
#define GDR_SCROLL_BAR 12
GFX_IMPLEMENT_DYNCREATE(CIFVAvatarMallBuyItemList, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFVAvatarMallBuyItemList, CIFMainFrame)
                    ONG_COMMAND(GDR_TAKE, &TakeButton)
GFX_END_MESSAGE_MAP()



CIFVAvatarMallBuyItemList::CIFVAvatarMallBuyItemList(void){

    fullprice = 0;

    PricePics = 0;
    m_scroll = 0;
    item = 0;

    TakeBar = 0;
    SelectedItemName = L"";
    MallID = 0;
    ItemList = std::map<unsigned short, ItemsStruct>();
    BS_DEBUG_LOW(">" __FUNCTION__);
}
CIFVAvatarMallBuyItemList::~CIFVAvatarMallBuyItemList(void){

    BS_DEBUG_LOW(">" __FUNCTION__);
}
bool CIFVAvatarMallBuyItemList::OnCreate(long ln)
{
    // Populate inherited members
    CIFMainFrame::OnCreate(ln);

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifvavatarmallbuy.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    this->SetText(KmtGetText(L"UIIT_KMT_PURCHASE_LIST"));
    this->m_IRM.GetResObj(12, 1)->SetText(KmtGetText(L"UIIT_KMT_PURCHASE_ALL"));

    wnd_rect sz;

    sz.pos.x = 36;
    sz.pos.y = 321;
    sz.size.width = 16;
    sz.size.height = 16;
    PricePics = (CIFStatic*) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, GDR_STATIC1, 1);



    sz.pos.x= 56;
    sz.pos.y = 319;
    sz.size.width = 40;
    sz.size.height = 20;
    fullprice = (CIFStatic*) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, GDR_STATIC2, 1);
    fullprice->JustifyHorizontal(JUSTIFY_LEFT);
    fullprice->JustifyVertical(JUSTIFY_MIDDLE);




    UpdateMenuSize();
    this->ShowGWnd(false);
    return true;
}
#define GDR_CHECKAMK 13134
void CIFVAvatarMallBuyItemList::OpenButton(int timeoutSeconds) {
    this->m_IRM.GetResObj<CIFButton>(12, 1)->SetClickable(false);
    this->StartTimer(GDR_CHECKAMK, timeoutSeconds);
}
void CIFVAvatarMallBuyItemList::OnTimer(int timerId) {
    if (timerId == GDR_CHECKAMK) {
        this->KillTimer(GDR_CHECKAMK);
        if(ItemList.size() > 0)
        {
            TakeButton();
        }
        else
        {
            this->m_IRM.GetResObj<CIFButton>(12, 1)->SetClickable(true);

        }

    }
}
void CIFVAvatarMallBuyItemList::TakeButton(){

    if(ItemList.size() > 0)
    {

        for(std::map<unsigned short, ItemsStruct>::iterator it = ItemList.begin(); it != ItemList.end(); it++)
        {
            if(it->second.Price <= g_pCGInterface->m_silk_own)
            {
                CMsgStreamBuffer buf(0XB297);
                byte type = 12;
                buf << type;
                buf << it->second.DBID;
                ItemList.erase(it);
                UpdateList();
                SendMsg(buf);
                OpenButton(1000);
                break;
            }
            else
            {
                g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_THIS_PURCHASE_CANNOT_COMPLETE_DUE_TO_INSUFFICIENT_SILKS"));
                g_pCGInterface->ShowLogMessage(0xFFDBC99B, KmtGetText(L"UIIT_KMT_THIS_PURCHASE_CANNOT_COMPLETE_DUE_TO_INSUFFICIENT_SILKS"));
                this->m_IRM.GetResObj<CIFButton>(12, 1)->SetClickable(true);
                break;
            }

        }

    }

}


void CIFVAvatarMallBuyItemList::OnUpdate()
{
    //if (!g_pCGInterface->m_IRM.GetResObj<CIFVItemMall>(1353, 1)->IsVisible() || g_pCGInterface->m_IRM.GetResObj<CIFVAvatarMall>(1352, 1)->IsVisible())
    //{
    //    this->ShowGWnd(false);
    //}
}

void CIFVAvatarMallBuyItemList::UpdateMenuSize()
{
    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}

void CIFVAvatarMallBuyItemList::Clear()
{
    fullprice->SetText(L"");
    PricePics->TB_Func_13("", 1, 1);
}

const unsigned short priority[] = {3757, 5805, 7853, 4301, 2253};
const int prioritySize = sizeof(priority) / sizeof(priority[0]);

bool CompareByPriority(const CIFVAvatarMallBuyItemList::ItemsStruct& a, const CIFVAvatarMallBuyItemList::ItemsStruct& b) {
    int indexA = std::find(priority, priority + prioritySize, a.TypeId) - priority;
    int indexB = std::find(priority, priority + prioritySize, b.TypeId) - priority;
    return indexA < indexB;
}

void CIFVAvatarMallBuyItemList::UpdateList() {
    for (int i = 0; i < 6; i++) {
        m_IRM.GetResObj<CIFVAvatarMallBuyItemListSlot>(20 + i, 1)->ClearSlot();
    }

    // Map'in elemanlarını bir vector'e kopyala
    std::vector<ItemsStruct> sortedItems;
    for (std::map<unsigned short, ItemsStruct>::iterator it = ItemList.begin(); it != ItemList.end(); ++it) {
        sortedItems.push_back(it->second);
    }

    // Vector'ü öncelik sırasına göre sırala
    std::sort(sortedItems.begin(), sortedItems.end(), CompareByPriority);

    int i = 0;
    int FullPrice = 0;
    for (std::vector<ItemsStruct>::iterator it = sortedItems.begin(); it != sortedItems.end(); ++it) {
        FullPrice += it->Price;
        // FullPrice += it->second.Price;

        m_IRM.GetResObj<CIFVAvatarMallBuyItemListSlot>(20 +i,1)->SetName(it->DBID, it->ItemId, it->Price);
        m_IRM.GetResObj<CIFVAvatarMallBuyItemListSlot>(20 +i,1)->ShowGWnd(true);
        m_IRM.GetResObj<CIFVAvatarMallBuyItemListSlot>(20 +i,1)->BringToFront();
        m_IRM.GetResObj<CIFVAvatarMallBuyItemListSlot>(20 +i,1)->m_itemInfo->BringToFront();
        i++;
    }
    FullPriceForPacket = FullPrice;

    wchar_t Priceb[255];
    std::wstring test = Insert(FullPrice);
    swprintf_s(Priceb, KmtGetText(L"UIIT_KMT_TEXT_SILK"), test.c_str());


    m_IRM.GetResObj<CIFStatic>(44, 1)->SetText(Priceb);
    m_IRM.GetResObj<CIFStatic>(44, 1)->ShowGWnd(true);
}