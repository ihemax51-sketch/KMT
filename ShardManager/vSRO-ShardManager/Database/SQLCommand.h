#pragma once
#include "SQLConnection.h"

enum SQL_FETCH_RESULT
{
    SQL_FETCH_ROW,
    SQL_FETCH_NO_DATA,
    SQL_FETCH_ERROR
};

enum SQL_DATA_RESULT
{
    SQL_DATA_SUCCESS,
    SQL_DATA_SUCCESS_WITH_INFO,
    SQL_DATA_NULL,
    SQL_DATA_TRUNCATED,
    SQL_DATA_NO_DATA,
    SQL_DATA_ERROR
};

class SQLCommand
{
private: /// Private Members
    // Statement handler
    SQLHANDLE m_StmtHandle;
    // Keeps the connection state
    bool m_IsOpen;
	CRITICAL_SECTION m_HandleLock;
	SQLCommand(const SQLCommand&);
	SQLCommand& operator=(const SQLCommand&);
public: /// Public Properties
    // Check if the command has connection
    bool IsOpen() const;
public: /// Constructor
    SQLCommand();
    ~SQLCommand();
public: /// Public Methods
    // Open a connection command. Return success
    bool Open(const SQLConnection& Connection);
    // Executes a query command
    bool ExecuteQuery(SQLWCHAR* CommandText);
    // Clears all info associated
    void Clear();
	void Close();
    // Fetch data from last query
    bool FetchData();
    SQL_FETCH_RESULT FetchDataResult();
    // Read data from fetch
    bool GetData(SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType, SQLPOINTER TargetValue, SQLINTEGER BufferLength, SQLINTEGER* StrLen_or_IndPtr);
    SQL_DATA_RESULT GetDataResult(SQLUSMALLINT ColumnNumber, SQLSMALLINT TargetType, SQLPOINTER TargetValue, SQLINTEGER BufferLength, SQLINTEGER* StrLen_or_IndPtr);
    bool Cancel();

    // Get statement handle
    SQLHANDLE GetStmtHandle();
    // Release statement handle
    void ReleaseStmtHandle(SQLHANDLE hStmt);
};
