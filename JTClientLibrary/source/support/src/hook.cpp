#include "hook.h"
#include <Windows.h>
#include <stdio.h>

void placeHook(int trampoline_location, int target_location) {
    unsigned char jmp_inst[] = {0xE9, 0x00, 0x00, 0x00, 0x00};
    int distance;
    DWORD dwProtect = 0;

    distance = target_location - trampoline_location - 5;

    // Write jump-distance to instruction
    memcpy((jmp_inst + 1), &distance, 4);

    if (!VirtualProtect((LPVOID) trampoline_location, sizeof(jmp_inst), PAGE_EXECUTE_READWRITE, &dwProtect)) {
        perror("Failed to unprotect memory\n");
        return;
    }

    memcpy((LPVOID) trampoline_location, jmp_inst, sizeof(jmp_inst));
    FlushInstructionCache(GetCurrentProcess(), (LPCVOID) trampoline_location, sizeof(jmp_inst));

    DWORD otherProtect;
    if (!VirtualProtect((LPVOID) trampoline_location, sizeof(jmp_inst), dwProtect, &otherProtect)) {
        perror("Failed to restore protection on memory");
    }
}

void replaceOffset(int trampoline_location, int target_location) {

    char inst_offset[] = {0x00, 0x00, 0x00, 0x00};
    int distance;
    DWORD dwProtect = 0;

    int offset_location = trampoline_location + 1;

    distance = target_location - trampoline_location - 5;

    // Write jump-distance to instruction
    memcpy(inst_offset, &distance, 4);

    if (!VirtualProtect((LPVOID) offset_location, sizeof(inst_offset), PAGE_EXECUTE_READWRITE, &dwProtect)) {
        perror("Failed to unprotect memory\n");
        return;
    }

    memcpy((LPVOID) offset_location, inst_offset, sizeof(inst_offset));
    FlushInstructionCache(GetCurrentProcess(), (LPCVOID) offset_location, sizeof(inst_offset));

    DWORD otherProtect;
    if (!VirtualProtect((LPVOID) offset_location, sizeof(inst_offset), dwProtect, &otherProtect)) {
        perror("Failed to restore protection on memory");
    }
}

void replaceAddr(int addr, int value) {
    DWORD dwProtect;

    if (!VirtualProtect((LPVOID) addr, sizeof(int), PAGE_EXECUTE_READWRITE, &dwProtect)) {
        perror("Failed to unprotect memory\n");
        return;
    }

    *((int *) addr) = value;
    FlushInstructionCache(GetCurrentProcess(), (LPCVOID) addr, sizeof(int));

    DWORD otherProtect;
    if (!VirtualProtect((LPVOID) addr, sizeof(int), dwProtect, &otherProtect)) {
        perror("Failed to restore protection on memory");
    }
}

void vftableHook(unsigned int vftable_addr, int offset, int target_func) {
    replaceAddr(vftable_addr + offset * sizeof(void *), target_func);
}


void PatchMe(DWORD address, BYTE value) {
    DWORD oldProtect;
    void *addr = reinterpret_cast<void *>(address);
    if (VirtualProtect(addr, 1, PAGE_READWRITE, &oldProtect) == FALSE) {
        return;
    }
    *reinterpret_cast<BYTE *>(addr) = value;
    FlushInstructionCache(GetCurrentProcess(), addr, 1);
    VirtualProtect(addr, 1, oldProtect, &oldProtect);
}
void PatchJZtoJMP(void* address) {
    DWORD oldProtect;
    VirtualProtect(address, 2, PAGE_EXECUTE_READWRITE, &oldProtect); // Bellek korumasını değiştir
    unsigned char patch[] = { 0xEB, 0x16 }; // JMP opcode'u
    memcpy(address, patch, sizeof(patch)); // Opcode'u değiştir
    VirtualProtect(address, 2, oldProtect, &oldProtect); // Eski korumayı geri yükle
}

void Patch(char *dst, char *src, int size) {
    DWORD oldprotect;
    VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);
    memcpy(dst, src, size);
    VirtualProtect(dst, size, oldprotect, &oldprotect);
}

bool RenderNop(void *addr, int count) {
    DWORD oldProtect;
    if (!VirtualProtect(addr, count, PAGE_EXECUTE_READWRITE, &oldProtect))
        return false;

    memset(addr, 0x90, count);
    FlushInstructionCache(GetCurrentProcess(), addr, count);

    VirtualProtect(addr, count, oldProtect, &oldProtect);

    return true;
}


bool CopyBytes(int dst, const void *src, size_t size) {
    return CopyBytes(reinterpret_cast<void *>(dst), src, size);
}


bool CopyBytes(void *dst, const void *src, size_t size) {
    DWORD oldProtect;
    if (VirtualProtect(dst, size, PAGE_READWRITE, &oldProtect) == FALSE)
        return false;
    memcpy(dst, src, size);
    FlushInstructionCache(GetCurrentProcess(), dst, size);
    if (VirtualProtect(dst, size, oldProtect, &oldProtect) == FALSE)
        return false;
    return true;
}

void CALLFunction(int address, int jumpto) {
    try {
        int res = 0;
        char instruction[5];
        RenderCALLInstruction(address, jumpto, instruction);
        DWORD oldprot, dummy;

        res = VirtualProtect((void *) address, 5, PAGE_EXECUTE_READWRITE, &oldprot);

        if (res == 0) {
            throw -1;
        }
        memcpy((LPVOID) address, (LPVOID) instruction, 5);
        FlushInstructionCache(GetCurrentProcess(), (LPCVOID) address, 5);

        res = VirtualProtect((void *) address, 5, oldprot, &dummy);
        if (res == 0) {
            throw -2;
        }

    } catch (int ex) { printf("Detour::CALLFUNCTION failed with ex [%d]", ex); }
}

void RenderCALLInstruction(int address, int jumpto, char *buf) {
    try {
        int offset = (int) jumpto - ((int) address + 5);
        buf[0] = (char) 0xE8;
        *(DWORD *) (buf + 1) = offset;
    } catch (int ex) { printf("Detour::RenderCallInstruction failed with ex [%d]", ex); }
}
bool Write(uintptr_t offset, const void *data, int length) {
    LPVOID lpOffset = reinterpret_cast<LPVOID>(offset);
    DWORD dwOldProtect = 0;
    if (!VirtualProtect(lpOffset, length, PAGE_READWRITE, &dwOldProtect))
        return false;

    memcpy(lpOffset, data, length);
    FlushInstructionCache(GetCurrentProcess(), lpOffset, length);

    return VirtualProtect(lpOffset, length, dwOldProtect, &dwOldProtect);
}
void JMPFunction(int address, int jumpto) {
    try {
        bool res = false;
        char instruction[5];
        RenderJMPInstruction(address, jumpto, instruction);
        DWORD oldprot, dummy;
        res = VirtualProtect((void *) address, 5, PAGE_EXECUTE_READWRITE, &oldprot);
        if (res == false) {
            throw -1;
        }
        memcpy((LPVOID) address, (LPVOID) instruction, 5);
        FlushInstructionCache(GetCurrentProcess(), (LPCVOID) address, 5);

        res = VirtualProtect((void *) address, 5, oldprot, &dummy);
        if (res == false) {
            throw -2;
        }
    } catch (int ex) { printf("Detour::JMPFunction failed with ex [%d]", ex); }
}

void RenderJMPInstruction(int address, int jumpto, char *buf) {
    try {
        int offset = (int) jumpto - ((int) address + 5);
        buf[0] = (char) 0xE9;
        *(DWORD *) (buf + 1) = offset;
    } catch (int ex) { printf("Detour::RenderJMPInstruction failed with ex [%d]", ex); }
}
