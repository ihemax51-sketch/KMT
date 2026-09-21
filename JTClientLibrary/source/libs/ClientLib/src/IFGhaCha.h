#pragma


#include "IFMainFrame.h"
#include "IFSlotWithHelp.h"

class CIFGhaCha : public CIFMainFrame {
public:
    void PlayButton();
    void OnTimerIMPL(int timerId);
private:
    char pad_07BC[200]; //0x07BC
    CIFSlotWithHelp *MagicPopSlot; //0x0884
    char pad_0888[48]; //0x0888
    CIFButton *PlayButtonMaybe; //0x08B8
    void *CIFStatic1; //0x08BC
    void *CIFStatic2; //0x08C0
    void *CIFStatic3; //0x08C4
    void *CIFStatic4; //0x08C8
    void *CIFStatic5; //0x08CC
    void *CIFDecoStatic; //0x08D0
    void *CIFSpinButton; //0x08D4
    char pad_08D8[72]; //0x08D8

BEGIN_FIXTURE()
        ENSURE_SIZE(0x0920)
        ENSURE_OFFSET(m_pTitleText, 0x7b0)
        ENSURE_OFFSET(m_pHandleBar, 0x7b4)
        ENSURE_OFFSET(m_pCloseBtn, 0x7b8)
    END_FIXTURE()

    RUN_FIXTURE(CIFGhaCha)
};

