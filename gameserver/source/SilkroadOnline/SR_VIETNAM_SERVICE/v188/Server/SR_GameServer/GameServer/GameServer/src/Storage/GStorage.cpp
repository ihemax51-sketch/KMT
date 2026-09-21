///******************************************************************************
/// \File CGStorage.cpp
///
/// \Desc
///
///******************************************************************************

#include "Helpers.h"
#include "ServerFramework/ServerFramework.h"

#include "GStorageOP.h"
#include "GStorage.h"

#include "GObjChar.h"
#include <stdexcept>

#define ASSERT_STORAGE_OP_NULL() \
    ServerFramework::ReportLog(LOG_TYPE_FATAL, \
                                "## %s %s %d ## CharName[%s] / StorageType[%d]", \
                                __FILE__, __FUNCTION__, __LINE__, \
                                m_pOwner->GetCharName(), m_nStorageType); \


bool CGStorage::SetItem(int nSlotId, CGItem *pItem) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return false;
    }

    return pStorageOP->SetItem(this, nSlotId, pItem);
}

BYTE CGStorage::FindMyItem(CGStorage* pStorage, void* pItemData)
{
    printf("%p %p \n", pStorage, pItemData);
    return reinterpret_cast<BYTE(__thiscall*)(CGStorage*, CGStorage*, void*)>(0x004b9ff0)(this, pStorage, pItemData);
}

CGItem *CGStorage::PopItem(void *pItemData) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return NULL;
    }

    return pStorageOP->PopItem(this, pItemData);
}

CGItem *CGStorage::PopItem(int nSlotId) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return NULL;
    }

    return pStorageOP->PopItem(this, nSlotId);
}

bool CGStorage::MoveItem(int nSlotId, int nNewSlotId) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return false;
    }

    return pStorageOP->MoveItem(this, nSlotId, nNewSlotId);
}

CGItem *CGStorage::GetItem(int nSlotId) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return NULL;
    }

    return pStorageOP->GetItem(this, nSlotId);
}

CGItem *CGStorage::SplitItemTo(int nSplitToSlotId, int nSplitFromSlotId, int nCount) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return NULL;
    }

    return pStorageOP->SplitItemTo(this, nSplitToSlotId, nSplitFromSlotId, nCount);
}

bool CGStorage::SwapItem(int nSlotId, int nSwapWithSlotId) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return false;
    }

    return pStorageOP->SwapItem(this, nSlotId, nSwapWithSlotId);
}

void CGStorage::RemoveItem(void *pItemData, int a) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return;
    }

    pStorageOP->RemoveItem(this, pItemData, a);
}

void CGStorage::RemoveItem(int nSlotId) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return;
    }

    pStorageOP->RemoveItem(this, nSlotId);
}

BYTE CGStorage::FindFirstEmptySlot(int nSearchFromId) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return 255;
    }

    return pStorageOP->FindFirstEmptySlot(this, nSearchFromId);
}

BYTE CGStorage::FindTotalEmptySlot(int nSearchFromId) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return 0;
    }

    return pStorageOP->FindTotalEmptySlot(this, nSearchFromId);
}

CGItem *CGStorage::FindSameItem(CGItem *pItem, int nSearchFromId, BYTE *btSlotIdOut) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return NULL;
    }

    return pStorageOP->FindSameItem(this, 1, pItem, nSearchFromId, btSlotIdOut);
}

CGItem *CGStorage::FindItemHasSameTID(int a, int nSearchFromId, int nMaxSearchIndex, unsigned short usTypeId,
                                      BYTE *btSlotIdOut) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return NULL;
    }

    return pStorageOP->FindItemHasSameTID(this, a, nSearchFromId, nMaxSearchIndex, usTypeId, btSlotIdOut);
}

CGItem *CGStorage::FindItemHasSameRefObj(int nSearchFromId, int nMaxSearchIndex, RefObjCommon *pRefObj,
                                         BYTE *btSlotIdOut) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return NULL;
    }

    return pStorageOP->FindItemHasSameRefObj(this, true, nSearchFromId, nMaxSearchIndex, pRefObj, btSlotIdOut);
}

BYTE CGStorage::FindItemSlot(int nSearchFromId, CGItem *pItem) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return 255;
    }

    return pStorageOP->FindItemSlot(this, nSearchFromId, pItem);
}

BYTE CGStorage::FindItemSlot(void *pItemData) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return 255;
    }

    return pStorageOP->FindItemSlot(this, pItemData);
}

void *CGStorage::FindItemByDBID(int a1, int a2) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return NULL;
    }

    return pStorageOP->FindItemByDBID(this, a1, a2);
}

int CGStorage::CountCanBeMerged(int nItemSlotId, CGItem *pMyItem) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return 0;
    }

    return pStorageOP->CountCanBeMerged(this, nItemSlotId, pMyItem);
}

int CGStorage::CountItemsHasSameRefObj(int a, int nSearchFromId, int nMaxSearchIndex, RefObjCommon *pRefObj) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return 0;
    }

    return pStorageOP->CountItemsHasSameRefObj(this, a, nSearchFromId, nMaxSearchIndex, pRefObj);
}

int CGStorage::CountItem(int nSearchFromId, int nMaxSearchIndex, int bIsEmptySlot) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return 0;
    }

    return pStorageOP->CountItem(this, nSearchFromId, nMaxSearchIndex, bIsEmptySlot);
}

int CGStorage::CountEmptySlot(int nSearchFromId, int nMaxSearchIndex) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return 0;
    }

    return CountItem(nSearchFromId, nMaxSearchIndex, false);
}

BYTE CGStorage::ItemExists(int nSearchFromId) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return 255;
    }

    return pStorageOP->ItemExists(this, nSearchFromId);
}

bool CGStorage::CanStoreThisItem(void *p) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return false;
    }

    return pStorageOP->CanStoreThisItem(this, p);
}

void CGStorage::MakeAllItemOfflined() {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return;
    }

    pStorageOP->MakeAllItemOfflined(this);
}

void CGStorage::RefreshEquipParamEntryForPC(bool bRemoveKeeperParam) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return;
    }

    pStorageOP->RefreshEquipParamEntryForPC(this, bRemoveKeeperParam);
}

void CGStorage::RefreshEngagedCOSItem(void *pMaybeCOSData) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return;
    }

    pStorageOP->RefreshEngagedCOSItem(this, pMaybeCOSData);
}

bool CGStorage::CheckEquipsetIntegrity() {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return false;
    }

    return pStorageOP->CheckEquipsetIntegrity(this);
}

bool CGStorage::CheckAvatarsetIntegrity() {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return false;
    }

    return pStorageOP->CheckAvatarsetIntegrity(this);
}

void CGStorage::ProtectItemsFromPenaltyDrop(int bProtect) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return;
    }

    pStorageOP->ProtectItemsFromPenaltyDrop(this, bProtect);
}

void CGStorage::CheckRentalItemValidity() // 3078 packet 
{
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return;
    }

    pStorageOP->CheckRentalItemValidity(this);
}


bool CGStorage::PushItem(void *pSlotData, int a1) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return false;
    }

    return pStorageOP->PushItem(this, pSlotData, a1);
}

bool CGStorage::AttachLinkedStorageToItem(void *p) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return false;
    }

    return pStorageOP->AttachLinkedStorageToItem(this, p);
}

bool CGStorage::CanPushItem(CGItem *pItem) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return false;
    }

    return pStorageOP->CanPushItem(this, pItem);
}

BYTE CGStorage::GetLatestAvailableSlot() {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return 255;
    }

    return pStorageOP->GetLatestAvailableSlot(this);
}

void CGStorage::ApplySetItemOptLevel2ThisGroup(int a1, int a2) {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return;
    }
    printf("%d %d \n", a1, a2);
    pStorageOP->ApplySetItemOptLevel2ThisGroup(this, a1, a2);
}

void CGStorage::UpdateRentItemInEquipSlot() {
    if (m_pOwner == NULL)
        throw std::runtime_error("CGStorage owner is unavailable");

    CGStorageOP *pStorageOP = (CGStorageOP *) CGObjChar::GetStorageOP(m_nStorageType);
    if (pStorageOP == NULL) {
        ASSERT_STORAGE_OP_NULL()
        return;
    }

    pStorageOP->UpdateRentItemInEquipSlot(this);
}


int CGStorage::Testx(int param_1) {
    return -1;
return    reinterpret_cast<int(__thiscall*)(CGStorage*, int)>(0x00524950)(this, param_1);
}
