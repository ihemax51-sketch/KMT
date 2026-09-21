#pragma once
#ifndef Q_MEMORY_HELPER_H
#define Q_MEMORY_HELPER_H
#include <Rpc.h>


class MemoryHelper
{
public:
    template<typename T>
    inline static T & RefPtr(uintptr_t ptr)
    {
        return *reinterpret_cast<T*>(ptr);
    }

    template<typename T>
    static int addr_from_this(T funptr) {
        union {
            int addr;
            T ptr;
        } myu;

        myu.ptr = funptr;
        return myu.addr;
    }

    static void SetNop(DWORD addr, size_t Length);
    BOOL SetTrampoline(bool isLongJmp, void* src, void* dest, int nopSize);

    static void replaceAddr(int addr, int value);
    static void replaceOffset(int trampoline_location, int target_location);
    static bool UnProtect(void* addr, size_t count);
    static bool ReProtect();
    static void ReplaceByte(void* addr, int value);
    /*JMP = FALSE CALL = TRUE*/
    static bool Detour(bool jmpOrCall, void* source, void* dest);
    void WriteString(DWORD d_offset, char* s_val);
private:
    static void* m_tmpProtectAddr;
    static DWORD m_tmpProtect;
    static size_t m_tmpProtectSize;

};

#endif
inline void MemoryHelper::ReplaceByte(void* addr, int value)
{
    UnProtect(addr, sizeof(value));
    *(int*)addr = value;
    ReProtect();
}

extern MemoryHelper* g_MemoryHelper;