#pragma once
#include "IFWnd.h"
#include "IFStatic.h"
#include "IFGauge.h"
class CIFStateSlot : public CIFWnd {
GFX_DECLARE_DYNAMIC_EXISTING(CIFStateSlot, 0x00eec948)
public:
    CIFStateSlot();
    ~CIFStateSlot();
    void OnUpdateIMPL();
    bool OnCreateIMPL(long ln);
    void FUN_00806d50(undefined4 param_1, undefined1 param_2, undefined1 param_3, undefined1 param_4, undefined4 p5);
    void FUN_008065b0(undefined4 p1, undefined4 p2);
public:

    char pad_036C[12]; //0x036C
    CIFStatic* static1; //0x0378
    char pad_037C[4]; //0x037C
    CIFStatic *static2; //0x0380
    char pad_0384[4]; //0x0384
    CIFGauge *gauge; //0x0388
    char pad_038C[12]; //0x038C

BEGIN_FIXTURE()
        ENSURE_SIZE(0x398)
        ENSURE_OFFSET(static1, 0x0378)
        ENSURE_OFFSET(static2, 0x0380)
        ENSURE_OFFSET(gauge, 0x0388)
    END_FIXTURE()
    RUN_FIXTURE(CIFStateSlot)



};


