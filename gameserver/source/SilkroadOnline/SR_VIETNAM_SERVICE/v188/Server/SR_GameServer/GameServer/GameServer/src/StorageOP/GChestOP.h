///******************************************************************************
/// \File GChestOP.h
///
/// \Desc Game player chest operation
///
///******************************************************************************

#pragma once

#include "GStorageOP.h"

class CGChestOP : public CGStorageOP {
public:
    //what the heck how did it jump to here
    //CGPCInventoryOP::Func_27

    /// \address 004bbf50
    bool AttachLinkedStorageToItem(CGStorage *pStorage, void *p) override;

    /// \address 004bbf20
    void Delete() override;
};