#pragma once

#include "NIFMainFrame.h"

#include "NIFCommunityWnd.h"

class CNIFGuildWnd : public CNIFMainFrame {
private:
    char pad_0798[12]; //0x0798
public:
    CNIFCommunityWnd *m_pCommunity; //0x07A4
private:
    char pad_07a8[0x0840 - 0x07a8]; //0x07A8
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0840)
        ENSURE_OFFSET(m_pCommunity, 0x07A4)
    END_FIXTURE()

    RUN_FIXTURE(CNIFGuildWnd)
};