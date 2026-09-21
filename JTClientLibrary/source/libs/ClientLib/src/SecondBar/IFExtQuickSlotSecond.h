///******************************************************************************
/// \File IFExtQuickSlot.h
///
/// \Desc
///
/// \Author kyuubi09 on 2/16/2022.
///
/// \Copyright Copyright © 2023 SRO_DevKit.
///
///******************************************************************************

#pragma once

#include <IFExtQuickSlotSlot.h>
#include "IFWnd.h"
#include "IFExtQuickSlotOption.h"

#include "SRIFLIB/NIFUnderMenuBar.h"
#include "IFMySlot.h"

class CIFExtQuickSlotSecond : public CIFWnd {
GFX_DECLARE_DYNCREATE(CIFExtQuickSlotSecond)

GFX_DECLARE_MESSAGE_MAP(CIFExtQuickSlotSecond)

protected:
    enum TYPE : BYTE {
        TYPE_HORIZONTAL_1LINE,
        TYPE_HORIZONTAL_2LINE,
        TYPE_VERTICAL_1LINE,
        TYPE_VERTICAL_2LINE,
        DUMMY = 255
    };

    enum {
        GDR_EXT_QUICKSLOT_OPTION = 5, // CIFExtQuickSlotOption
        GDR_EXT_QUICKSLOT_WND = 10, // CIFDragableArea
        GDR_EXT_QUICKSLOT_OPT_BTN = 11, // CIFButton
        GDR_EXT_QUICKSLOT_HORI_BTN = 12, // CIFButton
        GDR_EXT_QUICKSLOT_VERT_BTN = 13, // CIFButton
        GDR_EXT_QUICKSLOT_ONOFF_BTN = 14, // CIFButton
        GDR_EXT_QUICKSLOT_SLOT_END = 20, // CIFStatic
        GDR_EXT_QUICKSLOT_SLOT_BOARD_1 = 30, // CIFStatic
        GDR_EXT_QUICKSLOT_SLOT_BOARD_2 = 31, // CIFStatic
        GDR_EXT_QUICKSLOT_SLOT_BOARD_3 = 32, // CIFStatic
        GDR_EXT_QUICKSLOT_SLOT_BOARD_4 = 33, // CIFStatic
        GDR_EXT_QUICKSLOT_SLOT_BOARD_5 = 34, // CIFStatic
        GDR_EXT_QUICKSLOT_SLOT_BOARD_6 = 35, // CIFStatic
        GDR_EXT_QUICKSLOT_SLOT_BOARD_7 = 36, // CIFStatic
        GDR_EXT_QUICKSLOT_SLOT_BOARD_8 = 37, // CIFStatic
        GDR_EXT_QUICKSLOT_SLOT_BOARD_9 = 38, // CIFStatic
        GDR_EXT_QUICKSLOT_SLOT_BOARD_10 = 39, // CIFStatic
        GDR_EXT_QUICKSLOT_SLOT_1 = 100, // CIFExtQuickSlotSlot
        GDR_EXT_QUICKSLOT_SLOT_2 = 101, // CIFExtQuickSlotSlot
        GDR_EXT_QUICKSLOT_SLOT_3 = 102, // CIFExtQuickSlotSlot
        GDR_EXT_QUICKSLOT_SLOT_4 = 103, // CIFExtQuickSlotSlot
        GDR_EXT_QUICKSLOT_SLOT_5 = 104, // CIFExtQuickSlotSlot
        GDR_EXT_QUICKSLOT_SLOT_6 = 105, // CIFExtQuickSlotSlot
        GDR_EXT_QUICKSLOT_SLOT_7 = 106, // CIFExtQuickSlotSlot
        GDR_EXT_QUICKSLOT_SLOT_8 = 107, // CIFExtQuickSlotSlot
        GDR_EXT_QUICKSLOT_SLOT_9 = 108, // CIFExtQuickSlotSlot
        GDR_EXT_QUICKSLOT_SLOT_10 = 109, // CIFExtQuickSlotSlot
    };

public:
    ///\address 0065e870
    CIFExtQuickSlotSecond();

    /// \address 0065ead0
    ~CIFExtQuickSlotSecond();

public:
    /// \address 0065f720
    bool OnCreate(long ln) override;

    void On_BtnClick();
    void On_BtnClickDown();
    void On_BtnClickUp();
    void ResetPosition();

    void UpdateBarStyle(int a);
    void OnUpdate() override;

public:
    /// \address 0065eef0
    void EnsureSlots();

public:
    CIFStatic* buttontile;
    CIFStatic* hotkey;
    CIFButton* button_up;
    CIFButton* button_down;
    CIFButton* OnOffBtn;
    CIFButton* OptBtn;
    CIFButton* HoriBtn;
    CIFButton* VertBtn;
    CIFStatic* End;
    CIFStatic* SlotBoard[10];
    CIFDragableArea* Drag;
    CIFMySlot* myslots[50];
    bool SlotsOpened;
    byte ActivePageNo;
    void UpdateSlots();
};
