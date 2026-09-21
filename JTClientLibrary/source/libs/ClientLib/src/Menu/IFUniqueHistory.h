#pragma once
#include <IFVerticalScroll.h>
#include "IFMainFrame.h"
#include "IFEdit.h"
#include "IFUniqueHistorySlot.h"

class CIFUniqueHistory : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFUniqueHistory)
GFX_DECLARE_MESSAGE_MAP(CIFUniqueHistory)
private:
    CIFUniqueHistory(void);
    ~CIFUniqueHistory(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void On_BtnClick();
public:
    undefined1 OnCloseWnd() override;
    void UpdateMenuSize();
    void Clear();
    void ClearDDJ();
    void UpdateRanks(bool resetPage = true);
    void ClearSelection();

    bool DpsEnabled();
    struct UniqueHistory
    {
        std::wstring UniqueName;
        byte Status;
        __int64 Elapsedtime;
        std::wstring Killer;
        int RegionID;
        float KilledX;
        float KilledY;
        float KilledZ;
        int WorldID;
        byte MapType;
        int MapIndex;
        std::vector<std::pair<std::n_wstring, std::n_wstring> > DpsMeter;
    };
    std::map<int, UniqueHistory> UniqueHistoryList;
    std::vector<int> UniqueHistoryOrder;

    int SelectedRegionID;
    float SelectedX;
    float SelectedZ;
    float SelectedY;
    int SelectedWorldID;
    byte SelectedMapType;
    int SelectedMapIndex;
    int SelectedUniqueID;

    int m_CurrentIndex;
    int m_MaxIndex;
void On_PrevBtn();
void On_NextBtn();
void LoadPage(int pageNumber);
void UpdateText(int Number);
};
