#pragma once

#define HOOK_ORIGINAL_MEMBER(address, func) \
    MemberFunctionHook<address> hook_ ## address ((func));

class MemberFunctionHookRegistry {
public:
    struct Entry {
        unsigned int trampoline;
        int target;
        bool installed;
        Entry* next;
    };

    static void Register(Entry* entry) {
        entry->installed = false;
        entry->next = Head();
        Head() = entry;

        if (Applied()) {
            Apply(entry);
        }
    }

    static void ApplyAll() {
        Applied() = true;
        for (Entry* entry = Head(); entry != 0; entry = entry->next) {
            Apply(entry);
        }
    }

private:
    static Entry*& Head() {
        static Entry* head = 0;
        return head;
    }

    static bool& Applied() {
        static bool applied = false;
        return applied;
    }

    static void Apply(Entry* entry) {
        if (entry == 0 || entry->installed) {
            return;
        }

        unsigned char jmpInst[] = {0xE9, 0x00, 0x00, 0x00, 0x00};
        int distance = entry->target - entry->trampoline - 5;
        DWORD dwProtect = 0;

        memcpy((jmpInst + 1), &distance, 4);

        if (!VirtualProtect((LPVOID) entry->trampoline, sizeof(jmpInst), PAGE_EXECUTE_READWRITE, &dwProtect)) {
            perror("Failed to unprotect memory\n");
            return;
        }

        memcpy((LPVOID) entry->trampoline, jmpInst, sizeof(jmpInst));

        DWORD otherProtect;
        if (!VirtualProtect((LPVOID) entry->trampoline, sizeof(jmpInst), dwProtect, &otherProtect)) {
            perror("Failed to restore protection on memory");
        }

        entry->installed = true;
    }
};

inline void ApplyRegisteredMemberHooks() {
    MemberFunctionHookRegistry::ApplyAll();
}

template<unsigned int Address>
class MemberFunctionHook {
public:
    template<typename FuncPtr>
    explicit MemberFunctionHook(FuncPtr func) {
        static int count = 0;

        if (count != 0) {
            throw "Too many registrations";
        }

        count++;

        union {
            int address;
            FuncPtr ptr;
        } myu;

        myu.ptr = func;

        m_entry.trampoline = Address;
        m_entry.target = myu.address;
        MemberFunctionHookRegistry::Register(&m_entry);
    }

private:
    MemberFunctionHookRegistry::Entry m_entry;
};
