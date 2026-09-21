#include "IFTitleManager.h"
#include <Game.h>
#include <ICPlayer.h>
#include <BSLib/multibyte.h>
#include "GInterface.h"
#include "IFTitleManagerSlot.h"
#include <CharacterDependentData.h>
#include <TextStringManager.h>
#include <CustomData/CustomSettingManager.h>
#include <sstream>

#define GDR_SELECTED_ITEM_APPLY 16
#define GDR_NEXT 902
#define GDR_PREV 901
#define GDR_SPIN_TEXT_PAGE 900
GFX_IMPLEMENT_DYNCREATE(CIFTitleManager, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFTitleManager, CIFMainFrame)
                    ONG_COMMAND(100, &CIFTitleManager::OnUnknownStuff)
                    ONG_COMMAND(GDR_SELECTED_ITEM_APPLY, &CIFTitleManager::On_BtnClick)
                    ONG_COMMAND(GDR_PREV, &On_PrevBtn)
                    ONG_COMMAND(GDR_NEXT, &On_NextBtn)
GFX_END_MESSAGE_MAP()

CIFTitleManager::CIFTitleManager(void){
    m_SelectedItemId = -1;
    ActiveTabNumber = -1;
    m_CurrentIndex = 1;
    m_MaxIndex = 1;
}
CIFTitleManager::~CIFTitleManager(void){
    if (m_pTabs) {
        free(m_pTabs);
        m_pTabs = 0;
    }
}
int CIFTitleManager::Func_4(int a2) {
    int v1 = 0;
    while (a2 != v1 + 100) {
        if (++v1 >= numberOfTabs)
            return -1;
    }

    return 100;
}

void CIFTitleManager::OnUnknownStuff() {
    int id = GetCurrentEventMsgCtrlId();
    int i = 0;

    for (int i = 0; i < numberOfTabs; ++i) {
        if (id == m_pTabs[i]->UniqueID()) {
            ActivateTabPage(i);
            return;
        }
    }
}

bool CIFTitleManager::OnCreate(long ln)
{

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);


    m_IRM.LoadFromFile("clientlibrary\\resinfo\\iftitlemanager.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    this->SetText(KmtGetText(L"UIIT_KMT_TITLE_MANAGER"));
    m_IRM.GetResObj(GDR_SELECTED_ITEM_APPLY, 1)->SetText(KmtGetText(L"UIIT_KMT_CONFIRM"));
    m_pTabs = new CIFSelectableArea *[2];
    this->m_IRM.GetResObj(GDR_SPIN_TEXT_PAGE, 1)->SetText(L"1");
    for (int i = 0; i < 2; i++) {


        RECT mrect;
        mrect.top = 5;
        mrect.left = 6;
        mrect.right = 0;
        mrect.bottom = 4;

        RECT selectable_area_size;
        selectable_area_size.top = 41;
        selectable_area_size.left = tabMarginLeft + tabWidth * i;
        selectable_area_size.right = tabWidth - 4;
        selectable_area_size.bottom = tabHeight;

        m_pTabs[i] = (CIFSelectableArea *) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSelectableArea),
                                                                 selectable_area_size, tabFirstId + i, 0);

        m_pTabs[i]->SetSomeRect(mrect);
        m_pTabs[i]->SetFont(this->N00009C2F);

        m_pTabs[i]->sub_64CE30("clientlibrary\\menu\\com_long_tab_on.ddj",
                               "clientlibrary\\menu\\com_long_tab_off.ddj", "clientlibrary\\menu\\com_long_tab_disable.ddj");


        if (i) {
            switch (i) {
                case 1:
                    m_pTabs[1]->SetText(KmtGetText(L"UIIT_KMT_COLORS"));
                    break;

            }
            m_pTabs[i]->sub_64CC30(0);
        } else {
            m_pTabs[0]->SetText(KmtGetText(L"UIIT_KMT_TITLES"));
            m_pTabs[i]->sub_64CC30(1);
        }
    }

    if(!m_Settings->TitleManagerColor)
    {
        m_pTabs[1]->ShowGWnd(false);
        m_pTabs[1]->SetClickable(false);
    }

    m_IRM.GetResObj<CIFStatic>(3, 1)->SetText(KmtGetText(L"UIIT_KMT_CURRENT_TITLE"));



    m_IRM.GetResObj(8, 1)->SetText(KmtGetText(L"UIIT_KMT_AVAILABLE_TITLES"));
    for (int i = 0; i < 7; ++i) {
       m_IRM.GetResObj<CIFTitleManagerSlot>(21 + i, 1)->ShowGWnd(true);
    }

    CIFTitleManager::TitleStruct TitleStruct =  CIFTitleManager::TitleStruct();
    TitleStruct.TitleID = 0;
    TitleStruct.TitleName = KmtGetText(L"UIIT_KMT_NO_TITLE");
    TitleList.push_back(TitleStruct);

    CIFTitleManager::ColorStruct ColorStruct =  CIFTitleManager::ColorStruct();
    ColorStruct.DBID = 0;
    ColorStruct.ColorName = KmtGetText(L"UIIT_KMT_DEFAULT_COLOR");
    ColorStruct.ColorCode = 0xFFE65B; /// yellow
    ColorList.push_back(ColorStruct);

   // m_Scroll = m_IRM.GetResObj<CIFVerticalScroll>(17, 1);
    //m_Scroll->m_scrollButton->SetUniqueID(5000);

    this->ShowGWnd(false);
    return true;
}

void CIFTitleManager::UpdateMenuSize()
{
    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}


void CIFTitleManager::OnUpdate()
{
    if(g_pMyPlayerObj->m_hwanlevel != 0)
    {
        this->m_IRM.GetResObj<CIFStatic>(19, 1)->SetText(g_pMyPlayerObj->GetTitleName().c_str());
        this->m_IRM.GetResObj<CIFStatic>(19, 1)->m_FontTexture.SetColor(g_pMyPlayerObj->fonttexture_title.m_color_fg);

    }
    else
    {
        this->m_IRM.GetResObj<CIFStatic>(19, 1)->SetText(KmtGetText(L"UIIT_KMT_NO_TITLE"));
        this->m_IRM.GetResObj<CIFStatic>(19, 1)->m_FontTexture.SetColor(0xFFFFFF);
    }

}

void CIFTitleManager::ClearDDJ() {

    for (int i = 0; i < 7; ++i)
    {
        m_IRM.GetResObj<CIFTitleManagerSlot>(21 + i, 1)->ClearDDJ();
    }
    m_SelectedItemId = -1;
}
void CIFTitleManager::Clear()
{
    for (int i = 0; i < 7; ++i)
    {
        m_IRM.GetResObj<CIFTitleManagerSlot>(21 + i, 1)->Clear();
    }
}
void CIFTitleManager::On_BtnClick() {
    if(ActiveTabNumber == 0)
    {
        if(m_SelectedItemId != -1)
        {
            CMsgStreamBuffer buf(0x169B);
            buf << ActiveTabNumber << m_SelectedItemId;
            SendMsg(buf);
            ButtonDelay(3000);
        }
    }
    else if(ActiveTabNumber == 1)
    {
        if(m_SelectedItemId != -1)
        {
            CMsgStreamBuffer buf(0x169B);
            buf << ActiveTabNumber << m_SelectedItemId;
            SendMsg(buf);
            ButtonDelay(4000);
        }
    }
}
#define TIMER_BUTTON 135
void CIFTitleManager::ButtonDelay(int timeoutSeconds) {
    this->m_IRM.GetResObj<CIFButton>(GDR_SELECTED_ITEM_APPLY, 1)->SetEnabledState(0);
    this->StartTimer(TIMER_BUTTON, timeoutSeconds);
}
void CIFTitleManager::OnTimer(int timerId) {
    if (timerId == TIMER_BUTTON) {
        this->KillTimer(TIMER_BUTTON);
        this->m_IRM.GetResObj<CIFButton>(GDR_SELECTED_ITEM_APPLY, 1)->SetEnabledState(1);
    }
}
void CIFTitleManager::ActivateTabPage(BYTE page) {
    for (int i = 0; i < numberOfTabs; i++) {
        if (i == page)
            continue;

        m_pTabs[i]->sub_64CC30(0);
        m_pTabs[i]->m_FontTexture.sub_8B4750(2);
    }

    m_pTabs[page]->sub_64CC30(1);
    ActiveTabNumber = page;
    Clear();
    ClearDDJ();

    if (page == 0) {
        // Title listesi sayfa başlatma
        m_CurrentIndex = 1;
        m_MaxIndex = (TitleList.size() + 6) / 7; // Sayfa sayısını hesapla
        m_IRM.GetResObj<CIFStatic>(3, 1)->SetText(KmtGetText(L"UIIT_KMT_CURRENT_TITLE"));
        m_IRM.GetResObj(8, 1)->SetText(KmtGetText(L"UIIT_KMT_AVAILABLE_TITLES"));

        // İlk sayfayı yükle
        LoadTitlePage(m_CurrentIndex);
        UpdateText(m_CurrentIndex);

    } else if (page == 1) {
        // Color listesi sayfa başlatma
        m_CurrentIndex = 1;
        m_MaxIndex = (ColorList.size() + 6) / 7; // Sayfa sayısını hesapla
        m_IRM.GetResObj<CIFStatic>(3, 1)->SetText(KmtGetText(L"UIIT_KMT_CURRENT_COLOR"));
        m_IRM.GetResObj(8, 1)->SetText(KmtGetText(L"UIIT_KMT_AVAILABLE_COLORS"));

        // İlk sayfayı yükle
        LoadColorPage(m_CurrentIndex);
        UpdateText(m_CurrentIndex);
    }
}

// Title listesi için sayfa yükleme fonksiyonu
void CIFTitleManager::LoadTitlePage(int pageNumber) {
    int startIndex = (pageNumber - 1) * 7;
    int endIndex = startIndex + 7;
    if (TitleList.size() < 1) {
        return;  // Geçersiz sayfa numarası, yükleme yapma
    }
    if (endIndex > TitleList.size()) {
        endIndex = TitleList.size();
    }
    Clear();
    ClearDDJ();

    // Yeni öğeleri yükle
    int slotIndex = 0;
    for (int i = startIndex; i < endIndex; ++i) {
        m_IRM.GetResObj<CIFTitleManagerSlot>(20 + slotIndex + 1, 1)->
                LoadItems(TitleList[i].TitleID, TitleList[i].TitleName.c_str());

        m_IRM.GetResObj<CIFTitleManagerSlot>(20 + slotIndex + 1, 1)->ShowGWnd(true);
        ++slotIndex;
    }
}

// Color listesi için sayfa yükleme fonksiyonu
void CIFTitleManager::LoadColorPage(int pageNumber) {
    int startIndex = (pageNumber - 1) * 7;
    int endIndex = startIndex + 7;
    if (ColorList.size() < 1) {
        return;  // Geçersiz sayfa numarası, yükleme yapma
    }
    if (endIndex > ColorList.size()) {
        endIndex = ColorList.size();
    }
    Clear();
    ClearDDJ();
    // Yeni öğeleri yükle
    int slotIndex = 0;
    for (int i = startIndex; i < endIndex; ++i) {
        m_IRM.GetResObj<CIFTitleManagerSlot>(20 + slotIndex + 1, 1)->
                LoadItemsColors(ColorList[i].DBID, ColorList[i].ColorName.c_str(), ColorList[i].ColorCode);
        m_IRM.GetResObj<CIFTitleManagerSlot>(20 + slotIndex + 1, 1)->ShowGWnd(true);
        ++slotIndex;
    }
}

void CIFTitleManager::UpdateText(int Number)
{
    wchar_t buffer1[255];
    swprintf_s(buffer1, L"%d", Number);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SPIN_TEXT_PAGE, 1)->SetText(buffer1);
}
void CIFTitleManager::On_NextBtn() {
    if (m_CurrentIndex < m_MaxIndex) {
        m_CurrentIndex++;
        if (ActiveTabNumber == 0) {
            LoadTitlePage(m_CurrentIndex);
            UpdateText(m_CurrentIndex);
        } else if (ActiveTabNumber == 1) {
            LoadColorPage(m_CurrentIndex);
            UpdateText(m_CurrentIndex);
        }
    }
}

void CIFTitleManager::On_PrevBtn() {
    if (m_CurrentIndex > 1) {
        m_CurrentIndex--;
        if (ActiveTabNumber == 0) {
            LoadTitlePage(m_CurrentIndex);
            UpdateText(m_CurrentIndex);
        } else if (ActiveTabNumber == 1) {
            LoadColorPage(m_CurrentIndex);
            UpdateText(m_CurrentIndex);
        }
    }
}
