//
// Created by Kurama on 12/12/2022.
//
#pragma once

#include "DBSessionMgr.h"
#include "DBTable.h"

#include <vector>

#include "Test/Test.h"

class CTableKeeper {
public:
    virtual ~CTableKeeper();

private:
    CDBSessionMgr *m_pMyDBSession; //0x0004
    std::vector<CDBTable *> m_vecDBLoadedTables; //0x0008
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0018)
        ENSURE_OFFSET(m_pMyDBSession, 0x0004)
        ENSURE_OFFSET(m_vecDBLoadedTables, 0x0008)
    END_FIXTURE()

    RUN_FIXTURE(CTableKeeper)
};