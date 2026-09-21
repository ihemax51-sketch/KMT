#pragma once

#include "IFWnd.h"
#include "IFStoreForPackage.h"
#include "IFStorageRoom.h"
#include "SOItem.h"
#include "IFSlotWithHelp.h"
class CIFInventory : public CIFWnd
{
GFX_DECLARE_DYNAMIC_EXISTING(CIFInventory, 0x00eea7c8)
public:
    bool OnCreateIMPL(long ln);
    /// \address 0006b3e50
    CSOItem* GetItemBySlot(int slotId);
    int InventorySlotCount();
    void FakePick(int nRefObjId, BYTE btSlot, int nCount);
    CIFSlotWithHelp* GetItemSlotBySlotId(int slotId);
    bool Func_28_IMPL(CGWnd* a1, int a2, int a3);
public:
    CIFStoreForPackage* m_storeWindow; //0x036C
    CIFStorageRoom* m_storageWindow; //0x0370
    char pad_0374[12]; //0x0374
    int N00000183; //0x0380
    char pad_0384[4]; //0x0384
    CIFStatic* m_goldAmtText; //0x0388
    char pad_038C[12]; //0x038C
    CIFButton* m_moneyDropButton; //0x0398
    char pad_039C[16]; //0x039C
    std::n_vector<CIFSlotWithHelp*> pSlots; //0x03AC
    char pad_03B0[52]; //0x03B0*/// its newest

/*BEGIN_FIXTURE()
        ENSURE_SIZE(0x03F0)
        //ENSURE_OFFSET(Char, 0x036C)
    END_FIXTURE()
    RUN_FIXTURE(CIFInventory)
*/


};//Size=0x03F0