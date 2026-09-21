///******************************************************************************
/// \File DBRecStream.h
///
/// \Desc
///
///******************************************************************************

#pragma once

#include "DBRecord.h"

class CDBRecStream : public CDBRecord {
private:
    void *m_pRowData; //0x0018
    char pad_001c[4]; //0x001C
    DWORD m_dwReadRowDataPos; //0x0020
    size_t m_cbRowSize; //0x0024
    char pad_0028[8]; //0x0028
public:
    void AllocRowData(size_t cbSize) override;

    void FlushRowDataPos() override;

    void *GetRowData() override;

    void SetRowData(CDBColum *pColumData, void *pValue) override;

private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0030)
        ENSURE_OFFSET(m_pRowData, 0x0018)
        ENSURE_OFFSET(m_dwReadRowDataPos, 0x0020)
        ENSURE_OFFSET(m_cbRowSize, 0x0024)
    END_FIXTURE()

    RUN_FIXTURE(CDBRecStream)
};
