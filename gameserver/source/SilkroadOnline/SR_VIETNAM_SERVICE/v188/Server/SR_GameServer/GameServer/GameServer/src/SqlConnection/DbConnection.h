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
#include "Memory/detours.h"

class CDbConnection
{
private:
    std::string m_strConnStr;
    SQLHANDLE m_hEnv;
    SQLHANDLE m_hConn;
public:
    CDbConnection(std::string strConnStr);
    virtual ~CDbConnection();

    bool Connect();
    void Disconnect();

    SQLHANDLE GetEnvHandle() const;
    SQLHANDLE GetConnHandle() const;
    std::string GetConnString() const;

    bool AllocStmt(SQLHANDLE& hStmt);
    bool FreeStmt(SQLHANDLE& hStmt);
};