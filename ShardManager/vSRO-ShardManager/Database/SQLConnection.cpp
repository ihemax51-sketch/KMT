#include "SQLConnection.h"
#include "../Utils/BSObj.h"

SQLConnection::SQLConnection()
	: m_EnvHandle(SQL_NULL_HENV),
	  m_ConnHandle(SQL_NULL_HDBC),
	  m_IsOpen(false)
{
}
SQLConnection::~SQLConnection()
{
	Close();
}

bool SQLConnection::IsOpen() const
{
	return m_IsOpen;
}

bool SQLConnection::Open(SQLWCHAR* ConnectionString)
{
	// Try to close last connection
	Close();

	SQLRETURN result;
	// Allocating handlers
	result = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_EnvHandle);
	if (!SQL_SUCCEEDED(result))
	{
		ShowError(SQL_HANDLE_ENV, m_EnvHandle, result);
		Close();
		return false;
	}
	result = SQLSetEnvAttr(m_EnvHandle, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
	if (!SQL_SUCCEEDED(result))
	{
		ShowError(SQL_HANDLE_ENV, m_EnvHandle, result);
		Close();
		return false;
	}
	result = SQLAllocHandle(SQL_HANDLE_DBC, m_EnvHandle, &m_ConnHandle);
	if (!SQL_SUCCEEDED(result))
	{
		ShowError(SQL_HANDLE_DBC, m_ConnHandle, result);
		Close();
		return false;
	}
	SQLSetConnectAttr(m_ConnHandle, SQL_LOGIN_TIMEOUT, (SQLPOINTER)10, 0);

	// Connect to database
	SQLWCHAR retConString[1000];
	result = SQLDriverConnect(
		m_ConnHandle,
		NULL,
		ConnectionString,
		SQL_NTS,
		retConString,
		static_cast<SQLSMALLINT>(_countof(retConString)),
		NULL,
		SQL_DRIVER_NOPROMPT);

	// Check connection results
	switch (result)
	{
	case SQL_SUCCESS:
	case SQL_SUCCESS_WITH_INFO:
		m_IsOpen = true;
		return true;
	}
	ShowError(SQL_HANDLE_DBC, m_ConnHandle, result);
	Close();
	return false;
}

void SQLConnection::Close()
{
	if (m_ConnHandle != SQL_NULL_HDBC)
	{
		if (m_IsOpen)
			SQLDisconnect(m_ConnHandle);
		SQLFreeHandle(SQL_HANDLE_DBC, m_ConnHandle);
	}
	if (m_EnvHandle != SQL_NULL_HENV)
		SQLFreeHandle(SQL_HANDLE_ENV, m_EnvHandle);

	m_IsOpen = false;
	m_ConnHandle = SQL_NULL_HDBC;
	m_EnvHandle = SQL_NULL_HENV;
}

void SQLConnection::ShowError(unsigned int hType, const SQLHANDLE& hHandle, SQLRETURN RetCode)
{
	if (RetCode == SQL_INVALID_HANDLE)
	{
		BS_ERROR("ODBC reported an invalid handle");
		return;
	}
	if (hHandle == SQL_NULL_HANDLE)
		return;

	SQLSMALLINT iRec = 0;
	SQLINTEGER  iError;
	WCHAR       wszMessage[1000];
	WCHAR       wszState[SQL_SQLSTATE_SIZE + 1];

	while (SQLGetDiagRec(hType, hHandle, ++iRec, wszState, &iError, wszMessage, (SQLSMALLINT)(sizeof(wszMessage) / sizeof(WCHAR)), (SQLSMALLINT*)NULL) == SQL_SUCCESS)
	{
		// Hide data truncated..
		if (wcsncmp(wszState, L"01004", 5))
		{
			char state[8] = { 0 };
			char message[1000] = { 0 };
			WideCharToMultiByte(CP_UTF8, 0, wszState, -1, state, sizeof(state), NULL, NULL);
			WideCharToMultiByte(CP_UTF8, 0, wszMessage, -1, message, sizeof(message), NULL, NULL);
			BS_ERROR("ODBC [%s] %s (native=%ld)", state, message, static_cast<long>(iError));
		}
	}
}

SQLHANDLE SQLConnection::GetConnectionHandle(const SQLConnection& Connection)
{
	return Connection.m_ConnHandle;
}
