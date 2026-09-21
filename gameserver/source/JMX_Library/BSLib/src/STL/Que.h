//
// Created by Kurama on 12/14/2022.
//
#pragma once

#include <queue>

#include "Test/Test.h"

template<typename T>
class CQue {
public:
    CQue() {
        m_nQueueIndex = -1;
    }

    virtual ~CQue() {
        m_nQueueIndex = 0;
    }

public:
    int m_nQueueIndex; //0x0004 ithink so
    std::queue<T> m_queueElemnt;  //0x0008
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x001c)
        ENSURE_OFFSET(m_queueElemnt, 0x0008)
    END_FIXTURE()

    RUN_FIXTURE(CQue)
};