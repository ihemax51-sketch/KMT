#pragma once

#define g_CICScriptObjManager (*(CICScriptObjManager **) 0x00ef1cf8)
class CICScriptObjManager {
public:
    void FUN_009e5240();
    void Fun_005d2a80(int p1, undefined4 p2);
    char pad_0000[24]; //0x0000
    std::list<std::n_wstring> N0001A3E2; //0x0018

};

class CICScriptObj
{
public:
    bool OnCreateIMPL(long ln);
void FUN_009e5130();
};

class CGScript
{
public:
    bool OnCreateIMPL(long ln);
void Fun_0051fa20(undefined4 * p1, undefined4 p2);
};