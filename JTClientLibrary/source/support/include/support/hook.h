#pragma once
#include <Windows.h>


#define MEMUTIL_READ_BY_PTR_OFFSET(ptr, offset, type) \
    *(type *) (((uintptr_t) ptr) + offset)

#define MEMUTIL_WRITE_VALUE(type, offset, value) \
    Write<type>(offset, value)

template<typename T>
int addr_from_this(T funptr) {
    union {
        int addr;
        T ptr;
    } myu;

    myu.ptr = funptr;
    return myu.addr;
}


template<typename T>
bool placeHook(int trampoline_location, T &target_location) {
    return placeHook(trampoline_location, reinterpret_cast<int>(&target_location));
}

bool placeHook(int trampoline_location, int target_location);

bool replaceOffset(int trampoline_location, int target_location);

bool replaceAddr(int addr, int value);

bool vftableHook(unsigned int vftable_addr, int offset, int target_func);

bool PatchMe(DWORD address, BYTE value);
bool PatchJZtoJMP(void* address);
bool Patch(char *dst, const char *src, int size);

bool RenderNop(void *addr, int count);


bool CopyBytes(int dst, const void *src, size_t size);
bool CopyBytes(void *dst, const void *src, size_t size);

template<typename T>
bool Write(uintptr_t offset, const T &value) {
    return Write(offset, &value, static_cast<int>(sizeof(T)));
}

bool Write(uintptr_t offset, const void *data, int length);

void RenderJMPInstruction(int address, int jumpto, char *buf);
bool JMPFunction(int address, int jumpto);
void RenderCALLInstruction(int address, int jumpto, char *buf);
bool CALLFunction(int address, int jumpto);
