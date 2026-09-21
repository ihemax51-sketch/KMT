#pragma once
#include <IFSlotWithHelp.h>
#include "IFMainFrame.h"
#include "IFBarWnd.h"
#include "IFTextBox.h"
class CIFVItemMallSlotNews : public CIFWnd {
GFX_DECLARE_DYNCREATE(CIFVItemMallSlotNews)
GFX_DECLARE_MESSAGE_MAP(CIFVItemMallSlotNews)
private:
    CIFVItemMallSlotNews(void);
    ~CIFVItemMallSlotNews(void);

    bool OnCreate(long ln) override;
    void Takebtn();
    void AddPreset();
    void OnUpdate() override;
public:
    void FillTheSlot(int itemIDint, int itemCount);
    void AddMarketItems(int ID, int ItemID, int ItemCount, int Price, int Payment);
public:
    int IconID;
    CIFSlotWithHelp* m_itemInfo;
    int PriceInts;
    int DataUnit;
    int ItemID;

};
