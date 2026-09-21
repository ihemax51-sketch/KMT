#pragma once

#include <string>
#include <map>

#include "Base.h"
#include "DBColumMgr.h"

class CDBTable;

/// \note Unknown class name so lets call it like that for now..
template<typename dbRec>
class CDBSet {
    // Ctor 00863520
public:
    CRuntimeClass *m_pRecDBRunTime; //0x0000
    std::string m_strTableName; //0x0004
    std::string m_strMainQuires[6]; //0x0020 // update, select with options
    std::map<DWORD, CDBColumMgr> m_mapColumMgr; //0x00E4
    DWORD m_dwTableBufferSize; //0x00F0
    char pad_00F4[4]; //0x00F4
    DWORD m_dwColumCount; //0x00F8
    char pad_00FC[4]; //0x00FC
    CDBTable *m_pMyTable; //0x0100
    char pad_0104[4]; //0x0104 // RecTable class size??
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0108)
        ENSURE_OFFSET(m_pRecDataRunTime, 0x0000)
        ENSURE_OFFSET(m_strTableName, 0x0004)
        ENSURE_OFFSET(m_strMainQuires, 0x0020)
        ENSURE_OFFSET(m_mapColumMgr, 0x00E4)
        ENSURE_OFFSET(m_dwTableBufferSize, 0x00F0)
        ENSURE_OFFSET(m_dwColumCount, 0x00F8)
        ENSURE_OFFSET(m_pMyTable, 0x0100)
    END_FIXTURE()

    RUN_FIXTURE(CDBSet)
};
