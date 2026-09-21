#pragma once

#include "Data/SkillData.h"
#include "IFSlotWithHelp.h"
#include "IFWnd.h"

class CIFSkillSlot : public CIFWnd
{
public:
    char pad_036C[40]; //0x036C
    int N0000267F; //0x0394
    int SkillID; //0x0398
    int N00002681; //0x039C
    char pad_03A0[8]; //0x03A0
    void *Button; //0x03A8
    CIFSlotWithHelp *m_pSlot; //0x03AC
    char pad_03B0[44]; //0x03B0
    CSkillData *SkillData; //0x03DC
}; //Size: 0x03E0