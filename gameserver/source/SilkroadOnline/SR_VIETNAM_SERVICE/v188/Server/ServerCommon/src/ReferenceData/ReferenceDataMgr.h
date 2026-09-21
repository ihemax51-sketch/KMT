///******************************************************************************
/// \File ReferenceDataMgr.h
///
/// \Desc Have all the ReferencData in the game
///
///******************************************************************************

#pragma once

#include "ReferenceData.h"

#include <list>

class CRefDataManager {
    // Ctor 007158f0
public:
    RefObjCommon *GetRefObj(DWORD dwObjID) const;

private:
    char pad_0000[108]; //0x0000
    std::list<std::pair<DWORD, RefObjCommon *>> m_listRefGameObj; //0x006C
private:
BEGIN_FIXTURE()
        ENSURE_OFFSET(m_listRefGameObj, 0x006C)
    END_FIXTURE()

    RUN_FIXTURE(CRefDataManager)
};

#define REFDATA_MGR (**reinterpret_cast<CRefDataManager **>(0x00d6aa14))
