#pragma once

#include "IObject.h"

class CIOBillboard : public CIObject {
protected:
    /// \address 00a626f0
    void FUN_00a626f0();

private:
    char pad_0x0e0[0x88 - 0x10];// 0xe0

protected:
    D3DCOLOR m_color;// 0x0154

public:
    int pad_0x158;

protected:
    float field_0x15c;

private:
    char pad_0x160[4];

protected:
    float field_0x164;// 0x0164
    float field_0x168;// 0x0168

private:
    char pad_0x16c[4];

private:
BEGIN_FIXTURE()
        ENSURE_OFFSET(m_color, 0x0154)
        ENSURE_OFFSET(field_0x15c, 0x015c)
        ENSURE_OFFSET(field_0x164, 0x0164)
        ENSURE_OFFSET(field_0x168, 0x0168)
        ENSURE_SIZE(0x170)
    END_FIXTURE()

    RUN_FIXTURE(CIOBillboard)
};
