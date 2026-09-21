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

#define __REGION_RESTRICTION_MGR_HIDE_NAME_REGION_LST		std::vector<WORD>
#define __REGION_RESTRICTION_MGR_HIDE_NAME_REGION_LST_IT	__REGION_RESTRICTION_MGR_HIDE_NAME_REGION_LST::iterator

class CRegionRestrictionMgr
{
private:
    static __REGION_RESTRICTION_MGR_HIDE_NAME_REGION_LST s_lstHideNameRegions;
    static bool LoadHideNameRegions();

public:
    static bool Initialize();
    static void AddHideNameRegion(WORD wRegionID);
    static bool IsHideNameRegion(WORD wRegionID);
};
