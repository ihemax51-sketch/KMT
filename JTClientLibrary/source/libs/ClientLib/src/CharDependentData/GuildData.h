#pragma once

#include "support/AbiTest.h"

#include "BSLib/BSLib.h"

struct SGuildData {
    // Constructor 0099bd90
private:
    char pad_0004[4]; //0x0004
public:
    bool bInGuild; //0x0004
private:
    char pad_0005[143]; //0x0005
public:
    std::n_set<DWORD> setAlliance; //0x0094 todo: check if this map or set plz
private:
BEGIN_FIXTURE()
        ENSURE_OFFSET(bInGuild, 0x0004)
        ENSURE_OFFSET(setAlliance, 0x0094)
    END_FIXTURE()

    RUN_FIXTURE(SGuildData)
};