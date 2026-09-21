//
// Created by kyuubi09 on 3/30/2023.
//

#include "GObjMobile.h"

IMPLEMENT_RUNTIMECLASS_EXISTING(CGObjMobile, 0x00adeba4)


void CGObjMobile::LiveSkill2(int SkillID)
{
    DWORD32* Skillinfo = reinterpret_cast<DWORD32 * (__stdcall*)()>(0x5AA0B0)();
    Skillinfo[2] = SkillID;
    reinterpret_cast<int(__stdcall*)(DWORD32, DWORD32*)>(0x59B480)((DWORD32)this + 0xA30, Skillinfo);
}