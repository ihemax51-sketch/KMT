//
// Created by YUMBUL on 19.03.2023.
//
#pragma once


#include <map>
#include "GObjChar.h"
class CGObjNPC : public CGObjChar
{
public:
public:
    std::map<DWORD, SAggroMapSecondPairItem> MyMap; //0x1CA0
    char pad_1CB0[32]; //0x1CB0
    int Moving; //0x1CCC
    char pad_1CD0[8]; //0x1CD0
BEGIN_FIXTURE()
        ENSURE_SIZE(0x1CD8)
        ENSURE_OFFSET(MyMap, 0x1CA0)
    END_FIXTURE()

    RUN_FIXTURE(CGObjNPC)
}; //Size: 0x1CD8

