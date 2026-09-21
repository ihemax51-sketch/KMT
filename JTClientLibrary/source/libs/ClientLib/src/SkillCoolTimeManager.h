#pragma once
#include <map>
#include <support/AbiTest.h>
#include <ghidra/undefined.h>
#include <BSLib/_internal/custom_stl.h>

class CSkillCoolTimeManager {
public:
    int FUN_009bba90(int p1);
    char pad_0000[4];
    std::n_list<int> N00000261; //0x0004 /// used item list
    char pad_0008[560]; //0x0008

    BEGIN_FIXTURE()

    ENSURE_SIZE(576)
    ENSURE_OFFSET(N00000261, 0x0004)
    END_FIXTURE()
    RUN_FIXTURE(CSkillCoolTimeManager)


};

