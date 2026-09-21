#pragma once

#include <map>
#include <list>
#include <BSLib/_internal/custom_stl.h>
#include <support/AbiTest.h>

class CCOSDataMgr
{
public:
    struct CosData
    {
        int RefObjID; //0x0000
        int UniqueID; //0x0004
        int HPValue; //0x0008
        char pad_000C[4]; //0x000C
        unsigned short HGPValue; //0x0010
        unsigned short N000010A8; //0x0012
        char pad_0014[84]; //0x0014
    };

    char pad[4];
    std::map<int, CosData*> CosList;
    int SelectedCosID;
    BEGIN_FIXTURE()
    ENSURE_SIZE(20)
    END_FIXTURE()

    RUN_FIXTURE(CCOSDataMgr)
};