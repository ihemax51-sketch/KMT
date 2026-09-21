#pragma once
#include <IFSelectableArea.h>
#include "IFMainFrame.h"

class CIFDynamicRanking : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFDynamicRanking)
GFX_DECLARE_MESSAGE_MAP(CIFDynamicRanking)
private:
    static const int MaxCategoryTabs = 9;
    static const int CategoryTabFirstId = 100;

    struct RankCategory
    {
        int Id;
        std::n_wstring Name;
    };

    CIFDynamicRanking(void);
    ~CIFDynamicRanking(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void OnTimer(int) override;
    int Func_4(int a2) override;
    void OnCategoryTab();
    void ActivateCategory(int index, bool requestRanks);
    void LayoutCategoryTabs();
    void SetCategoryTabsEnabled(bool enabled);
    std::n_wstring GetCategoryTabText(const std::n_wstring& text, int tabWidth) const;
    void SendPacket(byte type);

    CIFSelectableArea** m_pCategoryTabs;
    std::vector<RankCategory> m_RankCategories;
    int m_SelectedCategoryIndex;
    bool m_CategoryRequestLocked;
public:
    void UpdateMenuSize();
    void Clear();
    void ResetData();
    void ClearCategories();
    void AddCategory(int id, const std::n_wstring& name);
    void FinishCategories();
    void UpdateRanks();
    void UpdateSelfRank(std::n_wstring Name, int No, int Point);
    void Hide();
    struct RankStruct
    {
        int LineNum;
        std::wstring Charname;
        std::wstring Guild;
        std::wstring Points;
    };
    std::vector<RankStruct> RankList;


    int m_CurrentIndex;
    int m_MaxIndex;
    void LockCategoryTabs(int timeoutMilliseconds);
    void On_NextBtn();
    void On_PrevBtn();
    void UpdateText(int Number);
    void LoadPage(int pageNumber);
};
