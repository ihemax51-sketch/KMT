#pragma once

#include "NIFGauge.h"
#include "NIFStatic.h"
#include "NIFWnd.h"

#include "IFSlotWithHelpEx.h"

// https://www.elitepvpers.com/forum/sro-pserver-guides-releases/4256375-source-fix-old-exp-bar-writing-code.html
class CNIFUnderMenuBar : public CNIFWnd {
public:
    void Update();

    /// \address 0060b1d0
    bool IsPotionOrPillInQuickslot(int slot);
    void PressButtonAutoPotion();
    /// \address 0060b850
    void UseSlot(int slot);
    void SaveQuickslotData(int byIndexSot);
    void UpdateSkillSlot(unsigned int p1, unsigned int p2);
    void FindAndUpdate(int a1, int a2, int a3);

    void FUN_0060e540(byte p1, byte p2, byte p3, byte p4, byte p5);

    void Fun_0060a940(int byte);
    void LoadSlots(byte SlotSetq, undefined4 SlotType, undefined4 Data);
    bool Func_28_IMPL(int a1, int a2, int a3);
    void GetSlotUseItem(undefined4 Slots);
public:
    char padx[8];
    CNIFGauge *gauge_skillexp; //0x0350
    CNIFStatic *lbl_percentage; //0x0354
    CNIFStatic *lbl_spcount; //0x0358
    CNIFStatic *lbl_exp_bar_scaler; //0x035C
    CNIFStatic *lbl_360; //0x0360
    CNIFStatic *lbl_percent_bar; //0x0364
    CNIFStatic *lbl_level; //0x0368
    CNIFGauge *gauges[10]; //0x036C
    char pad_0394[4 * 16]; //0x0394
    CIFSlotWithHelpEx *m_pMySlots[51]; //0x03D4 //yep 50 slots ;-;
    char pad_cnifundermenubar[8];

private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x4a8)
        ENSURE_OFFSET(m_pMySlots, 0x3d4)
    END_FIXTURE()

    RUN_FIXTURE(CNIFUnderMenuBar)



};