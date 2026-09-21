#pragma once
#include <IFBarWnd.h>
#include "IFMainFrame.h"
#include "IFSlotWithHelp.h"
#include "IFTextBox.h"

class CIFMacroMenuAutoSkillPartySlot : public CIFWnd {
GFX_DECLARE_DYNCREATE(CIFMacroMenuAutoSkillPartySlot)
GFX_DECLARE_MESSAGE_MAP(CIFMacroMenuAutoSkillPartySlot)
public:
    CIFMacroMenuAutoSkillPartySlot(void);
    ~CIFMacroMenuAutoSkillPartySlot(void);

    bool OnCreate(long ln) override;
    void LoadItems(std::n_wstring CharName);
    void ClearDDJ();
    int OnMouseLeftUp(int a1, int x, int y) override;
    CIFStatic* m_CharnamePtr;
};
