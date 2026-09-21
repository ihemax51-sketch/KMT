//
// Created by YUMBUL on 18.02.2024.
//

#include "IFCOSCommand.h"

bool CIFCOSCommand::OnCreateIMPL(long ln) {
    bool b = reinterpret_cast<bool(__thiscall *)(CIFCOSCommand *, long)>(0x007a2970)(this, ln);




    return b;
}
void CIFCOSCommand::FUN_007a20a0() {
    reinterpret_cast<void (__thiscall *)(CIFCOSCommand *)>(0x007a20a0)(this);
    wnd_pos x = this->GetPos();
    this->MoveGWnd(x.x+22, x.y+9);
}
