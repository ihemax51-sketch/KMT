#include "hook.h"
#include <Windows.h>
#include <new>
#include <stdio.h>

namespace {
bool WriteVerified(void* destination, const void* data, size_t size, DWORD protection) {
    if (!destination || !data || size == 0)
        return false;
    DWORD oldProtection = 0;
    if (!VirtualProtect(destination, size, protection, &oldProtection))
        return false;
    memcpy(destination, data, size);
    const bool matches = memcmp(destination, data, size) == 0;
    const BOOL flushed = FlushInstructionCache(GetCurrentProcess(), destination, size);
    DWORD ignored = 0;
    const BOOL restored = VirtualProtect(destination, size, oldProtection, &ignored);
    return matches && flushed != FALSE && restored != FALSE;
}
}

bool placeHook(int trampoline_location, int target_location) {
    unsigned char jmp_inst[] = {0xE9, 0x00, 0x00, 0x00, 0x00};
    int distance;

    distance = target_location - trampoline_location - 5;

    // Write jump-distance to instruction
    memcpy((jmp_inst + 1), &distance, 4);

    const BYTE currentOpcode = *reinterpret_cast<const BYTE*>(trampoline_location);
    if (currentOpcode == 0xE9) {
        const int currentDistance = *reinterpret_cast<const int*>(trampoline_location + 1);
        return currentDistance == distance;
    }

    return WriteVerified((LPVOID)trampoline_location, jmp_inst, sizeof(jmp_inst), PAGE_EXECUTE_READWRITE);
}

bool replaceOffset(int trampoline_location, int target_location) {

    char inst_offset[] = {0x00, 0x00, 0x00, 0x00};
    int distance;

    int offset_location = trampoline_location + 1;

    distance = target_location - trampoline_location - 5;

    // Write jump-distance to instruction
    memcpy(inst_offset, &distance, 4);

    const BYTE opcode = *reinterpret_cast<const BYTE*>(trampoline_location);
    if (*reinterpret_cast<const int*>(offset_location) == distance)
        return true;
    // Supported native call sites are CALL rel32. A JMP or any other opcode
    // here represents foreign/unknown ownership and must not be overwritten.
    if (opcode != 0xE8)
        return false;
    return WriteVerified((LPVOID)offset_location, inst_offset, sizeof(inst_offset), PAGE_EXECUTE_READWRITE);
}

bool replaceAddr(int addr, int value) {
    if (*reinterpret_cast<const int*>(addr) == value)
        return true;
    return WriteVerified((LPVOID)addr, &value, sizeof(value), PAGE_EXECUTE_READWRITE);
}

bool vftableHook(unsigned int vftable_addr, int offset, int target_func) {
    return replaceAddr(vftable_addr + offset * sizeof(void *), target_func);
}


bool PatchMe(DWORD address, BYTE value) {
    if (*reinterpret_cast<const BYTE*>(address) == value)
        return true;
    return WriteVerified(reinterpret_cast<void*>(address), &value, 1, PAGE_EXECUTE_READWRITE);
}
bool PatchJZtoJMP(void* address) {
    unsigned char patch[] = { 0xEB, 0x16 }; // JMP opcode'u
    return WriteVerified(address, patch, sizeof(patch), PAGE_EXECUTE_READWRITE);
}

bool Patch(char *dst, const char *src, int size) {
    return size > 0 && WriteVerified(dst, src, static_cast<size_t>(size), PAGE_EXECUTE_READWRITE);
}

bool RenderNop(void *addr, int count) {
    if (!addr || count <= 0)
        return false;
    unsigned char* bytes = new (std::nothrow) unsigned char[count];
    if (!bytes)
        return false;
    memset(bytes, 0x90, count);
    const bool result = WriteVerified(addr, bytes, count, PAGE_EXECUTE_READWRITE);
    delete[] bytes;
    return result;
}


bool CopyBytes(int dst, const void *src, size_t size) {
    return CopyBytes(reinterpret_cast<void *>(dst), src, size);
}


bool CopyBytes(void *dst, const void *src, size_t size) {
    return WriteVerified(dst, src, size, PAGE_EXECUTE_READWRITE);
}

bool CALLFunction(int address, int jumpto) {
        char instruction[5];
        RenderCALLInstruction(address, jumpto, instruction);
        return WriteVerified((void*)address, instruction, 5, PAGE_EXECUTE_READWRITE);
}

void RenderCALLInstruction(int address, int jumpto, char *buf) {
    try {
        int offset = (int) jumpto - ((int) address + 5);
        buf[0] = (char) 0xE8;
        *(DWORD *) (buf + 1) = offset;
    } catch (int ex) { printf("Detour::RenderCallInstruction failed with ex [%d]", ex); }
}
bool Write(uintptr_t offset, const void *data, int length) {
    return length > 0 && WriteVerified(reinterpret_cast<void*>(offset), data,
        static_cast<size_t>(length), PAGE_EXECUTE_READWRITE);
}
bool JMPFunction(int address, int jumpto) {
        char instruction[5];
        RenderJMPInstruction(address, jumpto, instruction);
        return WriteVerified((void*)address, instruction, 5, PAGE_EXECUTE_READWRITE);
}

void RenderJMPInstruction(int address, int jumpto, char *buf) {
    try {
        int offset = (int) jumpto - ((int) address + 5);
        buf[0] = (char) 0xE9;
        *(DWORD *) (buf + 1) = offset;
    } catch (int ex) { printf("Detour::RenderJMPInstruction failed with ex [%d]", ex); }
}
