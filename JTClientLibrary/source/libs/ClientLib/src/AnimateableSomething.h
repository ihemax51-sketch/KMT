#pragma once


#include "ghidra/undefined.h"
#include <BSLib/Debug.h>
#include <support/AbiTest.h>

class CAnimateableSomething {
public:
    /// \address 009c7bc0
    void FUN_009c7bc0(float correction);

public:
    float field_0;

private:
    char pad[0x1c];

private:
    BEGIN_FIXTURE()
    ENSURE_OFFSET(field_0, 0x0000)
    ENSURE_SIZE(0x20)
    END_FIXTURE()
    RUN_FIXTURE(CAnimateableSomething)
};
