#include "IFStallSlot.h"

#include <support/hook.h>

extern bool g_bCurrentStallUsesSilk;

void CIFStallSlot::sub_6CB8A0(long a2, int a3)
{
    if (g_bCurrentStallUsesSilk)
    {
        PatchMe(0x006CB8F7 + 1, 0x3C);
        PatchMe(0x006CB8F7 + 2, 0xB6);
        PatchMe(0x006CB8F7 + 3, 0xD9);
    }
    else
    {
        PatchMe(0x006CB8F7 + 1, 0x9C);
        PatchMe(0x006CB8F7 + 2, 0x20);
        PatchMe(0x006CB8F7 + 3, 0xD9);
    }

    reinterpret_cast<void (__thiscall *)(CIFStallSlot *, long, int)>(0x006CB8A0)(this, a2, a3);
}
