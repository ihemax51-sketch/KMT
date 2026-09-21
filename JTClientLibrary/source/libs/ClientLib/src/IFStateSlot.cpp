//
// Created by YUMBUL on 21.10.2022.
//

#include <CustomData/CustomSettingManager.h>
#include "IFStateSlot.h"
#include "GInterface.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFStateSlot, 0x00eec948)

CIFStateSlot::CIFStateSlot(){

    reinterpret_cast<CIFStateSlot *(__thiscall *)(CIFStateSlot *)>(0x008064f0)(this);
}
CIFStateSlot::~CIFStateSlot(){
    reinterpret_cast<CIFStateSlot *(__thiscall *)(CIFStateSlot *)>(0x00806a90)(this);
}

bool CIFStateSlot::OnCreateIMPL(long ln){
    bool b = reinterpret_cast<bool(__thiscall *)(CIFStateSlot *, long)>(0x00806560)(this, ln);
    return b;
}

void CIFStateSlot::OnUpdateIMPL()
{
    reinterpret_cast<void (__thiscall *)(const CIFStateSlot *)>(0x006528a0)(this);
}



void CIFStateSlot::FUN_00806d50(undefined4 param_1, undefined1 param_2, undefined1 param_3, undefined1 param_4, undefined4 param_5)
{
    //printf("%d %d %d %d %d\n", param_1, param_2, param_3, param_4, param_5);
    if(m_Settings->EnableServerInfoSkill)
    {
        if(this->UniqueID() == 0)
        {
            this->ShowGWnd(false);
        }
    }

    reinterpret_cast<void*(__thiscall *)(CIFStateSlot *, undefined4,undefined1,undefined1,undefined1,undefined4)>(0x00806d50)(this, param_1, param_2, param_3, param_4, param_5);
    if(m_Settings->EnableServerInfoSkill)
    {
        if(this->UniqueID() == 0)
        {
            this->ShowGWnd(false);
            this->static1->ShowGWnd(false);
            this->static2->ShowGWnd(false);
            this->gauge->ShowGWnd(false);
        }
    }

}

void CIFStateSlot::FUN_008065b0(undefined4 p1, undefined4 p2)
{
    reinterpret_cast<void (__thiscall *)(CIFStateSlot *, undefined4,undefined4)>(0x008065b0)(this, p1, p2);
}