#pragma once
#include <IFTextBox.h>
#include "IFMainFrame.h"

class CIFVItemMallBuyItem : public CIFMainFrame
{
GFX_DECLARE_DYNCREATE(CIFVItemMallBuyItem)
GFX_DECLARE_MESSAGE_MAP(CIFVItemMallBuyItem)

private:
    CIFVItemMallBuyItem(void);
    ~CIFVItemMallBuyItem(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
public:
    void UpdateMenuSize();

    int IconID;
    CIFTextBox* IconName;
    CIFSlotWithHelp* m_itemInfo;

    int Quan;
    int Pricex;
    int MallID;
private:
    void On_BtnClick_1();

    void On_BtnClick_2();
    void AddQuantityBtn();
    void RemoveQuantityBtn();

};
