#pragma once

#include "NIFWnd.h"
#include "IFSlotWithHelp.h"


class CIFSlotWithHelpEx : public CNIFWnd {
GFX_DECLARE_DYNAMIC_EXISTING(CIFSlotWithHelpEx, 0x00eea4d0)
public:
    /// \address 00687d40
    bool OnCreate(long ln) override;

    /// \address 00687db0
    void Func_26(int a1) override;
    bool Func_28IMPL(CGWnd* a1, int a2, int a3);
public:
    CIFSlotWithHelp *m_pSlot; //0x0348

private:

};