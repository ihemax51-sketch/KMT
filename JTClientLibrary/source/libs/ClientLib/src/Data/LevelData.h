#pragma once

#include "support/AbiTest.h"

struct SLevelData {
    unsigned __int8 m_level;// 0x0000
    char gap2[4];
    unsigned __int64 m_expC;// 0x0008
    unsigned __int32 m_expM;// 0x000C
    char gap3[8];
    int m_jobExpTrader;
    int m_jobExpRobber;
    int m_jobExpHunter;

private:
    BEGIN_FIXTURE()
        ENSURE_OFFSET(m_expC, 0x0008)
        ENSURE_OFFSET(m_jobExpTrader, 0x001C)
        ENSURE_OFFSET(m_jobExpRobber, 0x0020)
        ENSURE_OFFSET(m_jobExpHunter, 0x0024)
    END_FIXTURE()

    RUN_FIXTURE(SLevelData)
};

class CLevelData {
public:
    virtual ~CLevelData();

    /// \address 0097ab50
    const SLevelData &GetData() const;

private:
    SLevelData data;

private:
    BEGIN_FIXTURE()
        ENSURE_OFFSET(data, 0x0008)
    END_FIXTURE()

    RUN_FIXTURE(CLevelData)
};
