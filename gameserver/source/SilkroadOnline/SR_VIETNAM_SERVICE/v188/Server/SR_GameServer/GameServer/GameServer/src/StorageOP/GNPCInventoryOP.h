///******************************************************************************
/// \File GNPCInventoryOP.h
///
/// \Desc Game npc inventorey operation
///
///******************************************************************************

#pragma once

#include "GStorageOP.h"

class CGNPCInventoryOP : public CGStorageOP {
public:
    bool CanStoreThisItem(CGStorage *pStorage, void *p) override;

    /// \address 004b9a80
    void Delete() override;
};