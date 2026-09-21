#pragma once


#include "IFMainFrame.h"


class CIFGhachaSelectWnd : public CIFMainFrame {
public:
    bool OnCreateIMPL(long ln);
    GFX_MSGMAP* MessageMap();
    void StartGame();
private:
    void *ComboBox; //0x07BC
    void *ComboBox1; //0x07C0
    void *ComboBox2; //0x07C4
    char pad_07C8[32]; //0x07C8

BEGIN_FIXTURE()
        ENSURE_SIZE(0x7e8)
        ENSURE_OFFSET(m_pTitleText, 0x7b0)
        ENSURE_OFFSET(m_pHandleBar, 0x7b4)
        ENSURE_OFFSET(m_pCloseBtn, 0x7b8)
    END_FIXTURE()

    RUN_FIXTURE(CIFGhachaSelectWnd)
    void ClickNone();
    void ClickAutoRefill();
    void ClickAutoPlay();
};
