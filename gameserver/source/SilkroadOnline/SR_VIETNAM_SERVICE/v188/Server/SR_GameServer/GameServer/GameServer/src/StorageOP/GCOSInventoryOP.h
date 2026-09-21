///******************************************************************************
/// \File GCOSInventoryOP.h
///
/// \Desc Game cos inventorey operation
///
///******************************************************************************

#pragma once

#include "GStorageOP.h"

class CGCOSInventoryOP : public CGStorageOP {
public:
    /// \address 004bb850
    bool CanStoreThisItem(CGStorage *pStorage, void *p) override;

    /// \address 004bb820
    void Delete() override;
};