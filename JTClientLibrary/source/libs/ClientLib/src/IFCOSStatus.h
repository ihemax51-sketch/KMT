#pragma once
#include "IFWnd.h"
#include "IFGauge.h"
#include "IFSlotWithHelp.h"


class CIFCOSStatus  : public CIFWnd {
GFX_DECLARE_DYNAMIC_EXISTING(CIFCOSStatus, 0x00eec108)

public:
    void OnUpdateIMPL();

public:
    char pad_036C[4]; //0x036C
    byte status; // 0x0370
    byte unk; // 0x0371
    byte unk1; // 0x372
    byte unk2; // 0x373
    int m_petUniqueID; //0x0374
    CIFSlotWithHelp* slot; //0x0378
    CIFGauge* gauge1; //0x037C
    CIFGauge* gauge2; //0x0380
    CIFWnd* wnd1; //0x0384
    CIFWnd *wnd2; //0x0388

BEGIN_FIXTURE()
        ENSURE_SIZE(908)
    END_FIXTURE()

    RUN_FIXTURE(CIFCOSStatus)

}; //0x3a4

