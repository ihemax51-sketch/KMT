#include "IFScrollManager.h"

// 0x008124F0
void CIFScrollManager::sub_008124F0(char a1) {
    reinterpret_cast<void(__thiscall*)(CIFScrollManager*, int)>(0x008124F0)(this, a1);
}

// 0x008124C0
void CIFScrollManager::sub_008124C0(int a1) {
    reinterpret_cast<void(__thiscall*)(CIFScrollManager*, int)>(0x008124C0)(this, a1);
}

// 0x008123F0
void CIFScrollManager::sub_008123F0(int a1) {
    reinterpret_cast<void(__thiscall*)(CIFScrollManager*, int)>(0x008123F0)(this, a1);
}

// 00812500
void CIFScrollManager::sub_00812500(int a1) {
    reinterpret_cast<void(__thiscall*)(CIFScrollManager*, int)>(0x00812500)(this, a1);
}

// 00812420
void CIFScrollManager::sub_00812420(int a1, int a2) {
    reinterpret_cast<void(__thiscall*)(CIFScrollManager*, int, int)>(0x00812420)(this, a1, a2);
}

// 0x00812c20
void CIFScrollManager::AddItem(CIFWnd* item, int a2, int a3) {
    reinterpret_cast<void(__thiscall*)(CIFScrollManager*, CIFWnd*, int, int)>(0x00812c20)(this, item, a2, a3);
}
void CIFScrollManager::AddItem2(int a1, int a2, int a3) {
    reinterpret_cast<void(__thiscall*)(CIFScrollManager*, int, int, int)>(0x00812c20)(this, a1, a2, a3);
}
void CIFScrollManager::DeleteItem(CIFWnd* item) {
    sub_008124F0(0);
    reinterpret_cast<void(__thiscall*)(CIFScrollManager*, CIFWnd*)>(0x00812ed0)(this, item);
    item->ShowGWnd(0);
}
