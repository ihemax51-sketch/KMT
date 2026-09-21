#pragma once

#include "IFFrame.h"
#include <ghidra/undefined.h>
#include "IFVerticalScroll.h"

class CIFScrollManager  : public CIFFrame {
GFX_DECLARE_DYNAMIC_EXISTING(CIFScrollManager, 0x00eecad4)
public:
    void sub_008124F0(char a1);
    //008124C0
    void sub_008124C0(int a1);
    //008123F0
    void sub_008123F0(int a1);
    //00812500
    void sub_00812500(int a1);
    //00812420
    void sub_00812420(int, int);
    //00812c20
    void AddItem(CIFWnd* item, int a2, int a3);

    //00812ed0
    void DeleteItem(CIFWnd* item);
    void AddItem2(int a1, int a2, int a3);

public:
    char pad_0x07B0[0x24]; //0x07B0
    CIFVerticalScroll* m_verticalScroll; //0x07D4
    char pad_0x07D8[0xC]; //0x07D8



BEGIN_FIXTURE()
        ENSURE_SIZE(2020); // 0x4CC
        ENSURE_OFFSET(m_verticalScroll, 0x07D4);
    END_FIXTURE()
    RUN_FIXTURE(CIFScrollManager)


};
