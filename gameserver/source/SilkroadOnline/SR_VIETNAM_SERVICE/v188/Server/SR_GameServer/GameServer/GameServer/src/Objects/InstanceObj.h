//
// Created by Kurama on 12/11/2022.
//
#pragma once

#include "Database/DBRecord.h"

#include "ReferenceData/ReferenceData.h"

class CInstanceObj : public CDBRecord {
DECLARE_DYNAMIC_EXISTING(CInstanceObj, 0x00ade3d0)

public:
    void FlushRowDataPos() override;

    virtual void InitObjectDataPtr() = 0;

    virtual void Func_10() = 0;

private:
    RefObjCommon *m_pObjectData; //0x0018
    char pad_001c[4]; //0x001c
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0020)
    END_FIXTURE()

    RUN_FIXTURE(CInstanceObj)
};