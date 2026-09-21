//
// Created by Kurama on 12/14/2022.
//
#pragma once

#include "Base.h"
#include "CriticalSectionBS.h"

class CServiceObject : public CBase {
DECLARE_DYNAMIC_EXISTING(CServiceObject, 0x00add8b0)

public:
    virtual void Func_2(class CTask *, void *);

    virtual void Func_3() = 0;

    virtual void Func_4() = 0;

    virtual class CTask *GetProcessTask();

private:
    char pad_0004[4]; //0x0004
    class CTask *m_pTask; //0x0008
    char pad_000c[4]; //0x000C
    CCriticalSectionBS m_csProcess; //0x0010
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x004C)
        ENSURE_OFFSET(m_pTask, 0x0008)
        ENSURE_OFFSET(m_csProcess, 0x0010)
    END_FIXTURE()

    RUN_FIXTURE(CServiceObject)
};