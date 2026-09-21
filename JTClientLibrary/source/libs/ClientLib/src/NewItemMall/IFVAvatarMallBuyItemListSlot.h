#pragma once
#include "IFMainFrame.h"
#include "IFBarWnd.h"
#include "IFSlotWithHelp.h"
#include "IFTextBox.h"

class CIFVAvatarMallBuyItemListSlot : public CIFWnd {
GFX_DECLARE_DYNCREATE(CIFVAvatarMallBuyItemListSlot)
GFX_DECLARE_MESSAGE_MAP(CIFVAvatarMallBuyItemListSlot)
public:
    CIFVAvatarMallBuyItemListSlot(void);
    ~CIFVAvatarMallBuyItemListSlot(void);

    bool OnCreate(long ln) override;
    void FillTheSlot(int itemIDint, int itemCount, int OptLevel);
    void TakeBtn();

    void SetName(int ID, int ItemId, int Price);
public:
    CIFSlotWithHelp *m_itemInfo;
    int ItemDBID;

    void ClearSlot();
};
