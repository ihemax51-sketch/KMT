//
// Created by Kurama on 12/15/2022.
//
#pragma once

#include "CriticalSectionBS.h"

namespace ServerFramework {
    class COverlapJobManager {
    public:
        virtual ~COverlapJobManager();

    private:
        CCriticalSectionBS m_csJob; //0x0004
        class CTask *m_pJobTask; //0x0040
        char pad_0044[12]; //0x0044 std::set
    private:
    BEGIN_FIXTURE()
            ENSURE_SIZE(0x0050)
            ENSURE_OFFSET(m_csJob, 0x0004)
            ENSURE_OFFSET(m_pJobTask, 0x0040)
        END_FIXTURE()

        RUN_FIXTURE(COverlapJobManager)
    };
}