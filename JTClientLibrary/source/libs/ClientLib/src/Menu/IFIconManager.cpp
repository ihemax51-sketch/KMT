#include "IFIconManager.h"
#include <Game.h>
#include <ICPlayer.h>
#include <BSLib/multibyte.h>
#include "GInterface.h"
#include <CharacterDependentData.h>
#include <TextStringManager.h>
#include <CustomData/CustomDataManager.h>
#include <CustomData/CustomSettingManager.h>



#define GDR_SELECTED_ITEM_APPLY 16
#define GDR_REMOVE_LEFT_ICON 26
#define GDR_REMOVE_RIGHT_ICON 27

#define GDR_NEXT 902
#define GDR_PREV 901
#define GDR_SPIN_TEXT_PAGE 900
GFX_IMPLEMENT_DYNCREATE(CIFIconManager, CIFMainFrame)
GFX_BEGIN_MESSAGE_MAP(CIFIconManager, CIFMainFrame)
                    ONG_COMMAND(GDR_SELECTED_ITEM_APPLY, &On_BtnClick)
                    ONG_COMMAND(GDR_PREV, &On_PrevBtn)
                    ONG_COMMAND(GDR_NEXT, &On_NextBtn)
GFX_END_MESSAGE_MAP()

CIFIconManager::CIFIconManager(void){
    IconList = std::vector<IconStruct>();
    m_CurrentIndex = 1;
    m_MaxIndex = 1;
}
CIFIconManager::~CIFIconManager(void){
}


bool CIFIconManager::OnCreate(long ln)
{

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);

    this->m_IRM.LoadFromFile("clientlibrary\\resinfo\\ificonmanager.txt");
    this->m_IRM.CreateInterfaceSection("Create", this);
    this->SetText(KmtGetText(L"UIIT_KMT_ICON_MANAGER"));

//    m_scroll = this->m_IRM.GetResObj<CIFVerticalScroll>(17, 1);



    this->m_IRM.GetResObj<CIFStatic>(24, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj(23, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStretchWnd>(22, 1)->ShowGWnd(false);


    if(m_Settings->IconManagerRight)
    {
        this->m_IRM.GetResObj<CIFStatic>(24, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj(23, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj<CIFStretchWnd>(22, 1)->ShowGWnd(true);

    }
    this->m_IRM.GetResObj<CIFStatic>(90, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(91, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(92, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(93, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(94, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(95, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(96, 1)->ShowGWnd(false);
    if(m_Settings->IconManagerRight)
    {
        this->m_IRM.GetResObj<CIFStatic>(90, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj<CIFStatic>(91, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj<CIFStatic>(92, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj<CIFStatic>(93, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj<CIFStatic>(94, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj<CIFStatic>(95, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj<CIFStatic>(96, 1)->ShowGWnd(true);
        this->m_IRM.GetResObj<CIFStatic>(90, 1)->BringToFront();
        this->m_IRM.GetResObj<CIFStatic>(91, 1)->BringToFront();
        this->m_IRM.GetResObj<CIFStatic>(92, 1)->BringToFront();
        this->m_IRM.GetResObj<CIFStatic>(93, 1)->BringToFront();
        this->m_IRM.GetResObj<CIFStatic>(94, 1)->BringToFront();
        this->m_IRM.GetResObj<CIFStatic>(95, 1)->BringToFront();
        this->m_IRM.GetResObj<CIFStatic>(96, 1)->BringToFront();
    }

    this->m_IRM.GetResObj(8, 1)->SetText(KmtGetText(L"UIIT_KMT_AVAILABLE_ICONS"));


    this->m_IRM.GetResObj(GDR_SPIN_TEXT_PAGE, 1)->SetText(L"1");

    this->m_IRM.GetResObj(16, 1)->SetText(KmtGetText(L"UIIT_KMT_CONFIRM"));
    // Önceki öğeleri temizle
    for (int i = 0; i < 7; ++i) {
        m_IRM.GetResObj<CIFIconManagerSlot>(30 + i + 1, 1)->ShowGWnd(true);
    }


    CIFIconManager::IconStruct IconStruct =  CIFIconManager::IconStruct();
    IconStruct.DBID = 0;
    IconStruct.side = 0;
    IconStruct.MediaPath = L"<No Icon>";
    IconList.push_back(IconStruct);

    if(m_Settings->IconManagerRight)
    {
        CIFIconManager::IconStruct IconStruct2 =  CIFIconManager::IconStruct();
        IconStruct2.DBID = 0;
        IconStruct2.side = 1;
        IconStruct2.MediaPath = L"<No Icon>";
        IconList.push_back(IconStruct2);
    }


    this->ShowGWnd(false);
    return true;
}

void CIFIconManager::UpdateMenuSize()
{

    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}


void CIFIconManager::OnUpdate()
{
    this->m_IRM.GetResObj<CIFStatic>(19, 1)->SetText(g_pMyPlayerObj->GetCharName().c_str());
    if(m_CustomDataManager->m_LeftCharIcons.find(g_pMyPlayerObj->GetCharName()) != m_CustomDataManager->m_LeftCharIcons.end())
    {
        std::map<int, std::n_string>::iterator foundedTheIcon = m_CustomDataManager->MediaIcons.find(m_CustomDataManager->m_LeftCharIcons[g_pMyPlayerObj->GetCharName()]);
        if (foundedTheIcon != m_CustomDataManager->MediaIcons.end()) {
            this->m_IRM.GetResObj<CIFStatic>(25, 1)->TB_Func_13(m_CustomDataManager->MediaIcons[m_CustomDataManager->m_LeftCharIcons[g_pMyPlayerObj->GetCharName()]], 1, 1);
        }
    }
    else
    {
        this->m_IRM.GetResObj<CIFStatic>(25, 1)->TB_Func_13("", 1, 1);
    }
    if(m_CustomDataManager->m_RightCharIcons.find(g_pMyPlayerObj->GetCharName()) != m_CustomDataManager->m_RightCharIcons.end())
    {
        std::map<int, std::n_string>::iterator foundedTheIcon = m_CustomDataManager->MediaIcons.find(m_CustomDataManager->m_RightCharIcons[g_pMyPlayerObj->GetCharName()]);
        if (foundedTheIcon != m_CustomDataManager->MediaIcons.end()) {
            this->m_IRM.GetResObj<CIFStatic>(24, 1)->TB_Func_13(m_CustomDataManager->MediaIcons[m_CustomDataManager->m_RightCharIcons[g_pMyPlayerObj->GetCharName()]], 1, 1);
        }
    }
    else
    {
        this->m_IRM.GetResObj<CIFStatic>(24, 1)->TB_Func_13("", 1, 1);
    }
}
void CIFIconManager::Clear()
{
    for (int i = 0; i < 7; ++i)
    {
        m_IRM.GetResObj<CIFIconManagerSlot>(31 + i, 1)->Clear();
    }
}
void CIFIconManager::ClearDDJ() {
    for (int i = 0; i < 7; ++i)
    {
        m_IRM.GetResObj<CIFIconManagerSlot>(31 + i, 1)->ClearDDJ();
    }
    m_SelectedItemId = -1;
    m_SelectedSide = -1;
    //selectedlabel->TB_Func_13("",0,0);
    //selectedlabel->SetText(L"");
}/*
int CIFIconManager::Func_36(int a1, short action, int a3, int a4) {
    if (action < 0) { // Aşağı kaydırma
        UpdateForScrollDown();
    } else if (action > 0) { // Yukarı kaydırma

        UpdateForScrollUp();
    }
    return 1;
}*/

void CIFIconManager::LoadItems()
{

    Clear();
    ClearDDJ();

    m_CurrentIndex = 1;  // Başlangıç sayfası
    m_MaxIndex = (IconList.size() + 6) / 7;  // Toplam sayfa sayısı, 7'ye tam bölünmeyenler için +1

    // İlk sayfayı yükle
    LoadPage(m_CurrentIndex);
    UpdateText(m_CurrentIndex);

}
void CIFIconManager::UpdateText(int Number)
{
    wchar_t buffer1[255];
    swprintf_s(buffer1, L"%d", Number);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SPIN_TEXT_PAGE, 1)->SetText(buffer1);
}
void CIFIconManager::On_BtnClick() {
    if(m_SelectedItemId != -1 && m_SelectedSide != -1)
    {
        CMsgStreamBuffer buf(0x169C);
        buf << m_SelectedSide << m_SelectedItemId;
        SendMsg(buf);
        ButtonDelay(4000);
    }
}

#define TIMER_BUTTON 135
void CIFIconManager::ButtonDelay(int timeoutSeconds) {
    this->m_IRM.GetResObj<CIFButton>(GDR_SELECTED_ITEM_APPLY, 1)->SetEnabledState(0);
    this->StartTimer(TIMER_BUTTON, timeoutSeconds);
}
void CIFIconManager::OnTimer(int timerId) {
    if (timerId == TIMER_BUTTON) {
        this->KillTimer(TIMER_BUTTON);
        this->m_IRM.GetResObj<CIFButton>(GDR_SELECTED_ITEM_APPLY, 1)->SetEnabledState(1);
    }
}
void CIFIconManager::LoadPage(int pageNumber) {
    int startIndex = (pageNumber - 1) * 7;  // Sayfada gösterilecek ilk öğenin indeksi
    int endIndex = startIndex + 7;  // Sayfada gösterilecek son öğenin indeksi

    if (IconList.size() < 1) {
        return;  // Geçersiz sayfa numarası, yükleme yapma
    }

    Clear();
    ClearDDJ();
    // Eğer liste sınırlarını aşıyorsa, son indeksi liste boyutuyla sınırla
    if (endIndex > IconList.size()) {
        endIndex = IconList.size();
    }

    // Yeni öğeleri yükle
    int slotIndex = 0;
    for (int i = startIndex; i < endIndex; ++i) {
        m_IRM.GetResObj<CIFIconManagerSlot>(30 + slotIndex + 1, 1)->
                LoadItems(IconList[i].DBID, IconList[i].MediaPath.c_str(), IconList[i].side);
        m_IRM.GetResObj<CIFIconManagerSlot>(30 + slotIndex + 1, 1)->ShowGWnd(true);
        ++slotIndex;
    }
}

void CIFIconManager::On_NextBtn() {
    if (m_CurrentIndex < m_MaxIndex) {
        m_CurrentIndex++;  // Sonraki sayfaya geç
        LoadPage(m_CurrentIndex);  // Yeni sayfa öğelerini yükle
        UpdateText(m_CurrentIndex);  // Sayfa numarasını güncelle
    }
}

void CIFIconManager::On_PrevBtn() {
    if (m_CurrentIndex > 1) {
        m_CurrentIndex--;  // Önceki sayfaya dön
        LoadPage(m_CurrentIndex);  // Önceki sayfa öğelerini yükle
        UpdateText(m_CurrentIndex);  // Sayfa numarasını güncelle
    }
}
