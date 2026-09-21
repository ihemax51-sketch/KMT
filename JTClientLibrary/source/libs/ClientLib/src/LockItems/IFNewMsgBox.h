#pragma once
#include <IFTextBox.h>
#include "IFMainFrame.h"
#include "IFNormalTile.h"


class CIFNewMsgBox : public CIFMainFrame {
GFX_DECLARE_DYNCREATE(CIFNewMsgBox)
GFX_DECLARE_MESSAGE_MAP(CIFNewMsgBox)

private:
    CIFNewMsgBox(void);
    ~CIFNewMsgBox(void);
    bool OnCreate(long ln) override;
    void OnUpdate() override;
    void On_BtnClick_1();
    void On_BtnClick_2();
public:
    void SetItemInfo(int ItemSlotIDs, unsigned short ItemTypeIDs);
    void UpdateMenuSize();
    byte Type;
    void SetType(byte WndFrom);
    CSOItem *selectedItem;
    CIFButton* ok;
    CIFButton* cancel;
    CIFTextBox* write;
};
