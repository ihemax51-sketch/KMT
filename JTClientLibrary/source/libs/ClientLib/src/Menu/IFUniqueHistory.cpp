#include "IFUniqueHistory.h"
#include <Game.h>
#include <ICPlayer.h>
#include <BSLib/multibyte.h>
#include "GInterface.h"
#include <CharacterDependentData.h>
#include <TextStringManager.h>
#include <GlobalDataManager.h>
#include <iostream>
#include <CustomInterface/IFDps.h>


#define GDR_NEXT 902
#define GDR_PREV 901
#define GDR_SPIN_TEXT_PAGE 900
#define GDR_RESULT_BUTTON 23
#define GDR_DPS_CHECKBOX 31
GFX_IMPLEMENT_DYNCREATE(CIFUniqueHistory, CIFMainFrame)

GFX_BEGIN_MESSAGE_MAP(CIFUniqueHistory, CIFMainFrame)
                    ONG_COMMAND(GDR_RESULT_BUTTON, &On_BtnClick)
                    ONG_COMMAND(GDR_PREV, &On_PrevBtn)
                    ONG_COMMAND(GDR_NEXT, &On_NextBtn)
GFX_END_MESSAGE_MAP()

CIFUniqueHistory::CIFUniqueHistory(void){
    UniqueHistoryList = std::map<int, UniqueHistory>();
    UniqueHistoryOrder = std::vector<int>();
    SelectedRegionID = 0;
    SelectedX = 0;
    SelectedZ = 0;
    SelectedY = 0;
    SelectedWorldID = 0;
    SelectedMapType = 0;
    SelectedMapIndex = 0;
    SelectedUniqueID = 0;
    m_CurrentIndex = 0;
    m_MaxIndex = 0;
}
CIFUniqueHistory::~CIFUniqueHistory(void){
}

bool CIFUniqueHistory::OnCreate(long ln)
{

    // Populate inherited members
    CIFMainFrame::OnCreate(ln);


    m_IRM.LoadFromFile("clientlibrary\\resinfo\\ifuniquehistory.txt");
    m_IRM.CreateInterfaceSection("Create", this);
    // DPS is opt-in. Initialize once so reopening the window preserves
    // the player's choice for this interface instance.
    CIFCheckBox* dpsCheckBox = m_IRM.GetResObj<CIFCheckBox>(GDR_DPS_CHECKBOX, 1);
    if (dpsCheckBox != NULL)
        dpsCheckBox->SetCheckBoxState(false);
    wnd_rect sz;

    this->SetText(KmtGetText(L"UIIT_KMT_UNIQUE_HISTORY"));

    this->m_IRM.GetResObj(28, 1)->SetText(KmtGetText(L"UIIT_KMT_NAME"));
    this->m_IRM.GetResObj(21, 1)->SetText(KmtGetText(L"UIIT_KMT_STATUS"));
    this->m_IRM.GetResObj(22, 1)->SetText(KmtGetText(L"UIIT_KMT_ELAPSED_TIME"));
    this->m_IRM.GetResObj(25, 1)->SetText(KmtGetText(L"UIIT_KMT_KILLER"));
    this->m_IRM.GetResObj(32, 1)->SetText(KmtGetText(L"UIIT_KMT_DPS"));
    this->m_IRM.GetResObj(GDR_SPIN_TEXT_PAGE, 1)->SetText(L"1");

    for (int i = 0; i < 9; ++i)
    {
        m_IRM.GetResObj<CIFUniqueHistorySlot>(11 + i, 1)->ShowGWnd(true);
    }

    this->ShowGWnd(false);
    return true;
}

void CIFUniqueHistory::UpdateMenuSize()
{

    int PosX = 0, PosY = 0;
    PosY = (g_CGame->GetRes().res->height/2) - (this->GetSize().height/2);
    PosX = (g_CGame->GetRes().res->width/2) - (this->GetSize().width/2);
    this->MoveGWnd(PosX, PosY);
    BringToFront();
}


void CIFUniqueHistory::OnUpdate()
{

}
bool CIFUniqueHistory::DpsEnabled()
{
    CIFCheckBox* checkBox = this->m_IRM.GetResObj<CIFCheckBox>(31, 1);
    return checkBox != NULL && checkBox->GetCheckedState_MAYBE();
}

void CIFUniqueHistory::Clear()
{
    for (int i = 0; i < 9; ++i)
    {
        CIFUniqueHistorySlot* slot = m_IRM.GetResObj<CIFUniqueHistorySlot>(11 + i, 1);
        if (slot != NULL)
        {
            slot->Clear();
            slot->ShowGWnd(false);
        }
    }
}
void CIFUniqueHistory::ClearDDJ() {
    for (int i = 0; i < 9; ++i)
    {
        m_IRM.GetResObj<CIFUniqueHistorySlot>(11 + i, 1)->ClearDDJ();
    }
}
void CIFUniqueHistory::UpdateRanks(bool resetPage) {
    Clear();
    ClearDDJ();
    UniqueHistoryOrder.clear();
    for (std::map<int, UniqueHistory>::iterator it = UniqueHistoryList.begin();
         it != UniqueHistoryList.end(); ++it) {
        UniqueHistoryOrder.push_back(it->first);
    }

    for (size_t i = 0; i < UniqueHistoryOrder.size(); ++i) {
        size_t best = i;
        for (size_t j = i + 1; j < UniqueHistoryOrder.size(); ++j) {
            const UniqueHistory &current = UniqueHistoryList[UniqueHistoryOrder[best]];
            const UniqueHistory &candidate = UniqueHistoryList[UniqueHistoryOrder[j]];
            if (candidate.Status > current.Status ||
                (candidate.Status == current.Status && candidate.Elapsedtime > current.Elapsedtime)) {
                best = j;
            }
        }
        if (best != i) {
            int id = UniqueHistoryOrder[i];
            UniqueHistoryOrder[i] = UniqueHistoryOrder[best];
            UniqueHistoryOrder[best] = id;
        }
    }

    m_MaxIndex = UniqueHistoryOrder.empty() ? 1 : (UniqueHistoryOrder.size() + 8) / 9;
    if (resetPage || m_CurrentIndex < 1)
        m_CurrentIndex = 1;
    if (m_CurrentIndex > m_MaxIndex)
        m_CurrentIndex = m_MaxIndex;

    LoadPage(m_CurrentIndex);
    UpdateText(m_CurrentIndex);
}

void CIFUniqueHistory::LoadPage(int pageNumber) {
    if (UniqueHistoryOrder.empty()) {
        return;
    }

    int startIndex = (pageNumber - 1) * 9;
    if (startIndex >= UniqueHistoryOrder.size()) {
        return;
    }
    int endIndex = startIndex + 9;

    Clear();
    ClearDDJ();
    if (endIndex > UniqueHistoryOrder.size()) {
        endIndex = UniqueHistoryOrder.size();
    }

    int slotIndex = 0;
    for (int i = startIndex; i < endIndex; ++i) {
        int uniqueId = UniqueHistoryOrder[i];
        std::map<int, UniqueHistory>::iterator it = UniqueHistoryList.find(uniqueId);
        if (it == UniqueHistoryList.end()) continue;
        m_IRM.GetResObj<CIFUniqueHistorySlot>(11 + slotIndex, 1)->
                SetName(slotIndex + 1, it->second.UniqueName.c_str(), it->second.Status,
                        it->second.Elapsedtime, it->second.Killer.c_str(),
                        it->second.RegionID, it->second.KilledX,
                        it->second.KilledY, it->second.KilledZ,
                        it->second.WorldID, it->second.MapType, it->second.MapIndex, uniqueId);
        if (uniqueId == SelectedUniqueID)
            m_IRM.GetResObj<CIFUniqueHistorySlot>(11 + slotIndex, 1)->SelectDDJ();
        m_IRM.GetResObj<CIFUniqueHistorySlot>(11 + slotIndex, 1)->ShowGWnd(true);
        ++slotIndex;
    }
}

void CIFUniqueHistory::On_NextBtn() {
    if (m_CurrentIndex < m_MaxIndex) {
        m_CurrentIndex++;
        LoadPage(m_CurrentIndex);  // Yeni sayfayı yükle
        UpdateText(m_CurrentIndex);  // Sayfa numarasını güncelle
    }
}

void CIFUniqueHistory::On_PrevBtn() {
    if (m_CurrentIndex > 1) {
        m_CurrentIndex--;
        LoadPage(m_CurrentIndex);  // Önceki sayfayı yükle
        UpdateText(m_CurrentIndex);  // Sayfa numarasını güncelle
    }
}

void CIFUniqueHistory::UpdateText(int Number) {
    wchar_t buffer1[255];
    swprintf_s(buffer1, L"%d/%d", Number, m_MaxIndex);
    this->m_IRM.GetResObj<CIFStatic>(GDR_SPIN_TEXT_PAGE, 1)->SetText(buffer1);
}

void CIFUniqueHistory::ClearSelection()
{
    ClearDDJ();
    SelectedX = 0;
    SelectedY = 0;
    SelectedZ = 0;
    SelectedRegionID = 0;
    SelectedWorldID = 0;
    SelectedMapType = 0;
    SelectedMapIndex = 0;
    SelectedUniqueID = 0;

    CIFDps* dps = g_pCGInterface->m_IRM.GetResObj<CIFDps>(DPSID, 1);
    if (dps != NULL && dps->IsVisible())
        dps->ShowGWnd(false);
}


void CIFUniqueHistory::On_BtnClick() {

}
undefined1 CIFUniqueHistory::OnCloseWnd(){
    ClearSelection();
    return CIFWnd::OnCloseWnd();
}
