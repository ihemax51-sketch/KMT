#pragma once

#include "CObjCharacter.h"
#include "IFWnd.h"
#include "SOItem.h"
#include "IFEquipSkillSlotWnd.h"
#include "IFSlotWithHelp.h"
class CIFEquipment : public CIFWnd
{
public:
    bool OnCreateIMPL(long ln);
    void AutoSort();
    void AutoOrder();
    void ApplyInventoryDesignLayout();
    GFX_MSGMAP* MessageMap();
    class CSOItem* GetEquipmentObjectBySlot(BYTE SlotIndex);

    void OnUpdateIMPL();
    void SetSlotLock(byte slot);
    void SetSlotUnLock(byte slot);
    void SetItemTime(byte slot, long min);
    class CSOItem* Test4(BYTE param_1);
    void TakeItem(int param_1);
public:

    /*  CCObjCharacter *Char; //0x036C
      char pad_0370[9040]; //0x0370

  BEGIN_FIXTURE()
          ENSURE_SIZE(0x26C0)
          ENSURE_OFFSET(Char, 0x036C)
      END_FIXTURE()
      RUN_FIXTURE(CIFEquipment)*/

    CCObjCharacter Char; //0x036C
    char pad_0510[212]; //0x0510
    CIFSlotWithHelp* AVATARHATSLOT;
    CIFSlotWithHelp* AVATARMAINSLOT;
    CIFSlotWithHelp* AVATAR_ATTACHSLOT;
    CIFSlotWithHelp* FLAGSLOT;
    CIFSlotWithHelp* DEVILSLOT;
    CSOItem AvatarSlot; //0x05F8
    class CSOItem N0000260E; //0x07C8
    class CSOItem N0000260F; //0x0998
    class CSOItem N00002610; //0x0B68
    CSOItem DevillSlot; //0x0D38
    int ActivePage; //0x0F08
    CIFEquipSkillSlotWnd * pSkillSlotWnd; //0x0F0C
    class CSOItem N00002614; //0x0F10
    class CSOItem N00002615; //0x10E0
    class CSOItem N00002616; //0x12B0
    class CSOItem N00002617; //0x1480
    class CSOItem N00002618; //0x1650
    class CSOItem N00002619; //0x1820
    CSOItem WeaponItem; //0x19F0
    class CSOItem N0000261B; //0x1BC0
    class CSOItem N0000261C; //0x1D90
    class CSOItem N0000261D; //0x1F60
    class CSOItem N0000261E; //0x2130
    class CSOItem N0000261F; //0x2300
    class CSOItem N00002620; //0x24D0
    char pad_26A0[4]; //0x26A0
    void *CIFButton; //0x26A4
    void *CIFButton2; //0x26A8
    void *CIFButton3; //0x26AC
    void *CIFButton4; //0x26B0
    void *CIFButton5; //0x26B4
    void *CIFPopupWnd; //0x26B8
    char pad_26BC[4]; //0x26BC

BEGIN_FIXTURE()
        ENSURE_SIZE(9920)
        ENSURE_OFFSET(Char, 0x036C)
    END_FIXTURE()
    RUN_FIXTURE(CIFEquipment)




}; //Size=0x26C0

// Applies the fixed native renderer coordinates before CIFEquipment is created.
// The setting is intentionally startup-only; switching an existing window live is unsupported.
void ConfigureInventoryDesignPatches(bool enableNewDesign);

