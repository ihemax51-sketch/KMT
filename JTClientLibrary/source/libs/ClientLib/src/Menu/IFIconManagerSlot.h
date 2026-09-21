#pragma once
#include <IFBarWnd.h>
#include "IFMainFrame.h"
#include "IFSlotWithHelp.h"
#include "IFTextBox.h"
class CIFIconManagerSlot : public CIFWnd {
GFX_DECLARE_DYNCREATE(CIFIconManagerSlot)
GFX_DECLARE_MESSAGE_MAP(CIFIconManagerSlot)
public:
    CIFIconManagerSlot(void);
    ~CIFIconManagerSlot(void);

    bool OnCreate(long ln) override;
    void LoadItems(byte DBID, std::n_wstring ItemName, int side);
    void ClearDDJ();
    int OnMouseLeftUp(int a1, int x, int y) override;
public:
    int ItemDBID;
    byte SideValue;

void Clear();
};
