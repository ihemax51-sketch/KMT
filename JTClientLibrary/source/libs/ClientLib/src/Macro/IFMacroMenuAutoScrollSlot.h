#pragma once

#include <IFWnd.h>
#include "IFMacroSCSlot.h"
class CIFMacroMenuAutoScrollSlot : public CIFWnd
{
GFX_DECLARE_DYNCREATE(CIFMacroMenuAutoScrollSlot)
GFX_DECLARE_MESSAGE_MAP(CIFMacroMenuAutoScrollSlot)
private:
    CIFMacroMenuAutoScrollSlot(void);
    ~CIFMacroMenuAutoScrollSlot(void);
public:
    void ClearSlots();
    CIFMacroSCSlot* Slot1[8];
    void AutoScrolling();
    bool Macro_AutoScroll;
    bool AutoScrollTimerRunning;
    unsigned long m_lastUseTick[8];
private:
    bool OnCreate(long ln) override;
    void OnUpdate() override;



};

