
#pragma once
#include <IFSelectableArea.h>
#include <IFScrollManager.h>
#include "IFMainFrame.h"
#include "IFEdit.h"
#include "IFChestSlot.h"

class CIFChest : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFChest)
GFX_DECLARE_MESSAGE_MAP(CIFChest)
private:
    CIFChest(void);
    ~CIFChest(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void On_BtnClick();
    void OnUnknownStuff();
    void OnTimer(int) override;

    int Func_4(int a2) override;
    static const int numberOfTabs = 2;
    static const int tabWidth = 96;
    static const int tabHeight = 24;
    static const int tabMarginLeft = 21;
    static const int tabFirstId = 100;
    CIFSelectableArea **m_pTabs; //0x04A4 is a pointer, type unknown
    int m_CurrentIndex;
    int m_MaxIndex;
    int m_PendingDbId;
    int m_ExpectedSnapshotCount;
    bool m_SnapshotLoading;

    void RequestNextTakeAllItem();
    void StopClaimWorkflow();


public:
    struct CharChest
    {
        int DbID;
        int ItemID;
        int Quantity;
        std::n_wstring Date;
        std::n_wstring Type;
        byte Plus;
    };

    std::map<int, CharChest> my_Chest;
    bool TakeAllWorking;
public:
    void UpdateMenuSize();
    void Clear();
    void ActivateTabPage(BYTE page);

    undefined1 OnCloseWnd() override;
void UpdateRanks();
void LoadPage(int pageNumber);
void UpdateText(int Number);
void On_NextBtn();
void On_PrevBtn();
void ReloadRanks();
bool RequestTake(int databaseId);
void BeginSnapshot(int expectedCount);
void AddSnapshotItem(int dbId, int itemId, int quantity, const std::n_wstring &date,
                     const std::n_wstring &type, byte plus);
void EndSnapshot();

};
