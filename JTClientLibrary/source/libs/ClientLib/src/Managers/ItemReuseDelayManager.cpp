//
// Created by YUMBUL on 7.04.2024.
//

#include "ItemReuseDelayManager.h"
float CItemReuseDelayManager::FUN_009ba680(int p1){
    return reinterpret_cast<float(__thiscall*)(CItemReuseDelayManager*, int)>(0x009ba680)(this, p1);
}
int CItemReuseDelayManager::FUN_009ba5c0(int p1)
{
    return reinterpret_cast<int(__thiscall*)(CItemReuseDelayManager*, int)>(0x009ba5c0)(this, p1);
}
float CItemReuseDelayManager::DelayMapFind_0x1c_MAYBE(int param_1) /// tried
{
    float aa = reinterpret_cast<float(__thiscall*)(CItemReuseDelayManager*, int)>(0x009ba6e0)(this, param_1);
    return aa;
}
float CItemReuseDelayManager::OtherDelayMapFind_0x20_MAYBE(undefined1 param_1)
{
    return reinterpret_cast<float(__thiscall*)(CItemReuseDelayManager*, undefined1)>(0x009ba560)(this, param_1);
}
float CItemReuseDelayManager::DelayMapFind_0x20_MAYBE(int param_1) // tried
{
    return reinterpret_cast<float(__thiscall*)(CItemReuseDelayManager*, int)>(0x009ba680)(this, param_1);
}
float CItemReuseDelayManager::FUN_009b94d0(undefined4 param_1){
    float aa = reinterpret_cast<float(__thiscall*)(CItemReuseDelayManager*, undefined4)>(0x009b94d0)(this, param_1);
    //printf("%d %.2f\n", param_1, aa);
    return aa;
}

