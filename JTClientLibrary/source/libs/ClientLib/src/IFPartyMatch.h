#include "IFFrame.h"
#include "IFHelperBubbleWindow.h"
#include "IFPartyMatchSlot.h"
#include "IFMainFrame.h"

class CIFPartyMatch : public CIFMainFrame {
GFX_DECLARE_DYNAMIC_EXISTING(CIFPartyMatch, 0x00EEB8CC)

    undefined1 OnCloseWndIMPL();

//FUN_00740870
//FUN_00740750
    void OnUpdateIMPL();
    void FUN_00740910(BYTE PatyType);
    void FUN_00740870(char param_1);
    void FUN_00740750(CIFHelperBubbleWindow* window);
    int OnMouseLeftUpIMPL(int a1, int x, int y) ;
    bool OnCreateIMPL(long ln);
    void FUN_00740d30(int p1, int p2);
public:
    char pad_07BC[36]; //0x07BC
    std::list<CIFPartyMatchSlot*> m_SlotList; //0x07E0
    CIFPartyMatchSlot * m_PartyMatchSlotSelected; //0x07EC
    char pad_07F0[16]; //0x07F0

BEGIN_FIXTURE()
        ENSURE_SIZE(2048)
        ENSURE_OFFSET(m_PartyMatchSlotSelected, 0x7EC)
    END_FIXTURE()

    RUN_FIXTURE(CIFPartyMatch)


}; //size 0x800

