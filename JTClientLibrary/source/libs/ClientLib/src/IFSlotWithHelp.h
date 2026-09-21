#pragma once

#include "IFWnd.h"

#include "SOItem.h"

enum eSlotState
{
    GDR_FELLOW_SKILL_SLOT_DEACTIVE = -15,
    GDR_FELLOW_SKILL_SLOT_ACTIVE = -16,
};
class CIFSlotWithHelp : public CIFWnd {
GFX_DECLARE_DYNAMIC_EXISTING(CIFSlotWithHelp, 0x00ee9B18)
public:
public:
    void SetSlotData(CSOItem *pItemSocket);
    bool Func_28IMPL(CGWnd* a1, int a2, int a3);
    int GetItemSourceParentWindowId() const;

    CSOItem* GetItem() const;

    const CSOItem &GetMyItem() const;

    /// \address 00666800
    int GetInventorySlotIndex() const;


    /// \address 00676570
    void UseItem();
    void GetSkillInfo(int SkillID);
    void SetSlotVisual(undefined4 pSlot);
    /// \address 00682040
    void FUN_00682040();
    int GetType();
    int GetParentWindowId();
    void CopySlot(CIFSlotWithHelp *pSlot);
    int GetSlot();
    void GetIconSlot(int Type, int a1);
    int GetSlotType();
    void ClearSlot();
    void FUN_006809a00();
    int GetSkillSlotInDex();
    int GetInventorySlotType();
    void SetSlot(int Slot);
    void SetType(int Value);
    void SetInventorySlotType(int SlotType);
    void SetSlotType(int Type);
    void SetRareName(class CIFHelperBubbleWindow *window);
    void FUN_006821f02(CIFHelperBubbleWindow *window);
    void WriteItemname(class CIFHelperBubbleWindow *a1);
    void WriteItemClass(class CIFHelperBubbleWindow *a1);
    void WriteItemType(class CIFHelperBubbleWindow *a1);
    void WriteItemType2(class CIFHelperBubbleWindow *a1);
    void WriteItemType3(class CIFHelperBubbleWindow *a1);
    void WriteItemType4(class CIFHelperBubbleWindow *a1);
    void WriteItemType5(class CIFHelperBubbleWindow *a1);
    void ClearSlotEmpty();
    void RenderMyselfIMPL();
    void AppendAdvancedInfo(class CIFHelperBubbleWindow * window);
    int OnMouseLeftUpIMPL(int a1, int x, int y);
    int OnMouseLeftDownIMPL(int a1, int x, int y);
    int OnMouseRightUPIMPL(int a1, int x, int y);
    void sub_686DB0() const;
private:
    int Type; //0x036C
    int m_itemSourceParentWindowId; // 0x0370
    int SlotNum; //0x0374
public:
    int m_inventorySlotIndex; //0x0378
    char pad_037c[0x0390 - 0x037c]; //0x037c
public:
    CSOItem* ItemInfo; // 0x0390
    char pad_0394[4]; //0x0394
    CSOItem m_myItem; // 0x0398
    char pad_0568[12]; //0x0568
    int N00000B60; //0x0574
    char pad_0578[28]; //0x0578
    byte N00000B68; //0x0594
    byte N595; //0x0595
    byte N00000FEF; //0x0596
    byte N00000FED; //0x0597
    char pad_0598[4]; //0x0598
    int AktifDeaktif; //0x059C
    char pad_05A0[188]; //0x05A0
    int m_SkillID; //0x065C
    char pad_0660[12]; //0x0660
    int active; //0x066C
    char pad_0670[72]; //0x0670
    int SlotisLocked;//0x06B8
    char pad_06BC[12];   //0x06BC

BEGIN_FIXTURE()

        ENSURE_SIZE(1736)
        ENSURE_OFFSET(Type, 0x036C)
        ENSURE_OFFSET(ItemInfo, 0x0390)
    END_FIXTURE()
    RUN_FIXTURE(CIFSlotWithHelp)



};