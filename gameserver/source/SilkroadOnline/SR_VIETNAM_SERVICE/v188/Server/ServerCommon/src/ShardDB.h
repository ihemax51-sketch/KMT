//
// Created by Kurama on 12/12/2022.
//
#pragma once

#include "STL/SingletonT.h"

#include "Database/DBSessionMgr.h"
#include "Database/TableKeeper.h"

enum DATABASE_LOAD_TYPE : int {
    DATABASE_LOAD_TYPE_NONE,
    DATABASE_LOAD_TYPE_SHARD_LOG,
    DATABASE_LOAD_TYPE_SHAR_AND_LOG,
};

class CShardDB : public CSingletonT<CShardDB> {
public:
    virtual ~CShardDB();

public:
    DATABASE_LOAD_TYPE m_nLoadedDBType; //0x0004
    CDBSessionMgr *m_pShardDBSession; //0x0008
    CDBSessionMgr *m_pShardLOGDBSession; //0x000C
    CTableKeeper m_pShardDBTables; //0x0010
    CTableKeeper m_pShardLOGDBTables; //0x0028
    // ..
private:
BEGIN_FIXTURE()
        //ENSURE_SIZE() unknown but its maybe 0x64
        ENSURE_OFFSET(m_nLoadedDBType, 0x0004)
        ENSURE_OFFSET(m_pShardDBSession, 0x0008)
        ENSURE_OFFSET(m_pShardLOGDBSession, 0x000C)
        ENSURE_OFFSET(m_pShardDBTables, 0x0010)
        ENSURE_OFFSET(m_pShardLOGDBTables, 0x0028)
    END_FIXTURE()

    RUN_FIXTURE(CShardDB)
};