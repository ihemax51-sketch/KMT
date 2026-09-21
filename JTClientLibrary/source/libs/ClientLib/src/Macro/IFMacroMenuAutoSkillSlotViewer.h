#pragma once
#include <IFSlotWithHelp.h>
#include "IFMainFrame.h"
#include "IFBarWnd.h"
#include "IFMacroMenuAutoSkillSlotSlot.h"

class CIFMacroMenuAutoSkillSlotViewer : public CIFWnd {
GFX_DECLARE_DYNCREATE(CIFMacroMenuAutoSkillSlotViewer)
GFX_DECLARE_MESSAGE_MAP(CIFMacroMenuAutoSkillSlotViewer)
private:
    CIFMacroMenuAutoSkillSlotViewer(void);
    ~CIFMacroMenuAutoSkillSlotViewer(void);

    bool OnCreate(long ln) override;
public:
    CIFMacroMenuAutoSkillSlotSlot*slot1[8];
};
