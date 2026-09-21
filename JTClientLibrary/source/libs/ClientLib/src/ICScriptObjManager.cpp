//
// Created by YUMBUL on 22.11.2024.
//

#include <GFX3DFunction/RStateMgr.h>
#include "ICScriptObjManager.h"
bool aaa = true;
void CICScriptObjManager::Fun_005d2a80(int p1, undefined4 p2)
{
    //printf("aaaaaaaaaaaaaaa %d %d %p\n", p1, p2, this);
    reinterpret_cast<void(__thiscall *) (CICScriptObjManager *, undefined4, undefined4)>(0x0051fa20)(this, p1, p2);

}
void CICScriptObjManager::FUN_009e5240(){


    N0001A3E2.push_back(L"ANI_READY06");


    //printf("%p \n", this);
    reinterpret_cast<void (__thiscall *)(CICScriptObjManager*)>(0x009e5240)(this);

}
bool CICScriptObj::OnCreateIMPL(long ln){
    //printf("hisss %p\n", this);
    return reinterpret_cast<bool(__thiscall*) (CICScriptObj*,long)>(0x009e2340)(this, ln);
}
void CICScriptObj::FUN_009e5130()
{
    //printf("hisss %p\n", this);
    reinterpret_cast<void (__thiscall *)(CICScriptObj*)>(0x009e5130)(this);

}



// std::wstring'deki karakterleri short türündeki bir diziye dönüştüren fonksiyon
short* ConvertStringToShort(const std::wstring& str) {
    // Her karakter için bir short ayıracak kadar yer ayırın
    short* shortArray = new short[str.size() + 1]; // Null sonlandırıcı için ek bir alan

    for (size_t i = 0; i < str.size(); ++i) {
        shortArray[i] = static_cast<short>(str[i]);
    }

    // Null sonlandırıcıyı ekleyin
    shortArray[str.size()] = 0;

    return shortArray;
}
bool loaded = false;
void CGScript::Fun_0051fa20(undefined4 *p1, undefined4 p2)
{
    if(loaded == false)
    {
        loaded = true;

        std::wstring testar = L"ANI_WAIT05";
        short* shortDatar = ConvertStringToShort(testar);


        int testark = g_RStateMgr.FUN_0047e880(shortDatar);
        this->Fun_0051fa20(reinterpret_cast<undefined4 *>(shortDatar), testark);

    }

   //printf("CGScript %p %p %p\n", p1, p2, this);
    reinterpret_cast<void(__thiscall *) (CGScript *, undefined4*, undefined4)>(0x0051fa20)(this, p1, p2);

}
bool CGScript::OnCreateIMPL(long ln){
    //printf("hisss %p\n", this);
    return reinterpret_cast<bool(__thiscall*) (CGScript*,long)>(0x00503cd0)(this, ln);
}