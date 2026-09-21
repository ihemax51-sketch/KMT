#include <NetProcessIn.h>
#include <ICPlayer.h>
#include <BSLib/multibyte.h>
#include "GInterface.h"
#include "IFSystemMessage.h"
#include "Game.h"
#include "IFVItemMall.h"
#include "IFVItemMallBuyItem.h"

#include <algorithm>
#include <BSLib/Debug.h>

#include <TextStringManager.h>
#include <Data/ItemMallGroupData.h>
#include <GlobalDataManager.h>
#include <CustomData/CustomDataManager.h>
#include <CustomData/CustomCICPlayer.h>
#include <CustomData/CustomSettingManager.h>

#define GDR_CHARGE 4
#define GDR_STATIC1 7
#define GDR_CHECKTOKENITEMS 17
#define GDR_CHECKGOLDITEMS 18
#define GDR_OPENLIST 19
#define GDR_RIGHBTN 13
#define GDR_LEFTBTN 14
#define GDR_STATIC13 25
#define GDR_SCROLL_BAR 160




#define GDR_SILK_RANK_ICON 9
#define GDR_CUR_SILK 6

GFX_IMPLEMENT_DYNCREATE(CIFVItemMall, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFVItemMall, CIFMainFrame)
                    ONG_COMMAND(100, &CIFVItemMall::OnUnknownStuff)
                    ONG_COMMAND(GDR_RIGHBTN, &CIFVItemMall::RightBtn)
                    ONG_COMMAND(GDR_LEFTBTN, &CIFVItemMall::LeftBtn)
                    ONG_COMMAND(GDR_CHARGE, &CIFVItemMall::Charge)
                    ONG_COMMAND(GDR_OPENLIST, &CIFVItemMall::OpenList)
                    ONG_COMMAND(GDR_CHECKTOKENITEMS, &CIFVItemMall::ItemMall)
                    ONG_COMMAND(GDR_CHECKGOLDITEMS, &CIFVItemMall::Preset)
                    ONG_COMMAND(23, &CIFVItemMall::TakeAll)
                    ONG_COMMAND(26, &CIFVItemMall::RightBtnTab)
                    ONG_COMMAND(27, &CIFVItemMall::LeftBtnTab)
GFX_END_MESSAGE_MAP()



void CIFVItemMall::RightBtnTab() {
    int nextGroup = m_currentGroup + 1;
    int firstIndex = nextGroup * 6;
    if (firstIndex >= numberOfTabs) return; // Ensure we don't go out of bounds

    // Geçerli grubun sekmelerini gizle
    for (int i = 0; i < 6; ++i) {
        if (m_currentGroup * 6 + i < numberOfTabs) {
            m_pTabs[m_currentGroup * 6 + i]->ShowGWnd(false);
        }
    }

    // Yeni grubun sekmelerini göster
    for (int i = 0; i < 6; ++i) {
        if (firstIndex + i < numberOfTabs) {
            m_pTabs[firstIndex + i]->ShowGWnd(true);
        }
    }

    m_currentGroup = nextGroup;
}

void CIFVItemMall::LeftBtnTab() {
    int previousGroup = m_currentGroup - 1;
    if (previousGroup < 0) return; // Ensure we don't go out of bounds

    int firstIndex = previousGroup * 6;

    // Geçerli grubun sekmelerini gizle
    for (int i = 0; i < 6; ++i) {
        if (m_currentGroup * 6 + i < numberOfTabs) {
            m_pTabs[m_currentGroup * 6 + i]->ShowGWnd(false);
        }
    }

    // Önceki grubun sekmelerini göster
    for (int i = 0; i < 6; ++i) {
        if (firstIndex + i < numberOfTabs) {
            m_pTabs[firstIndex + i]->ShowGWnd(true);
        }
    }

    m_currentGroup = previousGroup;
}
CIFVItemMall::CIFVItemMall(void){
    ActivedPage = 0;
    std::set<int> uniqueCategories;

    std::map<int, CustomDataManager::CustomItemMallItemStruct>::iterator it;
    for (it = m_CustomDataManager->CustomItemMallItemList.begin(); it != m_CustomDataManager->CustomItemMallItemList.end(); ++it) {
        uniqueCategories.insert(it->second.Type);

    }
    numberOfTabs = uniqueCategories.size();
    aaa = 0;
    FavItemList = std::map<int, FavItem>();
    SilkRank = 0;
    SilkHistory = 0;
    BS_DEBUG_LOW(">" __FUNCTION__);
}
CIFVItemMall::~CIFVItemMall(void){

    BS_DEBUG_LOW(">" __FUNCTION__);
}

int CIFVItemMall::Func_4(int a2) {
    int v1 = 0;
    while (a2 != v1 + 100) {
        if (++v1 >= 11)
            return -1;
    }

    return 100;
}
void CIFVItemMall::OnUnknownStuff() {
    int id = GetCurrentEventMsgCtrlId();
    int i = 0;

    for (int i = 0; i < numberOfTabs; ++i) {
        if (id == m_pTabs[i]->UniqueID()) {
            ActivateTabPage(i);
            return;
        }
    }
}

bool CIFVItemMall::OnCreate(long ln)
{
    // Populate inherited members
    CIFMainFrame::OnCreate(ln);
    //SetText(L"VFilter Item Mall");

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifvitemmall.txt");
    m_IRM.CreateInterfaceSection("Create", this);


    wnd_rect sz;

    sz.pos.x= 7;
    sz.pos.y = 31;
    sz.size.width = 185;
    sz.size.height = 45;
    title = (CIFStatic* ) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFStatic), sz, GDR_STATIC1, 1);
    title->SetFont(theApp.GetFont(2));
    title->m_FontTexture.SetColor(D3DCOLOR_ARGB(255,239,218,164));
    title->SetText(KmtGetText(L"UIIT_KMT_NEW_ITEM_MALL"));


    m_IRM.GetResObj(8, 1)->m_FontTexture.sub_8B4750(7);



    m_IRM.GetResObj<CIFStatic>(15, 1)->m_FontTexture.sub_8B4750(7);



    RECT m_slotRect = {13, 243, 169, 130};
    for(int i =0; i < 2;i++) {
        m_slotnews[i] = (CIFVItemMallSlotNews*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFVItemMallSlotNews), m_slotRect, 601 + i, 0);
        if(i%2 == 0)
        {
            m_slotRect.top = 373;
        }
        m_slotnews[i]->ShowGWnd(false);
    }

    std::set<int> uniqueCategories;

    std::map<int, CustomDataManager::CustomItemMallItemStruct>::iterator it;
    for (it = m_CustomDataManager->CustomItemMallItemList.begin(); it != m_CustomDataManager->CustomItemMallItemList.end(); ++it) {
        uniqueCategories.insert(it->second.Type);

    }


    m_pTabs = new CIFSelectableArea * [numberOfTabs];
    for (int i = 0; i < numberOfTabs; i++) {
        if (i % 6 == 0) {
            // Her 6. tab için X pozisyonunu sıfırlıyoruz.
            sz.pos.x = 192;
        } else {
            sz.pos.x = (192 + tabWidth * (i % 6)) + 4;
        }

        sz.pos.y = 85;
        sz.size.width = tabWidth + 1;
        sz.size.height = tabHeight;
        m_pTabs[i] = (CIFSelectableArea*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSelectableArea),
                                                               sz, tabFirstId + i, 0);
        m_pTabs[i]->SetFont(this->N00009C2F);


        m_pTabs[i]->sub_64CE30("clientlibrary\\mall\\newtab.ddj",
                               "clientlibrary\\mall\\newtaboff.ddj", "clientlibrary\\mall\\newtabdisable.ddj");
        //  m_pTabs[i]->m_FontTexture.SetColor(D3DCOLOR_ARGB(255,239,218,164));
        if (i) {
            m_pTabs[i]->sub_64CC30(0);
        } else {
            m_pTabs[i]->sub_64CC30(1);
        }

        if (i < 6) {
            m_pTabs[i]->ShowGWnd(true);
        } else {
            m_pTabs[i]->ShowGWnd(false);
        }
    }



    m_scroll = m_IRM.GetResObj<CIFScrollManager>(16, 1);
    m_scroll->sub_008124F0(0);
    m_scroll->sub_008124C0(0x90);
    m_scroll->sub_008123F0(3);
    m_scroll->sub_00812500(0);
    m_scroll->sub_00812420(-8, 0);

    m_scrollfav = m_IRM.GetResObj<CIFScrollManager>(20, 1);
    m_scrollfav->sub_008124F0(0);
    m_scrollfav->sub_008124C0(82);
    m_scrollfav->sub_008123F0(5);
    m_scrollfav->sub_00812500(0);
    m_scrollfav->sub_00812420(-8, 0);
    m_scrollfav->ShowGWnd(false);


    sz.pos.x = 0;
    sz.pos.y = 0;
    sz.size.width = 600;
    sz.size.height = 80;
    for(int i =0; i < 150;i++) {
        m_slotviewer[i] = (CIFVItemMallSlotViewer*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFVItemMallSlotViewer), sz, 301 + i, 0);
        m_slotviewer[i]->ShowGWnd(false);
    }

    sz.pos.x = 0;
    sz.pos.y = 0;
    sz.size.width = 600;
    sz.size.height = 80;
    for(int i =0; i < 100;i++) {
        favlist[i] = (CIFVItemMallFavList*)CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFVItemMallFavList), sz, 600 + i, 0);
        favlist[i]->ShowGWnd(false);
    }


    m_pCloseBtn->ShowGWnd(true);
    m_pCloseBtn->BringToFront();
    m_scroll->m_verticalScroll->m_arrowUpButton->TB_Func_13("clientlibrary\\mall\\mall_barup.ddj",1, 0);

    m_scroll->m_verticalScroll->m_arrowDownButton->TB_Func_13("clientlibrary\\mall\\mall_bardown.ddj",1, 0);

    m_scroll->m_verticalScroll->m_scrollButton->TB_Func_13("clientlibrary\\mall\\mall_newscroll.ddj",1, 0);

    m_scroll->m_verticalScroll->TB_Func_13("",1, 0);


    m_scrollfav->m_verticalScroll->m_arrowUpButton->TB_Func_13("clientlibrary\\mall\\mall_barup.ddj",1, 0);

    m_scrollfav->m_verticalScroll->m_arrowDownButton->TB_Func_13("clientlibrary\\mall\\mall_bardown.ddj",1, 0);

    m_scrollfav->m_verticalScroll->m_scrollButton->TB_Func_13("clientlibrary\\mall\\mall_newscroll.ddj",1, 0);

    m_scrollfav->m_verticalScroll->TB_Func_13("",1, 0);


    m_pHandleBar->ShowGWnd(false);
    m_pCloseBtn->TB_Func_13("clientlibrary\\mall\\mall_web_close_button.ddj", 1, 0);
    m_pCloseBtn->SetGWndSize(28,32);
    m_pCloseBtn->MoveGWnd(764, 2);
    ActivedPage = 0;
//    UpdateMenuSize();

    m_IRM.GetResObj(19, 1)->SetClickable(false);
    m_IRM.GetResObj(19, 1)->m_FontTexture.sub_8B4750(7);

    m_IRM.GetResObj(19, 1)->ShowGWnd(false);
    m_IRM.GetResObj(21, 1)->ShowGWnd(false);
    m_IRM.GetResObj(22, 1)->ShowGWnd(false);

    m_IRM.GetResObj(23, 1)->ShowGWnd(false);
    m_IRM.GetResObj(25, 1)->ShowGWnd(false);
    m_IRM.GetResObj(26, 1)->ShowGWnd(false);

    m_IRM.GetResObj(23, 1)->SetText(KmtGetText(L"UIIT_KMT_PURCHASE_ALL"));
    m_IRM.GetResObj(19, 1)->SetText(KmtGetText(L"UIIT_KMT_FAVORITES"));
    m_IRM.GetResObj(18, 1)->SetText(KmtGetText(L"UIIT_KMT_FAVORITES"));
    m_IRM.GetResObj(17, 1)->SetText(KmtGetText(L"UIIT_KMT_ITEM_MALL"));
    m_IRM.GetResObj(15, 1)->SetText(KmtGetText(L"UIIT_KMT_POPULAR_ITEMS"));

    m_IRM.GetResObj(15, 1)->SetText(KmtGetText(L"UIIT_KMT_POPULAR_ITEMS"));
    m_IRM.GetResObj(8, 1)->SetText(KmtGetText(L"UIIT_KMT_SILK_RANK"));
    m_IRM.GetResObj(4, 1)->SetText(KmtGetText(L"UIIT_KMT_SILK_CHARGE"));

    m_IRM.GetResObj(15, 1)->SetText(KmtGetText(L"UIIT_KMT_POPULAR_ITEMS"));

    this->ShowGWnd(false);
    return true;
}

void CIFVItemMall::OnUpdate()
{

    wchar_t Priceb[255];

    std::wstring test = Insert(g_pCGInterface->m_silk_own);
    swprintf_s(Priceb, KmtGetText(L"UIIT_KMT_SILK_TEXT"), test.c_str());

    this->m_IRM.GetResObj(GDR_CUR_SILK, 1)->SetText(Priceb);


    if(m_CustomDataManager->MediaIcons.find(SilkRank) != m_CustomDataManager->MediaIcons.end())
    {
        this->m_IRM.GetResObj<CIFStatic>(9, 1)->TB_Func_13(m_CustomDataManager->MediaIcons[SilkRank], 1, 1);
    }
    wchar_t Pricebx[255];
    std::wstring test2 = Insert(SilkHistory);
    swprintf_s(Pricebx, KmtGetText(L"UIIT_KMT_HISTORY_TEXT_SILK"), test2.c_str());

    this->m_IRM.GetResObj(24, 1)->SetText(Pricebx);


    /*   wchar_t buffer123[255];
       swprintf_s(buffer123, sizeof(buffer123), L"%s : %d", TSM_GETTEXTPTR(L"LEXA_MENU_WEBMALL_TOKEN"), my_Maps->Token);

       this->Token->SetText(buffer123);

       wchar_t buffer1234[255];
       swprintf_s(buffer1234, sizeof(buffer1234), L"%s : - %d", TSM_GETTEXTPTR(L"LEXA_MENU_WEBMALL_USEDTOKEN"), my_Maps->UsedToken);

       this->usedToken->SetText(buffer1234);
   */

    /* std::n_wstring Charname = g_pCICPlayer->GetCharName().c_str();

     std::map<std::n_wstring, unsigned __int32>::iterator foundedInfo = my_Maps->m_RightCharIcons.find(Charname);
     if (foundedInfo != my_Maps->m_RightCharIcons.end())
     {
         if (foundedInfo->first == Charname)
         {
             std::map<int, std::n_string>::iterator foundedTheIcon = my_Maps->m_MarketIconData.find(foundedInfo->second);
             if (foundedTheIcon != my_Maps->m_MarketIconData.end())
             {
                 VipIcon->TB_Func_13(foundedTheIcon->second, 0, 0);
             }
         }
     }
 */
}

void CIFVItemMall::ClearFav()
{
    for (int i = 0; i < 100; ++i)
    {
        m_scrollfav->DeleteItem(favlist[i]);
    }
}

void CIFVItemMall::Clear()
{
    for (int i = 0; i < 150; ++i)
    {
        m_scroll->DeleteItem(m_slotviewer[i]);
        m_scroll->DeleteItem(m_slotviewer[i]->slot1);
        m_scroll->DeleteItem(m_slotviewer[i]->slot2);
        m_scroll->DeleteItem(m_slotviewer[i]->slot3);
    }
}
void CIFVItemMall::ClearNew()
{
    for (int i = 0; i < 2; ++i)
    {
        m_slotnews[i]->ShowGWnd(false);

    }
}
void CIFVItemMall::UpdateMenuSize()
{
    GetNewList(0);

    ItemMall();

    g_pCGInterface->LockMovement(13);


    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}

std::n_wstring LastCategory;

void CIFVItemMall::ActivateTabPage(BYTE page) {
    for (int i = 0; i < numberOfTabs; i++) {
        if (i == page)
            continue;

        m_pTabs[i]->sub_64CC30(0);
//        m_pTabs[i]->m_FontTexture.sub_8B4750(2);
    }

    m_pTabs[page]->sub_64CC30(1);


    Getlist(m_pTabs[page]->GetNText().c_str(), ActivedPage);

}


bool CompareByNewBestItemIndex(const std::pair<int, CustomDataManager::CustomItemMallItemStruct>& a, const std::pair<int, CustomDataManager::CustomItemMallItemStruct>& b)
{
    return a.second.NewBestItemIndex < b.second.NewBestItemIndex;
}
void CIFVItemMall::GetNewList(int test)
{
    int i = -1;
    ClearNew();
    int sayfa = 0;

    // Önce gerekli öğeleri bir vektöre kopyalayalım
    std::vector<std::pair<int, CustomDataManager::CustomItemMallItemStruct>> sortedItems;

    for (std::map<int, CustomDataManager::CustomItemMallItemStruct>::iterator it = m_CustomDataManager->CustomItemMallItemList.begin(); it != m_CustomDataManager->CustomItemMallItemList.end(); ++it) {
        if (it->second.ShowInNewBest) {
            sortedItems.push_back(*it);
        }
    }

    // Şimdi vektörü NewBestItemIndex'e göre sıralayalım
    std::sort(sortedItems.begin(), sortedItems.end(), CompareByNewBestItemIndex);


    // Sıralı öğeleri işleyelim
    for (std::vector<std::pair<int, CustomDataManager::CustomItemMallItemStruct>>::iterator it = sortedItems.begin(); it != sortedItems.end(); ++it) {
        i++;
        if (sayfa == test) {
            m_slotnews[i]->AddMarketItems(it->second.ID, it->second.ItemID, it->second.ItemCount, it->second.SilkPrice, it->second.ShowInNewBest);
            m_slotnews[i]->ShowGWnd(true);
        }
        if (i == 1) {
            sayfa++;
            i = -1;
        }
    }
}


int CIFVItemMall::newlistnum()
{
    int num = 0;
    for (std::map<int, CustomDataManager::CustomItemMallItemStruct>::iterator it = m_CustomDataManager->CustomItemMallItemList.begin(); it != m_CustomDataManager->CustomItemMallItemList.end(); ++it)
    {

        if (it->second.ShowInNewBest)
        {
            num++;
        }
    }
    return num;
}

int CIFVItemMall::newlistpagenum()
{
    int toplamurunsayısı = newlistnum();

    return  toplamurunsayısı / 2;

}
void CIFVItemMall::RightBtn(){

    if(aaa >= 0&& aaa<newlistpagenum())
    {
        aaa++;
    }
    GetNewList(aaa);
    //printf("%d \n", aaa);
}
void CIFVItemMall::LeftBtn(){
    if(aaa > 0)
    {
        aaa--;
    }
    GetNewList(aaa);
    //printf("%d \n", aaa);
}
bool CompareByType(const std::pair<int, CustomDataManager::CustomItemMallItemStruct>& a, const std::pair<int, CustomDataManager::CustomItemMallItemStruct>& b)
{
    return a.second.Type < b.second.Type;
}
bool CompareByTypeAndIndex(const std::pair<int, CustomDataManager::CustomItemMallItemStruct>& a, const std::pair<int, CustomDataManager::CustomItemMallItemStruct>& b)
{
    if (a.second.Type == b.second.Type)
    {
        return a.second.ItemIndex < b.second.ItemIndex;
    }
    return a.second.Type < b.second.Type;
}

void CIFVItemMall::Getlist(std::n_wstring testtt, byte activepage)
{
    if(m_CustomDataManager->CustomItemMallItemList.size() == 0)
        return;
    Clear();
    LastCategory = testtt;
    int i = 0;
    int testi = 0;

    if(ActivedPage == 0)
    {
        std::vector<std::pair<int, CustomDataManager::CustomItemMallItemStruct> > items(m_CustomDataManager->CustomItemMallItemList.begin(), m_CustomDataManager->CustomItemMallItemList.end());

        // Sort vector by Type
        std::sort(items.begin(), items.end(), CompareByTypeAndIndex);

        // Process sorted vector
        for (std::vector<std::pair<int, CustomDataManager::CustomItemMallItemStruct> >::iterator it = items.begin(); it != items.end(); ++it)
        {
            if (testtt == it->second.Category)
            {
                i++;
                testi++;
                if(testi == 1)
                {
                    m_slotviewer[i]->slot1->AddMarketItems(it->second.ID, it->second.ItemID, it->second.ItemCount, it->second.SilkPrice,it->second.ShowInNewBest);
                    m_scroll->AddItem(m_slotviewer[i], 1, 0);
                    m_slotviewer[i]->slot1->ShowGWnd(true);
                }
                else if(testi == 2)
                {
                    m_slotviewer[i-1]->slot2->AddMarketItems(it->second.ID, it->second.ItemID, it->second.ItemCount, it->second.SilkPrice,it->second.ShowInNewBest);
                    m_slotviewer[i-1]->slot2->ShowGWnd(true);

                }
                else if(testi == 3)
                {
                    m_slotviewer[i-2]->slot3->AddMarketItems(it->second.ID, it->second.ItemID, it->second.ItemCount, it->second.SilkPrice,it->second.ShowInNewBest);
                    m_slotviewer[i-2]->slot3->ShowGWnd(true);
                    testi = 0;
                }
            }
        }
    }
}
void CIFVItemMall::UpdateFavList()
{
    if(this->m_IRM.GetResObj(22, 1)->IsVisible())
    {
        int FullPrice = 0;
        ClearFav();
        int i = 0;
        for(std::map<int, FavItem>::iterator it = FavItemList.begin(); it != FavItemList.end(); it++)
        {
            FullPrice += it->second.Price;
            favlist[i]->AddItem(it->second.DbID, it->second.ItemID,it->second.DataItemCount, it->second.Price);
            m_scrollfav->AddItem(favlist[i], 1, 0);
            // favlist[i]->ShowGWnd(true);
            i++;
        }

        wchar_t Priceb[255];
        std::wstring test = Insert(FullPrice);
        swprintf_s(Priceb, KmtGetText(L"UIIT_KMT_TEXT_SILK"), test.c_str());

        this->m_IRM.GetResObj(22, 1)->SetText(Priceb);
    }


}
void CIFVItemMall::UpdateMarket()
{
    if(m_CustomDataManager->CustomItemMallItemList.size() == 0)
        return;
    std::map<std::n_wstring, int> Categorys;
    int i = 0;
    for (int cateid = 0; cateid < numberOfTabs; ++cateid) {
        m_pTabs[cateid]->SetText(L"");
        m_pTabs[cateid]->ShowGWnd(false);
    }

    std::vector<std::pair<int, CustomDataManager::CustomItemMallItemStruct> > items(m_CustomDataManager->CustomItemMallItemList.begin(), m_CustomDataManager->CustomItemMallItemList.end());

    // Sort vector by Type
    std::sort(items.begin(), items.end(), CompareByType);

    // Process sorted vector
    for (std::vector<std::pair<int, CustomDataManager::CustomItemMallItemStruct> >::iterator it = items.begin(); it != items.end(); ++it)
    {
        std::n_wstring Test =  it->second.Category;
        if (Categorys.find(Test) == Categorys.end())
        {

            Categorys.insert((std::make_pair(Test, 1)));
            m_pTabs[i]->SetText(it->second.Category.c_str());
            m_pTabs[i]->ShowGWnd(true);
            i++;
        }

    }
    for (int j = 6; j < numberOfTabs; ++j) {
        m_pTabs[j]->ShowGWnd(false);
    }

    // m_currentGroup'u sıfırla
    m_currentGroup = 0;
}

void CIFVItemMall::OnClick_Exit(){


}


void CIFVItemMall::Charge()
{
    ShellExecute(NULL, "open", m_CustomDataManager->WebSiteUrl.c_str(), NULL, NULL, SW_SHOWNORMAL);
}
void CIFVItemMall::ItemMall()
{
    m_IRM.GetResObj(18, 1)->TB_Func_13("clientlibrary\\mall\\mall_check_button_off.ddj", 1, 1);
    m_IRM.GetResObj(17, 1)->TB_Func_13("clientlibrary\\mall\\mall_check_button.ddj", 1, 1);
    Clear();
    UpdateMarket();
    m_scroll->ShowGWnd(true);
    ActivedPage = 0;
    ActivateTabPage(0);


    m_IRM.GetResObj(19, 1)->ShowGWnd(false);
    m_scrollfav->ShowGWnd(false);

    for(int i =0; i < 100;i++) {
        favlist[i]->ShowGWnd(false);
    }
    m_IRM.GetResObj(21, 1)->ShowGWnd(false);
    m_IRM.GetResObj(22, 1)->ShowGWnd(false);
    m_IRM.GetResObj(23, 1)->ShowGWnd(false);

    m_IRM.GetResObj(25, 1)->ShowGWnd(true);
    m_IRM.GetResObj(26, 1)->ShowGWnd(true);
    m_IRM.GetResObj(27, 1)->ShowGWnd(true);

}
void CIFVItemMall::Preset()
{
    m_IRM.GetResObj(18, 1)->TB_Func_13("clientlibrary\\mall\\mall_check_button.ddj", 1, 1);
    m_IRM.GetResObj(17, 1)->TB_Func_13("clientlibrary\\mall\\mall_check_button_off.ddj", 1, 1);


    for(int i =0; i < 150;i++) {
        m_slotviewer[i]->ShowGWnd(false);
    }
    for (int i = 0; i < numberOfTabs; i++) {
        m_pTabs[i]->ShowGWnd(false);
    }
    m_scroll->ShowGWnd(false);

    m_IRM.GetResObj(19, 1)->ShowGWnd(true);
    m_scrollfav->ShowGWnd(true);
    m_IRM.GetResObj(21, 1)->ShowGWnd(true);
    m_IRM.GetResObj(22, 1)->ShowGWnd(true);
    m_IRM.GetResObj(23, 1)->ShowGWnd(true);
    m_IRM.GetResObj(25, 1)->ShowGWnd(false);
    m_IRM.GetResObj(26, 1)->ShowGWnd(false);
    m_IRM.GetResObj(27, 1)->ShowGWnd(false);

    UpdateFavList();

}
void CIFVItemMall::OpenList(){
    /*   if(g_pCGInterface->GetGuiFromList<CIFVItemMallBuyItemList>(1355)->IsVisible())
       {

           g_pCGInterface->GetGuiFromList<CIFVItemMallBuyItemList>(1355)->ShowGWnd(false);
           CGEffSoundBody::get()->PlaySound(L"snd_window_close");
       }
       else
       {
           g_pCGInterface->GetGuiFromList<CIFVItemMallBuyItemList>(1355)->MallID = 1;
           g_pCGInterface->GetGuiFromList<CIFVItemMallBuyItemList>(1355)->UpdateMenuSize();
           g_pCGInterface->GetGuiFromList<CIFVItemMallBuyItemList>(1355)->ShowGWnd(true);

           CGEffSoundBody::get()->PlaySound(L"snd_window_open");
       }*/
}
undefined CIFVItemMall::OnCloseWnd(){

    g_pCGInterface->UnLockMovement();

    if(g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->IsVisible())
    {
        g_pCGInterface->m_IRM.GetResObj<CIFVItemMallBuyItem>(NewItemMallBuyId, 1)->ShowGWnd(false);
    }
    return CIFWnd::OnCloseWnd();
}

#define GDR_CHECKAMK 131345
void CIFVItemMall::OpenButton(int timeoutSeconds) {
    this->m_IRM.GetResObj<CIFButton>(23, 1)->SetClickable(false);
    this->StartTimer(GDR_CHECKAMK, timeoutSeconds);
}
void CIFVItemMall::OnTimer(int timerId) {
    if (timerId == GDR_CHECKAMK) {
        this->KillTimer(GDR_CHECKAMK);
        if(FavItemList.size() > 0)
        {
            TakeAll();
        }
        else
        {
            this->m_IRM.GetResObj<CIFButton>(23, 1)->SetClickable(true);

        }

    }
}
void CIFVItemMall::TakeAll(){
    if(FavItemList.size() > 0)
    {
        for(std::map<int, FavItem>::iterator it = FavItemList.begin(); it != FavItemList.end(); it++)
        {
            if(it->second.Price <= g_pCGInterface->m_silk_own)
            {
                CMsgStreamBuffer buf(0XB297);
                byte type = 11;
                buf << type;
                buf << it->second.DbID;
                buf << 1;
                FavItemList.erase(it);
                UpdateFavList();
                SendMsg(buf);
                OpenButton(1000);
                break;
            }
            else
            {
                g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_THIS_PURCHASE_CANNOT_COMPLETE_DUE_TO_INSUFFICIENT_SILKS"));
                g_pCGInterface->ShowLogMessage(0xFFDBC99B, KmtGetText(L"UIIT_KMT_THIS_PURCHASE_CANNOT_COMPLETE_DUE_TO_INSUFFICIENT_SILKS"));
                this->m_IRM.GetResObj<CIFButton>(23, 1)->SetClickable(true);
                break;
            }

        }

    }
}
