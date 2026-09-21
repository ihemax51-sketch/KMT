#pragma once

#include "support/AbiTest.h"
#include <BSLib/BSLib.h>

class CIEntity : public CObjChild {

public:
    virtual void Func_7();

    virtual void Func_8();

    virtual void Func_9();

    virtual void Func_10();

    virtual void Func_11();

    virtual void Func_12();

    virtual void Func_13();


private: /* members */
    char pad_0020[0x10]; //0x0020

protected:
    int m_someId; //0x0030

private:
    char pad_0034[0x8]; //0x0034

protected:
    int m_ageInMilliseconds;

private:
    char pad_0040[0x14]; //0x0038

private:
BEGIN_FIXTURE()
        ENSURE_OFFSET(m_someId, 0x30)
        ENSURE_OFFSET(m_ageInMilliseconds, 0x003c)
        ENSURE_SIZE(0x54)
    END_FIXTURE()

    RUN_FIXTURE(CIEntity)
};
