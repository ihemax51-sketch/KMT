//
// Created by YUMBUL on 11.11.2024.
//

#include <MacroAlchemy/IFAlchemyMacro.h>
#include "NIFAlchemySubWndType1.h"

GFX_IMPLEMENT_DYNAMIC_EXISTING(CNIFAlchemySubWndType1, 0x00ee8508)

bool CNIFAlchemySubWndType1::OnCreateIMPL(long ln) {
    bool b = reinterpret_cast<bool(__thiscall *)(CNIFAlchemySubWndType1 *, long)>(0x0046c2c0)(this, ln);

    return b;
}
bool CNIFAlchemySubWndType1::OnReleaseIMPL(long ln) {
    bool b = reinterpret_cast<bool(__thiscall *)(CNIFAlchemySubWndType1 *, long)>(0x0046c2d0)(this, ln);

    return b;
}
void CNIFAlchemySubWndType1::OnUpdateIMPL()
{

    reinterpret_cast<void(__thiscall *)(CNIFAlchemySubWndType1 *)>(0x0046cd80)(this);

}
undefined4 CNIFAlchemySubWndType1::OnCloseWndIMPL()
{
    return reinterpret_cast<undefined4(__thiscall *)(CNIFAlchemySubWndType1 *)>(0x0046d7c0)(this);
}