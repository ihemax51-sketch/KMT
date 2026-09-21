#include "DbConnection.h"

CDbConnection::CDbConnection(std::string strConnStr)
{
    m_strConnStr = strConnStr;
    m_hEnv = SQL_NULL_HENV;
    m_hConn = SQL_NULL_HDBC;
}

CDbConnection::~CDbConnection()
{
    Disconnect();
}

bool CDbConnection::Connect()
{
    Disconnect();

    SQLCHAR retConnStr[1024] = { 0 };
    SQLRETURN retCode;

    retCode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_hEnv);
    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - Failed to allocate env handle" << std::endl;
        return false;
    }

    retCode = SQLSetEnvAttr(m_hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - Failed to set ODBC version env attr" << std::endl;
        SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
        m_hEnv = SQL_NULL_HENV;
        return false;
    }

    retCode = SQLAllocHandle(SQL_HANDLE_DBC, m_hEnv, &m_hConn);
    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - Failed to allocate conn handle" << std::endl;
        SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
        m_hEnv = SQL_NULL_HENV;
        return false;
    }

    retCode = SQLSetConnectAttr(
        m_hConn, SQL_LOGIN_TIMEOUT, reinterpret_cast<SQLPOINTER>(10), 0);
    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - Failed to set connection login timeout" << std::endl;
        SQLFreeHandle(SQL_HANDLE_DBC, m_hConn);
        m_hConn = SQL_NULL_HDBC;
        SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
        m_hEnv = SQL_NULL_HENV;
        return false;
    }

    retCode = SQLDriverConnectA(m_hConn, NULL, (SQLCHAR*)m_strConnStr.c_str(), SQL_NTS, retConnStr, sizeof(retConnStr), NULL, SQL_DRIVER_NOPROMPT);
    if (!SQL_SUCCEEDED(retCode))
    {
        SQLFreeHandle(SQL_HANDLE_DBC, m_hConn);
        m_hConn = SQL_NULL_HDBC;
        SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
        m_hEnv = SQL_NULL_HENV;
        return false;
    }

    return true;
}

void CDbConnection::Disconnect()
{
    if (m_hConn != SQL_NULL_HDBC)
    {
        const SQLHANDLE connection = m_hConn;
        m_hConn = SQL_NULL_HDBC;
        SQLDisconnect(connection);
        SQLFreeHandle(SQL_HANDLE_DBC, connection);
    }

    if (m_hEnv != SQL_NULL_HENV)
    {
        const SQLHANDLE environment = m_hEnv;
        m_hEnv = SQL_NULL_HENV;
        SQLFreeHandle(SQL_HANDLE_ENV, environment);
    }
}

SQLHANDLE CDbConnection::GetEnvHandle() const
{
    return m_hEnv;
}

SQLHANDLE CDbConnection::GetConnHandle() const
{
    return m_hConn;
}

std::string CDbConnection::GetConnString() const
{
    return m_strConnStr;
}

bool CDbConnection::AllocStmt(SQLHANDLE& hStmt)
{
    if (GetEnvHandle() == SQL_NULL_HENV || GetConnHandle() == SQL_NULL_HDBC)
    {
        std::cout << __FUNCTION__ << " - Cannot allocate stmt (null env/conn handle)" << std::endl;
        return false;
    }

    SQLRETURN retCode = SQLAllocStmt(GetConnHandle(), &hStmt);
    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - Failed to allocate stmt" << std::endl;
        return false;
    }

    retCode = SQLSetStmtAttr(
        hStmt, SQL_ATTR_QUERY_TIMEOUT, reinterpret_cast<SQLPOINTER>(30), 0);
    if (!SQL_SUCCEEDED(retCode))
    {
        std::cout << __FUNCTION__ << " - Failed to set statement query timeout" << std::endl;
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        hStmt = SQL_NULL_HSTMT;
        return false;
    }

    return true;
}

bool CDbConnection::FreeStmt(SQLHANDLE& hStmt)
{
    if (hStmt == SQL_NULL_HSTMT)
        return true;

    const SQLHANDLE statement = hStmt;
    hStmt = SQL_NULL_HSTMT;

    const SQLRETURN closeResult = SQLFreeStmt(statement, SQL_CLOSE);
    if (!SQL_SUCCEEDED(closeResult))
        std::cout << __FUNCTION__ << " - Failed to free stmt" << std::endl;

    const SQLRETURN freeResult = SQLFreeHandle(SQL_HANDLE_STMT, statement);
    if (!SQL_SUCCEEDED(freeResult))
    {
        std::cout << __FUNCTION__ << " - Failed to free stmt handle" << std::endl;
        return false;
    }

    return SQL_SUCCEEDED(closeResult);
}
