#pragma once

#include "IFButton.h"
#include "IFScrollBar.h"

class CIFVerticalScroll : public CIFScrollBar {
GFX_DECLARE_DYNAMIC_EXISTING(CIFVerticalScroll, 0x00ee9968)
public:
    /// \address 0065A390
    void sub_65A390(RECT a2);

    /// \address 0065A500
    void sub_65A500(short a2);

    /// \address 0065A5A0
    void sub_65A5A0(int a2);

    /// \address 0065A5C0
    void sub_65A5C0(int a2);

public:
    CIFButton *m_arrowUpButton;  //0x03AC
    CIFButton *m_arrowDownButton;//0x03B0
    CIFButton *m_scrollButton;   //0x03B4
    char pad_0x03B8[0x4];        //0x03B8

    BEGIN_FIXTURE()
        ENSURE_SIZE(0x3bc)
        ENSURE_OFFSET(m_arrowUpButton, 0x03AC)
        ENSURE_OFFSET(m_arrowDownButton, 0x03B0)
        ENSURE_OFFSET(m_scrollButton, 0x03B4)
    END_FIXTURE()

    RUN_FIXTURE(CIFVerticalScroll)
};
