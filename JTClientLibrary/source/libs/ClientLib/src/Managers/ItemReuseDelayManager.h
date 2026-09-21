#pragma once

#include <map>
#include <ghidra/undefined.h>
#include <BSLib/_internal/custom_stl.h>
#include <support/AbiTest.h>

class CItemReuseDelayManager {
public:
    float FUN_009ba680(int p1);
    int FUN_009ba5c0(int p1);
    float OtherDelayMapFind_0x20_MAYBE(undefined1 param_1);
    float DelayMapFind_0x20_MAYBE(int param_1);
    float FUN_009b94d0(undefined4  param_1);
    float  DelayMapFind_0x1c_MAYBE(int param_1);
public:
    char pad_0000[4];
    std::n_list<int> N00000261; //0x0004 /// used item list
    char pad_0008[560]; //0x0008

    BEGIN_FIXTURE()

    ENSURE_SIZE(576)
    ENSURE_OFFSET(N00000261, 0x0004)
    END_FIXTURE()
    RUN_FIXTURE(CItemReuseDelayManager)

};
