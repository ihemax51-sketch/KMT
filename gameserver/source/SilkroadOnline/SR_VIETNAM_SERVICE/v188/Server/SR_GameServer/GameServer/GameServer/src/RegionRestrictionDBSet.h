#pragma once
#include <Windows.h>
#include <iostream>
#include <vector>
#include <assert.h>
#include <string>
// Windows Header Files
#include <sstream>
#include <list>
#include <algorithm>
#include <assert.h>
#include <sstream>
#include <map>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <SqlConnection/DbConnection.h>
#include <SqlConnection/AutoCriticalSection.h>
#include "Memory/detours.h"

struct SHideNameRegionDbRecord
{
    int nID;
    WORD wRegionID;
};

#define __DBSET_HIDE_NAME_REGION_DB_RECORD_LST			std::list<SHideNameRegionDbRecord>
#define __DBSET_HIDE_NAME_REGION_DB_RECORD_LST_IT		__DBSET_HIDE_NAME_REGION_DB_RECORD_LST::iterator



class CRegionRestrictionDBSet
{
private:
    CDbConnection* m_pDbConnection;
    CAutoCriticalSection* m_pACS;
public:
    CRegionRestrictionDBSet(CDbConnection* pConn, CAutoCriticalSection* pACS);
    ~CRegionRestrictionDBSet();

    bool GetHideNameRegionRecords(__DBSET_HIDE_NAME_REGION_DB_RECORD_LST& result);

};