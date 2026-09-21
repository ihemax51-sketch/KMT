#include "LocalTime.h"

void CLocalTime::FUN_008b01b0(CLocalTime_Sub *data) {
    reinterpret_cast<void (__thiscall *)(CLocalTime *, CLocalTime_Sub *)>(0x008b01b0)(this, data);
}

/*
// CLocalTime::InitTime
int is_this_important::SetSomething(int someval)
{
	field_0 = someval;
	creation_ticks = GetTickCount();
	field_8 = 0;
	if (1000 * someval / 1440u) // 1440 number of minutes per day
	{
		field_C = 0;
		field_18 = 1;
		return 1;
	}
	return 0;
}
*/
undefined4 CLocalTime::InitTimer(int dwRealTime, short wDay, unsigned char byHour, unsigned char byMin, unsigned char a6) {
    return reinterpret_cast<undefined4(__thiscall *)(CLocalTime *, int, short, unsigned char, unsigned char, unsigned char)>(0x008b00f0)(this, dwRealTime, wDay, byHour, byMin, a6);
}
