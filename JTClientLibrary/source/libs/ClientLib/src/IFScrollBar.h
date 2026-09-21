#pragma once

#include "IFWnd.h"

class CIFScrollBar : public CIFWnd {

protected:
    /// \address 006590e0
    void FUN_006590e0(short a2);

public:
    /// \address 00659150
    void FUN_00659150(int a2);

protected:
    /// \address 006592a0
    void FUN_006592a0(int a2, int a3, int a4);

private:
    char pad_36c[0x10]; // 0x036c
protected:
    int m_field_0x37c;
public:
    int pad_0380;   // 4 byte
    int pad_0384;   // 4 byte
    int pad_0388;   // 4 byte
    int pad_038C;   // 4 byte
    int pad_0390;   // 4 byte
    int pad_0394;   // 4 byte
    int m_Size;   // pad_0398
    int m_Size2;   // 4 byte
public:
    int m_field_0x3a0;// 0x03a0
    int m_CurrentIndex;// 0x03a4
    int m_field_0x3a8;// 0x03a8


    BEGIN_FIXTURE()
        ENSURE_SIZE(0x3ac)
        ENSURE_OFFSET(m_field_0x37c, 0x037c)
        ENSURE_OFFSET(m_field_0x3a0, 0x03a0)
        ENSURE_OFFSET(m_CurrentIndex, 0x03a4)
        ENSURE_OFFSET(m_field_0x3a8, 0x03a8)
    END_FIXTURE()

    RUN_FIXTURE(CIFScrollBar)
};
