#include "SQLCommand.h"
#include "../Utils/BSObj.h"
#include <cwchar>

SQLCommand::SQLCommand()
	: m_StmtHandle(SQL_NULL_HSTMT),
	  m_IsOpen(false)
{
	InitializeCriticalSection(&m_HandleLock);
}
SQLCommand::~SQLCommand()
{
	Close();
	DeleteCriticalSection(&m_HandleLock);
}

bool SQLCommand::IsOpen() const
{
	return m_IsOpen;
}

bool SQLCommand::Open(const SQLConnection& Connection)
{
	Close();
	if (!Connection.IsOpen())
		return false;

	SQLRETURN result;
	result = SQLAllocHandle(SQL_HANDLE_STMT, SQLConnection::GetConnectionHandle(Connection), &m_StmtHandle);
	if (!SQL_SUCCEEDED(result))
	{
		SQLConnection::ShowError(SQL_HANDLE_STMT, m_StmtHandle, result);
		m_StmtHandle = SQL_NULL_HSTMT;
		return false;
	}
	SQLSetStmtAttr(m_StmtHandle, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)30, 0);
	m_IsOpen = true;
	return true;
}
bool SQLCommand::ExecuteQuery(SQLWCHAR* CommandText)
{
	if (!m_IsOpen || m_StmtHandle == SQL_NULL_HSTMT || CommandText == NULL)
		return false;

	Clear();
	SQLRETURN result = SQLExecDirect(m_StmtHandle, CommandText, SQL_NTS);
	if (!SQL_SUCCEEDED(result))
	{
		BS_ERROR("ODBC query execution failed");
		SQLConnection::ShowError(SQL_HANDLE_STMT, m_StmtHandle, result);
		return false;
	}
	return true;
}
void SQLCommand::Clear()
{
	if (m_StmtHandle != SQL_NULL_HSTMT)
		SQLCloseCursor(m_StmtHandle);
}
void SQLCommand::Close()
{
	EnterCriticalSection(&m_HandleLock);
	if (m_StmtHandle != SQL_NULL_HSTMT)
	{
		SQLCloseCursor(m_StmtHandle);
		SQLFreeHandle(SQL_HANDLE_STMT, m_StmtHandle);
	}
	m_StmtHandle = SQL_NULL_HSTMT;
	m_IsOpen = false;
	LeaveCriticalSection(&m_HandleLock);
}
bool SQLCommand::FetchData()
{
    return FetchDataResult() == SQL_FETCH_ROW;
}
SQL_FETCH_RESULT SQLCommand::FetchDataResult()
{
    if (!m_IsOpen || m_StmtHandle == SQL_NULL_HSTMT)
        return SQL_FETCH_ERROR;
    const SQLRETURN result = SQLFetch(m_StmtHandle);
    if (result == SQL_NO_DATA)
        return SQL_FETCH_NO_DATA;
    if (!SQL_SUCCEEDED(result))
    {
        SQLConnection::ShowError(SQL_HANDLE_STMT, m_StmtHandle, result);
        return SQL_FETCH_ERROR;
    }
    return SQL_FETCH_ROW;
}
bool SQLCommand::GetData(SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType, SQLPOINTER TargetValue, SQLINTEGER BufferLength, SQLINTEGER* StrLen_or_IndPtr)
{
	const SQL_DATA_RESULT result = GetDataResult(
		ColumnNumber, TargetType, TargetValue, BufferLength, StrLen_or_IndPtr);
	return result == SQL_DATA_SUCCESS ||
		result == SQL_DATA_SUCCESS_WITH_INFO ||
		result == SQL_DATA_NULL;
}

SQL_DATA_RESULT SQLCommand::GetDataResult(
	SQLUSMALLINT ColumnNumber,
	SQLSMALLINT TargetType,
	SQLPOINTER TargetValue,
	SQLINTEGER BufferLength,
	SQLINTEGER* StrLen_or_IndPtr)
{
	if (!m_IsOpen || m_StmtHandle == SQL_NULL_HSTMT)
		return SQL_DATA_ERROR;

	SQLINTEGER localLength = 0;
	SQLINTEGER* length = StrLen_or_IndPtr != NULL ? StrLen_or_IndPtr : &localLength;
	const SQLRETURN result = SQLGetData(
		m_StmtHandle, ColumnNumber, TargetType, TargetValue, BufferLength, length);

	if (result == SQL_NO_DATA)
		return SQL_DATA_NO_DATA;
	if (!SQL_SUCCEEDED(result))
	{
		BS_ERROR("ODBC column read failed (column=%u)", static_cast<unsigned>(ColumnNumber));
		SQLConnection::ShowError(SQL_HANDLE_STMT, m_StmtHandle, result);
		return SQL_DATA_ERROR;
	}
	if (*length == SQL_NULL_DATA)
		return SQL_DATA_NULL;

	const bool characterTarget = TargetType == SQL_C_CHAR || TargetType == SQL_C_WCHAR;
	const bool truncated = result == SQL_SUCCESS_WITH_INFO && characterTarget &&
		(*length == SQL_NO_TOTAL || BufferLength <= 0 || *length >= BufferLength);
	if (truncated)
	{
		BS_ERROR("ODBC column data was truncated (column=%u)", static_cast<unsigned>(ColumnNumber));
		return SQL_DATA_TRUNCATED;
	}

	return result == SQL_SUCCESS_WITH_INFO
		? SQL_DATA_SUCCESS_WITH_INFO
		: SQL_DATA_SUCCESS;
}

bool SQLCommand::Cancel()
{
	EnterCriticalSection(&m_HandleLock);
	const SQLHANDLE statement = m_StmtHandle;
	if (statement == SQL_NULL_HSTMT)
	{
		LeaveCriticalSection(&m_HandleLock);
		return true;
	}
	const bool cancelled = SQL_SUCCEEDED(
		SQLCancelHandle(SQL_HANDLE_STMT, statement));
	LeaveCriticalSection(&m_HandleLock);
	return cancelled;
}

SQLHANDLE SQLCommand::GetStmtHandle()
{
	return m_StmtHandle;
}

void SQLCommand::ReleaseStmtHandle(SQLHANDLE hStmt)
{
	if (hStmt == m_StmtHandle)
	{
		Close();
		return;
	}
	if (hStmt != SQL_NULL_HSTMT)
		SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}
