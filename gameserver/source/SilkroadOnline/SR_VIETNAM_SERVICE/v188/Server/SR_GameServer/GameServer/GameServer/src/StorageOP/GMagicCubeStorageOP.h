///******************************************************************************
/// \File GMagicCubeStorageOP.h
///
/// \Desc Game quickslot sotrage operation
///
///******************************************************************************

#pragma once

#include "GStorageOP.h"

class CGMagicCubeStorageOP : public CGStorageOP {
public:
    /// \address 004bc370
    CGItem *PopItem(CGStorage *pStorage, void *pItemData) override;

    /// \address 004bc570
    int CountItemsHasSameRefObj(CGStorage *pStorage, int a, int nSearchFromId, int nMaxSearchIndex, RefObjCommon *pRefObj) override;

    /// \address 004bc150
    bool PushItem(CGStorage *pStorage, void *pSlotData, int a1) override;

    /// \address 004bc4f0
    bool CanPushItem(CGStorage *pStorage, CGItem *pItem) override;

    /// \address 004bc0c0
    BYTE GetLatestAvailableSlot(CGStorage *pStorage) override;

    /// \address 004bc090
    void Delete() override;
};