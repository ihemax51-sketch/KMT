//
// Created by Kurama on 12/11/2022.
//
#pragma once

#include "Base.h"
#include "DBColum.h"

class CDBTable;

class CDBRecord : public CBase {
DECLARE_DYNAMIC_EXISTING(CDBRecord, 0x00add8ec)

private:
    char pad_0004[12]; //0x0004
    CDBTable *m_pDBTable; //0x0010
    char pad_0014[4]; //0x0014
public:
    virtual const char *GetTableName() const = 0;

    virtual class stTableRecordData *GetTableData() const = 0;

    virtual void AllocRowData(size_t cbSize);

    virtual void FlushRowDataPos();

    virtual void *GetRowData();

    virtual void SetRowData(CDBColum *pColumData, void *pValue);

    virtual void Func_8();

private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0018)
    END_FIXTURE()

    RUN_FIXTURE(CDBRecord)
};
