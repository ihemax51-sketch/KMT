#pragma once
#include <Windows.h>
#define MEMUTIL_WRITE_VALUE(type, offset, value) \
	Write<type>(offset, value)
template<typename T>
static bool Write(uintptr_t offset, T& value)
{
    LPVOID lpOffset = reinterpret_cast<LPVOID>(offset);

    DWORD dwOldProtect = 0;
    if (!VirtualProtect(lpOffset, sizeof(T), PAGE_READWRITE, &dwOldProtect))
        return false;

    *(T*)(offset) = value;

    VirtualProtect(lpOffset, sizeof(T), dwOldProtect, &dwOldProtect);
    return true;
}

template<typename T>
int addr_from_this(T funptr) {
    union  {
        int addr;
        T ptr;
    } myu;

    myu.ptr = funptr;
    return myu.addr;
}


template<typename T>
void placeHook(int trampoline_location, T& target_location)
{
    placeHook(trampoline_location, reinterpret_cast<int>(&target_location));
}

void plamceHook(int trampoline_location, int target_location);

void replaceOffset(int trampoline_location, int target_location);
void PatchMe(DWORD address, BYTE value);
void replaceAddr(int addr, int value);

void vftableHook(unsigned int vftable_addr, int offset, int target_func);

bool RenderNop(void* addr, int count);
bool CopyBytes(int dst, const void* src, size_t size);
bool CopyBytes(void* dst, const void* src, size_t size);

// Calls a virtual function from table pointer address
template<typename T>
T CallVirtual(void* vftable_addr, unsigned index)
{
    return (*static_cast<T**>(vftable_addr))[index];
}


#define REMODEL_PLACEHOLDER

#define REMODEL_DECLARE_VIRTUAL_NOIMPL_DCTOR(className) \
	virtual ~className() { };

#define REMODEL_DECLARE_VIRTUAL_NOIMPL(ret_type, fn, ret_value) \
	virtual ret_type fn { return ret_value; }


#define REMODEL_SAVE_REGISTERS_BEFORE_STDCALL \
{\
    __asm push eax   \
    __asm push ecx   \
    __asm push edx   \
}

#define REMODEL_RESTORE_REGISTERS_AFTER_STDCALL \
{\
    __asm pop edx   \
    __asm pop ecx   \
    __asm pop eax   \
}

