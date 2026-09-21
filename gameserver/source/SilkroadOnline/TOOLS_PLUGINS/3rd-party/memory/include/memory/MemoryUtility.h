#pragma once

#include <vadefs.h>
#include <Rpc.h>


//Assembly mnemonics
#define ASM_NOP 0x90
#define ASM_JMP 0xE9 //LONG
#define ASM_CALL 0xE8 //LONG

struct E_MEMUTUL_HOOK_TYPE
{
    enum Enum
    {
        LongJump,
        LongCall
    };
};

#define MEMUTIL_ASM_OPCODE_LONG_JUMP			0xE9
#define MEMUTIL_ASM_OPCODE_SHORT_JUMP			0xEB
#define MEMUTIL_ASM_OPCODE_LONG_CALL			0xE8
#define MEMUTIL_ASM_OPCODE_NOP					0x90


#define MEMUTIL_NOP(offset, count) \
	CMemoryUtility::Nop(offset, count)

#define MEMUTIL_READ_BY_PTR_OFFSET(ptr, offset, type) \
	*(type*)(((uintptr_t)ptr) + offset)

#define MEMUTIL_WRITE_BY_PTR_OFFSET(ptr, offset, type, value) \
    *(type*)(((uintptr_t)ptr) + offset) = value;

#define MEMUTIL_SETUP_HOOK(type, src, dest) \
	CMemoryUtility::SetupHook(type, src, dest);

#define MEMUTIL_ADD_PTR(ptr, offset) \
	(((uintptr_t)(ptr)) + offset)

class CMemoryUtility
{
public:
    static bool Nop(uintptr_t offset, size_t count);
    static bool SetupHook(E_MEMUTUL_HOOK_TYPE::Enum type, uintptr_t src, uintptr_t dest);

    static bool UnProtect(void* addr, size_t count);
    static bool ReProtect();
private:
    static void* m_tmpProtectAddr;
    static DWORD m_tmpProtect;
    static size_t m_tmpProtectSize;
};