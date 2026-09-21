#include "IFTimerWnd.h"


GFX_IMPLEMENT_DYNAMIC_EXISTING(CIFTimerWnd, 0x00ee7b20)



void CIFTimerWnd::StartTimerWnd(int time) {
    reinterpret_cast<void(__thiscall *)(CIFTimerWnd *, int)>(0x0055a2d0)(this, time);
}

unsigned int CIFTimerWnd::SetTimer(unsigned int time, unsigned int time2) {
    return reinterpret_cast<unsigned int(__thiscall *)(CIFTimerWnd *, unsigned int, unsigned int)>(0x00566240)(this, time, time2);
}

void CIFTimerWnd::StartTimerWnd2() {
    reinterpret_cast<void(__thiscall *)(CIFTimerWnd *)>(0x00566970)(this);
}
