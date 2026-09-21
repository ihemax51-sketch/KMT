///******************************************************************************
/// \File GGuildChestOP.h
///
/// \Desc Game guild storage operation
///
///******************************************************************************

#pragma once

#include "GStorageOP.h"

class CGGuildChestOP : public CGStorageOP {
public:
    //what the heck how did it jump to here
    //CGPCInventoryOP::Func_27

    /// \address 004bc040
    void Delete() override;
};