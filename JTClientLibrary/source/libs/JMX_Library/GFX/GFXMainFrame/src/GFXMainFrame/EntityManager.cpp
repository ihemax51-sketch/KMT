#include "EntityManager.h"

void CEntityManager::DestroyEntityIC(void* p) {
    reinterpret_cast<void(__thiscall*)(CEntityManager*, void*)>(0x00ba3770)(this, p);
}
