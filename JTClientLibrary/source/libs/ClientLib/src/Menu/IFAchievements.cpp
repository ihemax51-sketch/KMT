#include "IFAchievements.h"
#include <Game.h>
#include <ICPlayer.h>
#include <BSLib/multibyte.h>
#include "GInterface.h"
#include <CharacterDependentData.h>
#include <TextStringManager.h>
#include <GlobalDataManager.h>
#include <CustomData/CustomDataManager.h>
#include <CustomData/CustomCICPlayer.h>



#define GDR_BUTTON_USE 23
#define GDR_BUTTON_ALL 112
#define GDR_BUTTON_GENERAL 113
#define GDR_BUTTON_QUEST 114
#define GDR_BUTTON_UNIQUE 115
#define GDR_BUTTON_MATCH 116
#define GDR_BUTTON_EVENT 117
#define GDR_BUTTON_REMOVE 24
GFX_IMPLEMENT_DYNCREATE(CIFAchievements, CIFMainFrame)

GFX_BEGIN_MESSAGE_MAP(CIFAchievements, CIFMainFrame)
                    ONG_COMMAND(100, &CIFAchievements::OnUnknownStuff)
                    ONG_COMMAND(GDR_BUTTON_USE, &On_BtnClick)
                    ONG_COMMAND(GDR_BUTTON_ALL, &On_BtnClickAll)
                    ONG_COMMAND(GDR_BUTTON_GENERAL, &On_BtnClickGeneral)
                    ONG_COMMAND(GDR_BUTTON_QUEST, &On_BtnClickQuest)
                    ONG_COMMAND(GDR_BUTTON_UNIQUE, &On_BtnClickUnique)
                    ONG_COMMAND(GDR_BUTTON_MATCH, &On_BtnClickMatch)
                    ONG_COMMAND(GDR_BUTTON_EVENT, &On_BtnClickEvent)
                    ONG_COMMAND(GDR_BUTTON_REMOVE, &On_BtnClickRemove)

GFX_END_MESSAGE_MAP()

CIFAchievements::CIFAchievements(void){
    vscroll = 0;
    m_scroll = 0;
    descbox = 0;
    SelectedItemID = 0;
    ActiveTabNumber = -1;
    LastActiveCategory = -1;
    for (int i = 0; i < numberOfTabs; ++i)
        m_pTabs[i] = 0;
}
CIFAchievements::~CIFAchievements(void){
}

int CIFAchievements::Func_4(int a2) {
    int v1 = 0;
    while (a2 != v1 + 100) {
        if (++v1 >= 5)
            return -1;
    }

    return 100;
}

int CIFAchievements::Func_36(int a1, short action, int a3, int a4) {
    if (!vscroll)
        return 0;

    if (action <= 0) {
        if (action < 0) {
            vscroll->sub_65A5C0(0);
        }
    } else {
        vscroll->sub_65A5A0(0);
    }

    return 1;
}

bool CIFAchievements::OnCreate(long ln)
{

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);
    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifachievements.txt");
    m_IRM.CreateInterfaceSection("Create", this);

    const int decorativeIds[] = {1, 2, 4, 5, 8, 9, 10, 18, 19, 21, 118, 119};
    for (int decorativeIndex = 0;
         decorativeIndex < sizeof(decorativeIds) / sizeof(decorativeIds[0]);
         ++decorativeIndex) {
        CIFWnd* decorative = m_IRM.GetResObj<CIFWnd>(decorativeIds[decorativeIndex], 1);
        if (decorative)
            decorative->SetClickable(false);
    }

    for (int i = 0; i < numberOfTabs; i++) {


        RECT mrect;
        mrect.top = 5;
        mrect.left = 6;
        mrect.right = 0;
        mrect.bottom = 4;

        RECT selectable_area_size;
        selectable_area_size.top = 44;
        selectable_area_size.left = tabMarginLeft + tabWidth * i;
        selectable_area_size.right = tabWidth - 4;
        selectable_area_size.bottom = tabHeight;

        m_pTabs[i] = (CIFSelectableArea *) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSelectableArea),
                                                                 selectable_area_size, tabFirstId + i, 0);

        if (!m_pTabs[i])
            return false;

        m_pTabs[i]->SetSomeRect(mrect);
        m_pTabs[i]->SetFont(this->N00009C2F);

        m_pTabs[i]->sub_64CE30("interface\\ifcommon\\com_long_tab_on.ddj",
                               "interface\\ifcommon\\com_long_tab_off.ddj",
                               "interface\\ifcommon\\com_long_tab_disable.ddj");

        if (i) {
            switch (i) {
                case 1:
                    std::n_wstring msg = KmtGetText(L"UIIT_KMT_ACHIEVEMENTS");
                    m_pTabs[1]->SetText(msg.c_str());
                    m_pTabs[1]->SetTooltip(msg);
                    m_pTabs[1]->SetStyleThingy(TOOLTIP);
                    break;

            }
            m_pTabs[i]->sub_64CC30(0);
        } else {
            m_pTabs[0]->SetText(KmtGetText(L"UIIT_KMT_TITLES"));


            m_pTabs[i]->sub_64CC30(1);
        }
    }
    std::n_wstring msg = KmtGetText(L"UIIT_KMT_ACHIEVEMENTS");
    m_pTabs[1]->SetText(msg.c_str());
    m_pTabs[1]->SetTooltip(msg);
    m_pTabs[1]->SetStyleThingy(TOOLTIP);


    std::n_wstring category = KmtGetText(L"UIIT_KMT_CATEGORY");
    CIFWnd* categoryHeader = m_IRM.GetResObj(9, 1);
    CIFWnd* nameHeader = m_IRM.GetResObj(10, 1);
    if (!categoryHeader || !nameHeader)
        return false;
    categoryHeader->SetText(KmtGetText(L"UIIT_KMT_TYPE"));
    categoryHeader->SetTooltip(category);
    categoryHeader->SetStyleThingy(TOOLTIP);



    nameHeader->SetText(KmtGetText(L"UIIT_KMT_TITLE_NAME"));
    CIFStatic* currentItem = m_IRM.GetResObj<CIFStatic>(3, 1);
    if (!currentItem || !m_IRM.GetResObj(4, 1) || !m_IRM.GetResObj(5, 1) ||
        !m_IRM.GetResObj(7, 1) || !m_IRM.GetResObj(118, 1) ||
        !m_IRM.GetResObj(119, 1))
        return false;
    currentItem->SetText(KmtGetText(L"UIIT_KMT_CURRENT_TITLE"));


    m_scroll = this->m_IRM.GetResObj<CIFScrollManager>(11, 1);
    if (!m_scroll)
        return false;
    m_scroll->sub_008124F0(0);
    m_scroll->sub_008124C0(62);
    m_scroll->sub_008123F0(3);
    m_scroll->sub_00812500(0);
    m_scroll->sub_00812420(-8, 0);

    if (!EnsureSlotCount(12))
        return false;


    descbox = m_IRM.GetResObj<CIFTextBox>(25, 1);
    if (!descbox)
        return false;
    descbox->JustifyHorizontal(JUSTIFY_LEFT);
    descbox->JustifyVertical(JUSTIFY_MIDDLE);
    descbox->N00000608 = this;

    vscroll = this->m_IRM.GetResObj<CIFVerticalScroll>(22, 1);
    if (!vscroll)
        return false;

    descbox->LinkScrollbar(vscroll);
    descbox->m_LinesOfHistory = 500;
    descbox->m_HeightInLines = 3;
    descbox->SetLineHeight(15);
    descbox->sub_638B50(0);
    descbox->sub_64E380(0);
    descbox->sub_638C70(16);
    descbox->sub_638D50(1);
    descbox->sub_638D40(1);
    descbox->ShowGWnd(true);
    descbox->m_FontTexture.sub_8B4400(0, 0);
    descbox->m_FontTexture.sub_8B4750(0);


    CIFButton* allButton = m_IRM.GetResObj<CIFButton>(GDR_BUTTON_ALL, 1);
    CIFButton* generalButton = m_IRM.GetResObj<CIFButton>(GDR_BUTTON_GENERAL, 1);
    CIFButton* questButton = m_IRM.GetResObj<CIFButton>(GDR_BUTTON_QUEST, 1);
    CIFButton* uniqueButton = m_IRM.GetResObj<CIFButton>(GDR_BUTTON_UNIQUE, 1);
    CIFButton* matchButton = m_IRM.GetResObj<CIFButton>(GDR_BUTTON_MATCH, 1);
    CIFButton* eventButton = m_IRM.GetResObj<CIFButton>(GDR_BUTTON_EVENT, 1);
    CIFButton* useButton = m_IRM.GetResObj<CIFButton>(GDR_BUTTON_USE, 1);
    CIFButton* removeButton = m_IRM.GetResObj<CIFButton>(GDR_BUTTON_REMOVE, 1);
    CIFWnd* descriptionHeader = m_IRM.GetResObj(19, 1);
    if (!allButton || !generalButton || !questButton || !uniqueButton ||
        !matchButton || !eventButton || !useButton || !removeButton ||
        !descriptionHeader)
        return false;

    std::n_wstring msgall = KmtGetText(L"UIIT_KMT_ALL");
    allButton->SetText(msgall.c_str());
    allButton->SetTooltip(msgall);
    allButton->SetStyleThingy(TOOLTIP);

    std::n_wstring msggeneral = KmtGetText(L"UIIT_KMT_GENERAL");
    generalButton->SetText(msggeneral.c_str());
    generalButton->SetTooltip(msggeneral);
    generalButton->SetStyleThingy(TOOLTIP);

    std::n_wstring msgquest = KmtGetText(L"UIIT_KMT_QUEST");
    questButton->SetText(msgquest.c_str());
    questButton->SetTooltip(msgquest);
    questButton->SetStyleThingy(TOOLTIP);

    std::n_wstring msgunique = KmtGetText(L"UIIT_KMT_UNIQUE");
    uniqueButton->SetText(msgunique.c_str());
    uniqueButton->SetTooltip(msgunique);
    uniqueButton->SetStyleThingy(TOOLTIP);



    std::n_wstring msgmatch = KmtGetText(L"UIIT_KMT_MATCHES");
    matchButton->SetText(msgmatch.c_str());
    matchButton->SetTooltip(msgmatch);
    matchButton->SetStyleThingy(TOOLTIP);

    std::n_wstring msgevent = KmtGetText(L"UIIT_KMT_EVENT");
    eventButton->SetText(msgevent.c_str());
    eventButton->SetTooltip(msgevent);
    eventButton->SetStyleThingy(TOOLTIP);

    removeButton->SetText(KmtGetText(L"UIIT_KMT_REMOVE_TITLE"));
    useButton->SetText(KmtGetText(L"UIIT_KMT_USE_TITLE"));
    descriptionHeader->SetText(KmtGetText(L"UIIT_KMT_DESCRIPTIONS"));
    nameHeader->SetText(KmtGetText(L"UIIT_KMT_TITLE_NAME"));
    TB_Func_13("interface\\frame\\mframe_wnd_", 0, 1);
    SetText(KmtGetText(L"UIIT_KMT_ACHIEVEMENT_CENTER"));
    SetGWndSize(640, 500);
    if (m_pTitleText) {
        m_pTitleText->m_FontTexture.SetColor(D3DCOLOR_ARGB(255, 255, 255, 255));
        m_pTitleText->BringToFront();
    }
    if (m_pCloseBtn) {
        m_pCloseBtn->TB_Func_13("interface\\ifcommon\\com_windowclose.ddj", 0, 0);
        m_pCloseBtn->SetGWndSize(16, 16);
        m_pCloseBtn->ShowGWnd(true);
        m_pCloseBtn->BringToFront();
    }
    UpdateMenuSize();
    this->ShowGWnd(false);
    return true;
}

bool CIFAchievements::EnsureSlotCount(size_t required)
{
    while (m_slots.size() < required) {
        wnd_rect slotRect;
        ZeroMemory(&slotRect, sizeof(slotRect));
        slotRect.size.width = 560;
        slotRect.size.height = 60;

        const int childId = 302 + static_cast<int>(m_slots.size());
        CIFAchievementsSlot* slot = (CIFAchievementsSlot*)CGWnd::CreateInstance(
            this, GFX_RUNTIME_CLASS(CIFAchievementsSlot), slotRect, childId, 0);
        if (!slot)
            return false;

        m_slots.push_back(slot);
    }
    return true;
}

void CIFAchievements::OnUnknownStuff() {
    int id = GetCurrentEventMsgCtrlId();
    int i = 0;

    for (int i = 0; i < numberOfTabs; ++i) {
        if (id == m_pTabs[i]->UniqueID()) {
            ActivateTabPage(i);
            return;
        }
    }
}
void CIFAchievements::ActivateTabPage(BYTE page) {
    if (page >= numberOfTabs || !m_pTabs[page])
        return;

    for (int i = 0; i < numberOfTabs; i++) {
        if (i == page)
            continue;

        m_pTabs[i]->sub_64CC30(0);
        m_pTabs[i]->m_FontTexture.sub_8B4750(2);
    }


    m_pTabs[page]->sub_64CC30(1);

    if(page == 0)
    {
        ActiveTabNumber = 0;
        m_IRM.GetResObj(10, 1)->SetText(KmtGetText(L"UIIT_KMT_TITLE_NAME"));


        m_IRM.GetResObj<CIFButton>(23, 1)->ShowGWnd(true);
        m_IRM.GetResObj<CIFButton>(24, 1)->ShowGWnd(true);
        m_IRM.GetResObj<CIFStatic>(3, 1)->SetText(KmtGetText(L"UIIT_KMT_CURRENT_TITLE_FEAC17E7"));

        m_IRM.GetResObj(4, 1)->SetGWndSize(600, 28);
        m_IRM.GetResObj(5, 1)->SetGWndSize(592, 20);
        m_IRM.GetResObj(7, 1)->ShowGWnd(true);
        m_IRM.GetResObj(7, 1)->BringToFront();
        m_IRM.GetResObj(118, 1)->ShowGWnd(false);
        m_IRM.GetResObj(119, 1)->ShowGWnd(false);
        m_IRM.GetResObj<CIFButton>(23, 1)->SetEnabledState(false);
        Clear();
        ClearDDJ();
        SetDescBoxText(L" ");
        On_BtnClickAll();
    }
    else if(page == 1)
    {
        ActiveTabNumber = 1;
        m_IRM.GetResObj(10, 1)->SetText(KmtGetText(L"UIIT_KMT_NAME"));

        m_IRM.GetResObj<CIFButton>(23, 1)->ShowGWnd(false);
        m_IRM.GetResObj<CIFButton>(24, 1)->ShowGWnd(false);

        std::n_wstring category = KmtGetText(L"UIIT_KMT_ACHIEVEMENTS_B8888662");
        m_IRM.GetResObj(3, 1)->SetText(category.c_str());

        m_IRM.GetResObj(4, 1)->SetGWndSize(600, 28);
        m_IRM.GetResObj(5, 1)->SetGWndSize(592, 20);
        m_IRM.GetResObj(7, 1)->ShowGWnd(false);

        int completesize = 0;
        int achievemetsize = m_CustomDataManager->m_RefAchievement.size();
        for (std::map<int, byte>::iterator it = m_Player->m_Achievements.begin(); it != m_Player->m_Achievements.end(); ++it)
        {
            if(it->second != 0)
            {
                completesize++;
            }
        }
        if(completesize > 0 && achievemetsize > 0)
        {
            int barwidht = (588 * completesize) / achievemetsize;
            m_IRM.GetResObj(118, 1)->SetGWndSize(barwidht, 12);
            m_IRM.GetResObj(118, 1)->ShowGWnd(true);
            m_IRM.GetResObj(118, 1)->BringToFront();
            wchar_t buffer[1024];
            int yuzde = (completesize*100)/achievemetsize;
            swprintf(buffer, L"%d%%", yuzde);
            m_IRM.GetResObj(119, 1)->SetText(buffer);

            m_IRM.GetResObj(119, 1)->ShowGWnd(true);
            m_IRM.GetResObj(119, 1)->BringToFront();
        }
        else
        {
            m_IRM.GetResObj(118, 1)->SetGWndSize(1, 12);
            m_IRM.GetResObj(118, 1)->ShowGWnd(false);
            wchar_t buffer[1024];
            swprintf(buffer, L"%d%%", 0);
            m_IRM.GetResObj(119, 1)->SetText(buffer);
            m_IRM.GetResObj(119, 1)->ShowGWnd(true);
            m_IRM.GetResObj(119, 1)->BringToFront();
        }

        Clear();
        ClearDDJ();
        On_BtnClickAll();
        SetDescBoxText(L" ");
    }
}
void CIFAchievements::UpdateMenuSize()
{
    int width = 1024;
    int height = 768;
    if (g_CGame && g_CGame->GetRes().res) {
        width = g_CGame->GetRes().res->width;
        height = g_CGame->GetRes().res->height;
    }

    int PosX = (width - this->GetSize().width) / 2;
    int PosY = ((height - this->GetSize().height) / 2) + 10;
    if (PosX < 0)
        PosX = 0;
    if (PosY < 0)
        PosY = 0;
    if (PosY + this->GetSize().height > height)
        PosY = height - this->GetSize().height;
    if (PosY < 0)
        PosY = 0;
    this->MoveGWnd(PosX, PosY);
    if (m_pCloseBtn) {
        m_pCloseBtn->MoveGWnd(GetPos().x + 614, GetPos().y + 9);
        m_pCloseBtn->BringToFront();
    }
    BringToFront();
}
void CIFAchievements::SetDescBoxText(std::n_wstring string)
{
    //  vscroll->sub_65A5A0(999);
    descbox->SetText(string.c_str());
    vscroll->sub_65A5A0(999);

}

void CIFAchievements::SetUseButtonState(bool s)
{
    m_IRM.GetResObj<CIFButton>(23, 1)->SetEnabledState(s);
}

void CIFAchievements::OnUpdate()
{
    CIFMainFrame::OnUpdate();
    if (!IsVisible() || !g_pMyPlayerObj)
        return;

        std::n_wstring Test;
        g_pMyPlayerObj->fonttexture_playername.GetText(&Test);
        CIFStatic* currentTitle = this->m_IRM.GetResObj<CIFStatic>(7, 1);
        if (currentTitle)
            currentTitle->SetText(Test.c_str());
}
void CIFAchievements::Clear()
{
    if (!m_scroll)
        return;

    for (size_t i = 0; i < m_slots.size(); ++i)
    {
        m_scroll->DeleteItem(m_slots[i]);
        m_slots[i]->ShowGWnd(false);
    }
}
void CIFAchievements::ClearDDJ()
{
    SelectedItemID = 0;
    for (size_t i = 0; i < m_slots.size(); ++i)
    {
        m_slots[i]->ClearDDJ();
    }
    SetUseButtonState(false);
}
void CIFAchievements::On_BtnClick() {

    if(SelectedItemID > 0)
    {
        CMsgStreamBuffer buf(0x169A);
        buf << BYTE(0x8) << SelectedItemID;
        SendMsg(buf);
        UseButtonDelay(7500);
    }
}
void CIFAchievements::OnListUpdated(){
    PopulateList(LastActiveCategory);
}

void CIFAchievements::PopulateList(int category)
{
    Clear();
    ClearDDJ();
    SetDescBoxText(KmtGetText(L"UIIT_KMT_SELECT_AN_ACHIEVEMENT_TO_VIEW_ITS_REQUIREMENTS_AND_REWARD"));

    std::vector<CustomDataManager::Achievements> rows;
    if (ActiveTabNumber == 0) {
        for (std::map<int, byte>::iterator it = m_Player->m_Achievements.begin();
             it != m_Player->m_Achievements.end(); ++it) {
            std::map<int, CustomDataManager::Achievements>::iterator reference =
                m_CustomDataManager->m_RefAchievement.find(it->first);
            if (reference == m_CustomDataManager->m_RefAchievement.end())
                continue;
            if (it->second == 0 || reference->second.RewardType != 0)
                continue;
            if (category >= 0 && reference->second.Category != category)
                continue;
            rows.push_back(reference->second);
        }
    } else if (ActiveTabNumber == 1) {
        for (std::map<int, CustomDataManager::Achievements>::iterator it =
                 m_CustomDataManager->m_RefAchievement.begin();
             it != m_CustomDataManager->m_RefAchievement.end(); ++it) {
            if (category >= 0 && it->second.Category != category)
                continue;
            rows.push_back(it->second);
        }
    }

    if (!EnsureSlotCount(rows.size()))
        return;

    for (size_t i = 0; i < rows.size(); ++i) {
        m_slots[i]->WriteLine(rows[i].ID, rows[i].Category, rows[i].Name);
        m_slots[i]->ShowGWnd(true);
        m_scroll->AddItem(m_slots[i], 1, 0);
    }

    if (rows.empty())
        SetDescBoxText(KmtGetText(L"UIIT_KMT_NO_ACHIEVEMENTS_ARE_AVAILABLE_IN_THIS_CATEGORY"));
}

void CIFAchievements::HighlightCategory(int category)
{
    const int buttonIds[] = {
        GDR_BUTTON_ALL,
        GDR_BUTTON_GENERAL,
        GDR_BUTTON_QUEST,
        GDR_BUTTON_UNIQUE,
        GDR_BUTTON_MATCH,
        GDR_BUTTON_EVENT
    };
    const int selectedIndex = category < 0 ? 0 : category + 1;

    for (int i = 0; i < sizeof(buttonIds) / sizeof(buttonIds[0]); ++i) {
        CIFButton* button = m_IRM.GetResObj<CIFButton>(buttonIds[i], 1);
        if (!button)
            continue;
        button->m_FontTexture.SetColor(i == selectedIndex
            ? D3DCOLOR_ARGB(255, 255, 216, 117)
            : D3DCOLOR_ARGB(255, 255, 255, 255));
    }
}

void CIFAchievements::On_BtnClickAll(){
    LastActiveCategory = -1;
    HighlightCategory(-1);
    PopulateList(-1);
}
void CIFAchievements::On_BtnClickGeneral()
{
    LastActiveCategory = 0;
    HighlightCategory(0);
    PopulateList(0);
}
void CIFAchievements::On_BtnClickQuest(){
    LastActiveCategory = 1;
    HighlightCategory(1);
    PopulateList(1);
}
void CIFAchievements::On_BtnClickUnique(){
    LastActiveCategory = 2;
    HighlightCategory(2);
    PopulateList(2);
}
void CIFAchievements::On_BtnClickMatch(){
    LastActiveCategory = 3;
    HighlightCategory(3);
    PopulateList(3);
}
void CIFAchievements::On_BtnClickEvent(){
    LastActiveCategory = 4;
    HighlightCategory(4);
    PopulateList(4);
}

void CIFAchievements::On_BtnClickRemove(){
    CMsgStreamBuffer buf(0x169A);
    buf << BYTE(6);
    SendMsg(buf);
    RemoveButtonDelay(7500);
}
#define GDR_BUTTON_REMOVETIMER 13513
#define GDR_BUTTON_USETIMER 13514
void CIFAchievements::RemoveButtonDelay(int timeoutSeconds) {
    this->m_IRM.GetResObj<CIFButton>(GDR_BUTTON_REMOVE, 1)->SetEnabledState(0);
    this->StartTimer(GDR_BUTTON_REMOVETIMER, timeoutSeconds);
}
void CIFAchievements::UseButtonDelay(int timeoutSeconds) {
    this->m_IRM.GetResObj<CIFButton>(GDR_BUTTON_USE, 1)->SetEnabledState(0);
    this->StartTimer(GDR_BUTTON_USETIMER, timeoutSeconds);
}
void CIFAchievements::OnTimer(int timerId) {
    if (timerId == GDR_BUTTON_REMOVETIMER)
    {
        this->KillTimer(GDR_BUTTON_REMOVETIMER);
        this->m_IRM.GetResObj<CIFButton>(GDR_BUTTON_REMOVE, 1)->SetEnabledState(1);
    }
    if(timerId == GDR_BUTTON_USETIMER)
    {
        this->KillTimer(GDR_BUTTON_USETIMER);
        bool canUse = SelectedItemID > 0 &&
            m_Player->m_Achievements.find(SelectedItemID) != m_Player->m_Achievements.end() &&
            m_Player->m_Achievements[SelectedItemID] == 1;
        this->m_IRM.GetResObj<CIFButton>(GDR_BUTTON_USE, 1)->SetEnabledState(canUse);
    }
}
