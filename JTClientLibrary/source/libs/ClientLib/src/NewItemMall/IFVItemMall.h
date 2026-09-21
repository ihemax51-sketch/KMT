#pragma once
#include <IFRenderStatic.h>
#include <IFCheckBox.h>
#include "IFMainFrame.h"
#include "IFBarWnd.h"
#include "IFSpinButtonCtrl.h"
#include "IFVerticalScroll.h"
#include "IFScrollManager.h"
#include "IFStretchWnd.h"
#include "IFVItemMallSlotViewer.h"
#include "IFVItemMallSlotNews.h"
#include "IFVItemMallFavList.h"


class CIFVItemMall : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFVItemMall)
GFX_DECLARE_MESSAGE_MAP(CIFVItemMall)
private:
    CIFVItemMall(void);
    ~CIFVItemMall(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void OnClick_Exit();

public:
    undefined OnCloseWnd() override;
    void UpdateMenuSize();
    int Func_4(int a2) override;
    void ActivateTabPage(BYTE page);
    void OnUnknownStuff();
    void RightBtn();
    void LeftBtn();
    void RightBtnTab();
    void LeftBtnTab();
    int newlistnum();
    int newlistpagenum();
    /// \brief Show the given page
    /// \param page Page Number (0 to N)
    void Clear();
    void ClearFav();
    void UpdateMarket();
    void UpdateFavList();
    void Getlist(std::n_wstring testtt, byte test);
    void GetNewList(int test);


    void ClearNew();
    void Charge();
    void OpenList();
    void ItemMall();
    void Preset();
    void TakeAll();
    struct FavItem
    {
        int DbID;
        int ItemID;
        int DataItemCount;
        int Price;
    };
    std::map<int, FavItem> FavItemList;

    int aaa;
public:
    int numberOfTabs;
    static const int tabWidth = 88;
    static const int tabHeight = 31;
    static const int tabFirstId = 100;

    static const int maxVisibleTabs = 6; // Aynı anda kaç sekme görünecek
    int m_currentGroup;
    CIFStatic* title;
    CIFSelectableArea** m_pTabs;
    CIFScrollManager* m_scroll;
    CIFVItemMallSlotViewer*m_slotviewer[150];
    CIFVItemMallSlotNews* m_slotnews[2];
    CIFScrollManager* m_scrollfav;
    CIFVItemMallFavList* favlist[100];

    int ActivedPage;

void OpenButton(int timeoutSeconds);
void OnTimer(int timerId) override;

    int SilkRank;
    int SilkHistory;
};