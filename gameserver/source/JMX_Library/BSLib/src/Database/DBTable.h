//
// Created by Kurama on 12/12/2022.
//
#pragma once

#include "DBSessionMgr.h"
#include "DBSet.h"
#include "DBRecord.h"

class _record_allocator;

class CDBTable {
public:
    virtual ~CDBTable();

private:
    CDBSessionMgr *m_pMySession; //0x0004
    CDBSet<CDBRecord> *m_pTableData; //0x0008
    _record_allocator *m_pDBRecAlloc; //0x000C
    void *m_pfnAlloDBRec; //0x0010
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0014)
        ENSURE_OFFSET(m_pMySession, 0x0004)
    END_FIXTURE()

    RUN_FIXTURE(CDBTable)
};
