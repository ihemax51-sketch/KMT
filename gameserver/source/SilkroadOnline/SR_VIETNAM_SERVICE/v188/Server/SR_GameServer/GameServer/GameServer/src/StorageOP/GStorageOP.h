///******************************************************************************
/// \File GStorageOP.h
///
/// \Desc Game storage operation
///
///******************************************************************************

#pragma once

#include <Objects/GItem.h>
#include "ghidra/undefined.h"

#include "IStorage.h"

class CGStorage;
class RefObjCommon;


class CGStorageOP : public IStorage_ {
public:
    /// \note its maybe the destructor
    /// \address 004b9ab0
    virtual void OnDelete();

    /// \address 004b9be0
    virtual bool SetItem(CGStorage *pStorage, int nSlotId, CGItem *pItem);

    /// \brief Remove item
    virtual CGItem *PopItem(CGStorage *pStorage, void *pItemData);

    /// \brief Get the item and then remove it from the storage
    /// \address 004b9b50
    virtual CGItem *PopItem(CGStorage *pStorage, int nSlotId);

    /// \address 004b9ac0
    virtual bool MoveItem(CGStorage *pStorage, int nSlotId, int nNewSlotId);

    /// \address 004b9ba0
    virtual CGItem *GetItem(CGStorage *pStorage, int nSlotId);

    /// \address 004b9c60
    virtual CGItem *SplitItemTo(CGStorage *pStorage, int nSplitToSlotId, int nSplitFromSlotId, int nCount);

    /// \address 004b9db0
    virtual bool SwapItem(CGStorage *pStorage, int nSlotId, int nSwapWithSlotId);

    /// \address 004b9e40
    virtual void RemoveItem(CGStorage *pStorage, void *pItemData, undefined4);

    /// \address 004b9ef0
    virtual void RemoveItem(CGStorage *pStorage, int nSlotId);

    /// \address 004b9f20
    virtual BYTE FindFirstEmptySlot(CGStorage *pStorage, int nSearchFromId);

    /// \address 004b9f80
    virtual BYTE FindTotalEmptySlot(CGStorage *pStorage, int nSearchFromId);

    /// \address 004ba5f0
    virtual CGItem *FindSameItem(CGStorage *pStorage, int a, CGItem *pItem, int nSearchFromId, BYTE *btSlotIdOut);

    /// \address 004ba130
    virtual CGItem *FindItemHasSameTID(CGStorage *pStorage, int a, int nSearchFromId, int nMaxSearchIndex, unsigned short usTypeId,
                                       BYTE *btSlotIdOut);

    /// \address 004ba380
    virtual CGItem *FindItemHasSameRefObj(CGStorage *pStorage, int a, int nSearchFromId, int nMaxSearchIndex,
                                          RefObjCommon *pRefObj,
                                          BYTE *btSlotIdOut);

    /// \address 004b9ff0
    virtual BYTE FindItemSlot(CGStorage *pStorage, void *pItemData);

    /// \address 004ba040
    virtual BYTE FindItemSlot(CGStorage *pStorage, int nSearchFromId, CGItem *pItem);

    /// \address 004ba460
    virtual void *FindItemByDBID(CGStorage *pStorage, int a1, int a2, int nSearchFromId, int nMaxSearchIndex);

    /// \address 004ba420
    virtual void *FindItemByDBID(CGStorage *pStorage, int a1, int a2);

    /// \address 004b9d40
    virtual int CountCanBeMerged(CGStorage *pStorage, int nItemSlotId, CGItem *pMyItem);

    /// \address 004ba270
    virtual int CountItemsHasSameRefObj(CGStorage *pStorage, int a, int nSearchFromId, int nMaxSearchIndex, RefObjCommon *pRefObj);

    /// \address 004ba310
    virtual void CountEquiHasSameSetID(CGStorage *pStorage, int nSetId , BYTE *btCountOut);

    /// \address 004ba500
    virtual int CountItem(CGStorage *pStorage, int nSearchFromId, int nMaxSearchIndex, int bIsEmptySlot);

    /// \address 004ba570
    virtual int CountEmptySlot(CGStorage *pStorage, int nSearchFromId, int nMaxSearchIndex);

    /// \address 004ba090
    virtual CGItem *GetFirstItem(CGStorage *pStorage, int nSearchFromId, int nMaxSearchIndex, BYTE *btSlotIdOut);

    /// \address 004ba100
    virtual BYTE RandomSelectNonEmptySlotForDeathPenalty(CGStorage *pStorage, int a);

    /// \address 004ba590
    virtual BYTE ItemExists(CGStorage *pStorage, int nSearchFromId);

    virtual bool CanStoreThisItem(CGStorage *pStorage, void *p);

    /// \address 004ba6a0
    virtual void MakeAllItemOfflined(CGStorage *pStorage);

    /// \address 004ba720
    virtual void Func_29(undefined4, undefined4, undefined4);

    /// \address 004ba880
    virtual void Func_30(undefined4, undefined4, undefined4, undefined4, undefined4);

    /// \address 004ba910
    virtual void RefreshEquipParamEntryForPC(CGStorage *pStorage, bool bRemoveKeeperParam);

    /// \address 004ba940
    virtual void RefreshEngagedCOSItem(CGStorage *pStorage, void *pMaybeCOSData);

    /// \address 004baab0
    virtual bool CheckEquipsetIntegrity(CGStorage *pStorage);

    /// \address 004baae0
    virtual bool CheckAvatarsetIntegrity(CGStorage *pStorage);

    /// \address 004bab10
    virtual void ProtectItemsFromPenaltyDrop(CGStorage *pStorage, int bProtect);

    /// \note Nuh alot of params, feel free or as u wish do it plz... 'lazy fox'
    /// \address 004bab70
    virtual void InsertItemWithCreatedItemInDataBase(undefined4, undefined4, undefined4, undefined4, undefined4, undefined4, undefined4, undefined4, undefined4,
            undefined4, undefined4, undefined4, undefined4, undefined4);

    /// \address 004baea0
    virtual void CheckRentalItemValidity(CGStorage *pStorage);

    /// \brief Quick slot storage shit
    /// \address 004baed0
    virtual bool PushItem(CGStorage *pStorage, void *pSlotData, int a1);

    /// \address 004baf00
    virtual bool AttachLinkedStorageToItem(CGStorage *pStorage, void *p);

    /// \address 004baf30
    virtual bool CanPushItem(CGStorage *pStorage, CGItem *pItem);

    /// \address 004baf60
    virtual BYTE GetLatestAvailableSlot(CGStorage *pStorage);

    /// \address 004baf90
    virtual void ApplySetItemOptLevel2ThisGroup(CGStorage *pStorage, int a1, int a2);

    /// \address 004bafc0
    virtual void UpdateRentItemInEquipSlot(CGStorage *pStorage);

    /// \note unknown params
    /// \address 004b9a80
    virtual void Delete();

private:

};