///******************************************************************************
/// \File GPCInventoryOP.h
///
/// \Desc Game play inventorey operation
///
///******************************************************************************

#pragma once

#include "GStorageOP.h"

class CGPCInventoryOP : public CGStorageOP {
public:
    /// \address 004bc070
    bool CanStoreThisItem(CGStorage *pStorage, void *p) override;

    /// \address 004bb460
    BYTE RandomSelectNonEmptySlotForDeathPenalty(CGStorage *pStorage, int a) override;

    /// \address 004bb3e0
    void RefreshEquipParamEntryForPC(CGStorage *pStorage, bool bRemoveKeeperParam) override;

    /// \address 004bb020
    virtual bool CheckEquipsetIntegrity(CGStorage *pStorage) override;

    /// \address 004bb580
    bool AttachLinkedStorageToItem(CGStorage *pStorage, void *p) override;

    /// \address 004bb680
    void ApplySetItemOptLevel2ThisGroup(CGStorage *pStorage, int a1, int a2) override;

    /// \address 004bb730
    void UpdateRentItemInEquipSlot(CGStorage *pStorage) override;

    /// \address 004baff0
    void Delete() override;

private:
};