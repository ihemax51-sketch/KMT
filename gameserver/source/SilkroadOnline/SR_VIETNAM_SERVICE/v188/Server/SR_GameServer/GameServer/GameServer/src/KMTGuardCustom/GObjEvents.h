#pragma once
#include "GObj.h"

class CGObjEvents {
public:
    static void CheckRegionNeedChange(CGObj *obj, short region);
    static bool Initialize();
    static void Shutdown();
    static void Naked_OnLatestRegionChange();
};

