///******************************************************************************
/// \File DBColum.h
///
/// \Desc Cloum Database data
///
///******************************************************************************

#pragma once

#include "Windows.h"
#include <string>

#include "Test/Test.h"

enum COLUM_VALUE_TYPE : DWORD {
    COLUM_VALUE_TYPE_TINYINT,
    COLUM_VALUE_TYPE_INTEGER,
    COLUM_VALUE_TYPE_DECIMAL,
    COLUM_VALUE_TYPE_LONGLONG,
    COLUM_VALUE_TYPE_FLOAT,
    COLUM_VALUE_TYPE_STRING = 6,
    COLUM_VALUE_TYPE_DATE,
};

/// \note Unknown class name so lets call it like that for now..
class CDBColum {
    // Ctor 00862eb0
public:
    std::string m_strColumName; //0x0000
    COLUM_VALUE_TYPE m_dwValueType; //0x001C
    char pad_0020[4]; //0x0020
    size_t m_cbBufferSize; //0x0024
    char pad_0028[4]; //0x0028
    DWORD m_bNullAble; //0x002C
    DWORD m_dwColumPos; //0x0030
    DWORD m_bIdentity; //0x0034
    std::string m_strValueFormat; //0x0038
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0054)
        ENSURE_OFFSET(m_strColumName, 0x0000)
        ENSURE_OFFSET(m_dwValueType, 0x001C)
        ENSURE_OFFSET(m_cbBufferSize, 0x0024)
        ENSURE_OFFSET(m_bNullAble, 0x002C)
        ENSURE_OFFSET(m_dwColumPos, 0x0030)
        ENSURE_OFFSET(m_bIdentity, 0x0034)
        ENSURE_OFFSET(m_strValueFormat, 0x0038)
    END_FIXTURE()

    RUN_FIXTURE(CDBColum)
};
