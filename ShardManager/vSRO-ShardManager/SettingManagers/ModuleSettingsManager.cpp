#include "ModuleSettingsManager.h"
#include "../Utils/BSObj.h"

// Static member initialization
SQLConnection ModuleSettingsManager::m_Connection;
std::wstring ModuleSettingsManager::m_ConnectionString;

bool ModuleSettingsManager::Initialize(const std::wstring& connectionString) {
    m_ConnectionString = connectionString;
    if (!m_Connection.Open((SQLWCHAR*)m_ConnectionString.c_str())) {
        BS_ERROR("Unable to connect to the KMTGuard database");
        return false;
    }
    BS_INFO("KMTGuard database connection established");
    return true;
}

void ModuleSettingsManager::Close() {
    m_Connection.Close();
}

bool ModuleSettingsManager::ReadModuleSettings(std::vector<ModuleSetting>& settings) {
    if (!m_Connection.IsOpen())
        return false;

    SQLHSTMT hStmt = SQL_NULL_HSTMT;
    SQLRETURN retcode;

    // Allocate statement handle
    retcode = SQLAllocHandle(SQL_HANDLE_STMT, SQLConnection::GetConnectionHandle(m_Connection), &hStmt);
    if (!SQL_SUCCEEDED(retcode)) {
        SQLConnection::ShowError(SQL_HANDLE_DBC, SQLConnection::GetConnectionHandle(m_Connection), retcode);
        return false;
    }
    SQLSetStmtAttr(hStmt, SQL_ATTR_QUERY_TIMEOUT, (SQLPOINTER)30, 0);

    // Union membership and guild-point overflow are enforced by ShardManager,
    // while their customer-facing values live in the unified GameServer
    // settings catalog. Translate both public names to the existing internal
    // patch names and ignore retired duplicate ShardManager rows.
    const SQLWCHAR* query =
        L"SELECT ID, SettingName, Value "
        L"FROM [KMTGuard].[dbo].[System_ShardSettings] "
        L"WHERE SettingName NOT IN (N'UnionLimit', N'FixNegativeGuildPoint') "
        L"UNION ALL "
        L"SELECT -ID, "
        L"CASE SettingName "
        L"WHEN N'UNION_LIMIT' THEN N'UnionLimit' "
        L"WHEN N'GUILD_POINTS' THEN N'FixNegativeGuildPoint' END, "
        L"Value "
        L"FROM [KMTGuard].[dbo].[System_GameServerSettings] "
        L"WHERE SettingName IN (N'UNION_LIMIT', N'GUILD_POINTS')";
    retcode = SQLExecDirect(hStmt, const_cast<SQLWCHAR*>(query), SQL_NTS);
    if (!SQL_SUCCEEDED(retcode)) {
        SQLConnection::ShowError(SQL_HANDLE_STMT, hStmt, retcode);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return false;
    }

    // Bind columns
    SQLINTEGER id;
    SQLWCHAR name[100] = { 0 };
    SQLWCHAR value[100] = { 0 };
    SQLLEN idLen = 0, nameLen = 0, valueLen = 0;

    if (!SQL_SUCCEEDED(SQLBindCol(hStmt, 1, SQL_C_SLONG, &id, sizeof(id), &idLen)) ||
        !SQL_SUCCEEDED(SQLBindCol(hStmt, 2, SQL_C_WCHAR, name, sizeof(name), &nameLen)) ||
        !SQL_SUCCEEDED(SQLBindCol(hStmt, 3, SQL_C_WCHAR, value, sizeof(value), &valueLen))) {
        BS_ERROR("Unable to bind ShardManager settings columns");
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return false;
    }

    // Fetch and store results
    for (;;) {
        retcode = SQLFetch(hStmt);
        if (retcode == SQL_NO_DATA)
            break;
        if (retcode != SQL_SUCCESS || idLen == SQL_NULL_DATA || nameLen == SQL_NULL_DATA || valueLen == SQL_NULL_DATA) {
            BS_ERROR("ShardManager settings contain an invalid or truncated row");
            SQLConnection::ShowError(SQL_HANDLE_STMT, hStmt, retcode);
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return false;
        }
        ModuleSetting setting;
        setting.id = id;
        setting.name = name;
        setting.value = value;
        settings.push_back(setting);
    }

    // Cleanup
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    return true;
}
