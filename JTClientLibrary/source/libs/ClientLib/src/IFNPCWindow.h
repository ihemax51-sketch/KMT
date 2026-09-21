#pragma once

#include "IFMainFrame.h"
#include "IFNPCTalk.h"

class CIF_NPCWindow : public CIFMainFrame {
GFX_DECLARE_DYNAMIC_EXISTING(CIF_NPCWindow, 0x00eeb1bc)
public:
    void WriteNpcInfo_Maybe();
    int GetNpcObjID();
    void ShowGWndIMPL(bool bVisible);
    undefined OnCloseWndIMPL();
public:

    int Unk1; //0x07BC
    int NpcObjID; //0x07C0
    char pad_07C4[4]; //0x07C4
    CIF_NPCTalk *PtrCIF_NPCTalk; //0x07C8
    int maybemap; //0x07CC
    char pad_07D0[8]; //0x07D0

BEGIN_FIXTURE()
        ENSURE_SIZE(0x7d8)
        ENSURE_OFFSET(NpcObjID, 0x07C0)
    END_FIXTURE()

    RUN_FIXTURE(CIF_NPCWindow)



};

