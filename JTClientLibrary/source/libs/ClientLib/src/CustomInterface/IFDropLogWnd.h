#pragma once

#include "IFMainFrame.h"
#include "IFSlotWithHelp.h"
#include "IFNormalTile.h"
#include "IFFrame.h"
#include "IFButton.h"
#include "IFStatic.h"
#include <vector>

#define DROP_LOG_WINDOW_ID 13420

class CIFDropLogWnd : public CIFMainFrame {
    GFX_DECLARE_DYNCREATE(CIFDropLogWnd)
    GFX_DECLARE_MESSAGE_MAP(CIFDropLogWnd)

public:
    CIFDropLogWnd();
    ~CIFDropLogWnd();

    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void ShowGWnd(bool bVisible) override;
    undefined1 OnCloseWnd() override;
    int OnChar(UINT nChar, UINT a2, UINT a3);

    void HandleSlotClick(int slotID);
    void RefreshDropList();
    void UpdateWindowPos();
    void OnPageClick();
    void OpenGroundDropList();
    void OpenPossibleDrops(int monsterRefObjId, const wchar_t* monsterName);
    void HandlePossibleDropsResponse(bool success, int monsterRefObjId, const std::vector<int>& itemRefObjIds);

private:
    void ClearDisplaySlot(int slotIdx);
    void FillDisplaySlot(int slotIdx, unsigned int gid, int refObjId, bool clickable);
    void RequestPossibleDrops();
    void FillPossibleDrops();
    void ClearAllDisplaySlots();
    int GetCurrentItemCount() const;
    int GetMaxPage(int itemCount) const;

private:
    enum DisplayMode {
        MODE_GROUND_DROPS = 0,
        MODE_POSSIBLE_DROPS = 1
    };

    CIFNormalTile* m_pBackground;
    CIFFrame* m_pSlotFrame;
    CIFButton* m_pBtnPrev;
    CIFButton* m_pBtnNext;
    CIFStatic* m_pPageText;

    CIFStatic* m_pSlotBackgrounds[90];
    CIFSlotWithHelp* m_pSlots[90];
    unsigned int m_slotGIDs[90];
    bool m_bLastLButton;
    bool m_bLastRButton;
    int m_currentPage;
    DWORD m_lastPickupTick;
    DisplayMode m_displayMode;
    int m_possibleDropsMonsterId;
    bool m_possibleDropsLoading;
    std::n_wstring m_possibleDropsMonsterName;
    std::vector<int> m_possibleDropItemIds;
};
