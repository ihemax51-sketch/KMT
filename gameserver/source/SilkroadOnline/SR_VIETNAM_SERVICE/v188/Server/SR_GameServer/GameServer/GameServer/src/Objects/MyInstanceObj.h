#pragma once


#include <ReferenceData/ReferenceData.h>

class MyCInstanceObj
{
public:
    char pad_0000[16]; //0x0000
    class CDBTable* DBTable;; //0x0010
    char pad_0014[4]; //0x0014
    RefObjItem* pCRefObjItem; //0x0018
    char pad_001C[4]; //0x001C
private:
    BEGIN_FIXTURE()
    ENSURE_SIZE(32)
    ENSURE_OFFSET(DBTable, 0x0010)
    ENSURE_OFFSET(pCRefObjItem, 0x0018)
    END_FIXTURE()

    RUN_FIXTURE(MyCInstanceObj)
};
