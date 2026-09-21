#ifndef CINSTANCEPC_H
#define CINSTANCEPC_H

#include <Test/Test.h>
#include <ReferenceData/ReferenceData.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
class CInstancePC
{
public:
    char pad_0000[8]; //0x0000
    int classsize; //0x0008
    void *iDbTable; //0x000C
    char pad_0010[4]; //0x0010
    RefObjChar *iRefObjChar; //0x0014
    char pad_0018[8]; //0x0018
    uint32_t CharDBID; //0x0020
    char pad_0024[4]; //0x0024
    uint32_t CharRefObjID; //0x0028
    int StructID; //0x002C
    char CharName16[4]; //0x0030
    char pad_0034[52]; //0x0034
    uint32_t ExpOffset; //0x0068
    char pad_006C[8]; //0x006C
    uint16_t Strength; //0x0074
    uint16_t Intellect; //0x0076
    UINT64 Gold; //0x0078
    uint32_t RemainSkillPoint; //0x0080
    char pad_0084[8]; //0x0084
    uint32_t CurHealth; //0x008C
    uint32_t CurMana; //0x0090
    char pad_0094[16]; //0x0094
    uint32_t OwnerTeleportGate; //0x00A4
    char pad_00A8[4]; //0x00A8
    uint32_t InventorySize; //0x00AC
    char pad_00B0[48]; //0x00B0

private:
BEGIN_FIXTURE()
        ENSURE_SIZE(224)
        ENSURE_OFFSET(iDbTable, 0x000C)
    END_FIXTURE()

    RUN_FIXTURE(CInstancePC)
}; //Size: 0x00F8


#endif