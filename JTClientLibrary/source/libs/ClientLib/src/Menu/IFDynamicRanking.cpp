#include "IFDynamicRanking.h"
#include <Game.h>
#include <ICPlayer.h>
#include <BSLib/multibyte.h>
#include "GInterface.h"
#include "IFDynamicRankingSlot.h"
#include <CharacterDependentData.h>
#include <TextStringManager.h>
#include <GlobalDataManager.h>



#define GDR_NEXT 902
#define GDR_PREV 901
#define GDR_SPIN_TEXT_PAGE 900
#define GDR_CATEGORY_TAB_COMMAND 100
#define GDR_CATEGORY_TAB_TIMER 13134

GFX_IMPLEMENT_DYNCREATE(CIFDynamicRanking, CIFMainFrame)

GFX_BEGIN_MESSAGE_MAP(CIFDynamicRanking, CIFMainFrame)
                    ONG_COMMAND(GDR_CATEGORY_TAB_COMMAND, &OnCategoryTab)
                    ONG_COMMAND(GDR_PREV, &On_PrevBtn)
                    ONG_COMMAND(GDR_NEXT, &On_NextBtn)
GFX_END_MESSAGE_MAP()

CIFDynamicRanking::CIFDynamicRanking(void){
    m_pCategoryTabs = 0;
    m_RankCategories = std::vector<RankCategory>();
    m_SelectedCategoryIndex = -1;
    m_CategoryRequestLocked = false;
    RankList = std::vector<RankStruct>();
    m_CurrentIndex = 1;
    m_MaxIndex = 1;
}
CIFDynamicRanking::~CIFDynamicRanking(void){
    if (m_pCategoryTabs) {
        delete[] m_pCategoryTabs;
        m_pCategoryTabs = 0;
    }
}

bool CIFDynamicRanking::OnCreate(long ln)
{

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);


    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifdynamicranking.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    this->SetText(KmtGetText(L"UIIT_KMT_DYNAMIC_RANKING"));

    CIFWnd* guildHeader = this->m_IRM.GetResObj(21, 1);
    CIFWnd* nameHeader = this->m_IRM.GetResObj(28, 1);
    CIFWnd* numberHeader = this->m_IRM.GetResObj(9, 1);
    CIFWnd* pointsHeader = this->m_IRM.GetResObj(22, 1);
    if (guildHeader) guildHeader->SetText(KmtGetText(L"UIIT_KMT_GUILD"));
    if (nameHeader) nameHeader->SetText(KmtGetText(L"UIIT_KMT_NAME"));
    if (numberHeader) numberHeader->SetText(KmtGetText(L"UIIT_KMT_NO"));
    if (pointsHeader) pointsHeader->SetText(KmtGetText(L"UIIT_KMT_POINTS"));

    this->m_IRM.GetResObj<CIFStatic>(GDR_SPIN_TEXT_PAGE, 1)->SetText(L"1");
    this->m_IRM.GetResObj<CIFStatic>(31, 1)->m_FontTexture.SetColor(0x00FF00);
    this->m_IRM.GetResObj<CIFStatic>(32, 1)->m_FontTexture.SetColor(0x00FF00);
    this->m_IRM.GetResObj<CIFStatic>(33, 1)->m_FontTexture.SetColor(0x00FF00);
    this->m_IRM.GetResObj<CIFStatic>(34, 1)->m_FontTexture.SetColor(0x00FF00);
    this->m_IRM.GetResObj<CIFStatic>(31, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(32, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(33, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(34, 1)->ShowGWnd(false);

    m_pCategoryTabs = new CIFSelectableArea*[MaxCategoryTabs];
    for (int i = 0; i < MaxCategoryTabs; ++i)
    {
        RECT tabRect;
        tabRect.top = 35;
        tabRect.left = 17;
        tabRect.right = 88;
        tabRect.bottom = 24;

        m_pCategoryTabs[i] = (CIFSelectableArea*)CGWnd::CreateInstance(
                this, GFX_RUNTIME_CLASS(CIFSelectableArea), tabRect,
                CategoryTabFirstId + i, 0);
        if (!m_pCategoryTabs[i])
            continue;

        RECT textRect;
        textRect.top = 5;
        textRect.left = 6;
        textRect.right = 0;
        textRect.bottom = 4;
        m_pCategoryTabs[i]->SetSomeRect(textRect);
        m_pCategoryTabs[i]->SetFont(this->N00009C2F);
        m_pCategoryTabs[i]->sub_64CE30(
                "interface\\ifcommon\\com_long_tab_on.ddj",
                "interface\\ifcommon\\com_long_tab_off.ddj",
                "interface\\ifcommon\\com_long_tab_disable.ddj");
        m_pCategoryTabs[i]->sub_64CC30(0);
        m_pCategoryTabs[i]->SetClickable(false);
        m_pCategoryTabs[i]->ShowGWnd(false);
    }

    for (int i = 0; i < 10; ++i)
    {
        m_IRM.GetResObj<CIFDynamicRankingSlot>(50 + i, 1)->ShowGWnd(true);
    }
    this->ShowGWnd(false);
    return true;
}

int CIFDynamicRanking::Func_4(int controlId)
{
    if (controlId >= CategoryTabFirstId &&
        controlId < CategoryTabFirstId + MaxCategoryTabs)
        return GDR_CATEGORY_TAB_COMMAND;

    return -1;
}

std::n_wstring CIFDynamicRanking::GetCategoryTabText(
        const std::n_wstring& text, int tabWidth) const
{
    int maxCharacters = (tabWidth - 16) / 7;
    if (maxCharacters < 4)
        maxCharacters = 4;
    if ((int)text.length() <= maxCharacters)
        return text;

    return text.substr(0, maxCharacters - 3) + L"...";
}

void CIFDynamicRanking::LayoutCategoryTabs()
{
    // Match the native Dynamic Ranking reference: compact fixed-width tabs,
    // left aligned above the table instead of stretched across the window.
    const int tabLeft = 18;
    const int tabWidth = 70;
    const int tabGap = 4;
    const int maxTabsPerRow = 6;
    const int tabHeight = 24;
    const int categoryCount = (int)m_RankCategories.size();
    const bool hasSecondRow = categoryCount > maxTabsPerRow;
    const int rowTop[2] = { hasSecondRow ? 35 : 47, 59 };

    // Keep the original resinfo layout: both tab rows fit above its table.
    // Older media still contains the retired search button and label.
    const int retiredIds[] = { 3, 23, 6 };
    for (int i = 0; i < 3; ++i) {
        CIFWnd* control = m_IRM.GetResObj(retiredIds[i], 1);
        if (control) {
            control->ShowGWnd(false);
            control->SetClickable(false);
        }
    }
    const int decorationIds[] = { 1, 2, 7 };
    for (int i = 0; i < 3; ++i) {
        CIFWnd* control = m_IRM.GetResObj(decorationIds[i], 1);
        if (control) control->SetClickable(false);
    }
    if (!m_pCategoryTabs)
        return;

    for (int row = 0; row < 2; ++row)
    {
        const int firstIndex = row * maxTabsPerRow;
        int countInRow = categoryCount - firstIndex;
        if (countInRow <= 0)
            continue;
        if (countInRow > maxTabsPerRow)
            countInRow = maxTabsPerRow;

        for (int column = 0; column < countInRow; ++column)
        {
            const int index = firstIndex + column;
            CIFSelectableArea* tab = m_pCategoryTabs[index];
            if (!tab)
                continue;

            tab->SetGWndSize(tabWidth, tabHeight);
            tab->MoveGWnd(GetPos().x + tabLeft + column * (tabWidth + tabGap),
                          GetPos().y + rowTop[row]);
            const std::n_wstring label =
                    GetCategoryTabText(m_RankCategories[index].Name, tabWidth);
            tab->SetText(label.c_str());
            tab->SetTooltip(m_RankCategories[index].Name);
            // SetStyleThingy replaces the native flags; retain the tab's flags.
            unsigned int style = TOOLTIP;
            for (unsigned int bit = 1; bit != 0; bit <<= 1) {
                if (tab->IsStyleThingy(static_cast<StyleOptions>(bit)))
                    style |= bit;
            }
            tab->SetStyleThingy(static_cast<StyleOptions>(style));
            tab->SetClickable(!m_CategoryRequestLocked);
            tab->ShowGWnd(true);
            tab->BringToFront();
        }
    }
}

void CIFDynamicRanking::ClearCategories()
{
    KillTimer(GDR_CATEGORY_TAB_TIMER);
    m_RankCategories.clear();
    m_SelectedCategoryIndex = -1;
    m_CategoryRequestLocked = false;

    if (!m_pCategoryTabs)
        return;

    for (int i = 0; i < MaxCategoryTabs; ++i)
    {
        if (!m_pCategoryTabs[i])
            continue;
        m_pCategoryTabs[i]->SetText(L"");
        m_pCategoryTabs[i]->sub_64CC30(0);
        m_pCategoryTabs[i]->SetClickable(false);
        m_pCategoryTabs[i]->ShowGWnd(false);
    }
}

void CIFDynamicRanking::AddCategory(int id, const std::n_wstring& name)
{
    if (id < 1 || id > 9 || name.empty() ||
        m_RankCategories.size() >= MaxCategoryTabs)
        return;

    RankCategory category;
    category.Id = id;
    category.Name = name;
    m_RankCategories.push_back(category);
}

void CIFDynamicRanking::FinishCategories()
{
    LayoutCategoryTabs();
    if (!m_RankCategories.empty())
        ActivateCategory(0, true);
}

void CIFDynamicRanking::SetCategoryTabsEnabled(bool enabled)
{
    if (!m_pCategoryTabs)
        return;

    for (size_t i = 0; i < m_RankCategories.size(); ++i)
    {
        if (m_pCategoryTabs[i])
            m_pCategoryTabs[i]->SetClickable(enabled);
    }
}

void CIFDynamicRanking::ActivateCategory(int index, bool requestRanks)
{
    if (index < 0 || index >= (int)m_RankCategories.size() ||
        !m_pCategoryTabs || m_CategoryRequestLocked)
        return;

    for (size_t i = 0; i < m_RankCategories.size(); ++i)
    {
        if (m_pCategoryTabs[i])
            m_pCategoryTabs[i]->sub_64CC30((int)i == index ? 1 : 0);
    }
    m_SelectedCategoryIndex = index;

    if (requestRanks)
    {
        ResetData();
        LockCategoryTabs(5000);
        SendPacket((byte)m_RankCategories[index].Id);
    }
}

void CIFDynamicRanking::OnCategoryTab()
{
    const int selectedIndex = GetCurrentEventMsgCtrlId() - CategoryTabFirstId;
    if (selectedIndex == m_SelectedCategoryIndex)
        return;
    ActivateCategory(selectedIndex, true);
}

void CIFDynamicRanking::UpdateMenuSize()
{

    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    LayoutCategoryTabs();
    BringToFront();
}


void CIFDynamicRanking::OnUpdate()
{

}
void CIFDynamicRanking::Hide()
{
    this->m_IRM.GetResObj<CIFStatic>(31, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(32, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(33, 1)->ShowGWnd(false);
    this->m_IRM.GetResObj<CIFStatic>(34, 1)->ShowGWnd(false);
}
void CIFDynamicRanking::Clear()
{
    for (int i = 0; i < 10; ++i)
    {
        m_IRM.GetResObj<CIFDynamicRankingSlot>(50 + i, 1)->Clear();
    }
    UpdateSelfRank(L"empty", 0, -1);
}

void CIFDynamicRanking::ResetData()
{
    RankList.clear();
    m_CurrentIndex = 1;
    m_MaxIndex = 1;
    Clear();
    UpdateText(1);
}

void CIFDynamicRanking::UpdateRanks() {
    Clear();

    m_CurrentIndex = 1;  // Başlangıç sayfası
    m_MaxIndex = (RankList.size() + 9) / 10;  // Toplam sayfa sayısı, 7'ye tam bölünmeyenler için +1

    // İlk sayfayı yükle
    LoadPage(m_CurrentIndex);
    UpdateText(m_CurrentIndex);
    // The list response completes a request even when there is no own-rank row.
    OnTimer(GDR_CATEGORY_TAB_TIMER);
}
void CIFDynamicRanking::LoadPage(int pageNumber) {
    int startIndex = (pageNumber - 1) * 10;  // Sayfada gösterilecek ilk öğenin indeksi
    int endIndex = startIndex + 10;          // Sayfada gösterilecek son öğenin indeksi

    Clear();  // Önceki verileri temizle

    // Eğer liste sınırlarını aşıyorsa, son indeksi liste boyutuyla sınırla
    if (endIndex > RankList.size()) {
        endIndex = RankList.size();
    }
    // Yeni öğeleri yükle
    for (int i = startIndex; i < endIndex; ++i) {
        // Burada slotIndex yerine i'yi doğrudan kullanıyoruz ki doğru sıradan başlasın.
        int slotNumber = i - startIndex + 1; // Slot numarası 1'den başlamalı (sayfadaki sıralama)

        m_IRM.GetResObj<CIFDynamicRankingSlot>(49 + slotNumber, 1)->
                SetName(i + 1, RankList[i].Charname.c_str(), RankList[i].Guild.c_str(), RankList[i].Points.c_str()); // Sıralamaya göre numara
        m_IRM.GetResObj<CIFDynamicRankingSlot>(49 + slotNumber, 1)->ShowGWnd(true);

        // Eğer karakter oyuncunun karakteriyse işaretle
        if (RankList[i].Charname == g_pMyPlayerObj->GetCharName().c_str()) {
            m_IRM.GetResObj<CIFDynamicRankingSlot>(49 + slotNumber, 1)->NewUpdate();
        }
    }
}


void CIFDynamicRanking::UpdateText(int Number)
{
    wchar_t buffer1[255];
    swprintf_s(buffer1, L"%d", Number);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SPIN_TEXT_PAGE, 1)->SetText(buffer1);
}
void CIFDynamicRanking::On_NextBtn() {
    int totalPages = (RankList.size() + 9) / 10;  // Toplam sayfa sayısı
    if (m_CurrentIndex < totalPages) {
        m_CurrentIndex++;
        LoadPage(m_CurrentIndex);  // Yeni sayfayı yükle
        UpdateText(m_CurrentIndex);
    }
}

void CIFDynamicRanking::On_PrevBtn() {
    if (m_CurrentIndex > 1) {
        m_CurrentIndex--;
        LoadPage(m_CurrentIndex);  // Önceki sayfayı yükle
        UpdateText(m_CurrentIndex);
    }
}


void CIFDynamicRanking::UpdateSelfRank(std::n_wstring Name, int No, int Point)
{
    this->m_IRM.GetResObj<CIFStatic>(31, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(32, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(33, 1)->ShowGWnd(true);
    this->m_IRM.GetResObj<CIFStatic>(34, 1)->ShowGWnd(true);

    if(No != 0)
    {
        wchar_t buffer1[255];
        swprintf_s(buffer1, L"%d", No);

        this->m_IRM.GetResObj<CIFStatic>(31, 1)->SetText(buffer1);
    }
    else
    {
        this->m_IRM.GetResObj<CIFStatic>(31, 1)->SetText(L"0");
    }

    if(Name != L"empty")
    {
        this->m_IRM.GetResObj<CIFStatic>(32, 1)->SetText(Name.c_str());
    }
    else
    {
        this->m_IRM.GetResObj<CIFStatic>(32, 1)->SetText(g_pMyPlayerObj->GetCharName().c_str());
    }

    if(g_pMyPlayerObj->GetGuildName().empty())
    {
        this->m_IRM.GetResObj<CIFStatic>(33, 1)->SetText(KmtGetText(L"UIIT_KMT_NO_GUILD"));
    }
    else
    {
        this->m_IRM.GetResObj<CIFStatic>(33, 1)->SetText(g_pMyPlayerObj->GetGuildName().c_str());
    }

    if(Point != -1)
    {
        std::wstring MyPoints = Insert(Point);
        this->m_IRM.GetResObj<CIFStatic>(34, 1)->SetText(MyPoints.c_str());
    }
    else
    {
        this->m_IRM.GetResObj<CIFStatic>(34, 1)->SetText(KmtGetText(L"UIIT_KMT_NONE_A06007EB"));
    }

}
void CIFDynamicRanking::LockCategoryTabs(int timeoutMilliseconds) {
    m_CategoryRequestLocked = true;
    SetCategoryTabsEnabled(false);
    this->StartTimer(GDR_CATEGORY_TAB_TIMER, timeoutMilliseconds);
}

void CIFDynamicRanking::OnTimer(int timerId) {
    if (timerId == GDR_CATEGORY_TAB_TIMER) {
        this->KillTimer(GDR_CATEGORY_TAB_TIMER);
        m_CategoryRequestLocked = false;
        SetCategoryTabsEnabled(true);
    }
}
void CIFDynamicRanking::SendPacket(byte type)
{
    CMsgStreamBuffer buf(0x180A);
    buf << type;
    SendMsg(buf);
}
