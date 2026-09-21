///******************************************************************************
/// \File GAvatarInventoryOP.h
///
/// \Desc Game avatar player inventorey operation
///
///******************************************************************************

#pragma once

#include "GStorageOP.h"

class CGAvatarInventoryOP : public CGStorageOP {
public:
    /// \address 004bbcd0
    void RefreshEquipParamEntryForPC(CGStorage *pStorage, bool bRemoveKeeperParam) override;

    /// \address 004bb8a0
    bool CheckAvatarsetIntegrity(CGStorage *pStorage) override;

    /// \address 004bbd60
    void CheckRentalItemValidity(CGStorage *pStorage) override;

    /// \address 004bbe60
    void UpdateRentItemInEquipSlot(CGStorage *pStorage) override;

    /// \address 004bb870
    virtual void Delete() override;
};
