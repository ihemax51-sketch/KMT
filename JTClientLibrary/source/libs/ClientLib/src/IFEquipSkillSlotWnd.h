#pragma once
#include "IFSkillSlot.h"
#include "IFWnd.h"
#include "IFStatic.h"


class CIFEquipSkillSlotWnd : public CIFWnd
{
public:
    CIFSkillSlot *CIFSkillSlot1; //0x036C
    CIFSkillSlot *CIFSkillSlot2; //0x0370
    CIFSkillSlot *CIFSkillSlot3; //0x0374
    CIFSkillSlot *CIFSkillSlot4; //0x0378
    CIFStatic *CIFStatic1; //0x037C
    CIFStatic *CIFStatic2; //0x0380
    CIFStatic *CIFStatic3; //0x0384
    CIFStatic *CIFStatic4; //0x0388
}; //Size: 0x038C