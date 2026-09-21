#pragma once
#include <IFTextBox.h>
#include <IFSlotWithHelp.h>
#include "IFMainFrame.h"


class CIFVItemMallFavList : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFVItemMallFavList)
GFX_DECLARE_MESSAGE_MAP(CIFVItemMallFavList)

private:
    CIFVItemMallFavList(void);
    ~CIFVItemMallFavList(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
public:
    void AddItem(int IconID, int ItemID, int ItemCount, int Price);

    int IconID;
    CIFTextBox* IconName;
    CIFSlotWithHelp* m_itemInfo;

    int Quan;
    int Pricex;
    int MallID;
private:
    void On_BtnClick_1();

    void On_BtnClick_2();

    void FillTheSlot(int itemID, int itemCount);

};
