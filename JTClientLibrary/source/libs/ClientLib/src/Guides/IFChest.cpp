#include "IFChest.h"
#include <Game.h>
#include <ICPlayer.h>
#include <BSLib/multibyte.h>
#include "GInterface.h"
#include <CharacterDependentData.h>
#include <TextStringManager.h>
#include <GlobalDataManager.h>
#include <CustomData/CustomDataManager.h>


#define GDR_TAKE_ALL 23


#define GDR_NEXT 902
#define GDR_PREV 901
#define GDR_SPIN_TEXT_PAGE 900
#define CHEST_CLAIM_TIMEOUT_TIMER 1345
#define CHEST_TAKE_ALL_NEXT_TIMER 1346


GFX_IMPLEMENT_DYNCREATE(CIFChest, CIFMainFrame)

GFX_BEGIN_MESSAGE_MAP(CIFChest, CIFMainFrame)
                    ONG_COMMAND(100, &CIFChest::OnUnknownStuff)
                    ONG_COMMAND(GDR_TAKE_ALL, &CIFChest::On_BtnClick)
                    ONG_COMMAND(GDR_PREV, &On_PrevBtn)
                    ONG_COMMAND(GDR_NEXT, &On_NextBtn)
GFX_END_MESSAGE_MAP()

CIFChest::CIFChest(void){
    m_pTabs = 0;
    m_CurrentIndex = 1;
    m_MaxIndex = 1;
    m_PendingDbId = 0;
    m_ExpectedSnapshotCount = 0;
    m_SnapshotLoading = false;
    TakeAllWorking = false;
    my_Chest = std::map<int, CharChest>();
}
CIFChest::~CIFChest(void){
    if (m_pTabs) {
        delete[] m_pTabs;
        m_pTabs = 0;
    }
}
int CIFChest::Func_4(int a2) {
    int v1 = 0;
    while (a2 != v1 + 100) {
        if (++v1 >= numberOfTabs)
            return -1;
    }

    return 100;
}

void CIFChest::OnUnknownStuff() {
    int id = GetCurrentEventMsgCtrlId();
    int i = 0;

    for (int i = 0; i < numberOfTabs; ++i) {
        if (id == m_pTabs[i]->UniqueID()) {
            ActivateTabPage(i);
            return;
        }
    }
}
void CIFChest::ActivateTabPage(BYTE page) {

}
bool CIFChest::OnCreate(long ln)
{

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);
    wnd_rect sz;

    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifchest.txt");
    m_IRM.CreateInterfaceSection("Create", this);


    m_pTabs = new CIFSelectableArea *[numberOfTabs];

    for (int i = 0; i < numberOfTabs; i++) {

        RECT mrect;
        mrect.top = 5;
        mrect.left = 6;
        mrect.right = 0;
        mrect.bottom = 4;

        RECT selectable_area_size;
        selectable_area_size.top = 35;
        selectable_area_size.left = tabMarginLeft + tabWidth * i;
        selectable_area_size.right = tabWidth - 4;
        selectable_area_size.bottom = tabHeight;

        m_pTabs[i] = (CIFSelectableArea *) CGWnd::CreateInstance(this, GFX_RUNTIME_CLASS(CIFSelectableArea),
                                                                 selectable_area_size, tabFirstId + i, 0);

        m_pTabs[i]->SetSomeRect(mrect);
        m_pTabs[i]->SetFont(this->N00009C2F);

        m_pTabs[i]->sub_64CE30("interface\\ifcommon\\com_long_tab_on.ddj",
                               "interface\\ifcommon\\com_long_tab_off.ddj",
                               "interface\\ifcommon\\com_long_tab_disable.ddj");

        if (i) {
            switch (i) {
                case 1:
                    m_pTabs[1]->SetText(KmtGetText(L"UIIT_KMT_LOG"));
                    break;

            }
            m_pTabs[i]->sub_64CC30(0);
        } else {
            m_pTabs[0]->SetText(KmtGetText(L"UIIT_KMT_ITEMS"));
            m_pTabs[i]->sub_64CC30(1);
        }
    }
    m_pTabs[1]->ShowGWnd(false);
    m_pTabs[1]->SetClickable(false);


    this->SetText(KmtGetText(L"UIIT_KMT_ITEM_CHEST"));
    this->m_IRM.GetResObj(23, 1)->SetText(KmtGetText(L"UIIT_KMT_TAKE_ALL"));
    this->m_IRM.GetResObj(9, 1)->SetText(KmtGetText(L"UIIT_KMT_TAKE"));
    this->m_IRM.GetResObj(8, 1)->SetText(KmtGetText(L"UIIT_KMT_FROM"));

    this->m_IRM.GetResObj(7, 1)->SetText(KmtGetText(L"UIIT_KMT_DATE"));
    this->m_IRM.GetResObj(6, 1)->SetText(KmtGetText(L"UIIT_KMT_QTY"));
    this->m_IRM.GetResObj(5, 1)->SetText(KmtGetText(L"UIIT_KMT_ITEM_INFO"));
    this->m_IRM.GetResObj(10, 1)->SetText(KmtGetText(L"UIIT_KMT_NO"));
    this->m_IRM.GetResObj<CIFStatic>(GDR_SPIN_TEXT_PAGE, 1)->SetText(L"1");



    std::n_wstring msg = KmtGetText(L"UIIT_KMT_INVENTORY_REMAINING");
    std::n_wstring categoryshort = msg.substr(0, 12) + L"...";
    m_IRM.GetResObj(25, 1)->SetText(categoryshort.c_str());
    m_IRM.GetResObj(25, 1)->SetTooltip(msg);
    m_IRM.GetResObj(25, 1)->SetStyleThingy(TOOLTIP);

    for(int i = 0; i < 11; i++)
    {
        m_IRM.GetResObj<CIFChestSlot>(50 + i, 1)->ShowGWnd(true);
    }

    this->ShowGWnd(false);
    return true;
}

void CIFChest::UpdateMenuSize()
{

    int PosX = 0, PosY = 0;
    if (g_CGame && g_CGame->GetRes().res) {
        PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
        PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    }
    CIFButton* takeAll = m_IRM.GetResObj<CIFButton>(GDR_TAKE_ALL, 1);
    if (takeAll)
        takeAll->SetEnabledState(!my_Chest.empty() && !TakeAllWorking && m_PendingDbId == 0);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}
void CIFChest::Clear()
{
    for (int i = 0; i < 11; ++i)
    {
        m_IRM.GetResObj<CIFChestSlot>(50 + i, 1)->Clear();
    }
    // UpdateSelfRank(L"empty", 0, -1);
}

void CIFChest::UpdateRanks() {
    Clear();

    m_MaxIndex = my_Chest.empty() ? 1 : (int)((my_Chest.size() + 10) / 11);
    if (m_CurrentIndex < 1)
        m_CurrentIndex = 1;
    if (m_CurrentIndex > m_MaxIndex)
        m_CurrentIndex = m_MaxIndex;

    LoadPage(m_CurrentIndex);
    UpdateText(m_CurrentIndex);

    CIFButton* previous = m_IRM.GetResObj<CIFButton>(GDR_PREV, 1);
    CIFButton* next = m_IRM.GetResObj<CIFButton>(GDR_NEXT, 1);
    if (previous)
        previous->SetEnabledState(m_CurrentIndex > 1);
    if (next)
        next->SetEnabledState(m_CurrentIndex < m_MaxIndex);
}
void CIFChest::ReloadRanks() {
    Clear();

/*    // İlk sayfayı yükle
    LoadPage(m_CurrentIndex);
    UpdateText(m_CurrentIndex);*/
}

void CIFChest::LoadPage(int pageNumber) {
    int startIndex = (pageNumber - 1) * 11;
    int endIndex = startIndex + 11;
    int totalRecords = (int)my_Chest.size();

    Clear();

    if (startIndex >= totalRecords) {
        return;
    }
    if (endIndex > totalRecords) {
        endIndex = totalRecords;
    }

    std::map<int, CharChest>::iterator it = my_Chest.begin();
    std::advance(it, startIndex);

    int slotIndex = 0;
    for (int i = startIndex; i < endIndex; ++i, ++it) {
        if (it == my_Chest.end()) break;

        int slotNumber = i - startIndex + 1;


        m_IRM.GetResObj<CIFChestSlot>(49 + slotNumber, 1)->
                SetName(i + 1, it->second.DbID, it->second.ItemID, it->second.Quantity, it->second.Date.c_str(),
                        it->second.Type.c_str(), it->second.Plus);
        m_IRM.GetResObj<CIFChestSlot>(49 + slotNumber, 1)->ShowGWnd(true);
        ++slotIndex;
    }
}


void CIFChest::UpdateText(int Number)
{
    wchar_t buffer1[255];
    swprintf_s(buffer1, L"%d", Number);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SPIN_TEXT_PAGE, 1)->SetText(buffer1);
}
void CIFChest::On_NextBtn() {
    int totalPages = my_Chest.empty() ? 1 : (int)((my_Chest.size() + 10) / 11);
    if (m_CurrentIndex < totalPages) {
        m_CurrentIndex++;
        LoadPage(m_CurrentIndex);
        UpdateText(m_CurrentIndex);
        CIFButton* previous = m_IRM.GetResObj<CIFButton>(GDR_PREV, 1);
        CIFButton* next = m_IRM.GetResObj<CIFButton>(GDR_NEXT, 1);
        if (previous)
            previous->SetEnabledState(true);
        if (next)
            next->SetEnabledState(m_CurrentIndex < totalPages);
    }
}

void CIFChest::On_PrevBtn() {
    if (m_CurrentIndex > 1) {
        m_CurrentIndex--;
        LoadPage(m_CurrentIndex);
        UpdateText(m_CurrentIndex);
        CIFButton* previous = m_IRM.GetResObj<CIFButton>(GDR_PREV, 1);
        CIFButton* next = m_IRM.GetResObj<CIFButton>(GDR_NEXT, 1);
        if (previous)
            previous->SetEnabledState(m_CurrentIndex > 1);
        if (next)
            next->SetEnabledState(true);
    }
}
void CIFChest::OnUpdate()
{
    unsigned __int8 test = g_CGlobalDataManager->GetEmptyInventorySlots();
    wchar_t Point[33];
    swprintf_s(Point, L"%d  ", test);
    m_IRM.GetResObj(24, 1)->SetText(Point);

    CIFButton* takeAll = m_IRM.GetResObj<CIFButton>(GDR_TAKE_ALL, 1);
    if (takeAll)
        takeAll->SetEnabledState(!my_Chest.empty() && !TakeAllWorking && m_PendingDbId == 0);
}

void CIFChest::On_BtnClick() {
    if (g_pCGInterface->IsInteractionBlocked(13) || TakeAllWorking || m_PendingDbId != 0 || my_Chest.empty())
        return;

    TakeAllWorking = true;
    CIFButton* takeAll = m_IRM.GetResObj<CIFButton>(GDR_TAKE_ALL, 1);
    if (takeAll)
        takeAll->SetEnabledState(false);
    RequestNextTakeAllItem();

}

void CIFChest::OnTimer(int timerId) {
    if (timerId == CHEST_CLAIM_TIMEOUT_TIMER) {
        KillTimer(CHEST_CLAIM_TIMEOUT_TIMER);
        StopClaimWorkflow();
        if (g_pCGInterface)
            g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_ITEM_CHEST_REQUEST_TIMED_OUT_REOPEN_THE_CHEST_BEFORE_TRYING_AGAIN"));
    } else if (timerId == CHEST_TAKE_ALL_NEXT_TIMER) {
        KillTimer(CHEST_TAKE_ALL_NEXT_TIMER);
        RequestNextTakeAllItem();
    }
}

bool CIFChest::RequestTake(int databaseId)
{
    if (!g_pCGInterface || databaseId <= 0 || m_PendingDbId != 0 ||
        g_pCGInterface->IsInteractionBlocked(13))
        return false;

    std::map<int, CharChest>::const_iterator it = my_Chest.find(databaseId);
    if (it == my_Chest.end())
        return false;

    m_PendingDbId = databaseId;
    CMsgStreamBuffer buf(0xB296);
    buf << databaseId;
    SendMsg(buf);
    StartTimer(CHEST_CLAIM_TIMEOUT_TIMER, 15000);
    return true;
}

void CIFChest::RequestNextTakeAllItem()
{
    if (!TakeAllWorking || m_PendingDbId != 0)
        return;

    if (my_Chest.empty()) {
        StopClaimWorkflow();
        return;
    }

    if (!RequestTake(my_Chest.begin()->first))
        StopClaimWorkflow();
}

void CIFChest::StopClaimWorkflow()
{
    KillTimer(CHEST_CLAIM_TIMEOUT_TIMER);
    KillTimer(CHEST_TAKE_ALL_NEXT_TIMER);
    m_PendingDbId = 0;
    TakeAllWorking = false;

    CIFButton* takeAll = m_IRM.GetResObj<CIFButton>(GDR_TAKE_ALL, 1);
    if (takeAll)
        takeAll->SetEnabledState(!my_Chest.empty());
}

void CIFChest::BeginSnapshot(int expectedCount)
{
    m_ExpectedSnapshotCount = expectedCount;
    m_SnapshotLoading = true;
    my_Chest.clear();
    Clear();
}

void CIFChest::AddSnapshotItem(int dbId, int itemId, int quantity, const std::n_wstring &date,
                               const std::n_wstring &type, byte plus)
{
    if (!m_SnapshotLoading || dbId <= 0 || itemId <= 0 || quantity <= 0)
        return;

    CharChest item = CharChest();
    item.DbID = dbId;
    item.ItemID = itemId;
    item.Quantity = quantity;
    item.Date = date;
    item.Type = type;
    item.Plus = plus;
    my_Chest[dbId] = item;
}

void CIFChest::EndSnapshot()
{
    if (!m_SnapshotLoading)
        return;

    m_SnapshotLoading = false;
    if ((int)my_Chest.size() != m_ExpectedSnapshotCount) {
        my_Chest.clear();
        StopClaimWorkflow();
        UpdateRanks();
        if (g_pCGInterface)
            g_pCGInterface->ShowMessage_Warning(KmtGetText(L"UIIT_KMT_ITEM_CHEST_DATA_WAS_INCOMPLETE_PLEASE_REOPEN_THE_CHEST"));
        return;
    }

    int completedDbId = m_PendingDbId;
    bool claimSucceeded = completedDbId == 0 || my_Chest.find(completedDbId) == my_Chest.end();
    KillTimer(CHEST_CLAIM_TIMEOUT_TIMER);
    m_PendingDbId = 0;
    UpdateRanks();

    if (TakeAllWorking) {
        if (!claimSucceeded || my_Chest.empty()) {
            StopClaimWorkflow();
        } else {
            StartTimer(CHEST_TAKE_ALL_NEXT_TIMER, 250);
        }
    }
}

undefined1 CIFChest::OnCloseWnd() {
    StopClaimWorkflow();
    return CIFWnd::OnCloseWnd();
}
