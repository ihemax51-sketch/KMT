#include "PSOuterInterface.h"
#include "Game.h"

GFX_IMPLEMENT_RUNTIMECLASS_EXISTING(CPSOuterInterface, 0x00EED954);


CPSOuterInterface::CPSOuterInterface() {
    reinterpret_cast<void (__thiscall *)(CPSOuterInterface *)>(0x008629A0)(this);
}

void CPSOuterInterface::OnTimer(int a1) {
    reinterpret_cast<void (__thiscall *)(CPSOuterInterface *, int)>(0x00860C60)(this, a1);
}

void CPSOuterInterface::Func_40() {
    // Call the original game handler which shows the in-game disconnect dialog (TDIH).
    // Our hooked SetMsgBoxHandler(Id=1, a3=0) will add the Restart button to it.
    reinterpret_cast<void (__thiscall *)(CPSOuterInterface *)>(0x00862450)(this);
}

void CPSOuterInterface::Handle_0xFFC(CMsgStreamBuffer *p_msg) {
    reinterpret_cast<void (__thiscall *)(CPSOuterInterface *, CMsgStreamBuffer *)>(0x008611D0)(this, p_msg);
}

void CPSOuterInterface::OnUpdate() {
    reinterpret_cast<void (__thiscall *)(CPSOuterInterface *)>(0x00860CC0)(this);
}

void CPSOuterInterface::RenderMyself() {
    reinterpret_cast<void (__thiscall *)(CPSOuterInterface *)>(0x00860ED0)(this);
}

void CPSOuterInterface::WaitGWnd(bool a1) {
    reinterpret_cast<void (__thiscall *)(CPSOuterInterface *, bool)>(0x00862410)(this, a1);
}
