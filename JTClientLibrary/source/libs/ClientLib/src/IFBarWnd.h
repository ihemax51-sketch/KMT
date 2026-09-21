#pragma once

#include "IFWnd.h"
#include "IFMainFrame.h"

class CIFBarWnd : public CIFWnd {
GFX_DECLARE_DYNAMIC_EXISTING(CIFBarWnd, 0x00eecaf4)
public:
BEGIN_FIXTURE()
        ENSURE_SIZE(876)
    END_FIXTURE()
    RUN_FIXTURE(CIFBarWnd)

};
