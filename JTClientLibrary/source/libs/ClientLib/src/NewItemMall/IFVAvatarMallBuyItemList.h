#pragma once
#include "IFMainFrame.h"
#include "IFBarWnd.h"
#include "IFSpinButtonCtrl.h"
#include "IFVerticalScroll.h"
#include "IFScrollManager.h"
#include "IFStretchWnd.h"
#include "IFSelectableArea.h"

class CIFVAvatarMallBuyItemList : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFVAvatarMallBuyItemList)
GFX_DECLARE_MESSAGE_MAP(CIFVAvatarMallBuyItemList)
private:
    CIFVAvatarMallBuyItemList(void);
    ~CIFVAvatarMallBuyItemList(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void TakeButton();
    void OpenButton(int timeoutSeconds);
    void OnTimer(int) override;
public:
    void UpdateMenuSize();

    void Clear();

    struct ItemsStruct
    {
        int DBID;
        int ItemId;
        int Price;
        unsigned short TypeId;
    };
    std::map<unsigned short, ItemsStruct> ItemList;

public:
    CIFStatic* fullprice;
    CIFStatic* PricePics;
    CIFScrollManager* m_scroll;
    CIFStatic* item;
    CIFBarWnd* TakeBar;
    std::n_wstring SelectedItemName;
    int MallID;
    int FullPriceForPacket;
    void UpdateList();
};