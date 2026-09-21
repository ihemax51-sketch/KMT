#include "RegionRestrictionDBSet.h"

CRegionRestrictionDBSet::CRegionRestrictionDBSet(CDbConnection* pConn, CAutoCriticalSection* pACS)
{
    m_pDbConnection = pConn;
    m_pACS = pACS;
}

CRegionRestrictionDBSet::~CRegionRestrictionDBSet()
{

}

bool CRegionRestrictionDBSet::GetHideNameRegionRecords(__DBSET_HIDE_NAME_REGION_DB_RECORD_LST& result)
{
    SQLHANDLE hStmt = SQL_NULL_HSTMT;

    if (!m_pDbConnection->AllocStmt(hStmt))
    {
        std::cout << __FUNCTION__ << " - Failed to allocate stmt" << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        return false;
    }

    SQLRETURN retCode;
    const LPCSTR szQuery = "SELECT ID, RegionID FROM [dbo].[Security_HideNameRegions]";

    m_pACS->Enter();

    retCode = SQLExecDirectA(hStmt, (SQLCHAR*)szQuery, SQL_NTS);
    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLExecDirect failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return false;
    }

    retCode = SQLFetch(hStmt);

    if (retCode == SQL_NO_DATA)
    {
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return true;
    }

    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - SQLFetch failed, query = " << szQuery << std::endl;
        m_pDbConnection->FreeStmt(hStmt);
        m_pACS->Leave();
        return false;
    }

    SQLINTEGER cb;

    do
    {
        SHideNameRegionDbRecord record;
        ZeroMemory(&record, sizeof(SHideNameRegionDbRecord));

        SQLGetData(hStmt, 1, SQL_C_LONG, (SQLPOINTER)&record.nID, 0, &cb);
        SQLGetData(hStmt, 2, SQL_C_USHORT, (SQLPOINTER)&record.wRegionID, 0, &cb);

        result.push_back(record);
        retCode = SQLFetch(hStmt);
    } while (SQL_SUCCEEDED(retCode));

    m_pDbConnection->FreeStmt(hStmt);

    m_pACS->Leave();

    return true;
}

