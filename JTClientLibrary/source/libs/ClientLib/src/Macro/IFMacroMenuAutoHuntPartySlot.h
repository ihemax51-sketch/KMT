#pragma once
#include <IFBarWnd.h>
#include "IFMainFrame.h"
#include "IFSlotWithHelp.h"
#include "IFTextBox.h"

class CIFMacroMenuAutoHuntPartySlot : public CIFWnd {
GFX_DECLARE_DYNCREATE(CIFMacroMenuAutoHuntPartySlot)
GFX_DECLARE_MESSAGE_MAP(CIFMacroMenuAutoHuntPartySlot)
public:
    CIFMacroMenuAutoHuntPartySlot(void);
    ~CIFMacroMenuAutoHuntPartySlot(void);

    bool OnCreate(long ln) override;
    void LoadItems(std::n_wstring CharName);
    void ClearDDJ();
    int OnMouseLeftUp(int a1, int x, int y) override;
    CIFStatic* m_CharnamePtr;
    void SetBarSize(int widht, int height);
};
