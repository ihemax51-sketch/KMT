//
// Created by Kurama on 12/12/2022.
//
#pragma once

#include <Windows.h>
#include <sql.h>

#include "Test/Test.h"

class CDBSessionMgr {
public:
    virtual ~CDBSessionMgr();

private:
    char pad_0004[4]; //0x0004 conection index? connection time out? who knows.
    SQLHANDLE **m_pphConnection; //0x0008
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x000C)
        ENSURE_OFFSET(m_pphConnection, 0x0008)
    END_FIXTURE()

    RUN_FIXTURE(CDBSessionMgr)
};