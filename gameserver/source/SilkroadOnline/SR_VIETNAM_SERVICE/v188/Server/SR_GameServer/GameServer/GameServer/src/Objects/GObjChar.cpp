//
// Created by kyuubi09 on 3/30/2023.
//

#include <memory/hook.h>
#include "GObjChar.h"

#include "StorageOP/GStorageOP.h"
#include "StorageOP/GPCInventoryOP.h"
#include "StorageOP/GNPCInventoryOP.h"
#include "StorageOP/GCOSInventoryOP.h"
#include "StorageOP/GChestOP.h"
#include "StorageOP/GGuildChestOP.h"
#include "StorageOP/GAvatarInventoryOP.h"
#include "StorageOP/GMagicCubeStorageOP.h"

IMPLEMENT_DYNAMIC_EXISTING(CGObjChar, 0x00adeb7c)

IStorage_ *CGObjChar::GetStorageOP(int nType) {
    switch (nType) {
        case NORMAL_STORAGE_TYPE:
            return (&(*reinterpret_cast<CGStorageOP *>(0x00c66a98)));
        case PC_INVENTORY_TYPE:
            return (&(*reinterpret_cast<CGPCInventoryOP *>(0x00c66aa0)));
        case NPC_INVENTORY_TYPE:
            return (&(*reinterpret_cast<CGNPCInventoryOP *>(0x00c66aac)));
        case COS_INVENTORY_TYPE:
            return (&(*reinterpret_cast<CGCOSInventoryOP *>(0x00c66ab8)));
        case CHEST_TYPE:
            return (&(*reinterpret_cast<CGChestOP *>(0x00c66ad0)));
        case GUILD_CHEST_TYPE:
            return (&(*reinterpret_cast<CGGuildChestOP *>(0x00c66adc)));
        case AVATAR_INVENTORY_TYPE:
            return (&(*reinterpret_cast<CGAvatarInventoryOP *>(0x00c66ac4)));
        case MAGIC_CUBE_STORAGE_TYPE:
            return (&(*reinterpret_cast<CGMagicCubeStorageOP *>(0x00c66ae8)));
        default:
            return NULL;
    }
}
CGItem* CGObjChar::GetItemChar(int slot)
{
    CGItem *aa = CallVirtual<CGItem*(__thiscall*)(CGObjChar*, int)>(this, 350)(this, slot);
    return aa;
}
CGItem* CGObjChar::GetItemChar2(int slot)
{
    CGItem* aa = CallVirtual<CGItem* (__thiscall*)(CGObjChar*, int)>(this, 351)(this, slot);
    return aa;
}
void CGObjChar::Fun_214()
{
    //printf("%p %p\n", p1, p2);
    CallVirtual<void* (__thiscall*)(CGObjChar*)>(this, 211)(this);
}

void CGObjChar::Testx(int param_1, int p2) {
    printf ("%d %d \n", param_1, p2);
    reinterpret_cast<void(__thiscall*)(CGObjChar*, int, int)>(0x00524950)(this, param_1, p2);
}


int CGObjChar::Test(int p1, int p2)
{
    int aa = reinterpret_cast<int(__thiscall*)(CGObjChar*, int, int)>(0x004a8510)(this, p1, p2);
    //printf("%d %d %d\n", aa, p1, p2);
    return aa;
}
unsigned int CGObjChar::Fun_005291d0(unsigned int p1, unsigned int p2, unsigned int p3)
{

    unsigned aa = reinterpret_cast<unsigned int(__thiscall*)(CGObjChar*, unsigned int, unsigned int, unsigned int)>(0x005291d0)(this, p1, p2, p3);
    //printf("%d %d %d\n", aa, p1, p2, p3);
    return aa;
}

unsigned int CGObjChar::MobAttack(int param_1, int param_2, int param_3) {


    unsigned int aa = reinterpret_cast<unsigned int(__thiscall*)(CGObjChar*, int, int, int)>(0x0052a8e0)(this, param_1, param_2, param_3);
    //printf("%d %d %d %d \n", param_1, param_2, param_3, aa);
    return aa;

}

void CGObjChar::Fun_004a9b70(unsigned int p1, unsigned int p2)
{
    //printf("%p %p\n", p1, p2);
    reinterpret_cast<void(__thiscall*)(CGObjChar*, unsigned int, unsigned int)>(0x004a73f0)(this, p1, p2);
}
//unsigned int CGObjChar::Test() {
//    return 0;
//    //return;
//    reinterpret_cast<unsigned int(__thiscall*)(CGObjChar*)>(0x04e3f30)(this);
//}
