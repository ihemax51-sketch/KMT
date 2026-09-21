///******************************************************************************
/// \File GItem.h
///
/// \Desc
///
///******************************************************************************

#pragma once

#include "GObj.h"
#include "InstanceItem.h"

class CGItem : public IGObj{
public:
    void RefreshItemStats();
    static void FlushStreamBufferMsg(CMsg* pMsg);
    static void BindStreamBufferWithMsg(CMsg* pMsg);
    void WriteItemDataToMsg(void* pSteamBufferRelated, int eType);
    void WriteItemCosDataMsg(void* pStreamBuffer, int eType);
    void FUN_0048fb60();
    void FUN_00497340();
public:
    char pad_0000[44]; //0x0000
    void* Chardata; //0x0030
    CInstanceItem* InstanceItem; //0x0034
    char pad_0038[296]; //0x0038
    INT64 ID64; //0x0160
    char pad_0164[40]; //0x0164
    //char pad_0164[44]; //0x0164
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0190)
        ENSURE_OFFSET(Chardata, 0x0030)
        ENSURE_OFFSET(InstanceItem, 0x0034)
        ENSURE_OFFSET(ID64, 0x0160)
    END_FIXTURE()
    RUN_FIXTURE(CGItem)

};