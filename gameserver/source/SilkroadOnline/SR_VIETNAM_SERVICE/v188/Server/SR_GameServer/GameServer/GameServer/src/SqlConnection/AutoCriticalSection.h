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

class CAutoCriticalSection
{
private:
    CRITICAL_SECTION m_cs;
public:
    CAutoCriticalSection();
    virtual ~CAutoCriticalSection();

    void Enter();
    void Leave();
};