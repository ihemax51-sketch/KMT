///******************************************************************************
/// \File RefObjItem.h
///
/// \Desc
///
///******************************************************************************

#pragma once

#include "Database/DBRecStream.h"

class CRefObjItem : public CDBRecStream {
public:
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0030)
    END_FIXTURE()

    RUN_FIXTURE(CRefObjItem)
};
