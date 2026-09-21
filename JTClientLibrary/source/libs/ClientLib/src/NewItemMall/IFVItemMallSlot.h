#pragma once
#include <IFSlotWithHelp.h>
#include "IFMainFrame.h"
#include "IFBarWnd.h"
#include "IFTextBox.h"
class CIFVItemMallSlot : public CIFWnd {
GFX_DECLARE_DYNCREATE(CIFVItemMallSlot)
GFX_DECLARE_MESSAGE_MAP(CIFVItemMallSlot)
private:
    CIFVItemMallSlot(void);
    ~CIFVItemMallSlot(void);

    bool OnCreate(long ln) override;
    void Takebtn();
    void AddList();
public:
    void FillTheSlot(int itemIDint, int itemCount);
    void OnUpdate() override;
    void AddMarketItems(int ID, int ItemID, int ItemCount, int Price, int Payment);

public:
    int IconID;
    int PriceSilk;
    CIFSlotWithHelp * m_itemInfo;
    int DataQuantity;
    int ItemID;

};
