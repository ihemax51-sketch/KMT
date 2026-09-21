///******************************************************************************
/// \File CGStorage.h
///
/// \Desc
///
///******************************************************************************

#pragma once

#include <Windows.h>
#include <vector>

#include "Test/Test.h"

#include "ReferenceData/ReferenceData.h"
#include "IStorage.h"

class CGItem;
class CGObjChar;

class CGStorage {
public:
    virtual ~CGStorage();

public:
    /// \address 004b7e90
    bool SetItem(int nSlotId, CGItem *pItem);

    BYTE FindMyItem(CGStorage* pStorage, void* pItemData);

    /// \brief Remove the item by data
    /// \address 004b7f10
    CGItem *PopItem(void *pItemData);

    /// \address 004b91d0
    /// \returns The Removed item
    CGItem *PopItem(int nSlotId);

    /// \address 004b7f80
    bool MoveItem(int nSlotId, int nNewSlotId);

    /// \address 0004b8000
    CGItem *GetItem(int nSlotId);

    /// \address 004b8070
    CGItem *SplitItemTo(int nSplitToSlotId, int nSplitFromSlotId, int nCount);

    /// \address 004b80f0
    bool SwapItem(int nSlotId, int nSwapWithSlotId);

    /// \address 004b81e0
    void RemoveItem(void *pItemData, int a);

    /// \address 004b8170
    void RemoveItem(int nSlotId);

    /// \address 004b8260
    BYTE FindFirstEmptySlot(int nSearchFromId);

    /// \address 004b82e0
    BYTE FindTotalEmptySlot(int nSearchFromId);

    /// \address 004b8350
    CGItem *FindSameItem(CGItem *pItem, int nSearchFromId, BYTE *BTSlotIdOut);

    /// \address 004b83d0
    CGItem *FindItemHasSameTID(int a, int nSearchFromId, int nMaxSearchIndex, unsigned short usTypeId,
                               BYTE *btSlotIdOut);

    /// \address 004b8350
    CGItem *FindItemHasSameRefObj(int nSearchFromId, int nMaxSearchIndex,
                                  RefObjCommon *pRefObj,
                                  BYTE *btSlotIdOut);

    /// \address 004b84f0
    BYTE FindItemSlot(int nSearchFromId, CGItem *pItem);

    /// \address 004b8570
    BYTE FindItemSlot(void *pItemData);

    /// \address 004b85f0
    void *FindItemByDBID(int a1, int a2);

    /// \address 004b8670
    int CountCanBeMerged(int nItemSlotId, CGItem *pMyItem);

    /// \address 004b86f0
    int CountItemsHasSameRefObj(int a, int nSearchFromId, int nMaxSearchIndex, RefObjCommon *pRefObj);

    /// \address 004b8770
    int CountItem(int nSearchFromId, int nMaxSearchIndex, int bIsEmptySlot);

    /// \address 004b87f0
    int CountEmptySlot(int nSearchFromId, int nMaxSearchIndex);

    /// \address 004b8970
    BYTE ItemExists(int nSearchFromId);

    /// \address 004b89f0
    bool CanStoreThisItem(void *p);

    /// \address 004b8a60
    void MakeAllItemOfflined();

    /// \address 004b8d90
    void RefreshEquipParamEntryForPC(bool bRemoveKeeperParam);

    /// \address 004b8e00
    void RefreshEngagedCOSItem(void *pMaybeCOSData);

    /// \address 004b8e70
    bool CheckEquipsetIntegrity();

    /// \address 004b8ee0
    bool CheckAvatarsetIntegrity();

    /// \address 004b8f50
    void ProtectItemsFromPenaltyDrop(int bProtect);

    /// \address 004b9060
    void CheckRentalItemValidity();

    /// \address 004b9150
    bool PushItem(void *pSlotData, int a1);

    /// \note Unknown Param amk
    /// \address 004b9240
    bool AttachLinkedStorageToItem(void *p);

    /// \note I'm sure if it Item ptr
    /// \address 004b92b0
    bool CanPushItem(CGItem *pItem);

    /// \address 004b9320
    BYTE GetLatestAvailableSlot();

    /// \address 004b90d0
    void ApplySetItemOptLevel2ThisGroup(int a1, int a2);

    /// \address 004b93a0
    void UpdateRentItemInEquipSlot();
    int Testx(int param_1);
public:
    std::vector<CGItem *> m_vecItems; //0x0004
    CGObjChar *m_pOwner; //0x0014
    eStorageType m_nStorageType; //0x0018
    int m_cUseAbleSlot; //0x001C
    int m_nSlotsCount; //0x0020
private:
BEGIN_FIXTURE()
        ENSURE_SIZE(0x0024)
        ENSURE_OFFSET(m_vecItems, 0x0004)
        ENSURE_OFFSET(m_pOwner, 0x0014)
        ENSURE_OFFSET(m_nStorageType, 0x0018)
        ENSURE_OFFSET(m_cUseAbleSlot, 0x001C)
        ENSURE_OFFSET(m_nSlotsCount, 0x0020)
    END_FIXTURE()

    RUN_FIXTURE(CGStorage)
};