#include "MemoryUtility.h"

void* CMemoryUtility::m_tmpProtectAddr;
DWORD CMemoryUtility::m_tmpProtect;
size_t CMemoryUtility::m_tmpProtectSize;

bool CMemoryUtility::UnProtect(void* addr, size_t count)
{
    if (VirtualProtect(addr, count, PAGE_READWRITE, &m_tmpProtect) == FALSE)
        return false;

    m_tmpProtectAddr = addr;
    m_tmpProtectSize = count;

    return true;
}

bool CMemoryUtility::ReProtect()
{
    //restore original protection mode
    return VirtualProtect(m_tmpProtectAddr, m_tmpProtectSize, m_tmpProtect, &m_tmpProtect) != FALSE;
}


bool CMemoryUtility::Nop(uintptr_t offset, size_t count)
{
    LPVOID lpOffset = reinterpret_cast<LPVOID>(offset);
    DWORD dwOldProtect = 0;

    if (!VirtualProtect(lpOffset, count, PAGE_READWRITE, &dwOldProtect))
        return false;

    memset(lpOffset, MEMUTIL_ASM_OPCODE_NOP, count);

    return VirtualProtect(lpOffset, count, dwOldProtect, &dwOldProtect);
}

bool CMemoryUtility::SetupHook(E_MEMUTUL_HOOK_TYPE::Enum type, uintptr_t src, uintptr_t dest)
{
    DWORD dwOldProtect;
    LPVOID lpSource = reinterpret_cast<LPVOID>(src);
    LPVOID lpDest = reinterpret_cast<LPVOID>(dest);

    if (type == E_MEMUTUL_HOOK_TYPE::LongJump || type == E_MEMUTUL_HOOK_TYPE::LongCall)
    {
        char instruction[5];
        size_t nInstructionSize = sizeof(instruction);

        instruction[0] = (type == E_MEMUTUL_HOOK_TYPE::LongJump) ?
                         MEMUTIL_ASM_OPCODE_LONG_JUMP : MEMUTIL_ASM_OPCODE_LONG_CALL;

        *(DWORD*)(instruction + 1) = (dest - (src + 5));

        if (!VirtualProtect(lpSource, nInstructionSize, PAGE_EXECUTE_READWRITE, &dwOldProtect))
            return false;

        memcpy(lpSource, instruction, nInstructionSize);

        return VirtualProtect(lpSource, nInstructionSize, dwOldProtect, &dwOldProtect);
    }

    return false;
}
