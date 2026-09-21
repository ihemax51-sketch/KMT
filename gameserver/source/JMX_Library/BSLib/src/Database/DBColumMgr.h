///******************************************************************************
/// \File DBColumMgr.h
///
/// \Desc the table colums
///
///******************************************************************************

#pragma once

#include <string>
#include <vector>
#include "DBColum.h"

/// \note Unknown class name so lets call it like that for now..
/// Maybe Colum keeper?
class CDBColumMgr {
    // Ctor 0082ae50
    // operator_= 00861bd0
public:
    std::string m_strUnk; // Primary key?
    std::string m_strUnk2; // Secondary key?
    std::vector<CDBColum> m_vecColumList; //0x0038
private:
    BEGIN_FIXTURE()
    ENSURE_SIZE(0x0048)
    ENSURE_OFFSET(m_vecColumList, 0x0038)
    END_FIXTURE()

    RUN_FIXTURE(CDBColumMgr)
};
