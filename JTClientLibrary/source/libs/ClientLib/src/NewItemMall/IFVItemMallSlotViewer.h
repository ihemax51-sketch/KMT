#pragma once
#include <IFSlotWithHelp.h>
#include "IFMainFrame.h"
#include "IFBarWnd.h"
#include "IFVItemMallSlot.h"

class CIFVItemMallSlotViewer : public CIFWnd {
GFX_DECLARE_DYNCREATE(CIFVItemMallSlotViewer)
GFX_DECLARE_MESSAGE_MAP(CIFVItemMallSlotViewer)
private:
    CIFVItemMallSlotViewer(void);
    ~CIFVItemMallSlotViewer(void);

    bool OnCreate(long ln) override;
public:
    CIFVItemMallSlot* slot1;
    CIFVItemMallSlot* slot2;
    CIFVItemMallSlot* slot3;
};
