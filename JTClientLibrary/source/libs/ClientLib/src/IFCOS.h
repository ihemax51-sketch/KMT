#pragma once


#include "IFMainFrame.h"
#include "IFSelectableArea.h"
#include "IFCOSInfo.h"
#include "IFCOSInventory.h"
#include "IFCOSSetup.h"

class CIFCOS : public CIFMainFrame {
GFX_DECLARE_DYNAMIC_EXISTING(CIFCOS, 0x00EEC048)

public:
    //0xEEC048
    bool OnCreateIMPL(long ln);

    void OnUpdateIMPL();
    void Switch(int p1, int i);
    void FUN_0079fc20(int p1);
    void FUN_0079fcd0(int i);
    GFX_MSGMAP* MessageMap();
    void ActiveTab2();
    void ActiveTab3();
    void PressTabs();
    void ApplyFellowPetLayout(bool isFellowPet);
    void CreatePetInventoryButtons();
    void UpdatePetInventoryButtons();
    void UpdatePetSetupFilterButton();
    void OnPetAutoSort();
    void OnPetConvertToInventory();
    void ProcessPetInventoryActions();
public:
    int m_petUniqueID; //0x7bc
    BYTE m_CurrentTabID; //0x7c0
    char pad_07C1[3]; //0x07C1
    CIFSelectableArea* m_SelectableArea; //0x7c4
    CIFSelectableArea* m_SelectableArea1; //0x7c8
    CIFSelectableArea* m_SelectableArea2; //0x7cc
    CIFCOSInfo* m_CosInfo; //0x7d0
    CIFCOSInventory* m_COSInventory; //0x7d4
    CIFCOSSetup* m_COSSetup; //0x7d8
BEGIN_FIXTURE()
        ENSURE_SIZE(0x7dc)
        ENSURE_OFFSET(m_petUniqueID, 0x7bc)
        ENSURE_OFFSET(m_CurrentTabID, 0x7c0)
        ENSURE_OFFSET(m_SelectableArea, 0x7c4)
        ENSURE_OFFSET(m_SelectableArea1, 0x7c8)
        ENSURE_OFFSET(m_SelectableArea2, 0x7cc)
        ENSURE_OFFSET(m_CosInfo, 0x7d0)
        ENSURE_OFFSET(m_COSInventory, 0x7d4)
        ENSURE_OFFSET(m_COSSetup, 0x7d8)
    END_FIXTURE()

    RUN_FIXTURE(CIFCOS)




}; //size 0x7dc

