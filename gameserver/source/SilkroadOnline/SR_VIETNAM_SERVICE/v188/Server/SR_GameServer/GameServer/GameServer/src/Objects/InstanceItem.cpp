#pragma once

#include "InstanceItem.h"
#include <memory/MemoryUtility.h>



void CInstanceItem::SetPlus(BYTE btValue)
{
	MEMUTIL_WRITE_BY_PTR_OFFSET(this, 0x2C, BYTE, btValue);
	*(DWORD*)(this + 8) |= 8; //not rly neeeded?
}

