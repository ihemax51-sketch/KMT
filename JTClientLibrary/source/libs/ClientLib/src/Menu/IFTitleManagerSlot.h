#pragma once
#include <IFBarWnd.h>
#include "IFMainFrame.h"
#include "IFSlotWithHelp.h"
#include "IFTextBox.h"
class CIFTitleManagerSlot : public CIFWnd {
GFX_DECLARE_DYNCREATE(CIFTitleManagerSlot)
GFX_DECLARE_MESSAGE_MAP(CIFTitleManagerSlot)
public:
    CIFTitleManagerSlot(void);
    ~CIFTitleManagerSlot(void);

    bool OnCreate(long ln) override;
    void LoadItems(byte DBID, const wchar_t * ItemName);
    void LoadItemsColors(byte DBID, std::wstring ItemName, unsigned __int32 Color);
    void ClearDDJ();
    int OnMouseLeftUp(int a1, int x, int y) override;
    void Clear();
public:
    int ItemDBID;

};
