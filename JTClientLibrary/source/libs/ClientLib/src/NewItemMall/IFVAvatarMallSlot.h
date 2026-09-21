#pragma once
#include <IFSlotWithHelp.h>
#include "IFMainFrame.h"
#include "IFBarWnd.h"
#include "IFTextBox.h"
class CIFVAvatarMallSlot  : public CIFWnd {
GFX_DECLARE_DYNCREATE(CIFVAvatarMallSlot)
GFX_DECLARE_MESSAGE_MAP(CIFVAvatarMallSlot)
private:
    CIFVAvatarMallSlot(void);
    ~CIFVAvatarMallSlot(void);

    bool OnCreate(long ln) override;
    void Takebtn();
    void AddList();
    int OnMouseLeftUp(int a1, int x, int y) override;
public:
    void FillTheSlot(int itemIDint, int itemCount);
    void OnUpdate() override;
    void AddMarketItems(int ID,  int ItemCount, int Price, int PetObjID);
public:
    int IconID;
    int PriceSilk;
    CIFSlotWithHelp * m_itemInfo;
    int ObjCharID;
};
