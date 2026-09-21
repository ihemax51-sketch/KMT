#include "GItem.h"

#define FN_WRITE_ITEM_INFO_TO_MSG_OFFSET	0x0048FB60
#define FN_REFRESH_ITEM_STATS_OFFSET		0x00495CA0


void CGItem::RefreshItemStats()
{
	const DWORD dwCallAddr = FN_REFRESH_ITEM_STATS_OFFSET;

	__asm pushad;
	__asm pushfd;

	__asm xor al, al;
	__asm mov ecx, this;
	__asm call dwCallAddr;

	__asm popfd;
	__asm popad;
}
#define g_CStreamBufferUnk 0xCD7DFC
void CGItem::FlushStreamBufferMsg(CMsg* pMsg)
{
	__asm pushad;
	__asm pushfd;

	__asm mov eax, pMsg;
	__asm mov esi, g_CStreamBufferUnk;
	__asm mov edx, 0x42C820;
	__asm call edx;

	__asm popfd;
	__asm popad;
}
void CGItem::BindStreamBufferWithMsg(CMsg* pMsg)
{
	__asm pushad;
	__asm pushfd;

	__asm mov esi, pMsg;
	__asm mov edi, g_CStreamBufferUnk; //see IDA (CStreamBuffer related)
	__asm mov edx, 0x42C8F0; //call addr
	__asm call edx;

	__asm popfd;
	__asm popad;
}

void CGItem::WriteItemDataToMsg(void* pSteamBufferRelated, int eType)
{
	reinterpret_cast<void(__thiscall*)(CGItem*, void*, int)>(
		FN_WRITE_ITEM_INFO_TO_MSG_OFFSET)(this, pSteamBufferRelated, eType
			);
}

void CGItem::WriteItemCosDataMsg(void* pStreamBuffer, int eType)
{
    reinterpret_cast<void(__thiscall*)(CGItem*, void*, int)>(
            0x00492D40)(this, pStreamBuffer, eType);
}

void CGItem::FUN_0048fb60()
{
	const DWORD dwCallAddr = 0x004984c0;

	__asm pushad;
	__asm pushfd;

	__asm xor al, al;
	__asm mov ecx, this;
	__asm call dwCallAddr;

	__asm popfd;
	__asm popad;
	//printf("%p %d\n", p1, param2); 0x00491890
	//reinterpret_cast<void* (__thiscall*)(CGItem*)>(0x004939dc)(this);
}


void CGItem::FUN_00497340()
{
	reinterpret_cast<void* (__thiscall*)(CGItem*)>(0x00497340)(this);
}
